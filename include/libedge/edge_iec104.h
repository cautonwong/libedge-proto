#ifndef LIBEDGE_EDGE_IEC104_H
#define LIBEDGE_EDGE_IEC104_H

#include "edge_common.h"
#include <sys/uio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IEC104_START_BYTE 0x68

// APDU Type constants
#define IEC104_U_FRAME_STARTDT_ACT 0x07
#define IEC104_U_FRAME_STARTDT_CON 0x0B
#define IEC104_U_FRAME_STOPDT_ACT  0x13
#define IEC104_U_FRAME_STOPDT_CON  0x23
#define IEC104_U_FRAME_TESTFR_ACT  0x43
#define IEC104_U_FRAME_TESTFR_CON  0x83

typedef struct {
    edge_base_context_t base;

    // Send and Receive sequence numbers
    uint16_t vs;
    uint16_t vr;

    uint8_t frame_buffer[256];
} edge_iec104_context_t;


/**
 * @brief Build a STARTDT (activation) frame.
 * 
 * @param ctx The IEC104 context.
 * @param out_vec An iovec structure to be populated with the request frame.
 * @return edge_error_t EDGE_OK on success.
 */
edge_error_t edge_iec104_build_startdt_act(edge_iec104_context_t *ctx, struct iovec *out_vec);

/**
 * @brief Build an I-frame to transport an ASDU.
 * 
 * @param ctx The IEC104 context. Holds and increments the sequence numbers.
 * @param asdu A pointer to the pre-built Application Service Data Unit (ASDU).
 * @param asdu_len The length of the ASDU.
 * @param out_vec An iovec structure to be populated with the request frame.
 * @return edge_error_t EDGE_OK on success.
 */
edge_error_t edge_iec104_build_i_frame(
    edge_iec104_context_t *ctx, 
    const uint8_t *asdu, 
    size_t asdu_len, 
    struct iovec *out_vec);


#ifdef __cplusplus
}
#endif

#endif // LIBEDGE_EDGE_IEC104_H
