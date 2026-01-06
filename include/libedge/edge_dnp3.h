#ifndef LIBEDGE_PROTOCOLS_DNP3_H
#define LIBEDGE_PROTOCOLS_DNP3_H

#include "edge_core.h"

typedef struct {
    uint16_t src_addr;
    uint16_t dest_addr;
    uint8_t control;
} edge_dnp3_context_t;

void edge_dnp3_init(edge_dnp3_context_t *ctx, uint16_t src, uint16_t dest);

/**
 * @brief 构建 DNP3 链路层帧 (LPDU)
 * 包含 0x05 0x64 同步头与 CRC
 */
edge_error_t edge_dnp3_build_link_frame(edge_dnp3_context_t *ctx, edge_vector_t *v, uint8_t func, const void *payload, size_t len);

#endif // LIBEDGE_PROTOCOLS_DNP3_H
