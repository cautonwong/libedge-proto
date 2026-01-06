#include "protocols/edge_dlms.h"
#include <string.h>
#include <stdio.h>

void edge_dlms_init(edge_dlms_context_t *ctx, uint32_t client_addr, uint32_t server_addr) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(*ctx));
    edge_hdlc_init(&ctx->hdlc, client_addr, server_addr);
    ctx->state = EDGE_COSEM_STATE_IDLE;
}

void edge_dlms_reset(edge_dlms_context_t *ctx) {
    if (!ctx) return;
    ctx->state = EDGE_COSEM_STATE_IDLE;
    ctx->block_ctx.active = false;
    edge_hdlc_reset(&ctx->hdlc);
}

// --- APDU Builders ---

edge_error_t edge_dlms_build_aarq(edge_dlms_encoder_t *enc) {
    EP_ASSERT_OK(edge_vector_put_u8(enc->v, 0x60)); // AARQ Tag
    EP_ASSERT_OK(edge_dlms_encode_begin_container(enc, DLMS_TAG_STRUCTURE));
    {
        uint8_t app_ctx[] = {0x60, 0x85, 0x74, 0x05, 0x08, 0x01, 0x01};
        EP_ASSERT_OK(edge_dlms_encode_octet_string(enc, app_ctx, sizeof(app_ctx)));
        
        EP_ASSERT_OK(edge_dlms_encode_begin_container(enc, DLMS_TAG_OCTET_STRING));
        {
            EP_ASSERT_OK(edge_vector_put_u8(enc->v, 0x01)); 
            EP_ASSERT_OK(edge_vector_put_u8(enc->v, 0x00)); 
            EP_ASSERT_OK(edge_vector_put_u8(enc->v, 0x00));
            EP_ASSERT_OK(edge_vector_put_u8(enc->v, 0x06));
            uint8_t conf[] = {0x5F, 0x1F, 0x04, 0x00, 0x00, 0x7E, 0x1F, 0x04};
            EP_ASSERT_OK(edge_vector_append_copy(enc->v, conf, sizeof(conf)));
            EP_ASSERT_OK(edge_vector_put_be16(enc->v, 1024));
        }
        EP_ASSERT_OK(edge_dlms_encode_end_container(enc));
    }
    return edge_dlms_encode_end_container(enc);
}

edge_error_t edge_dlms_build_get_request(edge_dlms_encoder_t *enc, edge_dlms_service_type_t type, uint8_t invoke_id, const edge_dlms_object_t *obj) {
    EP_ASSERT_OK(edge_vector_put_u8(enc->v, (uint8_t)DLMS_APDU_GET_REQUEST));
    EP_ASSERT_OK(edge_vector_put_u8(enc->v, (uint8_t)type));
    EP_ASSERT_OK(edge_vector_put_u8(enc->v, invoke_id));
    
    if (type == DLMS_GET_NORMAL) {
        EP_ASSERT_OK(edge_vector_put_be16(enc->v, obj->class_id));
        EP_ASSERT_OK(edge_vector_append_copy(enc->v, obj->obis, 6));
        EP_ASSERT_OK(edge_vector_put_u8(enc->v, (uint8_t)obj->attribute_index));
        EP_ASSERT_OK(edge_vector_put_u8(enc->v, 0x00));
    } else if (type == DLMS_GET_NEXT) {
        EP_ASSERT_OK(edge_vector_put_be32(enc->v, 0));
    }
    return EP_OK;
}

edge_error_t edge_dlms_encode_selective_access(edge_dlms_encoder_t *enc, const uint8_t *from_date, const uint8_t *to_date) {
    EP_ASSERT_OK(edge_dlms_encode_begin_container(enc, DLMS_TAG_STRUCTURE));
    {
        EP_ASSERT_OK(edge_vector_put_u8(enc->v, 1));
        EP_ASSERT_OK(edge_dlms_encode_begin_container(enc, DLMS_TAG_STRUCTURE));
        {
            EP_ASSERT_OK(edge_dlms_encode_begin_container(enc, DLMS_TAG_STRUCTURE));
            {
                EP_ASSERT_OK(edge_vector_put_be16(enc->v, 8));
                uint8_t obis_clock[] = {0,0,1,0,0,255};
                EP_ASSERT_OK(edge_vector_append_copy(enc->v, obis_clock, 6));
                EP_ASSERT_OK(edge_vector_put_u8(enc->v, 2));
                EP_ASSERT_OK(edge_vector_put_be16(enc->v, 0)); // [修正] be16
            }
            EP_ASSERT_OK(edge_dlms_encode_end_container(enc));
            EP_ASSERT_OK(edge_dlms_encode_octet_string(enc, from_date, 12));
            EP_ASSERT_OK(edge_dlms_encode_octet_string(enc, to_date, 12));
            EP_ASSERT_OK(edge_dlms_encode_begin_container(enc, DLMS_TAG_ARRAY));
            EP_ASSERT_OK(edge_dlms_encode_end_container(enc));
        }
        EP_ASSERT_OK(edge_dlms_encode_end_container(enc));
    }
    return edge_dlms_encode_end_container(enc);
}