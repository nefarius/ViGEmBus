#include "XusbPdo.hpp"
#include "trace.h"
#include "XusbPdo.tmh"
#define NTSTRSAFE_LIB
#include <ntstrsafe.h>
#include <wdmguid.h>



PCWSTR ViGEm::Bus::Targets::EmulationTargetXUSB::_deviceDescription = L"Virtual Xbox 360 Controller";

ViGEm::Bus::Targets::EmulationTargetXUSB::EmulationTargetXUSB() : EmulationTargetPDO(0x045E, 0x028E)
{
	TargetType = Xbox360Wired;
	UsbConfigurationDescriptionSize = XUSB_DESCRIPTOR_SIZE;
}

NTSTATUS ViGEm::Bus::Targets::EmulationTargetXUSB::PrepareDevice(PWDFDEVICE_INIT DeviceInit, PUNICODE_STRING DeviceId,
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
	RtlUnicodeStringPrintf(&buffer, L"USB\\VID_%04X&PID_%04X", this->VendorId, this->ProductId);

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

NTSTATUS ViGEm::Bus::Targets::EmulationTargetXUSB::PrepareHardware()
{
	WDF_QUERY_INTERFACE_CONFIG ifaceCfg;

	INTERFACE dummyIface;

	dummyIface.Size = sizeof(INTERFACE);
	dummyIface.Version = 1;
	dummyIface.Context = static_cast<PVOID>(this->PdoDevice);

	dummyIface.InterfaceReference = WdfDeviceInterfaceReferenceNoOp;
	dummyIface.InterfaceDereference = WdfDeviceInterfaceDereferenceNoOp;

	/* XUSB.sys will query for the following three "dummy" interfaces
	* BUT WONT USE IT so we just expose them to satisfy initialization. (TODO: Check if still valid!)
	*/

	// Dummy PNP_LOCATION

	WDF_QUERY_INTERFACE_CONFIG_INIT(
		&ifaceCfg,
		static_cast<PINTERFACE>(&dummyIface),
		&GUID_PNP_LOCATION_INTERFACE,
		nullptr
	);

	NTSTATUS status = WdfDeviceAddQueryInterface(this->PdoDevice, &ifaceCfg);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR,
		            TRACE_XUSB,
		            "Couldn't register PNP_LOCATION dummy interface %!GUID! (WdfDeviceAddQueryInterface failed with status %!STATUS!)",
		            &GUID_PNP_LOCATION_INTERFACE,
		            status);

		return status;
	}

	// Dummy D3COLD_SUPPORT

	WDF_QUERY_INTERFACE_CONFIG_INIT(
		&ifaceCfg,
		static_cast<PINTERFACE>(&dummyIface),
		&GUID_D3COLD_SUPPORT_INTERFACE,
		nullptr
	);

	status = WdfDeviceAddQueryInterface(this->PdoDevice, &ifaceCfg);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR,
		            TRACE_XUSB,
		            "Couldn't register D3COLD_SUPPORT dummy interface %!GUID! (WdfDeviceAddQueryInterface failed with status %!STATUS!)",
		            &GUID_D3COLD_SUPPORT_INTERFACE,
		            status);

		return status;
	}

	// Dummy REENUMERATE_SELF_INTERFACE_STANDARD

	WDF_QUERY_INTERFACE_CONFIG_INIT(
		&ifaceCfg,
		static_cast<PINTERFACE>(&dummyIface),
		&GUID_REENUMERATE_SELF_INTERFACE_STANDARD,
		nullptr
	);

	status = WdfDeviceAddQueryInterface(this->PdoDevice, &ifaceCfg);
	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR,
		            TRACE_XUSB,
		            "Couldn't register REENUM_SELF_STD dummy interface %!GUID! (WdfDeviceAddQueryInterface failed with status %!STATUS!)",
		            &GUID_REENUMERATE_SELF_INTERFACE_STANDARD,
		            status);

		return status;
	}

	// Expose USB_BUS_INTERFACE_USBDI_GUID

	// This interface actually IS used
	USB_BUS_INTERFACE_USBDI_V1 xusbInterface;

	xusbInterface.Size = sizeof(USB_BUS_INTERFACE_USBDI_V1);
	xusbInterface.Version = USB_BUSIF_USBDI_VERSION_1;
	xusbInterface.BusContext = static_cast<PVOID>(this->PdoDevice);

	xusbInterface.InterfaceReference = WdfDeviceInterfaceReferenceNoOp;
	xusbInterface.InterfaceDereference = WdfDeviceInterfaceDereferenceNoOp;

	xusbInterface.SubmitIsoOutUrb = UsbSubmitIsoOutUrb;
	xusbInterface.GetUSBDIVersion = UsbGetUSBDIVersion;
	xusbInterface.QueryBusTime = UsbQueryBusTime;
	xusbInterface.QueryBusInformation = UsbQueryBusInformation;
	xusbInterface.IsDeviceHighSpeed = UsbIsDeviceHighSpeed;

	WDF_QUERY_INTERFACE_CONFIG_INIT(
		&ifaceCfg,
		reinterpret_cast<PINTERFACE>(&xusbInterface),
		&USB_BUS_INTERFACE_USBDI_GUID,
		nullptr
	);

	status = WdfDeviceAddQueryInterface(this->PdoDevice, &ifaceCfg);
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

NTSTATUS ViGEm::Bus::Targets::EmulationTargetXUSB::InitContext()
{
	WDF_OBJECT_ATTRIBUTES attributes;
	PUCHAR blobBuffer;

	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	attributes.ParentObject = this->PdoDevice;

	TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_XUSB, "Initializing XUSB context...");

	RtlZeroMemory(this->Rumble, ARRAYSIZE(this->Rumble));
	
	// Is later overwritten by actual XInput slot
	this->LedNumber = -1;
	
	RtlZeroMemory(&this->Packet, sizeof(XUSB_INTERRUPT_IN_PACKET));
	// Packet size (20 bytes = 0x14)
	this->Packet.Size = 0x14;

	this->ReportedCapabilities = FALSE;

	this->InterruptInitStage = 0;
	
	// Allocate blob storage
	NTSTATUS status = WdfMemoryCreate(
		&attributes,
		NonPagedPoolNx,
		XUSB_POOL_TAG,
		XUSB_BLOB_STORAGE_SIZE,
		&this->InterruptBlobStorage,
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
		this->PdoDevice,
		&holdingInQueueConfig, 
		WDF_NO_OBJECT_ATTRIBUTES, 
		&this->HoldingUsbInRequests
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

VOID ViGEm::Bus::Targets::EmulationTargetXUSB::GetDeviceDescriptorType(PUSB_DEVICE_DESCRIPTOR pDescriptor)
{
	pDescriptor->bLength = 0x12;
	pDescriptor->bDescriptorType = USB_DEVICE_DESCRIPTOR_TYPE;
	pDescriptor->bcdUSB = 0x0200; // USB v2.0
	pDescriptor->bDeviceClass = 0xFF;
	pDescriptor->bDeviceSubClass = 0xFF;
	pDescriptor->bDeviceProtocol = 0xFF;
	pDescriptor->bMaxPacketSize0 = 0x08;
	pDescriptor->idVendor = this->VendorId;
	pDescriptor->idProduct = this->ProductId;
	pDescriptor->bcdDevice = 0x0114;
	pDescriptor->iManufacturer = 0x01;
	pDescriptor->iProduct = 0x02;
	pDescriptor->iSerialNumber = 0x03;
	pDescriptor->bNumConfigurations = 0x01;
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
	
	TraceEvents(TRACE_LEVEL_VERBOSE,
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

	TraceEvents(TRACE_LEVEL_VERBOSE,
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

	TraceEvents(TRACE_LEVEL_VERBOSE,
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

	TraceEvents(TRACE_LEVEL_VERBOSE,
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
