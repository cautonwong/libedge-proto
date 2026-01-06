#include "protocols/edge_modbus.h"
#include "common/crc.h"
#include <string.h> // For memcpy

// ##############################################################################
// # HELPERS
// ##############################################################################

// Helper to write a 16-bit value in Big Endian format
static void write_u16_be(uint8_t *buf, uint16_t value) {
    buf[0] = (uint8_t)(value >> 8);
    buf[1] = (uint8_t)(value & 0xFF);
}

static void write_u16_le(uint8_t *buf, uint16_t value) {
    buf[0] = (uint8_t)(value & 0xFF);
    buf[1] = (uint8_t)(value >> 8);
}

// Helper to read a 16-bit value in Big Endian format (for parsing)
static edge_error_t cursor_read_be16(edge_cursor_t *c, uint16_t *val) {
    uint8_t buf[2];
    EDGE_ASSERT_OK(edge_cursor_read_bytes(c, buf, 2));
    *val = (uint16_t)(((uint16_t)buf[0] << 8) | buf[1]);
    return EDGE_OK;
}


// ##############################################################################
// # PUBLIC API - Request Builders
// ##############################################################################

void edge_modbus_init_context(edge_modbus_context_t *ctx, uint8_t slave_id, bool is_tcp) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(edge_modbus_context_t));
    ctx->slave_id = slave_id;
    ctx->is_tcp = is_tcp;
    ctx->transaction_id = 0; // Start at 0, will increment before first use
}

// Generic builder for simple read requests
static edge_error_t build_simple_read_req(edge_modbus_context_t *ctx, edge_vector_t *v, uint8_t fc, uint16_t address, uint16_t quantity) {
    if (!ctx || !v || quantity == 0) return EDGE_ERR_INVALID_ARG;
    if (v->used_count + 4 > v->max_capacity) return EDGE_ERR_BUFFER_TOO_SMALL; // For TCP: MBAP, FC, Addr, Qty
    if (!ctx->is_tcp && v->used_count + 5 > v->max_capacity) return EDGE_ERR_BUFFER_TOO_SMALL; // For RTU: SlaveID, FC, Addr, Qty, CRC

    // PDU components (these values will be copied into vector's scratch_pool)
    uint8_t fc_byte = fc;
    uint8_t addr_bytes[2]; write_u16_be(addr_bytes, address);
    uint8_t qty_bytes[2]; write_u16_be(qty_bytes, quantity);
    
    if (ctx->is_tcp) {
        // MBAP Header (7 bytes)
        uint8_t mbap_hdr_data[7];
        ctx->transaction_id++;
        write_u16_be(&mbap_hdr_data[0], ctx->transaction_id);
        write_u16_be(&mbap_hdr_data[2], 0); // Protocol ID
        write_u16_be(&mbap_hdr_data[4], 1 + 5); // Length: unit ID (1) + PDU (5)
        mbap_hdr_data[6] = ctx->slave_id;
        
        EDGE_ASSERT_OK(edge_vector_append_copy(v, mbap_hdr_data, 7));
        EDGE_ASSERT_OK(edge_vector_append_copy(v, &fc_byte, 1));
        EDGE_ASSERT_OK(edge_vector_append_copy(v, addr_bytes, 2));
        EDGE_ASSERT_OK(edge_vector_append_copy(v, qty_bytes, 2));
    } else { // RTU
        uint8_t slave_id_byte = ctx->slave_id;

        EDGE_ASSERT_OK(edge_vector_append_copy(v, &slave_id_byte, 1));
        EDGE_ASSERT_OK(edge_vector_append_copy(v, &fc_byte, 1));
        EDGE_ASSERT_OK(edge_vector_append_copy(v, addr_bytes, 2));
        EDGE_ASSERT_OK(edge_vector_append_copy(v, qty_bytes, 2));

        // CRC calculation over all existing iovec segments
        uint16_t crc = 0xFFFF;
        for(int i = 0; i < v->used_count; i++) {
            crc = edge_crc16_modbus_update(crc, v->iovs[i].iov_base, v->iovs[i].iov_len);
        }
        write_u16_le(ctx->crc_buf, crc); 
        
        edge_error_t append_err = edge_vector_append_ref(v, ctx->crc_buf, 2); // Capture return value
        if (append_err != EDGE_OK) return append_err; // Check explicitly
    }
    return EDGE_OK;
}

edge_error_t edge_modbus_build_read_coils_req(
    edge_modbus_context_t *ctx, edge_vector_t *v, uint16_t address, uint16_t quantity) {
    if (quantity > 2000) return EDGE_ERR_INVALID_ARG;
    return build_simple_read_req(ctx, v, MODBUS_FC_READ_COILS, address, quantity);
}

edge_error_t edge_modbus_build_read_discrete_inputs_req(
    edge_modbus_context_t *ctx, edge_vector_t *v, uint16_t address, uint16_t quantity) {
    if (quantity > 2000) return EDGE_ERR_INVALID_ARG;
    return build_simple_read_req(ctx, v, MODBUS_FC_READ_DISCRETE_INPUTS, address, quantity);
}

edge_error_t edge_modbus_build_read_holding_registers(
    edge_modbus_context_t *ctx, edge_vector_t *v, uint16_t address, uint16_t quantity) {
    if (quantity > 125) return EDGE_ERR_INVALID_ARG;
    return build_simple_read_req(ctx, v, MODBUS_FC_READ_HOLDING_REGISTERS, address, quantity);
}

edge_error_t edge_modbus_build_write_single_register_req(
    edge_modbus_context_t *ctx, edge_vector_t *v, uint16_t address, uint16_t value) {
    if (!ctx || !v) return EDGE_ERR_INVALID_ARG;
    if (v->used_count + 4 > v->max_capacity) return EDGE_ERR_BUFFER_TOO_SMALL;
    if (!ctx->is_tcp && v->used_count + 5 > v->max_capacity) return EDGE_ERR_BUFFER_TOO_SMALL;

    uint8_t fc_byte = MODBUS_FC_WRITE_SINGLE_REGISTER;
    uint8_t addr_bytes[2]; write_u16_be(addr_bytes, address);
    uint8_t val_bytes[2]; write_u16_be(val_bytes, value);

    if (ctx->is_tcp) {
        uint8_t mbap_hdr_data[7];
        ctx->transaction_id++;
        write_u16_be(&mbap_hdr_data[0], ctx->transaction_id);
        write_u16_be(&mbap_hdr_data[2], 0);
        write_u16_be(&mbap_hdr_data[4], 1 + 5); // Length: unit ID + PDU
        mbap_hdr_data[6] = ctx->slave_id;
        
        EDGE_ASSERT_OK(edge_vector_append_copy(v, mbap_hdr_data, 7));
        EDGE_ASSERT_OK(edge_vector_append_copy(v, &fc_byte, 1));
        EDGE_ASSERT_OK(edge_vector_append_copy(v, addr_bytes, 2));
        EDGE_ASSERT_OK(edge_vector_append_copy(v, val_bytes, 2));
    } else { // RTU
        uint8_t slave_id_byte = ctx->slave_id;

        EDGE_ASSERT_OK(edge_vector_append_copy(v, &slave_id_byte, 1));
        EDGE_ASSERT_OK(edge_vector_append_copy(v, &fc_byte, 1));
        EDGE_ASSERT_OK(edge_vector_append_copy(v, addr_bytes, 2));
        EDGE_ASSERT_OK(edge_vector_append_copy(v, val_bytes, 2));

        uint16_t crc = 0xFFFF;
        for(int i = 0; i < v->used_count; i++) {
            crc = edge_crc16_modbus_update(crc, v->iovs[i].iov_base, v->iovs[i].iov_len);
        }
        write_u16_le(ctx->crc_buf, crc);
        
        edge_error_t append_err = edge_vector_append_ref(v, ctx->crc_buf, 2);
        if (append_err != EDGE_OK) return append_err;
    }
    return EDGE_OK;
}

edge_error_t edge_modbus_build_write_multiple_coils_req(
    edge_modbus_context_t *ctx, edge_vector_t *v, uint16_t address, uint16_t quantity, const uint8_t *data) {
    
    if (!ctx || !v || !data || quantity == 0 || quantity > 1968) {
        return EDGE_ERR_INVALID_ARG;
    }
    // TCP needs 6 segments (MBAP, FC, Addr, Qty, ByteCount, Data)
    // RTU needs 7 segments (SlaveID, FC, Addr, Qty, ByteCount, Data, CRC)
    if (ctx->is_tcp && v->used_count + 6 > v->max_capacity) return EDGE_ERR_BUFFER_TOO_SMALL;
    if (!ctx->is_tcp && v->used_count + 7 > v->max_capacity) return EDGE_ERR_BUFFER_TOO_SMALL;

    uint8_t byte_count = (quantity + 7) / 8;
    
    // PDU Header components
    uint8_t fc_byte = MODBUS_FC_WRITE_MULTIPLE_COILS;
    uint8_t addr_bytes[2]; write_u16_be(addr_bytes, address);
    uint8_t qty_bytes[2]; write_u16_be(qty_bytes, quantity);
    uint8_t byte_count_byte = byte_count;

    if (ctx->is_tcp) {
        uint8_t mbap_hdr_data[7];
        ctx->transaction_id++;
        write_u16_be(&mbap_hdr_data[0], ctx->transaction_id);
        write_u16_be(&mbap_hdr_data[2], 0);
        write_u16_be(&mbap_hdr_data[4], 1 + 1 + 2 + 2 + 1 + byte_count); // Length: unit ID + PDU Header + Data
        mbap_hdr_data[6] = ctx->slave_id;

        EDGE_ASSERT_OK(edge_vector_append_copy(v, mbap_hdr_data, 7));
        EDGE_ASSERT_OK(edge_vector_append_copy(v, &fc_byte, 1));
        EDGE_ASSERT_OK(edge_vector_append_copy(v, addr_bytes, 2));
        EDGE_ASSERT_OK(edge_vector_append_copy(v, qty_bytes, 2));
        EDGE_ASSERT_OK(edge_vector_append_copy(v, &byte_count_byte, 1));
        EDGE_ASSERT_OK(edge_vector_append_ref(v, data, byte_count)); // Point to user data
    } else { // RTU
        uint8_t slave_id_byte = ctx->slave_id;

        EDGE_ASSERT_OK(edge_vector_append_copy(v, &slave_id_byte, 1));
        EDGE_ASSERT_OK(edge_vector_append_copy(v, &fc_byte, 1));
        EDGE_ASSERT_OK(edge_vector_append_copy(v, addr_bytes, 2));
        EDGE_ASSERT_OK(edge_vector_append_copy(v, qty_bytes, 2));
        EDGE_ASSERT_OK(edge_vector_append_copy(v, &byte_count_byte, 1));
        EDGE_ASSERT_OK(edge_vector_append_ref(v, data, byte_count)); // Point to user data

        uint16_t crc = 0xFFFF;
        for(int i = 0; i < v->used_count; i++) {
            crc = edge_crc16_modbus_update(crc, v->iovs[i].iov_base, v->iovs[i].iov_len);
        }
        write_u16_le(ctx->crc_buf, crc);
        
        edge_error_t append_err = edge_vector_append_ref(v, ctx->crc_buf, 2);
        if (append_err != EDGE_OK) return append_err;
    }
    return EDGE_OK;
}

edge_error_t edge_modbus_build_write_multiple_registers_req(
    edge_modbus_context_t *ctx,
    edge_vector_t *v,
    uint16_t address,
    uint16_t quantity,
    const uint16_t *data_uint16) { // Renamed to avoid confusion

    if (!ctx || !v || !data_uint16 || quantity == 0 || quantity > 123) {
        return EDGE_ERR_INVALID_ARG;
    }
    // TCP needs 6 segments (MBAP, FC, Addr, Qty, ByteCount, Data)
    // RTU needs 7 segments (SlaveID, FC, Addr, Qty, ByteCount, Data, CRC)
    if (ctx->is_tcp && v->used_count + 6 > v->max_capacity) return EDGE_ERR_BUFFER_TOO_SMALL;
    if (!ctx->is_tcp && v->used_count + 7 > v->max_capacity) return EDGE_ERR_BUFFER_TOO_SMALL;

    uint8_t byte_count = quantity * 2;
    
    // PDU Header components
    uint8_t fc_byte = MODBUS_FC_WRITE_MULTIPLE_REGISTERS;
    uint8_t addr_bytes[2]; write_u16_be(addr_bytes, address);
    uint8_t qty_bytes[2]; write_u16_be(qty_bytes, quantity);
    uint8_t byte_count_byte = byte_count;

    // Data values (uint16_t array) need to be converted to uint8_t stream (Big Endian)
    // This requires ctx->converted_data_buf.
    for(uint16_t i = 0; i < quantity; i++) {
        write_u16_be(&ctx->converted_data_buf[i*2], data_uint16[i]);
    }

    if (ctx->is_tcp) {
        uint8_t mbap_hdr_data[7];
        ctx->transaction_id++;
        write_u16_be(&mbap_hdr_data[0], ctx->transaction_id);
        write_u16_be(&mbap_hdr_data[2], 0);
        write_u16_be(&mbap_hdr_data[4], 1 + 1 + 2 + 2 + 1 + byte_count); // Length: unit ID + PDU Header + Data
        mbap_hdr_data[6] = ctx->slave_id;

        EDGE_ASSERT_OK(edge_vector_append_copy(v, mbap_hdr_data, 7));
        EDGE_ASSERT_OK(edge_vector_append_copy(v, &fc_byte, 1));
        EDGE_ASSERT_OK(edge_vector_append_copy(v, addr_bytes, 2));
        EDGE_ASSERT_OK(edge_vector_append_copy(v, qty_bytes, 2));
        EDGE_ASSERT_OK(edge_vector_append_copy(v, &byte_count_byte, 1));
        EDGE_ASSERT_OK(edge_vector_append_ref(v, ctx->converted_data_buf, byte_count)); // Point to converted data
    } else { // RTU
        uint8_t slave_id_byte = ctx->slave_id;

        EDGE_ASSERT_OK(edge_vector_append_copy(v, &slave_id_byte, 1));
        EDGE_ASSERT_OK(edge_vector_append_copy(v, &fc_byte, 1));
        EDGE_ASSERT_OK(edge_vector_append_copy(v, addr_bytes, 2));
        EDGE_ASSERT_OK(edge_vector_append_copy(v, qty_bytes, 2));
        EDGE_ASSERT_OK(edge_vector_append_copy(v, &byte_count_byte, 1));
        EDGE_ASSERT_OK(edge_vector_append_ref(v, ctx->converted_data_buf, byte_count)); // Point to converted data

        uint16_t crc = 0xFFFF;
        for(int i = 0; i < v->used_count; i++) {
            crc = edge_crc16_modbus_update(crc, v->iovs[i].iov_base, v->iovs[i].iov_len);
        }
        write_u16_le(ctx->crc_buf, crc);
        
        edge_error_t append_err = edge_vector_append_ref(v, ctx->crc_buf, 2);
        if (append_err != EDGE_OK) return append_err;
    }

    return EDGE_OK;
}

// ##############################################################################
// # PARSING LOGIC - Not yet refactored to use Cursor
// ##############################################################################

static uint16_t read_u16_be(const uint8_t *buf) {
    return (uint16_t)((uint16_t)buf[0] << 8) | buf[1];
}

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
        case MODBUS_FC_READ_HOLDING_REGISTERS:
        case MODBUS_FC_READ_INPUT_REGISTERS:
            if (callbacks) {
                if (pdu_len < 2) return EDGE_ERR_INVALID_FRAME;
                uint8_t byte_count = pdu[1];
                if (pdu_len != 2 + byte_count) return EDGE_ERR_INVALID_FRAME;
                
                if (function_code == MODBUS_FC_READ_COILS && callbacks->on_read_coils_response) {
                    callbacks->on_read_coils_response(ctx, 0, 0, &pdu[2], byte_count);
                } else if (function_code == MODBUS_FC_READ_DISCRETE_INPUTS && callbacks->on_read_discrete_inputs_response) {
                    callbacks->on_read_discrete_inputs_response(ctx, 0, 0, &pdu[2], byte_count);
                } else if (function_code == MODBUS_FC_READ_HOLDING_REGISTERS && callbacks->on_read_holding_registers_response) {
                    callbacks->on_read_holding_registers_response(ctx, 0, 0, &pdu[2], byte_count);
                } else if (function_code == MODBUS_FC_READ_INPUT_REGISTERS && callbacks->on_read_input_registers_response) {
                    callbacks->on_read_input_registers_response(ctx, 0, 0, &pdu[2], byte_count);
                }
            }
            break;

        case MODBUS_FC_WRITE_SINGLE_COIL:
        case MODBUS_FC_WRITE_SINGLE_REGISTER:
            if (callbacks) {
                if (pdu_len != 5) return EDGE_ERR_INVALID_FRAME;
                uint16_t address = read_u16_be(&pdu[1]);
                uint16_t value = read_u16_be(&pdu[3]);

                if (function_code == MODBUS_FC_WRITE_SINGLE_COIL && callbacks->on_write_single_coil_response) {
                    callbacks->on_write_single_coil_response(ctx, address, value);
                } else if (function_code == MODBUS_FC_WRITE_SINGLE_REGISTER && callbacks->on_write_single_register_response) {
                    callbacks->on_write_single_register_response(ctx, address, value);
                }
            }
            break;
        
        case MODBUS_FC_WRITE_MULTIPLE_COILS:
        case MODBUS_FC_WRITE_MULTIPLE_REGISTERS:
            if (callbacks) {
                if (pdu_len != 5) return EDGE_ERR_INVALID_FRAME;
                uint16_t address = read_u16_be(&pdu[1]);
                uint16_t quantity = read_u16_be(&pdu[3]);

                if (function_code == MODBUS_FC_WRITE_MULTIPLE_COILS && callbacks->on_write_multiple_coils_response) {
                    callbacks->on_write_multiple_coils_response(ctx, address, quantity);
                } else if (function_code == MODBUS_FC_WRITE_MULTIPLE_REGISTERS && callbacks->on_write_multiple_registers_response) {
                    callbacks->on_write_multiple_registers_response(ctx, address, quantity);
                }
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
