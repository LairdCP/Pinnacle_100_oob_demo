/**
 * @file bt_scan.c
 * @brief
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(bt_scan);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include "FrameworkIncludes.h"
#include "bt_scan.h"

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
/* Sensor events are not received properly unless filter duplicates is OFF
 * Bug 16484: Zephyr 2.x - Retest filter duplicates */
#define BT_LE_SCAN_CONFIG1                                                     \
	BT_LE_SCAN_PARAM(BT_HCI_LE_SCAN_ACTIVE,                                \
			 BT_HCI_LE_SCAN_FILTER_DUP_DISABLE,                    \
			 BT_GAP_SCAN_FAST_INTERVAL, BT_GAP_SCAN_FAST_WINDOW)

/******************************************************************************/
/* Global Data Definitions                                                    */
/******************************************************************************/

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
static atomic_t scanning = ATOMIC_INIT(0);

K_SEM_DEFINE(stop_requests, 0, CONFIG_BT_MAX_CONN);

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
void bt_scan_start(void)
{
	if (k_sem_count_get(&stop_requests) != 0) {
		return;
	}

	if (atomic_cas(&scanning, 0, 1)) {
		int err = bt_le_scan_start(BT_LE_SCAN_CONFIG1,
					   bt_scan_adv_handler);
		LOG_DBG("%d", err);
	}
}

void bt_scan_stop(void)
{
	k_sem_give(&stop_requests);
	if (atomic_cas(&scanning, 1, 0)) {
		int err = bt_le_scan_stop();
		LOG_DBG("%d", err);
	}
}

void bt_scan_resume(s32_t timeout)
{
	k_sem_take(&stop_requests, timeout);
	bt_scan_start();
}

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
__weak void bt_scan_adv_handler(const bt_addr_le_t *addr, s8_t rssi, u8_t type,
				struct net_buf_simple *ad)
{
	char bt_addr[BT_ADDR_LE_STR_LEN];
	memset(bt_addr, 0, BT_ADDR_LE_STR_LEN);
	bt_addr_le_to_str(addr, bt_addr, BT_ADDR_LE_STR_LEN);
	LOG_DBG("Advert from %s RSSI: %d TYPE: %d", bt_addr, rssi, type);
	LOG_HEXDUMP_DBG(ad->data, ad->len, "LwM2M Client PSK (hex)");
}
