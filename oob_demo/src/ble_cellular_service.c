/* ble_cellular_service.c - BLE Cellular Service
 *
 * Copyright (c) 2019 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(oob_cell_svc);

#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/bluetooth.h>

#include "oob_common.h"
#include "laird_bluetooth.h"
#include "ble_cellular_service.h"

#define CELL_SVC_LOG_ERR(...) LOG_ERR(__VA_ARGS__)
#define CELL_SVC_LOG_WRN(...) LOG_WRN(__VA_ARGS__)
#define CELL_SVC_LOG_INF(...) LOG_INF(__VA_ARGS__)
#define CELL_SVC_LOG_DBG(...) LOG_DBG(__VA_ARGS__)

#define CELL_SVC_BASE_UUID_128(_x_)                                            \
	BT_UUID_INIT_128(0x36, 0xa3, 0x4d, 0x40, 0xb6, 0x70, 0x69, 0xa6, 0xb1, \
			 0x4e, 0x84, 0x9e, LSB_16(_x_), MSB_16(_x_), 0x78,     \
			 0x43)

static struct bt_uuid_128 CELL_SVC_UUID = CELL_SVC_BASE_UUID_128(0x7c60);
static struct bt_uuid_128 IMEI_UUID = CELL_SVC_BASE_UUID_128(0x7c61);
static struct bt_uuid_128 APN_UUID = CELL_SVC_BASE_UUID_128(0x7c62);
static struct bt_uuid_128 APN_USERNAME_UUID = CELL_SVC_BASE_UUID_128(0x7c63);
static struct bt_uuid_128 APN_PASSWORD_UUID = CELL_SVC_BASE_UUID_128(0x7c64);
static struct bt_uuid_128 NETWORK_STATE_UUID = CELL_SVC_BASE_UUID_128(0x7c65);
static struct bt_uuid_128 FW_VERSION_UUID = CELL_SVC_BASE_UUID_128(0x7c66);
static struct bt_uuid_128 STARTUP_STATE_UUID = CELL_SVC_BASE_UUID_128(0x7c67);
static struct bt_uuid_128 RSSI_UUID = CELL_SVC_BASE_UUID_128(0x7c68);
static struct bt_uuid_128 SINR_UUID = CELL_SVC_BASE_UUID_128(0x7c69);

struct ble_cellular_service {
	char imei_value[MDM_HL7800_IMEI_SIZE];
	struct mdm_hl7800_apn apn;
	char network_state[MDM_HL7800_MAX_NETWORK_STATE_SIZE];
	char fw_ver_value[CELL_SVC_LTE_FW_VER_LENGTH_MAX + 1];
	char startup_state[MDM_HL7800_MAX_STARTUP_STATE_SIZE];
	s32_t rssi;
	s32_t sinr;
};

struct ccc_table {
	struct lbt_ccc_element apn_value;
	struct lbt_ccc_element apn_username;
	struct lbt_ccc_element apn_password;
	struct lbt_ccc_element network_state;
	struct lbt_ccc_element startup_state;
	struct lbt_ccc_element rssi;
	struct lbt_ccc_element sinr;
};

static struct ble_cellular_service bcs;
static struct ccc_table ccc;
static struct bt_conn *(*get_connection_handle_fptr)(void);

static ssize_t read_imei(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, u16_t len, u16_t offset)
{
	return lbt_read_string(conn, attr, buf, len, offset,
			       MDM_HL7800_IMEI_STRLEN);
}

static ssize_t read_apn(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, u16_t len, u16_t offset)
{
	return lbt_read_string(conn, attr, buf, len, offset,
			       MDM_HL7800_APN_MAX_STRLEN);
}

static ssize_t update_apn_in_modem(ssize_t length)
{
	if (length > 0) {
		s32_t status = mdm_hl7800_update_apn(&bcs.apn);
		if (status < 0) {
			return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		}
	}
	return length;
}

static ssize_t write_apn(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, u16_t len, u16_t offset, u8_t flags)
{
	ssize_t length = lbt_write_string(conn, attr, buf, len, offset, flags,
					  MDM_HL7800_APN_MAX_STRLEN);

	return update_apn_in_modem(length);
}

static ssize_t read_apn_username(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr, void *buf,
				 u16_t len, u16_t offset)
{
	return lbt_read_string(conn, attr, buf, len, offset,
			       MDM_HL7800_APN_USERNAME_MAX_STRLEN);
}

static ssize_t write_apn_username(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  const void *buf, u16_t len, u16_t offset,
				  u8_t flags)
{
	ssize_t length = lbt_write_string(conn, attr, buf, len, offset, flags,
					  MDM_HL7800_APN_USERNAME_MAX_STRLEN);

	return length; // Don't update APN struct in modem until password is written.
}

static ssize_t read_apn_password(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr, void *buf,
				 u16_t len, u16_t offset)
{
	return lbt_read_string(conn, attr, buf, len, offset,
			       MDM_HL7800_APN_PASSWORD_MAX_STRLEN);
}

static ssize_t write_apn_password(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  const void *buf, u16_t len, u16_t offset,
				  u8_t flags)
{
	ssize_t length = lbt_write_string(conn, attr, buf, len, offset, flags,
					  MDM_HL7800_APN_PASSWORD_MAX_STRLEN);

	return update_apn_in_modem(length);
}

static ssize_t read_network_state(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr, void *buf,
				  u16_t len, u16_t offset)
{
	return lbt_read_string(conn, attr, buf, len, offset,
			       MDM_HL7800_MAX_NETWORK_STATE_STRLEN);
}

static ssize_t read_fw_ver(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   u16_t len, u16_t offset)
{
	return lbt_read_string(conn, attr, buf, len, offset,
			       CELL_SVC_LTE_FW_VER_LENGTH_MAX);
}

static ssize_t read_startup_state(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr, void *buf,
				  u16_t len, u16_t offset)
{
	return lbt_read_string(conn, attr, buf, len, offset,
			       MDM_HL7800_MAX_NETWORK_STATE_STRLEN);
}

/* The stack doesn't populate the handles in the attribute table.  
 * That prevents one possible refactoring.
 */
static void apn_value_ccc_handler(const struct bt_gatt_attr *attr, u16_t value)
{
	ccc.apn_value.notify = IS_NOTIFIABLE(value);
}

static void apn_username_ccc_handler(const struct bt_gatt_attr *attr,
				     u16_t value)
{
	ccc.apn_username.notify = IS_NOTIFIABLE(value);
}

static void apn_password_ccc_handler(const struct bt_gatt_attr *attr,
				     u16_t value)
{
	ccc.apn_password.notify = IS_NOTIFIABLE(value);
}

static void network_state_ccc_handler(const struct bt_gatt_attr *attr,
				      u16_t value)
{
	ccc.network_state.notify = IS_NOTIFIABLE(value);
}

static void startup_state_ccc_handler(const struct bt_gatt_attr *attr,
				      u16_t value)
{
	ccc.startup_state.notify = IS_NOTIFIABLE(value);
}

static void rssi_ccc_handler(const struct bt_gatt_attr *attr, u16_t value)
{
	ccc.rssi.notify = IS_NOTIFIABLE(value);
}

static void sinr_ccc_handler(const struct bt_gatt_attr *attr, u16_t value)
{
	ccc.sinr.notify = IS_NOTIFIABLE(value);
}

/* Cellular Service Declaration */
#define APN_VALUE_INDEX 7
#define APN_USERNAME_INDEX 10
#define APN_PASSWORD_INDEX 13
#define NETWORK_STATE_INDEX 16
#define STARTUP_STATE_INDEX 19
#define RSSI_INDEX 22
#define SINR_INDEX 25

BT_GATT_SERVICE_DEFINE(
	cell_svc, BT_GATT_PRIMARY_SERVICE(&CELL_SVC_UUID),
	BT_GATT_CHARACTERISTIC(&IMEI_UUID.uuid, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_imei, NULL,
			       bcs.imei_value),
	BT_GATT_CHARACTERISTIC(&FW_VERSION_UUID.uuid, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_fw_ver, NULL,
			       bcs.fw_ver_value),
	BT_GATT_CHARACTERISTIC(&APN_UUID.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
				       BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, read_apn,
			       write_apn, bcs.apn.value),
	LBT_GATT_CCC(apn_value),
	BT_GATT_CHARACTERISTIC(&APN_USERNAME_UUID.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
				       BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_apn_username, write_apn_username,
			       bcs.apn.username),
	LBT_GATT_CCC(apn_username),
	BT_GATT_CHARACTERISTIC(&APN_PASSWORD_UUID.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
				       BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_apn_password, write_apn_password,
			       bcs.apn.password),
	LBT_GATT_CCC(apn_password),
	BT_GATT_CHARACTERISTIC(&NETWORK_STATE_UUID.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, read_network_state, NULL,
			       bcs.network_state),
	LBT_GATT_CCC(network_state),
	BT_GATT_CHARACTERISTIC(&STARTUP_STATE_UUID.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, read_startup_state, NULL,
			       bcs.startup_state),
	LBT_GATT_CCC(startup_state),
	BT_GATT_CHARACTERISTIC(&RSSI_UUID.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, lbt_read_integer, NULL,
			       &bcs.rssi),
	LBT_GATT_CCC(rssi),
	BT_GATT_CHARACTERISTIC(&SINR_UUID.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, lbt_read_integer, NULL,
			       &bcs.sinr),
	LBT_GATT_CCC(sinr));

void cell_svc_assign_connection_handler_getter(struct bt_conn *(*function)(void))
{
	get_connection_handle_fptr = function;
}

void cell_svc_set_imei(const char *imei)
{
	if (imei) {
		memcpy(bcs.imei_value, imei, MDM_HL7800_IMEI_STRLEN);
	}
}

static void cell_svc_notify(bool notify, u16_t index, u16_t length)
{
	if (get_connection_handle_fptr == NULL) {
		return;
	}

	struct bt_conn *connection_handle = get_connection_handle_fptr();
	if (connection_handle != NULL) {
		if (notify) {
			bt_gatt_notify(connection_handle,
				       &cell_svc.attrs[index],
				       cell_svc.attrs[index].user_data, length);
		}
	}
}

void cell_svc_set_network_state(char *state)
{
	__ASSERT_NO_MSG(state != NULL);
	strncpy_replace_underscore_with_space(bcs.network_state, state,
					      sizeof(bcs.network_state));
	cell_svc_notify(ccc.network_state.notify, NETWORK_STATE_INDEX,
			strlen(bcs.network_state));
}

void cell_svc_set_startup_state(char *state)
{
	__ASSERT_NO_MSG(state != NULL);
	strncpy_replace_underscore_with_space(bcs.startup_state, state,
					      sizeof(bcs.startup_state));
	cell_svc_notify(ccc.startup_state.notify, STARTUP_STATE_INDEX,
			strlen(bcs.startup_state));
}

void cell_svc_set_apn(struct mdm_hl7800_apn *access_point)
{
	__ASSERT_NO_MSG(apn != NULL);
	memcpy(&bcs.apn, access_point, sizeof(struct mdm_hl7800_apn));
	cell_svc_notify(ccc.apn_value.notify, APN_VALUE_INDEX,
			strlen(bcs.apn.value));
	cell_svc_notify(ccc.apn_username.notify, APN_USERNAME_INDEX,
			strlen(bcs.apn.username));
	cell_svc_notify(ccc.apn_password.notify, APN_PASSWORD_INDEX,
			strlen(bcs.apn.password));
}

void cell_svc_set_rssi(int value)
{
	bcs.rssi = (s32_t)value;
	cell_svc_notify(ccc.rssi.notify, RSSI_INDEX, sizeof(bcs.rssi));
}

void cell_svc_set_sinr(int value)
{
	bcs.sinr = (s32_t)value;
	cell_svc_notify(ccc.sinr.notify, SINR_INDEX, sizeof(bcs.sinr));
}

void cell_svc_set_fw_ver(const char *ver)
{
	__ASSERT_NO_MSG(ver != NULL);
	memcpy(bcs.fw_ver_value, ver, CELL_SVC_LTE_FW_VER_STRLEN_MAX);
}
