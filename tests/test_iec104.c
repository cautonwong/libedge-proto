#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include "cmocka.h"
#include "libedge/edge_iec104.h"

static void test_build_startdt_act(void **state) {
    (void) state;

    edge_iec104_context_t ctx;
    struct iovec iov;

    edge_error_t err = edge_iec104_build_startdt_act(&ctx, &iov);
    assert_int_equal(err, EDGE_OK);

    uint8_t expected_frame[] = {0x68, 0x04, 0x07, 0x00, 0x00, 0x00};

    assert_int_equal(iov.iov_len, sizeof(expected_frame));
    assert_memory_equal(iov.iov_base, expected_frame, sizeof(expected_frame));
}

static void test_build_i_frame(void **state) {
    (void) state;

    edge_iec104_context_t ctx = {0};
    ctx.vs = 5; // Send sequence number
    ctx.vr = 3; // Receive sequence number
    struct iovec iov;

    const uint8_t asdu[] = {0x01, 0x02, 0x03, 0x04}; // A fake ASDU

    edge_error_t err = edge_iec104_build_i_frame(&ctx, asdu, sizeof(asdu), &iov);
    assert_int_equal(err, EDGE_OK);

    // N(S) = 5 -> shifted = 10 (0x000A)
    // N(R) = 3 -> shifted = 6 (0x0006)
    // Control Field: 0A 00 06 00
    uint8_t expected_frame[] = {
        0x68, 8,    // Start, Length (4+4)
        0x0A, 0x00, // Control Field 1,2 (N(S))
        0x06, 0x00, // Control Field 3,4 (N(R))
        0x01, 0x02, 0x03, 0x04 // ASDU
    };

    assert_int_equal(iov.iov_len, sizeof(expected_frame));
    assert_memory_equal(iov.iov_base, expected_frame, sizeof(expected_frame));

    // Check that the send sequence number was incremented
    assert_int_equal(ctx.vs, 6);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_build_startdt_act),
        cmocka_unit_test(test_build_i_frame),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
