/**
 * @file xf_ymodem_example_sender.c
 * @author catcatBlue (catcatblue@qq.com)
 * @brief
 * @version 1.0
 * @date 2025-01-01
 *
 * Copyright (c) 2025, CorAL. All rights reserved.
 *
 */

/* ==================== [Includes] ========================================== */

#include "xf_utils.h"
#include "xf_osal.h"
#include "xf_hal.h"
#include "xf_sys.h"
#include "xf_ymodem.h"

#include "ex_md5.h"

#include "xf_ymodem_example.h"

/* ==================== [Defines] =========================================== */

/* ==================== [Typedefs] ========================================== */

/* ==================== [Static Prototypes] ================================= */

static void app_ymodem_uart_setup(void);
static void app_init_xf_ymodem(void);
static int32_t port_xf_ymodem_write(const void *src, uint32_t size, uint32_t timeout_ms);
static int32_t port_xf_ymodem_read(void *dst, uint32_t size, uint32_t timeout_ms);
static void port_xf_ymodem_flush(void);
static void port_xf_ymodem_delay_ms(uint32_t ms);

static xf_err_t app_cyclic_fill_buffer(
    const uint8_t *p_src_buf, uint32_t src_buf_size,
    uint8_t       *p_dst_buf, uint32_t dst_buf_size, uint32_t dst_len_transmitted);

/* ==================== [Static Variables] ================================== */

static const char *const TAG = "sender";

static xf_ymodem_t s_ymodem = {0};
static xf_ymodem_t *sp_ym = &s_ymodem;
/* 不需要预留尾随 '\0' */
static uint8_t s_ym_buf[XF_YMODEM_STX_PACKET_SIZE] = {0};

static const xf_ymodem_ops_t port_xf_ymodem_ops = {
    .read           = port_xf_ymodem_read,
    .write          = port_xf_ymodem_write,
    .flush          = port_xf_ymodem_flush,
    .delay_ms       = port_xf_ymodem_delay_ms,
    .user_parse     = NULL,                     /*!< 可以没有用户自定义解析函数 */
    .user_file_info = NULL,                     /*!< 可以没有用户自定义文件信息填充函数 */
};

static const char sc_file_data[] = {
    "1234567890123456789012345678901234567890123456789012345678901234567890"
    "1234567890123456789012345678901234567890123456789012345678901234567890"
    "1234567890123456789012345678901234567890123456789012345678901234567890"
    "1234567890123456789012345678901234567890123456789012345678901234567890"
    "1234567890123456789012345678901234567890123456789012345678901234567890"
    "1234567890123456789012345678901234567890123456789012345678901234567890"
    "1234567890123456789012345678901234567890123456789012345678901234567890"
    "1234567890123456789012345678901234567890123456789012345678901234567890"
    "1234567890123456789012345678901234567890123456789012345678901234567890"
    "1234567890123456789012345678901234567890123456789012345678901234567890"
    "1234567890123456789012345678901234567890123456789012345678901234567890"
    "1234567890123456789012345678901234567890123456789012345678901234567890"
    "1234567890123456789012345678901234567890123456789012345678901234567890"
    "1234567890123456789012345678901234567890123456789012345678901234567890"
    "1234567890123456789012345678901234567890123456789012345678901234567890"
    "1234567890123456789012345678901234567890123456789012345678901234567890"
    "1234567890123456789012345678901234567890123456789012345678901234567890"
    "1234567890123456789012345678901234567890123456789012345678901234567890"
    "1234567890123456789012345678901234567890123456789012345678901234567890"
    "1234567890123456789012345678901234567890123456789012345678901234567890"
    "1234567890123456789012345678901234567890123456789012345678901234567890"
    "1234567890123456789012345678901234567890123456789012345678901234567890"
    "1234567890123456789012345678901234567890123456789012345678901234567890"
    "1234567890123456789012345678901234567890123456789012345678901234567890"
    "1234567890123456789012345678901234567890123456789012345678901234567890"
    "1234567890123456789012345678901234567890123456789012345678901234567890"
    "1234567890123456789012345678901234567890123456789012345678901234567890"
    "1234567890123456789012345678901234567890123456789012345678901234567890"
    "1234567890123456789012345678901234567890123456789012345678901234567890"
    "12345678901234567890"
};

/* ==================== [Macros] ============================================ */

/* ==================== [Global Functions] ================================== */

void xf_ymodem_example_sender(void)
{
    xf_err_t xf_ret = XF_OK;

    xf_ymodem_t *p_ym               = sp_ym;

    char file_name[65]              = {"1-1234567890.md"};

    xf_ymodem_file_info_t file_info = {0};
    file_info.p_name_buf            = file_name;
    file_info.buf_size              = xf_strlen(file_name);
    /* 可以修改更大(如 1*1024*1024)，app_cyclic_fill_buffer 将循环读取 sc_file_data */
    file_info.file_len              = (ARRAY_SIZE(sc_file_data) - 1);

    uint8_t *p_buf                  = NULL;
    uint32_t buf_size               = 0;

    ex_md5_ctx_t md5_ctx;
    uint8_t digest[MD5_MAX_LEN]     = {0};
    char digest_str[MD5_MAX_LEN * 2 + 1] = {0};

    app_init_xf_ymodem();
    xf_sys_watchdog_disable();

    XF_LOGI(TAG, "file_name:    %s", file_info.p_name_buf);
    XF_LOGI(TAG, "file_len:     %d", (int)file_info.file_len);

    while (1) {
        xf_ret = xf_ymodem_send_handshake(p_ym, &file_info);
        if (xf_ret != XF_OK) {
            xf_osal_delay_ms(100);
            continue;
        }

        ex_md5_init(&md5_ctx);
        /* 发送文件数据 */
        while (1) {
            xf_ret = xf_ymodem_send_get_buf_and_len(p_ym, &p_buf, &buf_size);
            if (xf_ret != XF_OK) {
                /* 出错了或者没有数据可发了 */
                XF_LOGE(TAG, "xf_ret:%s", xf_err_to_name(xf_ret));
                XF_LOGE(TAG, "p_ym->state:%d", (int)p_ym->state);
                break;
            }

            app_cyclic_fill_buffer(
                (uint8_t *)sc_file_data, (ARRAY_SIZE(sc_file_data) - 1),
                p_buf, buf_size, p_ym->file_len_transmitted);

            ex_md5_update(&md5_ctx, (const uint8_t *)p_buf, buf_size);

            XF_LOGI(TAG, "progress: %8d/%d", (int)p_ym->file_len_transmitted + buf_size, (int)p_ym->file_len);
            xf_ret = xf_ymodem_send_data(p_ym);
            if (xf_ret != XF_OK) {
                break;
            }
        }

        /* 处理传输结果 */
        if (xf_ret == XF_ERR_RESOURCE) {
            switch (p_ym->error_code) {
            case XF_YMODEM_OK: {
                /* 正常结束，已发送完所有数据 */
                XF_LOGI(TAG, "All have been sent.");
            } break;
            case XF_YMODEM_ERR_CAN: {
                /* 对方已取消 */
                XF_LOGW(TAG, "The peer has canceled reception.");
            } break;
            case XF_YMODEM_ERR_NO_DATA: {
                /* 指定时间内未收到信号，对方已离线 */
                XF_LOGW(TAG, "The peer end is offline.");
            } break;
            default: {
                XF_LOGW(TAG, "p_ym->error_code:%d", (int)p_ym->error_code);
            } break;
            }
        } else if (xf_ret == XF_ERR_TIMEOUT) {
            /* 指定时间内未收到数据，对方已离线 */
            XF_LOGW(TAG, "The peer end is offline.");
        } else {
            XF_LOGW(TAG, "xf_ret:%s", xf_err_to_name(xf_ret));
            XF_LOGW(TAG, "p_ym->state:%d", (int)p_ym->state);
        }
    
        ex_md5_final(&md5_ctx, digest);

        for (int i = 0; i < MD5_MAX_LEN; i++) {
            xf_sprintf(&digest_str[i * 2], "%02x", (unsigned int)digest[i]);
        }
        XF_LOGI(TAG, "Computed MD5 hash: %s", digest_str);

        xf_osal_delay_ms(3 * 1000);
    }
}

/* ==================== [Static Functions] ================================== */

static void app_init_xf_ymodem(void)
{
    xf_err_t xf_ret = XF_OK;

    app_ymodem_uart_setup();

    /* 初始化 ymodem 对象 */
    sp_ym->p_buf        = s_ym_buf;
    sp_ym->buf_size     = ARRAY_SIZE(s_ym_buf);
    sp_ym->retry_num    = 10;
    sp_ym->timeout_ms   = 50;
    sp_ym->user_data    = NULL;
    sp_ym->ops          = &port_xf_ymodem_ops;

    xf_ret = xf_ymodem_check(sp_ym);
    XF_ERROR_CHECK(xf_ret);
}

static void app_ymodem_uart_setup(void)
{
    xf_hal_uart_init(UART_NUM, UART_BAUDRATE);
    xf_hal_uart_set_gpio(UART_NUM, UART_TX_PIN, UART_RX_PIN);
    xf_hal_uart_enable(UART_NUM);
}

static int32_t port_xf_ymodem_write(const void *src, uint32_t size, uint32_t timeout_ms)
{
    UNUSED(timeout_ms);
    int32_t wlen;
    wlen = xf_hal_uart_write(UART_NUM, src, size);
    return wlen;
}

static int32_t port_xf_ymodem_read(void *dst, uint32_t size, uint32_t timeout_ms)
{
    int32_t rlen_real = 0;
    int32_t rlen = 0;
    uint32_t time_ms = 0;
    uint32_t time_start_ms = (uint32_t)xf_sys_time_get_ms();
    uint32_t time_end_ms = time_start_ms + timeout_ms;

    do {
        rlen = xf_hal_uart_read(
                   UART_NUM, (uint8_t *)dst + rlen_real, size - rlen_real);
        if (rlen > 0) {
            rlen_real += rlen;
        }
        if (rlen_real >= (int32_t)size) {
            break;
        }
        /*
            esp32 测试数据
            1.  `xf_osal_delay_ms(1);`
                太慢(921600, 正常约 52 KBps, 此方式只有 40 KBps)。
            2. `xf_sys_watchdog_kick(); xf_osal_thread_yield();`
                没用
            3. 每 20 ms 睡眠一次，速度约 47.6 KBps(20MB 数据量)，看门狗不会叫。
            4. 关闭看门狗。
         */
        time_ms = xf_sys_time_get_ms();
        if ((time_ms - time_start_ms + 1) % 20 == 0) {
            xf_osal_delay_ms(1);
        }
    } while (time_ms < time_end_ms);

    return rlen_real;
}

static void port_xf_ymodem_flush(void)
{
    int32_t rlen = 0;
    uint8_t dummy_buf[32]   = {0};
    while (1) {
        rlen = xf_hal_uart_read(UART_NUM, dummy_buf, ARRAY_SIZE(dummy_buf));
        if (rlen <= 0) {
            break;
        }
    }
}

static void port_xf_ymodem_delay_ms(uint32_t ms)
{
    xf_osal_delay_ms(ms);
}

static xf_err_t app_cyclic_fill_buffer(
    const uint8_t *p_src_buf, uint32_t src_buf_size,
    uint8_t       *p_dst_buf, uint32_t dst_buf_size, uint32_t dst_len_transmitted)
{
    XF_CHECK(NULL == p_src_buf, XF_ERR_INVALID_ARG,
             TAG, "p_src_buf:%s", xf_err_to_name(XF_ERR_INVALID_ARG));
    XF_CHECK(0 == src_buf_size, XF_ERR_INVALID_ARG,
             TAG, "src_buf_size:%s", xf_err_to_name(XF_ERR_INVALID_ARG));
    XF_CHECK(NULL == p_dst_buf, XF_ERR_INVALID_ARG,
             TAG, "p_dst_buf:%s", xf_err_to_name(XF_ERR_INVALID_ARG));
    if (dst_buf_size == 0) {
        return XF_OK;
    }
    uint32_t len_curr2end = (dst_len_transmitted % dst_buf_size); // 当前从 p_src_buf 开始读取的位置
    uint32_t len_start2curr = dst_buf_size - len_curr2end;        // 当前剩余可用字节数
    if (len_start2curr < dst_buf_size) {
        // 如果剩余数据不足
        xf_memcpy(p_dst_buf, p_src_buf + len_curr2end, len_start2curr); // 先拷贝剩余的部分
        xf_memcpy(p_dst_buf + len_start2curr, p_src_buf, dst_buf_size - len_start2curr); // 从头开始补齐
    } else {
        // 如果剩余数据足够
        xf_memcpy(p_dst_buf, p_src_buf + len_curr2end, dst_buf_size);
    }
    return XF_OK;
}
