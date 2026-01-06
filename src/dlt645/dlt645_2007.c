#include "libedge/edge_dlt645.h"
#include <string.h>
#include <stdio.h> // For debugging

// Helper to calculate the DLT645 checksum (sum of bytes)
static uint8_t dlt645_checksum(const uint8_t *buffer, size_t length) {
    uint8_t sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += buffer[i];
    }
    return sum;
}

// Helper to decode the data payload by subtracting 0x33 from each byte
static void dlt645_subtract_33(uint8_t *buffer, size_t length) {
    for (size_t i = 0; i < length; i++) {
        buffer[i] -= 0x33;
    }
}

edge_error_t edge_dlt645_build_read_req(
    edge_dlt645_context_t *ctx,
    const uint8_t address[DLT645_ADDR_LEN],
    const uint8_t data_id[4],
    struct iovec *out_vec) {

    if (!ctx || !address || !data_id || !out_vec) {
        return EDGE_ERR_INVALID_ARG;
    }

    uint8_t *frame = ctx->frame_buffer;
    size_t offset = 0;

    // Frame Start
    frame[offset++] = DLT645_FRAME_START;

    // Address
    memcpy(&frame[offset], address, DLT645_ADDR_LEN);
    offset += DLT645_ADDR_LEN;

    // Frame Start (again)
    frame[offset++] = DLT645_FRAME_START;

    // Control Code
    frame[offset++] = DLT645_CTRL_READ_DATA;

    // Data Length
    frame[offset++] = 0x04; // Length of Data ID

    // Data (Data ID + 0x33)
    for (int i = 0; i < 4; i++) {
        frame[offset++] = data_id[i] + 0x33;
    }

    // Checksum
    uint8_t cs = dlt645_checksum(frame, offset);
    frame[offset++] = cs;

    // Frame End
    frame[offset++] = DLT645_FRAME_END;

    // Set iovec
    out_vec->iov_base = frame;
    out_vec->iov_len = offset;

    return EDGE_OK;
}

edge_error_t edge_dlt645_parse_frame(
    edge_dlt645_context_t *ctx,
    const uint8_t *frame_data,
    size_t frame_len,
    const edge_dlt645_callbacks_t *callbacks) {
    
    if (!ctx || !frame_data || !callbacks) {
        return EDGE_ERR_INVALID_ARG;
    }

    // Basic frame validation
    if (frame_len < 12 || frame_data[0] != DLT645_FRAME_START || frame_data[frame_len - 1] != DLT645_FRAME_END) {
        return EDGE_ERR_INVALID_FRAME;
    }

    // Verify checksum
    uint8_t expected_cs = frame_data[frame_len - 2];
    uint8_t calculated_cs = dlt645_checksum(frame_data, frame_len - 2);
    if (expected_cs != calculated_cs) {
        return EDGE_ERR_INVALID_FRAME;
    }

    // Extract fields
    const uint8_t *address = &frame_data[1];
    uint8_t ctrl_code = frame_data[8];
    uint8_t data_len = frame_data[9];

    // Check control code for response type
    if (ctrl_code == 0x91) {
        if (!callbacks->on_read_response) return EDGE_OK; // No callback to call

        if (data_len < 4) {
            return EDGE_ERR_INVALID_FRAME;
        }

        uint8_t *payload = ctx->frame_buffer; // Use context buffer for manipulation
        memcpy(payload, &frame_data[10], data_len);

        // Subtract 0x33 from the entire payload
        dlt645_subtract_33(payload, data_len);

        const uint8_t *data_id = payload;
        const uint8_t *data = &payload[4];
        size_t actual_data_len = data_len - 4;

        callbacks->on_read_response(ctx, address, data_id, data, actual_data_len);

    } else {
        // Unhandled Ctrl code
    }

    return EDGE_OK;
}
