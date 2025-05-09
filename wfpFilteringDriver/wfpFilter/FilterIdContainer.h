#pragma once
#include "ntdef.h"
#include "basetsd.h"

#define FILTER_NAME_SIZE 10

int List_addId(const UINT64 id, const char* name);
void List_removeId(const char* name, const HANDLE hEngine); //if the remove does not succeed it will be cleaned up at the end of the program
void List_del(const HANDLE hEngine);