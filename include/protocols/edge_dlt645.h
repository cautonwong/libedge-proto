#ifndef LIBEDGE_PROTOCOLS_DLT645_H
#define LIBEDGE_PROTOCOLS_DLT645_H

#include "edge_core.h"

typedef struct {
    uint8_t addr_bcd[6];
} edge_dlt645_context_t;

void edge_dlt645_init(edge_dlt645_context_t *ctx, const char *addr_str);

/**
 * @brief 构建读数据请求
 * @param di 数据标识 (例如 0x00010000)
 */
edge_error_t edge_dlt645_build_read_req(edge_dlt645_context_t *ctx, edge_vector_t *v, uint32_t di);

/**
 * @brief 解析 DLT645 响应帧并提取数据域
 */
edge_error_t edge_dlt645_parse_frame(edge_dlt645_context_t *ctx, edge_cursor_t *c, uint32_t *out_di, const uint8_t **out_data, size_t *out_len);

#endif // LIBEDGE_PROTOCOLS_DLT645_H
