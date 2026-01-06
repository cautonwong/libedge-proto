#ifndef LIBEDGE_PROTOCOLS_MODBUS_H
#define LIBEDGE_PROTOCOLS_MODBUS_H

#include "edge_core.h"

// Function Codes
typedef enum {
    MODBUS_FC_READ_COILS                    = 0x01,
    MODBUS_FC_READ_DISCRETE_INPUTS          = 0x02,
    MODBUS_FC_READ_HOLDING_REGISTERS        = 0x03,
    MODBUS_FC_READ_INPUT_REGISTERS          = 0x04,
    MODBUS_FC_WRITE_SINGLE_COIL             = 0x05,
    MODBUS_FC_WRITE_SINGLE_REGISTER         = 0x06,
    MODBUS_FC_WRITE_MULTIPLE_COILS          = 0x0F,
    MODBUS_FC_WRITE_MULTIPLE_REGISTERS      = 0x10,
} edge_modbus_function_code_t;

// Max iovec segments needed. Write Multiple RTU is the largest:
// SlaveID(1)+FC(1)+Addr(1)+Qty(1)+ByteCount(1)+Data(1)+CRC(1) = 7
#define MODBUS_MAX_IOVEC 8 // Use a safe-high value

typedef struct {
    bool is_tcp;
    uint8_t slave_id;
    uint16_t transaction_id;
    
    // Buffers for stack-allocated iovec data that must live until writev is called
    uint8_t hdr_buf[7]; 
    uint8_t pdu_buf[256]; // For building PDU components
    uint8_t crc_buf[2];
    uint8_t converted_data_buf[256]; // For write multiple registers
} edge_modbus_context_t;


// --- Public API Functions ---

void edge_modbus_init_context(edge_modbus_context_t *ctx, uint8_t slave_id, bool is_tcp);

edge_error_t edge_modbus_build_read_coils_req(
    edge_modbus_context_t *ctx, edge_vector_t *v, uint16_t address, uint16_t quantity);

edge_error_t edge_modbus_build_read_discrete_inputs_req(
    edge_modbus_context_t *ctx, edge_vector_t *v, uint16_t address, uint16_t quantity);

edge_error_t edge_modbus_build_read_holding_registers(
    edge_modbus_context_t *ctx, edge_vector_t *v, uint16_t address, uint16_t quantity);

edge_error_t edge_modbus_build_write_single_register_req(
    edge_modbus_context_t *ctx, edge_vector_t *v, uint16_t address, uint16_t value);

edge_error_t edge_modbus_build_write_multiple_coils_req(
    edge_modbus_context_t *ctx, edge_vector_t *v, uint16_t address, uint16_t quantity, const uint8_t *data);

edge_error_t edge_modbus_build_write_multiple_registers_req(
    edge_modbus_context_t *ctx, edge_vector_t *v, uint16_t address, uint16_t quantity, const uint16_t *data);


#endif // LIBEDGE_PROTOCOLS_MODBUS_H