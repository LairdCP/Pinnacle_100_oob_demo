/**
 * @file bt_scan.c
 * @brief
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(bt_scan);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <kernel.h>
#include <stddef.h>
#include "bt_scan.h"

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
/* Sensor events are not received properly unless filter duplicates is OFF
 * Bug 16484: Zephyr 2.x - Retest filter duplicates and move this to app */
#define BT_LE_SCAN_CONFIG1                                                     \
	BT_LE_SCAN_PARAM(BT_HCI_LE_SCAN_ACTIVE,                                \
			 BT_HCI_LE_SCAN_FILTER_DUP_DISABLE,                    \
			 BT_GAP_SCAN_FAST_INTERVAL, BT_GAP_SCAN_FAST_WINDOW)

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
static void scan_start(void);
static bool valid_user_id(int id);

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
static atomic_t scanning = ATOMIC_INIT(0);
static atomic_t bts_users = ATOMIC_INIT(0);
static atomic_t stop_requests = ATOMIC_INIT(0);
static atomic_t start_requests = ATOMIC_INIT(0);
static bt_le_scan_cb_t *adv_handlers[CONFIG_BT_SCAN_MAX_USERS];

static void bt_scan_adv_handler(const bt_addr_le_t *addr, s8_t rssi, u8_t type,
				struct net_buf_simple *ad);

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
bool bt_scan_register(int *pId, bt_le_scan_cb_t *cb)
{
	*pId = (int)atomic_inc(&bts_users);
	if (valid_user_id(*pId)) {
		adv_handlers[*pId] = cb;
		return true;
	} else {
		return false;
	}
}

void bt_scan_start(int id)
{
	if (valid_user_id(id)) {
		atomic_set_bit(&start_requests, id);
		scan_start();
	}
}

void bt_scan_stop(int id)
{
	if (valid_user_id(id)) {
		atomic_clear_bit(&start_requests, id);
		atomic_set_bit(&stop_requests, id);
		if (atomic_cas(&scanning, 1, 0)) {
			int err = bt_le_scan_stop();
			LOG_DBG("%d", err);
		}
	}
}

void bt_scan_resume(int id)
{
	if (valid_user_id(id)) {
		atomic_clear_bit(&stop_requests, id);
		scan_start();
	}
}

void bt_scan_restart(int id)
{
	if (valid_user_id(id)) {
		atomic_clear_bit(&stop_requests, id);
		atomic_set_bit(&start_requests, id);
		scan_start();
	}
}

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
static void scan_start(void)
{
	if (atomic_get(&start_requests) == 0) {
		return;
	}

	if (atomic_get(&stop_requests) != 0) {
		return;
	}

	if (atomic_cas(&scanning, 0, 1)) {
		int err = bt_le_scan_start(BT_LE_SCAN_CONFIG1,
					   bt_scan_adv_handler);
		LOG_DBG("%d", err);
	}
}

static bool valid_user_id(int id)
{
	if (id < CONFIG_BT_SCAN_MAX_USERS) {
		return true;
	} else {
		LOG_ERR("Invalid bt scan user id");
		return false;
	}
}

static void bt_scan_adv_handler(const bt_addr_le_t *addr, s8_t rssi, u8_t type,
				struct net_buf_simple *ad)
{
#if CONFIG_BT_SCAN_VERBOSE_ADV_HANDLER
	char bt_addr[BT_ADDR_LE_STR_LEN];
	memset(bt_addr, 0, BT_ADDR_LE_STR_LEN);
	bt_addr_le_to_str(addr, bt_addr, BT_ADDR_LE_STR_LEN);
	LOG_DBG("Advert from %s RSSI: %d Type: %d", bt_addr, rssi, type);
	LOG_HEXDUMP_DBG(ad->data, ad->len, "Data:");
#endif

	size_t i;
	for (i = 0; i < CONFIG_BT_SCAN_MAX_USERS; i++) {
		if (adv_handlers[i] != NULL) {
			adv_handlers[i](addr, rssi, type, ad);
		}
	}
}
