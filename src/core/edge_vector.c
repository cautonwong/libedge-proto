#include "edge_core.h"
#include <string.h>
#include <arpa/inet.h>

void edge_vector_init(edge_vector_t *v, struct iovec *iovs, int max_capacity) {
    if (!v || !iovs) return;
    v->iovs = iovs; v->max_capacity = max_capacity;
    v->used_count = 0; v->total_len = 0; v->scratch_used = 0;
    v->last_was_scratch = false;
}

size_t edge_vector_length(const edge_vector_t *v) { return v ? v->total_len : 0; }

const uint8_t* edge_vector_get_ptr(const edge_vector_t *v, size_t offset) {
    if (!v || offset >= v->total_len) return NULL;
    size_t current = 0;
    for (int i = 0; i < v->used_count; i++) {
        if (offset < current + v->iovs[i].iov_len) return (const uint8_t*)v->iovs[i].iov_base + (offset - current);
        current += v->iovs[i].iov_len;
    }
    return NULL;
}

/**
 * @brief [专家级 API] 修改已写入的数据 (用于长度回填)
 */
edge_error_t edge_vector_patch(edge_vector_t *v, size_t offset, const void *data, size_t len) {
    if (!v || offset + len > v->total_len) return EP_ERR_OUT_OF_BOUNDS;
    uint8_t *p = (uint8_t *)edge_vector_get_ptr(v, offset);
    if (p) {
        memcpy(p, data, len);
        return EP_OK;
    }
    return EP_ERR_GENERIC;
}

edge_error_t edge_vector_append_ref(edge_vector_t *v, const void *ptr, size_t len) {
    if (!v || !ptr || len == 0) return EP_ERR_INVALID_ARG;
    if (v->used_count >= v->max_capacity) return EP_ERR_BUFFER_TOO_SMALL;
    v->iovs[v->used_count].iov_base = (void *)ptr;
    v->iovs[v->used_count].iov_len = len;
    v->used_count++; v->total_len += len;
    v->last_was_scratch = false;
    return EP_OK;
}

edge_error_t edge_vector_append_copy(edge_vector_t *v, const void *data, size_t len) {
    if (!v || !data || len == 0) return EP_ERR_INVALID_ARG;
    if (v->last_was_scratch && (v->scratch_used + len <= EDGE_VECTOR_SCRATCH_SIZE)) {
        memcpy(&v->scratch[v->scratch_used], data, len);
        v->iovs[v->used_count - 1].iov_len += len;
        v->scratch_used += len; v->total_len += len;
        return EP_OK;
    }
    if (v->used_count >= v->max_capacity || v->scratch_used + len > EDGE_VECTOR_SCRATCH_SIZE) 
        return EP_ERR_BUFFER_TOO_SMALL;
    void *dest = &v->scratch[v->scratch_used];
    memcpy(dest, data, len);
    v->iovs[v->used_count].iov_base = dest;
    v->iovs[v->used_count].iov_len = len;
    v->used_count++; v->scratch_used += len; v->total_len += len;
    v->last_was_scratch = true;
    return EP_OK;
}

edge_error_t edge_vector_put_u8(edge_vector_t *v, uint8_t val) { return edge_vector_append_copy(v, &val, 1); }
edge_error_t edge_vector_put_be16(edge_vector_t *v, uint16_t val) { uint16_t be = htobe16(val); return edge_vector_append_copy(v, &be, 2); }
edge_error_t edge_vector_put_be32(edge_vector_t *v, uint32_t val) { uint32_t be = htobe32(val); return edge_vector_append_copy(v, &be, 4); }
edge_error_t edge_vector_put_le16(edge_vector_t *v, uint16_t val) { uint16_t le = htole16(val); return edge_vector_append_copy(v, &le, 2); }
edge_error_t edge_vector_put_le32(edge_vector_t *v, uint32_t val) { uint32_t le = htole32(val); return edge_vector_append_copy(v, &le, 4); }