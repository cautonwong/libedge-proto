#ifndef LIBEDGE_PROTOCOLS_MODBUS_H
#define LIBEDGE_PROTOCOLS_MODBUS_H

#include "edge_core.h"

/**
 * @brief Modbus Function Codes
 */
#define MODBUS_FC_READ_COILS            0x01
#define MODBUS_FC_READ_DISCRETE_INPUTS  0x02
#define MODBUS_FC_READ_HOLDING_REGISTERS 0x03
#define MODBUS_FC_READ_INPUT_REGISTERS   0x04
#define MODBUS_FC_WRITE_SINGLE_COIL      0x05
#define MODBUS_FC_WRITE_SINGLE_REGISTER  0x06
#define MODBUS_FC_WRITE_MULTIPLE_COILS   0x0F
#define MODBUS_FC_WRITE_MULTIPLE_REGISTERS 0x10

typedef struct {
    uint8_t slave_id;
    bool is_tcp;
    uint16_t transaction_id;
} edge_modbus_context_t;

// --- API ---

void edge_modbus_init(edge_modbus_context_t *ctx, uint8_t slave_id, bool is_tcp);

/**
 * @brief 构建读取保持寄存器请求 (0x03)
 */
edge_error_t edge_modbus_build_read_holding_req(edge_modbus_context_t *ctx, edge_vector_t *v, uint16_t addr, uint16_t quantity);

/**
 * @brief 解析 Modbus 响应报文
 */
edge_error_t edge_modbus_parse_response(edge_modbus_context_t *ctx, edge_cursor_t *c, uint8_t *out_fc, const uint8_t **out_data, size_t *out_len);

#endif // LIBEDGE_PROTOCOLS_MODBUS_H