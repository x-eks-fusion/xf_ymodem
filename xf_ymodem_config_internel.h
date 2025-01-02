/**
 * @file xf_ymodem_config_internel.h
 * @author catcatBlue (catcatblue@qq.com)
 * @brief 
 * @version 1.0
 * @date 2025-01-02
 * 
 * Copyright (c) 2025, CorAL. All rights reserved.
 * 
 */

#ifndef __XF_YMODEM_CONFIG_INTERNEL_H__
#define __XF_YMODEM_CONFIG_INTERNEL_H__

/* ==================== [Includes] ========================================== */

#include "xf_ymodem_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== [Defines] =========================================== */

#if (!defined(XF_YMODEM_XSHELL_ENABLE) || (XF_YMODEM_XSHELL_ENABLE) || defined(__DOXYGEN__))
#define XF_YMODEM_XSHELL_IS_ENABLE (1)
#else
#define XF_YMODEM_XSHELL_IS_ENABLE (0)
#endif

#if (!defined(XF_YMODEM_DEBUG_ENABLE) || (XF_YMODEM_DEBUG_ENABLE) || defined(__DOXYGEN__))
#define XF_YMODEM_DEBUG_IS_ENABLE (1)
#else
#define XF_YMODEM_DEBUG_IS_ENABLE (0)
#endif

/* ==================== [Typedefs] ========================================== */

/* ==================== [Global Prototypes] ================================= */

/* ==================== [Macros] ============================================ */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __XF_YMODEM_CONFIG_INTERNEL_H__ */
