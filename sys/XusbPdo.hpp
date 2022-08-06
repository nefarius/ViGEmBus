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

	protected:
		void ProcessPendingNotification(WDFQUEUE Queue) override;
		void DmfDeviceModulesAdd(_In_ PDMFMODULE_INIT DmfModuleInit) override;
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
