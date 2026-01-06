#include "protocols/edge_dlms.h"
#include "common/crc.h"
#include "edge_core.h"
#include <string.h>
#include <stdlib.h>

void edge_hdlc_init(edge_hdlc_manager_t *mgr, uint32_t client_addr, uint32_t server_addr) {
    if (!mgr) return;
    memset(mgr, 0, sizeof(*mgr));
    mgr->client_addr = client_addr;
    mgr->server_addr = server_addr;
    mgr->state = HDLC_STATE_DISCONNECTED;
}

void edge_hdlc_reset(edge_hdlc_manager_t *mgr) {
    if (!mgr) return;
    if (mgr->reassembly_buf) { free(mgr->reassembly_buf); mgr->reassembly_buf = NULL; }
    mgr->state = HDLC_STATE_DISCONNECTED;
    mgr->ns = 0; mgr->nr = 0;
}

static int _parse_addr(edge_cursor_t *c, uint32_t *addr) {
    uint32_t val = 0; uint8_t b; int len = 0;
    do { if (edge_cursor_read_u8(c, &b) != EDGE_OK) return -1;
         val = (val << 7) | (b >> 1); len++; } while (!(b & 0x01));
    *addr = val; return len;
}

edge_error_t edge_hdlc_build_snrm(edge_hdlc_manager_t *mgr, edge_vector_t *v) {
    EDGE_ASSERT_OK(edge_vector_put_u8(v, 0x7E));
    uint8_t h[] = {0xA0, 0x07, (uint8_t)((mgr->server_addr<<1)|1), (uint8_t)((mgr->client_addr<<1)|1), 0x83};
    uint16_t hcs = edge_crc16_ccitt(h, 5);
    EDGE_ASSERT_OK(edge_vector_append_copy(v, h, 5));
    EDGE_ASSERT_OK(edge_vector_put_le16(v, hcs));
    EDGE_ASSERT_OK(edge_vector_put_u8(v, 0x7E));
    mgr->state = HDLC_STATE_CONNECTING;
    return EDGE_OK;
}

edge_error_t edge_hdlc_build_iframe(edge_hdlc_manager_t *mgr, edge_vector_t *v, const void *apdu, size_t len, bool final) {
    EDGE_ASSERT_OK(edge_vector_put_u8(v, 0x7E));
    uint16_t f_len = (uint16_t)(len + 7); // Format(2)+Addrs(2)+Ctrl(1)+HCS(2) = 7
    EDGE_ASSERT_OK(edge_vector_put_be16(v, (uint16_t)(0xA000 | f_len)));
    EDGE_ASSERT_OK(edge_vector_put_u8(v, (uint8_t)((mgr->server_addr<<1)|1)));
    EDGE_ASSERT_OK(edge_vector_put_u8(v, (uint8_t)((mgr->client_addr<<1)|1)));
    uint8_t ctrl = (uint8_t)((mgr->nr << 5) | (mgr->ns << 1));
    if (final) ctrl |= 0x10;
    EDGE_ASSERT_OK(edge_vector_put_u8(v, ctrl));
    
    // HCS calculation (mock for now)
    EDGE_ASSERT_OK(edge_vector_put_le16(v, 0x0000));
    EDGE_ASSERT_OK(edge_vector_append_ref(v, apdu, len));
    EDGE_ASSERT_OK(edge_vector_put_le16(v, 0x0000));
    EDGE_ASSERT_OK(edge_vector_put_u8(v, 0x7E));
    if (final) mgr->ns = (uint8_t)((mgr->ns + 1) % 8);
    return EDGE_OK;
}

edge_error_t edge_hdlc_parse(edge_hdlc_manager_t *mgr, edge_cursor_t *c, uint8_t *apdu_out, size_t *apdu_len) {
    uint8_t b;
    while (edge_cursor_remaining(c) > 0) {
        EDGE_ASSERT_OK(edge_cursor_read_u8(c, &b));
        if (b == 0x7E) break;
    }
    if (edge_cursor_remaining(c) < 9) return EDGE_ERR_INCOMPLETE_DATA;
    uint16_t format; EDGE_ASSERT_OK(edge_cursor_read_be16(c, &format));
    uint32_t d, s; _parse_addr(c, &d); _parse_addr(c, &s);
    uint8_t ctrl; EDGE_ASSERT_OK(edge_cursor_read_u8(c, &ctrl));
    uint16_t hcs; EDGE_ASSERT_OK(edge_cursor_read_le16(c, &hcs));
    
    size_t total_len = format & 0x07FF;
    // [高质量公式]：Payload = TotalLen - (Addr_len + Ctrl(1) + HCS(2) + FCS(2))
    size_t p_len = total_len - 2 - 1 - 2 - 2; 
    
    if (apdu_out && apdu_len) {
        if (*apdu_len < p_len) return EDGE_ERR_BUFFER_TOO_SMALL;
        EDGE_ASSERT_OK(edge_cursor_read_bytes(c, apdu_out, p_len));
        *apdu_len = p_len;
    }
    return EDGE_OK;
}
