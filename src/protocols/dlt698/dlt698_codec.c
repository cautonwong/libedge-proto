#include "edge_core.h"
#include <string.h>

/**
 * @brief DL/T 698.45 Data Tags
 */
typedef enum {
    D698_TAG_NULL       = 0,
    D698_TAG_ARRAY      = 1,
    D698_TAG_STRUCTURE  = 2,
    D698_TAG_BOOL       = 3,
    D698_TAG_BITSTRING  = 4,
    D698_TAG_DOUBLE_LONG = 5, // int32
    D698_TAG_OCTET_STRING = 9,
    D698_TAG_LONG_UNSIGNED = 18, // uint16
    D698_TAG_ENUM       = 22
} d698_data_tag_t;

/**
 * @brief 编码 698 变长整数 (OAD 等)
 */
edge_error_t edge_d698_encode_oad(edge_vector_t *v, uint32_t oad) {
    return edge_vector_put_be32(v, oad);
}

/**
 * @brief 高质量 698 结果集解析器
 */
edge_error_t edge_d698_parse_data(edge_cursor_t *c, d698_data_tag_t *tag, const uint8_t **payload, size_t *len) {
    uint8_t t;
    EP_ASSERT_OK(edge_cursor_read_u8(c, &t));
    *tag = (d698_data_tag_t)t;
    
    switch(t) {
        case D698_TAG_OCTET_STRING: {
            uint8_t l;
            EP_ASSERT_OK(edge_cursor_read_u8(c, &l));
            *len = l;
            *payload = edge_cursor_get_ptr(c, l);
            break;
        }
        case D698_TAG_LONG_UNSIGNED: {
            *len = 2;
            *payload = edge_cursor_get_ptr(c, 2);
            break;
        }
        default: return EP_ERR_NOT_SUPPORTED;
    }
    return EP_OK;
}
