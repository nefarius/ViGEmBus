/*
* Virtual Gamepad Emulation Framework - Windows kernel-mode bus driver
*
* MIT License
*
* Copyright (c) 2016-2019 Nefarius Software Solutions e.U. and Contributors
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/


#include "busenum.h"
#include "busenum.tmh"

#include "EmulationTargetPDO.hpp"
#include "XusbPdo.hpp"
#include "Ds4Pdo.hpp"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, Bus_PlugInDevice)
#pragma alloc_text (PAGE, Bus_UnPlugDevice)
#endif

using ViGEm::Bus::Core::PDO_IDENTIFICATION_DESCRIPTION;
using ViGEm::Bus::Core::EmulationTargetPDO;
using ViGEm::Bus::Targets::EmulationTargetXUSB;
using ViGEm::Bus::Targets::EmulationTargetDS4;

//
// Simulates a device plug-in event.
// 
EXTERN_C NTSTATUS Bus_PlugInDevice(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_ BOOLEAN IsInternal,
    _Out_ size_t* Transferred)
{
	PDO_IDENTIFICATION_DESCRIPTION  description;
    NTSTATUS                        status;
    PVIGEM_PLUGIN_TARGET            plugIn;
    WDFFILEOBJECT                   fileObject;
    PFDO_FILE_DATA                  pFileData;
    size_t                          length = 0;

    UNREFERENCED_PARAMETER(IsInternal);
	
    PAGED_CODE();


    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_BUSENUM, "%!FUNC! Entry");

    status = WdfRequestRetrieveInputBuffer(
        Request, 
        sizeof(VIGEM_PLUGIN_TARGET), 
        reinterpret_cast<PVOID*>(&plugIn), 
        &length
    );
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_BUSENUM,
            "WdfRequestRetrieveInputBuffer failed with status %!STATUS!", status);
        return status;
    }

    if ((sizeof(VIGEM_PLUGIN_TARGET) != plugIn->Size) || (length != plugIn->Size))
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_BUSENUM,
            "sizeof(VIGEM_PLUGIN_TARGET) buffer size mismatch [%d != %d]",
            sizeof(VIGEM_PLUGIN_TARGET), plugIn->Size);
        return STATUS_INVALID_PARAMETER;
    }

    if (plugIn->SerialNo == 0)
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_BUSENUM,
            "Serial no. 0 not allowed");
        return STATUS_INVALID_PARAMETER;
    }

    *Transferred = length;

    fileObject = WdfRequestGetFileObject(Request);
    if (fileObject == NULL)
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_BUSENUM,
            "WdfRequestGetFileObject failed to fetch WDFFILEOBJECT from request 0x%p",
            Request);
        return STATUS_INVALID_PARAMETER;
    }

    pFileData = FileObjectGetData(fileObject);
    if (pFileData == NULL)
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_BUSENUM,
            "FileObjectGetData failed to get context data for 0x%p",
            fileObject);
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Initialize the description with the information about the newly
    // plugged in device.
    //
    WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(&description.Header, sizeof(description));

    description.SerialNo = plugIn->SerialNo;
    description.SessionId = pFileData->SessionId;
    
    // Set default IDs if supplied values are invalid
    if (plugIn->VendorId == 0 || plugIn->ProductId == 0)
    {
        switch (plugIn->TargetType)
        {
        case Xbox360Wired:

            description.Target = new EmulationTargetXUSB(plugIn->SerialNo, pFileData->SessionId);

            break;
        case DualShock4Wired:

            description.Target = new EmulationTargetDS4(plugIn->SerialNo, pFileData->SessionId);

            break;
        default:
            return STATUS_NOT_SUPPORTED;
        }
    }
    else
    {
        switch (plugIn->TargetType)
        {
        case Xbox360Wired:

            description.Target = new EmulationTargetXUSB(
                plugIn->SerialNo, 
                pFileData->SessionId,
                plugIn->VendorId,
                plugIn->ProductId
            );

            break;
        case DualShock4Wired:

            description.Target = new EmulationTargetDS4(
                plugIn->SerialNo, 
                pFileData->SessionId,
                plugIn->VendorId,
                plugIn->ProductId
            );

            break;
        default:
            return STATUS_NOT_SUPPORTED;
        }
    }

    status = WdfChildListAddOrUpdateChildDescriptionAsPresent(
        WdfFdoGetDefaultChildList(Device),
        &description.Header, 
        NULL
    );

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_BUSENUM,
            "WdfChildListAddOrUpdateChildDescriptionAsPresent failed with status %!STATUS!",
            status);

        goto pluginEnd;
    }

    //
    // The requested serial number is already in use
    // 
    if (status == STATUS_OBJECT_NAME_EXISTS)
    {
        status = STATUS_INVALID_PARAMETER;

        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_BUSENUM,
            "The described PDO already exists (%!STATUS!)",
            status);

        goto pluginEnd;
    }

    //status = NT_SUCCESS(status) ? STATUS_PENDING : status;

pluginEnd:

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_BUSENUM, "%!FUNC! Exit with status %!STATUS!", status);

    return status;
}

//
// Simulates a device unplug event.
// 
EXTERN_C NTSTATUS Bus_UnPlugDevice(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_ BOOLEAN IsInternal,
    _Out_ size_t* Transferred)
{
    NTSTATUS                            status;
    WDFDEVICE                           hChild;
    WDFCHILDLIST                        list;
    WDF_CHILD_LIST_ITERATOR             iterator;
    WDF_CHILD_RETRIEVE_INFO             childInfo;
    PDO_IDENTIFICATION_DESCRIPTION      description;
    BOOLEAN                             unplugAll;
    PVIGEM_UNPLUG_TARGET                unPlug;
    WDFFILEOBJECT                       fileObject;
    PFDO_FILE_DATA                      pFileData = NULL;
    size_t                              length = 0;
	
    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_BUSENUM, "%!FUNC! Entry");

    status = WdfRequestRetrieveInputBuffer(
        Request, 
        sizeof(VIGEM_UNPLUG_TARGET), 
        (PVOID*)&unPlug, 
        &length
    );

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_BUSENUM,
            "WdfRequestRetrieveInputBuffer failed with status %!STATUS!",
            status);
        return status;
    }

    if ((sizeof(VIGEM_UNPLUG_TARGET) != unPlug->Size) || (length != unPlug->Size))
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_BUSENUM,
            "sizeof(VIGEM_UNPLUG_TARGET) buffer size mismatch [%d != %d]",
            sizeof(VIGEM_UNPLUG_TARGET), unPlug->Size);
        return STATUS_INVALID_PARAMETER;
    }

    *Transferred = length;
    unplugAll = (unPlug->SerialNo == 0);

    fileObject = WdfRequestGetFileObject(Request);
    if (fileObject == NULL)
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_BUSENUM,
            "WdfRequestGetFileObject failed to fetch WDFFILEOBJECT from request 0x%p",
            Request);
        return STATUS_INVALID_PARAMETER;
    }

    pFileData = FileObjectGetData(fileObject);
    if (pFileData == NULL)
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_BUSENUM,
            "FileObjectGetData failed to get context data for 0x%p",
            fileObject);
        return STATUS_INVALID_PARAMETER;
    }

    TraceEvents(TRACE_LEVEL_VERBOSE,
        TRACE_BUSENUM,
        "Starting child list traversal");

    list = WdfFdoGetDefaultChildList(Device);

    WDF_CHILD_LIST_ITERATOR_INIT(&iterator, WdfRetrievePresentChildren);

    WdfChildListBeginIteration(list, &iterator);

    for (;;)
    {
        WDF_CHILD_RETRIEVE_INFO_INIT(&childInfo, &description.Header);
        WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(&description.Header, sizeof(description));

        status = WdfChildListRetrieveNextDevice(list, &iterator, &hChild, &childInfo);

        // Error or no more children, end loop
        if (!NT_SUCCESS(status) || status == STATUS_NO_MORE_ENTRIES)
        {
            TraceEvents(TRACE_LEVEL_VERBOSE,
                TRACE_BUSENUM,
                "WdfChildListRetrieveNextDevice returned with status %!STATUS!",
                status);
            break;
        }

        // If unable to retrieve device
        if (childInfo.Status != WdfChildListRetrieveDeviceSuccess)
        {
            TraceEvents(TRACE_LEVEL_VERBOSE,
                TRACE_BUSENUM,
                "childInfo.Status = %d",
                childInfo.Status);
            continue;
        }

        // Child isn't the one we looked for, skip
        if (!unplugAll && description.SerialNo != unPlug->SerialNo)
        {
            TraceEvents(TRACE_LEVEL_VERBOSE,
                TRACE_BUSENUM,
                "Seeking serial mismatch: %d != %d",
                description.SerialNo,
                unPlug->SerialNo);
            continue;
        }

        TraceEvents(TRACE_LEVEL_VERBOSE,
            TRACE_BUSENUM,
            "description.SessionId = %d, pFileData->SessionId = %d",
            description.SessionId,
            pFileData->SessionId);

        // Only unplug owned children
        if (IsInternal || description.SessionId == pFileData->SessionId)
        {
            // Unplug child
            status = WdfChildListUpdateChildDescriptionAsMissing(list, &description.Header);
            if (!NT_SUCCESS(status))
            {
                TraceEvents(TRACE_LEVEL_ERROR,
                    TRACE_BUSENUM,
                    "WdfChildListUpdateChildDescriptionAsMissing failed with status %!STATUS!",
                    status);
            }
        }
    }

    WdfChildListEndIteration(list, &iterator);

    TraceEvents(TRACE_LEVEL_VERBOSE,
        TRACE_BUSENUM,
        "Finished child list traversal");

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_BUSENUM, "%!FUNC! Exit with status %!STATUS!", STATUS_SUCCESS);

    return STATUS_SUCCESS;
}

//
// Sends a report update to an XUSB PDO.
// 
EXTERN_C NTSTATUS Bus_XusbSubmitReport(WDFDEVICE Device, ULONG SerialNo, PXUSB_SUBMIT_REPORT Report, BOOLEAN FromInterface)
{
    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_BUSENUM, "%!FUNC! Entry");

    return Bus_SubmitReport(Device, SerialNo, Report, FromInterface);
}

//
// Queues an inverted call to receive XUSB-specific updates.
// 
EXTERN_C NTSTATUS Bus_QueueNotification(WDFDEVICE Device, ULONG SerialNo, WDFREQUEST Request)
{
    NTSTATUS status;

    TraceDbg(TRACE_BUSENUM, "%!FUNC! Entry");

    const auto pdo = EmulationTargetPDO::GetPdoBySerial(Device, SerialNo);

    // Validate child
    if (pdo == nullptr)
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_BUSENUM,
            "PDO with serial %d not found",
            SerialNo);
        return STATUS_NO_SUCH_DEVICE;
    }

    status = pdo->EnqueueNotification(Request);

    status = (NT_SUCCESS(status)) ? STATUS_PENDING : status;

    TraceDbg(TRACE_BUSENUM, "%!FUNC! Exit with status %!STATUS!", status);

    return status;
}

//
// Sends a report update to a DS4 PDO.
// 
NTSTATUS Bus_Ds4SubmitReport(WDFDEVICE Device, ULONG SerialNo, PDS4_SUBMIT_REPORT Report, BOOLEAN FromInterface)
{
    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_BUSENUM, "%!FUNC! Entry");

    return Bus_SubmitReport(Device, SerialNo, Report, FromInterface);
}

EXTERN_C NTSTATUS Bus_SubmitReport(WDFDEVICE Device, ULONG SerialNo, PVOID Report, BOOLEAN FromInterface)
{
    UNREFERENCED_PARAMETER(FromInterface);
	
    const auto pdo = EmulationTargetPDO::GetPdoBySerial(Device, SerialNo);

    // Validate child
    if (pdo == nullptr)
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_BUSENUM,
            "PDO with serial %d not found",
            SerialNo);
        return STATUS_NO_SUCH_DEVICE;
    }

    return pdo->SubmitReport(Report);
}
