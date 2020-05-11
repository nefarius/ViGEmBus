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

#include "Ds4Pdo.hpp"
#include "trace.h"
#include "Ds4Pdo.tmh"
#define NTSTRSAFE_LIB
#include <ntstrsafe.h>
#include <hidclass.h>



PCWSTR ViGEm::Bus::Targets::EmulationTargetDS4::_deviceDescription = L"Virtual DualShock 4 Controller";

ViGEm::Bus::Targets::EmulationTargetDS4::EmulationTargetDS4(ULONG Serial, LONG SessionId, USHORT VendorId,
                                                            USHORT ProductId) : EmulationTargetPDO(
	Serial, SessionId, VendorId, ProductId)
{
	_TargetType = DualShock4Wired;
	_UsbConfigurationDescriptionSize = DS4_DESCRIPTOR_SIZE;
}

NTSTATUS ViGEm::Bus::Targets::EmulationTargetDS4::PdoPrepareDevice(PWDFDEVICE_INIT DeviceInit,
	PUNICODE_STRING DeviceId, PUNICODE_STRING DeviceDescription)
{
	NTSTATUS status;
	DECLARE_UNICODE_STRING_SIZE(buffer, _maxHardwareIdLength);

	// prepare device description
	status = RtlUnicodeStringInit(DeviceDescription, _deviceDescription);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR,
			TRACE_DS4,
			"RtlUnicodeStringInit failed with status %!STATUS!",
			status);
		return status;
	}

	// Set hardware IDs
	RtlUnicodeStringPrintf(&buffer, L"USB\\VID_%04X&PID_%04X&REV_0100",
		this->_VendorId, this->_ProductId);

	status = WdfPdoInitAddHardwareID(DeviceInit, &buffer);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR,
			TRACE_DS4,
			"WdfPdoInitAddHardwareID failed with status %!STATUS!",
			status);
		return status;
	}

	RtlUnicodeStringCopy(DeviceId, &buffer);

	RtlUnicodeStringPrintf(&buffer, L"USB\\VID_%04X&PID_%04X",
		this->_VendorId, this->_ProductId);

	status = WdfPdoInitAddHardwareID(DeviceInit, &buffer);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR,
			TRACE_DS4,
			"WdfPdoInitAddHardwareID failed with status %!STATUS!",
			status);
		return status;
	}

	// Set compatible IDs
	RtlUnicodeStringInit(&buffer, L"USB\\Class_03&SubClass_00&Prot_00");

	status = WdfPdoInitAddCompatibleID(DeviceInit, &buffer);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR,
			TRACE_DS4,
			"WdfPdoInitAddCompatibleID (#01) failed with status %!STATUS!",
			status);
		return status;
	}

	RtlUnicodeStringInit(&buffer, L"USB\\Class_03&SubClass_00");

	status = WdfPdoInitAddCompatibleID(DeviceInit, &buffer);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR,
			TRACE_DS4,
			"WdfPdoInitAddCompatibleID (#02) failed with status %!STATUS!",
			status);
		return status;
	}

	RtlUnicodeStringInit(&buffer, L"USB\\Class_03");

	status = WdfPdoInitAddCompatibleID(DeviceInit, &buffer);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR,
			TRACE_DS4,
			"WdfPdoInitAddCompatibleID (#03) failed with status %!STATUS!",
			status);
		return status;
	}

	return STATUS_SUCCESS;
}

NTSTATUS ViGEm::Bus::Targets::EmulationTargetDS4::PdoPrepareHardware()
{
	WDF_QUERY_INTERFACE_CONFIG ifaceCfg;
	INTERFACE devinterfaceHid;

	devinterfaceHid.Size = sizeof(INTERFACE);
	devinterfaceHid.Version = 1;
	devinterfaceHid.Context = static_cast<PVOID>(this->_PdoDevice);

	devinterfaceHid.InterfaceReference = WdfDeviceInterfaceReferenceNoOp;
	devinterfaceHid.InterfaceDereference = WdfDeviceInterfaceDereferenceNoOp;

	// Expose GUID_DEVINTERFACE_HID so HIDUSB can initialize
	WDF_QUERY_INTERFACE_CONFIG_INIT(
		&ifaceCfg,
		(PINTERFACE)&devinterfaceHid,
		&GUID_DEVINTERFACE_HID,
		NULL
	);

	NTSTATUS status = WdfDeviceAddQueryInterface(this->_PdoDevice, &ifaceCfg);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR,
			TRACE_DS4,
			"WdfDeviceAddQueryInterface failed with status %!STATUS!",
			status);
		return status;
	}

	// Set default HID input report (everything zero`d)
	UCHAR DefaultHidReport[DS4_REPORT_SIZE] =
	{
		0x01, 0x82, 0x7F, 0x7E, 0x80, 0x08, 0x00, 0x58,
		0x00, 0x00, 0xFD, 0x63, 0x06, 0x03, 0x00, 0xFE,
		0xFF, 0xFC, 0xFF, 0x79, 0xFD, 0x1B, 0x14, 0xD1,
		0xE9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1B, 0x00,
		0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80,
		0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00,
		0x80, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
		0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00
	};

	// Initialize HID reports to defaults
	RtlCopyBytes(this->_Report, DefaultHidReport, DS4_REPORT_SIZE);
	RtlZeroMemory(&this->_OutputReport, sizeof(DS4_OUTPUT_REPORT));

	// Start pending IRP queue flush timer
	WdfTimerStart(this->_PendingUsbInRequestsTimer, DS4_QUEUE_FLUSH_PERIOD);

	return STATUS_SUCCESS;
}

NTSTATUS ViGEm::Bus::Targets::EmulationTargetDS4::PdoInitContext()
{
	NTSTATUS            status;

	// Initialize periodic timer
	WDF_TIMER_CONFIG timerConfig;
	WDF_TIMER_CONFIG_INIT_PERIODIC(
		&timerConfig,
		PendingUsbRequestsTimerFunc,
		DS4_QUEUE_FLUSH_PERIOD
	);

	// Timer object attributes
	WDF_OBJECT_ATTRIBUTES timerAttribs;
	WDF_OBJECT_ATTRIBUTES_INIT(&timerAttribs);

	// PDO is parent
	timerAttribs.ParentObject = this->_PdoDevice;

	// Create timer
	status = WdfTimerCreate(
		&timerConfig,
		&timerAttribs,
		&this->_PendingUsbInRequestsTimer
	);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR,
			TRACE_DS4,
			"WdfTimerCreate failed with status %!STATUS!",
			status);
		return status;
	}

	// Load/generate MAC address

	// TODO: tidy up this region

	WDFKEY keyParams, keyTargets, keyDS, keySerial;
	UNICODE_STRING keyName, valueName;

	status = WdfDriverOpenParametersRegistryKey(
		WdfGetDriver(),
		STANDARD_RIGHTS_ALL,
		WDF_NO_OBJECT_ATTRIBUTES,
		&keyParams
	);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR,
			TRACE_DS4,
			"WdfDriverOpenParametersRegistryKey failed with status %!STATUS!",
			status);
		return status;
	}

	RtlUnicodeStringInit(&keyName, L"Targets");

	status = WdfRegistryCreateKey(
		keyParams,
		&keyName,
		KEY_ALL_ACCESS,
		REG_OPTION_NON_VOLATILE,
		NULL,
		WDF_NO_OBJECT_ATTRIBUTES,
		&keyTargets
	);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR,
			TRACE_DS4,
			"WdfRegistryCreateKey failed with status %!STATUS!",
			status);
		return status;
	}

	RtlUnicodeStringInit(&keyName, L"DualShock");

	status = WdfRegistryCreateKey(
		keyTargets,
		&keyName,
		KEY_ALL_ACCESS,
		REG_OPTION_NON_VOLATILE,
		NULL,
		WDF_NO_OBJECT_ATTRIBUTES,
		&keyDS
	);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR,
			TRACE_DS4,
			"WdfRegistryCreateKey failed with status %!STATUS!",
			status);
		return status;
	}

	DECLARE_UNICODE_STRING_SIZE(serialPath, 4);
	RtlUnicodeStringPrintf(&serialPath, L"%04d", this->_SerialNo);

	status = WdfRegistryCreateKey(
		keyDS,
		&serialPath,
		KEY_ALL_ACCESS,
		REG_OPTION_NON_VOLATILE,
		NULL,
		WDF_NO_OBJECT_ATTRIBUTES,
		&keySerial
	);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR,
			TRACE_DS4,
			"WdfRegistryCreateKey failed with status %!STATUS!",
			status);
		return status;
	}

	RtlUnicodeStringInit(&valueName, L"TargetMacAddress");

	status = WdfRegistryQueryValue(
		keySerial,
		&valueName,
		sizeof(MAC_ADDRESS),
		&this->_TargetMacAddress,
		NULL,
		NULL
	);

	TraceEvents(TRACE_LEVEL_INFORMATION,
		TRACE_DS4,
		"MAC-Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
		this->_TargetMacAddress.Vendor0,
		this->_TargetMacAddress.Vendor1,
		this->_TargetMacAddress.Vendor2,
		this->_TargetMacAddress.Nic0,
		this->_TargetMacAddress.Nic1,
		this->_TargetMacAddress.Nic2);

	if (status == STATUS_OBJECT_NAME_NOT_FOUND)
	{
		GenerateRandomMacAddress(&this->_TargetMacAddress);

		status = WdfRegistryAssignValue(
			keySerial,
			&valueName,
			REG_BINARY,
			sizeof(MAC_ADDRESS),
			static_cast<PVOID>(&this->_TargetMacAddress)
		);
		if (!NT_SUCCESS(status))
		{
			TraceEvents(TRACE_LEVEL_ERROR,
				TRACE_DS4,
				"WdfRegistryAssignValue failed with status %!STATUS!",
				status);
			return status;
		}
	}
	else if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR,
			TRACE_DS4,
			"WdfRegistryQueryValue failed with status %!STATUS!",
			status);
		return status;
	}

	WdfRegistryClose(keySerial);
	WdfRegistryClose(keyDS);
	WdfRegistryClose(keyTargets);
	WdfRegistryClose(keyParams);

	return STATUS_SUCCESS;
}

VOID ViGEm::Bus::Targets::EmulationTargetDS4::GetConfigurationDescriptorType(PUCHAR Buffer, ULONG Length)
{
	UCHAR Ds4DescriptorData[DS4_DESCRIPTOR_SIZE] =
	{
		0x09,        // bLength
		0x02,        // bDescriptorType (Configuration)
		0x29, 0x00,  // wTotalLength 41
		0x01,        // bNumInterfaces 1
		0x01,        // bConfigurationValue
		0x00,        // iConfiguration (String Index)
		0xC0,        // bmAttributes Self Powered
		0xFA,        // bMaxPower 500mA

		0x09,        // bLength
		0x04,        // bDescriptorType (Interface)
		0x00,        // bInterfaceNumber 0
		0x00,        // bAlternateSetting
		0x02,        // bNumEndpoints 2
		0x03,        // bInterfaceClass
		0x00,        // bInterfaceSubClass
		0x00,        // bInterfaceProtocol
		0x00,        // iInterface (String Index)

		0x09,        // bLength
		0x21,        // bDescriptorType (HID)
		0x11, 0x01,  // bcdHID 1.11
		0x00,        // bCountryCode
		0x01,        // bNumDescriptors
		0x22,        // bDescriptorType[0] (HID)
		0xD3, 0x01,  // wDescriptorLength[0] 467

		0x07,        // bLength
		0x05,        // bDescriptorType (Endpoint)
		0x84,        // bEndpointAddress (IN/D2H)
		0x03,        // bmAttributes (Interrupt)
		0x40, 0x00,  // wMaxPacketSize 64
		0x05,        // bInterval 5 (unit depends on device speed)

		0x07,        // bLength
		0x05,        // bDescriptorType (Endpoint)
		0x03,        // bEndpointAddress (OUT/H2D)
		0x03,        // bmAttributes (Interrupt)
		0x40, 0x00,  // wMaxPacketSize 64
		0x05,        // bInterval 5 (unit depends on device speed)

					 // 41 bytes

					 // best guess: USB Standard Descriptor
	};

	RtlCopyBytes(Buffer, Ds4DescriptorData, Length);
}

NTSTATUS ViGEm::Bus::Targets::EmulationTargetDS4::UsbGetDeviceDescriptorType(PUSB_DEVICE_DESCRIPTOR pDescriptor)
{
	pDescriptor->bLength = 0x12;
	pDescriptor->bDescriptorType = USB_DEVICE_DESCRIPTOR_TYPE;
	pDescriptor->bcdUSB = 0x0200; // USB v2.0
	pDescriptor->bDeviceClass = 0x00; // per Interface
	pDescriptor->bDeviceSubClass = 0x00;
	pDescriptor->bDeviceProtocol = 0x00;
	pDescriptor->bMaxPacketSize0 = 0x40;
	pDescriptor->idVendor = this->_VendorId;
	pDescriptor->idProduct = this->_ProductId;
	pDescriptor->bcdDevice = 0x0100;
	pDescriptor->iManufacturer = 0x01;
	pDescriptor->iProduct = 0x02;
	pDescriptor->iSerialNumber = 0x00;
	pDescriptor->bNumConfigurations = 0x01;

	return STATUS_SUCCESS;
}

NTSTATUS ViGEm::Bus::Targets::EmulationTargetDS4::SelectConfiguration(PURB Urb)
{
	if (Urb->UrbHeader.Length < DS4_CONFIGURATION_SIZE)
	{
		TraceEvents(TRACE_LEVEL_WARNING,
			TRACE_USBPDO,
			">> >> >> URB_FUNCTION_SELECT_CONFIGURATION: Invalid ConfigurationDescriptor");
		return STATUS_INVALID_PARAMETER;
	}

	PUSBD_INTERFACE_INFORMATION pInfo = &Urb->UrbSelectConfiguration.Interface;

	TraceEvents(TRACE_LEVEL_VERBOSE,
		TRACE_DS4,
		">> >> >> URB_FUNCTION_SELECT_CONFIGURATION: Length %d, Interface %d, Alternate %d, Pipes %d",
		(int)pInfo->Length,
		(int)pInfo->InterfaceNumber,
		(int)pInfo->AlternateSetting,
		pInfo->NumberOfPipes);

	pInfo->Class = 0x03; // HID
	pInfo->SubClass = 0x00;
	pInfo->Protocol = 0x00;

	pInfo->InterfaceHandle = (USBD_INTERFACE_HANDLE)0xFFFF0000;

	pInfo->Pipes[0].MaximumTransferSize = 0x00400000;
	pInfo->Pipes[0].MaximumPacketSize = 0x40;
	pInfo->Pipes[0].EndpointAddress = 0x84;
	pInfo->Pipes[0].Interval = 0x05;
	pInfo->Pipes[0].PipeType = (USBD_PIPE_TYPE)0x03;
	pInfo->Pipes[0].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0084;
	pInfo->Pipes[0].PipeFlags = 0x00;

	pInfo->Pipes[1].MaximumTransferSize = 0x00400000;
	pInfo->Pipes[1].MaximumPacketSize = 0x40;
	pInfo->Pipes[1].EndpointAddress = 0x03;
	pInfo->Pipes[1].Interval = 0x05;
	pInfo->Pipes[1].PipeType = (USBD_PIPE_TYPE)0x03;
	pInfo->Pipes[1].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0003;
	pInfo->Pipes[1].PipeFlags = 0x00;

	return STATUS_SUCCESS;
}

void ViGEm::Bus::Targets::EmulationTargetDS4::AbortPipe()
{
	// Higher driver shutting down, emptying PDOs queues
	WdfTimerStop(this->_PendingUsbInRequestsTimer, TRUE);
}

NTSTATUS ViGEm::Bus::Targets::EmulationTargetDS4::UsbClassInterface(PURB Urb)
{
	struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST* pRequest = &Urb->UrbControlVendorClassRequest;

	TraceEvents(TRACE_LEVEL_VERBOSE,
		TRACE_USBPDO,
		">> >> >> URB_FUNCTION_CLASS_INTERFACE");
	TraceEvents(TRACE_LEVEL_VERBOSE,
		TRACE_USBPDO,
		">> >> >> TransferFlags = 0x%X, Request = 0x%X, Value = 0x%X, Index = 0x%X, BufLen = %d",
		pRequest->TransferFlags,
		pRequest->Request,
		pRequest->Value,
		pRequest->Index,
		pRequest->TransferBufferLength);

	switch (pRequest->Request)
	{
	case HID_REQUEST_GET_REPORT:
	{
		UCHAR reportId = hid_get_report_id(pRequest);
		UCHAR reportType = hid_get_report_type(pRequest);

		TraceEvents(TRACE_LEVEL_VERBOSE,
			TRACE_USBPDO,
			">> >> >> >> GET_REPORT(%d): %d",
			reportType, reportId);

		switch (reportType)
		{
		case HID_REPORT_TYPE_FEATURE:
		{
			switch (reportId)
			{
			case HID_REPORT_ID_0:
			{
				// Source: http://eleccelerator.com/wiki/index.php?title=DualShock_4#Class_Requests
				UCHAR Response[] =
				{
					0xA3, 0x41, 0x75, 0x67, 0x20, 0x20, 0x33, 0x20,
					0x32, 0x30, 0x31, 0x33, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x30, 0x37, 0x3A, 0x30, 0x31, 0x3A, 0x31,
					0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x01, 0x00, 0x31, 0x03, 0x00, 0x00,
					0x00, 0x49, 0x00, 0x05, 0x00, 0x00, 0x80, 0x03,
					0x00
				};

				pRequest->TransferBufferLength = ARRAYSIZE(Response);
				RtlCopyBytes(pRequest->TransferBuffer, Response, ARRAYSIZE(Response));

				break;
			}
			case HID_REPORT_ID_1:
			{
				// Source: http://eleccelerator.com/wiki/index.php?title=DualShock_4#Class_Requests
				UCHAR Response[] =
				{
					0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87,
					0x22, 0x7B, 0xDD, 0xB2, 0x22, 0x47, 0xDD, 0xBD,
					0x22, 0x43, 0xDD, 0x1C, 0x02, 0x1C, 0x02, 0x7F,
					0x1E, 0x2E, 0xDF, 0x60, 0x1F, 0x4C, 0xE0, 0x3A,
					0x1D, 0xC6, 0xDE, 0x08, 0x00
				};

				pRequest->TransferBufferLength = ARRAYSIZE(Response);
				RtlCopyBytes(pRequest->TransferBuffer, Response, ARRAYSIZE(Response));

				break;
			}
			case HID_REPORT_MAC_ADDRESSES_ID:
			{
				// Source: http://eleccelerator.com/wiki/index.php?title=DualShock_4#Class_Requests
				UCHAR Response[] =
				{
					0x12, 0x8B, 0x09, 0x07, 0x6D, 0x66, 0x1C, 0x08,
					0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
				};

				// Insert (auto-generated) target MAC address into response
				RtlCopyBytes(Response + 1, &this->_TargetMacAddress, sizeof(MAC_ADDRESS));
				// Adjust byte order
				ReverseByteArray(Response + 1, sizeof(MAC_ADDRESS));

				// Insert (auto-generated) host MAC address into response
				RtlCopyBytes(Response + 10, &this->_HostMacAddress, sizeof(MAC_ADDRESS));
				// Adjust byte order
				ReverseByteArray(Response + 10, sizeof(MAC_ADDRESS));

				pRequest->TransferBufferLength = ARRAYSIZE(Response);
				RtlCopyBytes(pRequest->TransferBuffer, Response, ARRAYSIZE(Response));

				break;
			}
			default:
				break;
			}
			break;
		}
		default:
			break;
		}

		break;
	}
	case HID_REQUEST_SET_REPORT:
	{
		UCHAR reportId = hid_get_report_id(pRequest);
		UCHAR reportType = hid_get_report_type(pRequest);

		TraceEvents(TRACE_LEVEL_VERBOSE,
			TRACE_USBPDO,
			">> >> >> >> SET_REPORT(%d): %d",
			reportType, reportId);

		switch (reportType)
		{
		case HID_REPORT_TYPE_FEATURE:
		{
			switch (reportId)
			{
			case HID_REPORT_ID_3:
			{
				// Source: http://eleccelerator.com/wiki/index.php?title=DualShock_4#Class_Requests
				UCHAR Response[] =
				{
					0x13, 0xAC, 0x9E, 0x17, 0x94, 0x05, 0xB0, 0x56,
					0xE8, 0x81, 0x38, 0x08, 0x06, 0x51, 0x41, 0xC0,
					0x7F, 0x12, 0xAA, 0xD9, 0x66, 0x3C, 0xCE
				};

				pRequest->TransferBufferLength = ARRAYSIZE(Response);
				RtlCopyBytes(pRequest->TransferBuffer, Response, ARRAYSIZE(Response));

				break;
			}
			case HID_REPORT_ID_4:
			{
				// Source: http://eleccelerator.com/wiki/index.php?title=DualShock_4#Class_Requests
				UCHAR Response[] =
				{
					0x14, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00
				};

				pRequest->TransferBufferLength = ARRAYSIZE(Response);
				RtlCopyBytes(pRequest->TransferBuffer, Response, ARRAYSIZE(Response));

				break;
			}
			default:
				break;
			}
			break;
		}
		default:
			break;
		}

		break;
	}
	default:
		break;
	}

	return STATUS_SUCCESS;
}

NTSTATUS ViGEm::Bus::Targets::EmulationTargetDS4::UsbGetDescriptorFromInterface(PURB Urb)
{
	NTSTATUS status = STATUS_INVALID_PARAMETER;
	UCHAR Ds4HidReportDescriptor[] =
	{
		0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
		0x09, 0x05,        // Usage (Game Pad)
		0xA1, 0x01,        // Collection (Application)
		0x85, 0x01,        //   Report ID (1)
		0x09, 0x30,        //   Usage (X)
		0x09, 0x31,        //   Usage (Y)
		0x09, 0x32,        //   Usage (Z)
		0x09, 0x35,        //   Usage (Rz)
		0x15, 0x00,        //   Logical Minimum (0)
		0x26, 0xFF, 0x00,  //   Logical Maximum (255)
		0x75, 0x08,        //   Report Size (8)
		0x95, 0x04,        //   Report Count (4)
		0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
		0x09, 0x39,        //   Usage (Hat switch)
		0x15, 0x00,        //   Logical Minimum (0)
		0x25, 0x07,        //   Logical Maximum (7)
		0x35, 0x00,        //   Physical Minimum (0)
		0x46, 0x3B, 0x01,  //   Physical Maximum (315)
		0x65, 0x14,        //   Unit (System: English Rotation, Length: Centimeter)
		0x75, 0x04,        //   Report Size (4)
		0x95, 0x01,        //   Report Count (1)
		0x81, 0x42,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)
		0x65, 0x00,        //   Unit (None)
		0x05, 0x09,        //   Usage Page (Button)
		0x19, 0x01,        //   Usage Minimum (0x01)
		0x29, 0x0E,        //   Usage Maximum (0x0E)
		0x15, 0x00,        //   Logical Minimum (0)
		0x25, 0x01,        //   Logical Maximum (1)
		0x75, 0x01,        //   Report Size (1)
		0x95, 0x0E,        //   Report Count (14)
		0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
		0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
		0x09, 0x20,        //   Usage (0x20)
		0x75, 0x06,        //   Report Size (6)
		0x95, 0x01,        //   Report Count (1)
		0x15, 0x00,        //   Logical Minimum (0)
		0x25, 0x7F,        //   Logical Maximum (127)
		0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
		0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
		0x09, 0x33,        //   Usage (Rx)
		0x09, 0x34,        //   Usage (Ry)
		0x15, 0x00,        //   Logical Minimum (0)
		0x26, 0xFF, 0x00,  //   Logical Maximum (255)
		0x75, 0x08,        //   Report Size (8)
		0x95, 0x02,        //   Report Count (2)
		0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
		0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
		0x09, 0x21,        //   Usage (0x21)
		0x95, 0x36,        //   Report Count (54)
		0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
		0x85, 0x05,        //   Report ID (5)
		0x09, 0x22,        //   Usage (0x22)
		0x95, 0x1F,        //   Report Count (31)
		0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0x04,        //   Report ID (4)
		0x09, 0x23,        //   Usage (0x23)
		0x95, 0x24,        //   Report Count (36)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0x02,        //   Report ID (2)
		0x09, 0x24,        //   Usage (0x24)
		0x95, 0x24,        //   Report Count (36)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0x08,        //   Report ID (8)
		0x09, 0x25,        //   Usage (0x25)
		0x95, 0x03,        //   Report Count (3)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0x10,        //   Report ID (16)
		0x09, 0x26,        //   Usage (0x26)
		0x95, 0x04,        //   Report Count (4)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0x11,        //   Report ID (17)
		0x09, 0x27,        //   Usage (0x27)
		0x95, 0x02,        //   Report Count (2)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0x12,        //   Report ID (18)
		0x06, 0x02, 0xFF,  //   Usage Page (Vendor Defined 0xFF02)
		0x09, 0x21,        //   Usage (0x21)
		0x95, 0x0F,        //   Report Count (15)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0x13,        //   Report ID (19)
		0x09, 0x22,        //   Usage (0x22)
		0x95, 0x16,        //   Report Count (22)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0x14,        //   Report ID (20)
		0x06, 0x05, 0xFF,  //   Usage Page (Vendor Defined 0xFF05)
		0x09, 0x20,        //   Usage (0x20)
		0x95, 0x10,        //   Report Count (16)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0x15,        //   Report ID (21)
		0x09, 0x21,        //   Usage (0x21)
		0x95, 0x2C,        //   Report Count (44)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x06, 0x80, 0xFF,  //   Usage Page (Vendor Defined 0xFF80)
		0x85, 0x80,        //   Report ID (128)
		0x09, 0x20,        //   Usage (0x20)
		0x95, 0x06,        //   Report Count (6)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0x81,        //   Report ID (129)
		0x09, 0x21,        //   Usage (0x21)
		0x95, 0x06,        //   Report Count (6)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0x82,        //   Report ID (130)
		0x09, 0x22,        //   Usage (0x22)
		0x95, 0x05,        //   Report Count (5)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0x83,        //   Report ID (131)
		0x09, 0x23,        //   Usage (0x23)
		0x95, 0x01,        //   Report Count (1)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0x84,        //   Report ID (132)
		0x09, 0x24,        //   Usage (0x24)
		0x95, 0x04,        //   Report Count (4)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0x85,        //   Report ID (133)
		0x09, 0x25,        //   Usage (0x25)
		0x95, 0x06,        //   Report Count (6)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0x86,        //   Report ID (134)
		0x09, 0x26,        //   Usage (0x26)
		0x95, 0x06,        //   Report Count (6)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0x87,        //   Report ID (135)
		0x09, 0x27,        //   Usage (0x27)
		0x95, 0x23,        //   Report Count (35)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0x88,        //   Report ID (136)
		0x09, 0x28,        //   Usage (0x28)
		0x95, 0x22,        //   Report Count (34)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0x89,        //   Report ID (137)
		0x09, 0x29,        //   Usage (0x29)
		0x95, 0x02,        //   Report Count (2)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0x90,        //   Report ID (144)
		0x09, 0x30,        //   Usage (0x30)
		0x95, 0x05,        //   Report Count (5)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0x91,        //   Report ID (145)
		0x09, 0x31,        //   Usage (0x31)
		0x95, 0x03,        //   Report Count (3)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0x92,        //   Report ID (146)
		0x09, 0x32,        //   Usage (0x32)
		0x95, 0x03,        //   Report Count (3)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0x93,        //   Report ID (147)
		0x09, 0x33,        //   Usage (0x33)
		0x95, 0x0C,        //   Report Count (12)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0xA0,        //   Report ID (160)
		0x09, 0x40,        //   Usage (0x40)
		0x95, 0x06,        //   Report Count (6)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0xA1,        //   Report ID (161)
		0x09, 0x41,        //   Usage (0x41)
		0x95, 0x01,        //   Report Count (1)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0xA2,        //   Report ID (162)
		0x09, 0x42,        //   Usage (0x42)
		0x95, 0x01,        //   Report Count (1)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0xA3,        //   Report ID (163)
		0x09, 0x43,        //   Usage (0x43)
		0x95, 0x30,        //   Report Count (48)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0xA4,        //   Report ID (164)
		0x09, 0x44,        //   Usage (0x44)
		0x95, 0x0D,        //   Report Count (13)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0xA5,        //   Report ID (165)
		0x09, 0x45,        //   Usage (0x45)
		0x95, 0x15,        //   Report Count (21)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0xA6,        //   Report ID (166)
		0x09, 0x46,        //   Usage (0x46)
		0x95, 0x15,        //   Report Count (21)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0xF0,        //   Report ID (240)
		0x09, 0x47,        //   Usage (0x47)
		0x95, 0x3F,        //   Report Count (63)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0xF1,        //   Report ID (241)
		0x09, 0x48,        //   Usage (0x48)
		0x95, 0x3F,        //   Report Count (63)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0xF2,        //   Report ID (242)
		0x09, 0x49,        //   Usage (0x49)
		0x95, 0x0F,        //   Report Count (15)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0xA7,        //   Report ID (167)
		0x09, 0x4A,        //   Usage (0x4A)
		0x95, 0x01,        //   Report Count (1)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0xA8,        //   Report ID (168)
		0x09, 0x4B,        //   Usage (0x4B)
		0x95, 0x01,        //   Report Count (1)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0xA9,        //   Report ID (169)
		0x09, 0x4C,        //   Usage (0x4C)
		0x95, 0x08,        //   Report Count (8)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0xAA,        //   Report ID (170)
		0x09, 0x4E,        //   Usage (0x4E)
		0x95, 0x01,        //   Report Count (1)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0xAB,        //   Report ID (171)
		0x09, 0x4F,        //   Usage (0x4F)
		0x95, 0x39,        //   Report Count (57)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0xAC,        //   Report ID (172)
		0x09, 0x50,        //   Usage (0x50)
		0x95, 0x39,        //   Report Count (57)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0xAD,        //   Report ID (173)
		0x09, 0x51,        //   Usage (0x51)
		0x95, 0x0B,        //   Report Count (11)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0xAE,        //   Report ID (174)
		0x09, 0x52,        //   Usage (0x52)
		0x95, 0x01,        //   Report Count (1)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0xAF,        //   Report ID (175)
		0x09, 0x53,        //   Usage (0x53)
		0x95, 0x02,        //   Report Count (2)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x85, 0xB0,        //   Report ID (176)
		0x09, 0x54,        //   Usage (0x54)
		0x95, 0x3F,        //   Report Count (63)
		0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0xC0,              // End Collection
	};

	struct _URB_CONTROL_DESCRIPTOR_REQUEST* pRequest = &Urb->UrbControlDescriptorRequest;

	TraceEvents(TRACE_LEVEL_VERBOSE,
		TRACE_USBPDO,
		">> >> >> _URB_CONTROL_DESCRIPTOR_REQUEST: Buffer Length %d",
		pRequest->TransferBufferLength);

	if (pRequest->TransferBufferLength >= ARRAYSIZE(Ds4HidReportDescriptor))
	{
		RtlCopyMemory(pRequest->TransferBuffer, Ds4HidReportDescriptor, ARRAYSIZE(Ds4HidReportDescriptor));
		status = STATUS_SUCCESS;

		//
		// Notify client library that PDO is ready
		// 
		KeSetEvent(&this->_PdoBootNotificationEvent, 0, FALSE);
	}

	return status;
}

NTSTATUS ViGEm::Bus::Targets::EmulationTargetDS4::UsbSelectInterface(PURB Urb)
{
	UNREFERENCED_PARAMETER(Urb);

	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS ViGEm::Bus::Targets::EmulationTargetDS4::UsbGetStringDescriptorType(PURB Urb)
{
	TraceEvents(TRACE_LEVEL_VERBOSE,
		TRACE_USBPDO,
		"Index = %d",
		Urb->UrbControlDescriptorRequest.Index);

	switch (Urb->UrbControlDescriptorRequest.Index)
	{
	case 0:
	{
		// "American English"
		UCHAR LangId[] =
		{
			0x04, 0x03, 0x09, 0x04
		};

		Urb->UrbControlDescriptorRequest.TransferBufferLength = ARRAYSIZE(LangId);
		RtlCopyBytes(Urb->UrbControlDescriptorRequest.TransferBuffer, LangId, ARRAYSIZE(LangId));

		break;
	}
	case 1:
	{
		TraceEvents(TRACE_LEVEL_VERBOSE,
			TRACE_USBPDO,
			"LanguageId = 0x%X",
			Urb->UrbControlDescriptorRequest.LanguageId);

		if (Urb->UrbControlDescriptorRequest.TransferBufferLength < DS4_MANUFACTURER_NAME_LENGTH)
		{
			auto pDesc = static_cast<PUSB_STRING_DESCRIPTOR>(Urb->UrbControlDescriptorRequest.TransferBuffer);
			pDesc->bLength = DS4_MANUFACTURER_NAME_LENGTH;
			break;
		}

		// "Sony Computer Entertainment"
		UCHAR ManufacturerString[DS4_MANUFACTURER_NAME_LENGTH] =
		{
			0x38, 0x03, 0x53, 0x00, 0x6F, 0x00, 0x6E, 0x00,
			0x79, 0x00, 0x20, 0x00, 0x43, 0x00, 0x6F, 0x00,
			0x6D, 0x00, 0x70, 0x00, 0x75, 0x00, 0x74, 0x00,
			0x65, 0x00, 0x72, 0x00, 0x20, 0x00, 0x45, 0x00,
			0x6E, 0x00, 0x74, 0x00, 0x65, 0x00, 0x72, 0x00,
			0x74, 0x00, 0x61, 0x00, 0x69, 0x00, 0x6E, 0x00,
			0x6D, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x74, 0x00
		};

		Urb->UrbControlDescriptorRequest.TransferBufferLength = DS4_MANUFACTURER_NAME_LENGTH;
		RtlCopyBytes(Urb->UrbControlDescriptorRequest.TransferBuffer, ManufacturerString, DS4_MANUFACTURER_NAME_LENGTH);

		break;
	}
	case 2:
	{
		TraceEvents(TRACE_LEVEL_VERBOSE,
			TRACE_USBPDO,
			"LanguageId = 0x%X",
			Urb->UrbControlDescriptorRequest.LanguageId);

		if (Urb->UrbControlDescriptorRequest.TransferBufferLength < DS4_PRODUCT_NAME_LENGTH)
		{
			auto pDesc = static_cast<PUSB_STRING_DESCRIPTOR>(Urb->UrbControlDescriptorRequest.TransferBuffer);
			pDesc->bLength = DS4_PRODUCT_NAME_LENGTH;
			break;
		}

		// "Wireless Controller"
		UCHAR ProductString[DS4_PRODUCT_NAME_LENGTH] =
		{
			0x28, 0x03, 0x57, 0x00, 0x69, 0x00, 0x72, 0x00,
			0x65, 0x00, 0x6C, 0x00, 0x65, 0x00, 0x73, 0x00,
			0x73, 0x00, 0x20, 0x00, 0x43, 0x00, 0x6F, 0x00,
			0x6E, 0x00, 0x74, 0x00, 0x72, 0x00, 0x6F, 0x00,
			0x6C, 0x00, 0x6C, 0x00, 0x65, 0x00, 0x72, 0x00
		};

		Urb->UrbControlDescriptorRequest.TransferBufferLength = DS4_PRODUCT_NAME_LENGTH;
		RtlCopyBytes(Urb->UrbControlDescriptorRequest.TransferBuffer, ProductString, DS4_PRODUCT_NAME_LENGTH);

		break;
	}
	default:
		break;
	}

	return STATUS_SUCCESS;
}

NTSTATUS ViGEm::Bus::Targets::EmulationTargetDS4::UsbBulkOrInterruptTransfer(_URB_BULK_OR_INTERRUPT_TRANSFER* pTransfer, WDFREQUEST Request)
{
	NTSTATUS     status;
	WDFREQUEST   notifyRequest;
	
	// Data coming FROM us TO higher driver
	if (pTransfer->TransferFlags & USBD_TRANSFER_DIRECTION_IN
		&& pTransfer->PipeHandle == reinterpret_cast<USBD_PIPE_HANDLE>(0xFFFF0084))
	{
		TraceDbg(
			TRACE_USBPDO,
			">> >> >> Incoming request, queuing...");

		/* This request is sent periodically and relies on data the "feeder"
		   has to supply, so we queue this request and return with STATUS_PENDING.
		   The request gets completed as soon as the "feeder" sent an update. */
		status = WdfRequestForwardToIoQueue(Request, this->_PendingUsbInRequests);

		return (NT_SUCCESS(status)) ? STATUS_PENDING : status;
	}

	// Store relevant bytes of buffer in PDO context
	RtlCopyBytes(&this->_OutputReport,
		static_cast<PUCHAR>(pTransfer->TransferBuffer) + DS4_OUTPUT_BUFFER_OFFSET,
		DS4_OUTPUT_BUFFER_LENGTH);

	// Notify user-mode process that new data is available
	status = WdfIoQueueRetrieveNextRequest(this->_PendingNotificationRequests, &notifyRequest);

	if (NT_SUCCESS(status))
	{
		PDS4_REQUEST_NOTIFICATION notify = NULL;

		status = WdfRequestRetrieveOutputBuffer(
			notifyRequest, 
			sizeof(DS4_REQUEST_NOTIFICATION), 
			reinterpret_cast<PVOID*>(&notify),
			nullptr
		);

		if (NT_SUCCESS(status))
		{
			// Assign values to output buffer
			notify->Size = sizeof(DS4_REQUEST_NOTIFICATION);
			notify->SerialNo = this->_SerialNo;
			notify->Report = this->_OutputReport;

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
	
	return status;
}

NTSTATUS ViGEm::Bus::Targets::EmulationTargetDS4::UsbControlTransfer(PURB Urb)
{
	NTSTATUS status;

	switch (Urb->UrbControlTransfer.SetupPacket[6])
	{
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

NTSTATUS ViGEm::Bus::Targets::EmulationTargetDS4::SubmitReport(PVOID NewReport)
{
	NTSTATUS    status;
	WDFREQUEST  usbRequest;

	status = WdfIoQueueRetrieveNextRequest(this->_PendingUsbInRequests, &usbRequest);

	if (!NT_SUCCESS(status))
		return status;

	// Get pending IRP
	PIRP pendingIrp = WdfRequestWdmGetIrp(usbRequest);

	// Get USB request block
	PURB urb = static_cast<PURB>(URB_FROM_IRP(pendingIrp));

	// Get transfer buffer
	auto Buffer = static_cast<PUCHAR>(urb->UrbBulkOrInterruptTransfer.TransferBuffer);

	urb->UrbBulkOrInterruptTransfer.TransferBufferLength = DS4_REPORT_SIZE;

	/* Copy report to cache and transfer buffer
	 * Skip first byte as it contains the never changing report id */
	RtlCopyBytes(this->_Report + 1, &(static_cast<PDS4_SUBMIT_REPORT>(NewReport))->Report, sizeof(DS4_REPORT));

	if (Buffer)
		RtlCopyBytes(Buffer, this->_Report, DS4_REPORT_SIZE);

	// Complete pending request
	WdfRequestComplete(usbRequest, status);

	return status;
}

VOID ViGEm::Bus::Targets::EmulationTargetDS4::PendingUsbRequestsTimerFunc(
	_In_ WDFTIMER Timer
)
{
	auto ctx = reinterpret_cast<EmulationTargetDS4*>(Core::EmulationTargetPdoGetContext(WdfTimerGetParentObject(Timer))->Target);

	WDFREQUEST              usbRequest;
	PIRP                    pendingIrp;
	PIO_STACK_LOCATION      irpStack;

	TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_DS4, "%!FUNC! Entry");

	// Get pending USB request
	NTSTATUS status = WdfIoQueueRetrieveNextRequest(ctx->_PendingUsbInRequests, &usbRequest);

	if (NT_SUCCESS(status))
	{
		// Get pending IRP
		pendingIrp = WdfRequestWdmGetIrp(usbRequest);
		irpStack = IoGetCurrentIrpStackLocation(pendingIrp);

		// Get USB request block
		PURB urb = (PURB)irpStack->Parameters.Others.Argument1;

		// Get transfer buffer
		PUCHAR Buffer = (PUCHAR)urb->UrbBulkOrInterruptTransfer.TransferBuffer;
		// Set buffer length to report size
		urb->UrbBulkOrInterruptTransfer.TransferBufferLength = DS4_REPORT_SIZE;

		// Copy cached report to transfer buffer 
		if (Buffer)
			RtlCopyBytes(Buffer, ctx->_Report, DS4_REPORT_SIZE);

		// Complete pending request
		WdfRequestComplete(usbRequest, status);
	}

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DS4, "%!FUNC! Exit with status %!STATUS!", status);
}
