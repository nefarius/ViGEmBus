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


#include "busenum.h"
#include "queue.tmh"

#include "EmulationTargetPDO.hpp"
#include "XusbPdo.hpp"
#include "Ds4Pdo.hpp"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, Bus_EvtIoDefault)
#endif

using ViGEm::Bus::Core::PDO_IDENTIFICATION_DESCRIPTION;
using ViGEm::Bus::Core::EmulationTargetPDO;
using ViGEm::Bus::Targets::EmulationTargetXUSB;
using ViGEm::Bus::Targets::EmulationTargetDS4;


EXTERN_C_START

//
// Responds to I/O control requests sent to the FDO.
// 
VOID Bus_EvtIoDeviceControl(
	IN WDFQUEUE Queue,
	IN WDFREQUEST Request,
	IN size_t OutputBufferLength,
	IN size_t InputBufferLength,
	IN ULONG IoControlCode
)
{
	NTSTATUS status = STATUS_INVALID_PARAMETER;
	WDFDEVICE Device;
	size_t length = 0;
	PXUSB_SUBMIT_REPORT xusbSubmit = nullptr;
	PXUSB_REQUEST_NOTIFICATION xusbNotify = nullptr;
	PDS4_SUBMIT_REPORT ds4Submit = nullptr;
	PDS4_REQUEST_NOTIFICATION ds4Notify = nullptr;
	PVIGEM_CHECK_VERSION pCheckVersion = nullptr;
	PXUSB_GET_USER_INDEX pXusbGetUserIndex = nullptr;
	EmulationTargetPDO* pdo;

	Device = WdfIoQueueGetDevice(Queue);

	TraceDbg(TRACE_QUEUE, "%!FUNC! Entry (device: 0x%p)", Device);

	switch (IoControlCode)
	{
#pragma region IOCTL_VIGEM_CHECK_VERSION

	case IOCTL_VIGEM_CHECK_VERSION:

		TraceDbg(TRACE_QUEUE, "IOCTL_VIGEM_CHECK_VERSION");

		status = WdfRequestRetrieveInputBuffer(
			Request,
			sizeof(VIGEM_CHECK_VERSION),
			reinterpret_cast<PVOID*>(&pCheckVersion),
			&length
		);

		if (!NT_SUCCESS(status) || length != sizeof(VIGEM_CHECK_VERSION))
		{
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		status = (pCheckVersion->Version == VIGEM_COMMON_VERSION) ? STATUS_SUCCESS : STATUS_NOT_SUPPORTED;

		TraceEvents(TRACE_LEVEL_VERBOSE,
		            TRACE_QUEUE,
		            "Requested version: 0x%04X, compiled version: 0x%04X",
		            pCheckVersion->Version, VIGEM_COMMON_VERSION);

		break;

#pragma endregion

#pragma region IOCTL_VIGEM_PLUGIN_TARGET

	case IOCTL_VIGEM_PLUGIN_TARGET:

		TraceDbg(TRACE_QUEUE, "IOCTL_VIGEM_PLUGIN_TARGET");

		status = Bus_PlugInDevice(Device, Request, FALSE, &length);

		break;

#pragma endregion

#pragma region IOCTL_VIGEM_UNPLUG_TARGET

	case IOCTL_VIGEM_UNPLUG_TARGET:

		TraceDbg(TRACE_QUEUE, "IOCTL_VIGEM_UNPLUG_TARGET");

		status = Bus_UnPlugDevice(Device, Request, FALSE, &length);

		break;

#pragma endregion

#pragma region IOCTL_XUSB_SUBMIT_REPORT

	case IOCTL_XUSB_SUBMIT_REPORT:

		TraceDbg(TRACE_QUEUE, "IOCTL_XUSB_SUBMIT_REPORT");

		status = WdfRequestRetrieveInputBuffer(
			Request,
			sizeof(XUSB_SUBMIT_REPORT),
			reinterpret_cast<PVOID*>(&xusbSubmit),
			&length
		);

		if (!NT_SUCCESS(status))
		{
			TraceEvents(TRACE_LEVEL_ERROR,
			            TRACE_QUEUE,
			            "WdfRequestRetrieveInputBuffer failed with status %!STATUS!",
			            status);
			break;
		}

		if ((sizeof(XUSB_SUBMIT_REPORT) == xusbSubmit->Size) && (length == InputBufferLength))
		{
			// This request only supports a single PDO at a time
			if (xusbSubmit->SerialNo == 0)
			{
				TraceEvents(TRACE_LEVEL_ERROR,
				            TRACE_QUEUE,
				            "Invalid serial 0 submitted");

				status = STATUS_INVALID_PARAMETER;
				break;
			}

			if (!EmulationTargetPDO::GetPdoByTypeAndSerial(Device, Xbox360Wired, xusbSubmit->SerialNo, &pdo))
				status = STATUS_DEVICE_DOES_NOT_EXIST;
			else
				status = pdo->SubmitReport(xusbSubmit);
		}

		break;

#pragma endregion

#pragma region IOCTL_XUSB_REQUEST_NOTIFICATION

	case IOCTL_XUSB_REQUEST_NOTIFICATION:

		TraceDbg(TRACE_QUEUE, "IOCTL_XUSB_REQUEST_NOTIFICATION");

		// Don't accept the request if the output buffer can't hold the results
		if (OutputBufferLength < sizeof(XUSB_REQUEST_NOTIFICATION))
		{
			TraceEvents(TRACE_LEVEL_ERROR,
			            TRACE_QUEUE,
			            "Output buffer %d too small, require at least %d",
			            static_cast<int>(OutputBufferLength), static_cast<int>(sizeof(XUSB_REQUEST_NOTIFICATION)));
			break;
		}

		status = WdfRequestRetrieveInputBuffer(
			Request,
			sizeof(XUSB_REQUEST_NOTIFICATION),
			reinterpret_cast<PVOID*>(&xusbNotify),
			&length
		);

		if (!NT_SUCCESS(status))
		{
			TraceEvents(TRACE_LEVEL_ERROR,
			            TRACE_QUEUE,
			            "WdfRequestRetrieveInputBuffer failed with status %!STATUS!",
			            status);
			break;
		}

		if ((sizeof(XUSB_REQUEST_NOTIFICATION) == xusbNotify->Size) && (length == InputBufferLength))
		{
			// This request only supports a single PDO at a time
			if (xusbNotify->SerialNo == 0)
			{
				TraceEvents(TRACE_LEVEL_ERROR,
				            TRACE_QUEUE,
				            "Invalid serial 0 submitted");

				status = STATUS_INVALID_PARAMETER;
				break;
			}

			if (!EmulationTargetPDO::GetPdoByTypeAndSerial(Device, Xbox360Wired, xusbNotify->SerialNo, &pdo))
				status = STATUS_DEVICE_DOES_NOT_EXIST;
			else
			{
				status = pdo->EnqueueNotification(Request);

				status = (NT_SUCCESS(status)) ? STATUS_PENDING : status;
			}
		}

		break;

#pragma endregion

#pragma region IOCTL_DS4_SUBMIT_REPORT

	case IOCTL_DS4_SUBMIT_REPORT:

		TraceDbg(TRACE_QUEUE, "IOCTL_DS4_SUBMIT_REPORT");

		status = WdfRequestRetrieveInputBuffer(
			Request,
			sizeof(DS4_SUBMIT_REPORT),
			reinterpret_cast<PVOID*>(&ds4Submit),
			&length
		);

		if (!NT_SUCCESS(status))
		{
			TraceEvents(TRACE_LEVEL_ERROR,
			            TRACE_QUEUE,
			            "WdfRequestRetrieveInputBuffer failed with status %!STATUS!",
			            status);
			break;
		}

		if ((sizeof(DS4_SUBMIT_REPORT) == ds4Submit->Size) && (length == InputBufferLength))
		{
			// This request only supports a single PDO at a time
			if (ds4Submit->SerialNo == 0)
			{
				TraceEvents(TRACE_LEVEL_ERROR,
				            TRACE_QUEUE,
				            "Invalid serial 0 submitted");

				status = STATUS_INVALID_PARAMETER;
				break;
			}

			if (!EmulationTargetPDO::GetPdoByTypeAndSerial(Device, DualShock4Wired, ds4Submit->SerialNo, &pdo))
				status = STATUS_DEVICE_DOES_NOT_EXIST;
			else
				status = pdo->SubmitReport(ds4Submit);
		}

		break;

#pragma endregion

#pragma region IOCTL_DS4_REQUEST_NOTIFICATION

	case IOCTL_DS4_REQUEST_NOTIFICATION:

		TraceDbg(TRACE_QUEUE, "IOCTL_DS4_REQUEST_NOTIFICATION");

		// Don't accept the request if the output buffer can't hold the results
		if (OutputBufferLength < sizeof(DS4_REQUEST_NOTIFICATION))
		{
			TraceEvents(TRACE_LEVEL_ERROR,
			            TRACE_QUEUE,
			            "Output buffer %d too small, require at least %d",
			            static_cast<int>(OutputBufferLength), static_cast<int>(sizeof(DS4_REQUEST_NOTIFICATION)));
			break;
		}

		status = WdfRequestRetrieveInputBuffer(
			Request,
			sizeof(DS4_REQUEST_NOTIFICATION),
			reinterpret_cast<PVOID*>(&ds4Notify),
			&length
		);

		if (!NT_SUCCESS(status))
		{
			TraceEvents(TRACE_LEVEL_ERROR,
			            TRACE_QUEUE,
			            "WdfRequestRetrieveInputBuffer failed with status %!STATUS!",
			            status);
			break;
		}

		if ((sizeof(DS4_REQUEST_NOTIFICATION) == ds4Notify->Size) && (length == InputBufferLength))
		{
			// This request only supports a single PDO at a time
			if (ds4Notify->SerialNo == 0)
			{
				TraceEvents(TRACE_LEVEL_ERROR,
				            TRACE_QUEUE,
				            "Invalid serial 0 submitted");

				status = STATUS_INVALID_PARAMETER;
				break;
			}

			if (!EmulationTargetPDO::GetPdoByTypeAndSerial(Device, DualShock4Wired, ds4Notify->SerialNo, &pdo))
				status = STATUS_DEVICE_DOES_NOT_EXIST;
			else
			{
				status = pdo->EnqueueNotification(Request);

				status = (NT_SUCCESS(status)) ? STATUS_PENDING : status;
			}
		}

		break;

#pragma endregion

#pragma region IOCTL_XUSB_GET_USER_INDEX

	case IOCTL_XUSB_GET_USER_INDEX:

		TraceDbg(TRACE_QUEUE, "IOCTL_XUSB_GET_USER_INDEX");

		// Don't accept the request if the output buffer can't hold the results
		if (OutputBufferLength < sizeof(XUSB_GET_USER_INDEX))
		{
			KdPrint((DRIVERNAME "IOCTL_XUSB_GET_USER_INDEX: output buffer too small: %ul\n", OutputBufferLength));
			break;
		}

		status = WdfRequestRetrieveInputBuffer(
			Request,
			sizeof(XUSB_GET_USER_INDEX),
			reinterpret_cast<PVOID*>(&pXusbGetUserIndex),
			&length);

		if (!NT_SUCCESS(status))
		{
			KdPrint((DRIVERNAME "WdfRequestRetrieveInputBuffer failed 0x%x\n", status));
			break;
		}

		if ((sizeof(XUSB_GET_USER_INDEX) == pXusbGetUserIndex->Size) && (length == InputBufferLength))
		{
			// This request only supports a single PDO at a time
			if (pXusbGetUserIndex->SerialNo == 0)
			{
				status = STATUS_INVALID_PARAMETER;
				break;
			}

			if (!EmulationTargetPDO::GetPdoByTypeAndSerial(Device, Xbox360Wired, pXusbGetUserIndex->SerialNo, &pdo))
			{
				status = STATUS_DEVICE_DOES_NOT_EXIST;
				break;
			}

			status = static_cast<EmulationTargetXUSB*>(pdo)->GetUserIndex(&pXusbGetUserIndex->UserIndex);
		}

		break;

#pragma endregion

	default:

		TraceEvents(TRACE_LEVEL_WARNING,
		            TRACE_QUEUE,
		            "Unknown I/O control code 0x%X", IoControlCode);

		break; // default status is STATUS_INVALID_PARAMETER
	}

	if (status != STATUS_PENDING)
	{
		WdfRequestCompleteWithInformation(Request, status, length);
	}

	TraceDbg(TRACE_QUEUE, "%!FUNC! Exit with status %!STATUS!", status);
}

//
// Catches unsupported requests.
// 
VOID Bus_EvtIoDefault(
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request
)
{
	UNREFERENCED_PARAMETER(Queue);
	UNREFERENCED_PARAMETER(Request);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "%!FUNC! Entry");

	WdfRequestComplete(Request, STATUS_INVALID_DEVICE_REQUEST);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "%!FUNC! Exit");
}

EXTERN_C_END
