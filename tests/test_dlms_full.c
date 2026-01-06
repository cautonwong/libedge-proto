#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include "cmocka.h"
#include "protocols/edge_dlms.h"

/**
 * @brief 验证专家级 AARQ 与 HLS 握手封装
 */
static void test_dlms_expert_full_chain(void **state) {
    (void)state;
    edge_vector_t v;
    struct iovec iov[16];
    edge_vector_init(&v, iov, 16);
    
    edge_dlms_encoder_t enc;
    edge_dlms_encoder_init(&enc, &v);
    
    // 1. 构建业务请求: GET Normal
    edge_dlms_object_t voltage = { .class_id = 3, .obis = {1,0,32,7,0,255}, .attribute_index = 2 };
    assert_int_equal(edge_dlms_build_get_request(&enc, DLMS_GET_NORMAL, 0x01, &voltage), EP_OK);
    
    // 2. 应用安全层 (Suite 0: Auth & Encrypt)
    edge_dlms_security_ctx_t sec = { .policy = EDGE_DLMS_SEC_AUTH_ENCRYPTED, .invocation_counter = 100 };
    memcpy(sec.system_title, "EDGEOS01", 8);
    assert_int_equal(edge_dlms_encrypt_apdu(&sec, &v, 0x30), EP_OK);
    
    // 3. 验证结果
    assert_true(v.total_len > 20);
    assert_int_equal(sec.invocation_counter, 101);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dlms_expert_full_chain),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
