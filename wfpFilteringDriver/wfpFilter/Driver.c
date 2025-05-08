#include <ntddk.h>
#include "setup.h"

NTSTATUS UnloadDriver(_In_ PDRIVER_OBJECT driverObject) {
	UNREFERENCED_PARAMETER(driverObject);
	
	WfpCleanup();
	KdPrint(("Unloading the driver...\n"));

	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT driverObject, _In_ PUNICODE_STRING registryPath) {
	UNREFERENCED_PARAMETER(registryPath);

	KdPrint(("Loading the driver...\n"));
	driverObject->DriverUnload = UnloadDriver;

	NTSTATUS status = WfpInit(driverObject);
	if (!(NT_SUCCESS(status))) {
		KdPrint(("Driver failed to load!\n"));
		return status;
	}

	KdPrint(("Driver loaded!\n"));

	return STATUS_SUCCESS;
}