#include "FilterIdContainer.h"
#include <ntddk.h>
#include "ndis/nbl.h"
#include <fwpsk.h>
#include <fwpmu.h>

ID_NODE idList =
{
	.id = 0,
	.next = NULL
};

int List_addId(const UINT64 id)
{
	if (idList.id == 0)
	{
		idList.id = id;
		return;
	}
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
	cur->next = newId;
	return 0;
}

void List_removeId(const UINT64 id, HANDLE hEngine)
{
	ID_NODE* cur = &idList;
	while (cur->next != NULL && cur->next->id == id)
	{
		cur = cur->next;
	}
	if (cur->id != id && cur->next == NULL)
	{
		return;
	}
	ID_NODE* nextId = NULL;
	if (cur->next->next != NULL)
	{
		nextId = cur->next->next;
	}

	FwpmFilterDeleteById0(hEngine, cur->next->id);
	ExFreePoolWithTag(cur->next, 'idNd');
	cur->next = nextId;
}

void List_del(HANDLE hEngine)
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
