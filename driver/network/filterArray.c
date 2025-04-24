#include "filterArray.h"
#include <fwpmu.h>

//this is sort of a private func because it is defined only in c file 
void Array_init(const size_t size, HANDLE hEngine, idArray* arr)
{
	if(arr != NULL)
	{
		if (arr->arrayId != NULL)
		{
			Array_del(arr);
		}
		arr->arrayId = (UINT64*)ExAllocatePool2(NonPagedPool, size * sizeof(UINT64), 'idAr');
		arr->length = size;
		arr->hEngine = hEngine;
		arr->usedLength = 0;
	}
}

//this is sort of a private func because it is defined only in c file 
void Array_expand(const size_t expandSize, idArray* arr)
{
	if (arr != NULL)
	{
		if (arr->length == 0)
		{
			arr->length++;
		}
		UINT64* newArray = (UINT64*)ExAllocatePool2(NonPagedPool, (arr->length + expandSize) * sizeof(UINT64), 'idAr');
		if (arr->arrayId != NULL)
		{
			RtlCopyMemory(newArray, arr->arrayId, arr->length * sizeof(UINT64));
			ExFreePool2(arr->arrayId, 'idAr', NULL, 0);
		}
		arr->arrayId = newArray;
		arr->length += expandSize;
	}
}

void Array_addId(const UINT64 id, idArray* arr)
{
	if (arr != NULL)
	{
		if (arr->arrayId == NULL)
		{
			Array_init(1, arr->hEngine, arr);
		}
		if (arr->length == arr->usedLength)
		{
			Array_expand(arr->length, arr);
		}
		arr->arrayId[arr->usedLength] = id;
		arr->usedLength++;
	}
}

void Array_removeId(const UINT64 id, idArray* arr)
{
	if(arr != NULL)
	{
		for (UINT64 curId = 0; curId < arr->length; curId++)
		{
			if (id == arr->arrayId[curId])
			{
				FwpmFilterDeleteById0(arr->hEngine, arr->arrayId[curId]);
				arr->arrayId[curId] = 0;
			}
		}
	}
}

void Array_del(idArray* arr)
{
	if(arr != NULL)
	{
		for (UINT64 id = 0; id < arr->length; id++)
		{
			if (arr->arrayId[id] != 0)
			{
				FwpmFilterDeleteById0(arr->hEngine, arr->arrayId[id]);
			}
		}
		ExFreePool2(arr->arrayId, 'idAr', NULL, 0);
		arr->arrayId = NULL;
		arr->length = 0;
		arr->usedLength = 0;
	}
}
