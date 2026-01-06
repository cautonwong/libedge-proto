#ifndef LIBEDGE_CRC_H
#define LIBEDGE_CRC_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t edge_crc16_modbus(const uint8_t *data, size_t length);
uint16_t edge_crc16_modbus_update(uint16_t crc, const void *data_ptr, size_t length);
uint16_t edge_crc16_ccitt(const uint8_t *data, size_t length);

/**
 * @brief DNP3 专用反射 CRC-16
 */
uint16_t edge_crc16_dnp3(const uint8_t *data, size_t length);

#ifdef __cplusplus
}
#endif

#endif
