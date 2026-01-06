#include "edge_core.h"
#include <string.h>

typedef enum {
    DNP3_FC_CONFIRM     = 0x00,
    DNP3_FC_READ        = 0x01,
    DNP3_FC_WRITE       = 0x02,
    DNP3_FC_SELECT      = 0x03,
    DNP3_FC_OPERATE     = 0x04
} dnp3_function_code_t;

typedef struct {
    uint8_t group;
    uint8_t variation;
    uint8_t qualifier;
    uint32_t range_start;
    uint32_t range_stop;
} dnp3_object_header_t;

/**
 * @brief 解析 DNP3 对象头 (Object Header)
 */
edge_error_t dnp3_parse_object_header(edge_cursor_t *c, dnp3_object_header_t *head) {
    EDGE_ASSERT_OK(edge_cursor_read_u8(c, &head->group));
    EDGE_ASSERT_OK(edge_cursor_read_u8(c, &head->variation));
    EDGE_ASSERT_OK(edge_cursor_read_u8(c, &head->qualifier));
    
    // 根据 Qualifier 解析范围 (Range)
    uint8_t q_code = head->qualifier & 0x0F;
    if (q_code == 0x00) { // 1-byte start/stop
        uint8_t s, e;
        edge_cursor_read_u8(c, &s); edge_cursor_read_u8(c, &e);
        head->range_start = s; head->range_stop = e;
    } else if (q_code == 0x01) { // 2-byte start/stop
        uint16_t s, e;
        edge_cursor_read_be16(c, &s); edge_cursor_read_be16(c, &e);
        head->range_start = s; head->range_stop = e;
    }
    
    return EDGE_OK;
}

/**
 * @brief 构建应用层读请求
 */
edge_error_t dnp3_build_read_request(edge_vector_t *v, uint8_t app_seq, uint8_t group, uint8_t variation) {
    // Application Control: FIR, FIN, CON, SEQ
    uint8_t app_ctrl = 0xC0 | (app_seq & 0x0F);
    EDGE_ASSERT_OK(edge_vector_put_u8(v, app_ctrl));
    EDGE_ASSERT_OK(edge_vector_put_u8(v, DNP3_FC_READ));
    
    // Object Header: Group, Variation, Qualifier=0x06 (All objects)
    EDGE_ASSERT_OK(edge_vector_put_u8(v, group));
    EDGE_ASSERT_OK(edge_vector_put_u8(v, variation));
    EDGE_ASSERT_OK(edge_vector_put_u8(v, 0x06));
    
    return EDGE_OK;
}
