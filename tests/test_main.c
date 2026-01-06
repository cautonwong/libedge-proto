#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include "cmocka.h"

#include "common/crc.h"

/* A test case for the CRC16-Modbus calculation. */
static void test_crc16_modbus(void **state) {
    (void) state; /* Unused */
    
    // Test vector from https://www.modbustools.com/modbus_crc16.html
    // 01 03 00 01 00 01
    uint8_t test_data[] = {0x01, 0x03, 0x00, 0x01, 0x00, 0x01};
    uint16_t expected_crc = 0xCAD5; // CRC is transmitted LSB first, so 0xCA15
                                   // In memory, it's 0x15CA
    
    uint16_t calculated_crc = edge_crc16_modbus(test_data, sizeof(test_data));

    // cmocka assertion
    assert_int_equal(calculated_crc, expected_crc);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_crc16_modbus),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
