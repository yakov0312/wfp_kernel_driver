#include "Setup.h"
#include "DeviceControl.h"
#include "PacketProcess.h"

static HANDLE subscriptionHandle; //handle to the subscription

//declaretion of private functions
static NTSTATUS CalloutRegister();
static NTSTATUS CalloutAdd();
static void setEngineRelated(void* context, FWPM_SERVICE_STATE serviceState);
static NTSTATUS SublayerAdd();
static VOID TermFilterDeviceObject();
static VOID TermCalloutData();
static VOID TermWfpEngine();

// * public 
NTSTATUS WfpInit(PDRIVER_OBJECT driverObject)
{
	UNICODE_STRING deviceName = RTL_CONSTANT_STRING(DEVICE_NAME);
	UNICODE_STRING symName = RTL_CONSTANT_STRING(SYM_NAME);

	driverObject->MajorFunction[IRP_MJ_CREATE] = networkCreateC;//does not have to do anything just be defined
	driverObject->MajorFunction[IRP_MJ_CLOSE] = networkCloseC;
	driverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = networkDeviceControl;
	// Create a device object (used in the callout registration)
	NTSTATUS status = IoCreateDevice(driverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &res.filterDeviceObject);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("Failed to create the filter device object (0x%X).\n", status));
		return status;
	}

	status = IoCreateSymbolicLink(&symName, &deviceName);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("Failed to create the symbolicLink (0x%X).\n", status));
		return status;
	}

	FWPM_SERVICE_STATE state = FwpmBfeStateGet();
	if (state == FWPM_SERVICE_RUNNING)
	{
		setEngineRelated(NULL, state);
	}
	else
	{
		status = FwpmBfeStateSubscribeChanges(res.filterDeviceObject, setEngineRelated, NULL, &subscriptionHandle);
		if (!NT_SUCCESS(status))
		{
			KdPrint(("Failed subscribe to the BFE(0x%X).\n", status));
			return status;
		}
	}

	return status;
}

// * public
VOID WfpCleanup()
{
	TermCalloutData();
	TermWfpEngine();
	TermFilterDeviceObject();
}

// * private 
static NTSTATUS CalloutRegister() {

	// https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/fwpsk/ns-fwpsk-fwps_callout0_
	FWPS_CALLOUT calloutU = {
		.calloutKey = packetUDP,		// Unique GUID that identifies the callout
		.flags = 0,					// None
		.classifyFn = processPacketUDP,		// Callout function used to process network data
		.notifyFn = CalloutNotify,		// Callout function used to receive notifications from the filter engine, not needed in our case (but needs to be defined)
		.flowDeleteFn = NULL,// Callout function used to process terminated data, not needed in our case (does't need to be defined)
	};
	// https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/fwpsk/nf-fwpsk-fwpscalloutregister0
	NTSTATUS result = FwpsCalloutRegister(res.filterDeviceObject, &calloutU, &res.calloutIdUDP);
	if (!NT_SUCCESS(result))
	{
		return result;
	}

	FWPS_CALLOUT calloutT = {
		.calloutKey = packetTCP,		// Unique GUID that identifies the callout
		.flags = 0,					// None
		.classifyFn = processPacketTCP,		// Callout function used to process network data
		.notifyFn = CalloutNotify,		// Callout function used to receive notifications from the filter engine, not needed in our case (but needs to be defined)
		.flowDeleteFn = NULL				// Callout function used to process terminated data, not needed in our case (does't need to be defined)
	};
	return FwpsCalloutRegister(res.filterDeviceObject, &calloutT, &res.calloutIdTCP);
}

// * private
static NTSTATUS CalloutAdd() {

	// https://learn.microsoft.com/en-us/windows/win32/api/fwpmtypes/ns-fwpmtypes-fwpm_callout0
	FWPM_CALLOUT calloutU = {
		.flags = 0,								 // None
		.displayData.name = L"udpCall",
		.displayData.description = L"filterUdp",
		.calloutKey = packetUDP,					 // Unique GUID that identifies the callout, should be the same as the registered FWPS_CALLOUT GUID
		.applicableLayer = FWPM_LAYER_DATAGRAM_DATA_V4// https://learn.microsoft.com/en-us/windows/win32/fwp/management-filtering-layer-identifiers-
	};

	// https://learn.microsoft.com/en-us/windows/win32/api/fwpmu/nf-fwpmu-fwpmcalloutadd0
	NTSTATUS result = FwpmCalloutAdd(res.hEngine, &calloutU, NULL, &res.calloutIdUDP);
	if (!NT_SUCCESS(result))
	{
		return result;
	}

	FWPM_CALLOUT calloutT =
	{
		.flags = 0,
		.displayData.name = L"tcpCall",
		.displayData.description = L"filterTcp",
		.calloutKey = packetTCP,
		.applicableLayer = FWPM_LAYER_STREAM_V4
	};

	return FwpmCalloutAdd(res.hEngine, &calloutT, NULL, &res.calloutIdTCP);
}

// * private callback
static void setEngineRelated(void* context, FWPM_SERVICE_STATE serviceState)
{
	if (serviceState == FWPM_SERVICE_RUNNING)
	{
		NTSTATUS status = FwpmEngineOpen(NULL, RPC_C_AUTHN_DEFAULT, NULL, NULL, &res.hEngine);
		if (!NT_SUCCESS(status))
		{
			KdPrint(("Failed to open the filter engine (0x%X).\n", status));
		}

		// Register a callout with the filter engine
		status = CalloutRegister();
		if (!NT_SUCCESS(status))
		{
			KdPrint(("Failed to register the filter callout (0x%X).\n", status));
		}

		// Add the callout to the system's layers
		status = CalloutAdd();
		if (!NT_SUCCESS(status))
		{
			KdPrint(("Failed to add the filter callout (0x%X).\n", status));
		}

		// Add a sublayer to the system
		status = SublayerAdd();
		if (!NT_SUCCESS(status))
		{
			KdPrint(("Failed to add the sublayer (0x%X).\n", status));
		}
		FwpmBfeStateUnsubscribeChanges0(subscriptionHandle);
	}
}

// * private 
static NTSTATUS SublayerAdd() 
{
	// https://learn.microsoft.com/en-us/windows/win32/api/fwpmtypes/ns-fwpmtypes-fwpm_sublayer0
	FWPM_SUBLAYER sublayer = {
		.displayData.name = L"udpLayer",
		.subLayerKey = sublayerGuidU,
		.weight = 65535,
	};

	// https://learn.microsoft.com/en-us/windows/win32/api/fwpmu/nf-fwpmu-fwpmsublayeradd0
	return FwpmSubLayerAdd(res.hEngine, &sublayer, NULL);
}

// * private 
static VOID TermFilterDeviceObject()
{
	KdPrint(("Terminating the device object.\n"));

	if (res.filterDeviceObject)
	{
		// Remove the filter device object
		IoDeleteDevice(res.filterDeviceObject);
		res.filterDeviceObject = NULL;
	}
}

// * private
static VOID TermCalloutData() {
	KdPrint(("Terminating filters, sublayers and callouts.\n"));

	if (res.hEngine)
	{
		// Remove the added filters and sublayers 
		List_del(res.hEngine);
		FwpmSubLayerDeleteByKey(res.hEngine, &sublayerGuidU);

		// Remove the callout from the FWPM_LAYER_INBOUND_TRANSPORT_V4 layer
		if (res.calloutIdTCP)
		{
			FwpmCalloutDeleteById(res.hEngine, res.calloutIdTCP);
			res.calloutIdTCP = 0;
		}

		// Unregister the callout
		if (res.calloutIdUDP)
		{
			FwpmCalloutDeleteById(res.hEngine, res.calloutIdUDP);
			res.calloutIdUDP = 0;
		}
	}
}

// * private 
static VOID TermWfpEngine()
{
	KdPrint(("Terminating the filter engine handle.\n"));

	if (res.hEngine) {

		// Close the filter engine handle
		FwpmEngineClose(res.hEngine);
		res.hEngine = NULL;
	}
}