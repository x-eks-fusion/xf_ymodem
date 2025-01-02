/**
 * @file xf_ymodem_internel.h
 * @author catcatBlue (catcatblue@qq.com)
 * @brief xf_ymodem 内部接口。
 * @version 1.0
 * @date 2025-01-02
 * 
 * Copyright (c) 2025, CorAL. All rights reserved.
 * 
 */

#ifndef __XF_YMODEM_INTERNEL_H__
#define __XF_YMODEM_INTERNEL_H__

/* ==================== [Includes] ========================================== */

#include "xf_utils.h"
#include "xf_ymodem_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== [Defines] =========================================== */

/* ==================== [Typedefs] ========================================== */

/* ==================== [Global Prototypes] ================================= */

/* 以下是内部接口，用户通常不需要使用 */

uint16_t xf_ymodem_crc16(uint16_t crc_start, const uint8_t *buf, uint32_t len);

bool xf_ymodem_is_hex(char ch);
uint32_t xf_ymodem_convert_hex(char ch);
xf_err_t xf_ymodem_str_to_u32(
    const uint8_t *p_str, uint32_t len, uint32_t *p_u32);
#define DECIMAL                     10
#define HEXADECIMAL                 16
xf_err_t xf_ymodem_u32_to_str(
    uint32_t u32_val, uint32_t radix,
    uint8_t *p_buf, uint32_t buf_size, uint32_t *p_len);
xf_err_t xf_ymodem_show_packet(uint8_t *packet, uint32_t packet_size);

xf_err_t xf_ymodem_putc(xf_ymodem_t *p_ym, uint8_t ch);

xf_err_t xf_ymodem_check_packet(xf_ymodem_t *p_ym);
xf_err_t xf_ymodem_recv_check_packet_header(xf_ymodem_t *p_ym);

xf_err_t xf_ymodem_flush_read(xf_ymodem_t *p_ym);

xf_err_t xf_ymodem_recv_get_packet(xf_ymodem_t *p_ym);

xf_err_t xf_ymodem_recv_request_file_info(xf_ymodem_t *p_ym);
xf_err_t xf_ymodem_parse_file_info(
    xf_ymodem_t *p_ym, xf_ymodem_file_info_t *p_info);

xf_err_t xf_ymodem_recv_get_file_data(xf_ymodem_t *p_ym);

/* 获取数据指针(p_ym->p_buf + offset_internal)，减少一次拷贝 */
xf_err_t xf_ymodem_recv_get_data_ptr(
    xf_ymodem_t *p_ym, uint8_t **pp_data_buf, uint32_t *p_buf_size);

xf_err_t xf_ymodem_recv_end(xf_ymodem_t *p_ym);

/* send */

xf_err_t xf_ymodem_getc(xf_ymodem_t *p_ym, uint8_t *p_ch);

xf_err_t xf_ymodem_send_packet(xf_ymodem_t *p_ym);

xf_err_t xf_ymodem_send_get_packet_data_len(
    xf_ymodem_t *p_ym, uint32_t *p_data_len);

xf_err_t xf_ymodem_send_regular_packet_data(
    xf_ymodem_t *p_ym, uint32_t data_len);

xf_err_t xf_ymodem_send_packet(xf_ymodem_t *p_ym);

xf_err_t xf_ymodem_send_prepare_packet_protocol_segment(xf_ymodem_t *p_ym);

xf_err_t xf_ymodem_prepare_file_info(
    xf_ymodem_t *p_ym, xf_ymodem_file_info_t *p_info);

/* ==================== [Macros] ============================================ */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __XF_YMODEM_INTERNEL_H__ */
