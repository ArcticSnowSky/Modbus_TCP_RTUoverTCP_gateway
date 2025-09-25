/*
 * File   : endian.h
 * Author : Thomas Mailaender
 * Date   : 2025-09-16
 *
 * Description : Definitions for simple colorized console logging
 */

#ifndef __ENDIAN_H__
#define __ENDIAN_H__

#include <stdint.h>

/// @brief Big-Endian
uint16_t read_uint16_reverse(const uint8_t *buffer) {
    return ((uint16_t)buffer[0] << 8) | buffer[1];
}

/// @brief Big-Endian
uint32_t read_uint32_reverse(const uint8_t *buffer) {
    return ((uint32_t)buffer[0] << 24) |
           ((uint32_t)buffer[1] << 16) |
           ((uint32_t)buffer[2] << 8)  |
           buffer[3];
}

/// @brief Big-Endian
uint64_t read_uint64_reverse(const uint8_t *buffer) {
    return ((uint64_t)buffer[0] << 56) |
           ((uint64_t)buffer[1] << 48) |
           ((uint64_t)buffer[2] << 40) |
           ((uint64_t)buffer[3] << 32) |
           ((uint64_t)buffer[4] << 24) |
           ((uint64_t)buffer[5] << 16) |
           ((uint64_t)buffer[6] << 8)  |
           buffer[7];
}

#endif