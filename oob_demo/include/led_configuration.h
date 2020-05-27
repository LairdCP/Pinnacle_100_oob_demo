/**
 * @file led_configuration.h
 * @brief
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __LED_CONFIGURATION_H__
#define __LED_CONFIGURATION_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include "laird_led.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Board definitions                                                          */
/******************************************************************************/

#define LED1_DEV DT_GPIO_LEDS_LED_1_GPIO_CONTROLLER
#define LED1 DT_GPIO_LEDS_LED_1_GPIO_PIN
#define LED2_DEV DT_GPIO_LEDS_LED_2_GPIO_CONTROLLER
#define LED2 DT_GPIO_LEDS_LED_2_GPIO_PIN
#define LED3_DEV DT_GPIO_LEDS_LED_3_GPIO_CONTROLLER
#define LED3 DT_GPIO_LEDS_LED_3_GPIO_PIN
#define LED4_DEV DT_GPIO_LEDS_LED_4_GPIO_CONTROLLER
#define LED4 DT_GPIO_LEDS_LED_4_GPIO_PIN

enum led_index {
	BLUE_LED1 = 0,
	GREEN_LED2,
	RED_LED3,
	GREEN_LED4, /* not recommended for use - reserved for bootloader */
};
BUILD_ASSERT_MSG(CONFIG_NUMBER_OF_LEDS > GREEN_LED4, "LED object too small");

#ifdef __cplusplus
}
#endif

#endif /* __LED_CONFIGURATION_H__ */
