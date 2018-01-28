/*
MIT License

Copyright (c) 2016 Benjamin "Nefarius" Höglinger

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#include "busenum.h"
#include <wdmguid.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, Bus_EvtDeviceAdd)
#pragma alloc_text (PAGE, Bus_DeviceFileCreate)
#pragma alloc_text (PAGE, Bus_FileClose)
#pragma alloc_text (PAGE, Bus_PdoStageResult)
#endif


//
// Driver entry routine.
// 
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;
    WDFDRIVER driver;

    KdPrint((DRIVERNAME "Virtual Gamepad Emulation Bus Driver [built: %s %s]\n", __DATE__, __TIME__));

    WDF_DRIVER_CONFIG_INIT(&config, Bus_EvtDeviceAdd);

    status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, &driver);

    if (!NT_SUCCESS(status))
    {
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
    WDF_OBJECT_ATTRIBUTES       collectionAttributes;
    PFDO_DEVICE_DATA            pFDOData;
    VIGEM_BUS_INTERFACE         busInterface;
    PINTERFACE                  interfaceHeader;

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    KdPrint((DRIVERNAME "Bus_EvtDeviceAdd: 0x%p\n", Driver));

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
        KdPrint((DRIVERNAME "Error creating device 0x%x\n", status));
        return status;
    }

    KdPrint((DRIVERNAME "Created FDO: 0x%X\n", device));

    pFDOData = FdoGetData(device);
    if (pFDOData == NULL)
    {
        KdPrint((DRIVERNAME "Error creating device context\n"));
        return STATUS_UNSUCCESSFUL;
    }

    pFDOData->InterfaceReferenceCounter = 0;
    pFDOData->NextSessionId = FDO_FIRST_SESSION_ID;

#pragma endregion

#pragma region Create pending requests collection & lock

    WDF_OBJECT_ATTRIBUTES_INIT(&collectionAttributes);
    collectionAttributes.ParentObject = device;

    status = WdfCollectionCreate(&collectionAttributes, &pFDOData->PendingPluginRequests);
    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVERNAME "WdfCollectionCreate failed with status 0x%X\n", status));
        return STATUS_UNSUCCESSFUL;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&collectionAttributes);
    collectionAttributes.ParentObject = device;

    status = WdfSpinLockCreate(&collectionAttributes, &pFDOData->PendingPluginRequestsLock);
    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVERNAME "WdfSpinLockCreate failed with status 0x%X\n", status));
        return STATUS_UNSUCCESSFUL;
    }

#pragma endregion

#pragma region Add query interface

    // 
    // Set up the common interface header
    // 
    interfaceHeader = &busInterface.InterfaceHeader;

    interfaceHeader->Size = sizeof(VIGEM_BUS_INTERFACE);
    interfaceHeader->Version = VIGEM_BUS_INTERFACE_VERSION;
    interfaceHeader->Context = (PVOID)device;

    // 
    // We don't pay any particular attention to the reference
    // counting of this interface, but we MUST specify routines for
    // it. Luckily the framework provides dummy routines
    // 
    interfaceHeader->InterfaceReference = WdfDeviceInterfaceReferenceNoOp;
    interfaceHeader->InterfaceDereference = WdfDeviceInterfaceDereferenceNoOp;

    busInterface.BusPdoStageResult = Bus_PdoStageResult;

    WDF_QUERY_INTERFACE_CONFIG queryInterfaceConfig;

    WDF_QUERY_INTERFACE_CONFIG_INIT(&queryInterfaceConfig,
        interfaceHeader,
        &GUID_VIGEM_INTERFACE_PDO,
        WDF_NO_EVENT_CALLBACK);

    status = WdfDeviceAddQueryInterface(device,
        &queryInterfaceConfig);

    if (!NT_SUCCESS(status)) {
        KdPrint((DRIVERNAME "WdfDeviceAddQueryInterface failed 0x%0x\n", status));
        return(status);
    }

#pragma endregion

#pragma region Create default I/O queue for FDO

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchParallel);

    queueConfig.EvtIoDeviceControl = Bus_EvtIoDeviceControl;
    queueConfig.EvtIoInternalDeviceControl = Bus_EvtIoInternalDeviceControl;
    queueConfig.EvtIoDefault = Bus_EvtIoDefault;

    __analysis_assume(queueConfig.EvtIoStop != 0);
    status = WdfIoQueueCreate(device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &queue);
    __analysis_assume(queueConfig.EvtIoStop == 0);

    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVERNAME "WdfIoQueueCreate failed status 0x%x\n", status));
        return status;
    }

#pragma endregion

#pragma region Expose FDO interface

    status = WdfDeviceCreateDeviceInterface(device, &GUID_DEVINTERFACE_BUSENUM_VIGEM, NULL);

    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVERNAME "WdfDeviceCreateDeviceInterface failed status 0x%x\n", status));
        return status;
    }

#pragma endregion

#pragma region Set bus information

    busInfo.BusTypeGuid = GUID_BUS_TYPE_USB;
    busInfo.LegacyBusType = PNPBus;
    busInfo.BusNumber = 0;

    WdfDeviceSetBusInformationForChildren(device, &busInfo);

#pragma endregion

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

    pFileData = FileObjectGetData(FileObject);
    if (pFileData == NULL)
    {
        KdPrint((DRIVERNAME "Bus_DeviceFileCreate: ERROR! File handle context not found\n"));
    }
    else
    {
        pFDOData = FdoGetData(Device);
        if (pFDOData == NULL)
        {
            KdPrint((DRIVERNAME "Bus_DeviceFileCreate: ERROR! FDO context not found\n"));
            status = STATUS_NO_SUCH_DEVICE;
        }
        else
        {
            refCount = InterlockedIncrement(&pFDOData->InterfaceReferenceCounter);
            sessionId = InterlockedIncrement(&pFDOData->NextSessionId);

            pFileData->SessionId = sessionId;
            status = STATUS_SUCCESS;

            KdPrint((DRIVERNAME "Bus_DeviceFileCreate: File id=%d. Device refcount=%d\n", (int)sessionId, (int)refCount));
        }
    }

    WdfRequestComplete(Request, status);
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

    KdPrint((DRIVERNAME "Bus_FileClose called\n"));

    // Check common context
    pFileData = FileObjectGetData(FileObject);
    if (pFileData == NULL)
    {
        KdPrint((DRIVERNAME "Bus_FileClose: ERROR! File handle context not found\n"));
        return;
    }

    device = WdfFileObjectGetDevice(FileObject);

    pFDOData = FdoGetData(device);
    if (pFDOData == NULL)
    {
        KdPrint((DRIVERNAME "Bus_FileClose: ERROR! FDO context not found\n"));
        status = STATUS_NO_SUCH_DEVICE;
    }
    else
    {
        refCount = InterlockedDecrement(&pFDOData->InterfaceReferenceCounter);

        KdPrint((DRIVERNAME "Bus_FileClose: Device refcount=%d\n", (int)refCount));
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

        KdPrint((DRIVERNAME "Bus_FileClose enumerate: status=%d devicePID=%d currentPID=%d fileSessionId=%d deviceSessionId=%d ownerIsDriver=%d\n",
            (int)childInfo.Status,
            (int)description.OwnerProcessId,
            (int)CURRENT_PROCESS_ID(),
            (int)pFileData->SessionId,
            (int)description.SessionId,
            (int)description.OwnerIsDriver));

        // Only unplug devices with matching session id
        if (childInfo.Status == WdfChildListRetrieveDeviceSuccess
            && description.SessionId == pFileData->SessionId
            && !description.OwnerIsDriver)
        {
            KdPrint((DRIVERNAME "Bus_FileClose unplugging\n"));

            // "Unplug" child
            status = WdfChildListUpdateChildDescriptionAsMissing(list, &description.Header);
            if (!NT_SUCCESS(status))
            {
                KdPrint((DRIVERNAME "WdfChildListUpdateChildDescriptionAsMissing failed with status 0x%X\n", status));
            }
        }
    }

    WdfChildListEndIteration(list, &iterator);
}

//
// Called by PDO when a boot-up stage has been completed
// 
_Use_decl_annotations_
VOID
Bus_PdoStageResult(
    _In_ PINTERFACE InterfaceHeader,
    _In_ VIGEM_PDO_STAGE Stage,
    _In_ ULONG Serial,
    _In_ NTSTATUS Status
)
{
    ULONG i;
    PFDO_DEVICE_DATA pFdoData;
    WDFREQUEST curRequest;
    ULONG curSerial;
    ULONG items;

    UNREFERENCED_PARAMETER(InterfaceHeader);

    PAGED_CODE();

    KdPrint((DRIVERNAME "Bus_PdoStageResult: Stage: %d, Serial: %d, status: 0x%X\n", Stage, Serial, Status));

    pFdoData = FdoGetData(InterfaceHeader->Context);

    //
    // If any stage fails or is last stage, get associated request and complete it
    // 
    if (!NT_SUCCESS(Status) || Stage == ViGEmPdoInternalIoControl)
    {
        WdfSpinLockAcquire(pFdoData->PendingPluginRequestsLock);

        items = WdfCollectionGetCount(pFdoData->PendingPluginRequests);

        KdPrint((DRIVERNAME "Items count: %d\n", items));

        for (i = 0; i < items; i++)
        {
            curRequest = WdfCollectionGetItem(pFdoData->PendingPluginRequests, i);
            curSerial = PluginRequestGetData(curRequest)->Serial;

            KdPrint((DRIVERNAME "Serial: %d, curSerial: %d\n", Serial, curSerial));

            if (Serial == curSerial)
            {
                WdfRequestComplete(curRequest, Status);

                WdfCollectionRemove(pFdoData->PendingPluginRequests, curRequest);

                KdPrint((DRIVERNAME "Removed item with serial: %d\n", curSerial));

                break;
            }
        }
        WdfSpinLockRelease(pFdoData->PendingPluginRequestsLock);
    }
}
