/**
 * @file xf_ymodem.c
 * @author catcatBlue (catcatblue@qq.com)
 * @brief
 * @version 1.0
 * @date 2024-12-27
 *
 * Copyright (c) 2024, CorAL. All rights reserved.
 *
 */

/* ==================== [Includes] ========================================== */

#include "xf_ymodem.h"
#include "xf_ymodem_internel.h"

/* ==================== [Defines] =========================================== */

/* ==================== [Typedefs] ========================================== */

/* ==================== [Static Prototypes] ================================= */

/* ==================== [Static Variables] ================================== */

static const char *const TAG = "xf_ymodem";

/* ==================== [Macros] ============================================ */

#if XF_YMODEM_DEBUG_IS_ENABLE
#undef __FILENAME__
#if !defined(__FILENAME__)
/**
 * @brief 获取不含路径的文件名.
 */
#   define __FILENAME__                 (__builtin_strrchr(__FILE__, '/') \
                                            ? (__builtin_strrchr(__FILE__, '/') + 1) \
                                            : ((__builtin_strrchr(__FILE__, '\\') \
                                                    ? (__builtin_strrchr(__FILE__, '\\') + 1) \
                                                    : (__FILE__))\
                                              ) \
                                        )
#else
#   define __FILENAME__                 __FILE__
#endif
#   define ym_printf(...)               xf_log_printf(__VA_ARGS__)
#   define YM_LOGD(tag, format, ...)    ym_printf("%s[%s:%d(%s)]: " format "\r\n", tag, __FILENAME__, __LINE__, __func__, ##__VA_ARGS__)
#else
#   define ym_printf(...)
#   define YM_LOGD(tag, format, ...)
#endif /* XF_YMODEM_DEBUG_IS_ENABLE */

#if !defined(min)
#   define min(x, y)                (((x) < (y)) ? (x) : (y))
#endif

#if !defined(xf_strnlen)
#   include <string.h>
#   define xf_strnlen(s, maxlen)    strnlen(s, maxlen)
#endif

#if !defined(xf_strncpy)
#   define xf_strncpy(d, s, len)    strncpy(d, s, len)
#endif

/* ==================== [Global Functions] ================================== */

xf_err_t xf_ymodem_check(xf_ymodem_t *p_ym)
{
    XF_CHECK(NULL == p_ym, XF_ERR_INVALID_ARG,
             TAG, "p_ym:%s", xf_err_to_name(XF_ERR_INVALID_ARG));

    /* 检查 p_buf */
    if (p_ym->p_buf == NULL) {
        YM_LOGD(TAG, "p_ym->p_buf:%s", xf_err_to_name(XF_ERR_INVALID_ARG));
        return XF_ERR_INVALID_ARG;
    }

    if (p_ym->buf_size < XF_YMODEM_SOH_PACKET_SIZE) {
        YM_LOGD(TAG, "p_ym->buf_size(%d)<XF_YMODEM_SOH_PACKET_SIZE(%d)",
                (int)p_ym->buf_size, (int)XF_YMODEM_SOH_PACKET_SIZE);
        return XF_ERR_INVALID_ARG;
    }

    /* 检查 ops */
    if ((p_ym->ops == NULL)
            || (p_ym->ops->read == NULL)
            || (p_ym->ops->write == NULL)
            || (p_ym->ops->flush == NULL)
            || (p_ym->ops->delay_ms == NULL)
            /* user_parse, user_file_info 允许为 NULL */
       ) {
        YM_LOGD(TAG, "p_ym->ops:%s", xf_err_to_name(XF_ERR_INVALID_ARG));
        return XF_ERR_INVALID_ARG;
    }

    return XF_OK;
}

xf_err_t xf_ymodem_recv_handshake(
    xf_ymodem_t *p_ym, xf_ymodem_file_info_t *p_info)
{
    xf_err_t xf_ret         = XF_OK;

    XF_CHECK(NULL == p_ym, XF_ERR_INVALID_ARG,
             TAG, "p_ym:%s", xf_err_to_name(XF_ERR_INVALID_ARG));
    XF_CHECK(NULL == p_info, XF_ERR_INVALID_ARG,
             TAG, "p_info:%s", xf_err_to_name(XF_ERR_INVALID_ARG));

    p_ym->state = XF_YMODEM_NONE;

    /* 请求文件信息 */
    xf_ret = xf_ymodem_recv_request_file_info(p_ym);
    if (xf_ret != XF_OK) {
        return xf_ret;
    }

    /* 解析文件信息 */
    xf_ret = xf_ymodem_parse_file_info(p_ym, p_info);
    if (xf_ret != XF_OK) {
        return xf_ret;
    }

    return xf_ret;
}

xf_err_t xf_ymodem_recv_request_file_info(xf_ymodem_t *p_ym)
{
    xf_err_t xf_ret = XF_OK;

    XF_CHECK(NULL == p_ym, XF_ERR_INVALID_ARG,
             TAG, "p_ym:%s", xf_err_to_name(XF_ERR_INVALID_ARG));

    p_ym->file_len_transmitted = 0;

    xf_ymodem_flush_read(p_ym);

    /* 发送 C, 请求发送端发送包含文件名及长度的起始帧 */
    xf_ymodem_putc(p_ym, XF_YMODEM_C);
    xf_ret = xf_ymodem_recv_get_packet(p_ym);
    if (xf_ret != XF_OK) {
        // YM_LOGD(TAG, "recv_packet:%s", xf_err_to_name(xf_ret));
        return xf_ret;
    }

    if ((p_ym->state != XF_YMODEM_RECV_GOT_EOT1)
            || (p_ym->state != XF_YMODEM_RECV_GOT_EOT2)
            || (p_ym->state != XF_YMODEM_RECV_FEEDBACK_EOT2)
       ) {
        xf_ymodem_putc(p_ym, XF_YMODEM_ACK);
    }

    p_ym->state = XF_YMODEM_RECV_FILE_INFO_AVAILABLE;

    return xf_ret;
}

xf_err_t xf_ymodem_flush_read(xf_ymodem_t *p_ym)
{
    xf_err_t xf_ret         = XF_OK;

    XF_CHECK(NULL == p_ym, XF_ERR_INVALID_ARG,
             TAG, "p_ym:%s", xf_err_to_name(XF_ERR_INVALID_ARG));

    p_ym->ops->flush();
    xf_memset((char *)p_ym->p_buf, 0, p_ym->buf_size);

    return xf_ret;
}

xf_err_t xf_ymodem_recv_get_packet(xf_ymodem_t *p_ym)
{
    xf_err_t    xf_ret          = XF_OK;

    uint8_t     check_header    = true; /*!< 是否需要检查包头 */

    int32_t     rlen            = 0; /*!< p_ym->ops->read 返回值，可能是负数 */
    int32_t     expect_len      = 0; /*!< 预期接收长度 */

    int32_t     retry           = 0;
    int32_t     retry_for_check = 0;

    XF_CHECK(NULL == p_ym, XF_ERR_INVALID_ARG,
             TAG, "p_ym:%s", xf_err_to_name(XF_ERR_INVALID_ARG));

    if (p_ym->tx_ack == true) {
        xf_ymodem_putc(p_ym, XF_YMODEM_ACK);
        p_ym->tx_ack = false;
    }

    retry_for_check     = p_ym->retry_num + 1;

l_retry_for_check_error:;
l_retry_for_eot2:;

    check_header        = true;
    retry               = p_ym->retry_num + 1;
    p_ym->packet_len    = 0;
    p_ym->data_len      = 0;
    expect_len          = 1; /*!< 先收包头 */

    while (retry > 0) {
        retry--;
        rlen = p_ym->ops->read(
                   p_ym->p_buf  + p_ym->packet_len,
                   expect_len   - p_ym->packet_len,
                   p_ym->timeout_ms);
        if (rlen <= 0) {
            continue;
        }

        retry               = p_ym->retry_num + 1; /*!< 成功时重置计数 */
        p_ym->packet_len   += rlen;

        if ((check_header) && (p_ym->packet_len >= 1)) {
            check_header = false;
            xf_ret = xf_ymodem_recv_check_packet_header(p_ym);
            if (xf_ret != XF_OK) {
                goto l_xf_ret;
            }
            if (p_ym->data_len > 0) {
                expect_len = XF_YMODEM_PROT_SEG_SIZE + p_ym->data_len;
            }
        } /* check_header */

        if (p_ym->packet_len >= expect_len) {
            break;
        }
    }

    if (p_ym->packet_len < expect_len) {
        p_ym->error_code    = XF_YMODEM_ERR_NO_DATA;
        xf_ret              = XF_ERR_TIMEOUT;
        goto l_xf_ret;
    }

    xf_ymodem_show_packet(p_ym->p_buf, p_ym->data_len);

    /* 检查数据正确性 */
    if (p_ym->data_len > 0) {
        xf_ret = xf_ymodem_check_packet(p_ym);
        if (xf_ret != XF_OK) {
            if (retry_for_check > 0) {
                retry_for_check--;
                xf_ymodem_flush_read(p_ym);
                p_ym->ops->delay_ms(p_ym->timeout_ms);
                /* 发 NAK 让发送端重发 */
                xf_ymodem_putc(p_ym, XF_YMODEM_NAK);
                xf_ymodem_flush_read(p_ym);
                goto l_retry_for_check_error;
            }
            goto l_xf_ret;
        }
    }

    if (p_ym->state == XF_YMODEM_RECV_GOT_EOT1) {
        xf_ymodem_putc(p_ym, XF_YMODEM_NAK);
        goto l_retry_for_eot2;
    } else if (p_ym->state == XF_YMODEM_RECV_GOT_EOT2) {
        xf_ymodem_putc(p_ym, XF_YMODEM_ACK);
        p_ym->state = XF_YMODEM_RECV_FEEDBACK_EOT2;
    }

    p_ym->error_code    = XF_YMODEM_OK;

l_xf_ret:;

    return xf_ret;
}

xf_err_t xf_ymodem_recv_check_packet_header(xf_ymodem_t *p_ym)
{
    xf_err_t    xf_ret          = XF_OK;
    uint8_t     ch              = 0;

    XF_CHECK(NULL == p_ym, XF_ERR_INVALID_ARG,
             TAG, "p_ym:%s", xf_err_to_name(XF_ERR_INVALID_ARG));

    /*
        FIXME esp32 上，开 -Og 优化。
        在 xshell 发 CAN 过来时有概率修改 p_ym->p_buf 指针成 0x08081818。
     */
    ch = p_ym->p_buf[XF_YMODEM_HEADER_IDX];
    switch (ch) {
    case XF_YMODEM_SOH: {
        p_ym->data_len = XF_YMODEM_SOH_DATA_SIZE;
    } break;
    case XF_YMODEM_STX_1K: {
        p_ym->data_len = XF_YMODEM_STX_1K_DATA_SIZE;
    } break;
    case XF_YMODEM_STX_2K: {
        p_ym->data_len = XF_YMODEM_STX_2K_DATA_SIZE;
    } break;
    case XF_YMODEM_STX_4K: {
        p_ym->data_len = XF_YMODEM_STX_4K_DATA_SIZE;
    } break;
    case XF_YMODEM_STX_8K: {
        p_ym->data_len = XF_YMODEM_STX_8K_DATA_SIZE;
    } break;
    case XF_YMODEM_EOT: {
        p_ym->data_len      = 0;
        if (p_ym->state == XF_YMODEM_RECV_REQUEST_FILE_DATA) {
            p_ym->state = XF_YMODEM_RECV_GOT_EOT1;
        } else if (p_ym->state == XF_YMODEM_RECV_GOT_EOT1) {
            p_ym->state = XF_YMODEM_RECV_GOT_EOT2;
        }
    } break;
    case XF_YMODEM_CAN: {
        p_ym->data_len      = 0;
        p_ym->error_code    = XF_YMODEM_ERR_CAN;
        xf_ret              = XF_ERR_RESOURCE;
        goto l_xf_ret;
    } break;
    default: {
        YM_LOGD(TAG, "recv(%02X) Not Supported", (int)ch);
        p_ym->data_len      = 0;
        xf_ret              = XF_FAIL;
        goto l_xf_ret;
    } break;
    }

    if (XF_YMODEM_PROT_SEG_SIZE + p_ym->data_len > p_ym->buf_size) {
        YM_LOGD(TAG, "p_ym->data_len(%d) Not Supported", (int)p_ym->data_len);
        xf_ret = XF_FAIL;
        goto l_xf_ret;
    }

l_xf_ret:;
    return xf_ret;
}

xf_err_t xf_ymodem_check_packet(xf_ymodem_t *p_ym)
{
    xf_err_t    xf_ret          = XF_OK;
    uint16_t    crc16_expect    = 0;
    uint16_t    crc16_cal       = 0;
    uint16_t    crc16_hi        = 0;
    uint16_t    crc16_lo        = 0;

    XF_CHECK(NULL == p_ym, XF_ERR_INVALID_ARG,
             TAG, "p_ym:%s", xf_err_to_name(XF_ERR_INVALID_ARG));

    if (p_ym->data_len == 0) {
        return xf_ret;
    }

    /* 检查序列号 */
    if ((p_ym->p_buf[XF_YMODEM_PN_IDX] ^ p_ym->p_buf[XF_YMODEM_NPN_IDX]) != 0xFF) {
        YM_LOGD(TAG, "packet num error");
        p_ym->error_code = XF_YMODEM_ERR_PN;
        xf_ret = XF_ERR_INVALID_CHECK;
    }

    /* 检查 crc */
    crc16_cal = xf_ymodem_crc16(
                    XF_YMODEM_CRC_START_VAL_DEFAULT,
                    &p_ym->p_buf[XF_YMODEM_DATA_IDX], p_ym->data_len);
    crc16_hi = p_ym->p_buf[XF_YMODEM_DATA_IDX + p_ym->data_len + 0];
    crc16_lo = p_ym->p_buf[XF_YMODEM_DATA_IDX + p_ym->data_len + 1];
    crc16_expect = (((crc16_hi & 0xFF) << 8U)) | ((crc16_lo & 0xFF));
    if (crc16_expect != crc16_cal) {
        YM_LOGD(TAG, "crc error, expect(0x%04x), calculated(0x%04x)",
                (int)crc16_expect, (int)crc16_cal);
        p_ym->error_code = XF_YMODEM_ERR_CRC;
        xf_ret = XF_ERR_INVALID_CHECK;
    }

    if (xf_ret != XF_OK) {
        p_ym->data_len = 0;
    }

    return xf_ret;
}

xf_err_t xf_ymodem_parse_file_info(
    xf_ymodem_t *p_ym, xf_ymodem_file_info_t *p_info)
{
    xf_err_t xf_ret         = XF_OK;
    uint32_t file_name_actual_len       = 0;
    uint32_t file_len_str_actual_len    = 0;
    int32_t file_len        = 0;
    uint32_t cpy_len        = 0;
    uint32_t buf_idx        = 0;

    XF_CHECK(NULL == p_ym, XF_ERR_INVALID_ARG,
             TAG, "p_ym:%s", xf_err_to_name(XF_ERR_INVALID_ARG));
    XF_CHECK(p_info->p_name_buf == NULL, XF_ERR_INVALID_ARG,
             TAG, "p_info->p_name_buf:%s", xf_err_to_name(XF_ERR_INVALID_ARG));
    XF_CHECK(p_info->buf_size <= 1, XF_ERR_INVALID_ARG,
             TAG, "p_info->buf_size:%s", xf_err_to_name(XF_ERR_INVALID_ARG));
    XF_CHECK(p_ym->data_len == 0, XF_ERR_INVALID_ARG,
             TAG, "p_ym->data_len:%s", xf_err_to_name(XF_ERR_INVALID_ARG));

    /*
        格式:
              <file_name> + '\0'
            + [file_len] + '\0'
            + [(不支持)时间戳] + ' '
            + [(不支持)文件权限] + '\0'
            + 填充 '\0'
     */

    buf_idx                 = XF_YMODEM_DATA_IDX;

    /* 文件名 */
    if (p_ym->p_buf[buf_idx] == '\0') {
        YM_LOGD(TAG, "p_ym->p_buf[buf_idx]==\\0");
        p_ym->error_code    = XF_YMODEM_ERR_INVALID_FILE_NAME;
        xf_ret              = XF_FAIL;
        goto l_xf_ret;
    }
    file_name_actual_len    = xf_strnlen((const char *)&p_ym->p_buf[buf_idx], p_ym->data_len);
    if (file_name_actual_len >= p_ym->data_len) {
        YM_LOGD(TAG, "file_name_actual_len(%d)>=p_ym->data_len(%d)",
                (int)file_name_actual_len, (int)p_ym->data_len);
        p_ym->error_code    = XF_YMODEM_ERR_INVALID_FILE_NAME;
        xf_ret              = XF_FAIL;
        goto l_xf_ret;
    }
    cpy_len                 = min(p_info->buf_size - 1, /*!< 留一个 '\0' */
                                  file_name_actual_len);
    if (p_info->p_name_buf) {
        xf_strncpy(p_info->p_name_buf, (const char *)&p_ym->p_buf[buf_idx], cpy_len);
        p_info->p_name_buf[cpy_len]     = '\0';
    }
    buf_idx += file_name_actual_len;    /*!< 跳过文件名 */
    buf_idx++;                          /*!< 跳过 '\0' */

    /* 可能没有文件长度 */
    if (p_ym->p_buf[buf_idx] == '\0') {
        file_len = -1;
        goto l_skip_parse_len;
    }

    /* 文件长度 */
    file_len_str_actual_len = xf_strnlen((const char *)&p_ym->p_buf[buf_idx], p_ym->data_len - buf_idx);
    xf_ret = xf_ymodem_str_to_u32(&p_ym->p_buf[buf_idx], file_len_str_actual_len, (uint32_t *)&file_len);
    if (xf_ret != XF_OK) {
        YM_LOGD(TAG, "xf_ymodem_str_to_u32:%s", xf_err_to_name(xf_ret));
        return xf_ret;
    }
    buf_idx += file_len_str_actual_len; /*!< 跳过文件名 */
    buf_idx++;                          /*!< 跳过 '\0' */

l_skip_parse_len:;
    p_info->file_len    = file_len;
    p_ym->file_len      = file_len;

    if ((p_ym->ops->user_parse) && (buf_idx < (p_ym->data_len - 1))) {
        p_ym->ops->user_parse(
            &p_ym->p_buf[buf_idx], p_ym->data_len - buf_idx, p_ym->user_data);
    }

l_xf_ret:;
    return xf_ret;
}

xf_err_t xf_ymodem_recv_data(
    xf_ymodem_t *p_ym, uint8_t **pp_data_buf, uint32_t *p_buf_size)
{
    xf_err_t xf_ret         = XF_OK;
    int32_t retry;

    XF_CHECK(NULL == p_ym, XF_ERR_INVALID_ARG,
             TAG, "p_ym:%s", xf_err_to_name(XF_ERR_INVALID_ARG));
    XF_CHECK(NULL == pp_data_buf, XF_ERR_INVALID_ARG,
             TAG, "pp_data_buf:%s", xf_err_to_name(XF_ERR_INVALID_ARG));
    XF_CHECK(NULL == p_buf_size, XF_ERR_INVALID_ARG,
             TAG, "p_buf_size:%s", xf_err_to_name(XF_ERR_INVALID_ARG));

    retry = p_ym->retry_num + 1;
    while (retry > 0) {
        retry--;
        /* 获取文件数据 */
        xf_ret = xf_ymodem_recv_get_file_data(p_ym);
        if ((xf_ret == XF_OK)
                || (xf_ret == XF_ERR_RESOURCE) /*!< 对方已取消 */
           ) {
            break;
        }
        /* XF_ERR_TIMEOUT 时重试 */
    }
    if (xf_ret != XF_OK) {
        return xf_ret;
    }

    /* 已经接收到了两次结束信号，当前文件已传输完毕 */
    if (p_ym->state == XF_YMODEM_RECV_FEEDBACK_EOT2) {
        xf_ret = xf_ymodem_recv_end(p_ym);
        if (xf_ret == XF_OK) {
            p_ym->state = XF_YMODEM_RECV_END;
            return XF_ERR_RESOURCE;
        } else {
            return xf_ret;
        }
    }

    /* 传出文件数据指针 */
    xf_ret = xf_ymodem_recv_get_data_ptr(p_ym, pp_data_buf, p_buf_size);
    if (xf_ret != XF_OK) {
        return xf_ret;
    }

    return xf_ret;
}

xf_err_t xf_ymodem_recv_get_file_data(xf_ymodem_t *p_ym)
{
    xf_err_t xf_ret = XF_OK;

    XF_CHECK(NULL == p_ym, XF_ERR_INVALID_ARG,
             TAG, "p_ym:%s", xf_err_to_name(XF_ERR_INVALID_ARG));

    xf_ymodem_flush_read(p_ym);

    if (p_ym->state == XF_YMODEM_RECV_FILE_INFO_AVAILABLE) {
        /* 首次请求文件数据信息时需要发送 C */
        xf_ymodem_putc(p_ym, XF_YMODEM_C);
        p_ym->state = XF_YMODEM_RECV_REQUEST_FILE_DATA;
    }

    xf_ret = xf_ymodem_recv_get_packet(p_ym);
    if (xf_ret != XF_OK) {
        YM_LOGD(TAG, "recv_packet:%s", xf_err_to_name(xf_ret));
        return xf_ret;
    }

    if (p_ym->state == XF_YMODEM_RECV_REQUEST_FILE_DATA) {
        /* 下一次接收前要发送应答 */
        p_ym->tx_ack = true;
    }

    return xf_ret;
}

xf_err_t xf_ymodem_recv_end(xf_ymodem_t *p_ym)
{
    xf_err_t xf_ret = XF_OK;

    XF_CHECK(NULL == p_ym, XF_ERR_INVALID_ARG,
             TAG, "p_ym:%s", xf_err_to_name(XF_ERR_INVALID_ARG));

    xf_ymodem_flush_read(p_ym);

    xf_ymodem_putc(p_ym, XF_YMODEM_C);
    xf_ret = xf_ymodem_recv_get_packet(p_ym);
    if (xf_ret != XF_OK) {
        YM_LOGD(TAG, "recv_packet:%s", xf_err_to_name(xf_ret));
        return xf_ret;
    }

    xf_ymodem_putc(p_ym, XF_YMODEM_ACK);
#if XF_YMODEM_XSHELL_IS_ENABLE
    /* 对于 xshell 需要多发一个 O 才能结束 */
    xf_ymodem_putc(p_ym, 'O');
#endif

    p_ym->state = XF_YMODEM_NONE;

    return xf_ret;
}

xf_err_t xf_ymodem_recv_get_data_ptr(
    xf_ymodem_t *p_ym, uint8_t **pp_data_buf, uint32_t *p_buf_size)
{
    xf_err_t xf_ret             = XF_OK;
    uint32_t file_remain_len    = 0;

    XF_CHECK(NULL == p_ym, XF_ERR_INVALID_ARG,
             TAG, "p_ym:%s", xf_err_to_name(XF_ERR_INVALID_ARG));
    XF_CHECK(NULL == pp_data_buf, XF_ERR_INVALID_ARG,
             TAG, "pp_data_buf:%s", xf_err_to_name(XF_ERR_INVALID_ARG));
    XF_CHECK(NULL == p_buf_size, XF_ERR_INVALID_ARG,
             TAG, "p_buf_size:%s", xf_err_to_name(XF_ERR_INVALID_ARG));

    *pp_data_buf = &p_ym->p_buf[XF_YMODEM_DATA_IDX];

    file_remain_len = p_ym->file_len - p_ym->file_len_transmitted;

    if (p_ym->data_len <= file_remain_len) {
        /* 如果本包数据段长度小于剩余文件剩余的长度 -> 本包收到完整一包数据 */
        p_ym->file_len_transmitted += p_ym->data_len;
        *p_buf_size                = p_ym->data_len;
    } else {
        /* 否则本包只有部分数据有效 */
        p_ym->file_len_transmitted += file_remain_len;
        *p_buf_size                = file_remain_len;
    }

    return xf_ret;
}

xf_err_t xf_ymodem_cancel(xf_ymodem_t *p_ym)
{
    xf_err_t xf_ret = XF_OK;
    xf_ymodem_putc(p_ym, XF_YMODEM_CAN);
    xf_ymodem_putc(p_ym, XF_YMODEM_CAN);
    xf_ymodem_putc(p_ym, XF_YMODEM_CAN);
    xf_ymodem_putc(p_ym, XF_YMODEM_CAN);
    xf_ymodem_putc(p_ym, XF_YMODEM_CAN);
    return xf_ret;
}

/* send */

xf_err_t xf_ymodem_send_handshake(
    xf_ymodem_t *p_ym, xf_ymodem_file_info_t *p_info)
{
    xf_err_t    xf_ret      = XF_OK;
    uint8_t     ch          = 0;

    XF_CHECK(NULL == p_ym, XF_ERR_INVALID_ARG,
             TAG, "p_ym:%s", xf_err_to_name(XF_ERR_INVALID_ARG));
    XF_CHECK(NULL == p_info, XF_ERR_INVALID_ARG,
             TAG, "p_info:%s", xf_err_to_name(XF_ERR_INVALID_ARG));

    p_ym->packet_num    = 0;
    p_ym->state         = XF_YMODEM_NONE;

    /* 获取第 1 个 C */
    xf_ret = xf_ymodem_getc(p_ym, &ch);
    if (xf_ret != XF_OK) {
        YM_LOGD(TAG, "no data");
        return xf_ret;
    }
    if (ch != XF_YMODEM_C) {
        YM_LOGD(TAG, "recv(%02X) Not Supported", (int)ch);
        xf_ret = XF_FAIL;
        return xf_ret;
    }

    /* 准备文件信息 */
    xf_ret = xf_ymodem_prepare_file_info(p_ym, p_info);
    if (xf_ret != XF_OK) {
        return xf_ret;
    }

    /* 准备包协议 */
    xf_ret = xf_ymodem_send_prepare_packet_protocol_segment(p_ym);
    if (xf_ret != XF_OK) {
        return xf_ret;
    }

    /* 发送起始帧 */
    xf_ret = xf_ymodem_send_packet(p_ym);
    if (xf_ret != XF_OK) {
        return xf_ret;
    }

    p_ym->packet_num++;
    p_ym->state         = XF_YMODEM_SEND_FILE_INFO;

    if (p_ym->state == XF_YMODEM_SEND_FILE_INFO) {
        /* 获取第 1 次 ACK */
        xf_ret = xf_ymodem_getc(p_ym, &ch);
        if (xf_ret != XF_OK) {
            YM_LOGD(TAG, "xf_ret:%s", xf_err_to_name(xf_ret));
            return xf_ret;
        }
        if (ch != XF_YMODEM_ACK) {
            YM_LOGD(TAG, "recv(%02X) Not Supported", (int)ch);
            return xf_ret;
        }
        /* 获取第 2 次 C */
        xf_ret = xf_ymodem_getc(p_ym, &ch);
        if (xf_ret != XF_OK) {
            return xf_ret;
        }
        if (ch != XF_YMODEM_C) {
            YM_LOGD(TAG, "recv(%02X) Not Supported", (int)ch);
            return xf_ret;
        }
        p_ym->state = XF_YMODEM_SEND_FILE_DATA;
    }

    return xf_ret;
}

xf_err_t xf_ymodem_getc(xf_ymodem_t *p_ym, uint8_t *p_ch)
{
    xf_err_t    xf_ret          = XF_OK;
    int32_t     rlen            = 0;
    int32_t     retry           = 0;

    XF_CHECK(NULL == p_ym, XF_ERR_INVALID_ARG,
             TAG, "p_ym:%s", xf_err_to_name(XF_ERR_INVALID_ARG));

    retry = p_ym->retry_num + 1;
    while (retry > 0) {
        retry--;
        rlen = p_ym->ops->read(p_ch, 1, p_ym->timeout_ms);
        if (rlen > 0) {
            break;
        }
    }

    if (rlen < 1) {
        p_ym->error_code    = XF_YMODEM_ERR_NO_DATA;
        xf_ret              = XF_ERR_TIMEOUT;
    }

#if XF_YMODEM_DEBUG_IS_ENABLE
    if (rlen >= 1) {
        ym_printf("\t\t\t\t");
        switch (*p_ch) {
        case XF_YMODEM_C:
            ym_printf("C\r\n");
            break;
        case XF_YMODEM_CAN:
            ym_printf("CAN\r\n");
            break;
        case XF_YMODEM_NAK:
            ym_printf("NAK\r\n");
            break;
        case XF_YMODEM_ACK:
            ym_printf("ACK\r\n");
            break;
        default:
            break;
        }
    }
#endif /*XF_YMODEM_DEBUG_IS_ENABLE*/

    return xf_ret;
}

xf_err_t xf_ymodem_send_get_packet_data_len(
    xf_ymodem_t *p_ym, uint32_t *p_data_len)
{
    xf_err_t    xf_ret          = XF_OK;
    uint32_t    data_len        = 0;
    uint32_t    remaining_len   = 0;
    uint32_t    data_len_max    = 0;

    XF_CHECK(NULL == p_ym, XF_ERR_INVALID_ARG,
             TAG, "p_ym:%s", xf_err_to_name(XF_ERR_INVALID_ARG));

    remaining_len = p_ym->file_len - p_ym->file_len_transmitted;

    if (p_ym->buf_size >= XF_YMODEM_STX_8K_DATA_SIZE + XF_YMODEM_PROT_SEG_SIZE) {
        data_len_max = XF_YMODEM_STX_8K_DATA_SIZE;
    } else if (p_ym->buf_size >= XF_YMODEM_STX_4K_DATA_SIZE + XF_YMODEM_PROT_SEG_SIZE) {
        data_len_max = XF_YMODEM_STX_4K_DATA_SIZE;
    } else if (p_ym->buf_size >= XF_YMODEM_STX_2K_DATA_SIZE + XF_YMODEM_PROT_SEG_SIZE) {
        data_len_max = XF_YMODEM_STX_2K_DATA_SIZE;
    } else if (p_ym->buf_size >= XF_YMODEM_STX_1K_DATA_SIZE + XF_YMODEM_PROT_SEG_SIZE) {
        data_len_max = XF_YMODEM_STX_1K_DATA_SIZE;
    } else if (p_ym->buf_size >= XF_YMODEM_SOH_DATA_SIZE + XF_YMODEM_PROT_SEG_SIZE) {
        data_len_max = XF_YMODEM_SOH_DATA_SIZE;
    } else {
        YM_LOGD(TAG, "p_ym->buf_size(%d) Not Supported", (int)p_ym->buf_size);
    }

    if (remaining_len >= data_len_max) {
        data_len = data_len_max;
    } else {
        data_len = remaining_len;
    }
    *p_data_len     = data_len;
    p_ym->data_len  = data_len;

    return xf_ret;
}

xf_err_t xf_ymodem_send_get_buf_and_len(
    xf_ymodem_t *p_ym, uint8_t **pp_data_buf, uint32_t *p_buf_size)
{
    xf_err_t xf_ret             = XF_OK;

    XF_CHECK(NULL == p_ym, XF_ERR_INVALID_ARG,
             TAG, "p_ym:%s", xf_err_to_name(XF_ERR_INVALID_ARG));
    XF_CHECK(NULL == pp_data_buf, XF_ERR_INVALID_ARG,
             TAG, "pp_data_buf:%s", xf_err_to_name(XF_ERR_INVALID_ARG));
    XF_CHECK(NULL == p_buf_size, XF_ERR_INVALID_ARG,
             TAG, "p_buf_size:%s", xf_err_to_name(XF_ERR_INVALID_ARG));

    *pp_data_buf = &p_ym->p_buf[XF_YMODEM_DATA_IDX];

    xf_ret = xf_ymodem_send_get_packet_data_len(p_ym, p_buf_size);

    return xf_ret;
}

xf_err_t xf_ymodem_send_data(xf_ymodem_t *p_ym)
{
    xf_err_t    xf_ret          = XF_OK;
    uint8_t     ch              = 0;
    int32_t     retry_for_nak   = 0;

    XF_CHECK(NULL == p_ym, XF_ERR_INVALID_ARG,
             TAG, "p_ym:%s", xf_err_to_name(XF_ERR_INVALID_ARG));

    retry_for_nak       = p_ym->retry_num + 1;

    xf_ret = xf_ymodem_send_regular_packet_data(p_ym, p_ym->data_len);
    if (xf_ret != XF_OK) {
        return xf_ret;
    }

    /* 准备包协议 */
    xf_ret = xf_ymodem_send_prepare_packet_protocol_segment(p_ym);
    if (xf_ret != XF_OK) {
        return xf_ret;
    }

l_retry_for_nak:;
    /* 发送 */
    xf_ret = xf_ymodem_send_packet(p_ym);
    if (xf_ret != XF_OK) {
        return xf_ret;
    }

    /* 获取 ACK 或 NAK */
    xf_ret = xf_ymodem_getc(p_ym, &ch);
    if (xf_ret != XF_OK) {
        return xf_ret;
    }
    switch (ch) {
    case XF_YMODEM_NAK: {
        retry_for_nak--;
        YM_LOGD(TAG, "The peer receives the packet with an error.");
        if (retry_for_nak > 0) {
            goto l_retry_for_nak;
        }
        YM_LOGD(TAG, "The NAK retransmit counter has reached its maximum count.");
        p_ym->error_code    = XF_YMODEM_ERR_NAK_RETRY;
        xf_ret              = XF_ERR_RESOURCE;
        goto l_xf_ret;
    } break;
    case XF_YMODEM_ACK: {
        p_ym->packet_num++;
        p_ym->file_len_transmitted += p_ym->data_len;
    } break;
    case XF_YMODEM_CAN: {
        p_ym->error_code    = XF_YMODEM_ERR_CAN;
        xf_ret              = XF_ERR_RESOURCE;
        goto l_xf_ret;
    } break;
    default: {
        YM_LOGD(TAG, "recv(%02X) Not Supported", (int)ch);
        xf_ret              = XF_FAIL;
        goto l_xf_ret;
    } break;
    }

    if (p_ym->file_len_transmitted >= p_ym->file_len) {
        /* 传输完毕 */
        p_ym->state = XF_YMODEM_SEND_EOT1;

l_retry_for_eot:;
        if ((p_ym->state == XF_YMODEM_SEND_EOT1)
                || (p_ym->state == XF_YMODEM_SEND_EOT2)
           ) {
            xf_ymodem_putc(p_ym, XF_YMODEM_EOT);
        }
        /* 获取 ACK 或 NAK */
        xf_ret = xf_ymodem_getc(p_ym, &ch);
        if (xf_ret != XF_OK) {
            return xf_ret;
        }
        switch (ch) {
        case XF_YMODEM_NAK: {
            if (p_ym->state != XF_YMODEM_SEND_EOT1) {
                YM_LOGD(TAG, "The peer has sent an error signal(%02X).", (int)ch);
                p_ym->error_code    = XF_YMODEM_ERR_HEADER;
                xf_ret              = XF_FAIL;
                goto l_xf_ret;
            }
            p_ym->state = XF_YMODEM_SEND_EOT2;
            goto l_retry_for_eot;
        } break;
        case XF_YMODEM_ACK: {
            if (p_ym->state == XF_YMODEM_SEND_EOT2) {
                p_ym->state = XF_YMODEM_SEND_NULL_FILE_INFO;
                goto l_retry_for_eot;
            } else if (p_ym->state == XF_YMODEM_SEND_NULL_FILE_INFO) {
                p_ym->file_len              = 0;
                p_ym->file_len_transmitted  = 0;
                xf_ymodem_flush_read(p_ym);
                p_ym->state         = XF_YMODEM_SEND_END;
                p_ym->error_code    = XF_YMODEM_OK;
                xf_ret              = XF_ERR_RESOURCE;
                goto l_xf_ret;
            }
            YM_LOGD(TAG, "The peer has sent an error signal(%02X).", (int)ch);
            p_ym->error_code    = XF_YMODEM_ERR_HEADER;
            xf_ret              = XF_FAIL;
            goto l_xf_ret;
        } break;
        case XF_YMODEM_C: {
            if (p_ym->state != XF_YMODEM_SEND_NULL_FILE_INFO) {
                YM_LOGD(TAG, "The peer has sent an error signal(%02X).", (int)ch);
                p_ym->error_code    = XF_YMODEM_ERR_HEADER;
                xf_ret              = XF_FAIL;
                goto l_xf_ret;
            }

            /* 准备空包 */
            p_ym->p_buf[XF_YMODEM_HEADER_IDX] = XF_YMODEM_SOH;
            p_ym->packet_num = 0;
            xf_memset((char *)&p_ym->p_buf[XF_YMODEM_DATA_IDX],
                      0, XF_YMODEM_SOH_DATA_SIZE);
            p_ym->data_len = XF_YMODEM_SOH_DATA_SIZE;
            p_ym->packet_len = p_ym->data_len + XF_YMODEM_PROT_SEG_SIZE;

            /* 准备包协议 */
            xf_ret = xf_ymodem_send_prepare_packet_protocol_segment(p_ym);
            if (xf_ret != XF_OK) {
                xf_ret              = XF_FAIL;
                goto l_xf_ret;
            }

            /* 发送 */
            xf_ret = xf_ymodem_send_packet(p_ym);
            if (xf_ret != XF_OK) {
                xf_ret              = XF_FAIL;
                goto l_xf_ret;
            }

            /* 等待之后一次应答 */
            goto l_retry_for_eot;
        } break;
        default: {
            YM_LOGD(TAG, "recv(%02X) Not Supported", (int)ch);
            xf_ret              = XF_FAIL;
            goto l_xf_ret;
        } break;
        } /* switch */
    }

l_xf_ret:;
    return xf_ret;
}

xf_err_t xf_ymodem_send_regular_packet_data(
    xf_ymodem_t *p_ym, uint32_t data_len)
{
    xf_err_t    xf_ret          = XF_OK;
    uint8_t     header          = 0;
    uint32_t    pad_len         = 0;

    XF_CHECK(NULL == p_ym, XF_ERR_INVALID_ARG,
             TAG, "p_ym:%s", xf_err_to_name(XF_ERR_INVALID_ARG));

    if ((0 < data_len) && (data_len <= XF_YMODEM_SOH_DATA_SIZE)) {
        p_ym->data_len      = XF_YMODEM_SOH_DATA_SIZE;
        header              = XF_YMODEM_SOH;
    } else if ((XF_YMODEM_SOH_DATA_SIZE < data_len) && (data_len <= XF_YMODEM_STX_1K_DATA_SIZE)) {
        p_ym->data_len      = XF_YMODEM_STX_1K_DATA_SIZE;
        header              = XF_YMODEM_STX_1K;
    } else if ((XF_YMODEM_STX_1K_DATA_SIZE < data_len) && (data_len <= XF_YMODEM_STX_2K_DATA_SIZE)) {
        p_ym->data_len      = XF_YMODEM_STX_2K_DATA_SIZE;
        header              = XF_YMODEM_STX_2K;
    } else if ((XF_YMODEM_STX_2K_DATA_SIZE < data_len) && (data_len <= XF_YMODEM_STX_4K_DATA_SIZE)) {
        p_ym->data_len      = XF_YMODEM_STX_4K_DATA_SIZE;
        header              = XF_YMODEM_STX_8K;
    } else if ((XF_YMODEM_STX_4K_DATA_SIZE < data_len) && (data_len <= XF_YMODEM_STX_8K_DATA_SIZE)) {
        p_ym->data_len      = XF_YMODEM_STX_8K_DATA_SIZE;
        header              = XF_YMODEM_SOH;
    } else {
        YM_LOGD(TAG, "p_ym->data_len(%d) Not Supported", (int)p_ym->data_len);
    }
    p_ym->packet_len = p_ym->data_len + XF_YMODEM_PROT_SEG_SIZE;
    if (p_ym->packet_len > p_ym->buf_size) {
        YM_LOGD(TAG, "p_ym->packet_len(%d) Not Supported", (int)p_ym->packet_len);
    }

    p_ym->p_buf[XF_YMODEM_HEADER_IDX]   = header;

    /* 填充无效值 */
    pad_len = p_ym->data_len - data_len;
    if (pad_len > 0) {
        xf_memset(
            (char *)&p_ym->p_buf[XF_YMODEM_DATA_IDX + data_len],
            XF_YMODEM_PAD_VAL, pad_len);
    }

    return xf_ret;
}

xf_err_t xf_ymodem_send_packet(xf_ymodem_t *p_ym)
{
    xf_err_t    xf_ret          = XF_OK;
    int32_t     wlen            = 0;

    XF_CHECK(NULL == p_ym, XF_ERR_INVALID_ARG,
             TAG, "p_ym:%s", xf_err_to_name(XF_ERR_INVALID_ARG));

    wlen = p_ym->ops->write(p_ym->p_buf, p_ym->packet_len, p_ym->timeout_ms);
    if (wlen != p_ym->packet_len) {
        xf_ret = XF_FAIL;
    }

    xf_ymodem_show_packet(p_ym->p_buf, p_ym->data_len);

    return xf_ret;
}

xf_err_t xf_ymodem_send_prepare_packet_protocol_segment(xf_ymodem_t *p_ym)
{
    xf_err_t    xf_ret          = XF_OK;
    uint16_t    crc16           = 0;
    uint8_t     crc16_hi        = 0;
    uint8_t     crc16_lo        = 0;

    XF_CHECK(NULL == p_ym, XF_ERR_INVALID_ARG,
             TAG, "p_ym:%s", xf_err_to_name(XF_ERR_INVALID_ARG));

    p_ym->p_buf[XF_YMODEM_PN_IDX]       = p_ym->packet_num;
    p_ym->p_buf[XF_YMODEM_NPN_IDX]      = ~p_ym->packet_num;

    crc16 = xf_ymodem_crc16(
                XF_YMODEM_CRC_START_VAL_DEFAULT,
                &p_ym->p_buf[XF_YMODEM_DATA_IDX], p_ym->data_len);
    crc16_hi = (crc16 >> 8) & 0xFF;
    crc16_lo = (crc16) & 0xFF;

    p_ym->p_buf[XF_YMODEM_DATA_IDX + p_ym->data_len + 0] = crc16_hi;
    p_ym->p_buf[XF_YMODEM_DATA_IDX + p_ym->data_len + 1] = crc16_lo;

    return xf_ret;
}

xf_err_t xf_ymodem_prepare_file_info(
    xf_ymodem_t *p_ym, xf_ymodem_file_info_t *p_info)
{
    xf_err_t xf_ret         = XF_OK;
    uint32_t file_name_actual_len       = 0;
    uint32_t file_len_str_actual_len    = 0;
    uint32_t buf_idx        = 0;

    XF_CHECK(NULL == p_ym, XF_ERR_INVALID_ARG,
             TAG, "p_ym:%s", xf_err_to_name(XF_ERR_INVALID_ARG));
    XF_CHECK(p_info->p_name_buf == NULL, XF_ERR_INVALID_ARG,
             TAG, "p_info->p_name_buf:%s", xf_err_to_name(XF_ERR_INVALID_ARG));
    XF_CHECK(p_info->buf_size < 1, XF_ERR_INVALID_ARG,
             TAG, "p_info->buf_size:%s", xf_err_to_name(XF_ERR_INVALID_ARG));
    XF_CHECK(p_info->file_len < 1, XF_ERR_INVALID_ARG,
             TAG, "p_info->file_len:%s", xf_err_to_name(XF_ERR_INVALID_ARG));

    /*
        格式:
              <file_name> + '\0'
            + [file_len] + '\0'
            + [(不支持)时间戳] + ' '
            + [(不支持)文件权限] + '\0'
            + 填充 '\0'
     */

    buf_idx                 = XF_YMODEM_DATA_IDX;

#define XF_YMODEM_PT_DATA (XF_YMODEM_SOH_DATA_SIZE + XF_YMODEM_DATA_IDX)

    /*
        TODO 起始帧是否需要支持 XF_YMODEM_STX_DATA_SIZE?
     */

    /* 文件名 */
    file_name_actual_len = xf_strnlen(
                               (const char *)p_info->p_name_buf,
                               XF_YMODEM_SOH_DATA_SIZE - 1);
    xf_strncpy((char *)&p_ym->p_buf[buf_idx],
               p_info->p_name_buf, file_name_actual_len);
    buf_idx += file_name_actual_len;
    p_ym->p_buf[buf_idx] = '\0';
    buf_idx++;
    if (buf_idx >= (XF_YMODEM_PT_DATA - 1)) {
        xf_ret              = XF_FAIL;
        goto l_xf_ret;
    }

    xf_ret = xf_ymodem_u32_to_str(
                 (uint32_t)p_info->file_len, DECIMAL,
                 &p_ym->p_buf[buf_idx], XF_YMODEM_PT_DATA - buf_idx,
                 &file_len_str_actual_len);
    if (xf_ret != XF_OK) {
        goto l_xf_ret;
    }
    buf_idx                += file_len_str_actual_len;
    /* xf_ymodem_u32_to_str 内已经附加了 '\0' */

    p_ym->file_len = p_info->file_len;
    p_ym->file_len_transmitted = 0;

    if ((p_ym->ops->user_file_info) && (buf_idx < (XF_YMODEM_PT_DATA - 1))) {
        buf_idx += p_ym->ops->user_file_info(
                       &p_ym->p_buf[buf_idx],
                       XF_YMODEM_PT_DATA - buf_idx,
                       p_ym->user_data);
    }
    if (buf_idx > (XF_YMODEM_PT_DATA - 1)) {
        xf_ret              = XF_FAIL;
        goto l_xf_ret;
    }

    if (buf_idx < (XF_YMODEM_PT_DATA - 1)) {
        xf_memset((char *)&p_ym->p_buf[buf_idx], 0, XF_YMODEM_PT_DATA - buf_idx);
    }

    p_ym->p_buf[XF_YMODEM_HEADER_IDX] = XF_YMODEM_SOH;
    p_ym->data_len = XF_YMODEM_SOH_DATA_SIZE;
    p_ym->packet_len = p_ym->data_len + XF_YMODEM_PROT_SEG_SIZE;

l_xf_ret:;
    return xf_ret;
}

/* send */

uint16_t xf_ymodem_crc16(uint16_t crc_start, const uint8_t *buf, uint32_t len)
{
    uint16_t crc = crc_start;
    int i;
    while (len--) {
        crc = crc ^ *buf++ << 8;
        for (i = 0; i < 8; i++) {
            if (crc & 0x8000) {
                crc = crc << 1 ^ 0x1021;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

bool xf_ymodem_is_hex(char ch)
{
    if (((ch >= '0') && (ch <= '9'))
            || ((ch >= 'A') && (ch <= 'F'))
            || ((ch >= 'a') && (ch <= 'f'))) {
        return true;
    }
    return false;
}

uint32_t xf_ymodem_convert_hex(char ch)
{
    uint32_t val = 0;
    if ((ch >= '0') && (ch <= '9')) {
        val = (uint32_t)(int32_t)(ch - '0');
    } else if ((ch >= 'a') && (ch <= 'f')) {
        val = (uint32_t)(int32_t)(ch - 'a' + 10); /* 10: add 10 */
    } else if ((ch >= 'A') && (ch <= 'F')) {
        val = (uint32_t)(int32_t)(ch - 'A' + 10); /* 10: add 10 */
    }
    return val;
}

xf_err_t xf_ymodem_str_to_u32(
    const uint8_t *p_str, uint32_t len, uint32_t *p_u32)
{
    const uint8_t *s = p_str;
    uint32_t value = 0;
    uint32_t radix = DECIMAL;
    uint32_t count = len;
    if ((p_str == NULL) || (p_u32 == NULL) || (len == 0)) {
        return XF_ERR_INVALID_ARG;
    }
    /* 跳过空格 */
    while ((*s == ' ') && (count > 0)) {
        s++;
        count--;
    }
    /* 判断 16 进制，16 进制时最少有 "0x" 2 个字符 */
    if ((count > 1)
            && (s[0] == '0')
            && ((s[1] == 'x') || (s[1] == 'X'))) {
        radix       = HEXADECIMAL;
        s           += 2;
        count       -= 2;
    }
    while ((*s != '\0') && (count > 0)) {
        int8_t ch = (int8_t) * s;
        s++;
        if (!xf_ymodem_is_hex(ch)) {
            break;
        }
        uint32_t tmp = xf_ymodem_convert_hex(ch);
        if (tmp <= radix) {
            value *= radix;
            value += tmp;
        }
    }
    *p_u32 = value;
    return XF_OK;
}

xf_err_t xf_ymodem_u32_to_str(
    uint32_t u32_val, uint32_t radix,
    uint8_t *p_buf, uint32_t buf_size, uint32_t *p_len)
{
    char temp_buf[32]   = {0};
    uint32_t temp_idx   = 0;
    uint32_t buf_idx    = 0;

    if ((p_buf == NULL) || (buf_size <= 1)
            || ((radix != DECIMAL) && (radix != HEXADECIMAL))) {
        return XF_ERR_INVALID_ARG;
    }

    if (radix == HEXADECIMAL) {
        if (buf_size < 4) { /* '0' + 'x' + '0~f' + '\0' */
            return XF_FAIL;
        }
        p_buf[buf_idx++] = '0';
        p_buf[buf_idx++] = 'x';
    }
    do {
        uint32_t digit = u32_val % radix;
        temp_buf[temp_idx++] = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
        u32_val /= radix;
    } while (u32_val > 0);
    if (temp_idx + buf_idx > buf_size - 1) {
        return XF_FAIL;
    }
    while (temp_idx > 0) {
        p_buf[buf_idx++] = temp_buf[--temp_idx];
    }
    p_buf[buf_idx] = '\0';
    if (p_len) {
        *p_len = buf_idx + 1;
    }
    return XF_OK;
}

xf_err_t xf_ymodem_show_packet(uint8_t *packet, uint32_t packet_size)
{
    if (packet == NULL) {
        return XF_ERR_INVALID_ARG;
    }
#if XF_YMODEM_DEBUG_IS_ENABLE
    switch (packet[0]) {
    case XF_YMODEM_SOH:
        ym_printf("SOH ");
        break;
    case XF_YMODEM_STX:
        ym_printf("STX ");
        break;
    case XF_YMODEM_EOT:
        ym_printf("EOT ");
        break;
    case XF_YMODEM_ACK:
        ym_printf("ACK ");
        break;
    case XF_YMODEM_NAK:
        ym_printf("NAK ");
        break;
    case XF_YMODEM_CAN:
        ym_printf("CAN ");
        break;
    default:
        break;
    }
    if (packet_size > 3) {
        ym_printf("%02X %02X Data[%d] CRC CRC\r\n", packet[1], packet[2], (int)packet_size);
    } else {
        ym_printf("\r\n");
    }
#endif
    return 0;
}

xf_err_t xf_ymodem_putc(xf_ymodem_t *p_ym, uint8_t ch)
{
    int32_t wlen = 0;

    XF_CHECK(NULL == p_ym, XF_ERR_INVALID_ARG,
             TAG, "p_ym:%s", xf_err_to_name(XF_ERR_INVALID_ARG));

    if (ch == XF_YMODEM_EOT) {
        ym_printf("EOT\r\n");
        wlen = p_ym->ops->write(&ch, 1, p_ym->timeout_ms);
        return (wlen == 1) ? XF_OK : XF_FAIL;
    }

#if XF_YMODEM_DEBUG_IS_ENABLE
    ym_printf("\t\t\t\t");
    switch (ch) {
    case XF_YMODEM_ACK: {
        ym_printf("ACK\r\n");
    } break;
    case XF_YMODEM_NAK: {
        ym_printf("NAK\r\n");
    } break;
    case XF_YMODEM_CAN: {
        ym_printf("CAN\r\n");
    } break;
    case XF_YMODEM_C: {
        ym_printf("C\r\n");
    } break;
    default:
        ym_printf("%c\r\n", ch);
        break;
    }
#endif  /*XF_YMODEM_DEBUG_IS_ENABLE*/

    wlen = p_ym->ops->write(&ch, 1, p_ym->timeout_ms);
    return (wlen == 1) ? XF_OK : XF_FAIL;
}

/* ==================== [Static Functions] ================================== */
