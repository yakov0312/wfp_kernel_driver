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

// GUID headers
// https://www.gamedev.net/forums/topic/18905-initguid-an-explanation/
#define INITGUID
#include <guiddef.h>

EXTERN_C const GUID packetUDP;
EXTERN_C const GUID packetTCP;
EXTERN_C const GUID sublayerGuidU;

NTSTATUS networkDeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp);
NTSTATUS networkCreateC(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp);
NTSTATUS networkCloseC(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp);
