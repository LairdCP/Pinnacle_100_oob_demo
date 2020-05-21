/**
 * @file sensor_adv_format.h
 * @brief Advertisement format for Laird BT sensors
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __SENSOR_ADV_FORMAT_H__
#define __SENSOR_ADV_FORMAT_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr/types.h>
#include <bluetooth/bluetooth.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Common                                                                     */
/******************************************************************************/
#define SENSOR_ADDR_STR_SIZE 13
#define SENSOR_ADDR_STR_LEN (SENSOR_ADDR_STR_SIZE - 1)

#define SENSOR_NAME_MAX_SIZE 32
#define SENSOR_NAME_MAX_STR_LEN (SENSOR_NAME_MAX_SIZE - 1)

#define LAIRD_CONNECTIVITY_MANUFACTURER_SPECIFIC_COMPANY_ID1 0x0077
#define LAIRD_CONNECTIVITY_MANUFACTURER_SPECIFIC_COMPANY_ID2 0x00E4

/* clang-format off */
#define BT510_1M_PHY_AD_PROTOCOL_ID      0x0001
#define BT510_CODED_PHY_AD_PROTOCOL_ID   0x0002
#define BT510_1M_PHY_RSP_PROTOCOL_ID     0x0003
/* clang-format on */

/******************************************************************************/
/* BT510                                                                      */
/******************************************************************************/
#define BT510_RESET_ACK_TO_DUMP_DELAY_TICKS K_SECONDS(10)
#define BT510_WRITE_TO_RESET_DELAY_TICKS K_MSEC(1500)

/* Format of the Manufacturer Specific Data using 1M PHY in Advertisement */
struct Bt510AdEvent {
	u16_t companyId;
	u16_t protocolId;
	u16_t networkId;
	u16_t flags;
	bt_addr_t addr;
	u8_t recordType;
	u16_t id;
	u32_t epoch;
	u16_t data;
	u16_t dataReserved;
	u8_t resetCount;
} __packed;
typedef struct Bt510AdEvent Bt510AdEvent_t;

/* Format of the  Manufacturer Specific Data using 1M PHY in Scan Response */
struct Bt510Rsp {
	u16_t companyId;
	u16_t protocolId;
	u16_t productId;
	u8_t firmwareVersionMajor;
	u8_t firmwareVersionMinor;
	u8_t firmwareVersionPatch;
	u8_t firmwareType;
	u8_t configVersion;
	u8_t bootloaderVersionMajor;
	u8_t bootloaderVersionMinor;
	u8_t bootloaderVersionPatch;
	u8_t hardwareMinorVersion;
} __packed;
typedef struct Bt510Rsp Bt510Rsp_t;

/*
 * This is the format for the 1M PHY.
 */
#define BT510_MSD_AD_FIELD_LENGTH 0x1b
#define BT510_MSD_AD_PAYLOAD_LENGTH (BT510_MSD_AD_FIELD_LENGTH - 1)
BUILD_ASSERT_MSG(sizeof(Bt510AdEvent_t) == BT510_MSD_AD_PAYLOAD_LENGTH,
		 "BT510 Advertisement data size mismatch (check packing)");

#define BT510_MSD_RSP_FIELD_LENGTH 0x10
#define BT510_MSD_RSP_PAYLOAD_LENGTH (BT510_MSD_RSP_FIELD_LENGTH - 1)
BUILD_ASSERT_MSG(sizeof(Bt510Rsp_t) == BT510_MSD_RSP_PAYLOAD_LENGTH,
		 "BT510 Scan Response size mismatch (check packing)");

#define SENSOR_AD_HEADER_SIZE 4
extern const u8_t BT510_AD_HEADER[SENSOR_AD_HEADER_SIZE];
extern const u8_t BT510_RSP_HEADER[SENSOR_AD_HEADER_SIZE];

#ifdef __cplusplus
}
#endif

#endif /* __SENSOR_ADV_FORMAT_H__ */
