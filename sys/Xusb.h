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


// 
// For children emulating XUSB devices, the following dummy interfaces 
// have to be exposed by the PDO or else the child devices won't start
// 
// TODO: that statement might be obsolete, further testing required
// 

// Below these interfaces are from wdmguid.h being used as dummies
// {70211B0E-0AFB-47DB-AFC1-410BF842497A} PNP_LOCATION_INTERFACE
// {B38290E5-3CD0-4F9D-9937-F5FE2B44D47A} D3COLD_SUPPORT_INTERFACE
// {2AEB0243-6A6E-486B-82FC-D815F6B97006} REENUMERATE_SELF_INTERFACE_STANDARD

#pragma once

#if defined(_X86_)
#define XUSB_CONFIGURATION_SIZE         0x00E4
#else
#define XUSB_CONFIGURATION_SIZE         0x0130
#endif
#define XUSB_DESCRIPTOR_SIZE            0x0099
#define XUSB_RUMBLE_SIZE                0x08
#define XUSB_LEDSET_SIZE                0x03
#define XUSB_LEDNUM_SIZE                0x01
#define XUSB_INIT_STAGE_SIZE            0x03
#define XUSB_BLOB_STORAGE_SIZE          0x2A

#define XUSB_BLOB_00_OFFSET             0x00
#define XUSB_BLOB_01_OFFSET             0x03
#define XUSB_BLOB_02_OFFSET             0x06
#define XUSB_BLOB_03_OFFSET             0x09
#define XUSB_BLOB_04_OFFSET             0x0C
#define XUSB_BLOB_05_OFFSET             0x20
#define XUSB_BLOB_06_OFFSET             0x23
#define XUSB_BLOB_07_OFFSET             0x26

#define XUSB_IS_DATA_PIPE(_x_)          ((BOOLEAN)(_x_->PipeHandle == (USBD_PIPE_HANDLE)0xFFFF0081))
#define XUSB_IS_CONTROL_PIPE(_x_)       ((BOOLEAN)(_x_->PipeHandle == (USBD_PIPE_HANDLE)0xFFFF0083))

typedef struct _XUSB_INTERRUPT_IN_PACKET
{
    UCHAR Id;

    UCHAR Size;

    XUSB_REPORT Report;

} XUSB_INTERRUPT_IN_PACKET, *PXUSB_INTERRUPT_IN_PACKET;

//
// XUSB-specific device context data.
// 
typedef struct _XUSB_DEVICE_DATA
{
    //
    // Rumble buffer
    //
    UCHAR Rumble[XUSB_RUMBLE_SIZE];

    //
    // LED number (represents XInput slot index)
    //
    CHAR LedNumber;

    //
    // Report packet
    //
    XUSB_INTERRUPT_IN_PACKET Packet;

    //
    // Queue for incoming control interrupt transfer
    //
    WDFQUEUE HoldingUsbInRequests;

    //
    // Required for XInputGetCapabilities to work
    // 
    BOOLEAN ReportedCapabilities;

    //
    // Required for XInputGetCapabilities to work
    // 
    ULONG InterruptInitStage;

    //
    // Storage of binary blobs (packets) for PDO initialization
    // 
    WDFMEMORY InterruptBlobStorage;

} XUSB_DEVICE_DATA, *PXUSB_DEVICE_DATA;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(XUSB_DEVICE_DATA, XusbGetData)

EXTERN_C_START

NTSTATUS
Bus_XusbSubmitReport(
    WDFDEVICE Device,
    ULONG SerialNo,
    PXUSB_SUBMIT_REPORT Report,
    _In_ BOOLEAN FromInterface
);

//
// XUSB-specific functions
// 
NTSTATUS Xusb_PreparePdo(PWDFDEVICE_INIT DeviceInit, USHORT VendorId, USHORT ProductId, PUNICODE_STRING DeviceId, PUNICODE_STRING DeviceDescription);
NTSTATUS Xusb_PrepareHardware(WDFDEVICE Device);
NTSTATUS Xusb_AssignPdoContext(WDFDEVICE Device);
VOID Xusb_GetConfigurationDescriptorType(PUCHAR Buffer, ULONG Length);
VOID Xusb_GetDeviceDescriptorType(PUSB_DEVICE_DESCRIPTOR pDescriptor, PPDO_DEVICE_DATA pCommon);
VOID Xusb_SelectConfiguration(PUSBD_INTERFACE_INFORMATION pInfo);
NTSTATUS Xusb_GetUserIndex(WDFDEVICE Device, PXUSB_GET_USER_INDEX Request);

EXTERN_C_END
