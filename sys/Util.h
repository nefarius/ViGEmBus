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


#pragma once

//
// Returns the current caller process id.
// 
#define CURRENT_PROCESS_ID() ((DWORD)((DWORD_PTR)PsGetCurrentProcessId() & 0xFFFFFFFF))

#define IS_OWNER(_pdo_) (_pdo_->OwnerProcessId == CURRENT_PROCESS_ID())

//
// Represents a MAC address.
//
typedef struct _MAC_ADDRESS
{
    UCHAR Vendor0;
    UCHAR Vendor1;
    UCHAR Vendor2;
    UCHAR Nic0;
    UCHAR Nic1;
    UCHAR Nic2;
} MAC_ADDRESS, *PMAC_ADDRESS;


VOID ReverseByteArray(PUCHAR Array, INT Length);
VOID GenerateRandomMacAddress(PMAC_ADDRESS Address);
