#include "protocols/edge_dlms.h"
#include <string.h>
#include <stdio.h>

/**
 * @brief DLMS 从站 PDU 分发引擎
 */
edge_error_t edge_dlms_server_dispatch(edge_dlms_context_t *ctx, edge_cursor_t *req, edge_vector_t *resp) {
    if (!ctx || !req || !resp) return EDGE_ERR_INVALID_ARG;

    uint8_t service_tag;
    EDGE_ASSERT_OK(edge_cursor_read_u8(req, &service_tag));

    if (service_tag == (uint8_t)DLMS_APDU_GET_REQUEST) {
        uint8_t type;
        EDGE_ASSERT_OK(edge_cursor_read_u8(req, &type)); 
        uint8_t invoke_id;
        EDGE_ASSERT_OK(edge_cursor_read_u8(req, &invoke_id));

        if (type == 1) { // GET-Normal
            uint16_t class_id;
            uint8_t obis[6];
            uint8_t attr_index;
            
            EDGE_ASSERT_OK(edge_cursor_read_be16(req, &class_id));
            EDGE_ASSERT_OK(edge_cursor_read_bytes(req, obis, 6));
            EDGE_ASSERT_OK(edge_cursor_read_u8(req, &attr_index));

            for (size_t i = 0; i < ctx->resource_count; i++) {
                const edge_dlms_resource_t *res = &ctx->resources[i];
                if (res->obj.class_id == class_id && memcmp(res->obj.obis, obis, 6) == 0) {
                    edge_dlms_variant_t val = {0};
                    edge_error_t err = res->on_get(&res->obj, &val, res->user_data);
                    
                    edge_vector_put_u8(resp, (uint8_t)DLMS_APDU_GET_RESPONSE);
                    edge_vector_put_u8(resp, 0x01); 
                    edge_vector_put_u8(resp, invoke_id);
                    edge_vector_put_u8(resp, (err == EDGE_OK) ? 0x00 : 0x01);
                    
                    if (err == EDGE_OK && val.data) {
                        edge_vector_put_u8(resp, (uint8_t)val.tag);
                        edge_vector_append_copy(resp, val.data, val.length);
                    }
                    return EDGE_OK;
                }
            }
            edge_vector_put_u8(resp, (uint8_t)DLMS_APDU_GET_RESPONSE);
            edge_vector_put_u8(resp, 0x01); 
            edge_vector_put_u8(resp, invoke_id);
            edge_vector_put_u8(resp, 0x01); 
        }
    }

    return EDGE_ERR_NOT_SUPPORTED;
}