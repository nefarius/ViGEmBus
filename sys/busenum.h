/*
* Virtual Gamepad Emulation Framework - Windows kernel-mode bus driver
*
* MIT License
*
* Copyright (c) 2016-2020 Nefarius Software Solutions e.U. and Contributors
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

#include "trace.h"
#include <ntddk.h>
#include <wdf.h>
#define NTSTRSAFE_LIB
#include <ntstrsafe.h>
#include <ntintsafe.h>
#include <initguid.h>
#include "ViGEmBusDriver.h"
#include <ViGEm/km/BusShared.h>
#include "Queue.hpp"
#include <usb.h>
#include <usbbusif.h>
#include "Util.h"



#pragma region Macros

#define VIGEM_POOL_TAG                  0x45476956 // "EGiV"
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
