/**
 * @file xf_main.c
 * @author catcatBlue (catcatblue@qq.com)
 * @brief
 * @version 1.0
 * @date 2024-12-06
 *
 * @copyright Copyright (c) 2024
 *
 */

/* ==================== [Includes] ========================================== */

#include "xf_utils.h"
#include "xf_ymodem_example.h"

/* ==================== [Defines] =========================================== */

/* ==================== [Typedefs] ========================================== */

/* ==================== [Static Prototypes] ================================= */

/* ==================== [Static Variables] ================================== */

/* ==================== [Macros] ============================================ */

/* ==================== [Global Functions] ================================== */

void xf_main(void)
{
#if defined(CONFIG_XF_YMODEM_EXAMPLE_RECEIVER)
    xf_ymodem_example_receiver();
#elif defined(CONFIG_XF_YMODEM_EXAMPLE_SENDER)
    xf_ymodem_example_sender();
#endif
}

/* ==================== [Static Functions] ================================== */
