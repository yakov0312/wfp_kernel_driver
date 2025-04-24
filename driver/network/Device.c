#define INITGUID
#include <initguid.h> 
#include <guiddef.h>
#include "Device.h"
#include "ndis/nblapi.h"
#include "ndis/nblaccessors.h"

DEFINE_GUID(
    packetUDP,
    0x466277a2, 0x43c7, 0x4c16, 0x9e, 0xda, 0x8b, 0xe9, 0x1d, 0xe1, 0xa0, 0x9e
);

DEFINE_GUID(
    packetTCP,
    0xd0c5be90, 0xe70d, 0x4c51, 0x8f, 0x95, 0xb8, 0xba, 0xef, 0xa, 0x44, 0x98
);

DEFINE_GUID(
    sublayerGuid,
    0x7defbe04, 0xc57, 0x4686, 0x8f, 0x52, 0x5c, 0x52, 0x88, 0x31, 0x9a, 0x65);



VOID networkEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
) {
    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    if (IoControlCode == IOCTL_ADD_FILTER)
    {
        PFILTER_SETTINGS filter = NULL;
        size_t bufferSize = 0;
        char* payload = NULL;
        NTSTATUS status = WdfRequestRetrieveInputBuffer(Request, sizeof(FILTER_SETTINGS), &filter, &bufferSize);
        if (status != STATUS_SUCCESS || InputBufferLength < sizeof(FILTER_SETTINGS))
        {
            WdfRequestComplete(Request, status);
            return;
        }
        if (InputBufferLength == sizeof(FILTER_SETTINGS))
        {
            //no payload
            addFilter(filter, NULL);
            WdfRequestComplete(Request, status);
            return;
        }

        payload = (char*)((char*)filter + sizeof(FILTER_SETTINGS));
        PFILTER_CONTEXT context = (PFILTER_CONTEXT)ExAllocatePool2(NonPagedPool, sizeof(FILTER_CONTEXT), 'ctxt');
        if (context == NULL) 
        {
            WdfRequestComplete(Request, STATUS_INSUFFICIENT_RESOURCES);
            return;
        }
        context->lengthOfData = filter->payloadSize;
        context->payload = (char*)ExAllocatePool2(NonPagedPool, context->lengthOfData, 'load');
        if (context->payload == NULL)
        {
            ExFreePool(context);
            WdfRequestComplete(Request, STATUS_INSUFFICIENT_RESOURCES);
            return;
        }
        memcpy(context->payload, payload, context->lengthOfData);

        if (InputBufferLength > sizeof(FILTER_SETTINGS) + context->lengthOfData)
        {
            context->lengthOfChange = filter->changeToSize;
            char* changePayload = (char*)(payload + context->lengthOfData);
            context->changeTo = (char*)ExAllocatePool2(NonPagedPool, context->lengthOfChange, 'cgTo');
            if (context->changeTo == NULL)
            {
                ExFreePool(context->payload);
                ExFreePool(context);
                WdfRequestComplete(Request, STATUS_INSUFFICIENT_RESOURCES);
                return;
            }
            memcpy(context->changeTo, changePayload, context->lengthOfChange);
        }
        
        addFilter(filter, context);
    }
    else if (IoControlCode == IOCTL_REMOVE_FILTER)
    {
        // Handle removing a filter
    }
    else
    {    
        WdfRequestComplete(Request, STATUS_INVALID_DEVICE_REQUEST);
        return;
    }

    WdfRequestComplete(Request, STATUS_SUCCESS);
}

VOID addFilter(const PFILTER_SETTINGS filterS, const PFILTER_CONTEXT context)
{
    FWPM_FILTER filter = { 0 };
    FWPM_FILTER_CONDITION conditions[5] = { 0 };

    int i = 0;
    if (filterS->srcIpAddr != 0)
    {
        conditions[i].fieldKey = FWPM_CONDITION_IP_SOURCE_ADDRESS;
        conditions[i].matchType = FWP_MATCH_EQUAL;
        conditions[i].conditionValue.type = FWP_V4_ADDR_MASK;
        conditions[i].conditionValue.v4AddrMask = (FWP_V4_ADDR_AND_MASK*)ExAllocatePool2(NonPagedPool, sizeof(FWP_V4_ADDR_AND_MASK), 'addr');
        if (conditions[i].conditionValue.v4AddrMask == NULL) 
        {
            return;
        }
        conditions[i].conditionValue.v4AddrMask->addr = filterS->srcIpAddr;
        conditions[i].conditionValue.v4AddrMask->mask = 0xFFFFFFFF;
        i++;
    }

    if (filterS->dstIpAddr != 0)
    {
        conditions[i].fieldKey = FWPM_CONDITION_IP_DESTINATION_ADDRESS;
        conditions[i].conditionValue.type = FWP_V4_ADDR_MASK;
        conditions[i].conditionValue.v4AddrMask = (FWP_V4_ADDR_AND_MASK*)ExAllocatePool2(NonPagedPool, sizeof(FWP_V4_ADDR_AND_MASK), 'addr');
        if (conditions[i].conditionValue.v4AddrMask == NULL)
        {
            if (conditions[0].conditionValue.v4AddrMask != NULL)
            {
                ExFreePool(conditions[0].conditionValue.v4AddrMask);
            }
            return;
        }
        conditions[i].conditionValue.v4AddrMask->addr = filterS->dstIpAddr;
        conditions[i].conditionValue.v4AddrMask->mask = 0xFFFFFFFF;
        conditions[i].matchType = FWP_MATCH_EQUAL;
        i++;
    }
    if (filterS->dstPort != 0)
    {
        conditions[i].fieldKey = FWPM_CONDITION_IP_DESTINATION_PORT;
        conditions[i].matchType = FWP_MATCH_EQUAL;
        conditions[i].conditionValue.type = FWP_UINT16;  
        conditions[i].conditionValue.uint16 = filterS->dstPort;
        i++;
    }
    if (filterS->isUDP)
    {
        conditions[i].fieldKey = FWPM_CONDITION_IP_PROTOCOL;
        conditions[i].matchType = FWP_MATCH_EQUAL;
        conditions[i].conditionValue.type = FWP_UINT16;
        conditions[i].conditionValue.uint16 = (filterS->isUDP) ? IPPROTO_UDP : IPPROTO_TCP;
        i++;
    }

    filter.action.type = FWP_ACTION_BLOCK;
    filter.filterCondition = conditions;
    filter.action.calloutKey = (filterS->isUDP) ? packetUDP : packetTCP;
    filter.subLayerKey = sublayerGuid;
    filter.weight.type = FWP_EMPTY;
    filter.numFilterConditions = i;
    filter.layerKey = (filterS->isUDP) ? FWPM_LAYER_INBOUND_TRANSPORT_V4 : FWPM_LAYER_STREAM_V4;

    if (context != NULL)
    {
        filter.rawContext = (UINT64)context;
        filter.action.type = FWP_ACTION_CALLOUT_INSPECTION;
    }
    UINT64 id = 0;
    FwpmFilterAdd0(res.hEngine, &filter, NULL, &id);
    Array_addId(id, &(res.filterArr));

    for (int j = 0; j < i; j++)
    {
        if (conditions[j].conditionValue.type == FWP_V4_ADDR_MASK &&
            conditions[j].conditionValue.v4AddrMask != NULL) 
        {
            ExFreePool2(conditions[j].conditionValue.v4AddrMask, 'addr', NULL, 0);
        }
    }
}

void processPacketUDP(
    const FWPS_INCOMING_VALUES0* inFixedValues,
    const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
    void* layerData,
    const void* classifyContext,
    const FWPS_FILTER3* filter,
    UINT64 flowContext,
    FWPS_CLASSIFY_OUT0* classifyOut)
{
    UNREFERENCED_PARAMETER(flowContext);
    PFILTER_CONTEXT context = (PFILTER_CONTEXT)filter->context;
    if (context == NULL)
    {
        classifyOut->actionType = FWP_ACTION_BLOCK;
        return;
    }
    else if (context->payload != NULL)
    {
        NET_BUFFER_LIST* packetList = (NET_BUFFER_LIST*)layerData;
        NET_BUFFER* firstFragment = NET_BUFFER_LIST_FIRST_NB(packetList);

        ULONG packetSize = firstFragment->DataLength;
        ULONG ipHeaderSize = inMetaValues->ipHeaderSize;
        ULONG totalHeaderLength = ipHeaderSize + UDP_HEADER_SIZE;
        ULONG payloadLength = packetSize - totalHeaderLength;

        PVOID udpBuffer = (PBYTE)ExAllocatePool2(POOL_FLAG_NON_PAGED, (SIZE_T)packetSize, 'udpP');

        if (!udpBuffer) 
        {
            return;
        }

        PBYTE udpPacket = (PBYTE)NdisGetDataBuffer(firstFragment, (ULONG)packetSize, udpBuffer, 1, 0);
        if (!udpPacket) 
        {
            ExFreePoolWithTag((PVOID)udpBuffer, 'udpP');
            return;
        }
        
        PBYTE udpPayload = ExAllocatePoolWithTag(POOL_FLAG_NON_PAGED, (SIZE_T)payloadLength, 'udpD');
        if (!udpPayload) 
        {
            return;
        }

        RtlCopyMemory(udpPayload, &udpPacket[totalHeaderLength], payloadLength);

        for (size_t i = 0; i < payloadLength - context->lengthOfData; i++)
        {
            if (memcmp(&udpPayload[i], context->payload, context->lengthOfData) == 0)
            {
                if (context->changeTo == NULL)
                {
                    classifyOut->actionType = FWP_ACTION_BLOCK;
                    return;
                }
                else
                {
                    //here create new buffer for the data and change it then calculate the checksum 
                    classifyOut->actionType = FWP_ACTION_CONTINUE;
                    return;
                }
            }
        }
    }
    else
    {
        classifyOut->actionType = FWP_ACTION_CONTINUE;
        return;
    }
}

void processPacketTCP(
    const FWPS_INCOMING_VALUES0* inFixedValues,
    const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
    void* layerData,
    const void* classifyContext,
    const FWPS_FILTER3* filter,
    UINT64 flowContext,
    FWPS_CLASSIFY_OUT0* classifyOut)
{
    UNREFERENCED_PARAMETER(flowContext);
    UNREFERENCED_PARAMETER(inFixedValues);
    UNREFERENCED_PARAMETER(inMetaValues);
    UNREFERENCED_PARAMETER(layerData);
    return;
}

UINT32 calculateChecksum(const PUDP_HEADER udpHeader, const PPSEUDO_HEADER pseudoHeader, const unsigned char* data)
{
    size_t size = sizeof(UDP_HEADER) + sizeof(PSEUDO_HEADER) + pseudoHeader->length;
    if (size % 2 != 0)
    {
        size++;
    }
    char* buffer = (char*)ExAllocatePool2(NonPagedPool, size, 'cSum');
    if (buffer == NULL)
    {
        return 0;
    }
    RtlCopyMemory(buffer, udpHeader, sizeof(UDP_HEADER));
    RtlCopyMemory(buffer + sizeof(UDP_HEADER), pseudoHeader, sizeof(PSEUDO_HEADER));
    RtlCopyMemory(buffer + sizeof(UDP_HEADER) + sizeof(PSEUDO_HEADER), data, pseudoHeader->length);
    UINT16* chunkBuffer = (UINT16*)buffer;

    UINT32 totalSize = 0;
    for (int i = 0; i < size / 2; i++)
    {
        totalSize += chunkBuffer[i];
        if (totalSize > 0xFFFF)
        {
            totalSize = (totalSize & 0xFFFF) + (totalSize >> 16);
        }
    }

    ExFreePoolWithTag(buffer, 'cSum');

    return ~totalSize & 0xFFFF;;
}
