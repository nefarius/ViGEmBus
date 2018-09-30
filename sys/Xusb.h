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


// 
// For children emulating XUSB devices, the following dummy interfaces 
// have to be exposed by the PDO or else the child devices won't start
// 
// TODO: that statement might be obsolete, further testing required
// 

// {70211B0E-0AFB-47DB-AFC1-410BF842497A}
// ReSharper disable once CppMissingIncludeGuard
DEFINE_GUID(GUID_DEVINTERFACE_XUSB_UNKNOWN_0,
    0x70211B0E, 0x0AFB, 0x47DB, 0xAF, 0xC1, 0x41, 0x0B, 0xF8, 0x42, 0x49, 0x7A);

// {B38290E5-3CD0-4F9D-9937-F5FE2B44D47A}
DEFINE_GUID(GUID_DEVINTERFACE_XUSB_UNKNOWN_1,
    0xB38290E5, 0x3CD0, 0x4F9D, 0x99, 0x37, 0xF5, 0xFE, 0x2B, 0x44, 0xD4, 0x7A);

// {2AEB0243-6A6E-486B-82FC-D815F6B97006}
DEFINE_GUID(GUID_DEVINTERFACE_XUSB_UNKNOWN_2,
    0x2AEB0243, 0x6A6E, 0x486B, 0x82, 0xFC, 0xD8, 0x15, 0xF6, 0xB9, 0x70, 0x06);

#pragma once

#if defined(_X86_)
#define XUSB_CONFIGURATION_SIZE         0x00E4
#else
#define XUSB_CONFIGURATION_SIZE         0x0130
#endif
#define XUSB_DESCRIPTOR_SIZE	        0x0099
#define XUSB_RUMBLE_SIZE                0x08
#define XUSB_LEDSET_SIZE                0x03
#define XUSB_LEDNUM_SIZE                0x01
#define XUSB_INIT_STAGE_SIZE            0x03
#define XUSB_INIT_BLOB_COUNT			0x07

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
	PVOID InterruptInitStageBlobs[XUSB_INIT_BLOB_COUNT];

} XUSB_DEVICE_DATA, *PXUSB_DEVICE_DATA;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(XUSB_DEVICE_DATA, XusbGetData)


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
NTSTATUS Xusb_ReleaseHardware(WDFDEVICE Device);
NTSTATUS Xusb_AssignPdoContext(WDFDEVICE Device);
VOID Xusb_GetConfigurationDescriptorType(PUCHAR Buffer, ULONG Length);
VOID Xusb_GetDeviceDescriptorType(PUSB_DEVICE_DESCRIPTOR pDescriptor, PPDO_DEVICE_DATA pCommon);
VOID Xusb_SelectConfiguration(PUSBD_INTERFACE_INFORMATION pInfo);
NTSTATUS Xusb_GetUserIndex(WDFDEVICE Device, PXUSB_GET_USER_INDEX Request);
