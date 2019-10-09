/*
 * Copyright (c) 2019 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LED_H
#define LED_H

#define LED_OFF 0
#define LED_ON 1

#define LED1_DEV DT_GPIO_LEDS_LED_1_GPIOS_CONTROLLER
#define LED1 DT_GPIO_LEDS_LED_1_GPIOS_PIN
#define LED2_DEV DT_GPIO_LEDS_LED_2_GPIOS_CONTROLLER
#define LED2 DT_GPIO_LEDS_LED_2_GPIOS_PIN
#define LED3_DEV DT_GPIO_LEDS_LED_3_GPIOS_CONTROLLER
#define LED3 DT_GPIO_LEDS_LED_3_GPIOS_PIN
#define LED4_DEV DT_GPIO_LEDS_LED_4_GPIOS_CONTROLLER
#define LED4 DT_GPIO_LEDS_LED_4_GPIOS_PIN

#define GREEN_LED LED2
#define RED_LED LED3

#define LED_ON_TIME 25 // mseconds

/**
 *  Init the LEDs for the board
 */
void led_init(void);

void led_flash_green(void);
void led_flash_red(void);

#endif /* LED_H */