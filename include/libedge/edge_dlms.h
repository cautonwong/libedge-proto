#ifndef LIBEDGE_EDGE_DLMS_H
#define LIBEDGE_EDGE_DLMS_H

#include "edge_common.h"
#include <sys/uio.h>

#ifdef __cplusplus
extern "C" {
#endif

// xDLMS APDU Tags
typedef enum {
    DLMS_APDU_TAG_GET_REQUEST  = 192, // C0
    DLMS_APDU_TAG_GET_RESPONSE = 196, // C4
    // ... other apdu tags
} edge_dlms_apdu_tag_t;

// Get-Request APDU Choice
typedef enum {
    DLMS_GET_REQUEST_NORMAL = 1,
    DLMS_GET_REQUEST_NEXT = 2,
    DLMS_GET_REQUEST_WITH_LIST = 3,
} edge_dlms_get_request_choice_t;

typedef struct {
    edge_base_context_t base;
    uint8_t frame_buffer[256];
} edge_dlms_context_t;

/**
 * @brief Builds a Get-Request-Normal APDU.
 * 
 * @param ctx The DLMS context.
 * @param invoke_id The invoke-id-and-priority byte.
 * @param class_id The COSEM interface class ID.
 * @param obis_code A pointer to the 6-byte OBIS code.
 * @param attribute_id The COSEM attribute ID.
 * @param out_vec An iovec structure to be populated with the APDU.
 * @return edge_error_t EDGE_OK on success.
 */
edge_error_t edge_dlms_build_get_request_normal(
    edge_dlms_context_t *ctx,
    uint8_t invoke_id,
    uint16_t class_id,
    const uint8_t obis_code[6],
    int8_t attribute_id,
    struct iovec *out_vec);


#ifdef __cplusplus
}
#endif

#endif // LIBEDGE_EDGE_DLMS_H
