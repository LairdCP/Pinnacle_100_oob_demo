/**
 * @file bt_scan.h
 * @brief Controls Bluetooth scanning for multiple centrals/observers with a
 * counting semaphore.
 *
 * @note Scanning must be disabled in order to connect.
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT_SCAN_H__
#define __BT_SCAN_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr/types.h>
#include <stddef.h>
#include <bluetooth/bluetooth.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/
/**
 * @brief Register user of scan module.
 *
 * @param pId user id
 * @param cb Register advertisement handler callback
 */
bool bt_scan_register(int *pId, bt_le_scan_cb_t *cb);

/**
 * @brief Start scanning (if there aren't any stop requests).
 */
void bt_scan_start(int id);

/**
 * @brief Stop scanning.
 */
void bt_scan_stop(int id);

/**
 * @brief Clear stop request.
 * Restart scanning if there aren't any stop requests and there is at least
 * one start request.
 */
void bt_scan_resume(int id);

/**
 * @brief Clear stop request, set start request, and start scanning
 * if there aren't any other stop requests.
 */
void bt_scan_restart(int id);

#ifdef __cplusplus
}
#endif

#endif /* __BT_SCAN_H__ */
