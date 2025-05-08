#pragma once
#include "ntdef.h"
#include "basetsd.h"

typedef struct idNode
{
	UINT64 id;
	struct idNode* next;
} ID_NODE;

extern ID_NODE idList;

int List_addId(const UINT64 id);
void List_removeId(const UINT64 id, HANDLE hEngine); //if the remove does not succeed it will be cleaned up at the end of the program
void List_del(HANDLE hEngine);