/**
 * @file sensor_gateway_parser.c
 * @brief Uses jsmn to parse JSON from AWS that controls gateway functionality
 * and sensor configuration.
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(sensor_gateway_parser);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define JSMN_PARENT_LINKS
#define JSMN_HEADER
#include "jsmn.h"

#include "sensor_cmd.h"
#include "sensor_table.h"
#include "coap_fota_shadow.h"
#include "FrameworkIncludes.h"

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/

#define CHILD_ARRAY_SIZE 3
#define CHILD_ARRAY_INDEX 0
#define ARRAY_NAME_INDEX 1
#define ARRAY_EPOCH_INDEX 2
#define ARRAY_WLIST_INDEX 3
#define JSMN_NO_CHILDREN 0
#define RECORD_TYPE_INDEX 1
#define EVENT_DATA_INDEX 3

#define GATEWAY_TOPIC_SUB_STR "deviceId-"
#define GET_ACCEPTED_SUB_STR "/get/accepted"
#define SENSOR_SHADOW_PREFIX "$aws/things/"

#define MAX_CONVERSION_STR_SIZE 11
#define MAX_CONVERSION_STR_LEN (MAX_CONVERSION_STR_SIZE - 1)

/******************************************************************************/
/* Global                                                                     */
/******************************************************************************/
extern struct k_mutex jsmn_mutex;
extern jsmn_parser jsmn;
extern jsmntok_t tokens[CONFIG_JSMN_NUMBER_OF_TOKENS];

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
static int tokensFound;
static int nextParent;
static int jsonIndex;

static bool getAcceptedTopic;

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
static void JsonParse(const char *pJson);
static bool JsonValid(void);
static void GatewayParser(const char *pTopic, const char *pJson);
static void SensorParser(const char *pTopic, const char *pJson);
static void SensorDeltaParser(const char *pTopic, const char *pJson);
static void SensorEventLogParser(const char *pTopic, const char *pJson);
static void ParseEventArray(const char *pTopic, const char *pJson);
static void FotaParser(const char *pTopic, const char *pJson,
		       enum fota_image_type Type);
static void FotaHostParser(const char *pTopic, const char *pJson);
static void FotaBlockSizeParser(const char *pTopic, const char *pJson);
static void UnsubscribeToGetAcceptedHandler(void);

static int FindType(const char *pJson, const char *s, jsmntype_t Type,
		    int Parent);

static void ParseArray(const char *pJson, int ExpectedSensors);

static jsmntok_t *FindState(const char *pJson);
static bool FindConfigVersion(const char *pJson, uint32_t *pVersion);
static uint32_t ConvertUint(const char *pJson, int Index);
static uint32_t ConvertHex(const char *pJson, int Index);

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
void SensorGatewayParser(const char *pTopic, const char *pJson)
{
	k_mutex_lock(&jsmn_mutex, K_FOREVER);

	JsonParse(pJson);
	if (!JsonValid()) {
		LOG_ERR("Unable to parse subscription %d", tokensFound);
		return;
	}

	getAcceptedTopic = strstr(pTopic, GET_ACCEPTED_SUB_STR) != NULL;
	if (strstr(pTopic, GATEWAY_TOPIC_SUB_STR) != NULL) {
		GatewayParser(pTopic, pJson);
		FotaParser(pTopic, pJson, APP_IMAGE_TYPE);
		FotaParser(pTopic, pJson, MODEM_IMAGE_TYPE);
		FotaHostParser(pTopic, pJson);
		FotaBlockSizeParser(pTopic, pJson);
		UnsubscribeToGetAcceptedHandler();
	} else {
		SensorParser(pTopic, pJson);
	}

	k_mutex_unlock(&jsmn_mutex);
}

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
static void JsonParse(const char *pJson)
{
	jsmn_init(&jsmn);
	/* Strip off metadata because it is too much to process. */
	/* This assumes order to the JSON ... */
	char *ignore = strstr(pJson, ",\"metadata\":");
	if (ignore != NULL) {
		*ignore = '}';
		*(ignore + 1) = 0;
	}
	tokensFound = jsmn_parse(&jsmn, pJson, strlen(pJson), tokens,
				 CONFIG_JSMN_NUMBER_OF_TOKENS);

	if (tokensFound < 0) {
		LOG_ERR("jsmn status: %d", tokensFound);
	} else {
		LOG_DBG("jsmn tokens required: %d", tokensFound);
	}
}

/* Check that there were enough tokens to parse string.
 * After parsing the first thing should be the JSON object { }. */
static bool JsonValid(void)
{
	return ((tokensFound > 0) && (tokens[0].type == JSMN_OBJECT));
}

/**
 * @brief Process $aws/things/deviceId-X/shadow/update/accepted to find sensors
 * that need to be added/removed.
 *
 * Process $aws/things/deviceId-%s/shadow/get/accepted to get the list of
 * sensors when the Pinnacle has reset.
 *
 * @note This function assumes that the AWS task acknowledges the publish so
 * that it isn't repeatedly sent to the gateway.
 */
static void GatewayParser(const char *pTopic, const char *pJson)
{
	jsonIndex = 1;
	nextParent = 0;
	/* Now try to find {"state": {"bt510": {"sensors": */
	FindType(pJson, "state", JSMN_OBJECT, nextParent);
	if (getAcceptedTopic) {
		/* Add to heirarchy {"state":{"reported": ... */
		FindType(pJson, "reported", JSMN_OBJECT, nextParent);
	}
	FindType(pJson, "bt510", JSMN_OBJECT, nextParent);
	FindType(pJson, "sensors", JSMN_ARRAY, nextParent);

	if (jsonIndex != 0) {
		/* Backup one token to get the number of arrays (sensors). */
		int expectedSensors = tokens[jsonIndex - 1].size;
		ParseArray(pJson, expectedSensors);
	} else {
		LOG_DBG("Did not find sensor array");
		/* It is okay for the list to be empty or non-existant.
		 * When rebooting after talking to sensors - then it
		 * shouldn't be.
		 */
	}
}

static void UnsubscribeToGetAcceptedHandler(void)
{
	/* Once this has been processed (after reset) we can unsubscribe. */
	if (getAcceptedTopic) {
		FRAMEWORK_MSG_CREATE_AND_SEND(FWK_ID_CLOUD, FWK_ID_CLOUD,
					      FMC_AWS_GET_ACCEPTED_RECEIVED);
	}
}

static void FotaParser(const char *pTopic, const char *pJson,
		       enum fota_image_type Type)
{
	UNUSED_PARAMETER(pTopic);

	jsonIndex = 1;
	nextParent = 0;
	/* Try to find "state":{"app":{"desired":"2.1.0","switchover":10}} */
	FindType(pJson, "state", JSMN_OBJECT, nextParent);
	if (getAcceptedTopic) {
		FindType(pJson, "reported", JSMN_OBJECT, nextParent);
	}
	FindType(pJson, coap_fota_get_image_name(Type), JSMN_OBJECT,
		 nextParent);

	if (jsonIndex != 0) {
		int savedIndex = jsonIndex;
		int savedParent = nextParent;
		int location = 0;

		location = FindType(pJson, SHADOW_FOTA_DESIRED_STR, JSMN_STRING,
				    nextParent);
		if (location > 0) {
			jsmntok_t *tok = &tokens[location + 1];
			coap_fota_set_desired_version(Type, &pJson[tok->start],
						      (tok->end - tok->start));
		}

		jsonIndex = savedIndex;
		nextParent = savedParent;
		location = FindType(pJson, SHADOW_FOTA_DESIRED_FILENAME_STR,
				    JSMN_STRING, nextParent);
		if (location > 0) {
			jsmntok_t *tok = &tokens[location + 1];
			coap_fota_set_desired_filename(Type, &pJson[tok->start],
						       (tok->end - tok->start));
		}

		jsonIndex = savedIndex;
		nextParent = savedParent;
		location = FindType(pJson, SHADOW_FOTA_SWITCHOVER_STR,
				    JSMN_PRIMITIVE, nextParent);
		if (location > 0) {
			coap_fota_set_switchover(
				Type, ConvertUint(pJson, location + 1));
		}

		jsonIndex = savedIndex;
		nextParent = savedParent;
		location = FindType(pJson, SHADOW_FOTA_START_STR,
				    JSMN_PRIMITIVE, nextParent);
		if (location > 0) {
			coap_fota_set_start(Type,
					    ConvertUint(pJson, location + 1));
		}

		jsonIndex = savedIndex;
		nextParent = savedParent;
		location = FindType(pJson, SHADOW_FOTA_ERROR_STR,
				    JSMN_PRIMITIVE, nextParent);
		if (location > 0) {
			coap_fota_set_error_count(
				Type, ConvertUint(pJson, location + 1));
		}
	}
}

static void FotaHostParser(const char *pTopic, const char *pJson)
{
	UNUSED_PARAMETER(pTopic);

	jsonIndex = 1;
	nextParent = 0;
	/* Try to find "state":{"fwBridge":"something.com"}} */
	FindType(pJson, "state", JSMN_OBJECT, nextParent);
	if (getAcceptedTopic) {
		FindType(pJson, "reported", JSMN_OBJECT, nextParent);
	}
	int location = FindType(pJson, SHADOW_FOTA_BRIDGE_STR, JSMN_STRING,
				nextParent);
	if (location > 0) {
		jsmntok_t *tok = &tokens[location + 1];
		coap_fota_set_host(&pJson[tok->start], (tok->end - tok->start));
	}
}

static void FotaBlockSizeParser(const char *pTopic, const char *pJson)
{
	UNUSED_PARAMETER(pTopic);

	jsonIndex = 1;
	nextParent = 0;
	FindType(pJson, "state", JSMN_OBJECT, nextParent);
	if (getAcceptedTopic) {
		FindType(pJson, "reported", JSMN_OBJECT, nextParent);
	}
	int location = FindType(pJson, SHADOW_FOTA_BLOCKSIZE_STR,
				JSMN_PRIMITIVE, nextParent);
	if (location > 0) {
		coap_fota_set_blocksize(ConvertUint(pJson, location + 1));
	}
}

static void SensorParser(const char *pTopic, const char *pJson)
{
	if (getAcceptedTopic) {
		SensorEventLogParser(pTopic, pJson);
	} else {
		SensorDeltaParser(pTopic, pJson);
	}
}

static void SensorDeltaParser(const char *pTopic, const char *pJson)
{
	uint32_t version = 0;
	jsmntok_t *pState = FindState(pJson);
	if (!FindConfigVersion(pJson, &version) || pState == NULL) {
		return;
	}

	/* The state object contains a string of the values that need to be set. */
	size_t stateLength = pState->end - pState->start;
	size_t bufSize = stateLength + strlen(SENSOR_CMD_SET_PREFIX) +
			 strlen(SENSOR_CMD_SUFFIX) + 1;

	SensorCmdMsg_t *pMsg =
		BufferPool_Take(FWK_BUFFER_MSG_SIZE(SensorCmdMsg_t, bufSize));
	if (pMsg != NULL) {
		pMsg->header.msgCode = FMC_CONFIG_REQUEST;
		pMsg->header.txId = FWK_ID_CLOUD;
		pMsg->header.rxId = FWK_ID_SENSOR_TASK;
		pMsg->size = bufSize;
		pMsg->length = (bufSize > 0) ? (bufSize - 1) : 0;

		/* The version in the delta document changes anytime a publsh occurs,
		 * so use a CRC to filter out duplicates. */
		pMsg->configVersion = version;

		memcpy(pMsg->addrString, pTopic + strlen(SENSOR_SHADOW_PREFIX),
		       SENSOR_ADDR_STR_LEN);

		/* Format AWS data into a JSON-RPC set command */
		strcat(pMsg->cmd, SENSOR_CMD_SET_PREFIX);
		/* JSON string isn't null terminated */
		strncat(pMsg->cmd, &pJson[pState->start], stateLength);
		strcat(pMsg->cmd, SENSOR_CMD_SUFFIX);
		FRAMEWORK_DEBUG_ASSERT(strlen(pMsg->cmd) == bufSize - 1);
		FRAMEWORK_MSG_SEND(pMsg);
	}
}

static void SensorEventLogParser(const char *pTopic, const char *pJson)
{
	jsonIndex = 1;
	nextParent = 0;
	/* Now try to find {"state":{"reported": ... "eventLog":
	 * Parents are required because shadow contains timestamps
	 * ("eventLog" wont be unique). */
	FindType(pJson, "state", JSMN_OBJECT, nextParent);
	FindType(pJson, "reported", JSMN_OBJECT, nextParent);
	FindType(pJson, "eventLog", JSMN_ARRAY, nextParent);

	ParseEventArray(pTopic, pJson);
}

/**
 * @brief This function updates the global index to the next token when an
 * item + type is found.  Otherwise, the index is set to zero.
 *
 * @retval > 0 then the item was found at this index
 * @retval <= 0, then the item was not found
 */
static int FindType(const char *pJson, const char *s, jsmntype_t Type,
		    int Parent)
{
	if (jsonIndex == 0) {
		return 0;
	}

	/* Analyze a pair of tokens of the form <string>, <type> */
	size_t i = jsonIndex;
	jsonIndex = 0;
	for (; ((i + 1) < tokensFound); i++) {
		int length = tokens[i].end - tokens[i].start;
		if ((tokens[i].type == JSMN_STRING) &&
		    ((int)strlen(s) == length) &&
		    (strncmp(pJson + tokens[i].start, s, length) == 0) &&
		    (tokens[i + 1].type == Type) &&
		    ((Parent == 0) || (tokens[i].parent == Parent))) {
			LOG_DBG("Found '%s' at index %d with parent %d", s, i,
				tokens[i].parent);
			nextParent = i + 1;
			jsonIndex = i + 2;
			break;
		}
	}
	return (jsonIndex - 2);
}

/**
 * @brief Parse the elements in the anonymous array into a c-structure.
 * (Zephyr library can't handle anonymous arrays.)
 * ["addrString", epoch, whitelist (boolean)]
 * The epoch isn't used.
 */
static void ParseArray(const char *pJson, int ExpectedSensors)
{
	if (jsonIndex == 0) {
		return;
	}

	SensorWhitelistMsg_t *pMsg =
		BufferPool_Take(sizeof(SensorWhitelistMsg_t));
	if (pMsg == NULL) {
		return;
	}

	size_t maxSensors = MIN(ExpectedSensors, CONFIG_SENSOR_TABLE_SIZE);
	int sensorsFound = 0;
	size_t i = jsonIndex;
	while (((i + CHILD_ARRAY_SIZE) < tokensFound) &&
	       (sensorsFound < maxSensors)) {
		int addrLength = tokens[i].end - tokens[i].start;
		if ((tokens[i + CHILD_ARRAY_INDEX].type == JSMN_ARRAY) &&
		    (tokens[i + CHILD_ARRAY_INDEX].size == CHILD_ARRAY_SIZE) &&
		    (tokens[i + ARRAY_NAME_INDEX].type == JSMN_STRING) &&
		    (tokens[i + ARRAY_NAME_INDEX].size == JSMN_NO_CHILDREN) &&
		    (tokens[i + ARRAY_EPOCH_INDEX].type == JSMN_PRIMITIVE) &&
		    (tokens[i + ARRAY_EPOCH_INDEX].size == JSMN_NO_CHILDREN) &&
		    (tokens[i + ARRAY_WLIST_INDEX].type == JSMN_PRIMITIVE) &&
		    (tokens[i + ARRAY_WLIST_INDEX].size == JSMN_NO_CHILDREN)) {
			LOG_DBG("Found array at %d", i);
			strncpy(pMsg->sensors[sensorsFound].addrString,
				&pJson[tokens[i + ARRAY_NAME_INDEX].start],
				MIN(addrLength, SENSOR_ADDR_STR_LEN));
			/* The 't' in true is used to determine true/false.
			 * This is safe because primitives are
			 * numbers, true, false, and null. */
			pMsg->sensors[sensorsFound].whitelist =
				(pJson[tokens[i + ARRAY_WLIST_INDEX].start] ==
				 't');
			sensorsFound += 1;
			i += CHILD_ARRAY_SIZE + 1;
		} else {
			LOG_ERR("Gateway Shadow parsing error");
			break;
		}
	}

	pMsg->header.msgCode = FMC_WHITELIST_REQUEST;
	pMsg->header.rxId = FWK_ID_SENSOR_TASK;
	pMsg->sensorCount = sensorsFound;
	FRAMEWORK_MSG_SEND(pMsg);

	LOG_INF("Processed %d of %d sensors in desired list from AWS",
		sensorsFound, ExpectedSensors);
}

static void ParseEventArray(const char *pTopic, const char *pJson)
{
	SensorShadowInitMsg_t *pMsg =
		BufferPool_Take(sizeof(SensorShadowInitMsg_t));
	if (pMsg == NULL) {
		return;
	}

	/* If the event log isn't found a message still needs to be sent. */
	int expectedLogs = tokens[jsonIndex - 1].size;
	int maxLogs = 0;
	if (jsonIndex == 0) {
		LOG_DBG("Could not find event log");
	} else {
		maxLogs = MIN(expectedLogs, CONFIG_SENSOR_LOG_MAX_SIZE);
	}

	/* 1st and 3rd items are hex. {"eventLog":[["01",466280,"0899"]] */
	size_t i = jsonIndex;
	size_t j = 0;
	while (((i + CHILD_ARRAY_SIZE) < tokensFound) && (j < maxLogs)) {
		if ((tokens[i + CHILD_ARRAY_INDEX].type == JSMN_ARRAY) &&
		    (tokens[i + CHILD_ARRAY_INDEX].size == CHILD_ARRAY_SIZE) &&
		    (tokens[i + RECORD_TYPE_INDEX].type == JSMN_STRING) &&
		    (tokens[i + RECORD_TYPE_INDEX].size == JSMN_NO_CHILDREN) &&
		    (tokens[i + ARRAY_EPOCH_INDEX].type == JSMN_PRIMITIVE) &&
		    (tokens[i + ARRAY_EPOCH_INDEX].size == JSMN_NO_CHILDREN) &&
		    (tokens[i + EVENT_DATA_INDEX].type == JSMN_STRING) &&
		    (tokens[i + EVENT_DATA_INDEX].size == JSMN_NO_CHILDREN)) {
			LOG_DBG("Found array at %d", i);
			pMsg->events[j].recordType =
				ConvertHex(pJson, i + RECORD_TYPE_INDEX);
			pMsg->events[j].epoch =
				ConvertUint(pJson, i + ARRAY_EPOCH_INDEX);
			pMsg->events[j].data =
				ConvertHex(pJson, i + EVENT_DATA_INDEX);
			LOG_DBG("%u %x,%d,%x", j, pMsg->events[j].recordType,
				pMsg->events[j].epoch, pMsg->events[j].data);
			j += 1;
			i += CHILD_ARRAY_SIZE + 1;
		} else {
			LOG_ERR("Sensor shadow event log parsing error");
			break;
		}
	}

	pMsg->eventCount = j;
	memcpy(pMsg->addrString, pTopic + strlen(SENSOR_SHADOW_PREFIX),
	       SENSOR_ADDR_STR_LEN);
	pMsg->header.msgCode = FMC_SENSOR_SHADOW_INIT;
	pMsg->header.rxId = FWK_ID_SENSOR_TASK;
	LOG_INF("Processed %d of %d sensor events in shadow", pMsg->eventCount,
		expectedLogs);
	FRAMEWORK_MSG_SEND(pMsg);
}

/**
 * @note sets jsonIndex to 1
 *
 * @retval NULL if not found
 * @retval pointer to token if found
 */
static jsmntok_t *FindState(const char *pJson)
{
	jsmntok_t *result = NULL;
	jsonIndex = 1;
	int stateLocation = FindType(pJson, "state", JSMN_OBJECT, 0);
	if (stateLocation > 0) {
		stateLocation += 1;
		result = &tokens[stateLocation];
	}
	return result;
}

/**
 * @retval true if version found, otherwise false
 * @note sets jsonIndex to 1
 */
static bool FindConfigVersion(const char *pJson, uint32_t *pVersion)
{
	jsonIndex = 1;
	int versionLocation =
		FindType(pJson, "configVersion", JSMN_PRIMITIVE, 0);
	if (versionLocation > 0) {
		versionLocation += 1; /* index of data */
		*pVersion = ConvertUint(pJson, versionLocation);
		return true;
	}
	*pVersion = 0;
	return false;
}

static uint32_t ConvertUint(const char *pJson, int Index)
{
	char str[MAX_CONVERSION_STR_SIZE];
	int length = tokens[Index].end - tokens[Index].start;
	memset(str, 0, sizeof(str));
	memcpy(str, &pJson[tokens[Index].start], length);
	return strtoul(str, NULL, 10);
}

static uint32_t ConvertHex(const char *pJson, int Index)
{
	char str[MAX_CONVERSION_STR_SIZE];
	int length = tokens[Index].end - tokens[Index].start;
	memset(str, 0, sizeof(str));
	memcpy(str, &pJson[tokens[Index].start], length);
	return strtoul(str, NULL, 16);
}
