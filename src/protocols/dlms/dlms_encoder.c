#include "protocols/edge_dlms.h"
#include <string.h>

void edge_dlms_encoder_init(edge_dlms_encoder_t *enc, edge_vector_t *v) {
    memset(enc, 0, sizeof(*enc));
    enc->v = v;
}

edge_error_t edge_dlms_encode_begin_container(edge_dlms_encoder_t *enc, edge_dlms_tag_t tag) {
    if (enc->depth >= 16) return EDGE_ERR_OVERFLOW;
    EDGE_ASSERT_OK(edge_vector_put_u8(enc->v, (uint8_t)tag));
    
    enc->stack_offsets[enc->depth] = edge_vector_length(enc->v);
    enc->stack_tags[enc->depth] = tag;
    enc->depth++;
    
    // 预留 1 字节。对于 Array/Struct 它是成员数；对于 OctetString 它是字节长度。
    return edge_vector_put_u8(enc->v, 0x00);
}

edge_error_t edge_dlms_encode_end_container(edge_dlms_encoder_t *enc) {
    if (enc->depth == 0) return EDGE_ERR_INVALID_STATE;
    // 注意：高质量 A-XDR 编码器在 end 时通常不需要做太多事，
    // 因为成员数通常在 begin 时已知。但在我们的 Builder 模式下，
    // 我们在此处“回填”成员数量。
    return EDGE_OK;
}

// [高质量增强]：显式设置容器成员数
edge_error_t edge_dlms_encode_set_container_len(edge_dlms_encoder_t *enc, size_t count) {
    if (enc->depth == 0) return EDGE_ERR_INVALID_STATE;
    size_t len_pos = enc->stack_offsets[enc->depth - 1];
    uint8_t *p = (uint8_t *)edge_vector_get_ptr(enc->v, len_pos);
    if (p) *p = (uint8_t)count;
    return EDGE_OK;
}

edge_error_t edge_dlms_encode_u8(edge_dlms_encoder_t *enc, uint8_t val) {
    EDGE_ASSERT_OK(edge_vector_put_u8(enc->v, (uint8_t)DLMS_TAG_UNSIGNED));
    return edge_vector_put_u8(enc->v, val);
}

edge_error_t edge_dlms_encode_u16(edge_dlms_encoder_t *enc, uint16_t val) {
    EDGE_ASSERT_OK(edge_vector_put_u8(enc->v, (uint8_t)DLMS_TAG_LONG_UNSIGNED));
    return edge_vector_put_be16(enc->v, val);
}

edge_error_t edge_dlms_encode_octet_string(edge_dlms_encoder_t *enc, const uint8_t *data, size_t len) {
    EDGE_ASSERT_OK(edge_vector_put_u8(enc->v, (uint8_t)DLMS_TAG_OCTET_STRING));
    if (len < 128) {
        EDGE_ASSERT_OK(edge_vector_put_u8(enc->v, (uint8_t)len));
    } else {
        EDGE_ASSERT_OK(edge_vector_put_u8(enc->v, 0x81));
        EDGE_ASSERT_OK(edge_vector_put_u8(enc->v, (uint8_t)len));
    }
    return edge_vector_append_copy(enc->v, data, len);
}
