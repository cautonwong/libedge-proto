#include "protocols/edge_dlt645.h"
#include <string.h>
#include <stdio.h>

static uint8_t _calc_cs(const uint8_t *buf, size_t len) {
    uint8_t cs = 0;
    for (size_t i = 0; i < len; i++) cs += buf[i];
    return cs;
}

void edge_dlt645_init(edge_dlt645_context_t *ctx, const char *addr_str) {
    if (!ctx || !addr_str) return;
    // Simple BCD conversion for address
    for (int i = 0; i < 6; i++) {
        unsigned int val;
        sscanf(addr_str + (5 - i) * 2, "%02X", &val);
        ctx->addr_bcd[i] = (uint8_t)val;
    }
}

edge_error_t edge_dlt645_build_read_req(edge_dlt645_context_t *ctx, edge_vector_t *v, uint32_t di) {
    EDGE_ASSERT_OK(edge_vector_put_u8(v, 0x68));
    EDGE_ASSERT_OK(edge_vector_append_copy(v, ctx->addr_bcd, 6));
    EDGE_ASSERT_OK(edge_vector_put_u8(v, 0x68));
    EDGE_ASSERT_OK(edge_vector_put_u8(v, 0x11)); // Read Control
    EDGE_ASSERT_OK(edge_vector_put_u8(v, 4));    // Length
    
    // DI with 0x33 offset
    uint8_t di_bytes[4];
    di_bytes[0] = (uint8_t)((di & 0xFF) + 0x33);
    di_bytes[1] = (uint8_t)(((di >> 8) & 0xFF) + 0x33);
    di_bytes[2] = (uint8_t)(((di >> 16) & 0xFF) + 0x33);
    di_bytes[3] = (uint8_t)(((di >> 24) & 0xFF) + 0x33);
    EDGE_ASSERT_OK(edge_vector_append_copy(v, di_bytes, 4));
    
    // CS
    uint8_t cs = 0;
    for (int i = 0; i < v->used_count; i++) {
        for (size_t j = 0; j < v->iovs[i].iov_len; j++) {
            cs += ((uint8_t*)v->iovs[i].iov_base)[j];
        }
    }
    EDGE_ASSERT_OK(edge_vector_put_u8(v, cs));
    EDGE_ASSERT_OK(edge_vector_put_u8(v, 0x16)); // End
    
    return EDGE_OK;
}

edge_error_t edge_dlt645_parse_frame(edge_dlt645_context_t *ctx, edge_cursor_t *c, uint32_t *out_di, const uint8_t **out_data, size_t *out_len) {
    uint8_t b;
    // Skip FE
    while (edge_cursor_remaining(c) > 0) {
        edge_cursor_read_u8(c, &b);
        if (b == 0x68) break;
    }
    
    if (edge_cursor_remaining(c) < 11) return EDGE_ERR_INCOMPLETE_DATA;
    
    uint8_t addr[6];
    EDGE_ASSERT_OK(edge_cursor_read_bytes(c, addr, 6));
    EDGE_ASSERT_OK(edge_cursor_read_u8(c, &b)); // 68
    
    uint8_t ctrl, len;
    EDGE_ASSERT_OK(edge_cursor_read_u8(c, &ctrl));
    EDGE_ASSERT_OK(edge_cursor_read_u8(c, &len));
    
    if (len < 4) return EDGE_ERR_INVALID_FRAME;
    
    uint8_t di_bytes[4];
    EDGE_ASSERT_OK(edge_cursor_read_bytes(c, di_bytes, 4));
    *out_di = (uint32_t)(di_bytes[0] - 0x33) | 
              ((uint32_t)(di_bytes[1] - 0x33) << 8) |
              ((uint32_t)(di_bytes[2] - 0x33) << 16) |
              ((uint32_t)(di_bytes[3] - 0x33) << 24);
              
    *out_len = len - 4;
    *out_data = edge_cursor_get_ptr(c, *out_len);
    
    return EDGE_OK;
}
