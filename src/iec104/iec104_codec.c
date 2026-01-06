#include "libedge/edge_iec104.h"
#include <string.h>

edge_error_t edge_iec104_build_startdt_act(edge_iec104_context_t *ctx, struct iovec *out_vec) {
    if (!ctx || !out_vec) {
        return EDGE_ERR_INVALID_ARG;
    }

    uint8_t *frame = ctx->frame_buffer;
    frame[0] = IEC104_START_BYTE;
    frame[1] = 4; // Length of APDU
    frame[2] = IEC104_U_FRAME_STARTDT_ACT;
    frame[3] = 0x00;
    frame[4] = 0x00;
    frame[5] = 0x00;

    out_vec->iov_base = frame;
    out_vec->iov_len = 6;

    return EDGE_OK;
}

edge_error_t edge_iec104_build_i_frame(
    edge_iec104_context_t *ctx, 
    const uint8_t *asdu, 
    size_t asdu_len, 
    struct iovec *out_vec) {
    
    if (!ctx || !asdu || !out_vec || asdu_len == 0 || asdu_len > 249) {
        return EDGE_ERR_INVALID_ARG;
    }

    uint8_t *frame = ctx->frame_buffer;
    size_t apdu_len = 4 + asdu_len; // 4 control bytes + ASDU

    frame[0] = IEC104_START_BYTE;
    frame[1] = (uint8_t)apdu_len;

    // Control Field 1 & 2: N(S)
    // Sequence numbers are shifted left by 1 bit.
    uint16_t ns = ctx->vs << 1;
    frame[2] = ns & 0xFF;
    frame[3] = (ns >> 8) & 0xFF;

    // Control Field 3 & 4: N(R)
    uint16_t nr = ctx->vr << 1;
    frame[4] = nr & 0xFF;
    frame[5] = (nr >> 8) & 0xFF;

    // Copy ASDU
    memcpy(&frame[6], asdu, asdu_len);

    // Set iovec
    out_vec->iov_base = frame;
    out_vec->iov_len = 2 + apdu_len; // Start byte + len byte + APDU

    // Increment send sequence number
    ctx->vs++;

    return EDGE_OK;
}
