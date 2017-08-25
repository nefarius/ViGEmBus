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


// {96E42B22-F5E9-42F8-B043-ED0F932F014F}
// ReSharper disable once CppMissingIncludeGuard
DEFINE_GUID(GUID_DEVINTERFACE_BUSENUM_VIGEM,
    0x96E42B22, 0xF5E9, 0x42F8, 0xB0, 0x43, 0xED, 0x0F, 0x93, 0x2F, 0x01, 0x4F);

// {02098E80-3C81-4CF0-A2C5-CD4D3B318902}
DEFINE_GUID(GUID_DEVINTERFACE_VIGEM,
    0x02098E80, 0x3C81, 0x4CF0, 0xA2, 0xC5, 0xCD, 0x4D, 0x3B, 0x31, 0x89, 0x02);

// {A8BA2D1F-894F-464A-B0CE-7A0C8FD65DF1}
DEFINE_GUID(GUID_DEVCLASS_VIGEM_RAWPDO,
    0xA8BA2D1F, 0x894F, 0x464A, 0xB0, 0xCE, 0x7A, 0x0C, 0x8F, 0xD6, 0x5D, 0xF1);

#pragma once

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

#define IOCTL_XUSB_REQUEST_NOTIFICATION BUSENUM_RW_IOCTL(IOCTL_VIGEM_BASE + 0x200)
#define IOCTL_XUSB_SUBMIT_REPORT        BUSENUM_W_IOCTL (IOCTL_VIGEM_BASE + 0x201)
#define IOCTL_DS4_SUBMIT_REPORT         BUSENUM_W_IOCTL (IOCTL_VIGEM_BASE + 0x202)
#define IOCTL_DS4_REQUEST_NOTIFICATION  BUSENUM_W_IOCTL (IOCTL_VIGEM_BASE + 0x203)
#define IOCTL_XGIP_SUBMIT_REPORT        BUSENUM_W_IOCTL (IOCTL_VIGEM_BASE + 0x204)
#define IOCTL_XGIP_SUBMIT_INTERRUPT     BUSENUM_W_IOCTL (IOCTL_VIGEM_BASE + 0x205)


//
//  Data structure used in PlugIn and UnPlug ioctls
//

//
// Represents the desired target type for the emulated device.
//  
typedef enum _VIGEM_TARGET_TYPE
{
    // 
    // Microsoft Xbox 360 Controller (wired)
    // 
    Xbox360Wired,
    // 
    // Microsoft Xbox One Controller (wired)
    // 
    XboxOneWired,
    //
    // Sony DualShock 4 (wired)
    // 
    DualShock4Wired
} VIGEM_TARGET_TYPE, *PVIGEM_TARGET_TYPE;

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
// Possible XUSB report buttons.
// 
typedef enum _XUSB_BUTTON
{
    XUSB_GAMEPAD_DPAD_UP = 0x0001,
    XUSB_GAMEPAD_DPAD_DOWN = 0x0002,
    XUSB_GAMEPAD_DPAD_LEFT = 0x0004,
    XUSB_GAMEPAD_DPAD_RIGHT = 0x0008,
    XUSB_GAMEPAD_START = 0x0010,
    XUSB_GAMEPAD_BACK = 0x0020,
    XUSB_GAMEPAD_LEFT_THUMB = 0x0040,
    XUSB_GAMEPAD_RIGHT_THUMB = 0x0080,
    XUSB_GAMEPAD_LEFT_SHOULDER = 0x0100,
    XUSB_GAMEPAD_RIGHT_SHOULDER = 0x0200,
    XUSB_GAMEPAD_GUIDE = 0x0400,
    XUSB_GAMEPAD_A = 0x1000,
    XUSB_GAMEPAD_B = 0x2000,
    XUSB_GAMEPAD_X = 0x4000,
    XUSB_GAMEPAD_Y = 0x8000
} XUSB_BUTTON, *PXUSB_BUTTON;

//
// Represents an XINPUT_GAMEPAD-compatible report structure.
// 
typedef struct _XUSB_REPORT
{
    USHORT wButtons;
    BYTE bLeftTrigger;
    BYTE bRightTrigger;
    SHORT sThumbLX;
    SHORT sThumbLY;
    SHORT sThumbRX;
    SHORT sThumbRY;
} XUSB_REPORT, *PXUSB_REPORT;

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

typedef struct _DS4_LIGHTBAR_COLOR
{
    //
    // Red part of the Lightbar (0-255).
    //
    UCHAR Red;

    //
    // Green part of the Lightbar (0-255).
    //
    UCHAR Green;

    //
    // Blue part of the Lightbar (0-255).
    //
    UCHAR Blue;
} DS4_LIGHTBAR_COLOR, *PDS4_LIGHTBAR_COLOR;

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
// DualShock 4 HID Input report
// 
typedef struct _DS4_REPORT
{
    BYTE bThumbLX;
    BYTE bThumbLY;
    BYTE bThumbRX;
    BYTE bThumbRY;
    USHORT wButtons;
    BYTE bSpecial;
    BYTE bTriggerL;
    BYTE bTriggerR;
} DS4_REPORT, *PDS4_REPORT;

//
// DualShock 4 digital buttons
// 
typedef enum _DS4_BUTTONS
{
    Ds4ThumbR = 1 << 15,
    Ds4ThumbL = 1 << 14,
    Ds4Options = 1 << 13,
    Ds4Share = 1 << 12,
    Ds4TriggerR = 1 << 11,
    Ds4TriggerL = 1 << 10,
    Ds4ShoulderR = 1 << 9,
    Ds4ShoulderL = 1 << 8,
    Ds4Triangle = 1 << 7,
    Ds4Circle = 1 << 6,
    Ds4Cross = 1 << 5,
    Ds4Square = 1 << 4
} DS4_BUTTONS, *PDS4_BUTTONS;

//
// DualShock 4 special buttons
// 
typedef enum _DS4_SPECIAL_BUTTONS
{
    Ds4Ps = 1 << 0,
    Ds4Touchpad = 1 << 1
} DS4_SPECIAL_BUTTONS, *PDS4_SPECIAL_BUTTONS;

//
// DualShock 4 directional pad values
// 
typedef enum _DS4_DPAD_DIRECTIONS
{
    Ds4DpadNone = 0x8,
    Ds4DpadNW = 0x7,
    Ds4DpadW = 0x6,
    Ds4DpadSW = 0x5,
    Ds4DpadS = 0x4,
    Ds4DpadSE = 0x3,
    Ds4DpadE = 0x2,
    Ds4DpadNE = 0x1,
    Ds4DpadN = 0x0
} DS4_DPAD_DIRECTIONS, *PDS4_DPAD_DIRECTIONS;

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
// Sets the current state of the D-PAD on a DualShock 4 report.
// 
VOID FORCEINLINE DS4_SET_DPAD(
    _Out_ PDS4_REPORT Report,
    _In_ DS4_DPAD_DIRECTIONS Dpad
)
{
    Report->wButtons &= ~0xF;
    Report->wButtons |= (USHORT)Dpad;
}

VOID FORCEINLINE DS4_REPORT_INIT(
    _Out_ PDS4_REPORT Report
)
{
    Report->bThumbLX = 0x80;
    Report->bThumbLY = 0x80;
    Report->bThumbRX = 0x80;
    Report->bThumbRY = 0x80;

    DS4_SET_DPAD(Report, Ds4DpadNone);
}

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

typedef struct _XGIP_REPORT
{
    UCHAR Buttons1;
    UCHAR Buttons2;
    SHORT LeftTrigger;
    SHORT RightTrigger;
    SHORT ThumbLX;
    SHORT ThumbLY;
    SHORT ThumbRX;
    SHORT ThumbRY;
} XGIP_REPORT, *PXGIP_REPORT;

//
// Xbox One request data
// 
typedef struct _XGIP_SUBMIT_REPORT
{
    //
    // sizeof(struct _XGIP_SUBMIT_REPORT)
    // 
    ULONG Size;

    //
    // Serial number of target device.
    // 
    ULONG SerialNo;

    //
    // HID Input report
    // 
    XGIP_REPORT Report;
} XGIP_SUBMIT_REPORT, *PXGIP_SUBMIT_REPORT;

//
// Initializes an Xbox One report.
// 
VOID FORCEINLINE XGIP_SUBMIT_REPORT_INIT(
    _Out_ PXGIP_SUBMIT_REPORT Report,
    _In_ ULONG SerialNo
)
{
    RtlZeroMemory(Report, sizeof(XGIP_SUBMIT_REPORT));

    Report->Size = sizeof(XGIP_SUBMIT_REPORT);
    Report->SerialNo = SerialNo;
}

//
// Xbox One interrupt data
// 
typedef struct _XGIP_SUBMIT_INTERRUPT
{
    //
    // sizeof(struct _XGIP_SUBMIT_INTERRUPT)
    // 
    ULONG Size;

    //
    // Serial number of target device.
    // 
    ULONG SerialNo;

    //
    // Interrupt buffer.
    // 
    UCHAR Interrupt[64];

    //
    // Length of interrupt buffer.
    // 
    ULONG InterruptLength;
} XGIP_SUBMIT_INTERRUPT, *PXGIP_SUBMIT_INTERRUPT;

//
// Initializes an Xbox One interrupt.
// 
VOID FORCEINLINE XGIP_SUBMIT_INTERRUPT_INIT(
    _Out_ PXGIP_SUBMIT_INTERRUPT Report,
    _In_ ULONG SerialNo
)
{
    RtlZeroMemory(Report, sizeof(XGIP_SUBMIT_INTERRUPT));

    Report->Size = sizeof(XGIP_SUBMIT_INTERRUPT);
    Report->SerialNo = SerialNo;
}

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

