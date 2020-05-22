/*
* Virtual Gamepad Emulation Framework - Windows kernel-mode bus driver
*
* BSD 3-Clause License
*
* Copyright (c) 2018-2020, Nefarius Software Solutions e.U. and Contributors
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice, this
*    list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
* 3. Neither the name of the copyright holder nor the names of its
*    contributors may be used to endorse or promote products derived from
*    this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#pragma once

#include "trace.h"
#include <ntddk.h>
#include <wdf.h>
#define NTSTRSAFE_LIB
#include <ntstrsafe.h>
#include <initguid.h>
#include <ViGEm/km/BusShared.h>
#include "Queue.hpp"



#pragma region Macros

#define DRIVERNAME                      "ViGEm: "

#pragma endregion

//
// FDO (bus device) context data
// 
typedef struct _FDO_DEVICE_DATA
{
    //
    // Counter of interface references
    // 
    LONG InterfaceReferenceCounter;

    //
    // Next SessionId to assign to a file handle
    // 
    LONG NextSessionId;

} FDO_DEVICE_DATA, * PFDO_DEVICE_DATA;

#define FDO_FIRST_SESSION_ID 100

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FDO_DEVICE_DATA, FdoGetData)

// 
// Context data associated with file objects created by user mode applications
// 
typedef struct _FDO_FILE_DATA
{
    //
    // SessionId associated with file handle.  Used to map file handles to emulated gamepad devices
    // 
    LONG SessionId;

} FDO_FILE_DATA, * PFDO_FILE_DATA;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FDO_FILE_DATA, FileObjectGetData)


EXTERN_C_START

#pragma region WDF callback prototypes

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD Bus_EvtDeviceAdd;

EVT_WDF_DEVICE_FILE_CREATE Bus_DeviceFileCreate;

EVT_WDF_FILE_CLOSE Bus_FileClose;

EVT_WDF_CHILD_LIST_CREATE_DEVICE Bus_EvtDeviceListCreatePdo;

EVT_WDF_CHILD_LIST_IDENTIFICATION_DESCRIPTION_COMPARE Bus_EvtChildListIdentificationDescriptionCompare;

EVT_WDF_OBJECT_CONTEXT_CLEANUP Bus_EvtDriverContextCleanup;

#pragma endregion

#pragma region Bus enumeration-specific functions

NTSTATUS
Bus_PlugInDevice(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_ BOOLEAN IsInternal,
    _Out_ size_t* Transferred
);

NTSTATUS
Bus_UnPlugDevice(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_ BOOLEAN IsInternal,
    _Out_ size_t* Transferred
);

#pragma endregion

EXTERN_C_END
