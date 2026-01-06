#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include "cmocka.h"
#include "protocols/edge_dlms.h"

static void test_dlms_axdr_expert_nesting(void **state) {
    (void)state;
    struct iovec iov[16]; edge_vector_t v; edge_vector_init(&v, iov, 16);
    edge_dlms_encoder_t enc; edge_dlms_encoder_init(&enc, &v);

    edge_dlms_encode_begin_container(&enc, DLMS_TAG_ARRAY);
    edge_dlms_encode_set_container_len(&enc, 1);
    {
        edge_dlms_encode_begin_container(&enc, DLMS_TAG_STRUCTURE);
        edge_dlms_encode_set_container_len(&enc, 1);
        {
            edge_dlms_encode_u16(&enc, 0x1234);
        }
        edge_dlms_encode_end_container(&enc);
    }
    edge_dlms_encode_end_container(&enc);

    edge_cursor_t c; edge_cursor_init(&c, v.iovs, v.used_count);
    edge_dlms_variant_t var;
    assert_int_equal(edge_dlms_decode_variant(&c, &var), EDGE_OK);
    assert_int_equal(var.tag, DLMS_TAG_ARRAY);
    assert_int_equal(var.length, 1);
}

// [新增专家级测试]：验证从站分发逻辑
static edge_error_t mock_get_handler(const edge_dlms_object_t *obj, edge_dlms_variant_t *val, void *user) {
    (void)obj; (void)val; (void)user;
    return EDGE_OK;
}

static void test_dlms_server_dispatch_basic(void **state) {
    (void)state;
    edge_dlms_context_t ctx = {0};
    edge_dlms_resource_t res = {
        .obj = { .class_id = 3, .obis = {1,0,1,8,0,255}, .attribute_index = 2 },
        .on_get = mock_get_handler
    };
    ctx.resources = &res;
    ctx.resource_count = 1;

    // 构造请求 PDU: [GET-Request] [Normal] [InvokeID] [Class:3] [OBIS] [Attr:2]
    uint8_t req_raw[] = { 192, 0x01, 0x01, 0x00, 0x03, 1,0,1,8,0,255, 0x02 };
    struct iovec iov = { .iov_base = req_raw, .iov_len = sizeof(req_raw) };
    edge_cursor_t c; edge_cursor_init(&c, &iov, 1);

    struct iovec resp_iov[4]; edge_vector_t v; edge_vector_init(&v, resp_iov, 4);
    
    assert_int_equal(edge_dlms_server_dispatch(&ctx, &c, &v), EDGE_OK);
    // 验证响应头部: [GET-Response] [Normal] [InvokeID] [Result:Success]
    uint8_t *resp_data = (uint8_t*)v.iovs[0].iov_base;
    assert_int_equal(resp_data[0], 196);
    assert_int_equal(resp_data[1], 0x01);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dlms_axdr_expert_nesting),
        cmocka_unit_test(test_dlms_server_dispatch_basic),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
