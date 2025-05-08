#pragma once
#include "Filtering.h"

#define DEVICE_NAME L"\\Device\\NETWORK_DR"
#define SYM_NAME L"\\DosDevices\\NETWORK_DR"

//global vars
HANDLE subscriptionHandle; //handle to the subscription

VOID TermFilterDeviceObject();
VOID TermCalloutData();
VOID TermWfpEngine();
VOID WfpCleanup();

NTSTATUS SublayerAdd();
NTSTATUS CalloutRegister();
NTSTATUS WfpInit(PDRIVER_OBJECT driverObject);
NTSTATUS CalloutAdd();
void NTAPI setEngineRelated(void* context, FWPM_SERVICE_STATE serviceState);
