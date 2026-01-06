#ifndef LIBEDGE_EDGE_COMMON_H
#define LIBEDGE_EDGE_COMMON_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Common error codes for the library.
 */
typedef enum {
    EDGE_OK = 0,
    EDGE_ERR_GENERAL = -1,
    EDGE_ERR_INVALID_ARG = -2,
    EDGE_ERR_BUFFER_TOO_SMALL = -3,
    EDGE_ERR_INVALID_FRAME = -4,
    // Add more specific error codes as needed
} edge_error_t;

/**
 * @brief Base context structure.
 * Can be included in protocol-specific contexts.
 */
typedef struct {
    void* user_data; // Pointer to user-defined parent structure
    edge_error_t last_error;
} edge_base_context_t;

/**
 * @brief Logging macros.
 * Application can redefine these to redirect log output.
 */
#ifndef EDGE_LOG_DEBUG
#include <stdio.h>
#define EDGE_LOG_DEBUG(format, ...) printf("[DEBUG] " format "\n", ##__VA_ARGS__)
#endif

#ifndef EDGE_LOG_INFO
#include <stdio.h>
#define EDGE_LOG_INFO(format, ...) printf("[INFO] " format "\n", ##__VA_ARGS__)
#endif

#ifndef EDGE_LOG_ERROR
#include <stdio.h>
#define EDGE_LOG_ERROR(format, ...) fprintf(stderr, "[ERROR] " format "\n", ##__VA_ARGS__)
#endif


#endif // LIBEDGE_EDGE_COMMON_H
