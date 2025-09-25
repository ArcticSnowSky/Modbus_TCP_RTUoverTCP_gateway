/*
 * File   : main.h
 * Author : Thomas Mailaender
 * Date   : 2025-09-16
 *
 * Description : Implementation of a Windows Service and console programm
 *               to realize a gateway between Modbus TCP ↔ Modbus RTU over TCP
 */

#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdint.h>

/// @brief For loop checks, verify if service is stopped
/// @return 
volatile boolean isStop();
const char* targetHost();
const int targetPort();

#endif