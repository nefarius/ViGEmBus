#pragma once

#include <ntddk.h>
#include <wdf.h>
#include <ntintsafe.h>

#include <usb.h>
#include <usbbusif.h>

#include <ViGEm/Common.h>
#include <initguid.h>

//
// Some insane macro-magic =3
// 
#define P99_PROTECT(...) __VA_ARGS__
#define COPY_BYTE_ARRAY(_dst_, _bytes_)   do {BYTE b[] = _bytes_; \
                                            RtlCopyMemory(_dst_, b, RTL_NUMBER_OF_V1(b)); } while (0)

namespace ViGEm::Bus::Core
{
	typedef struct _PDO_IDENTIFICATION_DESCRIPTION* PPDO_IDENTIFICATION_DESCRIPTION;

	class EmulationTargetPDO
	{
	public:
		EmulationTargetPDO(ULONG Serial, LONG SessionId, USHORT VendorId, USHORT ProductId);

		virtual ~EmulationTargetPDO() = default;

		static bool GetPdoBySerial(IN WDFDEVICE ParentDevice, IN ULONG SerialNo, OUT EmulationTargetPDO** Object);

		virtual NTSTATUS PdoPrepareDevice(PWDFDEVICE_INIT DeviceInit,
		                                  PUNICODE_STRING DeviceId,
		                                  PUNICODE_STRING DeviceDescription) = 0;

		virtual NTSTATUS PdoPrepareHardware() = 0;

		virtual NTSTATUS PdoInitContext() = 0;

		NTSTATUS PdoCreateDevice(_In_ WDFDEVICE ParentDevice,
		                         _In_ PWDFDEVICE_INIT DeviceInit);

		VOID SetSerial(ULONG Serial);

		ULONG GetSerial() const;

		bool operator==(EmulationTargetPDO& other) const
		{
			return (other._SerialNo == this->_SerialNo);
		}

		virtual NTSTATUS UsbGetDeviceDescriptorType(PUSB_DEVICE_DESCRIPTOR pDescriptor) = 0;

		NTSTATUS UsbSelectConfiguration(PURB Urb);

		void UsbAbortPipe();

		NTSTATUS UsbGetConfigurationDescriptorType(PURB Urb);

		virtual NTSTATUS UsbClassInterface(PURB Urb) = 0;

		virtual NTSTATUS UsbGetDescriptorFromInterface(PURB Urb) = 0;

		virtual NTSTATUS UsbSelectInterface(PURB Urb) = 0;

		virtual NTSTATUS UsbGetStringDescriptorType(PURB Urb) = 0;

		virtual NTSTATUS UsbBulkOrInterruptTransfer(struct _URB_BULK_OR_INTERRUPT_TRANSFER* pTransfer,
		                                            WDFREQUEST Request) = 0;

		virtual NTSTATUS UsbControlTransfer(PURB Urb) = 0;

		virtual NTSTATUS SubmitReport(PVOID NewReport) = 0;

		NTSTATUS EnqueueNotification(WDFREQUEST Request) const;

		bool IsOwnerProcess() const;

	private:
		static unsigned long current_process_id();

		static EVT_WDF_DEVICE_CONTEXT_CLEANUP EvtDeviceContextCleanup;

	protected:
		static const ULONG _maxHardwareIdLength = 0xFF;

		static PCWSTR _deviceLocation;

		static BOOLEAN USB_BUSIFFN UsbInterfaceIsDeviceHighSpeed(IN PVOID BusContext);

		static NTSTATUS USB_BUSIFFN UsbInterfaceQueryBusInformation(
			IN PVOID BusContext,
			IN ULONG Level,
			IN OUT PVOID BusInformationBuffer,
			IN OUT PULONG BusInformationBufferLength,
			OUT PULONG BusInformationActualLength
		);

		static NTSTATUS USB_BUSIFFN UsbInterfaceSubmitIsoOutUrb(IN PVOID BusContext, IN PURB Urb);

		static NTSTATUS USB_BUSIFFN UsbInterfaceQueryBusTime(IN PVOID BusContext, IN OUT PULONG CurrentUsbFrame);

		static VOID USB_BUSIFFN UsbInterfaceGetUSBDIVersion(
			IN PVOID BusContext,
			IN OUT PUSBD_VERSION_INFORMATION VersionInformation,
			IN OUT PULONG HcdCapabilities
		);

		static EVT_WDF_DEVICE_PREPARE_HARDWARE EvtDevicePrepareHardware;

		static EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL EvtIoInternalDeviceControl;

		static const int MAX_INSTANCE_ID_LEN = 80;

		virtual VOID GetConfigurationDescriptorType(PUCHAR Buffer, ULONG Length) = 0;

		virtual NTSTATUS SelectConfiguration(PURB Urb) = 0;

		virtual void AbortPipe() = 0;

		//
		// Unique serial number of the device on the bus
		// 
		ULONG _SerialNo{};

		// 
		// PID of the process creating this PDO
		// 
		DWORD _OwnerProcessId{};

		//
		// File object session ID
		// 
		LONG _SessionId{};

		//
		// Device type this PDO is emulating
		// 
		VIGEM_TARGET_TYPE _TargetType;

		//
		// If set, the vendor ID the emulated device is reporting
		// 
		USHORT _VendorId{};

		//
		// If set, the product ID the emulated device is reporting
		// 
		USHORT _ProductId{};

		//
		// Queue for incoming data interrupt transfer
		//
		WDFQUEUE _PendingUsbInRequests{};

		//
		// Queue for inverted calls
		//
		WDFQUEUE _PendingNotificationRequests{};

		//
		// This child objects' device object
		// 
		WDFDEVICE _PdoDevice{};

		//
		// Signals the bus that PDO is ready to receive data
		// 
		KEVENT _PdoBootNotificationEvent;

		//
		// Configuration descriptor size
		// 
		ULONG _UsbConfigurationDescriptionSize{};
	};

	typedef struct _PDO_IDENTIFICATION_DESCRIPTION
	{
		//
		// List entity header
		// 
		WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER Header;

		//
		// Primary key to identify PDO
		// 
		ULONG SerialNo;

		//
		// Session ID
		// 
		LONG SessionId;

		//
		// Context object of PDO
		// 
		EmulationTargetPDO* Target;
	} PDO_IDENTIFICATION_DESCRIPTION, *PPDO_IDENTIFICATION_DESCRIPTION;

	typedef struct _EMULATION_TARGET_PDO_CONTEXT
	{
		EmulationTargetPDO* Target;
	} EMULATION_TARGET_PDO_CONTEXT, *PEMULATION_TARGET_PDO_CONTEXT;

	WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(EMULATION_TARGET_PDO_CONTEXT, EmulationTargetPdoGetContext)
}
