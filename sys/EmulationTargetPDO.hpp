#pragma once

#include <ntddk.h>
#include <wdf.h>
#include <ntintsafe.h>

#include <usb.h>
#include <usbbusif.h>

#include <ViGEm/Common.h>

//
// Some insane macro-magic =3
// 
#define P99_PROTECT(...) __VA_ARGS__
#define COPY_BYTE_ARRAY(_dst_, _bytes_)   do {BYTE b[] = _bytes_; \
                                            RtlCopyMemory(_dst_, b, RTL_NUMBER_OF_V1(b)); } while (0)

namespace ViGEm::Bus::Core
{
	class EmulationTargetPDO
	{
	public:
		EmulationTargetPDO() = default;

		virtual ~EmulationTargetPDO() = default;

		virtual NTSTATUS PrepareDevice(PWDFDEVICE_INIT DeviceInit, USHORT VendorId, USHORT ProductId,
		                               PUNICODE_STRING DeviceId, PUNICODE_STRING DeviceDescription) = 0;

		virtual NTSTATUS PrepareHardware(WDFDEVICE Device) = 0;

		virtual NTSTATUS InitContext(WDFDEVICE Device) = 0;

		virtual VOID GetConfigurationDescriptorType(PUCHAR Buffer, ULONG Length) = 0;

		virtual VOID GetDeviceDescriptorType(PUSB_DEVICE_DESCRIPTOR pDescriptor, USHORT VendorId, USHORT ProductId) = 0;

		virtual VOID SelectConfiguration(PUSBD_INTERFACE_INFORMATION pInfo) = 0;
	protected:
		static const ULONG _maxHardwareIdLength = 0xFF;

		static BOOLEAN USB_BUSIFFN UsbIsDeviceHighSpeed(IN PVOID BusContext);

		static NTSTATUS USB_BUSIFFN UsbQueryBusInformation(
			IN PVOID BusContext,
			IN ULONG Level,
			IN OUT PVOID BusInformationBuffer,
			IN OUT PULONG BusInformationBufferLength,
			OUT PULONG BusInformationActualLength
		);

		static NTSTATUS USB_BUSIFFN UsbSubmitIsoOutUrb(IN PVOID BusContext, IN PURB Urb);

		static NTSTATUS USB_BUSIFFN UsbQueryBusTime(IN PVOID BusContext, IN OUT PULONG CurrentUsbFrame);

		static VOID USB_BUSIFFN UsbGetUSBDIVersion(
			IN PVOID BusContext,
			IN OUT PUSBD_VERSION_INFORMATION VersionInformation,
			IN OUT PULONG HcdCapabilities
		);

        //
	    // Unique serial number of the device on the bus
	    // 
        ULONG SerialNo;

        // 
        // PID of the process creating this PDO
        // 
        DWORD OwnerProcessId;

        //
        // Device type this PDO is emulating
        // 
        VIGEM_TARGET_TYPE TargetType;

        //
        // If set, the vendor ID the emulated device is reporting
        // 
        USHORT VendorId;

        //
        // If set, the product ID the emulated device is reporting
        // 
        USHORT ProductId;

        //
        // Queue for incoming data interrupt transfer
        //
        WDFQUEUE PendingUsbInRequests;

        //
        // Queue for inverted calls
        //
        WDFQUEUE PendingNotificationRequests;
	};
}
