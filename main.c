/*
 * File   : main.c
 * Author : Thomas Mailaender
 * Date   : 2025-09-16
 *
 * Description : Implementation of a Windows Service and console programm
 *               to realize a gateway between Modbus TCP ↔ Modbus RTU over TCP
 */


#include <stdint.h>
//#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h>  // Für tcp_keepalive und SIO_KEEPALIVE_VALS
#include <windows.h>
#include <time.h>

#include "cli.h"
#include "comm.h"

#pragma comment(lib, "ws2_32.lib")

#define SERVICE_NAME "ModbusProxyService"

SERVICE_STATUS_HANDLE g_StatusHandle;
HANDLE g_StopEvent;

void WINAPI ServiceMain(DWORD, LPTSTR *);
void WINAPI ServiceCtrlHandler(DWORD);
DWORD WINAPI ProxyThread(LPVOID);
DWORD WINAPI threadHandleSocket(LPVOID lpParamSocket);

volatile boolean stop = FALSE;      // When service stopped, stop => true
SOCKET listener;
int listener_port = 1502;
char target_host[256] = "127.0.0.1";
int target_port = 502;
boolean rtu_mode = FALSE;


/// @brief For loop checks, verify if service is stopped
/// @return 
volatile boolean isStop() { return stop; }

const char* targetHost() { return target_host; }
const int targetPort() { return target_port; }

int main(int argc, char *argv[]) {
    if (argc >= 2 && (strcmp(argv[1], "rtu") != 0 && strcmp(argv[1], "tcp") != 0)) {
        log_fln("Usage: %s rtu|tcp <listen_port> <target_host> <target_port>", argv[0]);
        return 1;
    } else {
        if (argc >= 2) rtu_mode = strcmp(argv[1], "rtu") == 0;
        if (argc >= 3) listener_port = atoi(argv[2]);
        if (argc >= 4) strcpy(target_host, argv[3]);
        if (argc >= 5) target_port = atoi(argv[4]);
    }

    SERVICE_TABLE_ENTRY ServiceTable[] = {
        {SERVICE_NAME, ServiceMain},
        {NULL, NULL}
    };
    if (!StartServiceCtrlDispatcher(ServiceTable)) {
        // Fehler beim Start als Dienst → vermutlich Konsolenmodus
        DWORD err = GetLastError();
        if (err == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) {
            ProxyThread(NULL);
            return 0;
        } else {
            log_efln("StartServiceCtrlDispatcher failed: %lu", err);
            return 1;
        }
    }
    return 0;
}

void WINAPI ServiceMain(DWORD argc, LPTSTR *argv) {
    g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);
    g_StopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    SetServiceStatus(g_StatusHandle, &(SERVICE_STATUS){SERVICE_WIN32_OWN_PROCESS, SERVICE_RUNNING, 1});
    CreateThread(NULL, 0, ProxyThread, NULL, 0, NULL);
    WaitForSingleObject(g_StopEvent, INFINITE);
    SetServiceStatus(g_StatusHandle, &(SERVICE_STATUS){SERVICE_WIN32_OWN_PROCESS, SERVICE_STOPPED});
}

void WINAPI ServiceCtrlHandler(DWORD ctrlCode) {
    switch (ctrlCode) {
        case SERVICE_CONTROL_STOP:
            stop = TRUE;
            if (listener != INVALID_SOCKET) {
                closesocket(listener);
                listener = INVALID_SOCKET;
            }
            SetEvent(g_StopEvent);

            SERVICE_STATUS status;
            status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
            status.dwCurrentState = SERVICE_STOP_PENDING;
            status.dwControlsAccepted = 1;
            status.dwWin32ExitCode = 0;
            status.dwCheckPoint = 1;
            status.dwWaitHint = 1000;
            SetServiceStatus(g_StatusHandle, &status);
    }
}

DWORD WINAPI ProxyThread(LPVOID lpParam) {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    rtu_mode
        ? log_ln("RTU over TCP <-> TCP")
        : log_ln("TCP <-> RTU over TCP");

    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == INVALID_SOCKET) {
        log_efln("Socket creation failed: %s", GetLastErrorString(FALSE));
        WSACleanup();
        return 1;
    }
    struct sockaddr_in addr = {AF_INET, htons(listener_port), INADDR_ANY};
    if (bind(listener, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        log_efln("Bind failed: %s", GetLastErrorString(FALSE));
        closesocket(listener);
        WSACleanup();
        return 1;
    }
    if (listen(listener, 5) == SOCKET_ERROR) {
        log_efln("Listen failed: %s", GetLastErrorString(FALSE));;
        closesocket(listener);
        WSACleanup();
        return 1;
    }
    log_fln("Listening on port %d...", listener_port);

    while (!stop) {
        SOCKET client = accept(listener, NULL, NULL);
        if (client == INVALID_SOCKET) {
            log_efln("Accept failed: %s", GetLastErrorString(FALSE));
            closesocket(client);
        } else {
#define MULTITHREADING
#ifndef MULTITHREADING
            if (rtu_mode)
                handleSocket_RTU2TCP(client);
            else
                handleSocket_TCP2RTU(client);
#else
            SOCKET* sockPtr = malloc(sizeof(SOCKET));
            if (!sockPtr) {
                log_efln("malloc failed: %s", GetLastErrorString(FALSE));
                continue;
            }
            *sockPtr = client;
            CreateThread(NULL, 0, threadHandleSocket, sockPtr, 0, NULL);
#endif
        }
    }

    WSACleanup();
    return 0;
}

DWORD WINAPI threadHandleSocket(LPVOID lpParamSocket) {
    SOCKET sock = *(SOCKET*)lpParamSocket;
    free(lpParamSocket);
    if (rtu_mode)
        handleSocket_RTU2TCP(sock);
    else
        handleSocket_TCP2RTU(sock);
    return 0;
}
