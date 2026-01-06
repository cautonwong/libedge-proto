#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include "cmocka.h"
#include "libedge/edge_dlt698.h"

// Test parsing a simple 'unsigned' data type
static void test_dlt698_parse_data_unsigned(void **state) {
    (void) state;
    
    // Encoded frame: tag(0x11) value(0xFE)
    const uint8_t frame[] = {0x11, 0xFE};
    edge_dlt698_data_t data;
    size_t consumed = 0;

    edge_error_t err = edge_dlt698_parse_data(frame, sizeof(frame), &data, &consumed);

    assert_int_equal(err, EDGE_OK);
    assert_int_equal(consumed, 2);
    assert_int_equal(data.tag, DLT698_DATA_UNSIGNED);
    assert_int_equal(data.value.u8_val, 254);
}

// Test parsing a simple 'integer' data type
static void test_dlt698_parse_data_integer(void **state) {
    (void) state;
    const uint8_t frame[] = {0x0F, 0x81}; // tag(integer), value(-127)
    edge_dlt698_data_t data;
    size_t consumed = 0;
    edge_error_t err = edge_dlt698_parse_data(frame, sizeof(frame), &data, &consumed);
    assert_int_equal(err, EDGE_OK);
    assert_int_equal(consumed, 2);
    assert_int_equal(data.tag, DLT698_DATA_INTEGER);
    assert_int_equal(data.value.i8_val, -127);
}

// Test parsing a 'long-unsigned' data type
static void test_dlt698_parse_data_long_unsigned(void **state) {
    (void) state;
    const uint8_t frame[] = {0x12, 0xFE, 0xDC}; // tag(long-unsigned), value(65244)
    edge_dlt698_data_t data;
    size_t consumed = 0;
    edge_error_t err = edge_dlt698_parse_data(frame, sizeof(frame), &data, &consumed);
    assert_int_equal(err, EDGE_OK);
    assert_int_equal(consumed, 3);
    assert_int_equal(data.tag, DLT698_DATA_LONG_UNSIGNED);
    assert_int_equal(data.value.u16_val, 65244);
}

// Test parsing a 'long' data type
static void test_dlt698_parse_data_long(void **state) {
    (void) state;
    const uint8_t frame[] = {0x10, 0x80, 0x01}; // tag(long), value(-32767)
    edge_dlt698_data_t data;
    size_t consumed = 0;
    edge_error_t err = edge_dlt698_parse_data(frame, sizeof(frame), &data, &consumed);
    assert_int_equal(err, EDGE_OK);
    assert_int_equal(consumed, 3);
    assert_int_equal(data.tag, DLT698_DATA_LONG);
    assert_int_equal(data.value.i16_val, -32767);
}

// Test parsing an 'octet-string' data type
static void test_dlt698_parse_data_octet_string(void **state) {
    (void) state;
    
    // Encoded frame: tag(0x09), len(0x04), value("test")
    const uint8_t frame[] = {0x09, 0x04, 't', 'e', 's', 't'};
    edge_dlt698_data_t data;
    size_t consumed = 0;

    edge_error_t err = edge_dlt698_parse_data(frame, sizeof(frame), &data, &consumed);

    assert_int_equal(err, EDGE_OK);
    assert_int_equal(consumed, 6);
    assert_int_equal(data.tag, DLT698_DATA_OCTET_STRING);
    assert_int_equal(data.value.string_val.len, 4);
    assert_memory_equal(data.value.string_val.ptr, "test", 4);
}

// Test parsing a 'structure' data type containing other elements
static void test_dlt698_parse_data_structure(void **state) {
    (void) state;
    
    // Encoded frame: tag(structure), count(2), [unsigned(10)], [integer(5)]
    const uint8_t frame[] = {0x02, 0x02, 0x11, 0x0A, 0x0F, 0x05};
    edge_dlt698_data_t data;
    size_t consumed = 0;
    const uint8_t *cursor = frame;
    size_t remaining_len = sizeof(frame);

    // 1. Parse the structure container
    edge_error_t err = edge_dlt698_parse_data(cursor, remaining_len, &data, &consumed);
    assert_int_equal(err, EDGE_OK);
    assert_int_equal(consumed, 2); // 1 for tag, 1 for count
    assert_int_equal(data.tag, DLT698_DATA_STRUCTURE);
    assert_int_equal(data.value.num_elements, 2);
    size_t num_elements = data.value.num_elements;

    // Advance cursor
    cursor += consumed;
    remaining_len -= consumed;

    assert_int_equal(num_elements, 2);

    // 2. Parse the first element (unsigned)
    err = edge_dlt698_parse_data(cursor, remaining_len, &data, &consumed);
    assert_int_equal(err, EDGE_OK);
    assert_int_equal(consumed, 2);
    assert_int_equal(data.tag, DLT698_DATA_UNSIGNED);
    assert_int_equal(data.value.u8_val, 10);

    // Advance cursor
    cursor += consumed;
    remaining_len -= consumed;

    // 3. Parse the second element (integer)
    err = edge_dlt698_parse_data(cursor, remaining_len, &data, &consumed);
    assert_int_equal(err, EDGE_OK);
    assert_int_equal(consumed, 2);
    assert_int_equal(data.tag, DLT698_DATA_INTEGER);
    assert_int_equal(data.value.i8_val, 5);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dlt698_parse_data_unsigned),
        cmocka_unit_test(test_dlt698_parse_data_integer),
        cmocka_unit_test(test_dlt698_parse_data_long_unsigned),
        cmocka_unit_test(test_dlt698_parse_data_long),
        cmocka_unit_test(test_dlt698_parse_data_octet_string),
        cmocka_unit_test(test_dlt698_parse_data_structure),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
