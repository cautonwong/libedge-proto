#include "libedge/edge_iec104.h"
#include <string.h>

/**
 * @brief 构建 Type 1: 单点信息 (M_SP_NA_1)
 */
edge_error_t edge_iec104_build_type1(edge_vector_t *v, uint32_t ioa, bool val, uint8_t qds) {
    EP_ASSERT_OK(edge_vector_put_u8(v, (uint8_t)(ioa & 0xFF)));
    EP_ASSERT_OK(edge_vector_put_u8(v, (uint8_t)((ioa >> 8) & 0xFF)));
    EP_ASSERT_OK(edge_vector_put_u8(v, (uint8_t)((ioa >> 16) & 0xFF)));
    
    uint8_t siq = val ? 0x01 : 0x00;
    siq |= (qds & 0xFE); // 保留品质描述位
    return edge_vector_put_u8(v, siq);
}

/**
 * @brief 构建 Type 45: 单命令 (C_SC_NA_1) - 遥控
 */
edge_error_t edge_iec104_build_type45(edge_vector_t *v, uint32_t ioa, bool val, bool select) {
    EP_ASSERT_OK(edge_vector_put_u8(v, (uint8_t)(ioa & 0xFF)));
    EP_ASSERT_OK(edge_vector_put_u8(v, (uint8_t)((ioa >> 8) & 0xFF)));
    EP_ASSERT_OK(edge_vector_put_u8(v, (uint8_t)((ioa >> 16) & 0xFF)));
    
    uint8_t sco = val ? 0x01 : 0x00;
    if (select) sco |= 0x80; // S/E bit (Select/Execute)
    return edge_vector_put_u8(v, sco);
}

/**
 * @brief 实现 Type 30 (带时标的单点)
 */
edge_error_t edge_iec104_build_type30(edge_vector_t *v, uint32_t ioa, bool val, const uint8_t *cp56) {
    EP_ASSERT_OK(edge_iec104_build_type1(v, ioa, val, 0));
    return edge_vector_append_copy(v, cp56, 7);
}
