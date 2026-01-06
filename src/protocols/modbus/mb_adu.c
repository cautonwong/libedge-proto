#include "protocols/edge_modbus.h"
#include "common/crc.h"
#include <string.h>

static void write_u16_be(uint8_t *buf, uint16_t value) {
    buf[0] = (uint8_t)(value >> 8);
    buf[1] = (uint8_t)(value & 0xFF);
}

static void write_u16_le(uint8_t *buf, uint16_t value) {
    buf[0] = (uint8_t)(value & 0xFF);
    buf[1] = (uint8_t)(value >> 8);
}

void edge_modbus_init_context(edge_modbus_context_t *ctx, uint8_t slave_id, bool is_tcp) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(edge_modbus_context_t));
    ctx->slave_id = slave_id;
    ctx->is_tcp = is_tcp;
}

// Generic builder for simple read requests
static edge_error_t build_simple_read_req(edge_modbus_context_t *ctx, edge_vector_t *v, uint8_t fc, uint16_t address, uint16_t quantity) {
    if (!ctx || !v) return EDGE_ERR_INVALID_ARG;

    // PDU: FC (1) + Addr (2) + Qty (2) = 5 bytes
    uint8_t *pdu = ctx->pdu_buf;
    pdu[0] = fc;
    write_u16_be(&pdu[1], address);
    write_u16_be(&pdu[3], quantity);
    
    if (ctx->is_tcp) {
        ctx->transaction_id++;
        uint8_t *mbap = ctx->hdr_buf;
        write_u16_be(&mbap[0], ctx->transaction_id);
        write_u16_be(&mbap[2], 0);
        write_u16_be(&mbap[4], 1 + 5);
        mbap[6] = ctx->slave_id;
        
        edge_vector_append_ref(v, mbap, 7);
        edge_vector_append_ref(v, pdu, 5);
    } else {
        ctx->hdr_buf[0] = ctx->slave_id;

        edge_vector_append_ref(v, ctx->hdr_buf, 1);
        edge_vector_append_ref(v, pdu, 5);

        uint8_t crc_data[1 + 5];
        memcpy(crc_data, v->iovs[0].iov_base, 1);
        memcpy(crc_data + 1, v->iovs[1].iov_base, 5);
        uint16_t crc = edge_crc16_modbus(crc_data, sizeof(crc_data));
        
        write_u16_le(ctx->crc_buf, crc);
        edge_vector_append_ref(v, ctx->crc_buf, 2);
    }
    return EDGE_OK;
}

edge_error_t edge_modbus_build_read_coils_req(
    edge_modbus_context_t *ctx, edge_vector_t *v, uint16_t address, uint16_t quantity) {
    return build_simple_read_req(ctx, v, MODBUS_FC_READ_COILS, address, quantity);
}

edge_error_t edge_modbus_build_read_discrete_inputs_req(
    edge_modbus_context_t *ctx, edge_vector_t *v, uint16_t address, uint16_t quantity) {
    return build_simple_read_req(ctx, v, MODBUS_FC_READ_DISCRETE_INPUTS, address, quantity);
}

edge_error_t edge_modbus_build_read_holding_registers(
    edge_modbus_context_t *ctx, edge_vector_t *v, uint16_t address, uint16_t quantity) {
    return build_simple_read_req(ctx, v, MODBUS_FC_READ_HOLDING_REGISTERS, address, quantity);
}

edge_error_t edge_modbus_build_write_single_register_req(
    edge_modbus_context_t *ctx, edge_vector_t *v, uint16_t address, uint16_t value) {
    if (!ctx || !v) return EDGE_ERR_INVALID_ARG;

    uint8_t *pdu = ctx->pdu_buf;
    pdu[0] = MODBUS_FC_WRITE_SINGLE_REGISTER;
    write_u16_be(&pdu[1], address);
    write_u16_be(&pdu[3], value);

    if (ctx->is_tcp) {
        ctx->transaction_id++;
        uint8_t *mbap = ctx->hdr_buf;
        write_u16_be(&mbap[0], ctx->transaction_id);
        write_u16_be(&mbap[2], 0);
        write_u16_be(&mbap[4], 1 + 5);
        mbap[6] = ctx->slave_id;
        
        edge_vector_append_ref(v, mbap, 7);
        edge_vector_append_ref(v, pdu, 5);
    } else {
        ctx->hdr_buf[0] = ctx->slave_id;
        edge_vector_append_ref(v, ctx->hdr_buf, 1);
        edge_vector_append_ref(v, pdu, 5);

        uint8_t crc_data[1 + 5];
        memcpy(crc_data, v->iovs[0].iov_base, 1);
        memcpy(crc_data + 1, v->iovs[1].iov_base, 5);
        uint16_t crc = edge_crc16_modbus(crc_data, sizeof(crc_data));
        
        write_u16_le(ctx->crc_buf, crc);
        edge_vector_append_ref(v, ctx->crc_buf, 2);
    }
    return EDGE_OK;
}