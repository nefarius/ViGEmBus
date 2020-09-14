/*
MIT License

Copyright (c) 2016-2019 Nefarius Software Solutions e.U. and Contributors

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


//
// GUID identifying the bus device. Used by client library to detect and communicate.
// 
// IMPORTANT: make sure to change this value if you fork it or introduce 
//				breaking changes!
// 
// {96E42B22-F5E9-42F8-B043-ED0F932F014F}
DEFINE_GUID(GUID_DEVINTERFACE_BUSENUM_VIGEM,
    0x96E42B22, 0xF5E9, 0x42F8, 0xB0, 0x43, 0xED, 0x0F, 0x93, 0x2F, 0x01, 0x4F);

#pragma once

#include "ViGEm/Common.h"

//
// Common version for user-mode library and driver compatibility
// 
// On initialization, the user-mode library has this number embedded
// and sends it to the bus on its enumeration. The bus compares this
// number to the one it was compiled with. If they match, the bus
// access is permitted and success reported. If they mismatch, an
// error is reported and the user-mode library skips this instance.
// 
#define VIGEM_COMMON_VERSION            0x0001

#define FILE_DEVICE_BUSENUM             FILE_DEVICE_BUS_EXTENDER
#define BUSENUM_IOCTL(_index_)          CTL_CODE(FILE_DEVICE_BUSENUM, _index_, METHOD_BUFFERED, FILE_READ_DATA)
#define BUSENUM_W_IOCTL(_index_)        CTL_CODE(FILE_DEVICE_BUSENUM, _index_, METHOD_BUFFERED, FILE_WRITE_DATA)
#define BUSENUM_R_IOCTL(_index_)        CTL_CODE(FILE_DEVICE_BUSENUM, _index_, METHOD_BUFFERED, FILE_READ_DATA)
#define BUSENUM_RW_IOCTL(_index_)       CTL_CODE(FILE_DEVICE_BUSENUM, _index_, METHOD_BUFFERED, FILE_WRITE_DATA | FILE_READ_DATA)

#define IOCTL_VIGEM_BASE 0x801

// 
// IO control codes
// 
#define IOCTL_VIGEM_PLUGIN_TARGET       BUSENUM_W_IOCTL (IOCTL_VIGEM_BASE + 0x000)
#define IOCTL_VIGEM_UNPLUG_TARGET       BUSENUM_W_IOCTL (IOCTL_VIGEM_BASE + 0x001)
#define IOCTL_VIGEM_CHECK_VERSION       BUSENUM_W_IOCTL (IOCTL_VIGEM_BASE + 0x002)
#define IOCTL_VIGEM_WAIT_DEVICE_READY   BUSENUM_W_IOCTL (IOCTL_VIGEM_BASE + 0x003)

#define IOCTL_XUSB_REQUEST_NOTIFICATION BUSENUM_RW_IOCTL(IOCTL_VIGEM_BASE + 0x200)
#define IOCTL_XUSB_SUBMIT_REPORT        BUSENUM_W_IOCTL (IOCTL_VIGEM_BASE + 0x201)
#define IOCTL_DS4_SUBMIT_REPORT         BUSENUM_W_IOCTL (IOCTL_VIGEM_BASE + 0x202)
#define IOCTL_DS4_REQUEST_NOTIFICATION  BUSENUM_W_IOCTL (IOCTL_VIGEM_BASE + 0x203)
//#define IOCTL_XGIP_SUBMIT_REPORT        BUSENUM_W_IOCTL (IOCTL_VIGEM_BASE + 0x204)
//#define IOCTL_XGIP_SUBMIT_INTERRUPT     BUSENUM_W_IOCTL (IOCTL_VIGEM_BASE + 0x205)
#define IOCTL_XUSB_GET_USER_INDEX       BUSENUM_RW_IOCTL(IOCTL_VIGEM_BASE + 0x206)


//
//  Data structure used in PlugIn and UnPlug ioctls
//

#pragma region Plugin

//
// Data structure used in IOCTL_VIGEM_PLUGIN_TARGET requests.
// 
typedef struct _VIGEM_PLUGIN_TARGET
{
    //
    // sizeof (struct _BUSENUM_HARDWARE)
    //
    IN ULONG Size;

    //
    // Serial number of target device.
    // 
    IN ULONG SerialNo;

    // 
    // Type of the target device to emulate.
    // 
    VIGEM_TARGET_TYPE TargetType;

    //
    // If set, the vendor ID the emulated device is reporting
    // 
    USHORT VendorId;

    //
    // If set, the product ID the emulated device is reporting
    // 
    USHORT ProductId;

} VIGEM_PLUGIN_TARGET, *PVIGEM_PLUGIN_TARGET;

//
// Initializes a VIGEM_PLUGIN_TARGET structure.
// 
VOID FORCEINLINE VIGEM_PLUGIN_TARGET_INIT(
    _Out_ PVIGEM_PLUGIN_TARGET PlugIn,
          _In_ ULONG SerialNo,
          _In_ VIGEM_TARGET_TYPE TargetType
)
{
    RtlZeroMemory(PlugIn, sizeof(VIGEM_PLUGIN_TARGET));

    PlugIn->Size = sizeof(VIGEM_PLUGIN_TARGET);
    PlugIn->SerialNo = SerialNo;
    PlugIn->TargetType = TargetType;
}

#pragma endregion 

#pragma region Unplug

//
// Data structure used in IOCTL_VIGEM_UNPLUG_TARGET requests.
// 
typedef struct _VIGEM_UNPLUG_TARGET
{
    //
    // sizeof (struct _REMOVE_HARDWARE)
    //
    IN ULONG Size;

    //
    // Serial number of target device.
    // 
    ULONG SerialNo;

} VIGEM_UNPLUG_TARGET, *PVIGEM_UNPLUG_TARGET;

//
// Initializes a VIGEM_UNPLUG_TARGET structure.
// 
VOID FORCEINLINE VIGEM_UNPLUG_TARGET_INIT(
    _Out_ PVIGEM_UNPLUG_TARGET UnPlug,
          _In_ ULONG SerialNo
)
{
    RtlZeroMemory(UnPlug, sizeof(VIGEM_UNPLUG_TARGET));

    UnPlug->Size = sizeof(VIGEM_UNPLUG_TARGET);
    UnPlug->SerialNo = SerialNo;
}

#pragma endregion

#pragma region Check version

typedef struct _VIGEM_CHECK_VERSION
{
    IN ULONG Size;

    IN ULONG Version;

} VIGEM_CHECK_VERSION, *PVIGEM_CHECK_VERSION;

VOID FORCEINLINE VIGEM_CHECK_VERSION_INIT(
    _Out_ PVIGEM_CHECK_VERSION CheckVersion,
    _In_ ULONG Version
)
{
    RtlZeroMemory(CheckVersion, sizeof(VIGEM_CHECK_VERSION));

    CheckVersion->Size = sizeof(VIGEM_CHECK_VERSION);
    CheckVersion->Version = Version;
}

#pragma endregion

#pragma region Wait device ready

typedef struct _VIGEM_WAIT_DEVICE_READY
{
    IN ULONG Size;

    IN ULONG SerialNo;

} VIGEM_WAIT_DEVICE_READY, * PVIGEM_WAIT_DEVICE_READY;

VOID FORCEINLINE VIGEM_WAIT_DEVICE_READY_INIT(
    _Out_ PVIGEM_WAIT_DEVICE_READY WaitReady,
    _In_ ULONG SerialNo
)
{
    RtlZeroMemory(WaitReady, sizeof(VIGEM_WAIT_DEVICE_READY));

    WaitReady->Size = sizeof(VIGEM_WAIT_DEVICE_READY);
    WaitReady->SerialNo = SerialNo;
}

#pragma endregion 

#pragma region XUSB (aka Xbox 360 device) section

//
// Data structure used in IOCTL_XUSB_REQUEST_NOTIFICATION requests.
// 
typedef struct _XUSB_REQUEST_NOTIFICATION
{
    //
    // sizeof(struct _XUSB_REQUEST_NOTIFICATION)
    // 
    ULONG Size;

    //
    // Serial number of target device.
    // 
    ULONG SerialNo;

    //
    // Vibration intensity value of the large motor (0-255).
    // 
    UCHAR LargeMotor;

    //
    // Vibration intensity value of the small motor (0-255).
    // 
    UCHAR SmallMotor;

    //
    // Index number of the slot/LED that XUSB.sys has assigned.
    // 
    UCHAR LedNumber;

} XUSB_REQUEST_NOTIFICATION, *PXUSB_REQUEST_NOTIFICATION;

//
// Initializes a XUSB_REQUEST_NOTIFICATION structure.
// 
VOID FORCEINLINE XUSB_REQUEST_NOTIFICATION_INIT(
    _Out_ PXUSB_REQUEST_NOTIFICATION Request,
          _In_ ULONG SerialNo
)
{
    RtlZeroMemory(Request, sizeof(XUSB_REQUEST_NOTIFICATION));

    Request->Size = sizeof(XUSB_REQUEST_NOTIFICATION);
    Request->SerialNo = SerialNo;
}

//
// Data structure used in IOCTL_XUSB_SUBMIT_REPORT requests.
// 
typedef struct _XUSB_SUBMIT_REPORT
{
    //
    // sizeof(struct _XUSB_SUBMIT_REPORT)
    // 
    ULONG Size;

    //
    // Serial number of target device.
    // 
    ULONG SerialNo;

    //
    // Report to submit to the target device.
    // 
    XUSB_REPORT Report;

} XUSB_SUBMIT_REPORT, *PXUSB_SUBMIT_REPORT;

//
// Initializes an XUSB report.
// 
VOID FORCEINLINE XUSB_SUBMIT_REPORT_INIT(
    _Out_ PXUSB_SUBMIT_REPORT Report,
    _In_ ULONG SerialNo
)
{
    RtlZeroMemory(Report, sizeof(XUSB_SUBMIT_REPORT));

    Report->Size = sizeof(XUSB_SUBMIT_REPORT);
    Report->SerialNo = SerialNo;
}

typedef struct _XUSB_GET_USER_INDEX
{
    //
    // sizeof(struct _XUSB_GET_USER_INDEX)
    // 
    ULONG Size;

    //
    // Serial number of target device.
    // 
    ULONG SerialNo;

    //
    // User index of target device.
    // 
    OUT ULONG UserIndex;

} XUSB_GET_USER_INDEX, *PXUSB_GET_USER_INDEX;

//
// Initializes XUSB_GET_USER_INDEX structure.
// 
VOID FORCEINLINE XUSB_GET_USER_INDEX_INIT(
    _Out_ PXUSB_GET_USER_INDEX GetRequest,
    _In_ ULONG SerialNo
)
{
    RtlZeroMemory(GetRequest, sizeof(XUSB_GET_USER_INDEX));

    GetRequest->Size = sizeof(XUSB_GET_USER_INDEX);
    GetRequest->SerialNo = SerialNo;
}

#pragma endregion

#pragma region DualShock 4 section

typedef struct _DS4_OUTPUT_REPORT
{
    //
    // Vibration intensity value of the small motor (0-255).
    // 
    UCHAR SmallMotor;

    //
    // Vibration intensity value of the large motor (0-255).
    // 
    UCHAR LargeMotor;

    //
    // Color values of the Lightbar.
    //
    DS4_LIGHTBAR_COLOR LightbarColor;

} DS4_OUTPUT_REPORT, *PDS4_OUTPUT_REPORT;

//
// Data structure used in IOCTL_DS4_REQUEST_NOTIFICATION requests.
// 
typedef struct _DS4_REQUEST_NOTIFICATION
{
    //
    // sizeof(struct _XUSB_REQUEST_NOTIFICATION)
    // 
    ULONG Size;

    //
    // Serial number of target device.
    // 
    ULONG SerialNo;

    //
    // The HID output report
    // 
    DS4_OUTPUT_REPORT Report;

} DS4_REQUEST_NOTIFICATION, *PDS4_REQUEST_NOTIFICATION;

//
// Initializes a DS4_REQUEST_NOTIFICATION structure.
// 
VOID FORCEINLINE DS4_REQUEST_NOTIFICATION_INIT(
    _Out_ PDS4_REQUEST_NOTIFICATION Request,
    _In_ ULONG SerialNo
)
{
    RtlZeroMemory(Request, sizeof(DS4_REQUEST_NOTIFICATION));

    Request->Size = sizeof(DS4_REQUEST_NOTIFICATION);
    Request->SerialNo = SerialNo;
}

//
// DualShock 4 request data
// 
typedef struct _DS4_SUBMIT_REPORT
{
    //
    // sizeof(struct _DS4_SUBMIT_REPORT)
    // 
    ULONG Size;

    //
    // Serial number of target device.
    // 
    ULONG SerialNo;

    //
    // HID Input report
    // 
    DS4_REPORT Report;

} DS4_SUBMIT_REPORT, *PDS4_SUBMIT_REPORT;

//
// Initializes a DualShock 4 report.
// 
VOID FORCEINLINE DS4_SUBMIT_REPORT_INIT(
    _Out_ PDS4_SUBMIT_REPORT Report,
    _In_ ULONG SerialNo
)
{
    RtlZeroMemory(Report, sizeof(DS4_SUBMIT_REPORT));

    Report->Size = sizeof(DS4_SUBMIT_REPORT);
    Report->SerialNo = SerialNo;

    DS4_REPORT_INIT(&Report->Report);
}

#include <pshpack1.h>

//
// DualShock 4 extended report request
// 
typedef struct _DS4_SUBMIT_REPORT_EX
{
    //
     // sizeof(struct _DS4_SUBMIT_REPORT_EX)
     // 
    _In_ ULONG Size;

    //
    // Serial number of target device.
    // 
    _In_ ULONG SerialNo;

    //
    // Full size HID report excluding fixed Report ID.
    // 
    _In_ DS4_REPORT_EX Report;

} DS4_SUBMIT_REPORT_EX, * PDS4_SUBMIT_REPORT_EX;

#include <poppack.h>

//
// Initializes a DualShock 4 extended report.
// 
VOID FORCEINLINE DS4_SUBMIT_REPORT_EX_INIT(
    _Out_ PDS4_SUBMIT_REPORT_EX Report,
    _In_ ULONG SerialNo
)
{
    RtlZeroMemory(Report, sizeof(DS4_SUBMIT_REPORT_EX));

    Report->Size = sizeof(DS4_SUBMIT_REPORT_EX);
    Report->SerialNo = SerialNo;
}

#pragma endregion
