#pragma once
#include <ntddk.h>
#include "ndis/nbl.h"
#include <fwpsk.h>
#include <fwpmk.h>
#include "FilterIdContainer.h"

//structs
typedef struct Resources
{
	UINT32 calloutIdUDP;
	UINT32 calloutIdTCP;
	HANDLE hEngine;
	PDEVICE_OBJECT filterDeviceObject;
} Resources;

//global vars
extern Resources res;