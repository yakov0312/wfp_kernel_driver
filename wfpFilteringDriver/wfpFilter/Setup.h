#pragma once
#include "wdm.h"

#define DEVICE_NAME L"\\Device\\NETWORK_DR"
#define SYM_NAME L"\\DosDevices\\NETWORK_DR"

VOID WfpCleanup();
NTSTATUS WfpInit(PDRIVER_OBJECT driverObject);
