/*
* Virtual Gamepad Emulation Framework - Windows kernel-mode bus driver
*
* BSD 3-Clause License
*
* Copyright (c) 2018-2022, Nefarius Software Solutions e.U. and Contributors
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


#include "Driver.h"
#include "trace.h"
#include "Driver.tmh"
#include <wdmguid.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, Bus_EvtDeviceAdd)
#pragma alloc_text (PAGE, Bus_DeviceFileCreate)
#pragma alloc_text (PAGE, Bus_FileClose)
#pragma alloc_text (PAGE, Bus_EvtDriverContextCleanup)
#endif

#include "Queue.hpp"
#include "EmulationTargetPDO.hpp"
#include "XusbPdo.hpp"
#include "Ds4Pdo.hpp"

using ViGEm::Bus::Core::PDO_IDENTIFICATION_DESCRIPTION;
using ViGEm::Bus::Core::EmulationTargetPDO;
using ViGEm::Bus::Targets::EmulationTargetXUSB;
using ViGEm::Bus::Targets::EmulationTargetDS4;


EXTERN_C_START

IoctlHandler_IoctlRecord ViGEmBus_IoctlSpecification[] =
{
	{IOCTL_VIGEM_CHECK_VERSION, sizeof(VIGEM_CHECK_VERSION), 0, Bus_CheckVersionHandler},
	{IOCTL_VIGEM_WAIT_DEVICE_READY, sizeof(VIGEM_WAIT_DEVICE_READY), 0, Bus_WaitDeviceReadyHandler},
	{IOCTL_VIGEM_PLUGIN_TARGET, sizeof(VIGEM_PLUGIN_TARGET), 0, Bus_PluginTargetHandler},
	{IOCTL_VIGEM_UNPLUG_TARGET, sizeof(VIGEM_UNPLUG_TARGET), 0, Bus_UnplugTargetHandler},
	{IOCTL_XUSB_SUBMIT_REPORT, sizeof(XUSB_SUBMIT_REPORT), 0, Bus_XusbSubmitReportHandler},
	{IOCTL_XUSB_REQUEST_NOTIFICATION, sizeof(XUSB_REQUEST_NOTIFICATION), sizeof(XUSB_REQUEST_NOTIFICATION), Bus_XusbRequestNotificationHandler},
	{IOCTL_DS4_SUBMIT_REPORT, sizeof(DS4_SUBMIT_REPORT), 0, Bus_Ds4SubmitReportHandler},
	{IOCTL_DS4_REQUEST_NOTIFICATION, sizeof(DS4_REQUEST_NOTIFICATION), sizeof(DS4_REQUEST_NOTIFICATION), Bus_Ds4RequestNotificationHandler},
	{IOCTL_XUSB_GET_USER_INDEX, sizeof(XUSB_GET_USER_INDEX), sizeof(XUSB_GET_USER_INDEX), Bus_XusbGetUserIndexHandler},
	{IOCTL_DS4_AWAIT_OUTPUT_AVAILABLE, sizeof(DS4_AWAIT_OUTPUT), sizeof(DS4_AWAIT_OUTPUT), Bus_Ds4AwaitOutputHandler},
};

//
// Driver entry routine.
// 
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	WDF_DRIVER_CONFIG       config;
	NTSTATUS                status;
	WDFDRIVER               driver;
	WDF_OBJECT_ATTRIBUTES   attributes;

	KdPrint((DRIVERNAME "Virtual Gamepad Emulation Bus Driver [built: %s %s]\n", __DATE__, __TIME__));

	//
	// Initialize WPP Tracing
	//
	WPP_INIT_TRACING(DriverObject, RegistryPath);

	TraceEvents(TRACE_LEVEL_INFORMATION,
		TRACE_DRIVER,
		"Loading Virtual Gamepad Emulation Bus Driver"
	);

	ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

	//
	// Register cleanup callback
	// 
	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	attributes.EvtCleanupCallback = Bus_EvtDriverContextCleanup;

	WDF_DRIVER_CONFIG_INIT(&config, Bus_EvtDeviceAdd);

	status = WdfDriverCreate(DriverObject, RegistryPath, &attributes, &config, &driver);

	if (!NT_SUCCESS(status))
	{
		WPP_CLEANUP(DriverObject);
		KdPrint((DRIVERNAME "WdfDriverCreate failed with status 0x%x\n", status));
	}

	return status;
}

//
// Bus-device creation routine.
// 
NTSTATUS Bus_EvtDeviceAdd(IN WDFDRIVER Driver, IN PWDFDEVICE_INIT DeviceInit)
{
	WDF_CHILD_LIST_CONFIG config;
	NTSTATUS status;
	WDFDEVICE device = NULL;
	PNP_BUS_INFORMATION busInfo;
	WDF_FILEOBJECT_CONFIG foConfig;
	WDF_OBJECT_ATTRIBUTES fdoAttributes;
	WDF_OBJECT_ATTRIBUTES fileHandleAttributes;
	PFDO_DEVICE_DATA pFDOData;
	PWSTR pSymbolicNameList;
	PDMFDEVICE_INIT dmfDeviceInit = NULL;

	UNREFERENCED_PARAMETER(Driver);

	PAGED_CODE();

	FuncEntry(TRACE_DRIVER);

#pragma region Check for duplicated FDO

	//
	// Note: this could be avoided if converted to non-PNP driver
	// and use of named device object. Food for thought for future.
	// 

	if (NT_SUCCESS(status = IoGetDeviceInterfaces(
		&GUID_DEVINTERFACE_BUSENUM_VIGEM,
		NULL,
		0, // Important!
		&pSymbolicNameList
	)))
	{
		const bool deviceAlreadyExists = (0 != *pSymbolicNameList);
		ExFreePool(pSymbolicNameList);

		if (deviceAlreadyExists)
		{
			TraceError(
				TRACE_DRIVER,
				"Device with interface GUID {%!GUID!} already exists (%ws)",
				&GUID_DEVINTERFACE_BUSENUM_VIGEM,
				pSymbolicNameList
			);

			return STATUS_RESOURCE_IN_USE;
		}
	}
	else
	{
		TraceEvents(TRACE_LEVEL_WARNING,
			TRACE_DRIVER,
			"IoGetDeviceInterfaces failed with status %!STATUS!",
			status);
	}

#pragma endregion

	do
	{
		dmfDeviceInit = DMF_DmfDeviceInitAllocate(DeviceInit);

		if (dmfDeviceInit == NULL)
		{
			TraceError(
				TRACE_DRIVER,
				"DMF_DmfDeviceInitAllocate failed"
			);

			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		DMF_DmfDeviceInitHookPnpPowerEventCallbacks(dmfDeviceInit, NULL);
		DMF_DmfDeviceInitHookPowerPolicyEventCallbacks(dmfDeviceInit, NULL);

		WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_BUS_EXTENDER);

#pragma region Prepare child list

		WDF_CHILD_LIST_CONFIG_INIT(&config, sizeof(PDO_IDENTIFICATION_DESCRIPTION), Bus_EvtDeviceListCreatePdo);

		config.EvtChildListIdentificationDescriptionCompare = EmulationTargetPDO::EvtChildListIdentificationDescriptionCompare;

		WdfFdoInitSetDefaultChildListConfig(DeviceInit, &config, WDF_NO_OBJECT_ATTRIBUTES);

#pragma endregion

#pragma region Assign File Object Configuration

		WDF_FILEOBJECT_CONFIG_INIT(&foConfig, Bus_DeviceFileCreate, Bus_FileClose, NULL);

		WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&fileHandleAttributes, FDO_FILE_DATA);

		DMF_DmfDeviceInitHookFileObjectConfig(dmfDeviceInit, &foConfig);

		WdfDeviceInitSetFileObjectConfig(DeviceInit, &foConfig, &fileHandleAttributes);

#pragma endregion

#pragma region Create FDO

		WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&fdoAttributes, FDO_DEVICE_DATA);

		if (!NT_SUCCESS(status = WdfDeviceCreate(
			&DeviceInit,
			&fdoAttributes,
			&device
		)))
		{
			TraceError(
				TRACE_DRIVER,
				"WdfDeviceCreate failed with status %!STATUS!",
				status);
			break;
		}

		pFDOData = FdoGetData(device);

		pFDOData->InterfaceReferenceCounter = 0;
		pFDOData->NextSessionId = FDO_FIRST_SESSION_ID;

#pragma endregion

#pragma region Expose FDO interface

		if (!NT_SUCCESS(status = WdfDeviceCreateDeviceInterface(
			device,
			&GUID_DEVINTERFACE_BUSENUM_VIGEM,
			NULL
		)))
		{
			TraceError(
				TRACE_DRIVER,
				"WdfDeviceCreateDeviceInterface failed with status %!STATUS!",
				status);
			break;
		}

#pragma endregion

#pragma region Set bus information

		busInfo.BusTypeGuid = GUID_BUS_TYPE_USB;
		busInfo.LegacyBusType = PNPBus;
		busInfo.BusNumber = 0;

		WdfDeviceSetBusInformationForChildren(device, &busInfo);

#pragma endregion

		//
		// DMF Module initialization
		//
		DMF_EVENT_CALLBACKS dmfEventCallbacks;
		DMF_EVENT_CALLBACKS_INIT(&dmfEventCallbacks);
		dmfEventCallbacks.EvtDmfDeviceModulesAdd = DmfDeviceModulesAdd;
		DMF_DmfDeviceInitSetEventCallbacks(
			dmfDeviceInit,
			&dmfEventCallbacks
		);

		status = DMF_ModulesCreate(device, &dmfDeviceInit);

		if (!NT_SUCCESS(status))
		{
			TraceEvents(
				TRACE_LEVEL_ERROR,
				TRACE_DRIVER,
				"DMF_ModulesCreate failed with status %!STATUS!",
				status
			);
			break;
		}

	} while (FALSE);

	if (dmfDeviceInit != NULL)
	{
		DMF_DmfDeviceInitFree(&dmfDeviceInit);
	}

	if (!NT_SUCCESS(status) && device != NULL)
	{
		WdfObjectDelete(device);
	}

	FuncExit(TRACE_DRIVER, "status=%!STATUS!", status);

	return status;
}

#pragma code_seg("PAGED")
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
DmfDeviceModulesAdd(
	_In_ WDFDEVICE Device,
	_In_ PDMFMODULE_INIT DmfModuleInit
)
{
	FuncEntry(TRACE_DRIVER);

	PFDO_DEVICE_DATA pDevCtx = FdoGetData(Device);

	DMF_MODULE_ATTRIBUTES moduleAttributes;
	DMF_CONFIG_IoctlHandler ioctlHandlerConfig;
	DMF_CONFIG_IoctlHandler_AND_ATTRIBUTES_INIT(&ioctlHandlerConfig, &moduleAttributes);

	ioctlHandlerConfig.DeviceInterfaceGuid = GUID_DEVINTERFACE_BUSENUM_VIGEM;
	ioctlHandlerConfig.IoctlRecordCount = ARRAYSIZE(ViGEmBus_IoctlSpecification);
	ioctlHandlerConfig.IoctlRecords = ViGEmBus_IoctlSpecification;
	ioctlHandlerConfig.ForwardUnhandledRequests = FALSE;

	DMF_DmfModuleAdd(
		DmfModuleInit,
		&moduleAttributes,
		WDF_NO_OBJECT_ATTRIBUTES,
		NULL
	);

	DMF_CONFIG_NotifyUserWithRequestMultiple notifyConfig;
	DMF_CONFIG_NotifyUserWithRequestMultiple_AND_ATTRIBUTES_INIT(&notifyConfig, &moduleAttributes);

	notifyConfig.MaximumNumberOfPendingRequests = 64 * 2;
	notifyConfig.SizeOfDataBuffer = sizeof(DS4_AWAIT_OUTPUT);
	notifyConfig.MaximumNumberOfPendingDataBuffers = 64;
	notifyConfig.ModeType.Modes.ReplayLastMessageToNewClients = FALSE;
	notifyConfig.CompletionCallback = Bus_EvtUserNotifyRequestComplete;

	DMF_DmfModuleAdd(
		DmfModuleInit,
		&moduleAttributes,
		WDF_NO_OBJECT_ATTRIBUTES,
		&pDevCtx->UserNotification
	);

	FuncExitNoReturn(TRACE_DRIVER);
}
#pragma code_seg()

// Gets called when the user-land process (or kernel driver) exits or closes the handle,
// and all IO has completed.
// 
_Use_decl_annotations_
VOID
Bus_DeviceFileCreate(
	_In_ WDFDEVICE     Device,
	_In_ WDFREQUEST    Request,
	_In_ WDFFILEOBJECT FileObject
)
{
	NTSTATUS         status = STATUS_INVALID_PARAMETER;
	PFDO_FILE_DATA   pFileData = NULL;
	PFDO_DEVICE_DATA pFDOData = NULL;
	LONG             refCount = 0;
	LONG             sessionId = 0;

	UNREFERENCED_PARAMETER(Request);

	PAGED_CODE();

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

	pFileData = FileObjectGetData(FileObject);

	if (pFileData == NULL)
	{
		TraceError(
			TRACE_DRIVER,
			"FileObjectGetData failed to return file object from WDFFILEOBJECT 0x%p",
			FileObject);
	}
	else
	{
		pFDOData = FdoGetData(Device);
		if (pFDOData == NULL)
		{
			TraceError(
				TRACE_DRIVER,
				"FdoGetData failed");
			status = STATUS_NO_SUCH_DEVICE;
		}
		else
		{
			refCount = InterlockedIncrement(&pFDOData->InterfaceReferenceCounter);
			sessionId = InterlockedIncrement(&pFDOData->NextSessionId);

			pFileData->SessionId = sessionId;
			status = STATUS_SUCCESS;

			TraceEvents(TRACE_LEVEL_INFORMATION,
				TRACE_DRIVER,
				"File/session id = %d, device ref. count = %d",
				(int)sessionId, (int)refCount);
		}
	}

	WdfRequestComplete(Request, status);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit with status %!STATUS!", status);
}

//
// Gets called when the user-land process (or kernel driver) exits or closes the handle.
// 
_Use_decl_annotations_
VOID
Bus_FileClose(
	WDFFILEOBJECT FileObject
)
{
	WDFDEVICE                      device;
	WDFDEVICE                      hChild;
	NTSTATUS                       status;
	WDFCHILDLIST                   list;
	WDF_CHILD_LIST_ITERATOR        iterator;
	WDF_CHILD_RETRIEVE_INFO        childInfo;
	PDO_IDENTIFICATION_DESCRIPTION description;
	PFDO_FILE_DATA                 pFileData = NULL;
	PFDO_DEVICE_DATA               pFDOData = NULL;
	LONG                           refCount = 0;

	PAGED_CODE();


	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

	// Check common context
	pFileData = FileObjectGetData(FileObject);
	if (pFileData == NULL)
	{
		TraceError(
			TRACE_DRIVER,
			"FileObjectGetData failed to return file object from WDFFILEOBJECT 0x%p",
			FileObject);
		return;
	}

	device = WdfFileObjectGetDevice(FileObject);

	pFDOData = FdoGetData(device);
	if (pFDOData == NULL)
	{
		TraceError(
			TRACE_DRIVER,
			"FdoGetData failed");
		status = STATUS_NO_SUCH_DEVICE;
	}
	else
	{
		refCount = InterlockedDecrement(&pFDOData->InterfaceReferenceCounter);

		TraceEvents(TRACE_LEVEL_INFORMATION,
			TRACE_DRIVER,
			"Device ref. count = %d",
			(int)refCount);
	}

	list = WdfFdoGetDefaultChildList(device);

	WDF_CHILD_LIST_ITERATOR_INIT(&iterator, WdfRetrievePresentChildren);

	WdfChildListBeginIteration(list, &iterator);

	for (;;)
	{
		WDF_CHILD_RETRIEVE_INFO_INIT(&childInfo, &description.Header);
		WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(&description.Header, sizeof(description));

		status = WdfChildListRetrieveNextDevice(list, &iterator, &hChild, &childInfo);
		if (!NT_SUCCESS(status) || status == STATUS_NO_MORE_ENTRIES)
		{
			break;
		}

		//TraceVerbose(
		//    TRACE_DRIVER,
		//    "PDO properties: status = %!STATUS!, pdoPID = %d, curPID = %d, pdoSID = %d, curSID = %d, internal = %d",
		//    (int)childInfo.Status,
		//    (int)description.OwnerProcessId,
		//    (int)CURRENT_PROCESS_ID(),
		//    (int)description.SessionId,
		//    (int)pFileData->SessionId,
		//    (int)description.OwnerIsDriver
		//);

		// Only unplug devices with matching session id
		if (childInfo.Status == WdfChildListRetrieveDeviceSuccess
			&& description.SessionId == pFileData->SessionId)
		{
			TraceEvents(TRACE_LEVEL_INFORMATION,
				TRACE_DRIVER,
				"Unplugging device with serial %d",
				description.SerialNo);

			// "Unplug" child
			status = WdfChildListUpdateChildDescriptionAsMissing(list, &description.Header);
			if (!NT_SUCCESS(status))
			{
				TraceError(
					TRACE_DRIVER,
					"WdfChildListUpdateChildDescriptionAsMissing failed with status %!STATUS!",
					status);
			}
		}
	}

	WdfChildListEndIteration(list, &iterator);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit with status %!STATUS!", status);
}

VOID
Bus_EvtDriverContextCleanup(
	_In_ WDFOBJECT DriverObject
)
/*++
Routine Description:

	Free all the resources allocated in DriverEntry.

Arguments:

	DriverObject - handle to a WDF Driver object.

Return Value:

	VOID.

--*/
{
	UNREFERENCED_PARAMETER(DriverObject);

	PAGED_CODE();

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

	//
	// Stop WPP Tracing
	//
	WPP_CLEANUP(WdfDriverWdmGetDriverObject((WDFDRIVER)DriverObject));

}

void Bus_EvtUserNotifyRequestComplete(
	_In_ DMFMODULE DmfModule,
	_In_ WDFREQUEST Request,
	_In_opt_ ULONG_PTR Context,
	_In_ NTSTATUS NtStatus
)
{
	UNREFERENCED_PARAMETER(DmfModule);

	auto pOutput = reinterpret_cast<PDS4_AWAIT_OUTPUT>(Context);
	PDS4_AWAIT_OUTPUT pNotify = NULL;
	size_t length = 0;

	if (NT_SUCCESS(WdfRequestRetrieveOutputBuffer(
		Request,
		sizeof(DS4_AWAIT_OUTPUT),
		reinterpret_cast<PVOID*>(&pNotify),
		&length)))
	{
		RtlCopyMemory(pNotify, pOutput, sizeof(DS4_AWAIT_OUTPUT));

		Util_DumpAsHex("NOTIFY_COMPLETE", pNotify, sizeof(DS4_AWAIT_OUTPUT));

		WdfRequestSetInformation(Request, sizeof(DS4_AWAIT_OUTPUT));
	}

	WdfRequestComplete(Request, NtStatus);
}

void Util_DumpAsHex(PCSTR Prefix, PVOID Buffer, ULONG BufferLength)
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

EXTERN_C_END
