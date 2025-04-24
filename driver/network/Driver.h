#pragma once

//includes:
#include "Device.h"

//consts
#define DEVICE_NAME L"\\Device\\NETWORK_DR"
#define SYM_NAME L"\\DosDevices\\NETWORK_DR"

//global vars:
EXTERN_C const GUID packetUDP;
EXTERN_C const GUID packetTCP;

//functions:
// 
//main function for the driver 
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath);

//register and add the callouts 
NTSTATUS calloutRegAdd(_In_ PDRIVER_OBJECT     DriverObject);

//callout function for device add 
NTSTATUS networkEvtDeviceAdd(_In_ WDFDRIVER Driver, _Inout_ PWDFDEVICE_INIT DeviceInit);

//unload function for the driver 
NTSTATUS DriverUnload(_In_ PDRIVER_OBJECT driverObject);