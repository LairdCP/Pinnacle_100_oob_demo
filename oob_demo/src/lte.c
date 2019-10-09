/* lte.c - LTE management
 *
 * Copyright (c) 2019 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(oob_lte);

#include <zephyr.h>
#include <net/net_if.h>
#include <net/net_core.h>
#include <net/net_context.h>
#include <net/net_mgmt.h>
#include <net/socket.h>

#include <modem_receiver.h>
#include <drivers/modem/hl7800.h>

#include "lte.h"

#define LTE_LOG_ERR(...) LOG_ERR(__VA_ARGS__)
#define LTE_LOG_WRN(...) LOG_WRN(__VA_ARGS__)
#define LTE_LOG_INF(...) LOG_INF(__VA_ARGS__)
#define LTE_LOG_DBG(...) LOG_DBG(__VA_ARGS__)

struct mgmt_events {
	u32_t event;
	net_mgmt_event_handler_t handler;
	struct net_mgmt_event_callback cb;
};

static struct net_if *iface;
static struct net_if_config *cfg;
static struct mdm_receiver_context *mdm_rcvr;
static struct dns_resolve_context *dns;
static struct lte_status lteStatus;
static lte_event_function_t lteCallbackFunction = NULL;

static void onLteEvent(enum lte_event event)
{
	if (lteCallbackFunction != NULL) {
		lteCallbackFunction(event);
	}
}

static void iface_ready_evt_handler(struct net_mgmt_event_callback *cb,
				    u32_t mgmt_event, struct net_if *iface)
{
	if (mgmt_event != NET_EVENT_DNS_SERVER_ADD) {
		return;
	}

	LTE_LOG_DBG("LTE DNS addr added!");
	onLteEvent(LTE_EVT_READY);
}

static void iface_down_evt_handler(struct net_mgmt_event_callback *cb,
				   u32_t mgmt_event, struct net_if *iface)
{
	if (mgmt_event != NET_EVENT_IF_DOWN) {
		return;
	}

	LTE_LOG_DBG("LTE is down");
	onLteEvent(LTE_EVT_DISCONNECTED);
}

static struct mgmt_events iface_events[] = {
	{ .event = NET_EVENT_DNS_SERVER_ADD,
	  .handler = iface_ready_evt_handler },
	{ .event = NET_EVENT_IF_DOWN, .handler = iface_down_evt_handler },
	{ 0 }
};

static void setup_iface_events(void)
{
	int i;

	for (i = 0; iface_events[i].event; i++) {
		net_mgmt_init_event_callback(&iface_events[i].cb,
					     iface_events[i].handler,
					     iface_events[i].event);

		net_mgmt_add_event_callback(&iface_events[i].cb);
	}
}

void lteRegisterEventCallback(lte_event_function_t callback)
{
	lteCallbackFunction = callback;
}

int lteInit(void)
{
	int rc = LTE_ERR_NONE;
	setup_iface_events();

	/* wait for network interface to be ready */
	iface = net_if_get_default();
	if (!iface) {
		LTE_LOG_ERR("Could not get iface");
		rc = LTE_ERR_NO_IFACE;
		goto exit;
	}

	cfg = net_if_get_config(iface);
	if (!cfg) {
		LTE_LOG_ERR("Could not get iface config");
		rc = LTE_ERR_IFACE_CFG;
		goto exit;
	}

	dns = dns_resolve_get_default();
	if (!dns) {
		LTE_LOG_ERR("Could not get DNS context");
		rc = LTE_ERR_DNS_CFG;
		goto exit;
	}

	/* Get the modem receive context */
	mdm_rcvr = mdm_receiver_context_from_id(0);
	if (mdm_rcvr == NULL) {
		LTE_LOG_ERR("Invalid modem receiver");
		rc = LTE_ERR_MDM_CTX;
		goto exit;
	}

	lteStatus.radio_version = mdm_rcvr->data_revision;
	lteStatus.IMEI = mdm_rcvr->data_imei;
	lteStatus.ICCID = (const char *)mdm_hl7800_get_iccid();
	lteStatus.serialNumber = (const char *)mdm_hl7800_get_sn();

exit:
	return rc;
}

bool lteIsReady(void)
{
	struct sockaddr_in *dnsAddr;

	if (iface != NULL && cfg != NULL && &dns->servers[0] != NULL) {
		dnsAddr = net_sin(&dns->servers[0].dns_server);
		return net_if_is_up(iface) && cfg->ip.ipv4 &&
		       !net_ipv4_is_addr_unspecified(&dnsAddr->sin_addr);
	}

	return false;
}

struct lte_status *lteGetStatus(void)
{
	mdm_hl7800_get_signal_quality(&lteStatus.rssi, &lteStatus.sinr);
	return &lteStatus;
}