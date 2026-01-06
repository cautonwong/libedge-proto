#include "libedge/edge_iec104.h"
#include <string.h>

/**
 * @brief 构建 S 帧 (确认帧) - 严格遵循小端编码
 */
edge_error_t edge_iec104_build_s_frame(edge_vector_t *v, edge_iec104_context_t *ctx) {
    EP_ASSERT_OK(edge_vector_put_u8(v, 0x68));
    EP_ASSERT_OK(edge_vector_put_u8(v, 0x04));
    EP_ASSERT_OK(edge_vector_put_u8(v, 0x01)); 
    EP_ASSERT_OK(edge_vector_put_u8(v, 0x00));
    EP_ASSERT_OK(edge_vector_put_le16(v, (uint16_t)(ctx->v_r << 1)));
    return EP_OK;
}

/**
 * @brief 工业级解析 APCI - 使用小端读取控制域
 */
edge_error_t edge_iec104_parse_apci(edge_cursor_t *c, uint16_t *ctrl1, uint16_t *ctrl2) {
    uint8_t start, len;
    EP_ASSERT_OK(edge_cursor_read_u8(c, &start));
    if (start != 0x68) return EP_ERR_INVALID_FRAME;
    
    EP_ASSERT_OK(edge_cursor_read_u8(c, &len));
    
    // [专家级修正]：IEC104 控制域是小端字节序
    EP_ASSERT_OK(edge_cursor_read_le16(c, ctrl1));
    EP_ASSERT_OK(edge_cursor_read_le16(c, ctrl2));
    
    return EP_OK;
}
