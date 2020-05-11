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

#include "trace.h"
#include <ntddk.h>
#include <wdf.h>
#define NTSTRSAFE_LIB
#include <ntstrsafe.h>
#include <ntintsafe.h>
#include <initguid.h>
#include "ViGEmBusDriver.h"
#include <ViGEm/km/BusShared.h>
#include "Queue.h"
#include <usb.h>
#include <usbbusif.h>
#include "Context.h"
#include "Util.h"



#pragma region Macros

#define HID_LANGUAGE_ID_LENGTH          0x04



#define VIGEM_POOL_TAG                  0x45476956 // "EGiV"
#define DRIVERNAME                      "ViGEm: "

#define ORC_PC_FREQUENCY_DIVIDER        1000
#define ORC_TIMER_START_DELAY           500 // ms
#define ORC_TIMER_PERIODIC_DUE_TIME     500 // ms
#define ORC_REQUEST_MAX_AGE             500 // ms

#pragma endregion



EXTERN_C_START

#pragma region WDF callback prototypes

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD Bus_EvtDeviceAdd;
EVT_WDF_DEVICE_FILE_CREATE Bus_DeviceFileCreate;
EVT_WDF_FILE_CLOSE Bus_FileClose;

EVT_WDF_CHILD_LIST_CREATE_DEVICE Bus_EvtDeviceListCreatePdo;

EVT_WDF_CHILD_LIST_IDENTIFICATION_DESCRIPTION_COMPARE Bus_EvtChildListIdentificationDescriptionCompare;

EVT_WDF_DEVICE_PREPARE_HARDWARE Pdo_EvtDevicePrepareHardware;

EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL Pdo_EvtIoInternalDeviceControl;

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

NTSTATUS
Bus_QueueNotification(
    WDFDEVICE Device,
    ULONG SerialNo,
    WDFREQUEST Request
);

NTSTATUS
Bus_SubmitReport(
    WDFDEVICE Device,
    ULONG SerialNo,
    PVOID Report,
    _In_ BOOLEAN FromInterface
);

VOID
Bus_PdoStageResult(
    _In_ PINTERFACE InterfaceHeader,
    _In_ VIGEM_PDO_STAGE Stage,
    _In_ ULONG Serial,
    _In_ NTSTATUS Status
);

#pragma endregion

EXTERN_C_END
