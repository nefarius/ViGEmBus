#pragma once

#include <ntifs.h>

#define INITIAL_ARRAY_CAPACITY PAGE_SIZE
#define ARRAY_POOL_TAG	'arrA'

typedef struct _BYTE_ARRAY
{
    UCHAR* Data;		//> array of data we're storing
    ULONG_PTR Size;		//> slots used so far
    ULONG_PTR Capacity;	//> total available memory
} BYTE_ARRAY, *PBYTE_ARRAY;

NTSTATUS InitByteArray(IN OUT PBYTE_ARRAY Array);

NTSTATUS AppendElementByteArray(IN PBYTE_ARRAY Array, IN PVOID Element);

NTSTATUS AppendElementsByteArray(IN PBYTE_ARRAY Array, IN PVOID Elements, IN ULONG NumElements);

NTSTATUS GetElementByteArray(IN PBYTE_ARRAY Array, IN ULONG Index, OUT PVOID Element);

NTSTATUS GetElementsByteArray(IN PBYTE_ARRAY Array, IN ULONG Index, OUT PVOID Elements, IN ULONG NumElements);

NTSTATUS SetElementByteArray(IN PBYTE_ARRAY Array, IN ULONG Index, IN PVOID Element);

NTSTATUS SetElementsByteArray(IN PBYTE_ARRAY Array, IN ULONG Index, IN PVOID Elements, IN ULONG NumElements);

NTSTATUS FreeByteArray(IN PBYTE_ARRAY Array);