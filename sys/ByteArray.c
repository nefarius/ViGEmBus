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


#include "ByteArray.h"
#include "trace.h"
#include "bytearray.tmh"

//
// Helpers
//

ULONG_PTR align_to_page_size(ULONG_PTR val)
{
    return (val + (PAGE_SIZE - 1)) & -PAGE_SIZE;
}


//
// Forward declarations
//

NTSTATUS IncreaseCapacityByteArray(IN PBYTE_ARRAY Array, IN ULONG NumElements);


//
// Implementation
//

NTSTATUS InitByteArray(IN OUT PBYTE_ARRAY Array)
{
    //
    // Initialize size and default capacity
    Array->Size = 0;
    Array->Capacity = INITIAL_ARRAY_CAPACITY;

    //
    // Allocate memory
    Array->Data = (UCHAR*)ExAllocatePoolWithTag(PagedPool, Array->Capacity, ARRAY_POOL_TAG);
    if (Array->Data == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    return STATUS_SUCCESS;
}

NTSTATUS AppendElementByteArray(IN PBYTE_ARRAY Array, IN PVOID Element)
{
    //
    // Make sure there is room to expand into
    if (((Array->Size + 1) * sizeof(UCHAR)) > Array->Capacity)
    {
        //
        // Increase capacity
        NTSTATUS status = IncreaseCapacityByteArray(Array, sizeof(UCHAR));
        if (!NT_SUCCESS(status))
            return status;
    }

    //
    // Append the element and increment the size
    RtlCopyMemory(Array->Data + (Array->Size * sizeof(UCHAR)), Element, sizeof(UCHAR));

    //
    // Increment size
    Array->Size += 1;

    return STATUS_SUCCESS;
}

NTSTATUS AppendElementsByteArray(IN PBYTE_ARRAY Array, IN PVOID Elements, IN ULONG NumElements)
{
    //
    // Make sure there is room to expand into
    if ((Array->Size + NumElements) * sizeof(UCHAR) > Array->Capacity)
    {
        //
        // Increase capacity
        NTSTATUS status = IncreaseCapacityByteArray(Array, NumElements);
        if (!NT_SUCCESS(status))
            return status;
    }

    //
    // Append the elements and increase the size
    RtlCopyMemory(Array->Data + (Array->Size * sizeof(UCHAR)), Elements, NumElements * sizeof(UCHAR));

    //
    // Increase size
    Array->Size += NumElements;

    return STATUS_SUCCESS;
}

NTSTATUS IncreaseCapacityByteArray(IN PBYTE_ARRAY Array, IN ULONG NumElements)
{
    UCHAR* NewData = NULL;

    //
    // Align new size to the immediate next page boundary
    Array->Capacity = align_to_page_size((Array->Size + NumElements) * sizeof(UCHAR));

    //
    // Allocate new data with new capacity
    NewData = (UCHAR*)ExAllocatePoolWithTag(PagedPool, Array->Capacity, ARRAY_POOL_TAG);
    if (NewData == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Copy old data over
    RtlCopyMemory(NewData, Array->Data, Array->Size * sizeof(UCHAR));

    //
    // Free old data
    ExFreePoolWithTag(Array->Data, ARRAY_POOL_TAG);

    //
    // Set data pointer to new allocation
    Array->Data = NewData;

    return STATUS_SUCCESS;
}

NTSTATUS GetElementByteArray(IN PBYTE_ARRAY Array, IN ULONG Index, OUT PVOID Element)
{
    //
    // Check array bounds
    if (Index >= Array->Size || (LONG)Index < 0)
        return STATUS_ARRAY_BOUNDS_EXCEEDED;

    //
    // Copy data over
    RtlCopyMemory(Element, Array->Data + (Index * sizeof(UCHAR)), sizeof(UCHAR));

    return STATUS_SUCCESS;
}

NTSTATUS GetElementsByteArray(IN PBYTE_ARRAY Array, IN ULONG Index, OUT PVOID Elements, IN ULONG NumElements)
{
    //
    // Check array bounds
    if (Index >= Array->Size || (LONG)Index < 0)
        return STATUS_ARRAY_BOUNDS_EXCEEDED;

    //
    // Copy data over
    RtlCopyMemory(Elements, Array->Data + (Index * sizeof(UCHAR)), NumElements * sizeof(UCHAR));

    return STATUS_SUCCESS;
}

NTSTATUS SetElementByteArray(IN PBYTE_ARRAY Array, IN ULONG Index, IN PVOID Element)
{
    //
    // Check array bounds
    if (Index >= Array->Size || (LONG)Index < 0)
        return STATUS_ARRAY_BOUNDS_EXCEEDED;

    //
    // Copy data over
    RtlCopyMemory(Array->Data + (Index * sizeof(UCHAR)), Element, sizeof(UCHAR));

    return STATUS_SUCCESS;
}

NTSTATUS SetElementsByteArray(IN PBYTE_ARRAY Array, IN ULONG Index, IN PVOID Elements, IN ULONG NumElements)
{
    //
    // Check array bounds
    if (Index >= Array->Size || (LONG)Index < 0)
        return STATUS_ARRAY_BOUNDS_EXCEEDED;

    //
    // Copy data over
    RtlCopyMemory(Array->Data + (Index * sizeof(UCHAR)), Elements, NumElements * sizeof(UCHAR));

    return STATUS_SUCCESS;
}

NTSTATUS FreeByteArray(IN PBYTE_ARRAY Array)
{
    if (Array->Data == NULL)
        return STATUS_MEMORY_NOT_ALLOCATED;

    //
    // Free data
    ExFreePoolWithTag(Array->Data, ARRAY_POOL_TAG);

    //
    // Null out everything
    Array->Data = NULL;
    Array->Size = 0;
    Array->Capacity = 0;

    return STATUS_SUCCESS;
}
