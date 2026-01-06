#include "edge_core.h"

/**
 * @brief DNP3 传输层控制字节
 */
#define DNP3_TR_FIR  0x40
#define DNP3_TR_FIN  0x80
#define DNP3_TR_SEQ_MASK 0x3F

typedef struct {
    uint8_t seq;
} dnp3_transport_ctx_t;

/**
 * @brief 封装传输层头部 (TH)
 */
edge_error_t dnp3_transport_wrap(dnp3_transport_ctx_t *ctx, edge_vector_t *v, bool first, bool last) {
    uint8_t ctrl = ctx->seq & DNP3_TR_SEQ_MASK;
    if (first) ctrl |= DNP3_TR_FIR;
    if (last)  ctrl |= DNP3_TR_FIN;
    
    EP_ASSERT_OK(edge_vector_put_u8(v, ctrl));
    ctx->seq = (uint8_t)((ctx->seq + 1) & DNP3_TR_SEQ_MASK);
    return EP_OK;
}
