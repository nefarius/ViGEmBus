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

#include "EmulationTargetPDO.hpp"
#include <ViGEm/km/BusShared.h>


namespace ViGEm::Bus::Targets
{
	//
	// Represents a MAC address.
	//
	typedef struct _MAC_ADDRESS
	{
		UCHAR Vendor0;
		UCHAR Vendor1;
		UCHAR Vendor2;
		UCHAR Nic0;
		UCHAR Nic1;
		UCHAR Nic2;
	} MAC_ADDRESS, * PMAC_ADDRESS;
	
	constexpr unsigned char hid_get_report_id(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST* pReq)
	{
		return pReq->Value & 0xFF;
	}

	constexpr unsigned char hid_get_report_type(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST* pReq)
	{
		return (pReq->Value >> 8) & 0xFF;
	}

	class EmulationTargetDS4 : public Core::EmulationTargetPDO
	{
	public:
		EmulationTargetDS4(ULONG Serial, LONG SessionId, USHORT VendorId = 0x054C, USHORT ProductId = 0x05C4);
		~EmulationTargetDS4() = default;

		NTSTATUS PdoPrepareDevice(PWDFDEVICE_INIT DeviceInit,
		                          PUNICODE_STRING DeviceId,
		                          PUNICODE_STRING DeviceDescription) override;

		NTSTATUS PdoPrepareHardware() override;

		NTSTATUS PdoInitContext() override;

		VOID GetConfigurationDescriptorType(PUCHAR Buffer, ULONG Length) override;

		NTSTATUS UsbGetDeviceDescriptorType(PUSB_DEVICE_DESCRIPTOR pDescriptor) override;

		NTSTATUS SelectConfiguration(PURB Urb) override;

		void AbortPipe() override;
		NTSTATUS UsbClassInterface(PURB Urb) override;
		NTSTATUS UsbGetDescriptorFromInterface(PURB Urb) override;
		NTSTATUS UsbSelectInterface(PURB Urb) override;
		NTSTATUS UsbGetStringDescriptorType(PURB Urb) override;
		NTSTATUS UsbBulkOrInterruptTransfer(_URB_BULK_OR_INTERRUPT_TRANSFER* pTransfer, WDFREQUEST Request) override;
		NTSTATUS UsbControlTransfer(PURB Urb) override;
		NTSTATUS SubmitReportImpl(PVOID NewReport) override;
	private:
		static PCWSTR _deviceDescription;

		static const int HID_REQUEST_GET_REPORT = 0x01;
		static const int HID_REQUEST_SET_REPORT = 0x09;
		static const int HID_REPORT_TYPE_FEATURE = 0x03;

		static const int HID_REPORT_ID_0 = 0xA3;
		static const int HID_REPORT_ID_1 = 0x02;
		static const int HID_REPORT_MAC_ADDRESSES_ID = 0x12;
		static const int HID_REPORT_ID_3 = 0x13;
		static const int HID_REPORT_ID_4 = 0x14;

		static const int DS4_DESCRIPTOR_SIZE = 0x0029;
#if defined(_X86_)
		static const int DS4_CONFIGURATION_SIZE = 0x0050;
#else
		static const int DS4_CONFIGURATION_SIZE = 0x0070;
#endif

		static const int DS4_MANUFACTURER_NAME_LENGTH = 0x38;
		static const int DS4_PRODUCT_NAME_LENGTH = 0x28;
		static const int DS4_OUTPUT_BUFFER_OFFSET = 0x04;
		static const int DS4_OUTPUT_BUFFER_LENGTH = 0x05;

		static const int DS4_REPORT_SIZE = 0x40;
		static const int DS4_QUEUE_FLUSH_PERIOD = 0x05;

		//
		// HID Input Report buffer
		//
		UCHAR _Report[DS4_REPORT_SIZE];

		//
		// Output report cache
		//
		DS4_OUTPUT_REPORT _OutputReport;

		//
		// Timer for dispatching interrupt transfer
		//
		WDFTIMER _PendingUsbInRequestsTimer;

		//
		// Auto-generated MAC address of the target device
		//
		MAC_ADDRESS _TargetMacAddress;

		//
		// Default MAC address of the host (not used)
		//
		MAC_ADDRESS _HostMacAddress;

		static EVT_WDF_TIMER PendingUsbRequestsTimerFunc;

		static VOID ReverseByteArray(PUCHAR Array, INT Length);

		static VOID GenerateRandomMacAddress(PMAC_ADDRESS Address);
	};
}
