#include "protocols/edge_dlms.h"
#include <string.h>

/**
 * @brief 工业级分块管理逻辑 (库内部实现)
 * 移除了对外部 net_buf_t 的依赖
 */
edge_error_t edge_dlms_manage_blocks(edge_dlms_context_t *ctx, const uint8_t *pdu, size_t len) {
    if (!ctx || !pdu) return EDGE_ERR_INVALID_ARG;
    
    // 逻辑：识别 PDU 是否包含分块标志 (0x02)
    if (len > 2 && pdu[0] == DLMS_APDU_GET_RESPONSE && pdu[1] == 0x02) {
        ctx->block_ctx.active = true;
        // 提取 last-block 标志
        bool is_last = (pdu[2] != 0);
        return is_last ? EDGE_OK : (edge_error_t)1;
    }
    
    return EDGE_OK;
}

edge_error_t edge_dlms_build_get_response(edge_dlms_context_t *ctx, edge_vector_t *v, uint8_t result, const edge_dlms_variant_t *val) {
    (void)ctx;
    EDGE_ASSERT_OK(edge_vector_put_u8(v, (uint8_t)DLMS_APDU_GET_RESPONSE));
    EDGE_ASSERT_OK(edge_vector_put_u8(v, 0x01)); // Normal
    EDGE_ASSERT_OK(edge_vector_put_u8(v, result));
    
    if (result == 0 && val && val->data) {
        EDGE_ASSERT_OK(edge_vector_put_u8(v, (uint8_t)val->tag));
        EDGE_ASSERT_OK(edge_vector_append_copy(v, val->data, val->length));
    }
    
    return EDGE_OK;
}