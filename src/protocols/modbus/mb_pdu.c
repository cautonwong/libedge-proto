#include "protocols/edge_modbus.h"
#include "common/crc.h"
#include <string.h>

void edge_modbus_init(edge_modbus_context_t *ctx, uint8_t slave_id, bool is_tcp) {
    if (!ctx) return;
    ctx->slave_id = slave_id;
    ctx->is_tcp = is_tcp;
    ctx->transaction_id = 0;
}

edge_error_t edge_modbus_build_read_holding_req(edge_modbus_context_t *ctx, edge_vector_t *v, uint16_t addr, uint16_t quantity) {
    if (!ctx || !v) return EP_ERR_INVALID_ARG;

    if (ctx->is_tcp) {
        ctx->transaction_id++;
        EP_ASSERT_OK(edge_vector_put_be16(v, ctx->transaction_id));
        EP_ASSERT_OK(edge_vector_put_be16(v, 0));
        EP_ASSERT_OK(edge_vector_put_be16(v, 6));
    }

    EP_ASSERT_OK(edge_vector_put_u8(v, ctx->slave_id));
    EP_ASSERT_OK(edge_vector_put_u8(v, MODBUS_FC_READ_HOLDING_REGISTERS));
    EP_ASSERT_OK(edge_vector_put_be16(v, addr));
    EP_ASSERT_OK(edge_vector_put_be16(v, quantity));

    if (!ctx->is_tcp) {
        uint16_t crc = 0xFFFF;
        for (int i = 0; i < v->used_count; i++) {
            crc = edge_crc16_modbus_update(crc, v->iovs[i].iov_base, v->iovs[i].iov_len);
        }
        uint8_t crc_bytes[2] = { (uint8_t)(crc & 0xFF), (uint8_t)(crc >> 8) };
        EP_ASSERT_OK(edge_vector_append_copy(v, crc_bytes, 2));
    }
    return EP_OK;
}

edge_error_t edge_modbus_parse_response(edge_modbus_context_t *ctx, edge_cursor_t *c, uint8_t *out_fc, const uint8_t **out_data, size_t *out_len) {
    if (ctx->is_tcp) {
        uint16_t tid, pid, len;
        EP_ASSERT_OK(edge_cursor_read_be16(c, &tid));
        EP_ASSERT_OK(edge_cursor_read_be16(c, &pid));
        EP_ASSERT_OK(edge_cursor_read_be16(c, &len));
    }

    uint8_t dev_id, fc;
    EP_ASSERT_OK(edge_cursor_read_u8(c, &dev_id));
    EP_ASSERT_OK(edge_cursor_read_u8(c, &fc));
    
    if (fc & 0x80) {
        uint8_t exception;
        edge_cursor_read_u8(c, &exception);
        return EP_ERR_GENERIC; // [修正]
    }

    uint8_t byte_count;
    EP_ASSERT_OK(edge_cursor_read_u8(c, &byte_count));
    if (edge_cursor_remaining(c) < byte_count) return EP_ERR_INCOMPLETE_DATA;

    *out_fc = fc;
    *out_len = byte_count;
    *out_data = edge_cursor_get_ptr(c, byte_count);
    return EP_OK;
}
