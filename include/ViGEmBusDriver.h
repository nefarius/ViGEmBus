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


// {A77BC4D5-6AF7-4E69-8DC4-6B88A6028CE6}
// ReSharper disable once CppMissingIncludeGuard
DEFINE_GUID(GUID_VIGEM_INTERFACE_PDO,
    0xA77BC4D5, 0x6AF7, 0x4E69, 0x8D, 0xC4, 0x6B, 0x88, 0xA6, 0x02, 0x8C, 0xE6);

// {A8BA2D1F-894F-464A-B0CE-7A0C8FD65DF1}
DEFINE_GUID(GUID_DEVCLASS_VIGEM_RAWPDO,
    0xA8BA2D1F, 0x894F, 0x464A, 0xB0, 0xCE, 0x7A, 0x0C, 0x8F, 0xD6, 0x5D, 0xF1);

#pragma once


//
// Describes the current stage a PDO completed
// 
typedef enum _VIGEM_PDO_STAGE
{
    ViGEmPdoCreate,
    ViGEmPdoPrepareHardware,
    ViGEmPdoInternalIoControl

} VIGEM_PDO_STAGE, *PVIGEM_PDO_STAGE;

//
// PDO stage result callback definition
// 
typedef
VOID
(*PVIGEM_BUS_PDO_STAGE_RESULT)(
    _In_ PINTERFACE InterfaceHeader,
    _In_ VIGEM_PDO_STAGE Stage,
    _In_ ULONG Serial,
    _In_ NTSTATUS Status
    );

typedef struct _VIGEM_BUS_INTERFACE {
    // 
    // Standard interface header, must be present
    // 
    INTERFACE InterfaceHeader;

    //
    // PDO stage result callback
    // 
    PVIGEM_BUS_PDO_STAGE_RESULT BusPdoStageResult;

} VIGEM_BUS_INTERFACE, *PVIGEM_BUS_INTERFACE;

#define VIGEM_BUS_INTERFACE_VERSION      1

VOID FORCEINLINE BUS_PDO_REPORT_STAGE_RESULT(
    VIGEM_BUS_INTERFACE Interface, 
    VIGEM_PDO_STAGE Stage,
    ULONG Serial,
    NTSTATUS Status
)
{
    (*Interface.BusPdoStageResult)(&Interface.InterfaceHeader, Stage, Serial, Status);
}

