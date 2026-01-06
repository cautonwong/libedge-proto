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

// Modbus Exception Codes
typedef enum {
    MODBUS_EX_NONE                          = 0x00,
    MODBUS_EX_ILLEGAL_FUNCTION              = 0x01,
    MODBUS_EX_ILLEGAL_DATA_ADDRESS          = 0x02,
    MODBUS_EX_ILLEGAL_DATA_VALUE            = 0x03,
    MODBUS_EX_SERVER_DEVICE_FAILURE         = 0x04,
} edge_modbus_exception_code_t;


// Max iovec segments needed. Write Multiple RTU is the largest:
// SlaveID(1)+FC(1)+Addr(1)+Qty(1)+ByteCount(1)+Data(1)+CRC(1) = 7
#define MODBUS_MAX_IOVEC 8 // Use a safe-high value

typedef struct {
    bool is_tcp;
    uint8_t slave_id;
    uint16_t transaction_id;
    
    // Tiny scratch buffer for small, stack-allocated items (e.g., FC, Length, etc.)
    // Must be large enough for the largest single scratch-needed component (e.g., MBAP Header is 7 bytes).
    uint8_t tiny_scratch_buf[8]; 
    uint8_t crc_buf[2]; // For RTU CRC
    // Buffer for converting user-provided uint16_t data to uint8_t stream (for write multiple registers)
    uint8_t converted_data_buf[256]; 
} edge_modbus_context_t;


// --- Callback Definitions ---

// Callback function pointer types for master responses
typedef void (*on_read_response_t)(edge_modbus_context_t *ctx, uint16_t address, uint16_t quantity, const uint8_t *data, uint8_t byte_count);
typedef void (*on_write_single_response_t)(edge_modbus_context_t *ctx, uint16_t address, uint16_t value);
typedef void (*on_write_multiple_response_t)(edge_modbus_context_t *ctx, uint16_t address, uint16_t quantity);
typedef void (*on_exception_response_t)(edge_modbus_context_t *ctx, uint8_t function_code, uint8_t exception_code);

// Callback structure
typedef struct {
    on_read_response_t on_read_coils_response;
    on_read_response_t on_read_discrete_inputs_response;
    on_read_response_t on_read_holding_registers_response;
    on_read_response_t on_read_input_registers_response;
    on_write_single_response_t on_write_single_coil_response;
    on_write_single_response_t on_write_single_register_response;
    on_write_multiple_response_t on_write_multiple_coils_response;
    on_write_multiple_response_t on_write_multiple_registers_response;
    on_exception_response_t on_exception_response;
} edge_modbus_callbacks_t;


// --- Public API Functions ---

void edge_modbus_init_context(edge_modbus_context_t *ctx, uint8_t slave_id, bool is_tcp);

// Builders
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

// Parser
edge_error_t edge_modbus_parse_frame(
    edge_modbus_context_t *ctx,
    const uint8_t *frame_data,
    size_t frame_len,
    const edge_modbus_callbacks_t *callbacks);


#endif // LIBEDGE_PROTOCOLS_MODBUS_H