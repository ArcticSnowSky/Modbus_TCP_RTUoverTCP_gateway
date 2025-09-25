/*
 * File   : comm.c
 * Author : Thomas Mailaender
 * Date   : 2025-09-16
 *
 * Description : Implementation of common communication
 *               functions to realize a gateway between
 *               Modbus TCP ↔ Modbus RTU over TCP
 */

#include "comm.h"

////#include <stdbool.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <ws2tcpip.h>
#include <mstcpip.h>  // Für tcp_keepalive und SIO_KEEPALIVE_VALS
//#include <windows.h>

#include "main.h"
#include "cli.h"
#include "crc.h"
#include "endian.h"


#define RTU_TIMEOUT 500
#define TCP_TIMEOUT 3000

#define BUFFER_SIZE 260
#define MAX_ERR_LEN 300

#define MBAP_LEN    6
#define RTU_MIN_LEN 6
#define TCP_MIN_LEN 6
#define RTU_ERR_LEN 5



void handleSocket_TCP2RTU(SOCKET master) {
    log_sln("New Master client connected");

    BOOL optval = TRUE;
    DWORD timeout = TCP_TIMEOUT;
    if (setsockopt(master, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)))
        log_efln("Error setsockopt(master, timeout) %s", GetLastErrorString(FALSE));
    if (setSocketKeepAlive(master, optval))
        log_efln("Error setSocketKeepAlive(master) %s", GetLastErrorString(FALSE));

    SOCKET slave = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in target_addr = {
        AF_INET, htons(targetPort())
    };
    target_addr.sin_addr.s_addr = inet_addr(targetHost());

    timeout = RTU_TIMEOUT;
    if (setsockopt(slave, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)))
        log_efln("Error setsockopt(slave, timeout) %s", GetLastErrorString(FALSE));
    if (setSocketKeepAlive(slave, optval))
        log_efln("Error setSocketKeepAlive(slave) %s", GetLastErrorString(FALSE));

    if (connect(slave, (struct sockaddr*)&target_addr, sizeof(target_addr)) == 0)
    {
        uint16_t mbap_len;
        int rcv_len, snd_len;
        byte master_buffer[BUFFER_SIZE];
        byte conversion[BUFFER_SIZE];
        byte slave_buffer[BUFFER_SIZE];
        char err[MAX_ERR_LEN] = "";

        while(!isStop())
        {
            // Receive TCP from master
            rcv_len = recv_mbap(master, master_buffer, BUFFER_SIZE);
            if (rcv_len ==  0) { log_wfln("Master disconnected (%s)", GetLastErrorString(FALSE)); break; }
            if (rcv_len == -1) { /*log_wln("Master read timeout");*/ continue; }
            if (rcv_len < 0) { log_efln("%s (%s)", simpleTcpInfoStr(rcv_len, "Master"), GetLastErrorString(FALSE)); break; }

            // Strip MBAP (7 bytes)
            memcpy(conversion, master_buffer + MBAP_LEN, rcv_len -MBAP_LEN);
            uint16_t crc = crc16(conversion, rcv_len -MBAP_LEN);
            memcpy(conversion + rcv_len -MBAP_LEN, &crc, sizeof(crc));

            clear_socket_in_buffer(slave);  // RTU without length, help against desync
            
            // Send RTU to slave
            snd_len = send_all(slave, conversion, rcv_len - MBAP_LEN +2);
            if (snd_len <= 0) { log_efln("%s (%s)", simpleTcpInfoStr(snd_len, "Slave"), GetLastErrorString(FALSE)); break; }

            // Receive RTU slave
            rcv_len = recv_rtu(slave, slave_buffer, BUFFER_SIZE, expected_pdu_length(conversion[1], conversion[4]<<8|conversion[5]));
            if (rcv_len <= 0) { log_efln("%s (%s)", simpleTcpInfoStr(rcv_len, "Slave"), GetLastErrorString(FALSE)); break; }

            // Rebuild MBAP
            mbap_len = rcv_len; // CRC weg, slave ID in mbap rein
            memset(conversion, 0, sizeof(conversion));
            memcpy(conversion, master_buffer, 4);   // Transaction ID, Protocol ID
            memcpy(conversion+4, &mbap_len, 2);
            memcpy(conversion+6, slave_buffer, rcv_len -2);

            //clear_socket_in_buffer(master);

            // Send TCP slave to master
            snd_len = send_all(master, conversion, rcv_len +6 -2);
            if (snd_len <= 0) { log_efln("%s (%s)", simpleTcpInfoStr(snd_len, "Master"), GetLastErrorString(FALSE)); break; }
        }
        closesocket(slave);
    }
    closesocket(master);
}


void handleSocket_RTU2TCP(SOCKET master) {
    log_sln("New Master client connected");

    BOOL optval = TRUE;
    DWORD timeout = TCP_TIMEOUT;
    if (setsockopt(master, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)))
        log_efln("Error setsockopt(master, timeout) %s", GetLastErrorString(FALSE));
    if (setSocketKeepAlive(master, optval))
        log_efln("Error setSocketKeepAlive(master) %s", GetLastErrorString(FALSE));

    SOCKET slave = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in target_addr = {
        AF_INET, htons(targetPort())
    };
    target_addr.sin_addr.s_addr = inet_addr(targetHost());

    timeout = RTU_TIMEOUT;
    if (setsockopt(slave, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)))
        log_efln("Error setsockopt(slave, timeout) %s", GetLastErrorString(FALSE));
    if (setSocketKeepAlive(slave, optval))
        log_efln("Error setSocketKeepAlive(slave) %s", GetLastErrorString(FALSE));

    if (connect(slave, (struct sockaddr*)&target_addr, sizeof(target_addr)) == 0)
    {
        uint16_t mbap_len;
        int rcv_len, snd_len;
        byte master_buffer[BUFFER_SIZE];
        byte conversion[BUFFER_SIZE];
        byte slave_buffer[BUFFER_SIZE];
        char err[MAX_ERR_LEN] = "";

        uint16_t transactionId = 1;
        const uint16_t protocolId = 0;

        while(!isStop())
        {
            // Receive RTU from master
            rcv_len = recv_rtu(master, master_buffer, BUFFER_SIZE, -1);
            if (rcv_len ==  0) { log_wfln("Master disconnected (%s)", GetLastErrorString(FALSE)); break; }
            if (rcv_len == -1) { /*log_wln("Master read timeout");*/ continue; }
            if (rcv_len < 0) { log_efln("%s (%s)", simpleTcpInfoStr(rcv_len, "Master"), GetLastErrorString(FALSE)); break; }

            // Rebuild MBAP
            mbap_len = rcv_len -2; // CRC weg, slave ID in mbap rein
            uint16_t _transactionId = read_uint16_reverse((uint8_t*)&transactionId);
            uint16_t _mbap_len = read_uint16_reverse((uint8_t*)&mbap_len);
            memset(conversion, 0, sizeof(conversion));
            memcpy(conversion, &_transactionId, 2);
            memcpy(conversion+2, &protocolId, 2);
            memcpy(conversion+4, &_mbap_len, 2);
            memcpy(conversion+6, master_buffer, rcv_len -2);    // CRC nicht einfügen

            transactionId++;

            clear_socket_in_buffer(slave);  // RTU without length, help against desync
            
            // Send MBAP to slave
            snd_len = send_all(slave, conversion,  rcv_len +6 -2); // +MBAP - crc
            if (snd_len <= 0) { log_efln("%s (%s)", simpleTcpInfoStr(snd_len, "Slave"), GetLastErrorString(FALSE)); break; }

            // Receive MBAP slave
            rcv_len = recv_mbap(slave, slave_buffer, BUFFER_SIZE);
            if (rcv_len <= 0) { log_efln("%s (%s)", simpleTcpInfoStr(rcv_len, "Slave"), GetLastErrorString(FALSE)); break; }

            // TransactionID Check
            uint16_t rcv_transactionId = read_uint16_reverse(slave_buffer);
            if (rcv_transactionId != transactionId -1) log_efln("TransactionMismatch: rcv %u != %u snt", rcv_transactionId, transactionId-1);

            // Strip MBAP (7 bytes)
            memset(conversion, 0, sizeof(conversion));
            memcpy(conversion, slave_buffer + MBAP_LEN, rcv_len -MBAP_LEN);
            uint16_t crc = crc16(conversion, rcv_len -MBAP_LEN);
            memcpy(conversion + rcv_len -MBAP_LEN, &crc, sizeof(crc));

            //clear_socket_in_buffer(master);

            // Send RTU slave to master
            snd_len = send_all(master, conversion, rcv_len - MBAP_LEN +2);
            if (snd_len <= 0) { log_efln("%s (%s)", simpleTcpInfoStr(snd_len, "Master"), GetLastErrorString(FALSE)); break; }
        }
        closesocket(slave);
    }
    closesocket(master);
}




const char* simpleTcpInfoStr(int val, char* name) {
    static char str[200] = "";

    switch (val) {
        case enSIMPLE_TCP_disconnected: sprintf(str, "%s disconnected", name); break;
        case enSIMPLE_TCP_error_timeout: sprintf(str, "%s timeout", name); break;
        case enSIMPLE_TCP_error_tooMuchData: sprintf(str, "%s too much data received", name); break;
        case enSIMPLE_TCP_error_bufferFull: sprintf(str, "%s buffer full", name); break;
        case enSIMPLE_TCP_error_crc: sprintf(str, "%s crc error", name); break;
        case enSIMPLE_TCP_aborted: strcpy(str, "Service aborted"); break;
        default:
            if (val < 0)
                sprintf(str, "%s recv unknown Error: %4d", val);
            else
                return NULL;
    }
    return str;
}

const char* ERRNOGetLastErrorString() {
    static char str[200] = "";
    switch (errno) {
        case EAGAIN:        strcpy(str, "Resource not available"); break;
        case EWOULDBLOCK:   strcpy(str, "Call would block");    break;
        case ECONNRESET:    strcpy(str, "Connection resetted"); break;
        case ECONNABORTED:  strcpy(str, "Connection aborted");  break;
        case ETIMEDOUT:     strcpy(str, "Timed out");           break;
        case ENETDOWN:      strcpy(str, "Network down");        break;
        case ENETRESET:     strcpy(str, "Network resetted");    break;
        case NO_ERROR:      strcpy(str, "No error");            break;
        default:    sprintf(str, "Unknown Error: %d", errno);   break;
    }
    return str;
}

const char* WSAGetLastErrorString() {
    static char str[200] = "";
    switch (WSAGetLastError()) {
        case WSAECONNRESET:
            strcpy(str, "Connection resetted");
            break;
        case WSAECONNABORTED:
            strcpy(str, "Connection aborted");
            break;
        case WSAETIMEDOUT:
            strcpy(str, "Timed out");
            break;
        case WSAENETDOWN:
            strcpy(str, "Networkconnection lost");
            break;
        case WSAENOTCONN:
            strcpy(str, "Not connected");
            break;
        case NO_ERROR:
            strcpy(str, "No error");
            break;
        default:
            sprintf(str, "Unknown Error: %d", WSAGetLastError());
            break;
    }
    return str;
}

/// @brief Get combined error string of errno and WSAGetLastError
/// @param hideNoError If true and no error, return empty string
/// @return Error string
const char* GetLastErrorString(boolean hideNoError) {
    const char* errno_str = ERRNOGetLastErrorString();
    const char* wsaerr_str = WSAGetLastErrorString();

    if (strcmp(errno_str, wsaerr_str) == 0) {
        return errno == 0 && hideNoError
            ? ""
            : errno_str;
    } else {
        static char str[200] = "";
        sprintf(str, "ErrNo: %d %s | WSA: %d %s", errno, errno_str, WSAGetLastError(), wsaerr_str);
        return str;
    }
}



int recv_mbap(SOCKET client, void* buffer, size_t size) {
    uint8_t *data = buffer;
    int rcv_len = 0;
    int len;
    do {
        errno = 0;
        len = recv(client, data + rcv_len, size - rcv_len, 0);
        if (len <= 0) {
            int err = WSAGetLastError();
            //log_efln("recv_mbap recv ret: %d, errno: %d, err %d", len, errno, err);
            switch (errno) {
                case EAGAIN:
                case EWOULDBLOCK:
                    log_efln("recv_mbap timeout ret: %d, errno %d, err %d\n", len, errno, err);
                    return enSIMPLE_TCP_error_timeout;

                case ECONNRESET:
                case ECONNABORTED:
                case ETIMEDOUT:
                case ENETDOWN:
                case ENETRESET:
                    log_efln("recv_mbap disconnect ret: %d, errno %d, err %d\n", len, errno, err);
                    return enSIMPLE_TCP_disconnected;

                default:
                    switch (err) {
                        case WSAETIMEDOUT:
                            return enSIMPLE_TCP_error_timeout;

                        case WSAECONNRESET:
                        case WSAECONNABORTED:
                        case WSAENETDOWN:
                        case WSAENOTCONN:
                        default:
                            return enSIMPLE_TCP_disconnected;
                    }
            }
            return len;
        }

        rcv_len += len;

        if (rcv_len > TCP_MIN_LEN) {
            int mbap_len = read_uint16_reverse(data +4);
            if (rcv_len == mbap_len + MBAP_LEN)
                return rcv_len;                         // Success
            else if (rcv_len > mbap_len + MBAP_LEN)
                return enSIMPLE_TCP_error_tooMuchData;  // Error
        }

        if (size - rcv_len <= 0)
            return enSIMPLE_TCP_error_bufferFull;       // Error

    } while (len > 0 && !isStop());

    return enSIMPLE_TCP_aborted;                        // Aborted
}

int recv_rtu(SOCKET client, void* buffer, size_t size, int expected_pdu_len) {
    uint8_t *data = buffer;
    int rcv_len = 0;
    int len;
    do {
        errno = 0;
        len = recv(client, data + rcv_len, size - rcv_len, 0);
        if (len <= 0) {
            if (rcv_len == RTU_ERR_LEN)
                return rcv_len; // RTU Error Response
            return len;
        }

        rcv_len += len;

        if (rcv_len >= RTU_MIN_LEN) {
            if (expected_pdu_len >= 0) {
                if (rcv_len == expected_pdu_len + 3) {
                    uint16_t crc_calc = crc16(buffer, rcv_len -2);
                    uint16_t crc_rcv = data[rcv_len -1] << 8 | data[rcv_len -2];
                    if (crc_calc == crc_rcv)
                        return rcv_len;                     // Success
                    return enSIMPLE_TCP_error_crc;
                }
                else if (rcv_len > expected_pdu_len + 3)
                    return enSIMPLE_TCP_error_tooMuchData;  // Error
            } else {
                // In case of unknown expected pdu
                // future data might still be okay
                // HOWEVER: With unknown length desync is possible!
                int avail = socket_data_available(client);
                if (avail <= 0) {
                    uint16_t crc_calc = crc16(buffer, rcv_len -2);
                    uint16_t crc_rcv = data[rcv_len -1] << 8 | data[rcv_len -2];
                    if (crc_calc != crc_rcv)
                        log_efln("Crc mismatch %u != %u", crc_calc, crc_rcv);
                    return rcv_len;                     // Unknown data
                }
                // else read available data next loop
            }
        }

        int test = socket_data_available(client) <= 0;
        if (rcv_len == RTU_ERR_LEN && test <= 0)
            return rcv_len; // RTU Error Response

        if (rcv_len == 2) {
            log_efln("RTU Response error: x%x%x", data[0], data[1]);
            break;
        }

        if (size - rcv_len <= 0)
            return enSIMPLE_TCP_error_bufferFull;       // Error

    } while (len > 0 && !isStop());

    return enSIMPLE_TCP_aborted;                        // Aborted
}

size_t send_all(int sockfd, const void *buffer, size_t length) {
    size_t total_sent = 0;
    const uint8_t *ptr = buffer;

    errno = 0;
    while (total_sent < length && !isStop()) {
        size_t sent = send(sockfd, ptr + total_sent, length - total_sent, 0);
        if (sent <= 0)
            return sent; // Fehler oder Verbindung geschlossen
        total_sent += sent;
    }
    if (isStop())
        return enSIMPLE_TCP_aborted;                        // Aborted
    return total_sent;
}

/// @brief In case of unknown data to protect a bit against desync: clear input
/// @param sockfd 
/// @return amount of garbaged data
int clear_socket_in_buffer(int sockfd) {
    u_long bytes_available = -1;
    int result;
    u_long cleared_data = 0;
    uint8_t buffer[100];
    while ((result = ioctlsocket(sockfd, FIONREAD, &bytes_available)) == 0 && bytes_available > 0)
        cleared_data += recv(sockfd, buffer, sizeof(buffer), 0);
    return cleared_data;
}

/// @brief Checks whether data is available
/// @param sockfd 
/// @return -1 Error, 0 No data available, +>=1 Data available
int socket_data_available(int sockfd) {
    u_long bytes_available = -1;
    errno = 0;
    int ioctlsocket_result = ioctlsocket(sockfd, FIONREAD, &bytes_available);
    if (ioctlsocket_result == 0) {
        int ret = bytes_available;
        return ret < 0
            ? INT_MAX
            : ret;
    }
    log_efln("socket_data_available() ioctlsocket(): %d", ioctlsocket_result);
    return ioctlsocket_result;
}

/// @brief Set TCP KeepAlive on socket
/// @param sockfd 
/// @param val TRUE to enable, FALSE to disable
/// @return 
int setSocketKeepAlive(int sockfd, BOOL val) {
    struct tcp_keepalive keepAliveSettings;
    keepAliveSettings.onoff = val;                // Keepalive aktivieren
    keepAliveSettings.keepalivetime = 60000;      // 60 Sekunden Inaktivität bis erste Probe
    keepAliveSettings.keepaliveinterval = 5000;   // 5 Sekunden zwischen Probes

    errno = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (char*)&val, sizeof(val)))
        log_efln("setSocketKeepAlive: Error setsockopt(keepalive) %s", GetLastErrorString(FALSE));

    DWORD bytesReturned;
    int result = WSAIoctl(
        sockfd,
        SIO_KEEPALIVE_VALS,
        &keepAliveSettings,
        sizeof(keepAliveSettings),
        NULL,
        0,
        &bytesReturned,
        NULL,
        NULL
    );

    if (result == SOCKET_ERROR)
        log_efln("setSocketKeepAlive: Error WSAIoctl %s", GetLastErrorString(FALSE));
    return result;
}

/// @brief Calculate expected Length of PDU-response (without adress and CRC)
int expected_pdu_length(uint8_t function_code, uint16_t quantity) {
    switch (function_code) {
        case 0x01: // Read Coils
        case 0x02: // Read Discrete Inputs
            return 2 + ((quantity + 7) / 8); // Byte count + coil bytes

        case 0x03: // Read Holding Registers
        case 0x04: // Read Input Registers
            return 2 + (quantity * 2); // Byte count + register bytes

        case 0x05: // Write Single Coil
        case 0x06: // Write Single Register
            return 5; // Echo: Function + Address + Value

        case 0x0F: // Write Multiple Coils
            return 5; // Echo: Function + Start Addr + Quantity

        case 0x10: // Write Multiple Registers
            return 5; // Echo: Function + Start Addr + Quantity

        default:
            return -1; // Unbekannter oder nicht unterstützter Funktionscode
    }
}
