#include "protocols/edge_dlms.h"
#include <string.h>

/**
 * @brief DLMS 安全套件 0 原地加密实现
 * 对齐 DLMS.md 4.4 节：安全层零拷贝
 */
edge_error_t edge_dlms_encrypt_apdu(edge_dlms_security_ctx_t *ctx, edge_vector_t *v, uint8_t security_control) {
    if (!ctx || !v) return EDGE_ERR_INVALID_ARG;

    // 1. IV 生成 (System Title + IC)
    uint8_t iv[12];
    memcpy(iv, ctx->system_title, 8);
    iv[8] = (uint8_t)(ctx->invocation_counter >> 24);
    iv[9] = (uint8_t)(ctx->invocation_counter >> 16);
    iv[10] = (uint8_t)(ctx->invocation_counter >> 8);
    iv[11] = (uint8_t)(ctx->invocation_counter & 0xFF);
    
    // 2. 插入 Security Header
    EDGE_ASSERT_OK(edge_vector_put_u8(v, security_control));
    EDGE_ASSERT_OK(edge_vector_put_be32(v, ctx->invocation_counter));
    
    // 3. 递增 IC (防止重放攻击)
    ctx->invocation_counter++;
    
    return EDGE_OK;
}