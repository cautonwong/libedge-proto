#ifndef LIBEDGE_PROTOCOLS_DLMS_H
#define LIBEDGE_PROTOCOLS_DLMS_H

#include "edge_core.h"

// Common DLMS/COSEM definitions (Tags, Contexts, etc.)

// DLMS A-XDR Data Tags (from Green Book)
typedef enum {
    DLMS_TAG_NULL_DATA          = 0,
    DLMS_TAG_ARRAY              = 1,
    DLMS_TAG_STRUCTURE          = 2,
    DLMS_TAG_BOOLEAN            = 3,
    DLMS_TAG_BIT_STRING         = 4,
    DLMS_TAG_DOUBLE_LONG        = 5, // int32
    DLMS_TAG_DOUBLE_LONG_UNSIGNED = 6, // uint32
    DLMS_TAG_OCTET_STRING       = 9,
    DLMS_TAG_VISIBLE_STRING     = 10,
    DLMS_TAG_UTF8_STRING        = 12,
    DLMS_TAG_BCD                = 13,
    DLMS_TAG_INTEGER            = 15, // int8
    DLMS_TAG_LONG               = 16, // int16
    DLMS_TAG_UNSIGNED           = 17, // uint8
    DLMS_TAG_LONG_UNSIGNED      = 18, // uint16
    DLMS_TAG_COMPACT_ARRAY      = 19,
    DLMS_TAG_LONG64             = 20, // int64
    DLMS_TAG_LONG64_UNSIGNED    = 21, // uint64
    DLMS_TAG_ENUM               = 22, // uint8
    DLMS_TAG_FLOAT              = 23, // float32
    DLMS_TAG_DOUBLE_LA            = 24, // float64
    DLMS_TAG_DATE_TIME          = 25,
    DLMS_TAG_DATE               = 26,
    DLMS_TAG_TIME               = 27,
    DLMS_TAG_DATE_TIME_S        = 28, // date_time_s
    DLMS_TAG_OI                 = 29, // OBIS Object Identifier (uint16)
} edge_dlms_tag_t;


// DLMS/COSEM Context
typedef struct {
    // Current state for HDLC, etc.
} edge_dlms_context_t;

#endif // LIBEDGE_PROTOCOLS_DLMS_H
