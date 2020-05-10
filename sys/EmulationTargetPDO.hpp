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
	// {A8BA2D1F-894F-464A-B0CE-7A0C8FD65DF1}
	DEFINE_GUID(GUID_DEVCLASS_VIGEM_RAWPDO,
	            0xA8BA2D1F, 0x894F, 0x464A, 0xB0, 0xCE, 0x7A, 0x0C, 0x8F, 0xD6, 0x5D, 0xF1);

	typedef struct _PDO_IDENTIFICATION_DESCRIPTION* PPDO_IDENTIFICATION_DESCRIPTION;

	class EmulationTargetPDO
	{
	public:
		EmulationTargetPDO(USHORT VID, USHORT PID);

		virtual ~EmulationTargetPDO() = default;

		virtual NTSTATUS PrepareDevice(PWDFDEVICE_INIT DeviceInit,
		                               PUNICODE_STRING DeviceId, PUNICODE_STRING DeviceDescription) = 0;

		virtual NTSTATUS PrepareHardware() = 0;

		virtual NTSTATUS InitContext() = 0;

		virtual VOID GetConfigurationDescriptorType(PUCHAR Buffer, ULONG Length) = 0;

		virtual VOID GetDeviceDescriptorType(PUSB_DEVICE_DESCRIPTOR pDescriptor) = 0;

		virtual VOID SelectConfiguration(PUSBD_INTERFACE_INFORMATION pInfo) = 0;

		NTSTATUS CreateDevice(_In_ WDFDEVICE Device,
		                      _In_ PWDFDEVICE_INIT DeviceInit,
		                      _In_ PPDO_IDENTIFICATION_DESCRIPTION Description);

		VOID SetSerial(ULONG Serial);

		ULONG GetSerial() const;

		bool operator==(EmulationTargetPDO& other) const
		{
			return (other.SerialNo == this->SerialNo);
		}
		
	protected:
		static const ULONG _maxHardwareIdLength = 0xFF;

		static PCWSTR _deviceLocation;

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

		static EVT_WDF_DEVICE_PREPARE_HARDWARE EvtDevicePrepareHardware;

		static EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL EvtIoInternalDeviceControl;

		static const int MAX_INSTANCE_ID_LEN = 80;

		//
		// Unique serial number of the device on the bus
		// 
		ULONG SerialNo{};

		// 
		// PID of the process creating this PDO
		// 
		DWORD OwnerProcessId{};

		//
		// Device type this PDO is emulating
		// 
		VIGEM_TARGET_TYPE TargetType;

		//
		// If set, the vendor ID the emulated device is reporting
		// 
		USHORT VendorId{};

		//
		// If set, the product ID the emulated device is reporting
		// 
		USHORT ProductId{};

		//
		// Queue for incoming data interrupt transfer
		//
		WDFQUEUE PendingUsbInRequests{};

		//
		// Queue for inverted calls
		//
		WDFQUEUE PendingNotificationRequests{};

		//
		// This child objects' device object
		// 
		WDFDEVICE PdoDevice;

		//
		// Signals the bus that PDO is ready to receive data
		// 
		KEVENT PdoBootNotificationEvent;
	};

	typedef struct _PDO_IDENTIFICATION_DESCRIPTION
	{
		WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER Header;
		
		EmulationTargetPDO* Target;
	} PDO_IDENTIFICATION_DESCRIPTION, *PPDO_IDENTIFICATION_DESCRIPTION;

	typedef struct _EMULATION_TARGET_PDO_CONTEXT
	{
		EmulationTargetPDO* Target;
	} EMULATION_TARGET_PDO_CONTEXT, *PEMULATION_TARGET_PDO_CONTEXT;

	WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(EMULATION_TARGET_PDO_CONTEXT, EmulationTargetPdoGetContext)
}
