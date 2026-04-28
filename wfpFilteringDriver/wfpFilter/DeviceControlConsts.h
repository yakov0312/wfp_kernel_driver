#pragma once
#include "FilterIdContainer.h"

// Codes
#define IOCTL_REMOVE_FILTER CTL_CODE(FILE_DEVICE_NETWORK, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ADD_FILTER CTL_CODE(FILE_DEVICE_NETWORK, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Struct that a program send to the driver with some settings about the fitler
typedef struct _FILTER_SETTINGS
{
    wchar_t filterName[FILTER_NAME_SIZE + 1]; // +1 for \0
    UINT16 dstPort;
    UINT32 srcIpAddr;
    UINT32 dstIpAddr;
    size_t payloadSize;
    size_t swapSize;
    BOOLEAN isUDP;
    UINT8 data[];
} FILTER_SETTINGS, *PFILTER_SETTINGS;

// Context to send to the callout func
typedef struct _FILTER_CONTEXT
{
    char* payload;
    size_t payloadSize;
    char* swap;
    size_t swapSize;
} FILTER_CONTEXT, * PFILTER_CONTEXT;