#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include "cmocka.h"
#include "edge_core.h"
#include "protocols/dlms/dlms_axdr.h" // Placeholder for testing AXDR

static void test_dlms_axdr_placeholder(void **state) {
    (void) state;
    assert_int_equal(1, 1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dlms_axdr_placeholder),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
