/*
* Virtual Gamepad Emulation Framework - Windows kernel-mode bus driver
*
* MIT License
*
* Copyright (c) 2016-2020 Nefarius Software Solutions e.U. and Contributors
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
#include "driver.tmh"
#include <wdmguid.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, Bus_EvtDeviceAdd)
#pragma alloc_text (PAGE, Bus_DeviceFileCreate)
#pragma alloc_text (PAGE, Bus_FileClose)
#pragma alloc_text (PAGE, Bus_EvtDriverContextCleanup)
#endif


#include "EmulationTargetPDO.hpp"
#include "XusbPdo.hpp"
#include "Ds4Pdo.hpp"

using ViGEm::Bus::Core::PDO_IDENTIFICATION_DESCRIPTION;
using ViGEm::Bus::Core::EmulationTargetPDO;
using ViGEm::Bus::Targets::EmulationTargetXUSB;
using ViGEm::Bus::Targets::EmulationTargetDS4;


EXTERN_C_START

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
        "Loading Virtual Gamepad Emulation Bus Driver [built: %s %s]",
        __DATE__, __TIME__);

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
    WDF_CHILD_LIST_CONFIG       config;
    NTSTATUS                    status;
    WDFDEVICE                   device;
    WDF_IO_QUEUE_CONFIG         queueConfig;
    PNP_BUS_INFORMATION         busInfo;
    WDFQUEUE                    queue;
    WDF_FILEOBJECT_CONFIG       foConfig;
    WDF_OBJECT_ATTRIBUTES       fdoAttributes;
    WDF_OBJECT_ATTRIBUTES       fileHandleAttributes;
    PFDO_DEVICE_DATA            pFDOData;

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_BUS_EXTENDER);
    // More than one process may talk to the bus at the same time
    WdfDeviceInitSetExclusive(DeviceInit, FALSE);
    // Bus is power policy owner over all PDOs
    WdfDeviceInitSetPowerPolicyOwnership(DeviceInit, TRUE);

#pragma region Prepare child list

    WDF_CHILD_LIST_CONFIG_INIT(&config, sizeof(PDO_IDENTIFICATION_DESCRIPTION), Bus_EvtDeviceListCreatePdo);

    config.EvtChildListIdentificationDescriptionCompare = Bus_EvtChildListIdentificationDescriptionCompare;

    WdfFdoInitSetDefaultChildListConfig(DeviceInit, &config, WDF_NO_OBJECT_ATTRIBUTES);

#pragma endregion

#pragma region Assign File Object Configuration

    WDF_FILEOBJECT_CONFIG_INIT(&foConfig, Bus_DeviceFileCreate, Bus_FileClose, NULL);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&fileHandleAttributes, FDO_FILE_DATA);

    WdfDeviceInitSetFileObjectConfig(DeviceInit, &foConfig, &fileHandleAttributes);

#pragma endregion

#pragma region Create FDO

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&fdoAttributes, FDO_DEVICE_DATA);

    status = WdfDeviceCreate(&DeviceInit, &fdoAttributes, &device);

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_DRIVER,
            "WdfDeviceCreate failed with status %!STATUS!",
            status);
        return status;
    }

    pFDOData = FdoGetData(device);
    if (pFDOData == NULL)
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_DRIVER,
            "FdoGetData failed");
        return STATUS_UNSUCCESSFUL;
    }

    pFDOData->InterfaceReferenceCounter = 0;
    pFDOData->NextSessionId = FDO_FIRST_SESSION_ID;

#pragma endregion

#pragma region Create default I/O queue for FDO

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchParallel);

    queueConfig.EvtIoDeviceControl = Bus_EvtIoDeviceControl;
    queueConfig.EvtIoDefault = Bus_EvtIoDefault;

    __analysis_assume(queueConfig.EvtIoStop != 0);
    status = WdfIoQueueCreate(device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &queue);
    __analysis_assume(queueConfig.EvtIoStop == 0);

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_DRIVER,
            "WdfIoQueueCreate failed with status %!STATUS!",
            status);
        return status;
    }

#pragma endregion

#pragma region Expose FDO interface

    status = WdfDeviceCreateDeviceInterface(device, &GUID_DEVINTERFACE_BUSENUM_VIGEM, NULL);

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_DRIVER,
            "WdfDeviceCreateDeviceInterface failed with status %!STATUS!",
            status);
        return status;
    }

#pragma endregion

#pragma region Set bus information

    busInfo.BusTypeGuid = GUID_BUS_TYPE_USB;
    busInfo.LegacyBusType = PNPBus;
    busInfo.BusNumber = 0;

    WdfDeviceSetBusInformationForChildren(device, &busInfo);

#pragma endregion

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit with status %!STATUS!", status);

    return status;
}

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
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_DRIVER,
            "FileObjectGetData failed to return file object from WDFFILEOBJECT 0x%p",
            FileObject);
    }
    else
    {
        pFDOData = FdoGetData(Device);
        if (pFDOData == NULL)
        {
            TraceEvents(TRACE_LEVEL_ERROR,
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
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_DRIVER,
            "FileObjectGetData failed to return file object from WDFFILEOBJECT 0x%p",
            FileObject);
        return;
    }

    device = WdfFileObjectGetDevice(FileObject);

    pFDOData = FdoGetData(device);
    if (pFDOData == NULL)
    {
        TraceEvents(TRACE_LEVEL_ERROR,
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

        //TraceEvents(TRACE_LEVEL_VERBOSE,
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
                TraceEvents(TRACE_LEVEL_ERROR,
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

EXTERN_C_END
