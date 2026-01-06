#include "libedge/edge_modbus.h"
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

// Helper to write a 16-bit value in Little Endian format
static void write_u16_le(uint8_t *buf, uint16_t value) {
    buf[0] = (uint8_t)(value & 0xFF);
    buf[1] = (uint8_t)(value >> 8);
}


// ##############################################################################
// # PUBLIC API - Request Builders
// ##############################################################################

void edge_modbus_init_context(edge_modbus_context_t *ctx, uint8_t slave_id, bool is_tcp) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(edge_modbus_context_t));
    ctx->base.user_data = NULL; // Initialize base context
    ctx->slave_id = slave_id;
    ctx->is_tcp = is_tcp;
    ctx->transaction_id = 0; // Start at 0, will increment before first use
}

edge_error_t edge_modbus_build_read_coils_req(
    edge_modbus_context_t *ctx, uint16_t address, uint16_t quantity,
    struct iovec *out_vec, int *out_vec_capacity) {
    
    if (!ctx || !out_vec || !out_vec_capacity || quantity == 0 || quantity > 2000) {
        return EDGE_ERR_INVALID_ARG;
    }

    int segments_needed = ctx->is_tcp ? MODBUS_MAX_IOVEC_TCP : MODBUS_MAX_IOVEC_RTU;
    if (*out_vec_capacity < segments_needed) return EDGE_ERR_BUFFER_TOO_SMALL;
    int current_iov_idx = 0;

    // PDU components
    uint8_t pdu_fc[1] = {MODBUS_FC_READ_COILS};
    uint8_t pdu_addr[2]; write_u16_be(pdu_addr, address);
    uint8_t pdu_qty[2]; write_u16_be(pdu_qty, quantity);
    
    if (ctx->is_tcp) {
        uint8_t mbap_hdr[7];
        ctx->transaction_id++;
        write_u16_be(&mbap_hdr[0], ctx->transaction_id);
        write_u16_be(&mbap_hdr[2], 0); // Protocol ID
        write_u16_be(&mbap_hdr[4], 1 + sizeof(pdu_fc) + sizeof(pdu_addr) + sizeof(pdu_qty)); // Length: unit ID + PDU
        mbap_hdr[6] = ctx->slave_id;

        out_vec[current_iov_idx].iov_base = mbap_hdr;
        out_vec[current_iov_idx].iov_len = sizeof(mbap_hdr);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_fc;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_fc);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_addr;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_addr);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_qty;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_qty);
        current_iov_idx++;
    } else { // RTU
        uint8_t rtu_slave_id[1] = {ctx->slave_id};
        
        // For CRC calculation, we need contiguous data: Slave ID + FC + Addr + Qty
        uint8_t crc_data[1 + sizeof(pdu_fc) + sizeof(pdu_addr) + sizeof(pdu_qty)];
        crc_data[0] = rtu_slave_id[0];
        crc_data[1] = pdu_fc[0];
        memcpy(&crc_data[2], pdu_addr, sizeof(pdu_addr));
        memcpy(&crc_data[4], pdu_qty, sizeof(pdu_qty));

        uint16_t crc = edge_crc16_modbus(crc_data, sizeof(crc_data));
        uint8_t crc_bytes[2]; write_u16_le(crc_bytes, crc);

        out_vec[current_iov_idx].iov_base = rtu_slave_id;
        out_vec[current_iov_idx].iov_len = sizeof(rtu_slave_id);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_fc;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_fc);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_addr;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_addr);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_qty;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_qty);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = crc_bytes;
        out_vec[current_iov_idx].iov_len = sizeof(crc_bytes);
        current_iov_idx++;
    }
    *out_vec_capacity = current_iov_idx;
    return EDGE_OK;
}

edge_error_t edge_modbus_build_read_discrete_inputs_req(
    edge_modbus_context_t *ctx, uint16_t address, uint16_t quantity,
    struct iovec *out_vec, int *out_vec_capacity) {

    if (!ctx || !out_vec || !out_vec_capacity || quantity == 0 || quantity > 2000) {
        return EDGE_ERR_INVALID_ARG;
    }
    int segments_needed = ctx->is_tcp ? MODBUS_MAX_IOVEC_TCP : MODBUS_MAX_IOVEC_RTU;
    if (*out_vec_capacity < segments_needed) return EDGE_ERR_BUFFER_TOO_SMALL;
    int current_iov_idx = 0;

    // PDU components
    uint8_t pdu_fc[1] = {MODBUS_FC_READ_DISCRETE_INPUTS};
    uint8_t pdu_addr[2]; write_u16_be(pdu_addr, address);
    uint8_t pdu_qty[2]; write_u16_be(pdu_qty, quantity);
    
    if (ctx->is_tcp) {
        uint8_t mbap_hdr[7];
        ctx->transaction_id++;
        write_u16_be(&mbap_hdr[0], ctx->transaction_id);
        write_u16_be(&mbap_hdr[2], 0); // Protocol ID
        write_u16_be(&mbap_hdr[4], 1 + sizeof(pdu_fc) + sizeof(pdu_addr) + sizeof(pdu_qty)); // Length: unit ID + PDU
        mbap_hdr[6] = ctx->slave_id;

        out_vec[current_iov_idx].iov_base = mbap_hdr;
        out_vec[current_iov_idx].iov_len = sizeof(mbap_hdr);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_fc;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_fc);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_addr;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_addr);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_qty;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_qty);
        current_iov_idx++;
    } else { // RTU
        uint8_t rtu_slave_id[1] = {ctx->slave_id};
        
        uint8_t crc_data[1 + sizeof(pdu_fc) + sizeof(pdu_addr) + sizeof(pdu_qty)];
        crc_data[0] = rtu_slave_id[0];
        crc_data[1] = pdu_fc[0];
        memcpy(&crc_data[2], pdu_addr, sizeof(pdu_addr));
        memcpy(&crc_data[4], pdu_qty, sizeof(pdu_qty));

        uint16_t crc = edge_crc16_modbus(crc_data, sizeof(crc_data));
        uint8_t crc_bytes[2]; write_u16_le(crc_bytes, crc);

        out_vec[current_iov_idx].iov_base = rtu_slave_id;
        out_vec[current_iov_idx].iov_len = sizeof(rtu_slave_id);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_fc;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_fc);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_addr;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_addr);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_qty;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_qty);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = crc_bytes;
        out_vec[current_iov_idx].iov_len = sizeof(crc_bytes);
        current_iov_idx++;
    }
    *out_vec_capacity = current_iov_idx;
    return EDGE_OK;
}

edge_error_t edge_modbus_build_read_holding_registers_req(
    edge_modbus_context_t *ctx, uint16_t address, uint16_t quantity,
    struct iovec *out_vec, int *out_vec_capacity) {

    if (!ctx || !out_vec || !out_vec_capacity || quantity == 0 || quantity > 125) {
        return EDGE_ERR_INVALID_ARG;
    }
    int segments_needed = ctx->is_tcp ? MODBUS_MAX_IOVEC_TCP : MODBUS_MAX_IOVEC_RTU;
    if (*out_vec_capacity < segments_needed) return EDGE_ERR_BUFFER_TOO_SMALL;
    int current_iov_idx = 0;

    // PDU components
    uint8_t pdu_fc[1] = {MODBUS_FC_READ_HOLDING_REGISTERS};
    uint8_t pdu_addr[2]; write_u16_be(pdu_addr, address);
    uint8_t pdu_qty[2]; write_u16_be(pdu_qty, quantity);
    
    if (ctx->is_tcp) {
        uint8_t mbap_hdr[7];
        ctx->transaction_id++;
        write_u16_be(&mbap_hdr[0], ctx->transaction_id);
        write_u16_be(&mbap_hdr[2], 0); // Protocol ID
        write_u16_be(&mbap_hdr[4], 1 + sizeof(pdu_fc) + sizeof(pdu_addr) + sizeof(pdu_qty)); // Length: unit ID + PDU
        mbap_hdr[6] = ctx->slave_id;

        out_vec[current_iov_idx].iov_base = mbap_hdr;
        out_vec[current_iov_idx].iov_len = sizeof(mbap_hdr);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_fc;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_fc);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_addr;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_addr);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_qty;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_qty);
        current_iov_idx++;
    } else { // RTU
        uint8_t rtu_slave_id[1] = {ctx->slave_id};
        
        uint8_t crc_data[1 + sizeof(pdu_fc) + sizeof(pdu_addr) + sizeof(pdu_qty)];
        crc_data[0] = rtu_slave_id[0];
        crc_data[1] = pdu_fc[0];
        memcpy(&crc_data[2], pdu_addr, sizeof(pdu_addr));
        memcpy(&crc_data[4], pdu_qty, sizeof(pdu_qty));

        uint16_t crc = edge_crc16_modbus(crc_data, sizeof(crc_data));
        uint8_t crc_bytes[2]; write_u16_le(crc_bytes, crc);

        out_vec[current_iov_idx].iov_base = rtu_slave_id;
        out_vec[current_iov_idx].iov_len = sizeof(rtu_slave_id);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_fc;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_fc);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_addr;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_addr);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_qty;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_qty);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = crc_bytes;
        out_vec[current_iov_idx].iov_len = sizeof(crc_bytes);
        current_iov_idx++;
    }
    *out_vec_capacity = current_iov_idx;
    return EDGE_OK;
}

edge_error_t edge_modbus_build_write_single_register_req(
    edge_modbus_context_t *ctx, uint16_t address, uint16_t value,
    struct iovec *out_vec, int *out_vec_capacity) {

    if (!ctx || !out_vec || !out_vec_capacity) {
        return EDGE_ERR_INVALID_ARG;
    }
    int segments_needed = ctx->is_tcp ? MODBUS_MAX_IOVEC_TCP : MODBUS_MAX_IOVEC_RTU;
    if (*out_vec_capacity < segments_needed) return EDGE_ERR_BUFFER_TOO_SMALL;
    int current_iov_idx = 0;

    // PDU components
    uint8_t pdu_fc[1] = {MODBUS_FC_WRITE_SINGLE_REGISTER};
    uint8_t pdu_addr[2]; write_u16_be(pdu_addr, address);
    uint8_t pdu_val[2]; write_u16_be(pdu_val, value);
    
    if (ctx->is_tcp) {
        uint8_t mbap_hdr[7];
        ctx->transaction_id++;
        write_u16_be(&mbap_hdr[0], ctx->transaction_id);
        write_u16_be(&mbap_hdr[2], 0); // Protocol ID
        write_u16_be(&mbap_hdr[4], 1 + sizeof(pdu_fc) + sizeof(pdu_addr) + sizeof(pdu_val)); // Length: unit ID + PDU
        mbap_hdr[6] = ctx->slave_id;

        out_vec[current_iov_idx].iov_base = mbap_hdr;
        out_vec[current_iov_idx].iov_len = sizeof(mbap_hdr);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_fc;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_fc);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_addr;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_addr);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_val;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_val);
        current_iov_idx++;
    } else { // RTU
        uint8_t rtu_slave_id[1] = {ctx->slave_id};
        
        uint8_t crc_data[1 + sizeof(pdu_fc) + sizeof(pdu_addr) + sizeof(pdu_val)];
        crc_data[0] = rtu_slave_id[0];
        crc_data[1] = pdu_fc[0];
        memcpy(&crc_data[2], pdu_addr, sizeof(pdu_addr));
        memcpy(&crc_data[4], pdu_val, sizeof(pdu_val));

        uint16_t crc = edge_crc16_modbus(crc_data, sizeof(crc_data));
        uint8_t crc_bytes[2]; write_u16_le(crc_bytes, crc);

        out_vec[current_iov_idx].iov_base = rtu_slave_id;
        out_vec[current_iov_idx].iov_len = sizeof(rtu_slave_id);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_fc;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_fc);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_addr;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_addr);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_val;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_val);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = crc_bytes;
        out_vec[current_iov_idx].iov_len = sizeof(crc_bytes);
        current_iov_idx++;
    }
    *out_vec_capacity = current_iov_idx;
    return EDGE_OK;
}

edge_error_t edge_modbus_build_write_multiple_coils_req(
    edge_modbus_context_t *ctx,
    uint16_t address,
    uint16_t quantity,
    const uint8_t *data, // data is the packed coil values
    struct iovec *out_vec,
    int *out_vec_capacity) {
    
    if (!ctx || !out_vec || !out_vec_capacity || !data || quantity == 0 || quantity > 1968) {
        return EDGE_ERR_INVALID_ARG;
    }
    int segments_needed = ctx->is_tcp ? MODBUS_MAX_IOVEC_TCP + 2 : MODBUS_MAX_IOVEC_RTU + 1; // +2 for pdu_byte_count + data, +1 for pdu_byte_count
    if (*out_vec_capacity < segments_needed) return EDGE_ERR_BUFFER_TOO_SMALL;
    int current_iov_idx = 0;

    uint8_t byte_count = (quantity + 7) / 8;
    
    // PDU Header components
    uint8_t pdu_fc[1] = {MODBUS_FC_WRITE_MULTIPLE_COILS};
    uint8_t pdu_addr[2]; write_u16_be(pdu_addr, address);
    uint8_t pdu_qty[2]; write_u16_be(pdu_qty, quantity);
    uint8_t pdu_byte_count[1] = {byte_count};

    if (ctx->is_tcp) {
        uint8_t mbap_hdr[7];
        ctx->transaction_id++;
        write_u16_be(&mbap_hdr[0], ctx->transaction_id);
        write_u16_be(&mbap_hdr[2], 0); // Protocol ID
        write_u16_be(&mbap_hdr[4], 1 + sizeof(pdu_fc) + sizeof(pdu_addr) + sizeof(pdu_qty) + sizeof(pdu_byte_count) + byte_count); // Length
        mbap_hdr[6] = ctx->slave_id;

        out_vec[current_iov_idx].iov_base = mbap_hdr;
        out_vec[current_iov_idx].iov_len = sizeof(mbap_hdr);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_fc;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_fc);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_addr;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_addr);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_qty;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_qty);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_byte_count;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_byte_count);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = (void*)data; // Point to user data
        out_vec[current_iov_idx].iov_len = byte_count;
        current_iov_idx++;
    } else { // RTU
        uint8_t rtu_slave_id[1] = {ctx->slave_id};
        
        // For CRC calculation, we need contiguous data: Slave ID + PDU Hdr + Data
        uint8_t crc_data[1 + sizeof(pdu_fc) + sizeof(pdu_addr) + sizeof(pdu_qty) + sizeof(pdu_byte_count) + byte_count];
        crc_data[0] = rtu_slave_id[0];
        crc_data[1] = pdu_fc[0];
        memcpy(&crc_data[2], pdu_addr, sizeof(pdu_addr));
        memcpy(&crc_data[4], pdu_qty, sizeof(pdu_qty));
        crc_data[6] = pdu_byte_count[0];
        memcpy(&crc_data[7], data, byte_count);

        uint16_t crc = edge_crc16_modbus(crc_data, sizeof(crc_data));
        uint8_t crc_bytes[2]; write_u16_le(crc_bytes, crc);

        out_vec[current_iov_idx].iov_base = rtu_slave_id;
        out_vec[current_iov_idx].iov_len = sizeof(rtu_slave_id);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_fc;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_fc);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_addr;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_addr);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_qty;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_qty);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_byte_count;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_byte_count);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = (void*)data; // Point to user data
        out_vec[current_iov_idx].iov_len = byte_count;
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = crc_bytes;
        out_vec[current_iov_idx].iov_len = sizeof(crc_bytes);
        current_iov_idx++;
    }
    *out_vec_capacity = current_iov_idx;
    return EDGE_OK;
}

edge_error_t edge_modbus_build_write_multiple_registers_req(
    edge_modbus_context_t *ctx,
    uint16_t address,
    uint16_t quantity,
    const uint16_t *data, // data is the array of uint16_t values
    struct iovec *out_vec,
    int *out_vec_capacity) {

    if (!ctx || !out_vec || !out_vec_capacity || !data || quantity == 0 || quantity > 123) {
        return EDGE_ERR_INVALID_ARG;
    }
    int segments_needed = ctx->is_tcp ? MODBUS_MAX_IOVEC_TCP + 2 : MODBUS_MAX_IOVEC_RTU + 1; // +2 for pdu_byte_count + data, +1 for pdu_byte_count
    if (*out_vec_capacity < segments_needed) return EDGE_ERR_BUFFER_TOO_SMALL;
    int current_iov_idx = 0;

    uint8_t byte_count = quantity * 2;
    
    // PDU Header components
    uint8_t pdu_fc[1] = {MODBUS_FC_WRITE_MULTIPLE_REGISTERS};
    uint8_t pdu_addr[2]; write_u16_be(pdu_addr, address);
    uint8_t pdu_qty[2]; write_u16_be(pdu_qty, quantity);
    uint8_t pdu_byte_count[1] = {byte_count};

    // Data values (uint16_t array) need to be converted to uint8_t stream (Big Endian)
    // This requires a temporary buffer.
    uint8_t converted_data[byte_count]; // This buffer must be large enough
    for(uint16_t i = 0; i < quantity; i++) {
        write_u16_be(&converted_data[i*2], data[i]);
    }

    if (ctx->is_tcp) {
        uint8_t mbap_hdr[7];
        ctx->transaction_id++;
        write_u16_be(&mbap_hdr[0], ctx->transaction_id);
        write_u16_be(&mbap_hdr[2], 0); // Protocol ID
        write_u16_be(&mbap_hdr[4], 1 + sizeof(pdu_fc) + sizeof(pdu_addr) + sizeof(pdu_qty) + sizeof(pdu_byte_count) + byte_count); // Length
        mbap_hdr[6] = ctx->slave_id;

        out_vec[current_iov_idx].iov_base = mbap_hdr;
        out_vec[current_iov_idx].iov_len = sizeof(mbap_hdr);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_fc;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_fc);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_addr;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_addr);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_qty;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_qty);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_byte_count;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_byte_count);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = converted_data; // Point to converted data
        out_vec[current_iov_idx].iov_len = byte_count;
        current_iov_idx++;
    } else { // RTU
        uint8_t rtu_slave_id[1] = {ctx->slave_id}; // Declare rtu_slave_id here
        // For RTU, we need to assemble the header and data together for CRC calculation
        // This requires a temporary buffer.
        uint8_t rtu_frame_buf[1 + sizeof(pdu_fc) + sizeof(pdu_addr) + sizeof(pdu_qty) + sizeof(pdu_byte_count) + byte_count];
        rtu_frame_buf[0] = ctx->slave_id;
        memcpy(&rtu_frame_buf[1], pdu_fc, sizeof(pdu_fc));
        memcpy(&rtu_frame_buf[2], pdu_addr, sizeof(pdu_addr));
        memcpy(&rtu_frame_buf[4], pdu_qty, sizeof(pdu_qty));
        rtu_frame_buf[6] = pdu_byte_count[0];
        memcpy(&rtu_frame_buf[7], converted_data, byte_count); // Use converted data

        uint16_t crc = edge_crc16_modbus(rtu_frame_buf, sizeof(rtu_frame_buf));
        uint8_t crc_bytes[2]; write_u16_le(crc_bytes, crc);

        out_vec[current_iov_idx].iov_base = rtu_slave_id;
        out_vec[current_iov_idx].iov_len = sizeof(rtu_slave_id);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_fc;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_fc);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_addr;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_addr);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_qty;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_qty);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = pdu_byte_count;
        out_vec[current_iov_idx].iov_len = sizeof(pdu_byte_count);
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = converted_data; // Point to converted data
        out_vec[current_iov_idx].iov_len = byte_count;
        current_iov_idx++;
        out_vec[current_iov_idx].iov_base = crc_bytes;
        out_vec[current_iov_idx].iov_len = sizeof(crc_bytes);
        current_iov_idx++;
    }
    *out_vec_capacity = current_iov_idx;
    return EDGE_OK;
}
// ##############################################################################
// # PARSING LOGIC
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