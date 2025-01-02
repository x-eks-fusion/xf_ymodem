/**
 * @file xf_ymodem.h
 * @author catcatBlue (catcatblue@qq.com)
 * @brief xf_ymodem 公共接口。
 * @version 1.0
 * @date 2024-12-27
 *
 * Copyright (c) 2024, CorAL. All rights reserved.
 *
 */

#ifndef __XF_YMODEM_H__
#define __XF_YMODEM_H__

/* ==================== [Includes] ========================================== */

#include "xf_utils.h"
#include "xf_ymodem_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== [Defines] =========================================== */

/* ==================== [Typedefs] ========================================== */

/* ==================== [Global Prototypes] ================================= */

/**
 * @brief 检查 xf_ymodem 对象合法性
 * 
 * @param p_ym                  xf_ymodem 对象指针。
 * @return xf_err_t 
 *      - XF_OK                 成功
 *      - XF_ERR_INVALID_ARG    无效参数
 * 
 * @code{c}
 * xf_ymodem_t s_ymodem = {0};
 * xf_ymodem_t *p_ym  = &s_ymodem;
 * 
 * p_ym->p_buf        = s_ym_buf;
 * p_ym->buf_size     = ARRAY_SIZE(s_ym_buf);
 * p_ym->retry_num    = 10;
 * p_ym->timeout_ms   = 50;
 * p_ym->user_data    = NULL;
 * p_ym->ops          = &port_xf_ymodem_ops;
 * 
 * xf_ret = xf_ymodem_check(sp_ym);
 * XF_ERROR_CHECK(xf_ret);
 * @endcode
 */
xf_err_t xf_ymodem_check(xf_ymodem_t *p_ym);

/**
 * @brief xf_ymodem 请求接收文件。
 * 
 * @param p_ym                  xf_ymodem 对象指针。
 * @param[out] p_file_info      成功时，传出发送端发来的文件信息。
 * @return xf_err_t 
 *      - XF_OK                 成功
 *      - XF_ERR_TIMEOUT        指定时间内未接收到数据
 *      - XF_ERR_INVALID_ARG    无效参数
 *      - XF_FAIL               失败
 * 
 * @code{c}
 * char file_name[65]              = {0};
 * xf_ymodem_file_info_t file_info = {0};
 * file_info.p_name_buf            = file_name;
 * file_info.buf_size              = ARRAY_SIZE(file_name);
 * file_info.file_len              = 0;
 * 
 * xf_ret = xf_ymodem_recv_handshake(p_ym, &file_info);
 * if (xf_ret == XF_OK) {
 *     XF_LOGI(TAG, "file_name:    %s", file_info.p_name_buf);
 *     XF_LOGI(TAG, "file_len:     %d", (int)file_info.file_len);
 * }
 * @endcode
 */
xf_err_t xf_ymodem_recv_handshake(
    xf_ymodem_t *p_ym, xf_ymodem_file_info_t *p_file_info);

/**
 * @brief xf_ymodem 接收文件数据。
 * 
 * @param p_ym                  xf_ymodem 对象指针。
 * @param[out] pp_data_buf      传出当前指向当前包数据缓冲区的指针，
 * @param p_buf_size            传出缓冲区大小，单位字节。
 * @return xf_err_t 
 *      - XF_OK                 成功收到数据
 *      - XF_ERR_RESOURCE       对方已取消或接收完毕，见 @ref xf_ymodem_t.error_code
 *      - XF_ERR_TIMEOUT        指定时间内未接收到数据
 *      - XF_ERR_INVALID_ARG    无效参数
 *      - XF_FAIL               失败
 * 
 * @code{c}
 * uint8_t *p_buf    = NULL;
 * uint32_t buf_size = 0;
 * while (1) {
 *     xf_ret = xf_ymodem_recv_data(p_ym, &p_buf, &buf_size);
 *     if (xf_ret == XF_OK) {
 *         XF_LOGI(TAG, "progress: %8d/%d", (int)p_ym->file_len_transmitted, (int)p_ym->file_len);
 *         // 此时已经获取指向文件数据的指针，用户需要处理 p_buf 内的数据
 *     } else {
 *         break;
 *     }
 * }
 * @endcode
 */
xf_err_t xf_ymodem_recv_data(
    xf_ymodem_t *p_ym, uint8_t **pp_data_buf, uint32_t *p_buf_size);

/**
 * @brief xf_ymodem 请求发送文件。
 * 
 * @param p_ym                  xf_ymodem 对象指针。
 * @param p_file_info           需要发送的文件的信息。
 * @return xf_err_t 
 *      - XF_OK                 成功
 *      - XF_ERR_TIMEOUT        指定时间内未接收到接收端请求
 *      - XF_ERR_INVALID_ARG    无效参数
 *      - XF_FAIL               失败
 * 
 * @code{c}
 * char file_name[65]              = {"1-1234567890.md"};
 * const char file_data[]          = {"12345678901234567890"};
 * xf_ymodem_file_info_t file_info = {0};
 * file_info.p_name_buf            = file_name;
 * file_info.buf_size              = xf_strlen(file_name);
 * file_info.file_len              = ARRAY_SIZE(file_data);
 * 
 * xf_ret = xf_ymodem_recv_handshake(p_ym, &file_info);
 * @endcode
 */
xf_err_t xf_ymodem_send_handshake(
    xf_ymodem_t *p_ym, xf_ymodem_file_info_t *p_info);

/**
 * @brief xf_ymodem 发送端获取文件数据缓冲区。
 * 
 * @param p_ym                  xf_ymodem 对象指针。
 * @param[out] pp_data_buf      传出当前包数据缓冲区的指针。
 *                              稍后由用户填充。
 * @param p_buf_size            传出本包缓冲区大小，单位字节。
 *                              用户需要向传出的缓冲区指针填充 *p_buf_size 字节。
 * @return xf_err_t 
 *      - XF_OK                 成功传出指针及所需的字节数
 *      - XF_ERR_INVALID_ARG    无效参数
 *      - XF_FAIL               失败
 * 
 * @code{c}
 * uint8_t *p_buf    = NULL;
 * uint32_t buf_size = 0;
 * while (1) {
 *     xf_ret = xf_ymodem_send_get_buf_and_len(p_ym, &p_buf, &buf_size);
 *     if (xf_ret != XF_OK) {
 *         break;
 *     }
 *     // 正常情况下 p_buf != NULL, buf_size != 0, 实际情况下最好检查一下
 *     xf_memcpy(p_buf, file_data + p_ym->file_len_transmitted, buf_size);
 *     xf_ret = xf_ymodem_send_data(p_ym);
 *     if (xf_ret != XF_OK) {
 *         break;
 *     }
 *     XF_LOGI(TAG, "progress: %8d/%d", (int)p_ym->file_len_transmitted + buf_size, (int)p_ym->file_len);
 * }
 * @endcode
 */
xf_err_t xf_ymodem_send_get_buf_and_len(
    xf_ymodem_t *p_ym, uint8_t **pp_data_buf, uint32_t *p_buf_size);

/**
 * @brief xf_ymodem 发送数据。
 * 
 * @note 调用 xf_ymodem_send_get_buf_and_len() 获取缓冲区并填充完毕后调用此函数发送。
 * 
 * @param p_ym                  xf_ymodem 对象指针。
 * @return xf_err_t 
 *      - XF_OK                 成功收到数据
 *      - XF_ERR_RESOURCE       对方已取消或发送完毕，见 @ref xf_ymodem_t.error_code
 *      - XF_ERR_TIMEOUT        指定时间内未接收到接收端应答
 *      - XF_ERR_INVALID_ARG    无效参数
 *      - XF_FAIL               失败
 */
xf_err_t xf_ymodem_send_data(xf_ymodem_t *p_ym);

/**
 * @brief xf_ymodem 取消传输。
 * 
 * @param p_ym                  xf_ymodem 对象指针。
 * @return xf_err_t 
 *      - XF_OK                 成功
 */
xf_err_t xf_ymodem_cancel(xf_ymodem_t *p_ym);

/* ==================== [Macros] ============================================ */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __XF_YMODEM_H__ */
