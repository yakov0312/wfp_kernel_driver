#pragma once

/*
*	Note:
*	If the linker complains, add the following additional dependencies:
*	- $(DDK_LIB_PATH)\fwpkclnt.lib
*	- $(DDK_LIB_PATH)\ndis.lib
*	- $(SDK_LIB_PATH)\uuid.lib
*/

// Network driver headers
#define NDIS630
#include <ndis.h>
#include "Resources.h"

#define INITGUID
#include <guiddef.h>

EXTERN_C const GUID packetUDP;
EXTERN_C const GUID packetTCP;
EXTERN_C const GUID sublayerGuidU;

// GUID headers
// https://www.gamedev.net/forums/topic/18905-initguid-an-explanation/
#define UDP_HEADER_SIZE 8
#define ETHERNET_HEADER_SIZE 14

//codes:
#define IOCTL_REMOVE_FILTER CTL_CODE(FILE_DEVICE_NETWORK, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ADD_FILTER CTL_CODE(FILE_DEVICE_NETWORK, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

//struct that a program send to the driver with some settings about the fitler
typedef struct _FILTER_SETTINGS
{
    char filterName[FILTER_NAME_SIZE]; //no need for the \0 here
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
//headers of udp
typedef struct UDP_HEADER
{
    UINT16 srcPort;
    UINT16 destPort;
    UINT16 length;
    UINT16 checksum;
} UDP_HEADER, * PUDP_HEADER;
//peudo headers
typedef struct PseudoHeader
{
    UINT32 srcIp;
    UINT32 dstIp;
    unsigned char zero;
    unsigned char protocol;
    UINT16 length;
} PSEUDO_HEADER, * PPSEUDO_HEADER;

NTSTATUS networkDeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp);
NTSTATUS networkCreateC(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp);
NTSTATUS networkCloseC(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp);

//callout function for udp layer
void processPacketUDP(const FWPS_INCOMING_VALUES0* inFixedValues, const FWPS_INCOMING_METADATA_VALUES0* inMetaValues, void* layerData, const void* classifyContext, const FWPS_FILTER3* filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0* classifyOut);
//callout function for tcp layer
void processPacketTCP(const FWPS_INCOMING_VALUES0* inFixedValues, const FWPS_INCOMING_METADATA_VALUES0* inMetaValues, void* layerData, const void* classifyContext, const FWPS_FILTER3* filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0* classifyOut);
NTSTATUS CalloutNotify(FWPS_CALLOUT_NOTIFY_TYPE notifyType, const GUID* filterKey, FWPS_FILTER* filter);

//calculate the checksum for a packet
UINT32 calculateChecksum(const PUDP_HEADER udpHeader, const PPSEUDO_HEADER pseudoHeader, const unsigned char* data);

NTSTATUS SublayerAdd();
VOID addFilter(const PFILTER_SETTINGS filterS, const PFILTER_CONTEXT context);
