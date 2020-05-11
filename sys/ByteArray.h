/*
* Virtual Gamepad Emulation Framework - Windows kernel-mode bus driver
*
* MIT License
*
* Copyright (c) 2016-2019 Nefarius Software Solutions e.U. and Contributors
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/


#pragma once

#include <ntifs.h>

EXTERN_C_START

#define INITIAL_ARRAY_CAPACITY PAGE_SIZE
#define ARRAY_POOL_TAG    'arrA'

typedef struct _BYTE_ARRAY
{
    UCHAR* Data;        //> array of data we're storing
    ULONG_PTR Size;        //> slots used so far
    ULONG_PTR Capacity;    //> total available memory
} BYTE_ARRAY, *PBYTE_ARRAY;

NTSTATUS InitByteArray(IN OUT PBYTE_ARRAY Array);

NTSTATUS AppendElementByteArray(IN PBYTE_ARRAY Array, IN PVOID Element);

NTSTATUS AppendElementsByteArray(IN PBYTE_ARRAY Array, IN PVOID Elements, IN ULONG NumElements);

NTSTATUS GetElementByteArray(IN PBYTE_ARRAY Array, IN ULONG Index, OUT PVOID Element);

NTSTATUS GetElementsByteArray(IN PBYTE_ARRAY Array, IN ULONG Index, OUT PVOID Elements, IN ULONG NumElements);

NTSTATUS SetElementByteArray(IN PBYTE_ARRAY Array, IN ULONG Index, IN PVOID Element);

NTSTATUS SetElementsByteArray(IN PBYTE_ARRAY Array, IN ULONG Index, IN PVOID Elements, IN ULONG NumElements);

NTSTATUS FreeByteArray(IN PBYTE_ARRAY Array);

EXTERN_C_END
