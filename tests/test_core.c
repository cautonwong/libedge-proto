#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include "cmocka.h"
#include "edge_core.h"

// Test vector initialization
static void test_vector_init(void **state) {
    (void) state;
    edge_vector_t v;
    struct iovec iov[10];
    edge_vector_init(&v, iov, 10);
    assert_int_equal(v.max_capacity, 10);
    assert_int_equal(v.used_count, 0);
    assert_int_equal(v.total_len, 0);
}

// Test vector append_ref (zero-copy)
static void test_vector_append_ref(void **state) {
    (void) state;
    edge_vector_t v;
    struct iovec iov[10];
    edge_vector_init(&v, iov, 10);

    uint8_t data1[] = {1, 2, 3};
    uint8_t data2[] = {4, 5};

    edge_vector_append_ref(&v, data1, sizeof(data1));
    edge_vector_append_ref(&v, data2, sizeof(data2));

    assert_int_equal(v.used_count, 2);
    assert_int_equal(v.total_len, 5);
    assert_ptr_equal(v.iovs[0].iov_base, data1);
    assert_int_equal(v.iovs[0].iov_len, 3);
    assert_ptr_equal(v.iovs[1].iov_base, data2);
    assert_int_equal(v.iovs[1].iov_len, 2);
}

// Test cursor initialization and remaining bytes
static void test_cursor_init_and_remaining(void **state) {
    (void) state;
    uint8_t data1[] = {1, 2, 3};
    uint8_t data2[] = {4, 5};
    struct iovec iov[] = {
        { .iov_base = data1, .iov_len = sizeof(data1) },
        { .iov_base = data2, .iov_len = sizeof(data2) }
    };

    edge_cursor_t c;
    edge_cursor_init(&c, iov, 2);

    assert_int_equal(edge_cursor_remaining(&c), 5);
}

// Test cursor reading across iovec boundaries
static void test_cursor_read_across_boundaries(void **state) {
    (void) state;
    uint8_t data1[] = {0xDE, 0xAD};
    uint8_t data2[] = {0xBE, 0xEF, 0xFE};
    struct iovec iov[] = {
        { .iov_base = data1, .iov_len = sizeof(data1) },
        { .iov_base = data2, .iov_len = sizeof(data2) }
    };

    edge_cursor_t c;
    edge_cursor_init(&c, iov, 2);

    uint8_t u8;
    uint16_t u16;

    // Read first byte
    assert_int_equal(edge_cursor_read_u8(&c, &u8), EDGE_OK);
    assert_int_equal(u8, 0xDE);
    assert_int_equal(edge_cursor_remaining(&c), 4);

    // Read be16 across the boundary
    assert_int_equal(edge_cursor_read_be16(&c, &u16), EDGE_OK);
    assert_int_equal(u16, 0xADBE);
    assert_int_equal(edge_cursor_remaining(&c), 2);

    // Read final two bytes
    assert_int_equal(edge_cursor_read_u8(&c, &u8), EDGE_OK);
    assert_int_equal(u8, 0xEF);
    assert_int_equal(edge_cursor_read_u8(&c, &u8), EDGE_OK);
    assert_int_equal(u8, 0xFE);
    assert_int_equal(edge_cursor_remaining(&c), 0);

    // Try to read past the end
    assert_int_equal(edge_cursor_read_u8(&c, &u8), EDGE_ERR_INCOMPLETE_DATA);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_vector_init),
        cmocka_unit_test(test_vector_append_ref),
        cmocka_unit_test(test_cursor_init_and_remaining),
        cmocka_unit_test(test_cursor_read_across_boundaries),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
