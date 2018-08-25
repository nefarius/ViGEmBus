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


#include <ntifs.h>
#include "busenum.h"
#include "util.tmh"


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
