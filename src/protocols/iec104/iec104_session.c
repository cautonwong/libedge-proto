#include "libedge/edge_iec104.h"
#include <string.h>

/**
 * @brief 处理接收到的有效控制域
 */
edge_error_t edge_iec104_session_on_recv(edge_iec104_context_t *ctx, edge_cursor_t *c, edge_vector_t *resp) {
    uint16_t ctrl1, ctrl2;
    
    // 1. 调用高质量解析器提取控制域 (内部处理小端转换)
    EDGE_ASSERT_OK(edge_iec104_parse_apci(c, &ctrl1, &ctrl2));

    if (ctrl1 & 0x01) {
        if (ctrl1 & 0x02) { // U-Frame
            uint8_t type = (uint8_t)(ctrl1 & 0xFC);
            if (type == 0x40) { // TESTFR ACT
                uint8_t con[] = {0x68, 0x04, 0x83, 0x00, 0x00, 0x00};
                edge_vector_append_copy(resp, con, sizeof(con));
            }
        } else { // S-Frame (Ack)
            ctx->v_a = (uint16_t)(ctrl2 >> 1);
        }
    } else { // I-Frame (Data)
        // 规约：V(R) 更新为收到的 V(S) + 1
        ctx->v_r = (uint16_t)(((ctrl1 >> 1) + 1) & 0x7FFF);
    }
    
    return EDGE_OK;
}
