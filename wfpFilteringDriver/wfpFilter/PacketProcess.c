#include "PacketProcess.h"
#include "DeviceControlConsts.h"
#include "Resources.h"
#include "ndis/nblaccessors.h"
#include "ndis/nblapi.h"

#define UDP_HEADER_SIZE 8
#define ETHERNET_HEADER_SIZE 14

//headers of udp
// * private struct
typedef struct UDP_HEADER
{
	UINT16 srcPort;
	UINT16 destPort;
	UINT16 length;
	UINT16 checksum;
} UDP_HEADER, * PUDP_HEADER;

//peudo headers 
// * private struct
typedef struct PseudoHeader
{
	UINT32 srcIp;
	UINT32 dstIp;
	unsigned char zero;
	unsigned char protocol;
	UINT16 length;
} PSEUDO_HEADER, * PPSEUDO_HEADER;

//private functions declaretion
UINT32 calculateChecksum(const PUDP_HEADER udpHeader, const PPSEUDO_HEADER pseudoHeader, const unsigned char* data);


void processPacketUDP(
	const FWPS_INCOMING_VALUES0* inFixedValues, const FWPS_INCOMING_METADATA_VALUES0* inMetaValues, 
	void* layerData, const void* classifyContext, const FWPS_FILTER3* filter,
	UINT64 flowContext, FWPS_CLASSIFY_OUT0* classifyOut)
{
	UNREFERENCED_PARAMETER(flowContext);
	UNREFERENCED_PARAMETER(inFixedValues);
	UNREFERENCED_PARAMETER(classifyContext);

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
	const FWPS_INCOMING_VALUES0* inFixedValues, const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
	void* layerData, const void* classifyContext, const FWPS_FILTER3* filter, 
	UINT64 flowContext,FWPS_CLASSIFY_OUT0* classifyOut)
{
	UNREFERENCED_PARAMETER(flowContext);
	UNREFERENCED_PARAMETER(inFixedValues);
	UNREFERENCED_PARAMETER(inMetaValues);
	UNREFERENCED_PARAMETER(layerData);
	UNREFERENCED_PARAMETER(classifyOut);
	UNREFERENCED_PARAMETER(classifyContext);
	UNREFERENCED_PARAMETER(filter);
}

//calculate the checksum for a packet
// * private function
UINT32 calculateChecksum(const PUDP_HEADER udpHeader, const PPSEUDO_HEADER pseudoHeader, const unsigned char* data)
{
	size_t size = sizeof(UDP_HEADER) + sizeof(PSEUDO_HEADER) + pseudoHeader->length;
	if (size % 2 != 0)
	{
		size++;
	}
	char* buffer = (char*)ExAllocatePool2(POOL_FLAG_NON_PAGED, size, 'cSum');
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

NTSTATUS CalloutNotify(FWPS_CALLOUT_NOTIFY_TYPE  notifyType, const GUID* filterKey, FWPS_FILTER* filter) 
{
	UNREFERENCED_PARAMETER(notifyType);
	UNREFERENCED_PARAMETER(filterKey);
	UNREFERENCED_PARAMETER(filter);

	// Needs to be defined, but isn't required for anything.
	return STATUS_SUCCESS;
}