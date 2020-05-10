#pragma once


#include "EmulationTargetPDO.hpp"
#include <ViGEm/km/BusShared.h>

namespace ViGEm::Bus::Targets
{
	constexpr auto XUSB_POOL_TAG = 'EGiV';

	typedef struct _XUSB_INTERRUPT_IN_PACKET
	{
		UCHAR Id;

		UCHAR Size;

		XUSB_REPORT Report;
	} XUSB_INTERRUPT_IN_PACKET, *PXUSB_INTERRUPT_IN_PACKET;


	class EmulationTargetXUSB : public Core::EmulationTargetPDO
	{
	public:
		EmulationTargetXUSB() = default;
		~EmulationTargetXUSB() = default;

		NTSTATUS PrepareDevice(PWDFDEVICE_INIT DeviceInit, USHORT VID, USHORT PID,
		                       PUNICODE_STRING DeviceId, PUNICODE_STRING DeviceDescription) override;

		NTSTATUS PrepareHardware(WDFDEVICE Device) override;

		NTSTATUS InitContext(WDFDEVICE Device) override;

		VOID GetConfigurationDescriptorType(PUCHAR Buffer, ULONG Length) override;

		VOID GetDeviceDescriptorType(PUSB_DEVICE_DESCRIPTOR pDescriptor) override;

		VOID SelectConfiguration(PUSBD_INTERFACE_INFORMATION pInfo) override;

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
	};

	//
	// XUSB-specific device context data.
	// 
	typedef struct _XUSB_PDO_CONTEXT
	{
		EmulationTargetXUSB* Context;
	} XUSB_PDO_CONTEXT, *PXUSB_PDO_CONTEXT;

	WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(XUSB_PDO_CONTEXT, XusbPdoGetContext)
}
