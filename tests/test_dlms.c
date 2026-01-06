#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include "cmocka.h"
#include "libedge/edge_dlms.h"

// Test building a Get-Request-Normal APDU
static void test_build_get_request_normal(void **state) {
    (void) state;

    edge_dlms_context_t ctx;
    struct iovec iov;

    uint8_t invoke_id = 0xC1; // Confirmed, high-priority
    uint16_t class_id = 8; // Clock object
    const uint8_t obis[] = {0, 0, 1, 0, 0, 255}; // 0.0.1.0.0.255
    int8_t attr_id = 2; // time attribute

    edge_error_t err = edge_dlms_build_get_request_normal(&ctx, invoke_id, class_id, obis, attr_id, &iov);
    assert_int_equal(err, EDGE_OK);

    // Expected APDU:
    uint8_t expected_apdu[] = {
        0xC0, // APDU Tag: get-request
        0x01, // Choice: get-request-normal
        0xC1, // Invoke ID
        // Cosem-Attribute-Descriptor
        0x02, // tag: structure
        0x03, // 3 components
            // Class ID
            0x12, // tag: long-unsigned
            0x00, 0x08, // value: 8
            // Instance ID (OBIS)
            0x09, // tag: octet-string
            0x06, // length: 6
            0x00, 0x00, 0x01, 0x00, 0x00, 0xFF, // value
            // Attribute ID
            0x0F, // tag: integer
            0x02, // value: 2
    };

    assert_int_equal(iov.iov_len, sizeof(expected_apdu));
    assert_memory_equal(iov.iov_base, expected_apdu, sizeof(expected_apdu));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_build_get_request_normal),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}