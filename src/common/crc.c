#include "common/crc.h"

uint16_t edge_crc16_ccitt(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        uint8_t b = data[i];
        for (int j = 0; j < 8; j++) {
            if ((crc ^ b) & 0x0001) crc = (uint16_t)((crc >> 1) ^ 0x8408);
            else crc >>= 1;
            b >>= 1;
        }
    }
    return (uint16_t)(crc ^ 0xFFFF);
}

uint16_t edge_crc16_modbus_update(uint16_t crc, const void *data_ptr, size_t length) {
    const uint8_t *data = (const uint8_t *)data_ptr;
    for (size_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) { crc >>= 1; crc ^= 0xA001; }
            else crc >>= 1;
        }
    }
    return crc;
}

uint16_t edge_crc16_dnp3(const uint8_t *data, size_t length) {
    uint16_t crc = 0x0000;
    for (size_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) crc = (uint16_t)((crc >> 1) ^ 0xA6BC);
            else crc >>= 1;
        }
    }
    return (uint16_t)(crc ^ 0xFFFF);
}
