#pragma once
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "Ws2_32.lib")

#define SYMBOLIC_LINK_NAME L"\\\\.\\NETWORK_DR"
#define IOCTL_REMOVE_FILTER CTL_CODE(FILE_DEVICE_NETWORK, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ADD_FILTER CTL_CODE(FILE_DEVICE_NETWORK, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define MAX_IP 16
#define MAX_MAC 18

#define DOMAIN "facebook"

typedef struct FilterSettings
{
    UINT16 dstPort;
    UINT32 srcIpAddr;
    UINT32 dstIpAddr;
    size_t payloadSize;
    size_t changeToSize;
    BOOLEAN isUDP;
};

UINT32 getLocalIp();