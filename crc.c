/*
 * File   : crc.c
 * Author : Thomas Mailaender
 * Date   : 2025-09-16
 *
 * Description : Implementation for simple crc functions
 */

#include <stdint.h>

// Modbus verwendet CRC-16-IBM mit Polynom 0x8005
#define POLYNOM 0xA001  // Achtung: Bit-reversed von 0x8005
#define INITIAL_CRC 0xFFFF

uint16_t crc16(const uint8_t *data, uint16_t length) {
    uint16_t crc = INITIAL_CRC;

    for (uint16_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ POLYNOM;
            else
                crc >>= 1;
        }
    }

    return crc;
}