#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include "cmocka.h"
#include "libedge/edge_dnp3.h"

static void test_dnp3_link_layer_segmentation(void **state) {
    (void)state;
    edge_dnp3_context_t ctx;
    edge_dnp3_init(&ctx, 0x0001, 0x0002);

    struct iovec iov[10];
    edge_vector_t v;
    edge_vector_init(&v, iov, 10);

    // 构造一个 20 字节的负载，确证它被分成了 16 + 4 两个 CRC 段
    uint8_t payload[20];
    for(int i=0; i<20; i++) payload[i] = (uint8_t)i;

    assert_int_equal(edge_dnp3_build_link_frame(&ctx, &v, 0x44, payload, 20), EDGE_OK);

    // 预期长度: Header(10) + Data1(16) + CRC1(2) + Data2(4) + CRC2(2) = 34 字节
    assert_int_equal(v.total_len, 34);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dnp3_link_layer_segmentation),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
