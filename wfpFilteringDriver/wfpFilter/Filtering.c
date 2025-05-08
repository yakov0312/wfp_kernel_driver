#include "Filtering.h"

DEFINE_GUID(
	packetUDP,
	0x466277a2, 0x43c7, 0x4c16, 0x9e, 0xda, 0x8b, 0xe9, 0x1d, 0xe1, 0xa0, 0x9e
);

DEFINE_GUID(
	packetTCP,
	0xd0c5be90, 0xe70d, 0x4c51, 0x8f, 0x95, 0xb8, 0xba, 0xef, 0xa, 0x44, 0x98
);

DEFINE_GUID(
	sublayerGuidU,
	0x7defbe04, 0xc57, 0x4686, 0x8f, 0x52, 0x5c, 0x52, 0x88, 0x31, 0x9a, 0x65);

NTSTATUS networkDeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp) 
{
	UNREFERENCED_PARAMETER(DeviceObject);
	PIO_STACK_LOCATION irpStack;
	NTSTATUS status = STATUS_SUCCESS;
	ULONG_PTR information = 0;
	if (res.hEngine == NULL)
	{
		status = STATUS_SERVER_UNAVAILABLE; //waiting for the bfe to start
		goto End;
	}

	irpStack = IoGetCurrentIrpStackLocation(Irp);

	PUCHAR inputBuffer = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;

	if (irpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_ADD_FILTER)
	{
		if (irpStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof(FILTER_SETTINGS))
		{
			FILTER_SETTINGS filter = {0};
			RtlCopyMemory(&filter, inputBuffer, sizeof(FILTER_SETTINGS));
			if (Irp->Size > sizeof(FILTER_SETTINGS))
			{
				PFILTER_CONTEXT context = (PFILTER_CONTEXT)ExAllocatePool2(NonPagedPool, sizeof(FILTER_CONTEXT), 'ctxt');
				if (context == NULL)
				{
					status = STATUS_INSUFFICIENT_RESOURCES;
					goto End;
				}
				context->lengthOfData = filter.payloadSize;
				context->payload = (char*)ExAllocatePool2(NonPagedPool, context->lengthOfData, 'load');
				if (context->payload == NULL)
				{
					ExFreePool(context);
					status = STATUS_INSUFFICIENT_RESOURCES;
					goto End;
				}
				RtlCopyMemory(context->payload, inputBuffer + sizeof(FILTER_SETTINGS), context->lengthOfData);
				if (irpStack->Parameters.DeviceIoControl.InputBufferLength > sizeof(FILTER_SETTINGS) + context->lengthOfData)
				{
					context->lengthOfChange = filter.changeToSize;
					context->changeTo = (char*)ExAllocatePool2(NonPagedPool, context->lengthOfChange, 'cgTo');
					if (context->changeTo == NULL)
					{
						ExFreePool(context->payload);
						ExFreePool(context);
						status = STATUS_INSUFFICIENT_RESOURCES;
						goto End;
					}
					RtlCopyMemory(context->changeTo, inputBuffer + sizeof(FILTER_SETTINGS) + filter.payloadSize, context->lengthOfChange);
				}
				addFilter(&filter, context);
			}
		}

	}
	else if (irpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_REMOVE_FILTER)
	{
		status = STATUS_SUCCESS;
	}
	else
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
	}
End:
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = information;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS networkCreateC(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp)
{
	// Simply return success for Create operation
	Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS networkCloseC(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp)
{
	// Simply return success for Close operation
	Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
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
	UNREFERENCED_PARAMETER(classifyOut);
	UNREFERENCED_PARAMETER(classifyContext);
	UNREFERENCED_PARAMETER(filter);
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


NTSTATUS CalloutNotify(
	FWPS_CALLOUT_NOTIFY_TYPE  notifyType,
	const GUID* filterKey,
	FWPS_FILTER* filter
) {
	UNREFERENCED_PARAMETER(notifyType);
	UNREFERENCED_PARAMETER(filterKey);
	UNREFERENCED_PARAMETER(filter);

	// Needs to be defined, but isn't required for anything.
	return STATUS_SUCCESS;
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
	filter.subLayerKey = sublayerGuidU;
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
	List_addId(id);

	for (int j = 0; j < i; j++)
	{
		if (conditions[j].conditionValue.type == FWP_V4_ADDR_MASK &&
			conditions[j].conditionValue.v4AddrMask != NULL)
		{
			ExFreePool2(conditions[j].conditionValue.v4AddrMask, 'addr', NULL, 0);
		}
	}
}
