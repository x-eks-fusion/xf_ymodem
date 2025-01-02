/**
 * @file xf_ymodem_types.h
 * @author catcatBlue (catcatblue@qq.com)
 * @brief xf_ymodem 公共类型。
 * @version 1.0
 * @date 2025-01-02
 *
 * Copyright (c) 2025, CorAL. All rights reserved.
 *
 */

#ifndef __XF_YMODEM_TYPES_H__
#define __XF_YMODEM_TYPES_H__

/* ==================== [Includes] ========================================== */

#include "xf_utils.h"
#include "xf_ymodem_config_internel.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== [Defines] =========================================== */

#define XF_YMODEM_SOH                   0x01    /*!< 133 字节长度帧头 */
#define XF_YMODEM_STX                   0x02    /*!< 1024 字节长度帧头 */
#define XF_YMODEM_EOT                   0x04    /*!< 文件传输结束命令 */
#define XF_YMODEM_ACK                   0x06    /*!< 接受正确应答命令 */
#define XF_YMODEM_NAK                   0x15    /*!< 重传当前数据包请求命令 */
#define XF_YMODEM_CAN                   0x18    /*!< 取消传输命令，连续发送 5 个该命令 */
#define XF_YMODEM_C                     0x43    /*!< 字符 C */

#define XF_YMODEM_STX_1K                XF_YMODEM_STX
#define XF_YMODEM_STX_2K                0x0a    /*!< 非标, 包数据长 2048 字节 */
#define XF_YMODEM_STX_4K                0x0b    /*!< 非标, 包数据长 4096 字节  */
#define XF_YMODEM_STX_8K                0x0c    /*!< 非标, 包数据长 8192 字节  */

#define XF_YMODEM_PAD_VAL               (0x1a)  /*!< 填充值  */

#define XF_YMODEM_HEADER_SIZE           (1)     /*!< SOH, STX, EOT, ... */
#define XF_YMODEM_PN_SIZE               (2)     /*!< packet num size + ~packet num size */
#define XF_YMODEM_CRC_SIZE              (2)     /*!< crc_h, crc_l */
#define XF_YMODEM_SOH_DATA_SIZE         (128)
#define XF_YMODEM_STX_DATA_SIZE         (1024)
#define XF_YMODEM_STX_1K_DATA_SIZE      (XF_YMODEM_STX_DATA_SIZE)
#define XF_YMODEM_STX_2K_DATA_SIZE      (XF_YMODEM_STX_DATA_SIZE * 2)
#define XF_YMODEM_STX_4K_DATA_SIZE      (XF_YMODEM_STX_DATA_SIZE * 4)
#define XF_YMODEM_STX_8K_DATA_SIZE      (XF_YMODEM_STX_DATA_SIZE * 8)

/**
 * @brief 协议段大小。
 */
#define XF_YMODEM_PROT_SEG_SIZE         (XF_YMODEM_HEADER_SIZE \
                                            + XF_YMODEM_PN_SIZE \
                                            + XF_YMODEM_CRC_SIZE)     /*!< Protocol segment */
/**
 * @brief SOH 包大小。
 */
#define XF_YMODEM_SOH_PACKET_SIZE       (XF_YMODEM_HEADER_SIZE \
                                            + XF_YMODEM_PN_SIZE \
                                            + XF_YMODEM_SOH_DATA_SIZE \
                                            + XF_YMODEM_CRC_SIZE)
/**
 * @brief STX 包大小。
 */
#define XF_YMODEM_STX_PACKET_SIZE       (XF_YMODEM_HEADER_SIZE \
                                            + XF_YMODEM_PN_SIZE \
                                            + XF_YMODEM_STX_DATA_SIZE \
                                            + XF_YMODEM_CRC_SIZE)

#define XF_YMODEM_HEADER_IDX            (0)     /*!< 包头索引 */
#define XF_YMODEM_PN_IDX                (1)     /*!< 包号索引 */
#define XF_YMODEM_NPN_IDX               (2)     /*!< 包号（取反）索引 */
#define XF_YMODEM_DATA_IDX              (3)     /*!< 数据索引 */

#define XF_YMODEM_CRC_START_VAL_DEFAULT (0)     /*!< crc 默认起始值 */

/* ==================== [Typedefs] ========================================== */

/**
 * @brief 对接 xf_ymodem 的操作。
 *
 * 必须实现: read, write, flush, delay_ms.
 * 可选的实现: user_parse, user_file_info.
 *
 */
typedef struct _xf_ymodem_ops_t {
    /**
     * @brief xf_ymodem 读取操作。
     * 
     * @param dst           xf_ymodem 提供的需要读取到此的缓冲区。
     * @param size          需要读取的字节数。
     * @param timeout_ms    超时时间，见 @ref xf_ymodem_t.timeout_ms .
     * @return int32_t      实际读取的字节数。
     *      - (<=0)         读取错误
     *      - (>0)          实际读取的字节数
     */
    int32_t (*read)(void *dst, uint32_t size, uint32_t timeout_ms);
    /**
     * @brief xf_ymodem 输出操作。
     * 
     * @param src           xf_ymodem 提供的需要输出的缓冲区。
     * @param size          缓冲区大小。单位字节。
     * @param timeout_ms    超时时间，见 @ref xf_ymodem_t.timeout_ms .
     *                      通常不需要使用超时时间, xf_ymodem 内部默认输出一定成功。
     * @return int32_t      实际输出的字节数。
     *      - (<=0)         输出错误
     *      - (>0)          实际输出的字节数，通常等于 size
     */
    int32_t (*write)(const void *src, uint32_t size, uint32_t timeout_ms);
    /**
     * @brief 刷新用户读取缓冲区。
     * @note 虽然可以使用 read 实现，但是此接口还有 xf_ymodem 需要重置缓冲区数据的含义
     */
    void (*flush)(void);
    /**
     * @brief 延迟指定 ms.
     * @param ms 需要延迟的 ms 数。
     * @note delay_ms 仅在运行接收端时，当接收到的数据 CRC 校验错误，发送 NAK 让发送端重发时
     *       使用，此时延迟 xf_ymodem_t.timeout_ms 时间，确保发送端能收到 NAK 信号。 
     */
    void (*delay_ms)(uint32_t ms);
    /**
     * @brief 用户自定义文件信息解析函数。
     * 
     * @note 此实现是可选的。
     * @note 当 xf_ymodem 作为接收端，接收到起始帧时，
     *       解析完文件名及文件长度后(并后移一个字节)，会调用此回调。
     * @code
     * <file_name> + '\0' + [file_len] + '\0' + ...
     *                                           ^ p_remaining_buf
     * @endcode
     * 
     * @param p_remaining_buf   xf_ymodem 提供的剩余字符串的缓冲区。
     * @param remaining_size    缓冲区的大小。单位字节。
     * @param user_data         用户数据，见 xf_ymodem_t.user_data .
     */
    void (*user_parse)(uint8_t *p_remaining_buf, uint32_t remaining_size, void *user_data);
    /**
     * @brief 用户自定义文件信息填充函数。
     * 
     * @note 此实现是可选的。
     * @note 当 xf_ymodem 作为发送端，需要发送起始帧时，
     *       填充完文件名及文件长度后(并后移一个字节)，会调用此回调。
     * @code
     * <file_name> + '\0' + [file_len] + '\0' + ...
     *                                           ^ p_remaining_buf
     * @endcode
     * 
     * @param p_remaining_buf   xf_ymodem 提供的剩余字符串的缓冲区。
     * @param remaining_size    缓冲区的大小。单位字节。
     * @param user_data         用户数据，见 xf_ymodem_t.user_data .
     */
    uint32_t (*user_file_info)(uint8_t *p_remaining_buf, uint32_t remaining_size, void *user_data);
} xf_ymodem_ops_t;

/**
 * @brief xf_ymodem 额外错误码枚举。
 */
typedef enum _xf_ymodem_err_code_t {
    XF_YMODEM_OK,                               /*!< 正常 */

    XF_YMODEM_ERR_INVALID_FILE_NAME,            /*!< 无效的文件名 */
    XF_YMODEM_ERR_NO_DATA,                      /*!< 指定重试次数内没有收到数据 */
    XF_YMODEM_ERR_PN,                           /*!< 指定重试次数内包号校验错误 */
    XF_YMODEM_ERR_CRC,                          /*!< 指定重试次数内 CRC 校验错误 */
    XF_YMODEM_ERR_CAN,                          /*!< 对方已取消 */
    XF_YMODEM_ERR_NAK_RETRY,                    /*!< NAK 重发数据包达到最大次数 */
    XF_YMODEM_ERR_HEADER,                       /*!< 接收端发送了错误信号 */

    XF_YMODEM_ERR_MAX,                          /*!< 最大值 */
} xf_ymodem_err_code_t;

/**
 * @brief xf_ymodem 额外错误码类型。
 * 额外指的是 对 xf_err_t 定义的错误码外的补充。
 */
typedef uint8_t xf_ymodem_err_t;

/**
 * @brief xf_ymodem 状态码类型。
 */
typedef enum _xf_ymodem_state_code_t {
    XF_YMODEM_NONE                      = 0x00,

    /* 1. 接收文件 */
    /* 1.1. 首次发送 C, 请求发送端发送类型为 SOH 的，包含文件名及长度的起始帧(代码中没有使用) */
    XF_YMODEM_RECV_REQUEST_FILE_INFO,
    /* 1.1. 已接收到起始帧 */
    XF_YMODEM_RECV_FILE_INFO_AVAILABLE,
    /* 1.1. 再次发送 C, 请求发送端发送文件内容 */
    XF_YMODEM_RECV_REQUEST_FILE_DATA,
    /* 1.1. 首次接收到 EOT, 需要发送 NAK */
    XF_YMODEM_RECV_GOT_EOT1,
    /* 1.1. 再次接收到 EOT, 需要发送 ACK */
    XF_YMODEM_RECV_GOT_EOT2,
    /* 1.1. 需要发送 C 反馈 EOT2, 之后再收一包文件信息，如果是空包则无下一个文件 */
    XF_YMODEM_RECV_FEEDBACK_EOT2,
    /* 1.1. 接收流程已结束 */
    XF_YMODEM_RECV_END,

    /* 2. 发送文件 */
    /* 2.1. 首次接收到 C, 可以发送起始帧 */
    XF_YMODEM_SEND_FILE_INFO,
    /* 2.1. 需要传送文件数据，并接收 ACK 或 NAK */
    XF_YMODEM_SEND_FILE_DATA,
    /* 2.1. 文件已传送完，已接收到 ACK, 需要发送 EOT 一次 */
    XF_YMODEM_SEND_EOT1,
    /* 2.1. 发送 EOT1 后收到 NAK, 需要再发 EOT 一次 */
    XF_YMODEM_SEND_EOT2,
    /* 2.1. 发送 EOT2 后收到 ACK, 需要接收 C 后发送空的起始帧 */
    XF_YMODEM_SEND_NULL_FILE_INFO,
    /* 2.1. 已收到最后一次应答，发送流程结束 */
    XF_YMODEM_SEND_END,

    XF_YMODEM_MAX,
} xf_ymodem_state_code_t;

/**
 * @brief xf_ymodem 对象容器类型。
 */
typedef struct _xf_ymodem_t {
    /**
     * @name 用户初始化区
     * @{
     */
    /**
     * @brief 指向缓冲区的指针。
     * xf_ymodem 模块将使用此缓冲区收发包。
     */
    uint8_t                *p_buf;
    /**
     * @brief 缓冲区大小。
     *  - 最小大小: XF_YMODEM_SOH_PACKET_SIZE.
     *  - buf_size 大于等于 XF_YMODEM_PROT_SEG_SIZE + XF_YMODEM_STX_1K_DATA_SIZE(2K, 4K, 8K)
     *    时，接收模式下自动支持标准 1K(1024 bytes) 数据段长，或非标的 2K, 4K, 8K 包长。
     *  - 如果不希望或发送非标的 2K, 4K, 8K 包长:
     *    buf_size <= XF_YMODEM_STX_PACKET_SIZE
     */
    uint32_t                buf_size;
    /**
     * @brief 每个操作的重试次数。为 N 时每个操作尝试 N + 1 次。
     */
    uint32_t                retry_num;
    /**
     * @brief 传给 xf_ymodem_ops_t.read 或 xf_ymodem_ops_t.write 的超时时间。
     */
    uint32_t                timeout_ms;
    /**
     * @brief 传给 xf_ymodem_ops_t.user_parse
     *        或 xf_ymodem_ops_t.user_file_info 的用户自定义数据。
     */
    void                   *user_data;
    /**
     * @brief 提供给 xf_ymodem 模块的操作。
     */
    const xf_ymodem_ops_t  *ops;
    /**
     * End of 用户初始化区
     * @}
     */

    /**
     * @name xf_ymodem私有区
     * @brief 用户只能读取，禁止修改。
     * @{
     */
    /* private: */
    uint32_t                packet_len; /*!< 当前包总长，含协议段等内容，可能为 1 */
    uint32_t                data_len;   /*!< 当前包数据段长 */
    int32_t                 file_len;   /*!< 当前传输事务文件长度 */
    int32_t                 file_len_transmitted;   /*!< 当前传输事务文件已传输的长度，
                                                     *    xf_ymodem 自动增加。
                                                     */
    xf_ymodem_err_t         error_code; /*!< 额外错误码 */
    uint8_t                 state;      /*!< xf_ymodem 当前状态码 */
    uint8_t                 packet_num; /*!< xf_ymodem 传输包号计数 */
    uint8_t                 tx_ack;     /*!< (用户无需读取)xf_ymodem 做接收端时发送应答信号标志 */
    /**
     * End of xf_ymodem私有区
     * @}
     */
} xf_ymodem_t;

/**
 * @brief xf_ymodem 起始帧文件信息。
 */
typedef struct _xf_ymodem_file_info_t {
    char       *p_name_buf;             /*!< 指向文件名缓冲区 */
    uint32_t    buf_size;               /*!< 文件名缓冲区大小, xf_ymodem 内自动添加 '\0' */
    int32_t     file_len;               /*!< 文件数据长度，单位字节 */
} xf_ymodem_file_info_t;

/* ==================== [Global Prototypes] ================================= */

/* ==================== [Macros] ============================================ */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __XF_YMODEM_TYPES_H__ */
