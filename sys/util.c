/*
MIT License

Copyright (c) 2016 Benjamin "Nefarius" Höglinger

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#include <ntifs.h>
#include "busenum.h"


VOID ReverseByteArray(PUCHAR Array, INT Length)
{
    PUCHAR s = (PUCHAR)ExAllocatePoolWithTag(NonPagedPool, sizeof(UCHAR) * Length, VIGEM_POOL_TAG);
    INT c, d;

    if (s == NULL)
        return;

    for (c = Length - 1, d = 0; c >= 0; c--, d++)
        *(s + d) = *(Array + c);

    for (c = 0; c < Length; c++)
        *(Array + c) = *(s + c);

    ExFreePoolWithTag(s, VIGEM_POOL_TAG);
}

VOID GenerateRandomMacAddress(PMAC_ADDRESS Address)
{
    // Vendor "C0:13:37"
    Address->Vendor0 = 0xC0;
    Address->Vendor1 = 0x13;
    Address->Vendor2 = 0x37;

    ULONG seed = KeQueryPerformanceCounter(NULL).LowPart;
    
    Address->Nic0 = RtlRandomEx(&seed) % 0xFF;
    Address->Nic1 = RtlRandomEx(&seed) % 0xFF;
    Address->Nic2 = RtlRandomEx(&seed) % 0xFF;
}
