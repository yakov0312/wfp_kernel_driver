#pragma once
#include <ntddk.h>
#include "ndis/nbl.h"
#include <fwpsk.h>



typedef struct idArray
{
	UINT64* arrayId;
	HANDLE hEngine;
	size_t length;
	size_t usedLength;
} idArray;

void Array_addId(const UINT64 id, idArray* arr);
void Array_removeId(const UINT64 id, idArray* arr);
void Array_del(idArray* arr);