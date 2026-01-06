#include "edge_core.h"
#include <string.h>

void edge_vector_init(edge_vector_t *v, struct iovec *iovs, int max_capacity) {
    if (!v || !iovs) return;
    v->iovs = iovs;
    v->max_capacity = max_capacity;
    v->used_count = 0;
    v->total_len = 0;
    v->scratch_offset = 0; // Initialize scratch pool offset
}

// Appends a reference to existing memory (zero-copy)
edge_error_t edge_vector_append_ref(edge_vector_t *v, const void *ptr, size_t len) {
    if (!v || !ptr || len == 0) {
        return EDGE_ERR_INVALID_ARG;
    }
    if (v->used_count >= v->max_capacity) {
        return EDGE_ERR_BUFFER_TOO_SMALL;
    }
    v->iovs[v->used_count].iov_base = (void*)ptr;
    v->iovs[v->used_count].iov_len = len;
    v->used_count++;
    v->total_len += len;
    return EDGE_OK;
}

// Appends by copying data into an internally managed scratch buffer
edge_error_t edge_vector_append_copy(edge_vector_t *v, const void *data, size_t len) {
    if (!v || !data || len == 0) {
        return EDGE_ERR_INVALID_ARG;
    }
    if (v->used_count >= v->max_capacity) {
        return EDGE_ERR_BUFFER_TOO_SMALL; // Not enough iovec slots
    }
    if (v->scratch_offset + len > VECTOR_SCRATCH_POOL_SIZE) {
        return EDGE_ERR_BUFFER_TOO_SMALL; // Not enough space in scratch pool
    }

    // Copy data into the scratch pool
    memcpy(&v->scratch_pool[v->scratch_offset], data, len);
    
    // Add reference to the iovec
    v->iovs[v->used_count].iov_base = &v->scratch_pool[v->scratch_offset];
    v->iovs[v->used_count].iov_len = len;
    v->used_count++;
    v->total_len += len;
    v->scratch_offset += len;

    return EDGE_OK;
}
