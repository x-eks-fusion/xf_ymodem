/**
 * @file xf_ymodem_example.h
 * @author catcatBlue (catcatblue@qq.com)
 * @brief 
 * @version 1.0
 * @date 2024-12-31
 * 
 * Copyright (c) 2024, CorAL. All rights reserved.
 * 
 */

#ifndef __XF_YMODEM_EXAMPLE_H__
#define __XF_YMODEM_EXAMPLE_H__

/* ==================== [Includes] ========================================== */

#include "xf_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== [Defines] =========================================== */

#define MD5_MAX_LEN     16
#define UART_NUM        CONFIG_XF_YMODEM_EXAMPLE_UART_NUM
#define UART_BAUDRATE   CONFIG_XF_YMODEM_EXAMPLE_UART_BAUDRATE
#define UART_TX_PIN     CONFIG_XF_YMODEM_EXAMPLE_UART_TX_PIN
#define UART_RX_PIN     CONFIG_XF_YMODEM_EXAMPLE_UART_RX_PIN

/* ==================== [Typedefs] ========================================== */

/* ==================== [Global Prototypes] ================================= */

void xf_ymodem_example_receiver(void);
void xf_ymodem_example_sender(void);

/* ==================== [Macros] ============================================ */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __XF_YMODEM_EXAMPLE_H__ */
