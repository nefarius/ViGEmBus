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
		static EVT_WDF_TIMER PendingUsbRequestsTimerFunc;

		static VOID ReverseByteArray(PUCHAR Array, INT Length);

		static VOID GenerateRandomMacAddress(PMAC_ADDRESS Address);
		
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
		UCHAR _Report[DS4_REPORT_SIZE]{};

		//
		// Output report cache
		//
		DS4_OUTPUT_REPORT _OutputReport{};

		//
		// Timer for dispatching interrupt transfer
		//
		WDFTIMER _PendingUsbInRequestsTimer = WDF_NO_HANDLE;

		//
		// Auto-generated MAC address of the target device
		//
		MAC_ADDRESS _TargetMacAddress{};

		//
		// Default MAC address of the host (not used)
		//
		MAC_ADDRESS _HostMacAddress{};
	};
}
