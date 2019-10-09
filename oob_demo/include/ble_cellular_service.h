/*
 * Copyright (c) 2019 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BLE_CELLULAR_SERVICE_H__
#define __BLE_CELLULAR_SERVICE_H__

#define CELL_SVC_IMEI_LENGTH 15
#define CELL_SVC_LTE_FW_VER_LENGTH_MAX 28
#define CELL_SVC_APN_MAX_LENGTH 63
#define CELL_SVC_APN_USERNAME_MAX_LENGTH 24
#define CELL_SVC_APN_PASSWORD_MAX_LENGTH 24

enum cell_status { CELL_STATUS_DISCONNECTED, CELL_STATUS_CONNECTED };

void cell_svc_set_imei(const char *imei);
void cell_svc_set_status(struct bt_conn *conn, enum cell_status status);
void cell_svc_set_fw_ver(const char *ver);

#endif