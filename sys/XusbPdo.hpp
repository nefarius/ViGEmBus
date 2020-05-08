#pragma once


#include "EmulationTargetPDO.hpp"

namespace ViGEm::Bus::Targets
{
	constexpr auto XUSB_POOL_TAG = 'EGiV';
	
	typedef struct _XUSB_REPORT
	{
		USHORT wButtons;
		BYTE bLeftTrigger;
		BYTE bRightTrigger;
		SHORT sThumbLX;
		SHORT sThumbLY;
		SHORT sThumbRX;
		SHORT sThumbRY;

	} XUSB_REPORT, * PXUSB_REPORT;
	
	typedef struct _XUSB_INTERRUPT_IN_PACKET
	{
		UCHAR Id;

		UCHAR Size;

		XUSB_REPORT Report;

	} XUSB_INTERRUPT_IN_PACKET, * PXUSB_INTERRUPT_IN_PACKET;

	//
	// XUSB-specific device context data.
	// 
	typedef struct _XUSB_DEVICE_DATA
	{
		//
		// Rumble buffer
		//
		UCHAR Rumble[0x08];

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

	} XUSB_DEVICE_DATA, * PXUSB_DEVICE_DATA;

	WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(XUSB_DEVICE_DATA, XusbPdoGetContext)
	
	class EmulationTargetXUSB : public Core::EmulationTargetPDO
	{
	public:
		EmulationTargetXUSB() = default;
		~EmulationTargetXUSB() = default;

		NTSTATUS PrepareDevice(PWDFDEVICE_INIT DeviceInit, USHORT VendorId, USHORT ProductId,
			PUNICODE_STRING DeviceId, PUNICODE_STRING DeviceDescription);

		NTSTATUS PrepareHardware(WDFDEVICE Device);

		NTSTATUS InitContext(WDFDEVICE Device);

		VOID GetConfigurationDescriptorType(PUCHAR Buffer, ULONG Length);

		VOID GetDeviceDescriptorType(PUSB_DEVICE_DESCRIPTOR pDescriptor, USHORT VendorId, USHORT ProductId);

		VOID SelectConfiguration(PUSBD_INTERFACE_INFORMATION pInfo);
		
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
	};
}
