#ifndef LIBEDGE_EDGE_CORE_H
#define LIBEDGE_EDGE_CORE_H

#include <sys/uio.h>
#include <stdint.h>
#include <stdbool.h>

// Re-define common error codes here
typedef enum {
    EDGE_OK = 0,
    EDGE_ERR_GENERAL = -1,
    EDGE_ERR_INVALID_ARG = -2,
    EDGE_ERR_BUFFER_TOO_SMALL = -3,
    EDGE_ERR_INVALID_FRAME = -4,
    EDGE_ERR_INCOMPLETE_DATA = -5,
    EDGE_ERR_OUT_OF_BOUNDS = -6,
} edge_error_t;

// The Vector Builder (Writer)
typedef struct {
    struct iovec *iovs;       // Pointer to the iovec array provided by the caller
    int max_capacity;         // The maximum number of iovec segments allowed
    int used_count;           // The number of iovec segments currently used
    size_t total_len;         // The total byte length of all segments combined
} edge_vector_t;

// The Vector Cursor (Reader)
typedef struct {
    const struct iovec *iovs; // The array of iovec segments to read from
    int count;                // The number of segments in the iovs array
    int current_iov;          // The index of the current iovec segment being read
    size_t current_offset;    // The offset within the current iovec segment
    size_t total_read;        // Total bytes read from the start
} edge_cursor_t;


// --- Vector API ---
void edge_vector_init(edge_vector_t *v, struct iovec *iovs, int max_capacity);
void edge_vector_append_ref(edge_vector_t *v, const void *ptr, size_t len);
void edge_vector_append_copy(edge_vector_t *v, void *scratch_buffer, const void *data, size_t len);


// --- Cursor API ---
void edge_cursor_init(edge_cursor_t *c, const struct iovec *iovs, int count);
edge_error_t edge_cursor_read_u8(edge_cursor_t *c, uint8_t *val);
edge_error_t edge_cursor_read_be16(edge_cursor_t *c, uint16_t *val);
edge_error_t edge_cursor_read_bytes(edge_cursor_t *c, uint8_t *buf, size_t len);
size_t edge_cursor_remaining(const edge_cursor_t *c);


#endif // LIBEDGE_EDGE_CORE_H
