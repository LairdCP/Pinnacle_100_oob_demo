/**
 * @file nv.c
 * @brief Non-volatile storage for the app
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(oob_nv);

#define NV_LOG_ERR(...) LOG_ERR(__VA_ARGS__)
#define NV_LOG_WRN(...) LOG_WRN(__VA_ARGS__)
#define NV_LOG_INF(...) LOG_INF(__VA_ARGS__)
#define NV_LOG_DBG(...) LOG_DBG(__VA_ARGS__)

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr.h>
#include <device.h>
#include <drivers/flash.h>
#include <storage/flash_map.h>
#include <fs/nvs.h>

#include "nv.h"

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
enum SETTING_ID {
	SETTING_ID_COMMISSIONED,
	SETTING_ID_DEV_CERT,
	SETTING_ID_DEV_KEY,
	SETTING_ID_AWS_ENDPOINT,
	SETTING_ID_AWS_CLIENT_ID,
	SETTING_ID_AWS_ROOT_CA,
	SETTING_ID_LWM2M_CONFIG
};

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
static struct nvs_fs fs;
static bool nvCommissioned;

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
int nvReadCommissioned(bool *commissioned)
{
	int rc;
	*commissioned = nvCommissioned;
	rc = nvs_read(&fs, SETTING_ID_COMMISSIONED, &nvCommissioned,
		      sizeof(nvCommissioned));
	if (rc <= 0) {
		*commissioned = false;
	}

	return rc;
}

int nvStoreCommissioned(bool commissioned)
{
	int rc;
	nvCommissioned = commissioned;
	rc = nvs_write(&fs, SETTING_ID_COMMISSIONED, &nvCommissioned,
		       sizeof(nvCommissioned));
	if (rc < 0) {
		NV_LOG_ERR("Error writing commissioned (%d)", rc);
	}

	return rc;
}

int nvInit(void)
{
	int rc = 0;
	struct flash_pages_info info;

	/* define the nvs file system by settings with:
	 *	sector_size equal to the pagesize,
	 *	starting at FLASH_AREA_OFFSET(storage)
	 */
	fs.offset = FLASH_AREA_OFFSET(storage);
	rc = flash_get_page_info_by_offs(device_get_binding(NV_FLASH_DEVICE),
					 fs.offset, &info);
	if (rc) {
		NV_LOG_ERR("Unable to get page info (%d)", rc);
		goto exit;
	}
	fs.sector_size = info.size;
	fs.sector_count = NUM_FLASH_SECTORS;

	rc = nvs_init(&fs, NV_FLASH_DEVICE);
	if (rc) {
		NV_LOG_ERR("Flash Init failed (%d)", rc);
		goto exit;
	}

	NV_LOG_INF("Free space in NV: %d", nvs_calc_free_space(&fs));

	rc = nvReadCommissioned(&nvCommissioned);
	if (rc <= 0) {
		/* setting not found, init it */
		rc = nvStoreCommissioned(false);
		if (rc <= 0) {
			NV_LOG_ERR("Could not write commissioned flag (%d)",
				   rc);
			goto exit;
		}
	}

exit:
	return rc;
}

int nvStoreDevCert(uint8_t *cert, uint16_t size)
{
	return nvs_write(&fs, SETTING_ID_DEV_CERT, cert, size);
}

int nvStoreDevKey(uint8_t *key, uint16_t size)
{
	return nvs_write(&fs, SETTING_ID_DEV_KEY, key, size);
}

int nvReadDevCert(uint8_t *cert, uint16_t size)
{
	return nvs_read(&fs, SETTING_ID_DEV_CERT, cert, size);
}

int nvReadDevKey(uint8_t *key, uint16_t size)
{
	return nvs_read(&fs, SETTING_ID_DEV_KEY, key, size);
}

int nvDeleteDevCert(void)
{
	return nvs_delete(&fs, SETTING_ID_DEV_CERT);
}

int nvDeleteDevKey(void)
{
	return nvs_delete(&fs, SETTING_ID_DEV_KEY);
}

int nvStoreAwsEndpoint(uint8_t *ep, uint16_t size)
{
	return nvs_write(&fs, SETTING_ID_AWS_ENDPOINT, ep, size);
}

int nvReadAwsEndpoint(uint8_t *ep, uint16_t size)
{
	return nvs_read(&fs, SETTING_ID_AWS_ENDPOINT, ep, size);
}

int nvStoreAwsClientId(uint8_t *id, uint16_t size)
{
	return nvs_write(&fs, SETTING_ID_AWS_CLIENT_ID, id, size);
}

int nvReadAwsClientId(uint8_t *id, uint16_t size)
{
	return nvs_read(&fs, SETTING_ID_AWS_CLIENT_ID, id, size);
}

int nvStoreAwsRootCa(uint8_t *cert, uint16_t size)
{
	return nvs_write(&fs, SETTING_ID_AWS_ROOT_CA, cert, size);
}

int nvReadAwsRootCa(uint8_t *cert, uint16_t size)
{
	return nvs_read(&fs, SETTING_ID_AWS_ROOT_CA, cert, size);
}

int nvDeleteAwsEndpoint(void)
{
	return nvs_delete(&fs, SETTING_ID_AWS_ENDPOINT);
}

int nvDeleteAwsClientId(void)
{
	return nvs_delete(&fs, SETTING_ID_AWS_CLIENT_ID);
}

int nvDeleteAwsRootCa(void)
{
	return nvs_delete(&fs, SETTING_ID_AWS_ROOT_CA);
}

int nvInitLwm2mConfig(void *data, void *init_value, uint16_t size)
{
	int rc = nvs_read(&fs, SETTING_ID_LWM2M_CONFIG, data, size);
	if (rc != size) {
		memcpy(data, init_value, size);
		rc = nvs_write(&fs, SETTING_ID_LWM2M_CONFIG, data, size);
		if (rc != size) {
			NV_LOG_ERR("Error writing LWM2M config (%d)", rc);
		}
	}
	return rc;
}

int nvWriteLwm2mConfig(void *data, uint16_t size)
{
	return nvs_write(&fs, SETTING_ID_LWM2M_CONFIG, data, size);
}
