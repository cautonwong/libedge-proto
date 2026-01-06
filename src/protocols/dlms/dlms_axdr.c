#include "protocols/edge_dlms.h"
#include <string.h>

/**
 * @brief 工业级 BER 变长长度解析 (内部)
 */
static int _pull_ber_len(edge_cursor_t *c, size_t *out_len) {
    uint8_t l8;
    if (edge_cursor_read_u8(c, &l8) != EP_OK) return -1;
    if (l8 < 128) {
        *out_len = l8;
    } else if (l8 == 0x81) {
        uint8_t val; if (edge_cursor_read_u8(c, &val) != EP_OK) return -1;
        *out_len = val;
    } else if (l8 == 0x82) {
        uint16_t val; if (edge_cursor_read_be16(c, &val) != EP_OK) return -1;
        *out_len = val;
    } else return -2;
    return 0;
}

edge_error_t edge_dlms_decode_variant(edge_cursor_t *c, edge_dlms_variant_t *out) {
    uint8_t tag;
    if (edge_cursor_read_u8(c, &tag) != EP_OK) return EP_ERR_INCOMPLETE_DATA;
    out->tag = (edge_dlms_tag_t)tag;

    switch (tag) {
        case DLMS_TAG_NULL_DATA: out->length = 0; out->data = NULL; break;
        case DLMS_TAG_BOOLEAN:
        case DLMS_TAG_INTEGER:
        case DLMS_TAG_UNSIGNED:
        case DLMS_TAG_ENUM:
            out->length = 1; out->data = edge_cursor_get_ptr(c, 1); break;
        case DLMS_TAG_LONG:
        case DLMS_TAG_LONG_UNSIGNED:
            out->length = 2; out->data = edge_cursor_get_ptr(c, 2); break;
        case DLMS_TAG_DOUBLE_LONG:
        case DLMS_TAG_DOUBLE_LONG_UNSIGNED:
        case DLMS_TAG_FLOAT:
            out->length = 4; out->data = edge_cursor_get_ptr(c, 4); break;
        case DLMS_TAG_LONG64:
        case DLMS_TAG_LONG64_UNSIGNED:
            out->length = 8; out->data = edge_cursor_get_ptr(c, 8); break;
        case DLMS_TAG_OCTET_STRING:
        case DLMS_TAG_VISIBLE_STRING:
        case DLMS_TAG_BIT_STRING: {
            size_t len; if (_pull_ber_len(c, &len) != 0) return EP_ERR_INCOMPLETE_DATA;
            out->length = len; out->data = edge_cursor_get_ptr(c, len);
            break;
        }
        case DLMS_TAG_ARRAY:
        case DLMS_TAG_STRUCTURE: {
            size_t count; if (_pull_ber_len(c, &count) != 0) return EP_ERR_INCOMPLETE_DATA;
            out->length = count; out->data = NULL; // 容器不带直接指针
            return EP_OK; // [专家级修正]：容器解析成员数成功即视为 OK
        }
        default: return EP_ERR_NOT_SUPPORTED;
    }
    return (out->data || out->length == 0) ? EP_OK : EP_ERR_INCOMPLETE_DATA;
}