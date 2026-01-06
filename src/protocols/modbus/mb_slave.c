#include "protocols/edge_modbus.h"
#include <string.h>

/**
 * @brief 从站 PDU 处理器
 * 自动处理 03, 04, 06, 16 等核心功能码
 */
edge_error_t edge_modbus_slave_handle_pdu(edge_modbus_context_t *ctx, edge_cursor_t *req, edge_vector_t *resp, 
                                          uint16_t *holding_regs, size_t reg_count) {
    uint8_t fc;
    EDGE_ASSERT_OK(edge_cursor_read_u8(req, &fc));
    
    uint16_t addr, qty;
    switch (fc) {
        case 0x03: // Read Holding Registers
            EDGE_ASSERT_OK(edge_cursor_read_be16(req, &addr));
            EDGE_ASSERT_OK(edge_cursor_read_be16(req, &qty));
            
            if (addr + qty > reg_count) {
                edge_vector_put_u8(resp, fc | 0x80);
                edge_vector_put_u8(resp, 0x02); // Illegal Address
                return EDGE_OK;
            }
            
            edge_vector_put_u8(resp, fc);
            edge_vector_put_u8(resp, (uint8_t)(qty * 2));
            for (int i = 0; i < qty; i++) {
                edge_vector_put_be16(resp, holding_regs[addr + i]);
            }
            break;
            
        case 0x06: // Write Single Register
            EDGE_ASSERT_OK(edge_cursor_read_be16(req, &addr));
            uint16_t val;
            EDGE_ASSERT_OK(edge_cursor_read_be16(req, &val));
            
            if (addr >= reg_count) return EDGE_ERR_OUT_OF_BOUNDS;
            holding_regs[addr] = val;
            
            // Echo back for 06
            edge_vector_put_u8(resp, fc);
            edge_vector_put_be16(resp, addr);
            edge_vector_put_be16(resp, val);
            break;
            
        default:
            edge_vector_put_u8(resp, fc | 0x80);
            edge_vector_put_u8(resp, 0x01); // Illegal Function
            break;
    }
    
    return EDGE_OK;
}
