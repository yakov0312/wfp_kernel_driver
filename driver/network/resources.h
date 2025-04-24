#pragma once
#include <ntddk.h>
#include "ndis/nbl.h"
#include <fwpsk.h>
#include <fwpmk.h>
#include "filterArray.h"

//structs
typedef struct Resources
{
	UINT32 calloutIdUDP;
	UINT32 calloutIdTCP;
	HANDLE hEngine;
	idArray filterArr;

} Resources;

//global vars
Resources res;