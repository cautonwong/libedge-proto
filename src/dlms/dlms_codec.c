#include "libedge/edge_dlms.h"
#include <string.h>

// ##############################################################################
// # xDLMS Data Encoders (Simplified)
// ##############################################################################

// Returns number of bytes written
static size_t encode_u16(uint8_t *buf, uint16_t value) {
    buf[0] = (uint8_t)(value >> 8);
    buf[1] = (uint8_t)(value & 0xFF);
    return 2;
}

// ##############################################################################
// # Public API
// ##############################################################################

edge_error_t edge_dlms_build_get_request_normal(
    edge_dlms_context_t *ctx,
    uint8_t invoke_id,
    uint16_t class_id,
    const uint8_t obis_code[6],
    int8_t attribute_id,
    struct iovec *out_vec) {
    
    if (!ctx || !obis_code || !out_vec) {
        return EDGE_ERR_INVALID_ARG;
    }

    uint8_t *frame = ctx->frame_buffer;
    size_t offset = 0;

    // APDU Tag
    frame[offset++] = DLMS_APDU_TAG_GET_REQUEST;
    // Get-Request Choice
    frame[offset++] = DLMS_GET_REQUEST_NORMAL;
    // Invoke-Id-And-Priority
    frame[offset++] = invoke_id;
    
    // Cosem-Attribute-Descriptor ::= SEQUENCE { }
    // We are not encoding the full DLMS 'Data' type system yet,
    // just the specific format for the attribute descriptor.
    // [02] - SEQUENCE tag
    // [03] - Length of sequence
    frame[offset++] = 2; // SEQUENCE tag
    frame[offset++] = 3; // 3 components in sequence

    // class-id: long-unsigned
    frame[offset++] = 18; // tag for long-unsigned
    offset += encode_u16(&frame[offset], class_id);

    // instance-id: octet-string
    frame[offset++] = 9; // tag for octet-string
    frame[offset++] = 6; // length of OBIS
    memcpy(&frame[offset], obis_code, 6);
    offset += 6;

    // attribute-id: integer
    frame[offset++] = 15; // tag for integer
    frame[offset++] = (uint8_t)attribute_id;
    
    out_vec->iov_base = frame;
    out_vec->iov_len = offset;

    return EDGE_OK;
}