#ifndef LIBEDGE_IEC104_H
#define LIBEDGE_IEC104_H

#include "edge_core.h"

typedef struct {
    uint16_t v_s; // Send sequence number
    uint16_t v_r; // Receive sequence number
    uint16_t v_a; // Acknowledged sequence number
    uint8_t  k;   // Max unacknowledged I-frames
    uint8_t  w;   // Max unacknowledged S-frames
} edge_iec104_context_t;

edge_error_t edge_iec104_parse_apci(edge_cursor_t *c, uint16_t *ctrl1, uint16_t *ctrl2);
edge_error_t edge_iec104_build_type1(edge_vector_t *v, uint32_t ioa, bool val, uint8_t qds);
edge_error_t edge_iec104_session_on_recv(edge_iec104_context_t *ctx, edge_cursor_t *c, edge_vector_t *resp);

#endif
