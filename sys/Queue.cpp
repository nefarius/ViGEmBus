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


#include "Driver.h"
#include "trace.h"
#include "Queue.tmh"

#include "EmulationTargetPDO.hpp"
#include "XusbPdo.hpp"
#include "Ds4Pdo.hpp"

using ViGEm::Bus::Core::PDO_IDENTIFICATION_DESCRIPTION;
using ViGEm::Bus::Core::EmulationTargetPDO;
using ViGEm::Bus::Targets::EmulationTargetXUSB;
using ViGEm::Bus::Targets::EmulationTargetDS4;


EXTERN_C_START

NTSTATUS
Bus_CheckVersionHandler(
	_In_ DMFMODULE DmfModule,
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request,
	_In_ ULONG IoctlCode,
	_In_reads_(InputBufferSize) VOID* InputBuffer,
	_In_ size_t InputBufferSize,
	_Out_writes_(OutputBufferSize) VOID* OutputBuffer,
	_In_ size_t OutputBufferSize,
	_Out_ size_t* BytesReturned
)
{
	UNREFERENCED_PARAMETER(DmfModule);
	UNREFERENCED_PARAMETER(Queue);
	UNREFERENCED_PARAMETER(Request);
	UNREFERENCED_PARAMETER(IoctlCode);
	UNREFERENCED_PARAMETER(InputBufferSize);
	UNREFERENCED_PARAMETER(OutputBufferSize);
	UNREFERENCED_PARAMETER(OutputBuffer);
	UNREFERENCED_PARAMETER(BytesReturned);

	FuncEntry(TRACE_QUEUE);

	PVIGEM_CHECK_VERSION pCheckVersion = (PVIGEM_CHECK_VERSION)InputBuffer;

	NTSTATUS status = (pCheckVersion->Version == VIGEM_COMMON_VERSION) ? STATUS_SUCCESS : STATUS_NOT_SUPPORTED;

	TraceVerbose(
		TRACE_QUEUE,
		"Requested version: 0x%04X, compiled version: 0x%04X",
		pCheckVersion->Version, VIGEM_COMMON_VERSION);

	FuncExit(TRACE_QUEUE, "status=%!STATUS!", status);

	return status;
}

NTSTATUS
Bus_WaitDeviceReadyHandler(
	_In_ DMFMODULE DmfModule,
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request,
	_In_ ULONG IoctlCode,
	_In_reads_(InputBufferSize) VOID* InputBuffer,
	_In_ size_t InputBufferSize,
	_Out_writes_(OutputBufferSize) VOID* OutputBuffer,
	_In_ size_t OutputBufferSize,
	_Out_ size_t* BytesReturned
)
{
	UNREFERENCED_PARAMETER(DmfModule);
	UNREFERENCED_PARAMETER(Request);
	UNREFERENCED_PARAMETER(IoctlCode);
	UNREFERENCED_PARAMETER(InputBufferSize);
	UNREFERENCED_PARAMETER(OutputBufferSize);
	UNREFERENCED_PARAMETER(OutputBuffer);
	UNREFERENCED_PARAMETER(BytesReturned);

	NTSTATUS status;

	FuncEntry(TRACE_QUEUE);

	PVIGEM_WAIT_DEVICE_READY pWaitDeviceReady = (PVIGEM_WAIT_DEVICE_READY)InputBuffer;

	// This request only supports a single PDO at a time
	if (pWaitDeviceReady->SerialNo == 0)
	{
		TraceError(
			TRACE_QUEUE,
			"Invalid serial 0 submitted");

		status = STATUS_INVALID_PARAMETER;
		goto exit;
	}

	status = EmulationTargetPDO::EnqueueWaitDeviceReady(
		WdfIoQueueGetDevice(Queue),
		pWaitDeviceReady->SerialNo,
		Request
	);

	status = NT_SUCCESS(status) ? STATUS_PENDING : STATUS_DEVICE_DOES_NOT_EXIST;

exit:
	FuncExit(TRACE_QUEUE, "status=%!STATUS!", status);

	return status;
}

NTSTATUS
Bus_PluginTargetHandler(
	_In_ DMFMODULE DmfModule,
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request,
	_In_ ULONG IoctlCode,
	_In_reads_(InputBufferSize) VOID* InputBuffer,
	_In_ size_t InputBufferSize,
	_Out_writes_(OutputBufferSize) VOID* OutputBuffer,
	_In_ size_t OutputBufferSize,
	_Out_ size_t* BytesReturned
)
{
	UNREFERENCED_PARAMETER(DmfModule);
	UNREFERENCED_PARAMETER(Request);
	UNREFERENCED_PARAMETER(IoctlCode);
	UNREFERENCED_PARAMETER(InputBufferSize);
	UNREFERENCED_PARAMETER(InputBuffer);
	UNREFERENCED_PARAMETER(OutputBufferSize);
	UNREFERENCED_PARAMETER(OutputBuffer);
	UNREFERENCED_PARAMETER(BytesReturned);

	FuncEntry(TRACE_QUEUE);

	NTSTATUS status = Bus_PlugInDevice(WdfIoQueueGetDevice(Queue), Request, FALSE, BytesReturned);

	FuncExit(TRACE_QUEUE, "status=%!STATUS!", status);

	return status;
}

NTSTATUS
Bus_UnplugTargetHandler(
	_In_ DMFMODULE DmfModule,
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request,
	_In_ ULONG IoctlCode,
	_In_reads_(InputBufferSize) VOID* InputBuffer,
	_In_ size_t InputBufferSize,
	_Out_writes_(OutputBufferSize) VOID* OutputBuffer,
	_In_ size_t OutputBufferSize,
	_Out_ size_t* BytesReturned
)
{
	UNREFERENCED_PARAMETER(DmfModule);
	UNREFERENCED_PARAMETER(Request);
	UNREFERENCED_PARAMETER(IoctlCode);
	UNREFERENCED_PARAMETER(InputBufferSize);
	UNREFERENCED_PARAMETER(InputBuffer);
	UNREFERENCED_PARAMETER(OutputBufferSize);
	UNREFERENCED_PARAMETER(OutputBuffer);
	UNREFERENCED_PARAMETER(BytesReturned);

	FuncEntry(TRACE_QUEUE);

	NTSTATUS status = Bus_UnPlugDevice(WdfIoQueueGetDevice(Queue), Request, FALSE, BytesReturned);

	FuncExit(TRACE_QUEUE, "status=%!STATUS!", status);

	return status;
}

NTSTATUS
Bus_XusbSubmitReportHandler(
	_In_ DMFMODULE DmfModule,
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request,
	_In_ ULONG IoctlCode,
	_In_reads_(InputBufferSize) VOID* InputBuffer,
	_In_ size_t InputBufferSize,
	_Out_writes_(OutputBufferSize) VOID* OutputBuffer,
	_In_ size_t OutputBufferSize,
	_Out_ size_t* BytesReturned
)
{
	UNREFERENCED_PARAMETER(DmfModule);
	UNREFERENCED_PARAMETER(Request);
	UNREFERENCED_PARAMETER(IoctlCode);
	UNREFERENCED_PARAMETER(InputBufferSize);
	UNREFERENCED_PARAMETER(InputBuffer);
	UNREFERENCED_PARAMETER(OutputBufferSize);
	UNREFERENCED_PARAMETER(OutputBuffer);
	UNREFERENCED_PARAMETER(BytesReturned);

	FuncEntry(TRACE_QUEUE);

	NTSTATUS status;
	EmulationTargetPDO* pdo;
	PXUSB_SUBMIT_REPORT xusbSubmit = (PXUSB_SUBMIT_REPORT)InputBuffer;

	// This request only supports a single PDO at a time
	if (xusbSubmit->SerialNo == 0)
	{
		TraceError(
			TRACE_QUEUE,
			"Invalid serial 0 submitted");

		status = STATUS_INVALID_PARAMETER;
		goto exit;
	}

	if (!EmulationTargetPDO::GetPdoByTypeAndSerial(WdfIoQueueGetDevice(Queue), Xbox360Wired, xusbSubmit->SerialNo, &pdo))
		status = STATUS_DEVICE_DOES_NOT_EXIST;
	else
		status = pdo->SubmitReport(xusbSubmit);

exit:
	FuncExit(TRACE_QUEUE, "status=%!STATUS!", status);

	return status;
}

NTSTATUS
Bus_XusbRequestNotificationHandler(
	_In_ DMFMODULE DmfModule,
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request,
	_In_ ULONG IoctlCode,
	_In_reads_(InputBufferSize) VOID* InputBuffer,
	_In_ size_t InputBufferSize,
	_Out_writes_(OutputBufferSize) VOID* OutputBuffer,
	_In_ size_t OutputBufferSize,
	_Out_ size_t* BytesReturned
)
{
	UNREFERENCED_PARAMETER(DmfModule);
	UNREFERENCED_PARAMETER(Request);
	UNREFERENCED_PARAMETER(IoctlCode);
	UNREFERENCED_PARAMETER(InputBufferSize);
	UNREFERENCED_PARAMETER(InputBuffer);
	UNREFERENCED_PARAMETER(OutputBufferSize);
	UNREFERENCED_PARAMETER(OutputBuffer);
	UNREFERENCED_PARAMETER(BytesReturned);

	FuncEntry(TRACE_QUEUE);

	NTSTATUS status;
	EmulationTargetPDO* pdo;
	PXUSB_REQUEST_NOTIFICATION xusbNotify = (PXUSB_REQUEST_NOTIFICATION)InputBuffer;

	// This request only supports a single PDO at a time
	if (xusbNotify->SerialNo == 0)
	{
		TraceError(
			TRACE_QUEUE,
			"Invalid serial 0 submitted");

		status = STATUS_INVALID_PARAMETER;
		goto exit;
	}

	if (!EmulationTargetPDO::GetPdoByTypeAndSerial(WdfIoQueueGetDevice(Queue), Xbox360Wired, xusbNotify->SerialNo, &pdo))
		status = STATUS_DEVICE_DOES_NOT_EXIST;
	else
	{
		status = pdo->EnqueueNotification(Request);

		status = (NT_SUCCESS(status)) ? STATUS_PENDING : status;
	}

exit:
	FuncExit(TRACE_QUEUE, "status=%!STATUS!", status);

	return status;
}

NTSTATUS
Bus_Ds4SubmitReportHandler(
	_In_ DMFMODULE DmfModule,
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request,
	_In_ ULONG IoctlCode,
	_In_reads_(InputBufferSize) VOID* InputBuffer,
	_In_ size_t InputBufferSize,
	_Out_writes_(OutputBufferSize) VOID* OutputBuffer,
	_In_ size_t OutputBufferSize,
	_Out_ size_t* BytesReturned
)
{
	UNREFERENCED_PARAMETER(DmfModule);
	UNREFERENCED_PARAMETER(Request);
	UNREFERENCED_PARAMETER(IoctlCode);
	UNREFERENCED_PARAMETER(OutputBufferSize);
	UNREFERENCED_PARAMETER(OutputBuffer);
	UNREFERENCED_PARAMETER(BytesReturned);

	FuncEntry(TRACE_QUEUE);

	NTSTATUS status;
	EmulationTargetPDO* pdo;
	PDS4_SUBMIT_REPORT ds4Submit = (PDS4_SUBMIT_REPORT)InputBuffer;

	//
		// Check if buffer is within expected bounds
		// 
	if (InputBufferSize < sizeof(DS4_SUBMIT_REPORT) || InputBufferSize > sizeof(DS4_SUBMIT_REPORT_EX))
	{
		TraceVerbose(
			TRACE_QUEUE,
			"Unexpected buffer size: %d",
			static_cast<ULONG>(InputBufferSize)
		);

		status = STATUS_INVALID_BUFFER_SIZE;
		goto exit;
	}

	//
	// Check if this makes sense before passing it on
	// 
	if (InputBufferSize != ds4Submit->Size)
	{
		TraceVerbose(
			TRACE_QUEUE,
			"Invalid buffer size: %d",
			ds4Submit->Size
		);

		status = STATUS_INVALID_BUFFER_SIZE;
		goto exit;
	}

	// 
	// This request only supports a single PDO at a time
	// 
	if (ds4Submit->SerialNo == 0)
	{
		TraceError(
			TRACE_QUEUE,
			"Invalid serial 0 submitted");

		status = STATUS_INVALID_PARAMETER;
		goto exit;
	}

	if (!EmulationTargetPDO::GetPdoByTypeAndSerial(WdfIoQueueGetDevice(Queue), DualShock4Wired, ds4Submit->SerialNo, &pdo))
		status = STATUS_DEVICE_DOES_NOT_EXIST;
	else
		status = pdo->SubmitReport(ds4Submit);

exit:
	FuncExit(TRACE_QUEUE, "status=%!STATUS!", status);

	return status;
}

NTSTATUS
Bus_Ds4RequestNotificationHandler(
	_In_ DMFMODULE DmfModule,
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request,
	_In_ ULONG IoctlCode,
	_In_reads_(InputBufferSize) VOID* InputBuffer,
	_In_ size_t InputBufferSize,
	_Out_writes_(OutputBufferSize) VOID* OutputBuffer,
	_In_ size_t OutputBufferSize,
	_Out_ size_t* BytesReturned
)
{
	UNREFERENCED_PARAMETER(DmfModule);
	UNREFERENCED_PARAMETER(Request);
	UNREFERENCED_PARAMETER(IoctlCode);
	UNREFERENCED_PARAMETER(OutputBufferSize);
	UNREFERENCED_PARAMETER(InputBufferSize);
	UNREFERENCED_PARAMETER(OutputBuffer);
	UNREFERENCED_PARAMETER(BytesReturned);

	FuncEntry(TRACE_QUEUE);

	NTSTATUS status;
	EmulationTargetPDO* pdo;
	PDS4_REQUEST_NOTIFICATION ds4Notify = (PDS4_REQUEST_NOTIFICATION)InputBuffer;

	// This request only supports a single PDO at a time
	if (ds4Notify->SerialNo == 0)
	{
		TraceError(
			TRACE_QUEUE,
			"Invalid serial 0 submitted");

		status = STATUS_INVALID_PARAMETER;
		goto exit;
	}

	if (!EmulationTargetPDO::GetPdoByTypeAndSerial(WdfIoQueueGetDevice(Queue), DualShock4Wired, ds4Notify->SerialNo, &pdo))
		status = STATUS_DEVICE_DOES_NOT_EXIST;
	else
	{
		status = pdo->EnqueueNotification(Request);

		status = (NT_SUCCESS(status)) ? STATUS_PENDING : status;
	}

exit:
	FuncExit(TRACE_QUEUE, "status=%!STATUS!", status);

	return status;
}

NTSTATUS
Bus_XusbGetUserIndexHandler(
	_In_ DMFMODULE DmfModule,
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request,
	_In_ ULONG IoctlCode,
	_In_reads_(InputBufferSize) VOID* InputBuffer,
	_In_ size_t InputBufferSize,
	_Out_writes_(OutputBufferSize) VOID* OutputBuffer,
	_In_ size_t OutputBufferSize,
	_Out_ size_t* BytesReturned
)
{
	UNREFERENCED_PARAMETER(DmfModule);
	UNREFERENCED_PARAMETER(Request);
	UNREFERENCED_PARAMETER(IoctlCode);
	UNREFERENCED_PARAMETER(OutputBufferSize);
	UNREFERENCED_PARAMETER(InputBufferSize);
	UNREFERENCED_PARAMETER(OutputBuffer);
	UNREFERENCED_PARAMETER(BytesReturned);

	FuncEntry(TRACE_QUEUE);

	NTSTATUS status;
	EmulationTargetPDO* pdo;
	PXUSB_GET_USER_INDEX pXusbGetUserIndex = (PXUSB_GET_USER_INDEX)InputBuffer;

	// This request only supports a single PDO at a time
	if (pXusbGetUserIndex->SerialNo == 0)
	{
		status = STATUS_INVALID_PARAMETER;
		goto exit;
	}

	if (!EmulationTargetPDO::GetPdoByTypeAndSerial(WdfIoQueueGetDevice(Queue), Xbox360Wired, pXusbGetUserIndex->SerialNo, &pdo))
	{
		status = STATUS_DEVICE_DOES_NOT_EXIST;
		goto exit;
	}

	status = static_cast<EmulationTargetXUSB*>(pdo)->GetUserIndex(&pXusbGetUserIndex->UserIndex);

exit:
	FuncExit(TRACE_QUEUE, "status=%!STATUS!", status);

	return status;
}

NTSTATUS
Bus_Ds4AwaitOutputHandler(
	_In_ DMFMODULE DmfModule,
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request,
	_In_ ULONG IoctlCode,
	_In_reads_(InputBufferSize) VOID* InputBuffer,
	_In_ size_t InputBufferSize,
	_Out_writes_(OutputBufferSize) VOID* OutputBuffer,
	_In_ size_t OutputBufferSize,
	_Out_ size_t* BytesReturned
)
{
	UNREFERENCED_PARAMETER(DmfModule);
	UNREFERENCED_PARAMETER(Request);
	UNREFERENCED_PARAMETER(IoctlCode);
	UNREFERENCED_PARAMETER(OutputBufferSize);
	UNREFERENCED_PARAMETER(InputBufferSize);
	UNREFERENCED_PARAMETER(OutputBuffer);
	UNREFERENCED_PARAMETER(BytesReturned);

	FuncEntry(TRACE_QUEUE);

	NTSTATUS status;
	EmulationTargetPDO* pdo;
	PDS4_AWAIT_OUTPUT pDs4AwaitOut = (PDS4_AWAIT_OUTPUT)InputBuffer;

	// This request only supports a single PDO at a time
	if (pDs4AwaitOut->SerialNo == 0)
	{
		status = STATUS_INVALID_PARAMETER;
		goto exit;
	}

	if (!EmulationTargetPDO::GetPdoByTypeAndSerial(WdfIoQueueGetDevice(Queue), DualShock4Wired, pDs4AwaitOut->SerialNo, &pdo))
	{
		status = STATUS_DEVICE_DOES_NOT_EXIST;
		goto exit;
	}

	status = static_cast<EmulationTargetDS4*>(pdo)->OutputReportRequestProcess(Request);

	status = NT_SUCCESS(status) ? STATUS_PENDING : status;

exit:
	FuncExit(TRACE_QUEUE, "status=%!STATUS!", status);

	return status;
}

EXTERN_C_END
