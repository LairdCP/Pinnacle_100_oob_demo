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

#include "oob_common.h"
#include "ble_cellular_service.h"

#define CELL_SVC_LOG_ERR(...) LOG_ERR(__VA_ARGS__)
#define CELL_SVC_LOG_WRN(...) LOG_WRN(__VA_ARGS__)
#define CELL_SVC_LOG_INF(...) LOG_INF(__VA_ARGS__)
#define CELL_SVC_LOG_DBG(...) LOG_DBG(__VA_ARGS__)

static struct bt_uuid_128 cell_svc_uuid =
	BT_UUID_INIT_128(0x36, 0xa3, 0x4d, 0x40, 0xb6, 0x70, 0x69, 0xa6, 0xb1,
			 0x4e, 0x84, 0x9e, 0x60, 0x7c, 0x78, 0x43);

static struct bt_uuid_128 cell_imei_uuid =
	BT_UUID_INIT_128(0x36, 0xa3, 0x4d, 0x40, 0xb6, 0x70, 0x69, 0xa6, 0xb1,
			 0x4e, 0x84, 0x9e, 0x61, 0x7c, 0x78, 0x43);

static struct bt_uuid_128 apn_uuid =
	BT_UUID_INIT_128(0x36, 0xa3, 0x4d, 0x40, 0xb6, 0x70, 0x69, 0xa6, 0xb1,
			 0x4e, 0x84, 0x9e, 0x62, 0x7c, 0x78, 0x43);

static struct bt_uuid_128 apn_username_uuid =
	BT_UUID_INIT_128(0x36, 0xa3, 0x4d, 0x40, 0xb6, 0x70, 0x69, 0xa6, 0xb1,
			 0x4e, 0x84, 0x9e, 0x63, 0x7c, 0x78, 0x43);

static struct bt_uuid_128 apn_password_uuid =
	BT_UUID_INIT_128(0x36, 0xa3, 0x4d, 0x40, 0xb6, 0x70, 0x69, 0xa6, 0xb1,
			 0x4e, 0x84, 0x9e, 0x64, 0x7c, 0x78, 0x43);

static struct bt_uuid_128 cell_status_uuid =
	BT_UUID_INIT_128(0x36, 0xa3, 0x4d, 0x40, 0xb6, 0x70, 0x69, 0xa6, 0xb1,
			 0x4e, 0x84, 0x9e, 0x65, 0x7c, 0x78, 0x43);

static struct bt_uuid_128 cell_fw_ver_uuid =
	BT_UUID_INIT_128(0x36, 0xa3, 0x4d, 0x40, 0xb6, 0x70, 0x69, 0xa6, 0xb1,
			 0x4e, 0x84, 0x9e, 0x66, 0x7c, 0x78, 0x43);

static char imei_value[CELL_SVC_IMEI_LENGTH + 1];
static char apn_value[CELL_SVC_APN_MAX_LENGTH + 1];
static char apn_username_value[CELL_SVC_APN_USERNAME_MAX_LENGTH + 1];
static char apn_password_value[CELL_SVC_APN_PASSWORD_MAX_LENGTH + 1];

static struct bt_gatt_ccc_cfg status_ccc_cfg[BT_GATT_CCC_MAX] = {};
static u8_t status_notify;
static enum cell_status status_value;

static char fw_ver_value[CELL_SVC_LTE_FW_VER_LENGTH_MAX + 1];

static ssize_t read_imei(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, u16_t len, u16_t offset)
{
	u16_t valueLen;

	const char *value = attr->user_data;
	valueLen = strlen(value);
	if (valueLen > CELL_SVC_IMEI_LENGTH) {
		valueLen = CELL_SVC_IMEI_LENGTH;
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, valueLen);
}

static ssize_t read_apn(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, u16_t len, u16_t offset)
{
	u16_t valueLen;

	const char *value = attr->user_data;
	valueLen = strlen(value);
	if (valueLen > CELL_SVC_APN_MAX_LENGTH) {
		valueLen = CELL_SVC_APN_MAX_LENGTH;
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, valueLen);
}

static ssize_t write_apn(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, u16_t len, u16_t offset, u8_t flags)
{
	u8_t *value = attr->user_data;

	if (offset + len > (sizeof(apn_value) - 1)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);
	/* null terminate the value that was written */
	*(value + offset + len) = 0;

	return len;
}

static ssize_t read_apn_username(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr, void *buf,
				 u16_t len, u16_t offset)
{
	u16_t valueLen;

	const char *value = attr->user_data;
	valueLen = strlen(value);
	if (valueLen > CELL_SVC_APN_USERNAME_MAX_LENGTH) {
		valueLen = CELL_SVC_APN_USERNAME_MAX_LENGTH;
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, valueLen);
}

static ssize_t write_apn_username(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  const void *buf, u16_t len, u16_t offset,
				  u8_t flags)
{
	u8_t *value = attr->user_data;

	if (offset + len > (sizeof(apn_username_value) - 1)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);
	/* null terminate the value that was written */
	*(value + offset + len) = 0;

	return len;
}

static ssize_t read_apn_password(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr, void *buf,
				 u16_t len, u16_t offset)
{
	u16_t valueLen;

	const char *value = attr->user_data;
	valueLen = strlen(value);
	if (valueLen > CELL_SVC_APN_PASSWORD_MAX_LENGTH) {
		valueLen = CELL_SVC_APN_PASSWORD_MAX_LENGTH;
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, valueLen);
}

static ssize_t write_apn_password(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  const void *buf, u16_t len, u16_t offset,
				  u8_t flags)
{
	u8_t *value = attr->user_data;

	if (offset + len > (sizeof(apn_password_value) - 1)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);
	/* null terminate the value that was written */
	*(value + offset + len) = 0;

	return len;
}

static ssize_t read_status(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   u16_t len, u16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(status_value));
}

static void status_cfg_changed(const struct bt_gatt_attr *attr, u16_t value)
{
	status_notify = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static ssize_t read_fw_ver(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   u16_t len, u16_t offset)
{
	u16_t valueLen;

	const char *value = attr->user_data;
	valueLen = strlen(value);
	if (valueLen > CELL_SVC_LTE_FW_VER_LENGTH_MAX) {
		valueLen = CELL_SVC_LTE_FW_VER_LENGTH_MAX;
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, valueLen);
}

/* Cellular Service Declaration */
#define CELL_SVC_STATUS_ATT_INDEX 9
BT_GATT_SERVICE_DEFINE(
	cell_svc, BT_GATT_PRIMARY_SERVICE(&cell_svc_uuid),
	BT_GATT_CHARACTERISTIC(&cell_imei_uuid.uuid, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_imei, NULL, imei_value),
	BT_GATT_CHARACTERISTIC(&apn_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, read_apn,
			       write_apn, apn_value),
	BT_GATT_CHARACTERISTIC(&apn_username_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_apn_username, write_apn_username,
			       apn_username_value),
	BT_GATT_CHARACTERISTIC(&apn_password_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_apn_password, write_apn_password,
			       apn_password_value),
	BT_GATT_CHARACTERISTIC(&cell_status_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, read_status, NULL,
			       &status_value),
	BT_GATT_CCC(status_ccc_cfg, status_cfg_changed),
	BT_GATT_CHARACTERISTIC(&cell_fw_ver_uuid.uuid, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_fw_ver, NULL,
			       fw_ver_value), );

void cell_svc_set_imei(const char *imei)
{
	if (imei) {
		memcpy(imei_value, imei, sizeof(imei_value) - 1);
	}
}

void cell_svc_set_status(struct bt_conn *conn, enum cell_status status)
{
	bool notify = false;

	if (status != status_value) {
		notify = true;
		status_value = status;
	}

	if ((conn != NULL) && status_notify && notify) {
		bt_gatt_notify(conn, &cell_svc.attrs[CELL_SVC_STATUS_ATT_INDEX], &status,
			       sizeof(status));
	}
}

void cell_svc_set_fw_ver(const char *ver)
{
	memcpy(fw_ver_value, ver, sizeof(fw_ver_value) - 1);
}