#pragma once

#include "EmulationTargetPDO.hpp"
#include <ViGEm/km/BusShared.h>

#include "Util.h"


namespace ViGEm::Bus::Targets
{
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
		EmulationTargetDS4();
		~EmulationTargetDS4() = default;
		
		NTSTATUS PrepareDevice(PWDFDEVICE_INIT DeviceInit,
		                       PUNICODE_STRING DeviceId,
		                       PUNICODE_STRING DeviceDescription) override;

		NTSTATUS PrepareHardware() override;

		NTSTATUS InitContext() override;

		VOID GetConfigurationDescriptorType(PUCHAR Buffer, ULONG Length) override;

		VOID GetDeviceDescriptorType(PUSB_DEVICE_DESCRIPTOR pDescriptor) override;

		NTSTATUS SelectConfiguration(PURB Urb) override;

		void AbortPipe() override;
		NTSTATUS UsbClassInterface(PURB Urb) override;
		NTSTATUS UsbGetDescriptorFromInterface(PURB Urb) override;
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
}
