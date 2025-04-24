#include "Driver.h"

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT     DriverObject,
    _In_ PUNICODE_STRING    RegistryPath
)
{

    NTSTATUS status = STATUS_SUCCESS;
    DriverObject->DriverUnload = DriverUnload;
    WDF_DRIVER_CONFIG config;

    WDF_DRIVER_CONFIG_INIT(&config, networkEvtDeviceAdd);
    DbgPrint("configured the config.");

    status = WdfDriverCreate(DriverObject,
        RegistryPath,
        WDF_NO_OBJECT_ATTRIBUTES,
        &config,
        WDF_NO_HANDLE);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = FwpmEngineOpen(
        NULL,
        RPC_C_AUTHN_WINNT,
        NULL,     
        NULL,        
        &(res.hEngine)  
    );
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    //register the callout and add the callout 
    calloutRegAdd(DriverObject);

    return status;
}


NTSTATUS calloutRegAdd(_In_ PDRIVER_OBJECT     DriverObject)
{
    //register the callouts:
    FWPS_CALLOUT3 callout = { 0 };

    callout.calloutKey = packetUDP;
    callout.classifyFn = processPacketUDP;
    NTSTATUS status = FwpsCalloutRegister(DriverObject, &callout, &(res.calloutIdUDP));

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    callout.calloutKey = packetTCP;
    callout.classifyFn = processPacketTCP;
    status = FwpsCalloutRegister(DriverObject, &callout, &(res.calloutIdTCP));

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    //add the callouts:
    FWPM_CALLOUT calloutU = { 0 };
    calloutU.applicableLayer = FWPM_LAYER_DATAGRAM_DATA_V4;
    calloutU.calloutKey = packetUDP;
    FwpmCalloutAdd(res.hEngine, &calloutU, NULL, res.calloutIdUDP);

    FWPM_CALLOUT calloutT = { 0 };
    calloutT.applicableLayer = FWPM_LAYER_STREAM_V4;
    calloutT.calloutKey = packetTCP;
    FwpmCalloutAdd(res.hEngine, &calloutT, NULL, res.calloutIdTCP);

    //here add sublayer
    FWPM_SUBLAYER sublayer = {
    .displayData.name = L"inspectLayer",
    .subLayerKey = sublayerGuid,
    .weight = 65535
    };

    return FwpmSubLayerAdd(res.hEngine, &sublayer, NULL);
}

NTSTATUS
networkEvtDeviceAdd(
    _In_    WDFDRIVER       Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
)
{
    UNREFERENCED_PARAMETER(Driver);
    WDFDEVICE hDevice;

    UNICODE_STRING deviceName;
    UNICODE_STRING symbolicLinkName;

    RtlInitUnicodeString(&deviceName, DEVICE_NAME);
    RtlInitUnicodeString(&symbolicLinkName, SYM_NAME);

    NTSTATUS status = WdfDeviceCreate(&DeviceInit,
        WDF_NO_OBJECT_ATTRIBUTES,
        &hDevice
    );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = WdfDeviceCreateSymbolicLink(hDevice, &symbolicLinkName);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    WDF_IO_QUEUE_CONFIG ioQueueConfig;
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchSequential);
    ioQueueConfig.EvtIoDeviceControl = networkEvtIoDeviceControl;

    status = WdfIoQueueCreate(hDevice, &ioQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, NULL);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return status;
}

NTSTATUS DriverUnload(_In_ PDRIVER_OBJECT driverObject)
{
    UNREFERENCED_PARAMETER(driverObject);

    Array_del(&(res.filterArr));
    FwpmSubLayerDeleteByKey(&(res.hEngine), &sublayerGuid);

    FwpmEngineClose(res.hEngine);

    if (res.calloutIdUDP)
    {
        FwpsCalloutUnregisterById(res.calloutIdUDP);
    }

    if (res.calloutIdTCP)
    {
        FwpsCalloutUnregisterById(res.calloutIdTCP);
    }

}
