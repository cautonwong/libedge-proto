#ifndef LIBEDGE_PROTOCOLS_DLMS_H
#define LIBEDGE_PROTOCOLS_DLMS_H

#include "edge_core.h"

// --- 1. Enums ---
typedef enum {
    DLMS_TAG_NULL_DATA          = 0,
    DLMS_TAG_ARRAY              = 1,
    DLMS_TAG_STRUCTURE          = 2,
    DLMS_TAG_BOOLEAN            = 3,
    DLMS_TAG_BIT_STRING         = 4,
    DLMS_TAG_DOUBLE_LONG        = 5,
    DLMS_TAG_DOUBLE_LONG_UNSIGNED = 6,
    DLMS_TAG_OCTET_STRING       = 9,
    DLMS_TAG_VISIBLE_STRING     = 10,
    DLMS_TAG_UTF8_STRING        = 12,
    DLMS_TAG_INTEGER            = 15,
    DLMS_TAG_LONG               = 16,
    DLMS_TAG_UNSIGNED           = 17,
    DLMS_TAG_LONG_UNSIGNED      = 18,
    DLMS_TAG_LONG64             = 20,
    DLMS_TAG_LONG64_UNSIGNED    = 21,
    DLMS_TAG_ENUM               = 22,
    DLMS_TAG_FLOAT              = 23,
    DLMS_TAG_DOUBLE_LA          = 24,
    DLMS_TAG_DATE_TIME          = 25,
} edge_dlms_tag_t;

typedef enum {
    DLMS_APDU_INITIATE_REQUEST      = 1,
    DLMS_APDU_INITIATE_RESPONSE     = 8,
    DLMS_APDU_GET_REQUEST           = 192,
    DLMS_APDU_GET_RESPONSE          = 196,
} edge_dlms_apdu_tag_t;

#define DLMS_SERVICE_AARQ 0x60
#define DLMS_SERVICE_AARE 0x61
#define DLMS_SERVICE_GET_REQUEST 192
#define DLMS_SERVICE_GET_RESPONSE 196

typedef enum {
    DLMS_GET_NORMAL                 = 1,
    DLMS_GET_NEXT                   = 2,
} edge_dlms_service_type_t;

typedef enum {
    EDGE_DLMS_SEC_NONE              = 0,
    EDGE_DLMS_SEC_AUTHENTICATED     = 1,
    EDGE_DLMS_SEC_ENCRYPTED         = 2,
    EDGE_DLMS_SEC_AUTH_ENCRYPTED    = 3
} edge_dlms_security_policy_t;

// --- 2. Structures (Core Metadata) ---

typedef struct {
    uint8_t system_title[8];
    uint32_t invocation_counter;
    edge_dlms_security_policy_t policy;
} edge_dlms_security_ctx_t;

typedef struct {
    edge_vector_t *v;
    int depth;
    size_t stack_offsets[16];
    edge_dlms_tag_t stack_tags[16];
} edge_dlms_encoder_t;

typedef struct {
    edge_dlms_tag_t tag;
    size_t length;
    const uint8_t *data;
} edge_dlms_variant_t;

typedef struct {
    uint16_t class_id;
    uint8_t  obis[6];
    int8_t   attribute_index;
} edge_dlms_object_t;

typedef enum {
    HDLC_STATE_DISCONNECTED,
    HDLC_STATE_CONNECTING,
    HDLC_STATE_CONNECTED,
} edge_hdlc_state_t;

typedef struct {
    edge_hdlc_state_t state;
    uint32_t server_addr;
    uint32_t client_addr;
    uint8_t ns;
    uint8_t nr;
    uint8_t *reassembly_buf;
    size_t reassembly_size;
} edge_hdlc_manager_t;

typedef enum {
    EDGE_COSEM_STATE_IDLE,
    EDGE_COSEM_STATE_ASSOCIATING,
    EDGE_COSEM_STATE_ASSOCIATED,
} edge_cosem_state_t;

typedef edge_error_t (*dlms_object_access_fn)(const edge_dlms_object_t *obj, edge_dlms_variant_t *val, void *user_data);

typedef struct {
    edge_dlms_object_t obj;
    dlms_object_access_fn on_get;
    dlms_object_access_fn on_set;
    void *user_data;
} edge_dlms_resource_t;

typedef struct {
    edge_hdlc_manager_t hdlc;
    edge_cosem_state_t state;
    uint16_t max_pdu_send;
    struct { bool active; uint32_t current_block; } block_ctx;
    const edge_dlms_resource_t *resources;
    size_t resource_count;
} edge_dlms_context_t;

// --- 3. APIs ---
edge_error_t edge_dlms_encrypt_apdu(edge_dlms_security_ctx_t *ctx, edge_vector_t *v, uint8_t security_control);
void edge_dlms_encoder_init(edge_dlms_encoder_t *enc, edge_vector_t *v);
edge_error_t edge_dlms_encode_begin_container(edge_dlms_encoder_t *enc, edge_dlms_tag_t tag);
edge_error_t edge_dlms_encode_set_container_len(edge_dlms_encoder_t *enc, size_t count);
edge_error_t edge_dlms_encode_end_container(edge_dlms_encoder_t *enc);
edge_error_t edge_dlms_encode_u8(edge_dlms_encoder_t *enc, uint8_t val);
edge_error_t edge_dlms_encode_u16(edge_dlms_encoder_t *enc, uint16_t val);
edge_error_t edge_dlms_encode_u32(edge_dlms_encoder_t *enc, uint32_t val);
edge_error_t edge_dlms_encode_octet_string(edge_dlms_encoder_t *enc, const uint8_t *data, size_t len);
edge_error_t edge_dlms_decode_variant(edge_cursor_t *c, edge_dlms_variant_t *out);

void edge_hdlc_init(edge_hdlc_manager_t *mgr, uint32_t client_addr, uint32_t server_addr);
void edge_hdlc_reset(edge_hdlc_manager_t *mgr);
edge_error_t edge_hdlc_build_snrm(edge_hdlc_manager_t *mgr, edge_vector_t *v);
edge_error_t edge_hdlc_build_iframe(edge_hdlc_manager_t *mgr, edge_vector_t *v, const void *apdu, size_t len, bool final);
edge_error_t edge_hdlc_parse(edge_hdlc_manager_t *mgr, edge_cursor_t *c, uint8_t *apdu_out, size_t *apdu_len);

edge_error_t edge_dlms_build_aarq(edge_dlms_encoder_t *enc);
edge_error_t edge_dlms_build_get_request(edge_dlms_encoder_t *enc, edge_dlms_service_type_t type, uint8_t invoke_id, const edge_dlms_object_t *obj);

edge_error_t edge_dlms_server_dispatch(edge_dlms_context_t *ctx, edge_cursor_t *req, edge_vector_t *resp);

#endif
