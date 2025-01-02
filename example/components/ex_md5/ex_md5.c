/**
 * @file ex_md5.c
 * @author catcatBlue (catcatblue@qq.com)
 * @brief
 * @version 1.0
 * @date 2024-12-31
 *
 * Copyright (c) 2024, CorAL. All rights reserved.
 *
 */

/* ==================== [Includes] ========================================== */

#include "ex_md5.h"

/* ==================== [Defines] =========================================== */

/* ==================== [Typedefs] ========================================== */

/* ==================== [Static Prototypes] ================================= */

// MD5主处理函数
static xf_err_t ex_md5_transform(uint32_t state[4], const uint8_t block[64]);

/* ==================== [Static Variables] ================================== */

static const char *const TAG = "ex_md5";

/* ==================== [Macros] ============================================ */

// MD5基本运算函数
#define F(x, y, z) ((x & y) | (~x & z))
#define G(x, y, z) ((x & z) | (y & ~z))
#define H(x, y, z) (x ^ y ^ z)
#define I(x, y, z) (y ^ (x | ~z))

#define ROTATE_LEFT(x, n) ((x << n) | (x >> (32 - n)))

#define FF(a, b, c, d, x, s, ac) { \
    a += F(b, c, d) + x + ac;      \
    a = ROTATE_LEFT(a, s);         \
    a += b;                        \
}
#define GG(a, b, c, d, x, s, ac) { \
    a += G(b, c, d) + x + ac;      \
    a = ROTATE_LEFT(a, s);         \
    a += b;                        \
}
#define HH(a, b, c, d, x, s, ac) { \
    a += H(b, c, d) + x + ac;      \
    a = ROTATE_LEFT(a, s);         \
    a += b;                        \
}
#define II(a, b, c, d, x, s, ac) { \
    a += I(b, c, d) + x + ac;      \
    a = ROTATE_LEFT(a, s);         \
    a += b;                        \
}

/* ==================== [Global Functions] ================================== */

xf_err_t ex_md5_init(ex_md5_ctx_t *p_ctx)
{
    XF_CHECK(NULL == p_ctx, XF_ERR_INVALID_ARG,
             TAG, "p_ctx:%s", xf_err_to_name(XF_ERR_INVALID_ARG));

    p_ctx->count[0] = 0;
    p_ctx->count[1] = 0;
    p_ctx->state[0] = 0x67452301;
    p_ctx->state[1] = 0xEFCDAB89;
    p_ctx->state[2] = 0x98BADCFE;
    p_ctx->state[3] = 0x10325476;

    return XF_OK;
}

xf_err_t ex_md5_update(ex_md5_ctx_t *p_ctx, const uint8_t *input, size_t inputLen)
{
    XF_CHECK(NULL == p_ctx, XF_ERR_INVALID_ARG,
             TAG, "p_ctx:%s", xf_err_to_name(XF_ERR_INVALID_ARG));
    XF_CHECK(NULL == input, XF_ERR_INVALID_ARG,
             TAG, "input:%s", xf_err_to_name(XF_ERR_INVALID_ARG));
    XF_CHECK(NULL == input, XF_ERR_INVALID_ARG,
             TAG, "input:%s", xf_err_to_name(XF_ERR_INVALID_ARG));

    size_t i, index, partLen;

    // 计算当前缓冲区的填充位置
    index = (p_ctx->count[0] >> 3) & 0x3F;

    // 更新位数
    if ((p_ctx->count[0] += inputLen << 3) < (inputLen << 3)) {
        p_ctx->count[1]++;
    }
    p_ctx->count[1] += (inputLen >> 29);

    partLen = 64 - index;

    // 处理填满一个缓冲区的情况
    if (inputLen >= partLen) {
        xf_memcpy(&p_ctx->buffer[index], input, partLen);
        ex_md5_transform(p_ctx->state, p_ctx->buffer);
        for (i = partLen; i + 63 < inputLen; i += 64) {
            ex_md5_transform(p_ctx->state, &input[i]);
        }
        index = 0;
    } else {
        i = 0;
    }

    // 剩余未处理的数据
    xf_memcpy(&p_ctx->buffer[index], &input[i], inputLen - i);

    return XF_OK;
}

xf_err_t ex_md5_final(ex_md5_ctx_t *p_ctx, uint8_t digest[16])
{
    XF_CHECK(NULL == digest, XF_ERR_INVALID_ARG,
             TAG, "digest:%s", xf_err_to_name(XF_ERR_INVALID_ARG));
    XF_CHECK(NULL == p_ctx, XF_ERR_INVALID_ARG,
             TAG, "p_ctx:%s", xf_err_to_name(XF_ERR_INVALID_ARG));

    uint8_t bits[8];
    size_t index, padLen;
    static const uint8_t PADDING[64] = { 0x80 };

    // 保存位数
    xf_memcpy(bits, p_ctx->count, 8);

    // 填充缓冲区
    index = (p_ctx->count[0] >> 3) & 0x3F;
    padLen = (index < 56) ? (56 - index) : (120 - index);
    ex_md5_update(p_ctx, PADDING, padLen);

    // 添加位数
    ex_md5_update(p_ctx, bits, 8);

    // 保存最终的MD5值
    xf_memcpy(digest, p_ctx->state, 16);

    return XF_OK;
}

/* ==================== [Static Functions] ================================== */

// MD5主处理函数
static xf_err_t ex_md5_transform(uint32_t state[4], const uint8_t block[64])
{
    XF_CHECK(NULL == state, XF_ERR_INVALID_ARG,
             TAG, "state:%s", xf_err_to_name(XF_ERR_INVALID_ARG));
    XF_CHECK(NULL == block, XF_ERR_INVALID_ARG,
             TAG, "block:%s", xf_err_to_name(XF_ERR_INVALID_ARG));

    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    uint32_t x[16];

    xf_memcpy(x, block, 64);

    // 第一轮
    FF(a, b, c, d, x[0], 7, 0xd76aa478);
    FF(d, a, b, c, x[1], 12, 0xe8c7b756);
    FF(c, d, a, b, x[2], 17, 0x242070db);
    FF(b, c, d, a, x[3], 22, 0xc1bdceee);
    FF(a, b, c, d, x[4], 7, 0xf57c0faf);
    FF(d, a, b, c, x[5], 12, 0x4787c62a);
    FF(c, d, a, b, x[6], 17, 0xa8304613);
    FF(b, c, d, a, x[7], 22, 0xfd469501);
    FF(a, b, c, d, x[8], 7, 0x698098d8);
    FF(d, a, b, c, x[9], 12, 0x8b44f7af);
    FF(c, d, a, b, x[10], 17, 0xffff5bb1);
    FF(b, c, d, a, x[11], 22, 0x895cd7be);
    FF(a, b, c, d, x[12], 7, 0x6b901122);
    FF(d, a, b, c, x[13], 12, 0xfd987193);
    FF(c, d, a, b, x[14], 17, 0xa679438e);
    FF(b, c, d, a, x[15], 22, 0x49b40821);

    // 第二轮
    GG(a, b, c, d, x[1], 5, 0xf61e2562);
    GG(d, a, b, c, x[6], 9, 0xc040b340);
    GG(c, d, a, b, x[11], 14, 0x265e5a51);
    GG(b, c, d, a, x[0], 20, 0xe9b6c7aa);
    GG(a, b, c, d, x[5], 5, 0xd62f105d);
    GG(d, a, b, c, x[10], 9, 0x02441453);
    GG(c, d, a, b, x[15], 14, 0xd8a1e681);
    GG(b, c, d, a, x[4], 20, 0xe7d3fbc8);
    GG(a, b, c, d, x[9], 5, 0x21e1cde6);
    GG(d, a, b, c, x[14], 9, 0xc33707d6);
    GG(c, d, a, b, x[3], 14, 0xf4d50d87);
    GG(b, c, d, a, x[8], 20, 0x455a14ed);
    GG(a, b, c, d, x[13], 5, 0xa9e3e905);
    GG(d, a, b, c, x[2], 9, 0xfcefa3f8);
    GG(c, d, a, b, x[7], 14, 0x676f02d9);
    GG(b, c, d, a, x[12], 20, 0x8d2a4c8a);

    // 第三轮
    HH(a, b, c, d, x[5], 4, 0xfffa3942);
    HH(d, a, b, c, x[8], 11, 0x8771f681);
    HH(c, d, a, b, x[11], 16, 0x6d9d6122);
    HH(b, c, d, a, x[14], 23, 0xfde5380c);
    HH(a, b, c, d, x[1], 4, 0xa4beea44);
    HH(d, a, b, c, x[4], 11, 0x4bdecfa9);
    HH(c, d, a, b, x[7], 16, 0xf6bb4b60);
    HH(b, c, d, a, x[10], 23, 0xbebfbc70);
    HH(a, b, c, d, x[13], 4, 0x289b7ec6);
    HH(d, a, b, c, x[0], 11, 0xeaa127fa);
    HH(c, d, a, b, x[3], 16, 0xd4ef3085);
    HH(b, c, d, a, x[6], 23, 0x04881d05);
    HH(a, b, c, d, x[9], 4, 0xd9d4d039);
    HH(d, a, b, c, x[12], 11, 0xe6db99e5);
    HH(c, d, a, b, x[15], 16, 0x1fa27cf8);
    HH(b, c, d, a, x[2], 23, 0xc4ac5665);

    // 第四轮
    II(a, b, c, d, x[0], 6, 0xf4292244);
    II(d, a, b, c, x[7], 10, 0x432aff97);
    II(c, d, a, b, x[14], 15, 0xab9423a7);
    II(b, c, d, a, x[5], 21, 0xfc93a039);
    II(a, b, c, d, x[12], 6, 0x655b59c3);
    II(d, a, b, c, x[3], 10, 0x8f0ccc92);
    II(c, d, a, b, x[10], 15, 0xffeff47d);
    II(b, c, d, a, x[1], 21, 0x85845dd1);
    II(a, b, c, d, x[8], 6, 0x6fa87e4f);
    II(d, a, b, c, x[15], 10, 0xfe2ce6e0);
    II(c, d, a, b, x[6], 15, 0xa3014314);
    II(b, c, d, a, x[13], 21, 0x4e0811a1);
    II(a, b, c, d, x[4], 6, 0xf7537e82);
    II(d, a, b, c, x[11], 10, 0xbd3af235);
    II(c, d, a, b, x[2], 15, 0x2ad7d2bb);
    II(b, c, d, a, x[9], 21, 0xeb86d391);

    // 更新状态
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;

    return XF_OK;
}

#if 0

// 测试代码
int main()
{
    ex_md5_ctx_t md5_ctx;
    uint8_t digest[16];
    const char *data = "Hello, world!";
    size_t len = strlen(data);

    ex_md5_init(&md5_ctx);
    ex_md5_update(&md5_ctx, (const uint8_t *)data, len);
    ex_md5_final(&md5_ctx, digest);

    printf("MD5: ");
    for (int i = 0; i < 16; i++) {
        printf("%02x", digest[i]);
    }
    printf("\n");

    return 0;
}

#endif
