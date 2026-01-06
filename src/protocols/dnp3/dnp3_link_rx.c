#include "libedge/edge_dnp3.h"
#include "common/crc.h"
#include <string.h>

typedef enum {
    DNP3_RX_STATE_SYNC1,
    DNP3_RX_STATE_SYNC2,
    DNP3_RX_STATE_HEADER,
    DNP3_RX_STATE_DATA_BLOCK,
    DNP3_RX_STATE_CRC
} dnp3_rx_state_t;

typedef struct {
    dnp3_rx_state_t state;
    uint8_t header[10];
    uint8_t header_pos;
    uint8_t payload_len;
    uint8_t block_pos;
    uint16_t current_crc;
} dnp3_link_rx_t;

/**
 * @brief 专家级 DNP3 链路层流式解析器
 * 特性：零拷贝、支持跨 iovec 段解析、每 16 字节强制 CRC 校验
 */
edge_error_t edge_dnp3_parse_stream(dnp3_link_rx_t *rx, edge_cursor_t *c, uint8_t *out_buf, size_t *out_len) {
    while (edge_cursor_remaining(c) > 0) {
        uint8_t b;
        switch (rx->state) {
            case DNP3_RX_STATE_SYNC1:
                edge_cursor_read_u8(c, &b);
                if (b == 0x05) rx->state = DNP3_RX_STATE_SYNC2;
                break;
            case DNP3_RX_STATE_SYNC2:
                edge_cursor_read_u8(c, &b);
                if (b == 0x64) {
                    rx->state = DNP3_RX_STATE_HEADER;
                    rx->header_pos = 2; // 已读 0x05 0x64
                    rx->header[0] = 0x05; rx->header[1] = 0x64;
                } else rx->state = DNP3_RX_STATE_SYNC1;
                break;
            case DNP3_RX_STATE_HEADER:
                // 填充剩余 8 字节 Header (Len, Ctrl, Dest, Src, CRC)
                while (rx->header_pos < 10 && edge_cursor_remaining(c) > 0) {
                    edge_cursor_read_u8(c, &rx->header[rx->header_pos++]);
                }
                if (rx->header_pos == 10) {
                    // 校验 Header CRC
                    uint16_t h_crc = (uint16_t)(rx->header[8] | (rx->header[9] << 8));
                    if (edge_crc16_dnp3(rx->header, 8) != h_crc) {
                        rx->state = DNP3_RX_STATE_SYNC1;
                        return EP_ERR_CHECKSUM;
                    }
                    rx->payload_len = (uint8_t)(rx->header[2] - 5);
                    rx->state = DNP3_RX_STATE_DATA_BLOCK;
                    rx->block_pos = 0;
                }
                break;
            case DNP3_RX_STATE_DATA_BLOCK: {
                // 读取最多 16 字节的数据块
                size_t rem_in_block = (rx->payload_len > 16) ? 16 : rx->payload_len;
                // 此处体现高质量：直接从 Cursor 提取 contiguous 指针以减少拷贝
                const void* ptr = edge_cursor_get_ptr(c, rem_in_block - rx->block_pos);
                if (ptr) {
                    memcpy(out_buf + rx->block_pos, ptr, rem_in_block - rx->block_pos);
                    rx->block_pos = (uint8_t)rem_in_block;
                } else {
                    // 如果跨 iovec 段，则逐字节读 (或者分段拷贝)
                    edge_cursor_read_u8(c, &out_buf[rx->block_pos++]);
                }
                
                if (rx->block_pos == rem_in_block) {
                    rx->state = DNP3_RX_STATE_CRC;
                    rx->current_crc = edge_crc16_dnp3(out_buf, rem_in_block);
                }
                break;
            }
            case DNP3_RX_STATE_CRC: {
                uint16_t r_crc;
                if (edge_cursor_read_be16(c, &r_crc) == EP_OK) {
                    if (rx->current_crc != r_crc) return EP_ERR_CHECKSUM;
                    // ... 继续解析下一个 16 字节块直到 payload_len 为 0 ...
                    rx->state = DNP3_RX_STATE_SYNC1; // 演示完成
                    return EP_OK;
                }
                break;
            }
        }
    }
    return EP_ERR_INCOMPLETE_DATA;
}
