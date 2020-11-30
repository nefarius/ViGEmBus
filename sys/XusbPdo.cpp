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

#include "Driver.h"
#include "XusbPdo.hpp"
#include "trace.h"
#include "XusbPdo.tmh"
#define NTSTRSAFE_LIB
#include <ntstrsafe.h>

#include <initguid.h>
#include <usbbusif.h>

#include <ViGEm/km/BusShared.h>
#include "Debugging.hpp"


PCWSTR ViGEm::Bus::Targets::EmulationTargetXUSB::_deviceDescription = L"Virtual Xbox 360 Controller";

ViGEm::Bus::Targets::EmulationTargetXUSB::EmulationTargetXUSB(ULONG Serial, LONG SessionId, USHORT VendorId,
	USHORT ProductId) : EmulationTargetPDO(
		Serial, SessionId, VendorId, ProductId)
{
	this->_TargetType = Xbox360Wired;
	this->_UsbConfigurationDescriptionSize = XUSB_DESCRIPTOR_SIZE;

	//
	// Set PNP Capabilities
	// 
	this->_PnpCapabilities.Removable = WdfTrue;
	this->_PnpCapabilities.SurpriseRemovalOK = WdfTrue;
	this->_PnpCapabilities.UniqueID = WdfTrue;

	//
	// Set Power Capabilities
	//	
	this->_PowerCapabilities.DeviceState[PowerSystemWorking] = PowerDeviceD0;
	this->_PowerCapabilities.DeviceState[PowerSystemSleeping1] = PowerDeviceD2;
	this->_PowerCapabilities.DeviceState[PowerSystemSleeping2] = PowerDeviceD2;
	this->_PowerCapabilities.DeviceState[PowerSystemSleeping3] = PowerDeviceD2;
	this->_PowerCapabilities.DeviceState[PowerSystemHibernate] = PowerDeviceD2;
	this->_PowerCapabilities.DeviceState[PowerSystemShutdown] = PowerDeviceD3;
	this->_PowerCapabilities.DeviceD1 = WdfTrue;
	this->_PowerCapabilities.DeviceD2 = WdfTrue;
	this->_PowerCapabilities.WakeFromD0 = WdfTrue;
	this->_PowerCapabilities.WakeFromD1 = WdfTrue;
	this->_PowerCapabilities.WakeFromD2 = WdfTrue;
}

NTSTATUS ViGEm::Bus::Targets::EmulationTargetXUSB::PdoPrepareDevice(PWDFDEVICE_INIT DeviceInit, PUNICODE_STRING DeviceId,
	PUNICODE_STRING DeviceDescription)
{
	NTSTATUS status;
	DECLARE_UNICODE_STRING_SIZE(buffer, _maxHardwareIdLength);

	// prepare device description
	status = RtlUnicodeStringInit(DeviceDescription, _deviceDescription);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR,
			TRACE_XUSB,
			"RtlUnicodeStringInit failed with status %!STATUS!",
			status);
		return status;
	}

	// Set hardware ID
	RtlUnicodeStringPrintf(&buffer, L"USB\\VID_%04X&PID_%04X", this->_VendorId, this->_ProductId);

	RtlUnicodeStringCopy(DeviceId, &buffer);

	status = WdfPdoInitAddHardwareID(DeviceInit, &buffer);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR,
			TRACE_XUSB,
			"WdfPdoInitAddHardwareID failed with status %!STATUS!",
			status);
		return status;
	}


	// Set compatible IDs
	RtlUnicodeStringInit(&buffer, L"USB\\MS_COMP_XUSB10");

	status = WdfPdoInitAddCompatibleID(DeviceInit, &buffer);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR,
			TRACE_XUSB,
			"WdfPdoInitAddCompatibleID #1 failed with status %!STATUS!",
			status);
		return status;
	}

	RtlUnicodeStringInit(&buffer, L"USB\\Class_FF&SubClass_5D&Prot_01");

	status = WdfPdoInitAddCompatibleID(DeviceInit, &buffer);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR,
			TRACE_XUSB,
			"WdfPdoInitAddCompatibleID #2 failed with status %!STATUS!",
			status);
		return status;
	}

	RtlUnicodeStringInit(&buffer, L"USB\\Class_FF&SubClass_5D");

	status = WdfPdoInitAddCompatibleID(DeviceInit, &buffer);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR,
			TRACE_XUSB,
			"WdfPdoInitAddCompatibleID #3 failed with status %!STATUS!",
			status);
		return status;
	}

	RtlUnicodeStringInit(&buffer, L"USB\\Class_FF");

	status = WdfPdoInitAddCompatibleID(DeviceInit, &buffer);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR,
			TRACE_XUSB,
			"WdfPdoInitAddCompatibleID #4 failed with status %!STATUS!",
			status);
		return status;
	}

	return STATUS_SUCCESS;
}

NTSTATUS ViGEm::Bus::Targets::EmulationTargetXUSB::PdoPrepareHardware()
{
	NTSTATUS status;
	WDF_QUERY_INTERFACE_CONFIG ifaceCfg;

	// 
	// Expose USB_BUS_INTERFACE_USBDI_GUID
	// 

	USB_BUS_INTERFACE_USBDI_V1 xusbInterface;

	xusbInterface.Size = sizeof(USB_BUS_INTERFACE_USBDI_V1);
	xusbInterface.Version = USB_BUSIF_USBDI_VERSION_1;
	xusbInterface.BusContext = static_cast<PVOID>(this->_PdoDevice);

	xusbInterface.InterfaceReference = WdfDeviceInterfaceReferenceNoOp;
	xusbInterface.InterfaceDereference = WdfDeviceInterfaceDereferenceNoOp;

	xusbInterface.SubmitIsoOutUrb = UsbInterfaceSubmitIsoOutUrb;
	xusbInterface.GetUSBDIVersion = UsbInterfaceGetUSBDIVersion;
	xusbInterface.QueryBusTime = UsbInterfaceQueryBusTime;
	xusbInterface.QueryBusInformation = UsbInterfaceQueryBusInformation;
	xusbInterface.IsDeviceHighSpeed = UsbInterfaceIsDeviceHighSpeed;

	WDF_QUERY_INTERFACE_CONFIG_INIT(
		&ifaceCfg,
		reinterpret_cast<PINTERFACE>(&xusbInterface),
		&USB_BUS_INTERFACE_USBDI_GUID,
		nullptr
	);

	status = WdfDeviceAddQueryInterface(this->_PdoDevice, &ifaceCfg);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR,
			TRACE_XUSB,
			"WdfDeviceAddQueryInterface failed with status %!STATUS!",
			status);
		return status;
	}

	return STATUS_SUCCESS;
}

NTSTATUS ViGEm::Bus::Targets::EmulationTargetXUSB::PdoInitContext()
{
	WDF_OBJECT_ATTRIBUTES attributes;
	PUCHAR blobBuffer;

	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	attributes.ParentObject = this->_PdoDevice;

	TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_XUSB, "Initializing XUSB context...");

	RtlZeroMemory(this->_Rumble, ARRAYSIZE(this->_Rumble));

	// Is later overwritten by actual XInput slot
	this->_LedNumber = -1;

	RtlZeroMemory(&this->_Packet, sizeof(XUSB_INTERRUPT_IN_PACKET));
	// Packet size (20 bytes = 0x14)
	this->_Packet.Size = 0x14;

	this->_ReportedCapabilities = FALSE;

	this->_InterruptInitStage = 0;

	// Allocate blob storage
	NTSTATUS status = WdfMemoryCreate(
		&attributes,
		NonPagedPoolNx,
		XUSB_POOL_TAG,
		XUSB_BLOB_STORAGE_SIZE,
		&this->_InterruptBlobStorage,
		reinterpret_cast<PVOID*>(&blobBuffer)
	);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR,
			TRACE_XUSB,
			"WdfMemoryCreate failed with status %!STATUS!",
			status);
		return status;
	}

	// Fill blob storage
	COPY_BYTE_ARRAY(blobBuffer, P99_PROTECT({
		// 0
		0x01, 0x03, 0x0E,
		// 1
		0x02, 0x03, 0x00,
		// 2
		0x03, 0x03, 0x03,
		// 3
		0x08, 0x03, 0x00,
		// 4
		0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0xe4, 0xf2,
		0xb3, 0xf8, 0x49, 0xf3, 0xb0, 0xfc, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		// 5
		0x01, 0x03, 0x03,
		// 6
		0x05, 0x03, 0x00,
		// 7
		0x31, 0x3F, 0xCF, 0xDC
		}));

	// I/O Queue for pending IRPs
	WDF_IO_QUEUE_CONFIG holdingInQueueConfig;

	// Create and assign queue for unhandled interrupt requests
	WDF_IO_QUEUE_CONFIG_INIT(&holdingInQueueConfig, WdfIoQueueDispatchManual);

	status = WdfIoQueueCreate(
		this->_PdoDevice,
		&holdingInQueueConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		&this->_HoldingUsbInRequests
	);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR,
			TRACE_XUSB,
			"WdfIoQueueCreate (HoldingUsbInRequests) failed with status %!STATUS!",
			status);
		return status;
	}

	return STATUS_SUCCESS;
}

VOID ViGEm::Bus::Targets::EmulationTargetXUSB::GetConfigurationDescriptorType(PUCHAR Buffer, ULONG Length)
{
	UCHAR XusbDescriptorData[XUSB_DESCRIPTOR_SIZE] =
	{
		0x09,        //   bLength
		0x02,        //   bDescriptorType (Configuration)
		0x99, 0x00,  //   wTotalLength 153
		0x04,        //   bNumInterfaces 4
		0x01,        //   bConfigurationValue
		0x00,        //   iConfiguration (String Index)
		0xA0,        //   bmAttributes Remote Wakeup
		0xFA,        //   bMaxPower 500mA

		0x09,        //   bLength
		0x04,        //   bDescriptorType (Interface)
		0x00,        //   bInterfaceNumber 0
		0x00,        //   bAlternateSetting
		0x02,        //   bNumEndpoints 2
		0xFF,        //   bInterfaceClass
		0x5D,        //   bInterfaceSubClass
		0x01,        //   bInterfaceProtocol
		0x00,        //   iInterface (String Index)

		0x11,        //   bLength
		0x21,        //   bDescriptorType (HID)
		0x00, 0x01,  //   bcdHID 1.00
		0x01,        //   bCountryCode
		0x25,        //   bNumDescriptors
		0x81,        //   bDescriptorType[0] (Unknown 0x81)
		0x14, 0x00,  //   wDescriptorLength[0] 20
		0x00,        //   bDescriptorType[1] (Unknown 0x00)
		0x00, 0x00,  //   wDescriptorLength[1] 0
		0x13,        //   bDescriptorType[2] (Unknown 0x13)
		0x01, 0x08,  //   wDescriptorLength[2] 2049
		0x00,        //   bDescriptorType[3] (Unknown 0x00)
		0x00,
		0x07,        //   bLength
		0x05,        //   bDescriptorType (Endpoint)
		0x81,        //   bEndpointAddress (IN/D2H)
		0x03,        //   bmAttributes (Interrupt)
		0x20, 0x00,  //   wMaxPacketSize 32
		0x04,        //   bInterval 4 (unit depends on device speed)

		0x07,        //   bLength
		0x05,        //   bDescriptorType (Endpoint)
		0x01,        //   bEndpointAddress (OUT/H2D)
		0x03,        //   bmAttributes (Interrupt)
		0x20, 0x00,  //   wMaxPacketSize 32
		0x08,        //   bInterval 8 (unit depends on device speed)

		0x09,        //   bLength
		0x04,        //   bDescriptorType (Interface)
		0x01,        //   bInterfaceNumber 1
		0x00,        //   bAlternateSetting
		0x04,        //   bNumEndpoints 4
		0xFF,        //   bInterfaceClass
		0x5D,        //   bInterfaceSubClass
		0x03,        //   bInterfaceProtocol
		0x00,        //   iInterface (String Index)

		0x1B,        //   bLength
		0x21,        //   bDescriptorType (HID)
		0x00, 0x01,  //   bcdHID 1.00
		0x01,        //   bCountryCode
		0x01,        //   bNumDescriptors
		0x82,        //   bDescriptorType[0] (Unknown 0x82)
		0x40, 0x01,  //   wDescriptorLength[0] 320
		0x02, 0x20, 0x16, 0x83, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x07,        //   bLength
		0x05,        //   bDescriptorType (Endpoint)
		0x82,        //   bEndpointAddress (IN/D2H)
		0x03,        //   bmAttributes (Interrupt)
		0x20, 0x00,  //   wMaxPacketSize 32
		0x02,        //   bInterval 2 (unit depends on device speed)

		0x07,        //   bLength
		0x05,        //   bDescriptorType (Endpoint)
		0x02,        //   bEndpointAddress (OUT/H2D)
		0x03,        //   bmAttributes (Interrupt)
		0x20, 0x00,  //   wMaxPacketSize 32
		0x04,        //   bInterval 4 (unit depends on device speed)

		0x07,        //   bLength
		0x05,        //   bDescriptorType (Endpoint)
		0x83,        //   bEndpointAddress (IN/D2H)
		0x03,        //   bmAttributes (Interrupt)
		0x20, 0x00,  //   wMaxPacketSize 32
		0x40,        //   bInterval 64 (unit depends on device speed)

		0x07,        //   bLength
		0x05,        //   bDescriptorType (Endpoint)
		0x03,        //   bEndpointAddress (OUT/H2D)
		0x03,        //   bmAttributes (Interrupt)
		0x20, 0x00,  //   wMaxPacketSize 32
		0x10,        //   bInterval 16 (unit depends on device speed)

		0x09,        //   bLength
		0x04,        //   bDescriptorType (Interface)
		0x02,        //   bInterfaceNumber 2
		0x00,        //   bAlternateSetting
		0x01,        //   bNumEndpoints 1
		0xFF,        //   bInterfaceClass
		0x5D,        //   bInterfaceSubClass
		0x02,        //   bInterfaceProtocol
		0x00,        //   iInterface (String Index)

		0x09,        //   bLength
		0x21,        //   bDescriptorType (HID)
		0x00, 0x01,  //   bcdHID 1.00
		0x01,        //   bCountryCode
		0x22,        //   bNumDescriptors
		0x84,        //   bDescriptorType[0] (Unknown 0x84)
		0x07, 0x00,  //   wDescriptorLength[0] 7

		0x07,        //   bLength
		0x05,        //   bDescriptorType (Endpoint)
		0x84,        //   bEndpointAddress (IN/D2H)
		0x03,        //   bmAttributes (Interrupt)
		0x20, 0x00,  //   wMaxPacketSize 32
		0x10,        //   bInterval 16 (unit depends on device speed)

		0x09,        //   bLength
		0x04,        //   bDescriptorType (Interface)
		0x03,        //   bInterfaceNumber 3
		0x00,        //   bAlternateSetting
		0x00,        //   bNumEndpoints 0
		0xFF,        //   bInterfaceClass
		0xFD,        //   bInterfaceSubClass
		0x13,        //   bInterfaceProtocol
		0x04,        //   iInterface (String Index)

		0x06,        //   bLength
		0x41,        //   bDescriptorType (Unknown)
		0x00, 0x01, 0x01, 0x03,
		// 153 bytes

		// best guess: USB Standard Descriptor
	};

	RtlCopyBytes(Buffer, XusbDescriptorData, Length);
}

NTSTATUS ViGEm::Bus::Targets::EmulationTargetXUSB::UsbGetDeviceDescriptorType(PUSB_DEVICE_DESCRIPTOR pDescriptor)
{
	pDescriptor->bLength = 0x12;
	pDescriptor->bDescriptorType = USB_DEVICE_DESCRIPTOR_TYPE;
	pDescriptor->bcdUSB = 0x0200; // USB v2.0
	pDescriptor->bDeviceClass = 0xFF;
	pDescriptor->bDeviceSubClass = 0xFF;
	pDescriptor->bDeviceProtocol = 0xFF;
	pDescriptor->bMaxPacketSize0 = 0x08;
	pDescriptor->idVendor = this->_VendorId;
	pDescriptor->idProduct = this->_ProductId;
	pDescriptor->bcdDevice = 0x0114;
	pDescriptor->iManufacturer = 0x01;
	pDescriptor->iProduct = 0x02;
	pDescriptor->iSerialNumber = 0x03;
	pDescriptor->bNumConfigurations = 0x01;

	return STATUS_SUCCESS;
}

NTSTATUS ViGEm::Bus::Targets::EmulationTargetXUSB::SelectConfiguration(PURB Urb)
{
	if (Urb->UrbHeader.Length < XUSB_CONFIGURATION_SIZE)
	{
		TraceEvents(TRACE_LEVEL_WARNING,
			TRACE_USBPDO,
			">> >> >> URB_FUNCTION_SELECT_CONFIGURATION: Invalid ConfigurationDescriptor");
		return STATUS_INVALID_PARAMETER;
	}

	PUSBD_INTERFACE_INFORMATION pInfo = &Urb->UrbSelectConfiguration.Interface;

	TraceDbg(
		TRACE_XUSB,
		">> >> >> URB_FUNCTION_SELECT_CONFIGURATION: Length %d, Interface %d, Alternate %d, Pipes %d",
		(int)pInfo->Length,
		(int)pInfo->InterfaceNumber,
		(int)pInfo->AlternateSetting,
		pInfo->NumberOfPipes);

	pInfo->Class = 0xFF;
	pInfo->SubClass = 0x5D;
	pInfo->Protocol = 0x01;

	pInfo->InterfaceHandle = (USBD_INTERFACE_HANDLE)0xFFFF0000;

	pInfo->Pipes[0].MaximumTransferSize = 0x00400000;
	pInfo->Pipes[0].MaximumPacketSize = 0x20;
	pInfo->Pipes[0].EndpointAddress = 0x81;
	pInfo->Pipes[0].Interval = 0x04;
	pInfo->Pipes[0].PipeType = (USBD_PIPE_TYPE)0x03;
	pInfo->Pipes[0].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0081;
	pInfo->Pipes[0].PipeFlags = 0x00;

	pInfo->Pipes[1].MaximumTransferSize = 0x00400000;
	pInfo->Pipes[1].MaximumPacketSize = 0x20;
	pInfo->Pipes[1].EndpointAddress = 0x01;
	pInfo->Pipes[1].Interval = 0x08;
	pInfo->Pipes[1].PipeType = (USBD_PIPE_TYPE)0x03;
	pInfo->Pipes[1].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0001;
	pInfo->Pipes[1].PipeFlags = 0x00;

	pInfo = (PUSBD_INTERFACE_INFORMATION)((PCHAR)pInfo + pInfo->Length);

	TraceDbg(
		TRACE_XUSB,
		">> >> >> URB_FUNCTION_SELECT_CONFIGURATION: Length %d, Interface %d, Alternate %d, Pipes %d",
		(int)pInfo->Length,
		(int)pInfo->InterfaceNumber,
		(int)pInfo->AlternateSetting,
		pInfo->NumberOfPipes);

	pInfo->Class = 0xFF;
	pInfo->SubClass = 0x5D;
	pInfo->Protocol = 0x03;

	pInfo->InterfaceHandle = (USBD_INTERFACE_HANDLE)0xFFFF0000;

	pInfo->Pipes[0].MaximumTransferSize = 0x00400000;
	pInfo->Pipes[0].MaximumPacketSize = 0x20;
	pInfo->Pipes[0].EndpointAddress = 0x82;
	pInfo->Pipes[0].Interval = 0x04;
	pInfo->Pipes[0].PipeType = (USBD_PIPE_TYPE)0x03;
	pInfo->Pipes[0].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0082;
	pInfo->Pipes[0].PipeFlags = 0x00;

	pInfo->Pipes[1].MaximumTransferSize = 0x00400000;
	pInfo->Pipes[1].MaximumPacketSize = 0x20;
	pInfo->Pipes[1].EndpointAddress = 0x02;
	pInfo->Pipes[1].Interval = 0x08;
	pInfo->Pipes[1].PipeType = (USBD_PIPE_TYPE)0x03;
	pInfo->Pipes[1].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0002;
	pInfo->Pipes[1].PipeFlags = 0x00;

	pInfo->Pipes[2].MaximumTransferSize = 0x00400000;
	pInfo->Pipes[2].MaximumPacketSize = 0x20;
	pInfo->Pipes[2].EndpointAddress = 0x83;
	pInfo->Pipes[2].Interval = 0x08;
	pInfo->Pipes[2].PipeType = (USBD_PIPE_TYPE)0x03;
	pInfo->Pipes[2].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0083;
	pInfo->Pipes[2].PipeFlags = 0x00;

	pInfo->Pipes[3].MaximumTransferSize = 0x00400000;
	pInfo->Pipes[3].MaximumPacketSize = 0x20;
	pInfo->Pipes[3].EndpointAddress = 0x03;
	pInfo->Pipes[3].Interval = 0x08;
	pInfo->Pipes[3].PipeType = (USBD_PIPE_TYPE)0x03;
	pInfo->Pipes[3].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0003;
	pInfo->Pipes[3].PipeFlags = 0x00;

	pInfo = (PUSBD_INTERFACE_INFORMATION)((PCHAR)pInfo + pInfo->Length);

	TraceDbg(
		TRACE_XUSB,
		">> >> >> URB_FUNCTION_SELECT_CONFIGURATION: Length %d, Interface %d, Alternate %d, Pipes %d",
		(int)pInfo->Length,
		(int)pInfo->InterfaceNumber,
		(int)pInfo->AlternateSetting,
		pInfo->NumberOfPipes);

	pInfo->Class = 0xFF;
	pInfo->SubClass = 0x5D;
	pInfo->Protocol = 0x02;

	pInfo->InterfaceHandle = (USBD_INTERFACE_HANDLE)0xFFFF0000;

	pInfo->Pipes[0].MaximumTransferSize = 0x00400000;
	pInfo->Pipes[0].MaximumPacketSize = 0x20;
	pInfo->Pipes[0].EndpointAddress = 0x84;
	pInfo->Pipes[0].Interval = 0x04;
	pInfo->Pipes[0].PipeType = (USBD_PIPE_TYPE)0x03;
	pInfo->Pipes[0].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0084;
	pInfo->Pipes[0].PipeFlags = 0x00;

	pInfo = (PUSBD_INTERFACE_INFORMATION)((PCHAR)pInfo + pInfo->Length);

	TraceDbg(
		TRACE_XUSB,
		">> >> >> URB_FUNCTION_SELECT_CONFIGURATION: Length %d, Interface %d, Alternate %d, Pipes %d",
		(int)pInfo->Length,
		(int)pInfo->InterfaceNumber,
		(int)pInfo->AlternateSetting,
		pInfo->NumberOfPipes);

	pInfo->Class = 0xFF;
	pInfo->SubClass = 0xFD;
	pInfo->Protocol = 0x13;

	pInfo->InterfaceHandle = (USBD_INTERFACE_HANDLE)0xFFFF0000;

	return STATUS_SUCCESS;
}

void ViGEm::Bus::Targets::EmulationTargetXUSB::AbortPipe()
{
}

NTSTATUS ViGEm::Bus::Targets::EmulationTargetXUSB::UsbClassInterface(PURB Urb)
{
	UNREFERENCED_PARAMETER(Urb);

	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS ViGEm::Bus::Targets::EmulationTargetXUSB::UsbGetDescriptorFromInterface(PURB Urb)
{
	UNREFERENCED_PARAMETER(Urb);

	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS ViGEm::Bus::Targets::EmulationTargetXUSB::UsbSelectInterface(PURB Urb)
{
	PUSBD_INTERFACE_INFORMATION pInfo = &Urb->UrbSelectInterface.Interface;

	TraceDbg(
		TRACE_USBPDO,
		">> >> >> URB_FUNCTION_SELECT_INTERFACE: Length %d, Interface %d, Alternate %d, Pipes %d",
		(int)pInfo->Length,
		(int)pInfo->InterfaceNumber,
		(int)pInfo->AlternateSetting,
		pInfo->NumberOfPipes);

	TraceDbg(
		TRACE_USBPDO,
		">> >> >> URB_FUNCTION_SELECT_INTERFACE: Class %d, SubClass %d, Protocol %d",
		(int)pInfo->Class,
		(int)pInfo->SubClass,
		(int)pInfo->Protocol);

	if (pInfo->InterfaceNumber == 1)
	{
		pInfo[0].Class = 0xFF;
		pInfo[0].SubClass = 0x5D;
		pInfo[0].Protocol = 0x03;
		pInfo[0].NumberOfPipes = 0x04;

		pInfo[0].InterfaceHandle = (USBD_INTERFACE_HANDLE)0xFFFF0000;

		pInfo[0].Pipes[0].MaximumTransferSize = 0x00400000;
		pInfo[0].Pipes[0].MaximumPacketSize = 0x20;
		pInfo[0].Pipes[0].EndpointAddress = 0x82;
		pInfo[0].Pipes[0].Interval = 0x04;
		pInfo[0].Pipes[0].PipeType = (USBD_PIPE_TYPE)0x03;
		pInfo[0].Pipes[0].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0082;
		pInfo[0].Pipes[0].PipeFlags = 0x00;

		pInfo[0].Pipes[1].MaximumTransferSize = 0x00400000;
		pInfo[0].Pipes[1].MaximumPacketSize = 0x20;
		pInfo[0].Pipes[1].EndpointAddress = 0x02;
		pInfo[0].Pipes[1].Interval = 0x08;
		pInfo[0].Pipes[1].PipeType = (USBD_PIPE_TYPE)0x03;
		pInfo[0].Pipes[1].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0002;
		pInfo[0].Pipes[1].PipeFlags = 0x00;

		pInfo[0].Pipes[2].MaximumTransferSize = 0x00400000;
		pInfo[0].Pipes[2].MaximumPacketSize = 0x20;
		pInfo[0].Pipes[2].EndpointAddress = 0x83;
		pInfo[0].Pipes[2].Interval = 0x08;
		pInfo[0].Pipes[2].PipeType = (USBD_PIPE_TYPE)0x03;
		pInfo[0].Pipes[2].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0083;
		pInfo[0].Pipes[2].PipeFlags = 0x00;

		pInfo[0].Pipes[3].MaximumTransferSize = 0x00400000;
		pInfo[0].Pipes[3].MaximumPacketSize = 0x20;
		pInfo[0].Pipes[3].EndpointAddress = 0x03;
		pInfo[0].Pipes[3].Interval = 0x08;
		pInfo[0].Pipes[3].PipeType = (USBD_PIPE_TYPE)0x03;
		pInfo[0].Pipes[3].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0003;
		pInfo[0].Pipes[3].PipeFlags = 0x00;

		return STATUS_SUCCESS;
	}

	if (pInfo->InterfaceNumber == 2)
	{
		pInfo[0].Class = 0xFF;
		pInfo[0].SubClass = 0x5D;
		pInfo[0].Protocol = 0x02;
		pInfo[0].NumberOfPipes = 0x01;

		pInfo[0].InterfaceHandle = (USBD_INTERFACE_HANDLE)0xFFFF0000;

		pInfo[0].Pipes[0].MaximumTransferSize = 0x00400000;
		pInfo[0].Pipes[0].MaximumPacketSize = 0x20;
		pInfo[0].Pipes[0].EndpointAddress = 0x84;
		pInfo[0].Pipes[0].Interval = 0x04;
		pInfo[0].Pipes[0].PipeType = (USBD_PIPE_TYPE)0x03;
		pInfo[0].Pipes[0].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0084;
		pInfo[0].Pipes[0].PipeFlags = 0x00;

		return STATUS_SUCCESS;
	}

	return STATUS_INVALID_PARAMETER;
}

NTSTATUS ViGEm::Bus::Targets::EmulationTargetXUSB::UsbGetStringDescriptorType(PURB Urb)
{
	UNREFERENCED_PARAMETER(Urb);

	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS ViGEm::Bus::Targets::EmulationTargetXUSB::UsbBulkOrInterruptTransfer(_URB_BULK_OR_INTERRUPT_TRANSFER* pTransfer, WDFREQUEST Request)
{
	NTSTATUS     status = STATUS_SUCCESS;
	WDFREQUEST   notifyRequest;

	// Data coming FROM us TO higher driver
	if (pTransfer->TransferFlags & USBD_TRANSFER_DIRECTION_IN)
	{
		TraceDbg(
			TRACE_USBPDO,
			">> >> >> Incoming request, queuing...");

		auto blobBuffer = static_cast<PUCHAR>(WdfMemoryGetBuffer(this->_InterruptBlobStorage, nullptr));

		if (xusb_is_data_pipe(pTransfer))
		{
			//
			// Send "boot sequence" first, then the actual inputs
			// 
			switch (this->_InterruptInitStage)
			{
			case 0:
				pTransfer->TransferBufferLength = XUSB_INIT_STAGE_SIZE;
				this->_InterruptInitStage++;
				RtlCopyMemory(
					pTransfer->TransferBuffer,
					&blobBuffer[XUSB_BLOB_00_OFFSET],
					XUSB_INIT_STAGE_SIZE
				);
				return STATUS_SUCCESS;
			case 1:
				pTransfer->TransferBufferLength = XUSB_INIT_STAGE_SIZE;
				this->_InterruptInitStage++;
				RtlCopyMemory(
					pTransfer->TransferBuffer,
					&blobBuffer[XUSB_BLOB_01_OFFSET],
					XUSB_INIT_STAGE_SIZE
				);
				return STATUS_SUCCESS;
			case 2:
				pTransfer->TransferBufferLength = XUSB_INIT_STAGE_SIZE;
				this->_InterruptInitStage++;
				RtlCopyMemory(
					pTransfer->TransferBuffer,
					&blobBuffer[XUSB_BLOB_02_OFFSET],
					XUSB_INIT_STAGE_SIZE
				);
				return STATUS_SUCCESS;
			case 3:
				pTransfer->TransferBufferLength = XUSB_INIT_STAGE_SIZE;
				this->_InterruptInitStage++;
				RtlCopyMemory(
					pTransfer->TransferBuffer,
					&blobBuffer[XUSB_BLOB_03_OFFSET],
					XUSB_INIT_STAGE_SIZE
				);
				return STATUS_SUCCESS;
			case 4:
				pTransfer->TransferBufferLength = sizeof(XUSB_INTERRUPT_IN_PACKET);
				this->_InterruptInitStage++;
				RtlCopyMemory(
					pTransfer->TransferBuffer,
					&blobBuffer[XUSB_BLOB_04_OFFSET],
					sizeof(XUSB_INTERRUPT_IN_PACKET)
				);
				return STATUS_SUCCESS;
			case 5:
				pTransfer->TransferBufferLength = XUSB_INIT_STAGE_SIZE;
				this->_InterruptInitStage++;
				RtlCopyMemory(
					pTransfer->TransferBuffer,
					&blobBuffer[XUSB_BLOB_05_OFFSET],
					XUSB_INIT_STAGE_SIZE
				);
				return STATUS_SUCCESS;
			default:
				/* This request is sent periodically and relies on data the "feeder"
				* has to supply, so we queue this request and return with STATUS_PENDING.
				* The request gets completed as soon as the "feeder" sent an update. */
				status = WdfRequestForwardToIoQueue(Request, this->_PendingUsbInRequests);

				return (NT_SUCCESS(status)) ? STATUS_PENDING : status;
			}
		}

		if (xusb_is_control_pipe(pTransfer))
		{
			if (!this->_ReportedCapabilities && pTransfer->TransferBufferLength >= XUSB_INIT_STAGE_SIZE)
			{
				RtlCopyMemory(
					pTransfer->TransferBuffer,
					&blobBuffer[XUSB_BLOB_06_OFFSET],
					XUSB_INIT_STAGE_SIZE
				);

				this->_ReportedCapabilities = TRUE;

				return STATUS_SUCCESS;
			}

			status = WdfRequestForwardToIoQueue(Request, this->_HoldingUsbInRequests);

			return (NT_SUCCESS(status)) ? STATUS_PENDING : status;
		}
	}

	// Data coming FROM the higher driver TO us
	TraceDbg(
		TRACE_USBPDO,
		">> >> >> URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER: Handle %p, Flags %X, Length %d",
		pTransfer->PipeHandle,
		pTransfer->TransferFlags,
		pTransfer->TransferBufferLength);

#pragma region Cache values

	if (pTransfer->TransferBufferLength == XUSB_LEDSET_SIZE) // Led
	{
		auto Buffer = static_cast<PUCHAR>(pTransfer->TransferBuffer);

		TraceDbg(
			TRACE_USBPDO,
			"-- LED Buffer: %02X %02X %02X",
			Buffer[0], Buffer[1], Buffer[2]);

		// extract LED byte to get controller slot
		if (Buffer[0] == 0x01 && Buffer[1] == 0x03 && Buffer[2] >= 0x02)
		{
			if (Buffer[2] == 0x02)this->_LedNumber = 0;
			if (Buffer[2] == 0x03)this->_LedNumber = 1;
			if (Buffer[2] == 0x04)this->_LedNumber = 2;
			if (Buffer[2] == 0x05)this->_LedNumber = 3;

			TraceDbg(
				TRACE_USBPDO,
				"-- LED Number: %d",
				this->_LedNumber);
		}

		//
		// Notify client library that PDO is ready
		// 
		KeSetEvent(&this->_PdoBootNotificationEvent, 0, FALSE);
	}

	// Extract rumble (vibration) information
	if (pTransfer->TransferBufferLength == XUSB_RUMBLE_SIZE)
	{
		auto Buffer = static_cast<PUCHAR>(pTransfer->TransferBuffer);

		TraceDbg(
			TRACE_USBPDO,
			"-- Rumble Buffer: %02X %02X %02X %02X %02X %02X %02X %02X",
			Buffer[0],
			Buffer[1],
			Buffer[2],
			Buffer[3],
			Buffer[4],
			Buffer[5],
			Buffer[6],
			Buffer[7]);

		RtlCopyBytes(this->_Rumble, Buffer, pTransfer->TransferBufferLength);
	}

#pragma endregion

	if (NT_SUCCESS(WdfIoQueueRetrieveNextRequest(
		this->_PendingNotificationRequests,
		&notifyRequest
	)))
	{
		PXUSB_REQUEST_NOTIFICATION notify = nullptr;

		status = WdfRequestRetrieveOutputBuffer(
			notifyRequest,
			sizeof(XUSB_REQUEST_NOTIFICATION),
			reinterpret_cast<PVOID*>(&notify),
			nullptr
		);

		if (NT_SUCCESS(status))
		{
			// Assign values to output buffer
			notify->Size = sizeof(XUSB_REQUEST_NOTIFICATION);
			notify->SerialNo = this->_SerialNo;
			notify->LedNumber = this->_LedNumber;
			notify->LargeMotor = this->_Rumble[3];
			notify->SmallMotor = this->_Rumble[4];

			DumpAsHex("!! XUSB_REQUEST_NOTIFICATION",
			          notify,
			          sizeof(XUSB_REQUEST_NOTIFICATION)
			);

			WdfRequestCompleteWithInformation(notifyRequest, status, notify->Size);
		}
		else
		{
			TraceEvents(TRACE_LEVEL_ERROR,
			            TRACE_USBPDO,
			            "WdfRequestRetrieveOutputBuffer failed with status %!STATUS!",
			            status);
		}
	}
	else
	{
		PVOID clientBuffer, contextBuffer;

		if (NT_SUCCESS(DMF_BufferQueue_Fetch(
			this->_UsbInterruptOutBufferQueue,
			&clientBuffer,
			&contextBuffer
		)) && pTransfer->TransferBufferLength <= MAX_OUT_BUFFER_QUEUE_SIZE)
		{
			RtlCopyMemory(
				clientBuffer,
				pTransfer->TransferBuffer,
				pTransfer->TransferBufferLength
			);

			*static_cast<size_t*>(contextBuffer) = pTransfer->TransferBufferLength;

			TraceDbg(TRACE_USBPDO, "Queued %Iu bytes", pTransfer->TransferBufferLength);

			DMF_BufferQueue_Enqueue(this->_UsbInterruptOutBufferQueue, clientBuffer);
		}
	}

	return status;
}

NTSTATUS ViGEm::Bus::Targets::EmulationTargetXUSB::UsbControlTransfer(PURB Urb)
{
	NTSTATUS status;
	PUCHAR   blobBuffer;

	switch (Urb->UrbControlTransfer.SetupPacket[6])
	{
	case 0x04:

		blobBuffer = static_cast<PUCHAR>(WdfMemoryGetBuffer(this->_InterruptBlobStorage, nullptr));
		//
		// Xenon magic
		// 
		RtlCopyMemory(
			Urb->UrbControlTransfer.TransferBuffer,
			&blobBuffer[XUSB_BLOB_07_OFFSET],
			0x04
		);
		status = STATUS_SUCCESS;

		break;
	case 0x14:
		//
		// This is some weird USB 1.0 condition and _must fail_
		// 
		Urb->UrbControlTransfer.Hdr.Status = USBD_STATUS_STALL_PID;
		status = STATUS_UNSUCCESSFUL;
		break;
	case 0x08:
		//
		// This is some weird USB 1.0 condition and _must fail_
		// 
		Urb->UrbControlTransfer.Hdr.Status = USBD_STATUS_STALL_PID;
		status = STATUS_UNSUCCESSFUL;
		break;
	default:
		status = STATUS_SUCCESS;
		break;
	}

	return status;
}

NTSTATUS ViGEm::Bus::Targets::EmulationTargetXUSB::SubmitReportImpl(PVOID NewReport)
{
	TraceDbg(TRACE_BUSENUM, "%!FUNC! Entry");

	NTSTATUS    status = STATUS_SUCCESS;
	BOOLEAN     changed;
	WDFREQUEST  usbRequest;

	changed = (RtlCompareMemory(&this->_Packet.Report,
		&static_cast<PXUSB_SUBMIT_REPORT>(NewReport)->Report,
		sizeof(XUSB_REPORT)) != sizeof(XUSB_REPORT));

	// Don't waste pending IRP if input hasn't changed
	if (!changed)
	{
		TraceDbg(
			TRACE_BUSENUM,
			"Input report hasn't changed since last update, aborting with %!STATUS!",
			status);
		return status;
	}

	TraceDbg(
		TRACE_BUSENUM,
		"Received new report, processing");

	status = WdfIoQueueRetrieveNextRequest(this->_PendingUsbInRequests, &usbRequest);

	if (!NT_SUCCESS(status))
		return status;

	// Get pending IRP
	PIRP pendingIrp = WdfRequestWdmGetIrp(usbRequest);

	// Get USB request block
	PURB urb = static_cast<PURB>(URB_FROM_IRP(pendingIrp));

	// Get transfer buffer
	auto Buffer = static_cast<PUCHAR>(urb->UrbBulkOrInterruptTransfer.TransferBuffer);

	urb->UrbBulkOrInterruptTransfer.TransferBufferLength = sizeof(XUSB_INTERRUPT_IN_PACKET);

	// Copy submitted report to cache
	RtlCopyBytes(&this->_Packet.Report, &(static_cast<PXUSB_SUBMIT_REPORT>(NewReport))->Report, sizeof(XUSB_REPORT));
	// Copy cached report to URB transfer buffer
	RtlCopyBytes(Buffer, &this->_Packet, sizeof(XUSB_INTERRUPT_IN_PACKET));

	// Complete pending request
	WdfRequestComplete(usbRequest, status);

	TraceDbg(TRACE_BUSENUM, "%!FUNC! Exit with status %!STATUS!", status);

	return status;
}

NTSTATUS ViGEm::Bus::Targets::EmulationTargetXUSB::GetUserIndex(PULONG UserIndex) const
{
	if (!this->IsOwnerProcess())
		return STATUS_ACCESS_DENIED;

	if (!UserIndex)
		return STATUS_INVALID_PARAMETER;
	
	if (this->_LedNumber >= 0)
	{
		*UserIndex = static_cast<ULONG>(this->_LedNumber);
		return STATUS_SUCCESS;
	}

	// If the index is negative at this stage, we've exceeded XUSER_MAX_COUNT
	// and need to fail this request with a distinct status.
	return STATUS_INVALID_DEVICE_OBJECT_PARAMETER;
}

void ViGEm::Bus::Targets::EmulationTargetXUSB::ProcessPendingNotification(WDFQUEUE Queue)
{
	NTSTATUS status;
	WDFREQUEST request;
	PVOID clientBuffer, contextBuffer;
	size_t bufferLength;
	PXUSB_REQUEST_NOTIFICATION notify = nullptr;

	TraceDbg(TRACE_BUSENUM, "%!FUNC! Entry");
	
	//
	// Loop through and drain all queued requests until buffer is empty
	// 
	while (NT_SUCCESS(WdfIoQueueRetrieveNextRequest(Queue, &request)))
	{
		status = DMF_BufferQueue_Dequeue(
			this->_UsbInterruptOutBufferQueue,
			&clientBuffer,
			&contextBuffer
		);

		//
		// Shouldn't happen, but if so, error out
		// 
		if (!NT_SUCCESS(status))
		{
			//
			// Don't requeue request as we maya be out of order now
			// 
			WdfRequestComplete(request, status);
			continue;
		}

		//
		// Actual buffer length
		// 
		bufferLength = *static_cast<size_t*>(contextBuffer);

		//
		// Validate packet
		// 
		if (bufferLength != XUSB_RUMBLE_SIZE)
		{
			DMF_BufferQueue_Reuse(this->_UsbInterruptOutBufferQueue, clientBuffer);
			WdfRequestComplete(request, STATUS_INVALID_BUFFER_SIZE);
			break; // await callback getting fired again
		}

		if (NT_SUCCESS(WdfRequestRetrieveOutputBuffer(
			request,
			sizeof(XUSB_REQUEST_NOTIFICATION),
			reinterpret_cast<PVOID*>(&notify),
			nullptr
		)))
		{
			// 
			// Assign values to output buffer
			// 
			notify->Size = sizeof(XUSB_REQUEST_NOTIFICATION);
			notify->SerialNo = this->_SerialNo;
			notify->LedNumber = this->_LedNumber;
			notify->LargeMotor = static_cast<PUCHAR>(clientBuffer)[3];
			notify->SmallMotor = static_cast<PUCHAR>(clientBuffer)[4];

			DumpAsHex("!! XUSB_REQUEST_NOTIFICATION", 
				notify, 
				sizeof(XUSB_REQUEST_NOTIFICATION)
			);
			
			WdfRequestCompleteWithInformation(request, status, notify->Size);
		}

		DMF_BufferQueue_Reuse(this->_UsbInterruptOutBufferQueue, clientBuffer);

		//
		// If no more buffer to process, exit loop and await next callback
		// 
		if (DMF_BufferQueue_Count(this->_UsbInterruptOutBufferQueue) == 0)
		{
			break;
		}
	}

	TraceDbg(TRACE_BUSENUM, "%!FUNC! Exit");
}
