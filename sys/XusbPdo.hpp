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

namespace ViGEm::Bus::Targets
{
	constexpr auto XUSB_POOL_TAG = 'XUiV';

	typedef struct _XUSB_INTERRUPT_IN_PACKET
	{
		UCHAR Id;

		UCHAR Size;

		XUSB_REPORT Report;
	} XUSB_INTERRUPT_IN_PACKET, *PXUSB_INTERRUPT_IN_PACKET;

	constexpr bool xusb_is_data_pipe(_URB_BULK_OR_INTERRUPT_TRANSFER* pTransfer)
	{
		return (pTransfer->PipeHandle == reinterpret_cast<USBD_PIPE_HANDLE>(0xFFFF0081));
	}

	constexpr bool xusb_is_control_pipe(_URB_BULK_OR_INTERRUPT_TRANSFER* pTransfer)
	{
		return (pTransfer->PipeHandle == reinterpret_cast<USBD_PIPE_HANDLE>(0xFFFF0083));
	}

	class EmulationTargetXUSB : public Core::EmulationTargetPDO
	{
	public:
		EmulationTargetXUSB(ULONG Serial, LONG SessionId, USHORT VendorId = 0x045E, USHORT ProductId = 0x028E);

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

		NTSTATUS GetUserIndex(PULONG UserIndex) const;
		
	private:
		static PCWSTR _deviceDescription;

#if defined(_X86_)
		static const int XUSB_CONFIGURATION_SIZE = 0x00E4;
#else
		static const int XUSB_CONFIGURATION_SIZE = 0x0130;
#endif
		static const int XUSB_DESCRIPTOR_SIZE = 0x0099;
		static const int XUSB_RUMBLE_SIZE = 0x08;
		static const int XUSB_LEDSET_SIZE = 0x03;
		static const int XUSB_LEDNUM_SIZE = 0x01;
		static const int XUSB_INIT_STAGE_SIZE = 0x03;
		static const int XUSB_BLOB_STORAGE_SIZE = 0x2A;

		static const int XUSB_BLOB_00_OFFSET = 0x00;
		static const int XUSB_BLOB_01_OFFSET = 0x03;
		static const int XUSB_BLOB_02_OFFSET = 0x06;
		static const int XUSB_BLOB_03_OFFSET = 0x09;
		static const int XUSB_BLOB_04_OFFSET = 0x0C;
		static const int XUSB_BLOB_05_OFFSET = 0x20;
		static const int XUSB_BLOB_06_OFFSET = 0x23;
		static const int XUSB_BLOB_07_OFFSET = 0x26;

		//
		// Rumble buffer
		//
		UCHAR _Rumble[XUSB_RUMBLE_SIZE];

		//
		// LED number (represents XInput slot index)
		//
		CHAR _LedNumber;

		//
		// Report packet
		//
		XUSB_INTERRUPT_IN_PACKET _Packet;

		//
		// Queue for incoming control interrupt transfer
		//
		WDFQUEUE _HoldingUsbInRequests;

		//
		// Required for XInputGetCapabilities to work
		// 
		BOOLEAN _ReportedCapabilities;

		//
		// Required for XInputGetCapabilities to work
		// 
		ULONG _InterruptInitStage;

		//
		// Storage of binary blobs (packets) for PDO initialization
		// 
		WDFMEMORY _InterruptBlobStorage;
	};
}
