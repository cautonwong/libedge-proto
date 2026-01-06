#include "edge_core.h"
#include <string.h>
#include <arpa/inet.h>

void edge_cursor_init(edge_cursor_t *c, const struct iovec *iovs, int count) {
    if (!c || !iovs) return;
    c->iovs = iovs; c->count = count;
    c->current_iov = 0; c->current_offset = 0; c->total_read = 0;
}

size_t edge_cursor_remaining(const edge_cursor_t *c) {
    if (!c || c->current_iov >= c->count) return 0;
    size_t rem = c->iovs[c->current_iov].iov_len - c->current_offset;
    for (int i = c->current_iov + 1; i < c->count; i++) rem += c->iovs[i].iov_len;
    return rem;
}

const void* edge_cursor_get_ptr(edge_cursor_t *c, size_t len) {
    if (!c || len == 0 || c->current_iov >= c->count) return NULL;
    size_t avail = c->iovs[c->current_iov].iov_len - c->current_offset;
    if (avail >= len) {
        const void *ptr = (const uint8_t*)c->iovs[c->current_iov].iov_base + c->current_offset;
        c->current_offset += len; c->total_read += len;
        if (c->current_offset >= c->iovs[c->current_iov].iov_len) { c->current_iov++; c->current_offset = 0; }
        return ptr;
    }
    return NULL;
}

edge_error_t edge_cursor_read_bytes(edge_cursor_t *c, uint8_t *buf, size_t len) {
    if (!c || !buf) return EP_ERR_INVALID_ARG;
    const void *ptr = edge_cursor_get_ptr(c, len);
    if (ptr) { memcpy(buf, ptr, len); return EP_OK; }
    if (edge_cursor_remaining(c) < len) return EP_ERR_INCOMPLETE_DATA;
    size_t copied = 0;
    while (copied < len) {
        size_t avail = c->iovs[c->current_iov].iov_len - c->current_offset;
        size_t to_copy = (len - copied < avail) ? (len - copied) : avail;
        memcpy(buf + copied, (const uint8_t*)c->iovs[c->current_iov].iov_base + c->current_offset, to_copy);
        c->current_offset += to_copy; copied += to_copy; c->total_read += to_copy;
        if (c->current_offset >= c->iovs[c->current_iov].iov_len) { c->current_iov++; c->current_offset = 0; }
    }
    return EP_OK;
}

edge_error_t edge_cursor_read_u8(edge_cursor_t *c, uint8_t *val) { return edge_cursor_read_bytes(c, val, 1); }
edge_error_t edge_cursor_read_be16(edge_cursor_t *c, uint16_t *val) {
    uint16_t raw; EP_ASSERT_OK(edge_cursor_read_bytes(c, (uint8_t*)&raw, 2)); *val = be16toh(raw); return EP_OK;
}
edge_error_t edge_cursor_read_le16(edge_cursor_t *c, uint16_t *val) {
    uint16_t raw; EP_ASSERT_OK(edge_cursor_read_bytes(c, (uint8_t*)&raw, 2)); *val = le16toh(raw); return EP_OK;
}
edge_error_t edge_cursor_read_be32(edge_cursor_t *c, uint32_t *val) {
    uint32_t raw; EP_ASSERT_OK(edge_cursor_read_bytes(c, (uint8_t*)&raw, 4)); *val = be32toh(raw); return EP_OK;
}
