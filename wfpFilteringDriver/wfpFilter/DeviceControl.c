#include "DeviceControl.h"
#include "DeviceControlConsts.h"

//guides 
DEFINE_GUID( packetUDP, 0x466277a2, 0x43c7, 0x4c16, 0x9e, 0xda, 0x8b, 0xe9, 0x1d, 0xe1, 0xa0, 0x9e);
DEFINE_GUID( packetTCP, 0xd0c5be90, 0xe70d, 0x4c51, 0x8f, 0x95, 0xb8, 0xba, 0xef, 0xa, 0x44, 0x98);
DEFINE_GUID( sublayerGuidU,0x7defbe04, 0xc57, 0x4686, 0x8f, 0x52, 0x5c, 0x52, 0x88, 0x31, 0x9a, 0x65);

//private functions declaretion
static VOID addFilter(const PFILTER_SETTINGS filterS, const PFILTER_CONTEXT context);

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
			FILTER_SETTINGS filter = { 0 };
			RtlCopyMemory(&filter, inputBuffer, sizeof(FILTER_SETTINGS));
			if (Irp->Size > sizeof(FILTER_SETTINGS))
			{
				if (filter.payloadSize == 0 || filter.filterName[0] == '\0' || filter.filterName[FILTER_NAME_SIZE] != '\0')
				{
					status = STATUS_INVALID_DEVICE_REQUEST; //if Irp size is bigger there have to be a payload
					goto End;
				}
				PFILTER_CONTEXT context = (PFILTER_CONTEXT)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(FILTER_CONTEXT), 'ctxt');
				if (context == NULL)
				{
					status = STATUS_INSUFFICIENT_RESOURCES;
					goto End;
				}
				context->lengthOfData = filter.payloadSize;
				context->payload = (char*)ExAllocatePool2(POOL_FLAG_NON_PAGED, context->lengthOfData, 'load');
				if (context->payload == NULL)
				{
					ExFreePool(context);
					status = STATUS_INSUFFICIENT_RESOURCES;
					goto End;
				}
				RtlCopyMemory(context->payload, inputBuffer + sizeof(FILTER_SETTINGS), context->lengthOfData);
				if (irpStack->Parameters.DeviceIoControl.InputBufferLength > sizeof(FILTER_SETTINGS) + context->lengthOfData)
				{
					if (filter.changeToSize == 0)
					{
						status = STATUS_INVALID_DEVICE_REQUEST; //if Irp size is bigger there have to be a payload
						goto End;
					}
					context->lengthOfChange = filter.changeToSize;
					context->changeTo = (char*)ExAllocatePool2(POOL_FLAG_NON_PAGED, context->lengthOfChange, 'cgTo');
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

// * private function
static VOID addFilter(const PFILTER_SETTINGS filterS, const PFILTER_CONTEXT context)
{
	FWPM_FILTER filter = { 0 };
	FWPM_FILTER_CONDITION conditions[5] = { 0 };

	int i = 0;
	if (filterS->srcIpAddr != 0)
	{
		conditions[i].fieldKey = FWPM_CONDITION_IP_LOCAL_ADDRESS;
		conditions[i].matchType = FWP_MATCH_EQUAL;
		conditions[i].conditionValue.type = FWP_V4_ADDR_MASK;
		conditions[i].conditionValue.v4AddrMask = (FWP_V4_ADDR_AND_MASK*)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(FWP_V4_ADDR_AND_MASK), 'addr');
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
		conditions[i].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
		conditions[i].conditionValue.type = FWP_V4_ADDR_MASK;
		conditions[i].conditionValue.v4AddrMask = (FWP_V4_ADDR_AND_MASK*)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(FWP_V4_ADDR_AND_MASK), 'addr');
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
		conditions[i].fieldKey = FWPM_CONDITION_IP_REMOTE_PORT;
		conditions[i].matchType = FWP_MATCH_EQUAL;
		conditions[i].conditionValue.type = FWP_UINT16;
		conditions[i].conditionValue.uint16 = filterS->dstPort;
		i++;
	}
	if (filterS->isUDP) //filter only udp
	{
		conditions[i].fieldKey = FWPM_CONDITION_IP_PROTOCOL;
		conditions[i].matchType = FWP_MATCH_EQUAL;
		conditions[i].conditionValue.type = FWP_UINT8;
		conditions[i].conditionValue.uint8 = IPPROTO_UDP;
		i++;
	}

	filter.action.type = (context != NULL) ? FWP_ACTION_CALLOUT_INSPECTION : FWP_ACTION_BLOCK;
	filter.rawContext = (UINT64)((context != NULL) ? context : (PVOID)NULL);
	filter.filterCondition = conditions;
	filter.numFilterConditions = i;
	filter.action.calloutKey = (filterS->isUDP) ? packetUDP : packetTCP;
	filter.subLayerKey = sublayerGuidU;
	filter.weight.type = FWP_EMPTY; //the sublayer has max weight so it does not really matter
	filter.layerKey = (filterS->isUDP) ? FWPM_LAYER_DATAGRAM_DATA_V4 : FWPM_LAYER_OUTBOUND_TRANSPORT_V4;
	filter.displayData.name = filterS->filterName;

	UINT64 id = 0;
	NTSTATUS status = FwpmFilterAdd0(res.hEngine, &filter, NULL, &id);
	if (NT_SUCCESS(status))
	{
		List_addId(id, filterS->filterName);
	}

	/*for (int j = 0; j < i; j++) * not sure if it is needed
	{
		if (conditions[j].conditionValue.type == FWP_V4_ADDR_MASK &&
			conditions[j].conditionValue.v4AddrMask != NULL)
		{
			ExFreePool2(conditions[j].conditionValue.v4AddrMask, 'addr', NULL, 0);
		}
	}*/
}
