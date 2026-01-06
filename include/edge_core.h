#ifndef LIBEDGE_CORE_H
#define LIBEDGE_CORE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/uio.h>

/* --- 1. Error Codes (Protocol Submodule Specific) --- */
typedef enum {
    EP_OK = 0,
    EP_ERR_GENERIC = -1,
    EP_ERR_INVALID_ARG = -2,
    EP_ERR_INCOMPLETE_DATA = -3,
    EP_ERR_BUFFER_TOO_SMALL = -4,
    EP_ERR_NOT_SUPPORTED = -5,
    EP_ERR_OVERFLOW = -6,
    EP_ERR_INVALID_STATE = -7,
    EP_ERR_CHECKSUM = -8,
    EP_ERR_INVALID_FRAME = -9,
    EP_ERR_OUT_OF_BOUNDS = -10
} edge_error_t;

#define EP_ASSERT_OK(expr) do { \
    edge_error_t _err = (expr); \
    if (_err != EP_OK) return _err; \
} while(0)

/* --- 2. Edge Cursor (Zero-Copy Parser) --- */
typedef struct {
    const struct iovec *iovs;
    int count;
    int current_iov;
    size_t current_offset;
    size_t total_read;
} edge_cursor_t;

void edge_cursor_init(edge_cursor_t *c, const struct iovec *iovs, int count);
size_t edge_cursor_remaining(const edge_cursor_t *c);
const void* edge_cursor_get_ptr(edge_cursor_t *c, size_t len);
edge_error_t edge_cursor_read_bytes(edge_cursor_t *c, uint8_t *buf, size_t len);
edge_error_t edge_cursor_read_u8(edge_cursor_t *c, uint8_t *val);
edge_error_t edge_cursor_read_be16(edge_cursor_t *c, uint16_t *val);
edge_error_t edge_cursor_read_le16(edge_cursor_t *c, uint16_t *val);
edge_error_t edge_cursor_read_be32(edge_cursor_t *c, uint32_t *val);

/* --- 3. Edge Vector (Zero-Copy Builder) --- */
#define EDGE_VECTOR_SCRATCH_SIZE 128

typedef struct {
    struct iovec *iovs;
    int max_capacity;
    int used_count;
    size_t total_len;
    uint8_t scratch[EDGE_VECTOR_SCRATCH_SIZE];
    size_t scratch_used;
    bool last_was_scratch;
} edge_vector_t;

void edge_vector_init(edge_vector_t *v, struct iovec *iovs, int max_capacity);
size_t edge_vector_length(const edge_vector_t *v);
const uint8_t* edge_vector_get_ptr(const edge_vector_t *v, size_t offset);
edge_error_t edge_vector_append_ref(edge_vector_t *v, const void *ptr, size_t len);
edge_error_t edge_vector_append_copy(edge_vector_t *v, const void *data, size_t len);
edge_error_t edge_vector_patch(edge_vector_t *v, size_t offset, const void *data, size_t len);
edge_error_t edge_vector_put_u8(edge_vector_t *v, uint8_t val);
edge_error_t edge_vector_put_be16(edge_vector_t *v, uint16_t val);
edge_error_t edge_vector_put_be32(edge_vector_t *v, uint32_t val);
edge_error_t edge_vector_put_le16(edge_vector_t *v, uint16_t val);
edge_error_t edge_vector_put_le32(edge_vector_t *v, uint32_t val);

#endif
