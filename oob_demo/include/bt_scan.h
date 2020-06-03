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
 * @brief Starts scanning if the number of stop requests is zero.
 */
void bt_scan_start(void);

/**
 * @brief Stops scanning and increments the number of stop requests.
 */
void bt_scan_stop(void);

/**
 * @brief Decrements the number of stop requests and then calls bt_scan_start.
 *
 * @param timeout determines how long this task will block on the stop request
 * counting semaphore.
 *
 * @note A user should only call this once for each time that it has called
 * stop.
 */
void bt_scan_resume(s32_t timeout);

/******************************************************************************/
/* If desired, override weak implementation in application                    */
/******************************************************************************/
/**
 * @brief This callback is triggered after receiving BLE adverts.
 */
void bt_scan_adv_handler(const bt_addr_le_t *addr, s8_t rssi, u8_t type,
			 struct net_buf_simple *ad);

#ifdef __cplusplus
}
#endif

#endif /* __BT_SCAN_H__ */
