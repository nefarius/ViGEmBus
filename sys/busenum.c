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
#include <wdmguid.h>
#include <usb.h>
#include "busenum.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, Bus_PlugInDevice)
#pragma alloc_text (PAGE, Bus_UnPlugDevice)
#endif



//
// Simulates a device plug-in event.
// 
NTSTATUS Bus_PlugInDevice(
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
    WDF_OBJECT_ATTRIBUTES           requestAttribs;
    PFDO_PLUGIN_REQUEST_DATA        pReqData;
    PFDO_DEVICE_DATA                pFdoData;

    PAGED_CODE();


    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_BUSENUM, "%!FUNC! Entry");

    pFdoData = FdoGetData(Device);

    status = WdfRequestRetrieveInputBuffer(Request, sizeof(VIGEM_PLUGIN_TARGET), (PVOID)&plugIn, &length);
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
    description.TargetType = plugIn->TargetType;
    description.OwnerProcessId = CURRENT_PROCESS_ID();
    description.SessionId = pFileData->SessionId;
    description.OwnerIsDriver = IsInternal;

    // Set default IDs if supplied values are invalid
    if (plugIn->VendorId == 0 || plugIn->ProductId == 0)
    {
        switch (plugIn->TargetType)
        {
        case Xbox360Wired:

            description.VendorId = 0x045E;
            description.ProductId = 0x028E;

            break;
        case DualShock4Wired:

            description.VendorId = 0x054C;
            description.ProductId = 0x05C4;

            break;
        default:
            return STATUS_NOT_SUPPORTED;
        }
    }
    else
    {
        description.VendorId = plugIn->VendorId;
        description.ProductId = plugIn->ProductId;
    }

    TraceEvents(TRACE_LEVEL_VERBOSE,
        TRACE_BUSENUM,
        "New PDO properties: serial = %d, type = %d, pid = %d, session = %d, internal = %d, vid = 0x%04X, pid = 0x%04X",
        description.SerialNo,
        description.TargetType,
        description.OwnerProcessId,
        description.SessionId,
        description.OwnerIsDriver,
        description.VendorId,
        description.ProductId
    );

    WdfSpinLockAcquire(pFdoData->PendingPluginRequestsLock);

    TraceEvents(TRACE_LEVEL_VERBOSE,
        TRACE_BUSENUM,
        "Current pending requests count: %d",
        WdfCollectionGetCount(pFdoData->PendingPluginRequests));

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

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&requestAttribs, FDO_PLUGIN_REQUEST_DATA);

    //
    // Allocate context data to request
    // 
    status = WdfObjectAllocateContext(Request, &requestAttribs, (PVOID)&pReqData);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_BUSENUM,
            "WdfObjectAllocateContext failed with status %!STATUS!",
            status);

        goto pluginEnd;
    }

    //
    // Glue current serial to request
    // 
    pReqData->Serial = plugIn->SerialNo;

    //
    // Timestamp the request to track its age
    // 
    pReqData->Timestamp = KeQueryPerformanceCounter(&pReqData->Frequency);

    //
    // Keep track of pending request in collection
    // 
    status = WdfCollectionAdd(pFdoData->PendingPluginRequests, Request);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_BUSENUM,
            "WdfCollectionAdd failed with status %!STATUS!",
            status);

        goto pluginEnd;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION,
        TRACE_BUSENUM,
        "Added item with serial: %d",
        plugIn->SerialNo);

    //
    // At least one request present in the collection; start clean-up timer
    // 
    WdfTimerStart(
        pFdoData->PendingPluginRequestsCleanupTimer,
        WDF_REL_TIMEOUT_IN_MS(ORC_TIMER_PERIODIC_DUE_TIME)
    );
    TraceEvents(TRACE_LEVEL_VERBOSE,
        TRACE_DRIVER,
        "Started periodic timer");

    status = NT_SUCCESS(status) ? STATUS_PENDING : status;

pluginEnd:

    WdfSpinLockRelease(pFdoData->PendingPluginRequestsLock);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_BUSENUM, "%!FUNC! Exit with status %!STATUS!", status);

    return status;
}

//
// Simulates a device unplug event.
// 
NTSTATUS Bus_UnPlugDevice(
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

    status = WdfRequestRetrieveInputBuffer(Request, sizeof(VIGEM_UNPLUG_TARGET), (PVOID)&unPlug, &length);

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

    if (!IsInternal)
    {
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
NTSTATUS Bus_XusbSubmitReport(WDFDEVICE Device, ULONG SerialNo, PXUSB_SUBMIT_REPORT Report, BOOLEAN FromInterface)
{
    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_BUSENUM, "%!FUNC! Entry");

    return Bus_SubmitReport(Device, SerialNo, Report, FromInterface);
}

//
// Queues an inverted call to receive XUSB-specific updates.
// 
NTSTATUS Bus_QueueNotification(WDFDEVICE Device, ULONG SerialNo, WDFREQUEST Request)
{
    NTSTATUS                    status = STATUS_INVALID_PARAMETER;
    WDFDEVICE                   hChild;
    PPDO_DEVICE_DATA            pdoData;
    PXUSB_DEVICE_DATA           xusbData;
    PDS4_DEVICE_DATA            ds4Data;


    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_BUSENUM, "%!FUNC! Entry");

    hChild = Bus_GetPdo(Device, SerialNo);

    // Validate child
    if (hChild == NULL)
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_BUSENUM,
            "Bus_GetPdo: PDO with serial %d not found",
            SerialNo);
        return STATUS_NO_SUCH_DEVICE;
    }

    // Check common context
    pdoData = PdoGetData(hChild);
    if (pdoData == NULL)
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_BUSENUM,
            "PdoGetData failed");
        return STATUS_INVALID_PARAMETER;
    }

    // Check if caller owns this PDO
    if (!IS_OWNER(pdoData))
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_BUSENUM,
            "PDO & Request ownership mismatch: %d != %d",
            pdoData->OwnerProcessId,
            CURRENT_PROCESS_ID());
        return STATUS_ACCESS_DENIED;
    }

    // Queue the request for later completion by the PDO and return STATUS_PENDING
    switch (pdoData->TargetType)
    {
    case Xbox360Wired:

        xusbData = XusbGetData(hChild);

        if (xusbData == NULL)
        {
            TraceEvents(TRACE_LEVEL_ERROR,
                TRACE_BUSENUM,
                "XusbGetData failed");
            break;
        }

        status = WdfRequestForwardToIoQueue(Request, pdoData->PendingNotificationRequests);

        break;
    case DualShock4Wired:

        ds4Data = Ds4GetData(hChild);

        if (ds4Data == NULL)
        {
            TraceEvents(TRACE_LEVEL_ERROR,
                TRACE_BUSENUM,
                "Ds4GetData failed");
            break;
        }

        status = WdfRequestForwardToIoQueue(Request, pdoData->PendingNotificationRequests);

        break;
    default:
        status = STATUS_NOT_SUPPORTED;
        TraceEvents(TRACE_LEVEL_WARNING,
            TRACE_BUSENUM,
            "Unknown target type: %d (%!STATUS!)",
            pdoData->TargetType,
            status);
        break;
    }

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_BUSENUM,
            "WdfRequestForwardToIoQueue failed with status %!STATUS!",
            status);
    }

    status = (NT_SUCCESS(status)) ? STATUS_PENDING : status;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_BUSENUM, "%!FUNC! Exit with status %!STATUS!", status);

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

WDFDEVICE Bus_GetPdo(IN WDFDEVICE Device, IN ULONG SerialNo)
{
    WDFCHILDLIST                list;
    WDF_CHILD_RETRIEVE_INFO     info;

    list = WdfFdoGetDefaultChildList(Device);

    PDO_IDENTIFICATION_DESCRIPTION description;

    WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(&description.Header, sizeof(description));

    description.SerialNo = SerialNo;

    WDF_CHILD_RETRIEVE_INFO_INIT(&info, &description.Header);

    return WdfChildListRetrievePdo(list, &info);
}

NTSTATUS Bus_SubmitReport(WDFDEVICE Device, ULONG SerialNo, PVOID Report, BOOLEAN FromInterface)
{
    NTSTATUS                    status = STATUS_SUCCESS;
    WDFDEVICE                   hChild;
    PPDO_DEVICE_DATA            pdoData;
    WDFREQUEST                  usbRequest;
    PIRP                        pendingIrp;
    BOOLEAN                     changed;


    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_BUSENUM, "%!FUNC! Entry");

    hChild = Bus_GetPdo(Device, SerialNo);

    // Validate child
    if (hChild == NULL)
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_BUSENUM,
            "Bus_GetPdo: PDO with serial %d not found",
            SerialNo);
        return STATUS_NO_SUCH_DEVICE;
    }

    // Check common context
    pdoData = PdoGetData(hChild);
    if (pdoData == NULL)
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_BUSENUM,
            "PdoGetData failed");
        return STATUS_INVALID_PARAMETER;
    }

    // Check if caller owns this PDO
    if (!FromInterface && !IS_OWNER(pdoData))
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_BUSENUM,
            "PDO & Request ownership mismatch: %d != %d",
            pdoData->OwnerProcessId,
            CURRENT_PROCESS_ID());
        return STATUS_ACCESS_DENIED;
    }

    // Check if input is different from previous value
    switch (pdoData->TargetType)
    {
    case Xbox360Wired:

        changed = (RtlCompareMemory(&XusbGetData(hChild)->Packet.Report,
            &((PXUSB_SUBMIT_REPORT)Report)->Report,
            sizeof(XUSB_REPORT)) != sizeof(XUSB_REPORT));

        break;
    case DualShock4Wired:

        changed = TRUE;

        break;
    default:

        changed = FALSE;

        break;
    }

    // Don't waste pending IRP if input hasn't changed
    if (!changed)
    {
        TraceEvents(TRACE_LEVEL_VERBOSE,
            TRACE_BUSENUM,
            "Input report hasn't changed since last update, aborting with %!STATUS!",
            status);
        return status;
    }

    TraceEvents(TRACE_LEVEL_VERBOSE,
        TRACE_BUSENUM,
        "Received new report, processing");

    // Get pending USB request
    switch (pdoData->TargetType)
    {
    case Xbox360Wired:

        status = WdfIoQueueRetrieveNextRequest(pdoData->PendingUsbInRequests, &usbRequest);

        break;
    case DualShock4Wired:

        status = WdfIoQueueRetrieveNextRequest(pdoData->PendingUsbInRequests, &usbRequest);

        break;
    default:

        status = STATUS_NOT_SUPPORTED;

        TraceEvents(TRACE_LEVEL_WARNING,
            TRACE_BUSENUM,
            "Unknown target type: %d (%!STATUS!)",
            pdoData->TargetType,
            status);

        goto endSubmitReport;
    }

    if (status == STATUS_PENDING)
        goto endSubmitReport;
    else if (!NT_SUCCESS(status))
        goto endSubmitReport;

    TraceEvents(TRACE_LEVEL_VERBOSE,
        TRACE_BUSENUM,
        "Processing pending IRP");

    // Get pending IRP
    pendingIrp = WdfRequestWdmGetIrp(usbRequest);

    // Get USB request block
    PURB urb = (PURB)URB_FROM_IRP(pendingIrp);

    // Get transfer buffer
    PUCHAR Buffer = (PUCHAR)urb->UrbBulkOrInterruptTransfer.TransferBuffer;

    switch (pdoData->TargetType)
    {
    case Xbox360Wired:

        urb->UrbBulkOrInterruptTransfer.TransferBufferLength = sizeof(XUSB_INTERRUPT_IN_PACKET);

        // Copy submitted report to cache
        RtlCopyBytes(&XusbGetData(hChild)->Packet.Report, &((PXUSB_SUBMIT_REPORT)Report)->Report, sizeof(XUSB_REPORT));
        // Copy cached report to URB transfer buffer
        RtlCopyBytes(Buffer, &XusbGetData(hChild)->Packet, sizeof(XUSB_INTERRUPT_IN_PACKET));

        break;
    case DualShock4Wired:

        urb->UrbBulkOrInterruptTransfer.TransferBufferLength = DS4_REPORT_SIZE;

        /* Copy report to cache and transfer buffer
         * Skip first byte as it contains the never changing report id */
        RtlCopyBytes(Ds4GetData(hChild)->Report + 1, &((PDS4_SUBMIT_REPORT)Report)->Report, sizeof(DS4_REPORT));

        if (Buffer)
            RtlCopyBytes(Buffer, Ds4GetData(hChild)->Report, DS4_REPORT_SIZE);

        break;
    default:
        status = STATUS_INVALID_PARAMETER;
        break;
    }

    // Complete pending request
    WdfRequestComplete(usbRequest, status);

endSubmitReport:

    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_BUSENUM, "%!FUNC! Exit with status %!STATUS!", status);

    return status;
}

