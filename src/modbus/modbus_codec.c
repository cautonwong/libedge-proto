#include "libedge/edge_modbus.h"
#include "common/crc.h"
#include <string.h> // For memcpy

// ##############################################################################
// # HELPERS
// ##############################################################################

static void write_u16_be(uint8_t *buf, uint16_t value) {
    buf[0] = (uint8_t)(value >> 8);
    buf[1] = (uint8_t)(value & 0xFF);
}

static void write_u16_le(uint8_t *buf, uint16_t value) {
    buf[0] = (uint8_t)(value & 0xFF);
    buf[1] = (uint8_t)(value >> 8);
}

static uint16_t read_u16_be(const uint8_t *buf) {
    return (uint16_t)((uint16_t)buf[0] << 8) | buf[1];
}

// Generic helper to finalize the frame for TCP or RTU mode
static edge_error_t finalize_frame(edge_modbus_context_t *ctx, uint8_t pdu_len, struct iovec *out_vec, int *out_vec_count) {
    // The PDU is assumed to be at the start of ctx->frame_buffer
    uint8_t* pdu = ctx->frame_buffer;

    if (ctx->is_tcp) {
        // For TCP, the PDU is not actually at the start. We need to shift it.
        // Move PDU to make space for MBAP header
        memmove(pdu + 7, pdu, pdu_len);

        ctx->transaction_id++;
        uint8_t *mbap = ctx->frame_buffer;
        write_u16_be(&mbap[0], ctx->transaction_id);
        write_u16_be(&mbap[2], 0); // Protocol ID
        write_u16_be(&mbap[4], pdu_len + 1); // Length
        mbap[6] = ctx->slave_id;

        out_vec[0].iov_base = mbap;
        out_vec[0].iov_len = 7 + pdu_len;
        *out_vec_count = 1;
    } else {
        // For RTU, the PDU is also not at the start.
        // Move PDU to make space for Slave ID
        memmove(pdu + 1, pdu, pdu_len);
        pdu[0] = ctx->slave_id;
        
        uint8_t* rtu_frame = ctx->frame_buffer;
        uint16_t frame_len_no_crc = 1 + pdu_len;

        uint16_t crc = edge_crc16_modbus(rtu_frame, frame_len_no_crc);
        write_u16_le(&rtu_frame[frame_len_no_crc], crc);

        out_vec[0].iov_base = rtu_frame;
        out_vec[0].iov_len = frame_len_no_crc + 2;
        *out_vec_count = 1;
    }
    return EDGE_OK;
}

// ##############################################################################
// # PUBLIC API
// ##############################################################################

void edge_modbus_init_context(edge_modbus_context_t *ctx, uint8_t slave_id, bool is_tcp) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(edge_modbus_context_t));
    ctx->slave_id = slave_id;
    ctx->is_tcp = is_tcp;
}

edge_error_t edge_modbus_build_read_coils_req(
    edge_modbus_context_t *ctx, uint16_t address, uint16_t quantity,
    struct iovec *out_vec, int *out_vec_count) {
    
    if (!ctx || !out_vec || !out_vec_count || quantity == 0 || quantity > 2000) {
        return EDGE_ERR_INVALID_ARG;
    }
    uint8_t *pdu = ctx->frame_buffer;
    pdu[0] = MODBUS_FC_READ_COILS;
    write_u16_be(&pdu[1], address);
    write_u16_be(&pdu[3], quantity);
    return finalize_frame(ctx, 5, out_vec, out_vec_count);
}

edge_error_t edge_modbus_build_read_discrete_inputs_req(
    edge_modbus_context_t *ctx, uint16_t address, uint16_t quantity,
    struct iovec *out_vec, int *out_vec_count) {

    if (!ctx || !out_vec || !out_vec_count || quantity == 0 || quantity > 2000) {
        return EDGE_ERR_INVALID_ARG;
    }
    uint8_t *pdu = ctx->frame_buffer;
    pdu[0] = MODBUS_FC_READ_DISCRETE_INPUTS;
    write_u16_be(&pdu[1], address);
    write_u16_be(&pdu[3], quantity);
    return finalize_frame(ctx, 5, out_vec, out_vec_count);
}

edge_error_t edge_modbus_build_read_holding_registers_req(
    edge_modbus_context_t *ctx, uint16_t address, uint16_t quantity,
    struct iovec *out_vec, int *out_vec_count) {

    if (!ctx || !out_vec || !out_vec_count || quantity == 0 || quantity > 125) {
        return EDGE_ERR_INVALID_ARG;
    }
    uint8_t *pdu = ctx->frame_buffer;
    pdu[0] = MODBUS_FC_READ_HOLDING_REGISTERS;
    write_u16_be(&pdu[1], address);
    write_u16_be(&pdu[3], quantity);
    return finalize_frame(ctx, 5, out_vec, out_vec_count);
}

edge_error_t edge_modbus_build_write_single_register_req(
    edge_modbus_context_t *ctx, uint16_t address, uint16_t value,
    struct iovec *out_vec, int *out_vec_count) {

    if (!ctx || !out_vec || !out_vec_count) {
        return EDGE_ERR_INVALID_ARG;
    }
    uint8_t *pdu = ctx->frame_buffer;
    pdu[0] = MODBUS_FC_WRITE_SINGLE_REGISTER;
    write_u16_be(&pdu[1], address);
    write_u16_be(&pdu[3], value);
    return finalize_frame(ctx, 5, out_vec, out_vec_count);
}

edge_error_t edge_modbus_build_write_multiple_coils_req(
    edge_modbus_context_t *ctx,
    uint16_t address,
    uint16_t quantity,
    const uint8_t *data,
    struct iovec *out_vec,
    int *out_vec_count) {
    
    if (!ctx || !out_vec || !out_vec_count || !data || quantity == 0 || quantity > 1968) {
        return EDGE_ERR_INVALID_ARG;
    }
    uint8_t *pdu = ctx->frame_buffer;
    uint8_t byte_count = (quantity + 7) / 8;
    uint8_t pdu_len = 1 + 2 + 2 + 1 + byte_count;

    pdu[0] = MODBUS_FC_WRITE_MULTIPLE_COILS;
    write_u16_be(&pdu[1], address);
    write_u16_be(&pdu[3], quantity);
    pdu[5] = byte_count;
    memcpy(&pdu[6], data, byte_count);

    return finalize_frame(ctx, pdu_len, out_vec, out_vec_count);
}

edge_error_t edge_modbus_build_write_multiple_registers_req(
    edge_modbus_context_t *ctx,
    uint16_t address,
    uint16_t quantity,
    const uint16_t *data,
    struct iovec *out_vec,
    int *out_vec_count) {

    if (!ctx || !out_vec || !out_vec_count || !data || quantity == 0 || quantity > 123) {
        return EDGE_ERR_INVALID_ARG;
    }
    uint8_t *pdu = ctx->frame_buffer;
    uint8_t byte_count = quantity * 2;
    uint8_t pdu_len = 1 + 2 + 2 + 1 + byte_count;
    
    pdu[0] = MODBUS_FC_WRITE_MULTIPLE_REGISTERS;
    write_u16_be(&pdu[1], address);
    write_u16_be(&pdu[3], quantity);
    pdu[5] = byte_count;
    for (uint16_t i = 0; i < quantity; i++) {
        write_u16_be(&pdu[6 + i * 2], data[i]);
    }

    return finalize_frame(ctx, pdu_len, out_vec, out_vec_count);
}

// ##############################################################################
// # PARSING LOGIC
// ##############################################################################

static edge_error_t parse_pdu(edge_modbus_context_t *ctx, const uint8_t *pdu, size_t pdu_len, const edge_modbus_callbacks_t *callbacks) {
    if (pdu_len < 1) return EDGE_ERR_INVALID_FRAME;
    uint8_t function_code = pdu[0];

    if (function_code & 0x80) { // Exception response
        if (pdu_len < 2) return EDGE_ERR_INVALID_FRAME;
        if (callbacks && callbacks->on_exception_response) {
            callbacks->on_exception_response(ctx, function_code & 0x7F, pdu[1]);
        }
        return EDGE_OK;
    }

    switch (function_code) {
        case MODBUS_FC_READ_COILS:
        case MODBUS_FC_READ_DISCRETE_INPUTS:
            if (callbacks) {
                if (pdu_len < 2) return EDGE_ERR_INVALID_FRAME;
                uint8_t byte_count = pdu[1];
                if (pdu_len != 2 + byte_count) return EDGE_ERR_INVALID_FRAME;

                if (function_code == MODBUS_FC_READ_COILS && callbacks->on_read_coils_response) {
                    callbacks->on_read_coils_response(ctx, 0, 0, &pdu[2], byte_count);
                } else if (function_code == MODBUS_FC_READ_DISCRETE_INPUTS && callbacks->on_read_discrete_inputs_response) {
                    callbacks->on_read_discrete_inputs_response(ctx, 0, 0, &pdu[2], byte_count);
                }
            }
            break;

        case MODBUS_FC_READ_HOLDING_REGISTERS:
        // TODO: Also handle Read Input Registers (0x04)
            if (callbacks && callbacks->on_read_holding_registers_response) {
                if (pdu_len < 2) return EDGE_ERR_INVALID_FRAME;
                uint8_t byte_count = pdu[1];
                if (pdu_len != 2 + byte_count) return EDGE_ERR_INVALID_FRAME;
                callbacks->on_read_holding_registers_response(ctx, 0, 0, &pdu[2], byte_count);
            }
            break;

        case MODBUS_FC_WRITE_SINGLE_COIL:
            if (callbacks && callbacks->on_write_single_coil_response) {
                if (pdu_len != 5) return EDGE_ERR_INVALID_FRAME;
                uint16_t address = read_u16_be(&pdu[1]);
                uint16_t value = read_u16_be(&pdu[3]);
                callbacks->on_write_single_coil_response(ctx, address, value);
            }
            break;

        case MODBUS_FC_WRITE_SINGLE_REGISTER:
            if (callbacks && callbacks->on_write_single_register_response) {
                if (pdu_len != 5) return EDGE_ERR_INVALID_FRAME;
                uint16_t address = read_u16_be(&pdu[1]);
                uint16_t value = read_u16_be(&pdu[3]);
                callbacks->on_write_single_register_response(ctx, address, value);
            }
            break;
        
        case MODBUS_FC_WRITE_MULTIPLE_COILS:
            if (callbacks && callbacks->on_write_multiple_coils_response) {
                if (pdu_len != 5) return EDGE_ERR_INVALID_FRAME;
                uint16_t address = read_u16_be(&pdu[1]);
                uint16_t quantity = read_u16_be(&pdu[3]);
                callbacks->on_write_multiple_coils_response(ctx, address, quantity);
            }
            break;

        case MODBUS_FC_WRITE_MULTIPLE_REGISTERS:
            if (callbacks && callbacks->on_write_multiple_registers_response) {
                if (pdu_len != 5) return EDGE_ERR_INVALID_FRAME;
                uint16_t address = read_u16_be(&pdu[1]);
                uint16_t quantity = read_u16_be(&pdu[3]);
                callbacks->on_write_multiple_registers_response(ctx, address, quantity);
            }
            break;
        
        default:
            // This is not necessarily an error, just an unhandled function code for parsing.
            break;
    }

    return EDGE_OK;
}

edge_error_t edge_modbus_parse_frame(
    edge_modbus_context_t *ctx,
    const uint8_t *frame_data,
    size_t frame_len,
    const edge_modbus_callbacks_t *callbacks) {
    
    if (!ctx || !frame_data || !callbacks) {
        return EDGE_ERR_INVALID_ARG;
    }

    if (ctx->is_tcp) {
        if (frame_len < 7) return EDGE_ERR_INVALID_FRAME;

        uint16_t length = read_u16_be(&frame_data[4]);
        uint8_t slave_id = frame_data[6];

        if (slave_id != ctx->slave_id) return EDGE_ERR_INVALID_FRAME; 
        if (length == 0 || (frame_len - 6) != length) return EDGE_ERR_INVALID_FRAME;

        const uint8_t *pdu = &frame_data[7];
        size_t pdu_len = length - 1;
        return parse_pdu(ctx, pdu, pdu_len, callbacks);

    } else {
        // TODO: Implement RTU parsing
        return EDGE_ERR_GENERAL;
    }
}