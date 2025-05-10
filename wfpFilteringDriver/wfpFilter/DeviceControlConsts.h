#pragma once
#include "FilterIdContainer.h"

//codes:
#define IOCTL_REMOVE_FILTER CTL_CODE(FILE_DEVICE_NETWORK, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ADD_FILTER CTL_CODE(FILE_DEVICE_NETWORK, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

//struct that a program send to the driver with some settings about the fitler
typedef struct _FILTER_SETTINGS
{
    wchar_t filterName[FILTER_NAME_SIZE + 1]; // +1 for \0
    UINT16 dstPort;
    UINT32 srcIpAddr;
    UINT32 dstIpAddr;
    size_t payloadSize;
    size_t changeToSize;
    BOOLEAN isUDP;
} FILTER_SETTINGS, * PFILTER_SETTINGS;

//context to send to the callout func
typedef struct _FILTER_CONTEXT
{
    char* payload;
    size_t lengthOfData;
    char* changeTo;
    size_t lengthOfChange;
} FILTER_CONTEXT, * PFILTER_CONTEXT;