#include "edge_core.h"
#include <string.h>

void edge_vector_init(edge_vector_t *v, struct iovec *iovs, int max_capacity) {
    if (!v || !iovs) return;
    v->iovs = iovs;
    v->max_capacity = max_capacity;
    v->used_count = 0;
    v->total_len = 0;
}

// Appends a reference to existing memory (zero-copy)
void edge_vector_append_ref(edge_vector_t *v, const void *ptr, size_t len) {
    if (!v || !ptr || len == 0 || v->used_count >= v->max_capacity) {
        return;
    }
    v->iovs[v->used_count].iov_base = (void*)ptr;
    v->iovs[v->used_count].iov_len = len;
    v->used_count++;
    v->total_len += len;
}

// Appends by copying data into a provided scratch buffer
void edge_vector_append_copy(edge_vector_t *v, void *scratch_buffer, const void *data, size_t len) {
    if (!v || !scratch_buffer || !data || len == 0 || v->used_count >= v->max_capacity) {
        return;
    }
    memcpy(scratch_buffer, data, len);
    v->iovs[v->used_count].iov_base = scratch_buffer;
    v->iovs[v->used_count].iov_len = len;
    v->used_count++;
    v->total_len += len;
}
