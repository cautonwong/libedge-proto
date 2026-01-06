#include "edge_core.h"

/**
 * @brief DL/T 698.45 Service Tags
 */
typedef enum {
    D698_SERVICE_GET_REQUEST    = 0x05,
    D698_SERVICE_GET_RESPONSE   = 0x85,
    D698_SERVICE_SET_REQUEST    = 0x06,
    D698_SERVICE_SET_RESPONSE   = 0x86,
    D698_SERVICE_ACTION_REQUEST = 0x07,
    D698_SERVICE_REPORT_NOTIF   = 0x88
} d698_service_tag_t;

/**
 * @brief 构建 GET-Request-Normal
 */
edge_error_t edge_d698_build_get_request(edge_vector_t *v, uint32_t oad) {
    EP_ASSERT_OK(edge_vector_put_u8(v, D698_SERVICE_GET_REQUEST));
    EP_ASSERT_OK(edge_vector_put_u8(v, 0x01)); // Method: Normal
    
    // OAD (4 bytes)
    return edge_vector_put_be32(v, oad);
}

/**
 * @brief 构建 ACTION-Request-Normal
 */
edge_error_t edge_d698_build_action_request(edge_vector_t *v, uint32_t omad, const uint8_t *data, size_t len) {
    EP_ASSERT_OK(edge_vector_put_u8(v, D698_SERVICE_ACTION_REQUEST));
    EP_ASSERT_OK(edge_vector_put_u8(v, 0x01)); // Method: Normal
    
    EP_ASSERT_OK(edge_vector_put_be32(v, omad));
    // 后续对接 698 Data 类型编解码
    return edge_vector_append_ref(v, data, len);
}
