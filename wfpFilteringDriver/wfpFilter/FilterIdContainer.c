#include "FilterIdContainer.h"
#include <ntddk.h>
#include "ndis/nbl.h"
#include <fwpsk.h>
#include <fwpmu.h>

typedef struct idNode
{
	UINT64 id;
	char name[FILTER_NAME_SIZE + 1]; // +1 for \0
	struct idNode* next;
} ID_NODE;

//just head - without any data(avoid allocation for the first one to prevent null ptr access if it got deleted and someone use it)
//have to declar it here to make it "private"
static ID_NODE idList = 
{
	.id = 0,
	.next = NULL
};

int List_addId(const UINT64 id, const char* name)
{
	ID_NODE* cur = &idList;
	while (cur->next != NULL)
	{
		cur = cur->next;
	}
	ID_NODE* newId = (ID_NODE*)ExAllocatePoolWithTag(POOL_FLAG_NON_PAGED, sizeof(ID_NODE), 'idNd');
	if (newId == NULL)
	{
		return - 1;
	}
	newId->id = id;
	strncpy(newId->name, name, FILTER_NAME_SIZE - 1);
	newId->name[FILTER_NAME_SIZE - 1] = '\0';
	cur->next = newId;
	return 0;
}

void List_removeId(const char* name, const HANDLE hEngine)
{
	ID_NODE* cur = &idList;
	while (cur->next != NULL && strcmp(cur->next->name, name) != 0)
	{
		cur = cur->next;
	}

	if (cur->next == NULL)
	{
		return;
	}

	ID_NODE* toDelete = cur->next;
	cur->next = toDelete->next;

	FwpmFilterDeleteById0(hEngine, toDelete->id);
	ExFreePoolWithTag(toDelete, 'idNd');
}


void List_del(const HANDLE hEngine)
{
	ID_NODE* cur = idList.next;
	ID_NODE* nextNode = NULL;

	if (idList.id != 0)
	{
		FwpmFilterDeleteById0(hEngine, idList.id);
	}

	while (cur != NULL)
	{
		nextNode = cur->next;

		FwpmFilterDeleteById0(hEngine, cur->id);
		ExFreePoolWithTag(cur, 'idNd');

		cur = nextNode;
	}
}
