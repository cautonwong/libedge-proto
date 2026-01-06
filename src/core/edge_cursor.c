#include "edge_core.h"
#include <string.h>

void edge_cursor_init(edge_cursor_t *c, const struct iovec *iovs, int count) {
    if (!c || !iovs) return;
    c->iovs = iovs;
    c->count = count;
    c->current_iov = 0;
    c->current_offset = 0;
    c->total_read = 0;
}

size_t edge_cursor_remaining(const edge_cursor_t *c) {
    if (!c || c->current_iov >= c->count) {
        return 0;
    }
    size_t remaining = 0;
    // Remaining in current iov
    remaining += c->iovs[c->current_iov].iov_len - c->current_offset;
    // Remaining in subsequent iovs
    for (int i = c->current_iov + 1; i < c->count; i++) {
        remaining += c->iovs[i].iov_len;
    }
    return remaining;
}

edge_error_t edge_cursor_read_bytes(edge_cursor_t *c, uint8_t *buf, size_t len) {
    if (!c || !buf) return EDGE_ERR_INVALID_ARG;
    if (edge_cursor_remaining(c) < len) return EDGE_ERR_INCOMPLETE_DATA;

    size_t bytes_copied = 0;
    while (bytes_copied < len) {
        if (c->current_iov >= c->count) return EDGE_ERR_INCOMPLETE_DATA; // Should not happen due to check above

        size_t chunk_len = c->iovs[c->current_iov].iov_len - c->current_offset;
        size_t to_copy = len - bytes_copied;
        if (to_copy > chunk_len) {
            to_copy = chunk_len;
        }

        memcpy(buf + bytes_copied, (uint8_t*)c->iovs[c->current_iov].iov_base + c->current_offset, to_copy);
        
        c->current_offset += to_copy;
        bytes_copied += to_copy;
        c->total_read += to_copy;

        // Move to the next iovec if the current one is finished
        if (c->current_offset >= c->iovs[c->current_iov].iov_len) {
            c->current_iov++;
            c->current_offset = 0;
        }
    }
    return EDGE_OK;
}

edge_error_t edge_cursor_read_u8(edge_cursor_t *c, uint8_t *val) {
    return edge_cursor_read_bytes(c, val, 1);
}

edge_error_t edge_cursor_read_be16(edge_cursor_t *c, uint16_t *val) {
    uint8_t buf[2];
    edge_error_t err = edge_cursor_read_bytes(c, buf, 2);
    if (err != EDGE_OK) return err;
    *val = (uint16_t)(((uint16_t)buf[0] << 8) | buf[1]);
    return EDGE_OK;
}
