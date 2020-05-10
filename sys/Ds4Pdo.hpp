#pragma once

#include "EmulationTargetPDO.hpp"
#include <ViGEm/km/BusShared.h>

#include "Util.h"


namespace ViGEm::Bus::Targets
{
	class EmulationTargetDS4 : public Core::EmulationTargetPDO
	{
	public:
		EmulationTargetDS4();
		~EmulationTargetDS4() = default;
		
		NTSTATUS PrepareDevice(PWDFDEVICE_INIT DeviceInit,
		                       PUNICODE_STRING DeviceId,
		                       PUNICODE_STRING DeviceDescription) override;

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


		static const int HID_GET_FEATURE_REPORT_SIZE_0 = 0x31;
		static const int HID_GET_FEATURE_REPORT_SIZE_1 = 0x25;
		static const int HID_GET_FEATURE_REPORT_MAC_ADDRESSES_SIZE = 0x10;

		static const int HID_SET_FEATURE_REPORT_SIZE_0 = 0x17;
		static const int HID_SET_FEATURE_REPORT_SIZE_1 = 0x11;

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
		static const int DS4_HID_REPORT_DESCRIPTOR_SIZE = 0x01D3;

		static const int DS4_MANUFACTURER_NAME_LENGTH = 0x38;
		static const int DS4_PRODUCT_NAME_LENGTH = 0x28;
		static const int DS4_OUTPUT_BUFFER_OFFSET = 0x04;
		static const int DS4_OUTPUT_BUFFER_LENGTH = 0x05;

		static const int DS4_REPORT_SIZE = 0x40;
		static const int DS4_QUEUE_FLUSH_PERIOD = 0x05;

		//
		// HID Input Report buffer
		//
		UCHAR Report[DS4_REPORT_SIZE];

		//
		// Output report cache
		//
		DS4_OUTPUT_REPORT OutputReport;

		//
		// Timer for dispatching interrupt transfer
		//
		WDFTIMER PendingUsbInRequestsTimer;

		//
		// Auto-generated MAC address of the target device
		//
		MAC_ADDRESS TargetMacAddress;

		//
		// Default MAC address of the host (not used)
		//
		MAC_ADDRESS HostMacAddress;

		static EVT_WDF_TIMER PendingUsbRequestsTimerFunc;
	};

	//
	// DS4-specific device context data.
	// 
	typedef struct _DS4_PDO_CONTEXT
	{
		EmulationTargetDS4* Context;
	} DS4_PDO_CONTEXT, *PDS4_PDO_CONTEXT;

	WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DS4_PDO_CONTEXT, Ds4PdoGetContext)
}
