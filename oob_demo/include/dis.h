/**
 * @file dis.h
 * @brief Device Information Service
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __DIS_H__
#define __DIS_H__

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/
/**
 * @brief Reads Zephyr version and registers the service with the
 * Bluetooth stack.
 */
void dis_initialize(void);

#ifdef __cplusplus
}
#endif

#endif /* __DIS_H__ */
