/**
 * @file ex_md5.h
 * @author catcatBlue (catcatblue@qq.com)
 * @brief
 * @version 1.0
 * @date 2024-12-31
 *
 * Copyright (c) 2024, CorAL. All rights reserved.
 *
 */

#ifndef __EX_MD5_H__
#define __EX_MD5_H__

/* ==================== [Includes] ========================================== */

#include "xf_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== [Defines] =========================================== */

/* ==================== [Typedefs] ========================================== */

// 定义MD5上下文
typedef struct _ex_md5_ctx_t {
    uint32_t state[4];  // 存储ABCD的状态
    uint32_t count[2];  // 64位计数器，按字节表示
    uint8_t buffer[64]; // 缓冲区
} ex_md5_ctx_t;

/* ==================== [Global Prototypes] ================================= */

// 初始化MD5上下文
xf_err_t ex_md5_init(ex_md5_ctx_t *p_ctx);

// 更新MD5的缓冲区数据
xf_err_t ex_md5_update(ex_md5_ctx_t *p_ctx, const uint8_t *input, size_t inputLen);

// 计算最终MD5结果
xf_err_t ex_md5_final(ex_md5_ctx_t *p_ctx, uint8_t digest[16]);

/* ==================== [Macros] ============================================ */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __EX_MD5_H__ */
