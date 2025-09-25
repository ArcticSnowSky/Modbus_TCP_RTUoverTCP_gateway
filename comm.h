/*
 * File   : comm.h
 * Author : Thomas Mailaender
 * Date   : 2025-09-16
 *
 * Description : Prototypes for common communication
 *               functions to realize a gateway between
 *               Modbus TCP ↔ Modbus RTU over TCP
 */

#ifndef __COMM_H__
#define __COMM_H__

#include <stdint.h>
#include <winsock2.h>


enum enSIMPLE_TCP {
    enSIMPLE_TCP_disconnected = 0,
    enSIMPLE_TCP_error_timeout = -1,
    enSIMPLE_TCP_aborted = -10,
    enSIMPLE_TCP_error_tooMuchData = -11,
    enSIMPLE_TCP_error_bufferFull = -12,
    enSIMPLE_TCP_error_crc = -13
};


/// @brief Handle new connected socket from Master as TCP,
/// @brief does the logic in a blocking mode waiting for data
/// @param master Accepted Socket (Master as TCP)
void handleSocket_TCP2RTU(SOCKET master);

/// @brief Handle new connected socket from Master as RTU,
/// @brief does the logic in a blocking mode waiting for data
/// @param master Accepted Socket (Master as RTU)
void handleSocket_RTU2TCP(SOCKET master);



/// @brief Get simpleTcpInfoStr for error code
/// @param val Returncode Error code (see enSIMPLE_TCP)
/// @param name Name to identify (e.g. "Master" or "Slave")
/// @return Error string or NULL if no error
const char* simpleTcpInfoStr(int val, char* name);

/// @brief Get errno as string
/// @return Error string
const char* ERRNOGetLastErrorString();

/// @brief Get WSAGetLastError as string
/// @return Error string
const char* WSAGetLastErrorString();

/// @brief Get combined error string of errno and WSAGetLastError
/// @param hideNoError If true and no error, return empty string
/// @return Error string
const char* GetLastErrorString(boolean hideNoError);



/// @brief Receive Modbus TCP (MBAP) packet from Master
/// @param client Socket to Master
/// @param buffer Buffer to store data
/// @param size Buffer size
/// @return >0 Length of received data, 0 disconnected, <0 error (see enSIMPLE_TCP)
int recv_mbap(SOCKET client, void* buffer, size_t size);


/// @brief Receive Modbus RTU packet from Slave
/// @param client Socket to Slave
/// @param buffer Buffer to store data
/// @param size Buffer size
/// @param expected_pdu_len Expected length of PDU (without adress and CRC), -1 if unknown
/// @return >0 Length of received data, 0 disconnected, <0 error (see enSIMPLE_TCP)
int recv_rtu(SOCKET client, void* buffer, size_t size, int expected_pdu_len);


/// @brief Send all data in buffer
/// @param sockfd Socket
/// @param buffer Data buffer
/// @param length Length of data
/// @return Number of sent bytes, 0 disconnected, <0 error (see enSIMPLE_TCP)
size_t send_all(int sockfd, const void *buffer, size_t length);



/// @brief In case of unknown data to protect a bit against desync: clear input
/// @param sockfd Socket
/// @return amount of garbaged data
int clear_socket_in_buffer(int sockfd);


/// @brief Checks whether data is available
/// @param sockfd Socket
/// @return -1 Error, 0 No data available, >=1 Data available
int socket_data_available(int sockfd);


/// @brief Set TCP KeepAlive on socket (and setting lower values)
/// @param sockfd Socket
/// @param val TRUE to enable, FALSE to disable
/// @return 0 if OK, <0 error (see enSIMPLE_TCP)
int setSocketKeepAlive(int sockfd, BOOL val);



/// @brief Calculate expected Length of PDU-response (without adress and CRC)
/// @param function_code Modbus Function code
/// @param quantity Quantity of registers (not bytes)
/// @return Expected length of PDU in bytes (without adress and CRC), -1 if unknown function code
int expected_pdu_length(uint8_t function_code, uint16_t quantity);

#endif