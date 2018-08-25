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
    ViGEmPdoInitFinished

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

