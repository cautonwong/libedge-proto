#ifndef LIBEDGE_EDGE_DLT698_H
#define LIBEDGE_EDGE_DLT698_H

#include "edge_common.h"

#ifdef __cplusplus
extern "C" {
#endif

// DL/T 698 Data Element Tags
typedef enum {
    DLT698_DATA_NULL_DATA = 0,
    DLT698_DATA_ARRAY = 1,
    DLT698_DATA_STRUCTURE = 2,
    DLT698_DATA_BOOL = 3,
    DLT698_DATA_BIT_STRING = 4,
    DLT698_DATA_DOUBLE_LONG = 5,
    DLT698_DATA_DOUBLE_LONG_UNSIGNED = 6,
    DLT698_DATA_OCTET_STRING = 9,
    DLT698_DATA_VISIBLE_STRING = 10,
    DLT698_DATA_UTF8_STRING = 12,
    DLT698_DATA_INTEGER = 15,
    DLT698_DATA_LONG = 16,
    DLT698_DATA_UNSIGNED = 17,
    DLT698_DATA_LONG_UNSIGNED = 18,
    // ... other tags
} edge_dlt698_data_tag_t;

// A structure to represent a variable-length string, pointing into a source buffer
typedef struct {
    const uint8_t *ptr;
    size_t len;
} dlt698_string_t;

// A structure to hold a single parsed DL/T 698 'Data' element
typedef struct {
    edge_dlt698_data_tag_t tag;
    union {
        bool bool_val;
        uint8_t u8_val;
        int8_t i8_val;
        uint16_t u16_val;
        int16_t i16_val;
        dlt698_string_t string_val;
        size_t num_elements; // For array and structure
        // Other value types to be added
    } value;
} edge_dlt698_data_t;

/**
 * @brief Parses a single 'Data' element from a buffer.
 * 
 * @param buffer The input buffer containing the encoded data.
 * @param len The length of the input buffer.
 * @param data_out A pointer to a struct where the parsed data will be stored.
 * @param consumed A pointer to a variable that will be set to the number of bytes consumed from the buffer.
 * @return edge_error_t EDGE_OK on success, or an error code.
 */
edge_error_t edge_dlt698_parse_data(const uint8_t *buffer, size_t len, edge_dlt698_data_t *data_out, size_t *consumed);


#ifdef __cplusplus
}
#endif

#endif // LIBEDGE_EDGE_DLT698_H
