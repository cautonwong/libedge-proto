/*
 * Copyright © 2008-2014 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */
#include "common/crc.h"

static const uint16_t modbus_crc16_table[256] = {

    // ... (table content as before) ...

};



uint16_t edge_crc16_modbus_update(uint16_t crc, const void *data_ptr, size_t length)
{
    const uint8_t *data = (const uint8_t *)data_ptr;
    for (size_t i = 0; i < length; i++) {
        uint8_t byte = data[i];
        crc = (crc >> 8) ^ modbus_crc16_table[(crc ^ byte) & 0xFF];
    }
    return crc;
}



uint16_t edge_crc16_modbus(const uint8_t *data, size_t length)

{

    return edge_crc16_modbus_update(0xFFFF, data, length);

}


