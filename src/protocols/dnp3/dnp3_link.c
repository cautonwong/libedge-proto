#include "libedge/edge_dnp3.h"
#include "common/crc.h"
#include <string.h>

void edge_dnp3_init(edge_dnp3_context_t *ctx, uint16_t src, uint16_t dest) {
    if (!ctx) return;
    ctx->src_addr = src;
    ctx->dest_addr = dest;
}

edge_error_t edge_dnp3_build_link_frame(edge_dnp3_context_t *ctx, edge_vector_t *v, uint8_t func, const void *payload, size_t len) {
    if (len > 250) return EDGE_ERR_OUT_OF_BOUNDS;

    uint8_t header[10];
    header[0] = 0x05; header[1] = 0x64; // Sync
    header[2] = (uint8_t)(len + 5);     // Length (Control + Addrs + payload)
    header[3] = func;
    header[4] = (uint8_t)(ctx->dest_addr & 0xFF);
    header[5] = (uint8_t)(ctx->dest_addr >> 8);
    header[6] = (uint8_t)(ctx->src_addr & 0xFF);
    header[7] = (uint8_t)(ctx->src_addr >> 8);
    
    uint16_t h_crc = edge_crc16_dnp3(header, 8);
    header[8] = (uint8_t)(h_crc & 0xFF);
    header[9] = (uint8_t)(h_crc >> 8);
    
    EDGE_ASSERT_OK(edge_vector_append_copy(v, header, 10));

    // [专家级逻辑]：每 16 字节数据块后面跟一个 CRC
    const uint8_t *p = (const uint8_t *)payload;
    size_t rem = len;
    while (rem > 0) {
        size_t chunk = (rem > 16) ? 16 : rem;
        EDGE_ASSERT_OK(edge_vector_append_ref(v, p, chunk));
        
        uint16_t p_crc = edge_crc16_dnp3(p, chunk);
        uint8_t crc_bytes[2] = { (uint8_t)(p_crc & 0xFF), (uint8_t)(p_crc >> 8) };
        EDGE_ASSERT_OK(edge_vector_append_copy(v, crc_bytes, 2));
        
        p += chunk;
        rem -= chunk;
    }

    return EDGE_OK;
}
