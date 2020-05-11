#include "Ds4Pdo.hpp"
#include "trace.h"
#include "Ds4Pdo.tmh"
#define NTSTRSAFE_LIB
#include <ntstrsafe.h>
#include <hidclass.h>



PCWSTR ViGEm::Bus::Targets::EmulationTargetDS4::_deviceDescription = L"Virtual DualShock 4 Controller";

ViGEm::Bus::Targets::EmulationTargetDS4::EmulationTargetDS4() : EmulationTargetPDO(0x054C, 0x05C4)
{
    TargetType = DualShock4Wired;
    UsbConfigurationDescriptionSize = DS4_DESCRIPTOR_SIZE;
}

NTSTATUS ViGEm::Bus::Targets::EmulationTargetDS4::PrepareDevice(PWDFDEVICE_INIT DeviceInit,
                                                                PUNICODE_STRING DeviceId, PUNICODE_STRING DeviceDescription)
{
    NTSTATUS status;
    UNICODE_STRING buffer;
    	
    // prepare device description
    status = RtlUnicodeStringInit(DeviceDescription, _deviceDescription);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_DS4,
            "RtlUnicodeStringInit failed with status %!STATUS!",
            status);
        return status;
    }

    // Set hardware IDs
    RtlUnicodeStringInit(&buffer, L"USB\\VID_054C&PID_05C4&REV_0100");

    status = WdfPdoInitAddHardwareID(DeviceInit, &buffer);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_DS4,
            "WdfPdoInitAddHardwareID failed with status %!STATUS!",
            status);
        return status;
    }

    RtlUnicodeStringCopy(DeviceId, &buffer);

    RtlUnicodeStringInit(&buffer, L"USB\\VID_054C&PID_05C4");

    status = WdfPdoInitAddHardwareID(DeviceInit, &buffer);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_DS4,
            "WdfPdoInitAddHardwareID failed with status %!STATUS!",
            status);
        return status;
    }

    // Set compatible IDs
    RtlUnicodeStringInit(&buffer, L"USB\\Class_03&SubClass_00&Prot_00");

    status = WdfPdoInitAddCompatibleID(DeviceInit, &buffer);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_DS4,
            "WdfPdoInitAddCompatibleID (#01) failed with status %!STATUS!",
            status);
        return status;
    }

    RtlUnicodeStringInit(&buffer, L"USB\\Class_03&SubClass_00");

    status = WdfPdoInitAddCompatibleID(DeviceInit, &buffer);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_DS4,
            "WdfPdoInitAddCompatibleID (#02) failed with status %!STATUS!",
            status);
        return status;
    }

    RtlUnicodeStringInit(&buffer, L"USB\\Class_03");

    status = WdfPdoInitAddCompatibleID(DeviceInit, &buffer);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_DS4,
            "WdfPdoInitAddCompatibleID (#03) failed with status %!STATUS!",
            status);
        return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS ViGEm::Bus::Targets::EmulationTargetDS4::PrepareHardware()
{
	WDF_QUERY_INTERFACE_CONFIG ifaceCfg;
    INTERFACE devinterfaceHid;

    devinterfaceHid.Size = sizeof(INTERFACE);
    devinterfaceHid.Version = 1;
    devinterfaceHid.Context = static_cast<PVOID>(this->PdoDevice);

    devinterfaceHid.InterfaceReference = WdfDeviceInterfaceReferenceNoOp;
    devinterfaceHid.InterfaceDereference = WdfDeviceInterfaceDereferenceNoOp;

    // Expose GUID_DEVINTERFACE_HID so HIDUSB can initialize
    WDF_QUERY_INTERFACE_CONFIG_INIT(
        &ifaceCfg, 
        (PINTERFACE)&devinterfaceHid,
        &GUID_DEVINTERFACE_HID,
        NULL
    );

    NTSTATUS status = WdfDeviceAddQueryInterface(this->PdoDevice, &ifaceCfg);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_DS4,
            "WdfDeviceAddQueryInterface failed with status %!STATUS!",
            status);
        return status;
    }

    // Set default HID input report (everything zero`d)
    UCHAR DefaultHidReport[DS4_REPORT_SIZE] =
    {
        0x01, 0x82, 0x7F, 0x7E, 0x80, 0x08, 0x00, 0x58,
        0x00, 0x00, 0xFD, 0x63, 0x06, 0x03, 0x00, 0xFE,
        0xFF, 0xFC, 0xFF, 0x79, 0xFD, 0x1B, 0x14, 0xD1,
        0xE9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1B, 0x00,
        0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80,
        0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00,
        0x80, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
        0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00
    };

    // Initialize HID reports to defaults
    RtlCopyBytes(this->Report, DefaultHidReport, DS4_REPORT_SIZE);
    RtlZeroMemory(&this->OutputReport, sizeof(DS4_OUTPUT_REPORT));

    // Start pending IRP queue flush timer
    WdfTimerStart(this->PendingUsbInRequestsTimer, DS4_QUEUE_FLUSH_PERIOD);

    return STATUS_SUCCESS;
}

NTSTATUS ViGEm::Bus::Targets::EmulationTargetDS4::InitContext()
{
    NTSTATUS            status;

    // Initialize periodic timer
    WDF_TIMER_CONFIG timerConfig;
    WDF_TIMER_CONFIG_INIT_PERIODIC(
        &timerConfig, 
        PendingUsbRequestsTimerFunc, 
        DS4_QUEUE_FLUSH_PERIOD
    );

    // Timer object attributes
    WDF_OBJECT_ATTRIBUTES timerAttribs;
    WDF_OBJECT_ATTRIBUTES_INIT(&timerAttribs);

    // PDO is parent
    timerAttribs.ParentObject = this->PdoDevice;

    // Create timer
    status = WdfTimerCreate(
        &timerConfig, 
        &timerAttribs, 
        &this->PendingUsbInRequestsTimer
    );
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_DS4,
            "WdfTimerCreate failed with status %!STATUS!",
            status);
        return status;
    }

    // Load/generate MAC address

    // TODO: tidy up this region

    WDFKEY keyParams, keyTargets, keyDS, keySerial;
    UNICODE_STRING keyName, valueName;

    status = WdfDriverOpenParametersRegistryKey(
        WdfGetDriver(), 
        STANDARD_RIGHTS_ALL, 
        WDF_NO_OBJECT_ATTRIBUTES, 
        &keyParams
    );
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_DS4,
            "WdfDriverOpenParametersRegistryKey failed with status %!STATUS!",
            status);
        return status;
    }

    RtlUnicodeStringInit(&keyName, L"Targets");

    status = WdfRegistryCreateKey(
        keyParams,
        &keyName,
        KEY_ALL_ACCESS,
        REG_OPTION_NON_VOLATILE,
        NULL,
        WDF_NO_OBJECT_ATTRIBUTES,
        &keyTargets
    );
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_DS4,
            "WdfRegistryCreateKey failed with status %!STATUS!",
            status);
        return status;
    }

    RtlUnicodeStringInit(&keyName, L"DualShock");

    status = WdfRegistryCreateKey(
        keyTargets,
        &keyName,
        KEY_ALL_ACCESS,
        REG_OPTION_NON_VOLATILE,
        NULL,
        WDF_NO_OBJECT_ATTRIBUTES,
        &keyDS
    );
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_DS4,
            "WdfRegistryCreateKey failed with status %!STATUS!",
            status);
        return status;
    }

    DECLARE_UNICODE_STRING_SIZE(serialPath, 4);
    RtlUnicodeStringPrintf(&serialPath, L"%04d", this->SerialNo);

    status = WdfRegistryCreateKey(
        keyDS,
        &serialPath,
        KEY_ALL_ACCESS,
        REG_OPTION_NON_VOLATILE,
        NULL,
        WDF_NO_OBJECT_ATTRIBUTES,
        &keySerial
    );
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_DS4,
            "WdfRegistryCreateKey failed with status %!STATUS!",
            status);
        return status;
    }

    RtlUnicodeStringInit(&valueName, L"TargetMacAddress");

    status = WdfRegistryQueryValue(
        keySerial, 
        &valueName,
        sizeof(MAC_ADDRESS), 
        &this->TargetMacAddress, 
        NULL, 
        NULL
    );

    TraceEvents(TRACE_LEVEL_INFORMATION,
        TRACE_DS4,
        "MAC-Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
        this->TargetMacAddress.Vendor0,
        this->TargetMacAddress.Vendor1,
        this->TargetMacAddress.Vendor2,
        this->TargetMacAddress.Nic0,
        this->TargetMacAddress.Nic1,
        this->TargetMacAddress.Nic2);

    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        GenerateRandomMacAddress(&this->TargetMacAddress);

        status = WdfRegistryAssignValue(
            keySerial, 
            &valueName, 
            REG_BINARY, 
            sizeof(MAC_ADDRESS), 
            static_cast<PVOID>(&this->TargetMacAddress)
        );
        if (!NT_SUCCESS(status))
        {
            TraceEvents(TRACE_LEVEL_ERROR,
                TRACE_DS4,
                "WdfRegistryAssignValue failed with status %!STATUS!",
                status);
            return status;
        }
    }
    else if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_DS4,
            "WdfRegistryQueryValue failed with status %!STATUS!",
            status);
        return status;
    }

    WdfRegistryClose(keySerial);
    WdfRegistryClose(keyDS);
    WdfRegistryClose(keyTargets);
    WdfRegistryClose(keyParams);

    return STATUS_SUCCESS;
}

VOID ViGEm::Bus::Targets::EmulationTargetDS4::GetConfigurationDescriptorType(PUCHAR Buffer, ULONG Length)
{
    UCHAR Ds4DescriptorData[DS4_DESCRIPTOR_SIZE] =
    {
        0x09,        // bLength
        0x02,        // bDescriptorType (Configuration)
        0x29, 0x00,  // wTotalLength 41
        0x01,        // bNumInterfaces 1
        0x01,        // bConfigurationValue
        0x00,        // iConfiguration (String Index)
        0xC0,        // bmAttributes Self Powered
        0xFA,        // bMaxPower 500mA

        0x09,        // bLength
        0x04,        // bDescriptorType (Interface)
        0x00,        // bInterfaceNumber 0
        0x00,        // bAlternateSetting
        0x02,        // bNumEndpoints 2
        0x03,        // bInterfaceClass
        0x00,        // bInterfaceSubClass
        0x00,        // bInterfaceProtocol
        0x00,        // iInterface (String Index)

        0x09,        // bLength
        0x21,        // bDescriptorType (HID)
        0x11, 0x01,  // bcdHID 1.11
        0x00,        // bCountryCode
        0x01,        // bNumDescriptors
        0x22,        // bDescriptorType[0] (HID)
        0xD3, 0x01,  // wDescriptorLength[0] 467

        0x07,        // bLength
        0x05,        // bDescriptorType (Endpoint)
        0x84,        // bEndpointAddress (IN/D2H)
        0x03,        // bmAttributes (Interrupt)
        0x40, 0x00,  // wMaxPacketSize 64
        0x05,        // bInterval 5 (unit depends on device speed)

        0x07,        // bLength
        0x05,        // bDescriptorType (Endpoint)
        0x03,        // bEndpointAddress (OUT/H2D)
        0x03,        // bmAttributes (Interrupt)
        0x40, 0x00,  // wMaxPacketSize 64
        0x05,        // bInterval 5 (unit depends on device speed)

                     // 41 bytes

                     // best guess: USB Standard Descriptor
    };

    RtlCopyBytes(Buffer, Ds4DescriptorData, Length);
}

VOID ViGEm::Bus::Targets::EmulationTargetDS4::GetDeviceDescriptorType(PUSB_DEVICE_DESCRIPTOR pDescriptor)
{
    pDescriptor->bLength = 0x12;
    pDescriptor->bDescriptorType = USB_DEVICE_DESCRIPTOR_TYPE;
    pDescriptor->bcdUSB = 0x0200; // USB v2.0
    pDescriptor->bDeviceClass = 0x00; // per Interface
    pDescriptor->bDeviceSubClass = 0x00;
    pDescriptor->bDeviceProtocol = 0x00;
    pDescriptor->bMaxPacketSize0 = 0x40;
    pDescriptor->idVendor = this->VendorId;
    pDescriptor->idProduct = this->ProductId;
    pDescriptor->bcdDevice = 0x0100;
    pDescriptor->iManufacturer = 0x01;
    pDescriptor->iProduct = 0x02;
    pDescriptor->iSerialNumber = 0x00;
    pDescriptor->bNumConfigurations = 0x01;
}

NTSTATUS ViGEm::Bus::Targets::EmulationTargetDS4::SelectConfiguration(PURB Urb)
{
    if (Urb->UrbHeader.Length < DS4_CONFIGURATION_SIZE)
    {
        TraceEvents(TRACE_LEVEL_WARNING,
            TRACE_USBPDO,
            ">> >> >> URB_FUNCTION_SELECT_CONFIGURATION: Invalid ConfigurationDescriptor");
        return STATUS_INVALID_PARAMETER;
    }
	
    PUSBD_INTERFACE_INFORMATION pInfo = &Urb->UrbSelectConfiguration.Interface;
	
    TraceEvents(TRACE_LEVEL_VERBOSE,
        TRACE_DS4,
        ">> >> >> URB_FUNCTION_SELECT_CONFIGURATION: Length %d, Interface %d, Alternate %d, Pipes %d",
        (int)pInfo->Length,
        (int)pInfo->InterfaceNumber,
        (int)pInfo->AlternateSetting,
        pInfo->NumberOfPipes);

    pInfo->Class = 0x03; // HID
    pInfo->SubClass = 0x00;
    pInfo->Protocol = 0x00;

    pInfo->InterfaceHandle = (USBD_INTERFACE_HANDLE)0xFFFF0000;

    pInfo->Pipes[0].MaximumTransferSize = 0x00400000;
    pInfo->Pipes[0].MaximumPacketSize = 0x40;
    pInfo->Pipes[0].EndpointAddress = 0x84;
    pInfo->Pipes[0].Interval = 0x05;
    pInfo->Pipes[0].PipeType = (USBD_PIPE_TYPE)0x03;
    pInfo->Pipes[0].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0084;
    pInfo->Pipes[0].PipeFlags = 0x00;

    pInfo->Pipes[1].MaximumTransferSize = 0x00400000;
    pInfo->Pipes[1].MaximumPacketSize = 0x40;
    pInfo->Pipes[1].EndpointAddress = 0x03;
    pInfo->Pipes[1].Interval = 0x05;
    pInfo->Pipes[1].PipeType = (USBD_PIPE_TYPE)0x03;
    pInfo->Pipes[1].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0003;
    pInfo->Pipes[1].PipeFlags = 0x00;

    return STATUS_SUCCESS;
}

void ViGEm::Bus::Targets::EmulationTargetDS4::AbortPipe()
{
    // Higher driver shutting down, emptying PDOs queues
    WdfTimerStop(this->PendingUsbInRequestsTimer, TRUE);
}

NTSTATUS ViGEm::Bus::Targets::EmulationTargetDS4::UsbClassInterface(PURB Urb)
{
    struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST* pRequest = &Urb->UrbControlVendorClassRequest;

    TraceEvents(TRACE_LEVEL_VERBOSE,
        TRACE_USBPDO,
        ">> >> >> URB_FUNCTION_CLASS_INTERFACE");
    TraceEvents(TRACE_LEVEL_VERBOSE,
        TRACE_USBPDO,
        ">> >> >> TransferFlags = 0x%X, Request = 0x%X, Value = 0x%X, Index = 0x%X, BufLen = %d",
        pRequest->TransferFlags,
        pRequest->Request,
        pRequest->Value,
        pRequest->Index,
        pRequest->TransferBufferLength);

	switch (pRequest->Request)
	{
	case HID_REQUEST_GET_REPORT:
	{
	    UCHAR reportId = hid_get_report_id(pRequest);
	    UCHAR reportType = hid_get_report_type(pRequest);

	    TraceEvents(TRACE_LEVEL_VERBOSE,
	        TRACE_USBPDO,
	        ">> >> >> >> GET_REPORT(%d): %d",
	        reportType, reportId);

	    switch (reportType)
	    {
	    case HID_REPORT_TYPE_FEATURE:
	    {
	        switch (reportId)
	        {
	        case HID_REPORT_ID_0:
	        {
	            // Source: http://eleccelerator.com/wiki/index.php?title=DualShock_4#Class_Requests
	            UCHAR Response[] =
	            {
	                0xA3, 0x41, 0x75, 0x67, 0x20, 0x20, 0x33, 0x20,
	                0x32, 0x30, 0x31, 0x33, 0x00, 0x00, 0x00, 0x00,
	                0x00, 0x30, 0x37, 0x3A, 0x30, 0x31, 0x3A, 0x31,
	                0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	                0x00, 0x00, 0x01, 0x00, 0x31, 0x03, 0x00, 0x00,
	                0x00, 0x49, 0x00, 0x05, 0x00, 0x00, 0x80, 0x03,
	                0x00
	            };

	            pRequest->TransferBufferLength = ARRAYSIZE(Response);
	            RtlCopyBytes(pRequest->TransferBuffer, Response, ARRAYSIZE(Response));

	            break;
	        }
	        case HID_REPORT_ID_1:
	        {
	            // Source: http://eleccelerator.com/wiki/index.php?title=DualShock_4#Class_Requests
	            UCHAR Response[] =
	            {
	                0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87,
	                0x22, 0x7B, 0xDD, 0xB2, 0x22, 0x47, 0xDD, 0xBD,
	                0x22, 0x43, 0xDD, 0x1C, 0x02, 0x1C, 0x02, 0x7F,
	                0x1E, 0x2E, 0xDF, 0x60, 0x1F, 0x4C, 0xE0, 0x3A,
	                0x1D, 0xC6, 0xDE, 0x08, 0x00
	            };

	            pRequest->TransferBufferLength = ARRAYSIZE(Response);
	            RtlCopyBytes(pRequest->TransferBuffer, Response, ARRAYSIZE(Response));

	            break;
	        }
	        case HID_REPORT_MAC_ADDRESSES_ID:
	        {
	            // Source: http://eleccelerator.com/wiki/index.php?title=DualShock_4#Class_Requests
	            UCHAR Response[] =
	            {
	                0x12, 0x8B, 0x09, 0x07, 0x6D, 0x66, 0x1C, 0x08,
	                0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	            };

	            // Insert (auto-generated) target MAC address into response
	            RtlCopyBytes(Response + 1, &this->TargetMacAddress, sizeof(MAC_ADDRESS));
	            // Adjust byte order
	            ReverseByteArray(Response + 1, sizeof(MAC_ADDRESS));

	            // Insert (auto-generated) host MAC address into response
	            RtlCopyBytes(Response + 10, &this->HostMacAddress, sizeof(MAC_ADDRESS));
	            // Adjust byte order
	            ReverseByteArray(Response + 10, sizeof(MAC_ADDRESS));

	            pRequest->TransferBufferLength = ARRAYSIZE(Response);
	            RtlCopyBytes(pRequest->TransferBuffer, Response, ARRAYSIZE(Response));

	            break;
	        }
	        default:
	            break;
	        }
	        break;
	    }
	    default:
	        break;
	    }

	    break;
	}
	case HID_REQUEST_SET_REPORT:
	{
	    UCHAR reportId = hid_get_report_id(pRequest);
	    UCHAR reportType = hid_get_report_type(pRequest);

	    TraceEvents(TRACE_LEVEL_VERBOSE,
	        TRACE_USBPDO,
	        ">> >> >> >> SET_REPORT(%d): %d",
	        reportType, reportId);

	    switch (reportType)
	    {
	    case HID_REPORT_TYPE_FEATURE:
	    {
	        switch (reportId)
	        {
	        case HID_REPORT_ID_3:
	        {
	            // Source: http://eleccelerator.com/wiki/index.php?title=DualShock_4#Class_Requests
	            UCHAR Response[] =
	            {
	                0x13, 0xAC, 0x9E, 0x17, 0x94, 0x05, 0xB0, 0x56,
	                0xE8, 0x81, 0x38, 0x08, 0x06, 0x51, 0x41, 0xC0,
	                0x7F, 0x12, 0xAA, 0xD9, 0x66, 0x3C, 0xCE
	            };

	            pRequest->TransferBufferLength = ARRAYSIZE(Response);
	            RtlCopyBytes(pRequest->TransferBuffer, Response, ARRAYSIZE(Response));

	            break;
	        }
	        case HID_REPORT_ID_4:
	        {
	            // Source: http://eleccelerator.com/wiki/index.php?title=DualShock_4#Class_Requests
	            UCHAR Response[] =
	            {
	                0x14, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	                0x00
	            };

	            pRequest->TransferBufferLength = ARRAYSIZE(Response);
	            RtlCopyBytes(pRequest->TransferBuffer, Response, ARRAYSIZE(Response));

	            break;
	        }
	        default:
	            break;
	        }
	        break;
	    }
	    default:
	        break;
	    }

	    break;
	}
	default:
	    break;
	}

    return STATUS_SUCCESS;
}

VOID ViGEm::Bus::Targets::EmulationTargetDS4::PendingUsbRequestsTimerFunc(
    _In_ WDFTIMER Timer
)
{
	auto ctx = reinterpret_cast<EmulationTargetDS4*>(Core::EmulationTargetPdoGetContext(WdfTimerGetParentObject(Timer)));

	WDFREQUEST              usbRequest;
    PIRP                    pendingIrp;
    PIO_STACK_LOCATION      irpStack;

    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_DS4, "%!FUNC! Entry");

    // Get pending USB request
    NTSTATUS status = WdfIoQueueRetrieveNextRequest(ctx->PendingUsbInRequests, &usbRequest);

    if (NT_SUCCESS(status))
    {
        // Get pending IRP
        pendingIrp = WdfRequestWdmGetIrp(usbRequest);
        irpStack = IoGetCurrentIrpStackLocation(pendingIrp);

        // Get USB request block
        PURB urb = (PURB)irpStack->Parameters.Others.Argument1;

        // Get transfer buffer
        PUCHAR Buffer = (PUCHAR)urb->UrbBulkOrInterruptTransfer.TransferBuffer;
        // Set buffer length to report size
        urb->UrbBulkOrInterruptTransfer.TransferBufferLength = DS4_REPORT_SIZE;

        // Copy cached report to transfer buffer 
        if (Buffer)
            RtlCopyBytes(Buffer,  ctx->Report, DS4_REPORT_SIZE);

        // Complete pending request
        WdfRequestComplete(usbRequest, status);
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DS4, "%!FUNC! Exit with status %!STATUS!", status);
}
