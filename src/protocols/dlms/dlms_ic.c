#include "protocols/edge_dlms.h"

/**
 * @brief 解析 Register (IC 3) 的数据负载
 * 结构：Structure { Value, Scaler_Unit }
 */
edge_error_t edge_dlms_decode_register(edge_cursor_t *c, double *out_val) {
    edge_dlms_variant_t var;
    EDGE_ASSERT_OK(edge_dlms_decode_variant(c, &var));
    
    if (var.tag == DLMS_TAG_STRUCTURE) {
        // 进入结构体
        edge_dlms_variant_t val_var, scaler_var;
        EDGE_ASSERT_OK(edge_dlms_decode_variant(c, &val_var));
        EDGE_ASSERT_OK(edge_dlms_decode_variant(c, &scaler_var));
        
        // 此处执行 Scaler 运算逻辑...
        *out_val = 123.45; // 演示
    }
    return EDGE_OK;
}
