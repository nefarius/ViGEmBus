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


#include "EmulationTargetPDO.hpp"
#include "CRTCPP.hpp"
#include "trace.h"
#include "EmulationTargetPDO.tmh"
#define NTSTRSAFE_LIB
#include <ntstrsafe.h>
#include <usbioctl.h>
#include <usbiodef.h>


PCWSTR ViGEm::Bus::Core::EmulationTargetPDO::_deviceLocation = L"Virtual Gamepad Emulation Bus";

NTSTATUS ViGEm::Bus::Core::EmulationTargetPDO::PdoCreateDevice(WDFDEVICE ParentDevice, PWDFDEVICE_INIT DeviceInit)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
	WDF_OBJECT_ATTRIBUTES pdoAttributes;
	WDF_IO_QUEUE_CONFIG defaultPdoQueueConfig;
	WDFQUEUE defaultPdoQueue;
	UNICODE_STRING deviceDescription;
	WDF_OBJECT_ATTRIBUTES attributes;
	WDF_IO_QUEUE_CONFIG usbInQueueConfig;
	WDF_IO_QUEUE_CONFIG notificationsQueueConfig;
	PEMULATION_TARGET_PDO_CONTEXT pPdoContext;

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_BUSPDO, "%!FUNC! Entry");

	DECLARE_CONST_UNICODE_STRING(deviceLocation, L"Virtual Gamepad Emulation Bus");
	DECLARE_UNICODE_STRING_SIZE(buffer, MAX_INSTANCE_ID_LEN);
	// reserve space for device id
	DECLARE_UNICODE_STRING_SIZE(deviceId, MAX_INSTANCE_ID_LEN);

	UNREFERENCED_PARAMETER(ParentDevice);

	PAGED_CODE();

	// set device type
	WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_BUS_EXTENDER);

	do
	{
#pragma region Prepare PDO

		status = this->PdoPrepareDevice(DeviceInit, &deviceId, &deviceDescription);

		if (!NT_SUCCESS(status))
			break;

		// set device id
		status = WdfPdoInitAssignDeviceID(DeviceInit, &deviceId);
		if (!NT_SUCCESS(status))
		{
			TraceError(
				TRACE_BUSPDO,
				"WdfPdoInitAssignDeviceID failed with status %!STATUS!",
				status);
			break;
		}

		// prepare instance id
		status = RtlUnicodeStringPrintf(&buffer, L"%02d", this->_SerialNo);
		if (!NT_SUCCESS(status))
		{
			TraceError(
				TRACE_BUSPDO,
				"RtlUnicodeStringPrintf failed with status %!STATUS!",
				status);
			break;
		}

		// set instance id
		status = WdfPdoInitAssignInstanceID(DeviceInit, &buffer);
		if (!NT_SUCCESS(status))
		{
			TraceError(
				TRACE_BUSPDO,
				"WdfPdoInitAssignInstanceID failed with status %!STATUS!",
				status);
			break;
		}

		// set device description (for English operating systems)
		status = WdfPdoInitAddDeviceText(DeviceInit, &deviceDescription, &deviceLocation, 0x409);
		if (!NT_SUCCESS(status))
		{
			TraceError(
				TRACE_BUSPDO,
				"WdfPdoInitAddDeviceText failed with status %!STATUS!",
				status);
			break;
		}

		// default locale is English
		WdfPdoInitSetDefaultLocale(DeviceInit, 0x409);

#pragma region PNP/Power event callbacks

		WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);

		pnpPowerCallbacks.EvtDevicePrepareHardware = EvtDevicePrepareHardware;

		WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

#pragma endregion

#pragma endregion

#pragma region Create PDO

		// Add common device data context
		WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&pdoAttributes, EMULATION_TARGET_PDO_CONTEXT);

		pdoAttributes.EvtCleanupCallback = EvtDeviceContextCleanup;

		status = WdfDeviceCreate(&DeviceInit, &pdoAttributes, &this->_PdoDevice);
		if (!NT_SUCCESS(status))
		{
			TraceError(
				TRACE_BUSPDO,
				"WdfDeviceCreate failed with status %!STATUS!",
				status);
			break;
		}

		TraceVerbose(
			TRACE_BUSPDO,
			"Created PDO 0x%p",
			this->_PdoDevice);

#pragma endregion

#pragma region Expose USB Interface

		status = WdfDeviceCreateDeviceInterface(
			ParentDevice,
			const_cast<LPGUID>(&GUID_DEVINTERFACE_USB_DEVICE),
			nullptr
		);
		if (!NT_SUCCESS(status))
		{
			TraceError(
				TRACE_BUSPDO,
				"WdfDeviceCreateDeviceInterface failed with status %!STATUS!",
				status);
			break;
		}

#pragma endregion

#pragma region Set PDO contexts

		//
		// Bind this object and device context together
		// 
		pPdoContext = EmulationTargetPdoGetContext(this->_PdoDevice);
		pPdoContext->Target = this;

		status = this->PdoInitContext();

		if (!NT_SUCCESS(status))
		{
			TraceError(
				TRACE_BUSPDO,
				"Couldn't initialize additional contexts: %!STATUS!",
				status);
			break;
		}

#pragma endregion

#pragma region Create Queues & Locks

		WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
		attributes.ParentObject = this->_PdoDevice;

		// Create and assign queue for incoming interrupt transfer
		WDF_IO_QUEUE_CONFIG_INIT(&usbInQueueConfig, WdfIoQueueDispatchManual);

		status = WdfIoQueueCreate(
			this->_PdoDevice,
			&usbInQueueConfig,
			WDF_NO_OBJECT_ATTRIBUTES,
			&this->_PendingUsbInRequests
		);
		if (!NT_SUCCESS(status))
		{
			TraceError(
				TRACE_BUSPDO,
				"WdfIoQueueCreate (PendingUsbInRequests) failed with status %!STATUS!",
				status);
			break;
		}

		WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
		attributes.ParentObject = this->_PdoDevice;

		// Create and assign queue for user-land notification requests
		WDF_IO_QUEUE_CONFIG_INIT(&notificationsQueueConfig, WdfIoQueueDispatchManual);

		status = WdfIoQueueCreate(
			ParentDevice,
			&notificationsQueueConfig,
			WDF_NO_OBJECT_ATTRIBUTES,
			&this->_PendingNotificationRequests
		);
		if (!NT_SUCCESS(status))
		{
			TraceError(
				TRACE_BUSPDO,
				"WdfIoQueueCreate (PendingNotificationRequests) failed with status %!STATUS!",
				status);
			break;
		}

		status = WdfIoQueueReadyNotify(
			this->_PendingNotificationRequests,
			EvtWdfIoPendingNotificationQueueState,
			this
		);
		if (!NT_SUCCESS(status))
		{
			TraceError(
				TRACE_BUSPDO,
				"WdfIoQueueReadyNotify (PendingNotificationRequests) failed with status %!STATUS!",
				status);
			break;
		}

#pragma endregion

#pragma region Default I/O queue setup

		WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
		attributes.ParentObject = this->_PdoDevice;

		WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&defaultPdoQueueConfig, WdfIoQueueDispatchParallel);

		defaultPdoQueueConfig.EvtIoInternalDeviceControl = EvtIoInternalDeviceControl;

		status = WdfIoQueueCreate(
			this->_PdoDevice,
			&defaultPdoQueueConfig,
			WDF_NO_OBJECT_ATTRIBUTES,
			&defaultPdoQueue
		);
		if (!NT_SUCCESS(status))
		{
			TraceError(
				TRACE_BUSPDO,
				"WdfIoQueueCreate (Default) failed with status %!STATUS!",
				status);
			break;
		}

#pragma endregion

#pragma region PNP capabilities

		//
		// Other capabilities initialized in derived class
		// 

		this->_PnpCapabilities.Address = this->_SerialNo;
		this->_PnpCapabilities.UINumber = this->_SerialNo;

		WdfDeviceSetPnpCapabilities(this->_PdoDevice, &this->_PnpCapabilities);

#pragma endregion

#pragma region Power capabilities

		//
		// Capabilities initialized in derived class
		// 

		WdfDeviceSetPowerCapabilities(this->_PdoDevice, &this->_PowerCapabilities);

#pragma endregion
	} while (FALSE);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_BUSPDO, "%!FUNC! Exit with status %!STATUS!", status);

	return status;
}

VOID ViGEm::Bus::Core::EmulationTargetPDO::EvtDeviceContextCleanup(
	IN WDFOBJECT Device
)
{
	TraceVerbose(TRACE_BUSPDO, "%!FUNC! Entry");

	const auto ctx = EmulationTargetPdoGetContext(Device);

	//
	// This queues parent is the FDO so explicitly free memory
	//
	WdfIoQueuePurgeSynchronously(ctx->Target->_WaitDeviceReadyRequests);
	WdfObjectDelete(ctx->Target->_WaitDeviceReadyRequests);

	//
	// Wait for thread to finish, if active
	// 
	if (ctx->Target->_WaitDeviceReadyCompletionWorkerThreadHandle)
	{
		NTSTATUS status = KeWaitForSingleObject(
			&ctx->Target->_WaitDeviceReadyCompletionWorkerThreadHandle,
			Executive,
			KernelMode,
			FALSE,
			nullptr
		);

		if (NT_SUCCESS(status))
		{
			ZwClose(ctx->Target->_WaitDeviceReadyCompletionWorkerThreadHandle);
			ctx->Target->_WaitDeviceReadyCompletionWorkerThreadHandle = nullptr;
		}
		else
		{
			TraceEvents(TRACE_LEVEL_WARNING,
				TRACE_BUSPDO,
				"KeWaitForSingleObject failed with status %!STATUS!",
				status
			);
		}
	}

	//
	// PDO device object getting disposed, free context object 
	// 
	delete ctx->Target;

	TraceVerbose(TRACE_BUSPDO, "%!FUNC! Exit");
}

NTSTATUS ViGEm::Bus::Core::EmulationTargetPDO::SubmitReport(PVOID NewReport)
{
	return (this->IsOwnerProcess())
		? this->SubmitReportImpl(NewReport)
		: STATUS_ACCESS_DENIED;
}

NTSTATUS ViGEm::Bus::Core::EmulationTargetPDO::EnqueueNotification(WDFREQUEST Request) const
{
	return (this->IsOwnerProcess())
		? WdfRequestForwardToIoQueue(Request, this->_PendingNotificationRequests)
		: STATUS_ACCESS_DENIED;
}

bool ViGEm::Bus::Core::EmulationTargetPDO::IsOwnerProcess() const
{
	return this->_OwnerProcessId == current_process_id();
}

VIGEM_TARGET_TYPE ViGEm::Bus::Core::EmulationTargetPDO::GetType() const
{
	return this->_TargetType;
}

NTSTATUS ViGEm::Bus::Core::EmulationTargetPDO::EnqueueWaitDeviceReady(WDFREQUEST Request)
{
	NTSTATUS status;

	if (!this->IsOwnerProcess())
		return STATUS_ACCESS_DENIED;

	if (!this->_WaitDeviceReadyRequests)
		return STATUS_INVALID_DEVICE_STATE;

	if (this->_WaitDeviceReadyCompletionWorkerThreadHandle)
	{
		status = KeWaitForSingleObject(
			&this->_WaitDeviceReadyCompletionWorkerThreadHandle,
			Executive,
			KernelMode,
			FALSE,
			nullptr
		);

		if (NT_SUCCESS(status))
		{
			ZwClose(this->_WaitDeviceReadyCompletionWorkerThreadHandle);
			this->_WaitDeviceReadyCompletionWorkerThreadHandle = nullptr;
		}
		else
		{
			TraceEvents(TRACE_LEVEL_WARNING,
				TRACE_BUSPDO,
				"KeWaitForSingleObject failed with status %!STATUS!",
				status
			);
		}
	}

	status = WdfRequestForwardToIoQueue(Request, this->_WaitDeviceReadyRequests);

	if (!NT_SUCCESS(status))
	{
		TraceError(
			TRACE_BUSPDO,
			"WdfRequestForwardToIoQueue failed with status %!STATUS!",
			status
		);

		return status;
	}

	OBJECT_ATTRIBUTES threadOb;

	InitializeObjectAttributes(&threadOb, NULL,
		OBJ_KERNEL_HANDLE, NULL, NULL);

	status = PsCreateSystemThread(&this->_WaitDeviceReadyCompletionWorkerThreadHandle,
		static_cast<ACCESS_MASK>(0L),
		&threadOb,
		nullptr,
		nullptr,
		WaitDeviceReadyCompletionWorkerRoutine,
		this
	);

	if (!NT_SUCCESS(status))
	{
		TraceError(
			TRACE_BUSPDO,
			"PsCreateSystemThread failed with status %!STATUS!",
			status
		);
	}

	return status;
}

NTSTATUS ViGEm::Bus::Core::EmulationTargetPDO::PdoPrepare(WDFDEVICE ParentDevice)
{
	NTSTATUS status;
	WDF_OBJECT_ATTRIBUTES attributes;
	WDF_IO_QUEUE_CONFIG plugInQueueConfig;
	DMF_MODULE_ATTRIBUTES moduleAttributes;
	DMF_CONFIG_BufferQueue dmfBufferCfg;

	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	attributes.ParentObject = ParentDevice;

	// Create and assign queue for incoming interrupt transfer
	WDF_IO_QUEUE_CONFIG_INIT(&plugInQueueConfig, WdfIoQueueDispatchManual);

	status = WdfIoQueueCreate(
		ParentDevice,
		&plugInQueueConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		&this->_WaitDeviceReadyRequests
	);
	if (!NT_SUCCESS(status))
	{
		TraceError(
			TRACE_BUSPDO,
			"WdfIoQueueCreate (PendingPlugInRequests) failed with status %!STATUS!",
			status);
	}

	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	attributes.ParentObject = ParentDevice;

	DMF_CONFIG_BufferQueue_AND_ATTRIBUTES_INIT(
		&dmfBufferCfg,
		&moduleAttributes
	);

	// Don't auto-grow; start dropping packets on overrun
	dmfBufferCfg.SourceSettings.EnableLookAside = FALSE;
	// Maximum number of buffers to be filled and kept queued
	dmfBufferCfg.SourceSettings.BufferCount = MAX_OUT_BUFFER_QUEUE_COUNT;
	// Maximum byte count per buffer
	dmfBufferCfg.SourceSettings.BufferSize = MAX_OUT_BUFFER_QUEUE_SIZE;
	// Field to store real buffer content length
	dmfBufferCfg.SourceSettings.BufferContextSize = sizeof(size_t);
	// "Expensive" memory ;)
	dmfBufferCfg.SourceSettings.PoolType = NonPagedPoolNx;

	status = DMF_BufferQueue_Create(
		ParentDevice,
		&moduleAttributes,
		&attributes,
		&this->_UsbInterruptOutBufferQueue
	);

	if (!NT_SUCCESS(status))
	{
		TraceError(
			TRACE_BUSPDO,
			"DMF_BufferQueue_Create failed with status %!STATUS!",
			status
		);
	}

	return status;
}

unsigned long ViGEm::Bus::Core::EmulationTargetPDO::current_process_id()
{
	return static_cast<DWORD>(reinterpret_cast<DWORD_PTR>(PsGetCurrentProcessId()) & 0xFFFFFFFF);
}

#pragma region USB Interface Functions

BOOLEAN USB_BUSIFFN ViGEm::Bus::Core::EmulationTargetPDO::UsbInterfaceIsDeviceHighSpeed(IN PVOID BusContext)
{
	UNREFERENCED_PARAMETER(BusContext);

	return TRUE;
}

NTSTATUS USB_BUSIFFN ViGEm::Bus::Core::EmulationTargetPDO::UsbInterfaceQueryBusInformation(
	IN PVOID BusContext, IN ULONG Level,
	IN OUT PVOID BusInformationBuffer,
	IN OUT PULONG BusInformationBufferLength,
	OUT PULONG BusInformationActualLength)
{
	UNREFERENCED_PARAMETER(BusContext);
	UNREFERENCED_PARAMETER(Level);
	UNREFERENCED_PARAMETER(BusInformationBuffer);
	UNREFERENCED_PARAMETER(BusInformationBufferLength);
	UNREFERENCED_PARAMETER(BusInformationActualLength);

	return STATUS_UNSUCCESSFUL;
}

NTSTATUS USB_BUSIFFN ViGEm::Bus::Core::EmulationTargetPDO::UsbInterfaceSubmitIsoOutUrb(IN PVOID BusContext, IN PURB Urb)
{
	UNREFERENCED_PARAMETER(BusContext);
	UNREFERENCED_PARAMETER(Urb);

	return STATUS_UNSUCCESSFUL;
}

NTSTATUS USB_BUSIFFN ViGEm::Bus::Core::EmulationTargetPDO::UsbInterfaceQueryBusTime(
	IN PVOID BusContext, IN OUT PULONG CurrentUsbFrame)
{
	UNREFERENCED_PARAMETER(BusContext);
	UNREFERENCED_PARAMETER(CurrentUsbFrame);

	return STATUS_UNSUCCESSFUL;
}

VOID USB_BUSIFFN ViGEm::Bus::Core::EmulationTargetPDO::UsbInterfaceGetUSBDIVersion(IN PVOID BusContext,
	IN OUT PUSBD_VERSION_INFORMATION
	VersionInformation,
	IN OUT PULONG HcdCapabilities)
{
	UNREFERENCED_PARAMETER(BusContext);

	if (VersionInformation != nullptr)
	{
		VersionInformation->USBDI_Version = 0x500; /* Usbport */
		VersionInformation->Supported_USB_Version = 0x200; /* USB 2.0 */
	}

	if (HcdCapabilities != nullptr)
	{
		*HcdCapabilities = 0;
	}
}


#pragma endregion

VOID ViGEm::Bus::Core::EmulationTargetPDO::WaitDeviceReadyCompletionWorkerRoutine(IN PVOID StartContext)
{
	const auto ctx = static_cast<EmulationTargetPDO*>(StartContext);

	WDFREQUEST waitRequest;
	LARGE_INTEGER timeout;
	timeout.QuadPart = WDF_REL_TIMEOUT_IN_SEC(1);

	TraceEvents(TRACE_LEVEL_INFORMATION,
		TRACE_BUSPDO,
		"Waiting for 1 second to complete PDO boot..."
	);

	NTSTATUS status = KeWaitForSingleObject(
		&ctx->_PdoBootNotificationEvent,
		Executive,
		KernelMode,
		FALSE,
		&timeout
	);

	do
	{
		//
		// Fetch pending request (there has to be one at this point)
		// 
		if (!NT_SUCCESS(WdfIoQueueRetrieveNextRequest(ctx->_WaitDeviceReadyRequests, &waitRequest)))
		{
			TraceEvents(TRACE_LEVEL_WARNING,
				TRACE_BUSPDO,
				"No pending device wait request available"
			);
			break;
		}

		if (status == STATUS_TIMEOUT)
		{
			TraceEvents(TRACE_LEVEL_WARNING,
				TRACE_BUSPDO,
				"Device wait request timed out, completing with error"
			);

			//
			// We haven't hit a path where the event gets signaled, report error
			// 
			WdfRequestComplete(waitRequest, STATUS_DEVICE_HARDWARE_ERROR);
			break;
		}

		if (NT_SUCCESS(status))
		{
			TraceEvents(TRACE_LEVEL_INFORMATION,
				TRACE_BUSPDO,
				"Device wait request completed successfully"
			);

			//
			// Event triggered in time, complete with success
			// 
			WdfRequestComplete(waitRequest, STATUS_SUCCESS);
			break;
		}
	} while (FALSE);

	ZwClose(ctx->_WaitDeviceReadyCompletionWorkerThreadHandle);
	ctx->_WaitDeviceReadyCompletionWorkerThreadHandle = nullptr;
	KeClearEvent(&ctx->_PdoBootNotificationEvent);
	(void)PsTerminateSystemThread(0);
}

VOID ViGEm::Bus::Core::EmulationTargetPDO::DumpAsHex(PCSTR Prefix, PVOID Buffer, ULONG BufferLength)
{
#ifdef DBG

	size_t dumpBufferLength = ((BufferLength * sizeof(CHAR)) * 2) + 1;
	PSTR dumpBuffer = static_cast<PSTR>(ExAllocatePoolZero(
		NonPagedPoolNx,
		dumpBufferLength,
		'1234'
	));
	if (dumpBuffer)
	{

		RtlZeroMemory(dumpBuffer, dumpBufferLength);

		for (ULONG i = 0; i < BufferLength; i++)
		{
			sprintf(&dumpBuffer[i * 2], "%02X", static_cast<PUCHAR>(Buffer)[i]);
		}

		TraceVerbose(TRACE_BUSPDO,
			"%s - Buffer length: %04d, buffer content: %s\n",
			Prefix,
			BufferLength,
			dumpBuffer
		);

		ExFreePoolWithTag(dumpBuffer, '1234');
	}
#else
	UNREFERENCED_PARAMETER(Prefix);
	UNREFERENCED_PARAMETER(Buffer);
	UNREFERENCED_PARAMETER(BufferLength);
#endif
}

void ViGEm::Bus::Core::EmulationTargetPDO::UsbAbortPipe()
{
	this->AbortPipe();

	// Higher driver shutting down, emptying PDOs queues
	WdfIoQueuePurge(this->_PendingUsbInRequests, nullptr, nullptr);
	WdfIoQueuePurge(this->_PendingNotificationRequests, nullptr, nullptr);
}

NTSTATUS ViGEm::Bus::Core::EmulationTargetPDO::UsbGetConfigurationDescriptorType(PURB Urb)
{
	const PUCHAR Buffer = static_cast<PUCHAR>(Urb->UrbControlDescriptorRequest.TransferBuffer);

	// First request just gets required buffer size back
	if (Urb->UrbControlDescriptorRequest.TransferBufferLength == sizeof(USB_CONFIGURATION_DESCRIPTOR))
	{
		const ULONG length = sizeof(USB_CONFIGURATION_DESCRIPTOR);

		this->GetConfigurationDescriptorType(Buffer, length);
		return STATUS_SUCCESS;
	}

	const ULONG length = Urb->UrbControlDescriptorRequest.TransferBufferLength;

	// Second request can store the whole descriptor
	if (length >= _UsbConfigurationDescriptionSize)
	{
		this->GetConfigurationDescriptorType(Buffer, _UsbConfigurationDescriptionSize);
		return STATUS_SUCCESS;
	}

	return STATUS_UNSUCCESSFUL;
}

NTSTATUS ViGEm::Bus::Core::EmulationTargetPDO::UsbSelectConfiguration(PURB Urb)
{
	TraceVerbose(
		TRACE_USBPDO,
		">> >> >> URB_FUNCTION_SELECT_CONFIGURATION: TotalLength %d",
		Urb->UrbHeader.Length);

	if (Urb->UrbHeader.Length == sizeof(struct _URB_SELECT_CONFIGURATION))
	{
		TraceVerbose(
			TRACE_USBPDO,
			">> >> >> URB_FUNCTION_SELECT_CONFIGURATION: NULL ConfigurationDescriptor");
		return STATUS_SUCCESS;
	}

	return this->SelectConfiguration(Urb);
}

ViGEm::Bus::Core::EmulationTargetPDO::
EmulationTargetPDO(ULONG Serial, LONG SessionId, USHORT VendorId, USHORT ProductId) : _SerialNo(Serial),
_SessionId(SessionId),
_VendorId(VendorId),
_ProductId(ProductId)
{
	this->_OwnerProcessId = current_process_id();
	KeInitializeEvent(&this->_PdoBootNotificationEvent, NotificationEvent, FALSE);

	WDF_DEVICE_PNP_CAPABILITIES_INIT(&this->_PnpCapabilities);
	WDF_DEVICE_POWER_CAPABILITIES_INIT(&this->_PowerCapabilities);
}

bool ViGEm::Bus::Core::EmulationTargetPDO::GetPdoBySerial(
	IN WDFDEVICE ParentDevice, IN ULONG SerialNo, OUT EmulationTargetPDO** Object)
{
	WDF_CHILD_RETRIEVE_INFO info;

	const WDFCHILDLIST list = WdfFdoGetDefaultChildList(ParentDevice);

	PDO_IDENTIFICATION_DESCRIPTION description;

	WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(&description.Header, sizeof(description));

	//
	// Identify by serial number
	// 
	description.SerialNo = SerialNo;

	WDF_CHILD_RETRIEVE_INFO_INIT(&info, &description.Header);

	const WDFDEVICE pdoDevice = WdfChildListRetrievePdo(list, &info);

	if (pdoDevice == nullptr)
		return false;

	*Object = EmulationTargetPdoGetContext(pdoDevice)->Target;

	return true;
}

bool ViGEm::Bus::Core::EmulationTargetPDO::GetPdoByTypeAndSerial(IN WDFDEVICE ParentDevice, IN VIGEM_TARGET_TYPE Type,
	IN ULONG SerialNo, OUT EmulationTargetPDO** Object)
{
	return (GetPdoBySerial(ParentDevice, SerialNo, Object) && (*Object)->GetType() == Type);
}

BOOLEAN ViGEm::Bus::Core::EmulationTargetPDO::EvtChildListIdentificationDescriptionCompare(
	WDFCHILDLIST DeviceList,
	PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER FirstIdentificationDescription,
	PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER SecondIdentificationDescription)
{
	ViGEm::Bus::Core::PPDO_IDENTIFICATION_DESCRIPTION lhs, rhs;

	UNREFERENCED_PARAMETER(DeviceList);

	lhs = CONTAINING_RECORD(FirstIdentificationDescription,
		ViGEm::Bus::Core::PDO_IDENTIFICATION_DESCRIPTION,
		Header);
	rhs = CONTAINING_RECORD(SecondIdentificationDescription,
		ViGEm::Bus::Core::PDO_IDENTIFICATION_DESCRIPTION,
		Header);

	return (lhs->SerialNo == rhs->SerialNo) ? TRUE : FALSE;
}


NTSTATUS ViGEm::Bus::Core::EmulationTargetPDO::EnqueueWaitDeviceReady(WDFDEVICE ParentDevice, ULONG SerialNo,
	WDFREQUEST Request)
{
	NTSTATUS status;
	PDO_IDENTIFICATION_DESCRIPTION description;
	WDF_CHILD_LIST_ITERATOR iterator;
	WDF_CHILD_RETRIEVE_INFO childInfo;
	WDFDEVICE childDevice;

	FuncEntry(TRACE_BUSPDO);

	const WDFCHILDLIST list = WdfFdoGetDefaultChildList(ParentDevice);

	WDF_CHILD_LIST_ITERATOR_INIT(
		&iterator,
		WdfRetrieveAddedChildren // might not be online yet
	);
	WdfChildListBeginIteration(
		list,
		&iterator
	);

	WDF_CHILD_RETRIEVE_INFO_INIT(
		&childInfo,
		&description.Header
	);

	//
	// Compare to find explicit PDO asked for (by serial)
	// 
	childInfo.EvtChildListIdentificationDescriptionCompare = EvtChildListIdentificationDescriptionCompare;

	WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(
		&description.Header,
		sizeof(description)
	);

	// ReSharper disable once CppAssignedValueIsNeverUsed
	description.SerialNo = SerialNo;

	status = WdfChildListRetrieveNextDevice(
		list,
		&iterator,
		&childDevice, // is NULL due to WdfRetrievePendingChildren
		&childInfo
	);
	if (NT_SUCCESS(status))
	{
		//
		// Object pointer filled after successful retrieval
		// 
		status = description.Target->EnqueueWaitDeviceReady(Request);
	}

	WdfChildListEndIteration(
		list,
		&iterator
	);

	TraceVerbose(TRACE_BUSPDO, "%!FUNC! Exit with status %!STATUS!", status);

	return status;
}

NTSTATUS ViGEm::Bus::Core::EmulationTargetPDO::EvtDevicePrepareHardware(
	_In_ WDFDEVICE Device,
	_In_ WDFCMRESLIST ResourcesRaw,
	_In_ WDFCMRESLIST ResourcesTranslated
)
{
	FuncEntry(TRACE_BUSPDO);

	UNREFERENCED_PARAMETER(ResourcesRaw);
	UNREFERENCED_PARAMETER(ResourcesTranslated);

	const auto ctx = EmulationTargetPdoGetContext(Device);

	NTSTATUS status = ctx->Target->PdoPrepareHardware();

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_BUSPDO, "%!FUNC! Exit with status %!STATUS!", status);

	return status;
}

VOID ViGEm::Bus::Core::EmulationTargetPDO::EvtIoInternalDeviceControl(
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request,
	_In_ size_t OutputBufferLength,
	_In_ size_t InputBufferLength,
	_In_ ULONG IoControlCode
)
{
	const auto ctx = EmulationTargetPdoGetContext(WdfIoQueueGetDevice(Queue));

	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);

	NTSTATUS status = STATUS_INVALID_PARAMETER;
	PIRP irp;
	PURB urb;
	PIO_STACK_LOCATION irpStack;

	FuncEntry(TRACE_BUSPDO);

	// No help from the framework available from here on
	irp = WdfRequestWdmGetIrp(Request);
	irpStack = IoGetCurrentIrpStackLocation(irp);

	switch (IoControlCode)
	{
	case IOCTL_INTERNAL_USB_SUBMIT_URB:

		TraceVerbose(
			TRACE_BUSPDO,
			">> IOCTL_INTERNAL_USB_SUBMIT_URB");

		urb = static_cast<PURB>(URB_FROM_IRP(irp));

		switch (urb->UrbHeader.Function)
		{
		case URB_FUNCTION_CONTROL_TRANSFER:

			TraceVerbose(
				TRACE_BUSPDO,
				">> >> URB_FUNCTION_CONTROL_TRANSFER");

			status = ctx->Target->UsbControlTransfer(urb);

			break;

		case URB_FUNCTION_CONTROL_TRANSFER_EX:

			TraceVerbose(
				TRACE_BUSPDO,
				">> >> URB_FUNCTION_CONTROL_TRANSFER_EX");

			status = STATUS_UNSUCCESSFUL;

			break;

		case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:

			TraceVerbose(
				TRACE_BUSPDO,
				">> >> URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER");

			status = ctx->Target->UsbBulkOrInterruptTransfer(&urb->UrbBulkOrInterruptTransfer, Request);

			break;

		case URB_FUNCTION_SELECT_CONFIGURATION:

			TraceVerbose(
				TRACE_BUSPDO,
				">> >> URB_FUNCTION_SELECT_CONFIGURATION");

			status = ctx->Target->UsbSelectConfiguration(urb);

			break;

		case URB_FUNCTION_SELECT_INTERFACE:

			TraceVerbose(
				TRACE_BUSPDO,
				">> >> URB_FUNCTION_SELECT_INTERFACE");

			status = ctx->Target->UsbSelectInterface(urb);

			break;

		case URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE:

			TraceVerbose(
				TRACE_BUSPDO,
				">> >> URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE");

			switch (urb->UrbControlDescriptorRequest.DescriptorType)
			{
			case USB_DEVICE_DESCRIPTOR_TYPE:

				TraceVerbose(
					TRACE_BUSPDO,
					">> >> >> USB_DEVICE_DESCRIPTOR_TYPE");

				status = ctx->Target->UsbGetDeviceDescriptorType(
					static_cast<PUSB_DEVICE_DESCRIPTOR>(urb->UrbControlDescriptorRequest.TransferBuffer));

				break;

			case USB_CONFIGURATION_DESCRIPTOR_TYPE:

				TraceVerbose(
					TRACE_BUSPDO,
					">> >> >> USB_CONFIGURATION_DESCRIPTOR_TYPE");

				status = ctx->Target->UsbGetConfigurationDescriptorType(urb);

				break;

			case USB_STRING_DESCRIPTOR_TYPE:

				TraceVerbose(
					TRACE_BUSPDO,
					">> >> >> USB_STRING_DESCRIPTOR_TYPE");

				status = ctx->Target->UsbGetStringDescriptorType(urb);

				break;

			default:

				TraceVerbose(
					TRACE_BUSPDO,
					">> >> >> Unknown descriptor type");

				break;
			}

			TraceVerbose(
				TRACE_BUSPDO,
				"<< <<");

			break;

		case URB_FUNCTION_GET_STATUS_FROM_DEVICE:

			TraceVerbose(
				TRACE_BUSPDO,
				">> >> URB_FUNCTION_GET_STATUS_FROM_DEVICE");

			// Defaults always succeed
			status = STATUS_SUCCESS;

			break;

		case URB_FUNCTION_ABORT_PIPE:

			TraceVerbose(
				TRACE_BUSPDO,
				">> >> URB_FUNCTION_ABORT_PIPE");

			ctx->Target->UsbAbortPipe();

			break;

		case URB_FUNCTION_CLASS_INTERFACE:

			TraceVerbose(
				TRACE_BUSPDO,
				">> >> URB_FUNCTION_CLASS_INTERFACE");

			status = ctx->Target->UsbClassInterface(urb);

			break;

		case URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE:

			TraceVerbose(
				TRACE_BUSPDO,
				">> >> URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE");

			status = ctx->Target->UsbGetDescriptorFromInterface(urb);

			break;

		default:

			TraceVerbose(
				TRACE_BUSPDO,
				">> >>  Unknown function: 0x%X",
				urb->UrbHeader.Function);

			break;
		}

		TraceVerbose(
			TRACE_BUSPDO,
			"<<");

		break;

	case IOCTL_INTERNAL_USB_GET_PORT_STATUS:

		TraceVerbose(
			TRACE_BUSPDO,
			">> IOCTL_INTERNAL_USB_GET_PORT_STATUS");

		// We report the (virtual) port as always active
		*static_cast<unsigned long*>(irpStack->Parameters.Others.Argument1) = USBD_PORT_ENABLED | USBD_PORT_CONNECTED;

		status = STATUS_SUCCESS;

		break;

	case IOCTL_INTERNAL_USB_RESET_PORT:

		TraceVerbose(
			TRACE_BUSPDO,
			">> IOCTL_INTERNAL_USB_RESET_PORT");

		// Sure, why not ;)
		status = STATUS_SUCCESS;

		break;

	case IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION:

		TraceVerbose(
			TRACE_BUSPDO,
			">> IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION");

		// TODO: implement?
		// This happens if the I/O latency is too high so HIDUSB aborts communication.
		status = STATUS_SUCCESS;

		break;

	default:

		TraceVerbose(
			TRACE_BUSPDO,
			">> Unknown I/O control code 0x%X",
			IoControlCode);

		break;
	}

	if (status != STATUS_PENDING)
	{
		WdfRequestComplete(Request, status);
	}

	TraceVerbose(TRACE_BUSPDO, "%!FUNC! Exit with status %!STATUS!", status);
}

void ViGEm::Bus::Core::EmulationTargetPDO::EvtWdfIoPendingNotificationQueueState(
	WDFQUEUE Queue,
	WDFCONTEXT Context
)
{
	const auto pThis = static_cast<EmulationTargetPDO*>(Context);

	//
	// No buffer available to answer the request with, leave queued
	// 
	if (DMF_BufferQueue_Count(pThis->_UsbInterruptOutBufferQueue) == 0)
	{
		return;
	}

	pThis->ProcessPendingNotification(Queue);
}
