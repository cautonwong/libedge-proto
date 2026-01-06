#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include "cmocka.h"
#include "libedge/edge_iec104.h"

/**
 * @brief [专家级测试] 验证 IEC104 APCI 解析与序列号自愈
 */
static void test_iec104_expert_session(void **state) {
    (void)state;
    edge_iec104_context_t ctx = {0};
    ctx.v_r = 1; // 假设之前收到了 1 帧
    
    // 构造一个 I 帧: [68] [04] [02 00] [02 00] (V(S)=1, V(R)=1)
    uint8_t raw[] = { 0x68, 0x04, 0x02, 0x00, 0x02, 0x00 };
    struct iovec iov = { .iov_base = raw, .iov_len = sizeof(raw) };
    edge_cursor_t c; edge_cursor_init(&c, &iov, 1);
    
    struct iovec resp_iov[4]; edge_vector_t resp; edge_vector_init(&resp, resp_iov, 4);
    
    // 执行会话处理
    assert_int_equal(edge_iec104_session_on_recv(&ctx, &c, &resp), EDGE_OK);
    
    // 验证接收序列号 V(R) 已滚动到 2
    assert_int_equal(ctx.v_r, 2);
}

/**
 * @brief [破坏性测试] 验证非法同步头拦截
 */
static void test_iec104_invalid_sync(void **state) {
    (void)state;
    uint8_t raw[] = { 0xFF, 0x04, 0x00, 0x00, 0x00, 0x00 }; // 错误同步头
    struct iovec iov = { .iov_base = raw, .iov_len = sizeof(raw) };
    edge_cursor_t c; edge_cursor_init(&c, &iov, 1);
    
    uint16_t c1, c2;
    assert_int_equal(edge_iec104_parse_apci(&c, &c1, &c2), EDGE_ERR_INVALID_FRAME);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_iec104_expert_session),
        cmocka_unit_test(test_iec104_invalid_sync),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}