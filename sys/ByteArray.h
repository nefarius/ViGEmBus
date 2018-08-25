/*
* Virtual Gamepad Emulation Framework - Windows kernel-mode bus driver
* Copyright (C) 2016-2018  Benjamin Höglinger-Stelzer
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


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