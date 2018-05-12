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

#include "trace.h"
#include <ntddk.h>
#include <wdf.h>
#define NTSTRSAFE_LIB
#include <ntstrsafe.h>
#include <ntintsafe.h>
#include <initguid.h>
#include "ViGEmBusDriver.h"
#include "ViGEmBusShared.h"
#include "Queue.h"
#include <usb.h>
#include <usbbusif.h>
#include "Context.h"
#include "Util.h"
#include "UsbPdo.h"
#include "Xusb.h"
#include "Ds4.h"
#include "Xgip.h"


#pragma region Macros

#define MAX_INSTANCE_ID_LEN             80
#define HID_LANGUAGE_ID_LENGTH          0x04

#define HID_REQUEST_GET_REPORT          0x01
#define HID_REQUEST_SET_REPORT          0x09
#define HID_REPORT_TYPE_FEATURE         0x03

#define VIGEM_POOL_TAG                  0x45476956 // "EGiV"
#define DRIVERNAME                      "ViGEm: "
#define MAX_HARDWARE_ID_LENGTH          0xFF

#pragma endregion

#pragma region Helpers

//
// Extracts the HID Report ID from the supplied class request.
//
#define HID_GET_REPORT_ID(_req_) ((_req_->Value) & 0xFF)

//
// Extracts the HID Report type from the supplied class request.
//
#define HID_GET_REPORT_TYPE(_req_) ((_req_->Value >> 8) & 0xFF)

//
// Some insane macro-magic =3
// 
#define P99_PROTECT(...) __VA_ARGS__
#define COPY_BYTE_ARRAY(_dst_, _bytes_)   do {BYTE b[] = _bytes_; \
                                            RtlCopyMemory(_dst_, b, RTL_NUMBER_OF_V1(b)); } while (0)

#pragma endregion


#pragma region WDF callback prototypes

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD Bus_EvtDeviceAdd;
EVT_WDF_DEVICE_FILE_CREATE Bus_DeviceFileCreate;
EVT_WDF_FILE_CLOSE Bus_FileClose;

EVT_WDF_CHILD_LIST_CREATE_DEVICE Bus_EvtDeviceListCreatePdo;

EVT_WDF_CHILD_LIST_IDENTIFICATION_DESCRIPTION_COMPARE Bus_EvtChildListIdentificationDescriptionCompare;

EVT_WDF_DEVICE_PREPARE_HARDWARE Bus_EvtDevicePrepareHardware;

EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL Pdo_EvtIoInternalDeviceControl;

EVT_WDF_TIMER Xgip_SysInitTimerFunc;

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
Bus_CreatePdo(
    _In_ WDFDEVICE Device,
    _In_ PWDFDEVICE_INIT ChildInit,
    _In_ PPDO_IDENTIFICATION_DESCRIPTION Description
);

NTSTATUS
Bus_QueueNotification(
    WDFDEVICE Device,
    ULONG SerialNo,
    WDFREQUEST Request
);

NTSTATUS
Bus_XgipSubmitReport(
    WDFDEVICE Device,
    ULONG SerialNo,
    PXGIP_SUBMIT_REPORT Report,
    _In_ BOOLEAN FromInterface
);

NTSTATUS
Bus_SubmitReport(
    WDFDEVICE Device,
    ULONG SerialNo,
    PVOID Report,
    _In_ BOOLEAN FromInterface
);

WDFDEVICE 
Bus_GetPdo(
    IN WDFDEVICE Device, 
    IN ULONG SerialNo);

VOID
Bus_PdoStageResult(
    _In_ PINTERFACE InterfaceHeader,
    _In_ VIGEM_PDO_STAGE Stage,
    _In_ ULONG Serial,
    _In_ NTSTATUS Status
);

#pragma endregion

