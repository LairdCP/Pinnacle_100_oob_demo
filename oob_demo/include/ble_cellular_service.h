/*
 * Copyright (c) 2019 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BLE_CELLULAR_SERVICE_H__
#define __BLE_CELLULAR_SERVICE_H__

#include <drivers/modem/hl7800.h>
#include <bluetooth/conn.h>

/* This comes from the modem context which stores things as a 
 * pointer to a string.  The size here is what the service limits
 * the length to.
 */
#define CELL_SVC_LTE_FW_VER_LENGTH_MAX 28
#define CELL_SVC_LTE_FW_VER_STRLEN_MAX (CELL_SVC_LTE_FW_VER_LENGTH_MAX - 1)

/* This comes from the oob_ble.  It is used to size the BLE characteristic. */
#define MAX_SENSOR_STATE_SIZE (sizeof("CONNECTED_AND_CONFIGURED"))
#define MAX_SENSOR_STATE_STRLEN (MAX_SENSOR_STATE_SIZE - 1)

/** @param function that sensor service should use to get connection handle when
 * determining if a value should by notified.
 */
void cell_svc_assign_connection_handler_getter(struct bt_conn *(*function)(void));

void cell_svc_set_imei(const char *imei);
void cell_svc_set_network_state(char *state);
void cell_svc_set_startup_state(char *state);
void cell_svc_set_apn(struct mdm_hl7800_apn *access_point);
void cell_svc_set_rssi(int value);
void cell_svc_set_sinr(int value);
void cell_svc_set_fw_ver(const char *ver);

#endif