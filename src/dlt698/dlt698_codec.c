#include "libedge/edge_dlt698.h"

static uint16_t read_u16_be(const uint8_t *buf) {
    return (uint16_t)(((uint16_t)buf[0] << 8) | buf[1]);
}

// Parses the variable-length 'length' field.
// Returns length, and sets 'consumed_len' to the number of bytes the length field itself took.
static size_t parse_length(const uint8_t *buffer, size_t len, size_t *consumed_len) {
    if (len < 1) {
        *consumed_len = 0;
        return 0;
    }
    
    // For now, only implementing the simple case where length < 128
    if (buffer[0] < 0x80) {
        *consumed_len = 1;
        return buffer[0];
    } else {
        // TODO: Implement multi-byte length parsing (0x81, 0x82, etc.)
        *consumed_len = 0;
        return 0;
    }
}


edge_error_t edge_dlt698_parse_data(const uint8_t *buffer, size_t len, edge_dlt698_data_t *data_out, size_t *consumed) {
    if (!buffer || !data_out || !consumed || len == 0) {
        return EDGE_ERR_INVALID_ARG;
    }

    *consumed = 0;
    size_t total_consumed = 0;

    uint8_t tag = buffer[total_consumed++];
    data_out->tag = (edge_dlt698_data_tag_t)tag;

    switch (tag) {
        case DLT698_DATA_INTEGER: {
            if (len < 2) return EDGE_ERR_BUFFER_TOO_SMALL;
            data_out->value.i8_val = (int8_t)buffer[total_consumed++];
            break;
        }
        case DLT698_DATA_UNSIGNED: {
            if (len < 2) return EDGE_ERR_BUFFER_TOO_SMALL;
            data_out->value.u8_val = buffer[total_consumed++];
            break;
        }
        case DLT698_DATA_LONG: {
            if (len < 3) return EDGE_ERR_BUFFER_TOO_SMALL;
            data_out->value.i16_val = (int16_t)read_u16_be(&buffer[total_consumed]);
            total_consumed += 2;
            break;
        }
        case DLT698_DATA_LONG_UNSIGNED: {
            if (len < 3) return EDGE_ERR_BUFFER_TOO_SMALL;
            data_out->value.u16_val = read_u16_be(&buffer[total_consumed]);
            total_consumed += 2;
            break;
        }
        case DLT698_DATA_OCTET_STRING: {
            if (len < 2) return EDGE_ERR_BUFFER_TOO_SMALL;
            size_t len_consumed = 0;
            size_t string_len = parse_length(&buffer[total_consumed], len - total_consumed, &len_consumed);
            if (len_consumed == 0) return EDGE_ERR_INVALID_FRAME;
            total_consumed += len_consumed;

            if (len < total_consumed + string_len) return EDGE_ERR_BUFFER_TOO_SMALL;

            data_out->value.string_val.ptr = &buffer[total_consumed];
            data_out->value.string_val.len = string_len;
            total_consumed += string_len;
            break;
        }
        case DLT698_DATA_ARRAY:
        case DLT698_DATA_STRUCTURE: {
            if (len < 2) return EDGE_ERR_BUFFER_TOO_SMALL;
            size_t len_consumed = 0;
            size_t num_elements = parse_length(&buffer[total_consumed], len - total_consumed, &len_consumed);
            if (len_consumed == 0) return EDGE_ERR_INVALID_FRAME;
            total_consumed += len_consumed;
            
            data_out->value.num_elements = num_elements;
            break;
        }

        default:
            // Unhandled type
            return EDGE_ERR_INVALID_FRAME;
    }

    *consumed = total_consumed;
    return EDGE_OK;
}
