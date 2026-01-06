#include "libedge/edge_dlms.h"
#include <string.h>

// ##############################################################################
// # Internal A-XDR Encoders
// ##############################################################################

// Each encoder function writes to the buffer and returns the number of bytes written.

static size_t axdr_encode_u16(uint8_t *buf, uint16_t value) {
    buf[0] = (uint8_t)(value >> 8);
    buf[1] = (uint8_t)(value & 0xFF);
    return 2;
}

static size_t axdr_encode_i8(uint8_t *buf, int8_t value) {
    buf[0] = (uint8_t)value;
    return 1;
}

static size_t axdr_encode_octet_string(uint8_t *buf, const uint8_t *str, uint8_t len) {
    buf[0] = len;
    memcpy(&buf[1], str, len);
    return 1 + len;
}

static size_t axdr_encode_cosem_attribute_descriptor(
    uint8_t *buf,
    uint16_t class_id,
    const uint8_t obis_code[6],
    int8_t attribute_id)
{
    size_t offset = 0;
    
    // SEQUENCE { ... }
    buf[offset++] = DLMS_DATA_TAG_STRUCTURE;
    buf[offset++] = 3; // 3 components

    // class-id: long-unsigned
    buf[offset++] = DLMS_DATA_TAG_LONG_UNSIGNED;
    offset += axdr_encode_u16(&buf[offset], class_id);

    // instance-id: octet-string
    buf[offset++] = DLMS_DATA_TAG_OCTET_STRING;
    offset += axdr_encode_octet_string(&buf[offset], obis_code, 6);

    // attribute-id: integer
    buf[offset++] = DLMS_DATA_TAG_INTEGER;
    offset += axdr_encode_i8(&buf[offset], attribute_id);

    return offset;
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

    // APDU Tag [C0]
    frame[offset++] = DLMS_APDU_TAG_GET_REQUEST;
    
    // Get-Request Choice [01]
    frame[offset++] = DLMS_GET_REQUEST_NORMAL;
    
    // Invoke-Id-And-Priority
    frame[offset++] = invoke_id;
    
    // Cosem-Attribute-Descriptor
    offset += axdr_encode_cosem_attribute_descriptor(&frame[offset], class_id, obis_code, attribute_id);
    
    out_vec->iov_base = frame;
    out_vec->iov_len = offset;

    return EDGE_OK;
}
