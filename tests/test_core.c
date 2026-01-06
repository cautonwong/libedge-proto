#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include "cmocka.h"
#include "edge_core.h"

static void test_vector_scratch_overflow(void **state) {
    (void)state;
    struct iovec iov[4]; edge_vector_t v; edge_vector_init(&v, iov, 4);
    
    uint8_t large[100]; memset(large, 0xAA, 100);
    // 第一次 Copy 消耗 100 字节 (Scratchpad 剩余 28)
    assert_int_equal(edge_vector_append_copy(&v, large, 100), EDGE_OK);
    // 第二次 Copy 50 字节，应当报错由于 Scratchpad 溢出
    assert_int_equal(edge_vector_append_copy(&v, large, 50), EDGE_ERR_BUFFER_TOO_SMALL);
    assert_int_equal(v.used_count, 1);
}

static void test_cursor_fragmented_read(void **state) {
    (void)state;
    uint8_t d1 = 0x12, d2 = 0x34;
    struct iovec iov[] = { {&d1, 1}, {&d2, 1} };
    edge_cursor_t c; edge_cursor_init(&c, iov, 2);
    
    uint16_t val;
    // 跨越 1 字节的 iovec 边界读取 2 字节
    assert_int_equal(edge_cursor_read_be16(&c, &val), EDGE_OK);
    assert_int_equal(val, 0x1234);
}

static void test_cursor_empty_iovec(void **state) {
    (void)state;
    uint8_t data = 0xFF;
    struct iovec iov[] = { {NULL, 0}, {&data, 1}, {NULL, 0} };
    edge_cursor_t c; edge_cursor_init(&c, iov, 3);
    
    uint8_t val;
    // 应当能跳过空节点读取到 0xFF
    assert_int_equal(edge_cursor_read_u8(&c, &val), EDGE_OK);
    assert_int_equal(val, 0xFF);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_vector_scratch_overflow),
        cmocka_unit_test(test_cursor_fragmented_read),
        cmocka_unit_test(test_cursor_empty_iovec),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}