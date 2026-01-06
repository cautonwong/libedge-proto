#ifndef LIBEDGE_CRC_H
#define LIBEDGE_CRC_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Calculates CRC-16/MODBUS.
 *
 * @param data Pointer to the data buffer.
 * @param length Length of the data in bytes.
 * @return The 16-bit CRC value.
 */
uint16_t edge_crc16_modbus(const uint8_t *data, size_t length);


#ifdef __cplusplus
}
#endif

#endif // LIBEDGE_CRC_H
