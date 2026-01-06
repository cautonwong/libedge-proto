#ifndef LIBEDGE_EDGE_DLT645_H
#define LIBEDGE_EDGE_DLT645_H

#include "edge_common.h"
#include <sys/uio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DLT645_FRAME_START 0x68
#define DLT645_FRAME_END 0x16
#define DLT645_ADDR_LEN 6

typedef enum {
    DLT645_CTRL_READ_DATA = 0x11,
    // Other control codes to be added
} edge_dlt645_ctrl_code_t;

typedef struct {
    edge_base_context_t base;
    uint8_t frame_buffer[256];
} edge_dlt645_context_t;

// Callback for a successful "Read Data" response
typedef void (*on_dlt645_read_response_t)(
    edge_dlt645_context_t *ctx,
    const uint8_t address[DLT645_ADDR_LEN],
    const uint8_t data_id[4],
    const uint8_t *data,
    size_t data_len);

// TODO: Add other callbacks for write responses, error responses etc.
typedef struct {
    on_dlt645_read_response_t on_read_response;
} edge_dlt645_callbacks_t;

/**
 * @brief Parses a received DL/T 645 frame.
 * 
 * @param ctx The DLT645 context.
 * @param frame_data Buffer containing the received frame.
 * @param frame_len Length of the received frame.
 * @param callbacks Struct of callbacks to be invoked on successful parse.
 * @return edge_error_t EDGE_OK on success, or an error code.
 */
edge_error_t edge_dlt645_parse_frame(
    edge_dlt645_context_t *ctx,
    const uint8_t *frame_data,
    size_t frame_len,
    const edge_dlt645_callbacks_t *callbacks);

/**
 * @brief Build a "Read Data" request frame for DL/T 645-2007.
 * 
 * @param ctx The DLT645 context.
 * @param address The 6-byte BCD encoded slave address.
 * @param data_id The 4-byte data identifier.
 * @param out_vec An iovec structure to be populated with the request frame.
 * @return edge_error_t EDGE_OK on success, or an error code.
 */
edge_error_t edge_dlt645_build_read_req(
    edge_dlt645_context_t *ctx,
    const uint8_t address[DLT645_ADDR_LEN],
    const uint8_t data_id[4],
    struct iovec *out_vec
);


#ifdef __cplusplus
}
#endif

#endif // LIBEDGE_EDGE_DLT645_H
