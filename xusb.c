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

NTSTATUS Xusb_PreparePdo(
    PWDFDEVICE_INIT DeviceInit,
    USHORT VendorId,
    USHORT ProductId,
    PUNICODE_STRING DeviceId,
    PUNICODE_STRING DeviceDescription)
{
    NTSTATUS status;
    DECLARE_UNICODE_STRING_SIZE(buffer, MAX_HARDWARE_ID_LENGTH);

    // prepare device description
    status = RtlUnicodeStringInit(DeviceDescription, L"Virtual Xbox 360 Controller");
    if (!NT_SUCCESS(status))
        return status;

    // Set hardware ID
    RtlUnicodeStringPrintf(&buffer, L"USB\\VID_%04X&PID_%04X", VendorId, ProductId);

    RtlUnicodeStringCopy(DeviceId, &buffer);

    status = WdfPdoInitAddHardwareID(DeviceInit, &buffer);
    if (!NT_SUCCESS(status))
        return status;

    // Set compatible IDs
    RtlUnicodeStringInit(&buffer, L"USB\\MS_COMP_XUSB10");

    status = WdfPdoInitAddCompatibleID(DeviceInit, &buffer);
    if (!NT_SUCCESS(status))
        return status;

    RtlUnicodeStringInit(&buffer, L"USB\\Class_FF&SubClass_5D&Prot_01");

    status = WdfPdoInitAddCompatibleID(DeviceInit, &buffer);
    if (!NT_SUCCESS(status))
        return status;

    RtlUnicodeStringInit(&buffer, L"USB\\Class_FF&SubClass_5D");

    status = WdfPdoInitAddCompatibleID(DeviceInit, &buffer);
    if (!NT_SUCCESS(status))
        return status;

    RtlUnicodeStringInit(&buffer, L"USB\\Class_FF");

    status = WdfPdoInitAddCompatibleID(DeviceInit, &buffer);
    if (!NT_SUCCESS(status))
        return status;

    return STATUS_SUCCESS;
}

NTSTATUS Xusb_PrepareHardware(WDFDEVICE Device)
{
    NTSTATUS status;
    WDF_QUERY_INTERFACE_CONFIG ifaceCfg;

    INTERFACE dummyIface;

    dummyIface.Size = sizeof(INTERFACE);
    dummyIface.Version = 1;
    dummyIface.Context = (PVOID)Device;

    dummyIface.InterfaceReference = WdfDeviceInterfaceReferenceNoOp;
    dummyIface.InterfaceDereference = WdfDeviceInterfaceDereferenceNoOp;

    /* XUSB.sys will query for the following three (unknown) interfaces
    * BUT WONT USE IT so we just expose them to satisfy initialization. */

    // Dummy 0

    WDF_QUERY_INTERFACE_CONFIG_INIT(&ifaceCfg, (PINTERFACE)&dummyIface, &GUID_DEVINTERFACE_XUSB_UNKNOWN_0, NULL);

    status = WdfDeviceAddQueryInterface(Device, &ifaceCfg);
    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVERNAME "Couldn't register unknown interface GUID: %08X-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X (status 0x%x)\n",
            GUID_DEVINTERFACE_XUSB_UNKNOWN_0.Data1,
            GUID_DEVINTERFACE_XUSB_UNKNOWN_0.Data2,
            GUID_DEVINTERFACE_XUSB_UNKNOWN_0.Data3,
            GUID_DEVINTERFACE_XUSB_UNKNOWN_0.Data4[0],
            GUID_DEVINTERFACE_XUSB_UNKNOWN_0.Data4[1],
            GUID_DEVINTERFACE_XUSB_UNKNOWN_0.Data4[2],
            GUID_DEVINTERFACE_XUSB_UNKNOWN_0.Data4[3],
            GUID_DEVINTERFACE_XUSB_UNKNOWN_0.Data4[4],
            GUID_DEVINTERFACE_XUSB_UNKNOWN_0.Data4[5],
            GUID_DEVINTERFACE_XUSB_UNKNOWN_0.Data4[6],
            GUID_DEVINTERFACE_XUSB_UNKNOWN_0.Data4[7],
            status));

        return status;
    }

    // Dummy 1

    WDF_QUERY_INTERFACE_CONFIG_INIT(&ifaceCfg, (PINTERFACE)&dummyIface, &GUID_DEVINTERFACE_XUSB_UNKNOWN_1, NULL);

    status = WdfDeviceAddQueryInterface(Device, &ifaceCfg);
    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVERNAME "Couldn't register unknown interface GUID: %08X-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X (status 0x%x)\n",
            GUID_DEVINTERFACE_XUSB_UNKNOWN_1.Data1,
            GUID_DEVINTERFACE_XUSB_UNKNOWN_1.Data2,
            GUID_DEVINTERFACE_XUSB_UNKNOWN_1.Data3,
            GUID_DEVINTERFACE_XUSB_UNKNOWN_1.Data4[0],
            GUID_DEVINTERFACE_XUSB_UNKNOWN_1.Data4[1],
            GUID_DEVINTERFACE_XUSB_UNKNOWN_1.Data4[2],
            GUID_DEVINTERFACE_XUSB_UNKNOWN_1.Data4[3],
            GUID_DEVINTERFACE_XUSB_UNKNOWN_1.Data4[4],
            GUID_DEVINTERFACE_XUSB_UNKNOWN_1.Data4[5],
            GUID_DEVINTERFACE_XUSB_UNKNOWN_1.Data4[6],
            GUID_DEVINTERFACE_XUSB_UNKNOWN_1.Data4[7],
            status));

        return status;
    }

    // Dummy 2

    WDF_QUERY_INTERFACE_CONFIG_INIT(&ifaceCfg, (PINTERFACE)&dummyIface, &GUID_DEVINTERFACE_XUSB_UNKNOWN_2, NULL);

    status = WdfDeviceAddQueryInterface(Device, &ifaceCfg);
    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVERNAME "Couldn't register unknown interface GUID: %08X-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X (status 0x%x)\n",
            GUID_DEVINTERFACE_XUSB_UNKNOWN_2.Data1,
            GUID_DEVINTERFACE_XUSB_UNKNOWN_2.Data2,
            GUID_DEVINTERFACE_XUSB_UNKNOWN_2.Data3,
            GUID_DEVINTERFACE_XUSB_UNKNOWN_2.Data4[0],
            GUID_DEVINTERFACE_XUSB_UNKNOWN_2.Data4[1],
            GUID_DEVINTERFACE_XUSB_UNKNOWN_2.Data4[2],
            GUID_DEVINTERFACE_XUSB_UNKNOWN_2.Data4[3],
            GUID_DEVINTERFACE_XUSB_UNKNOWN_2.Data4[4],
            GUID_DEVINTERFACE_XUSB_UNKNOWN_2.Data4[5],
            GUID_DEVINTERFACE_XUSB_UNKNOWN_2.Data4[6],
            GUID_DEVINTERFACE_XUSB_UNKNOWN_2.Data4[7],
            status));

        return status;
    }

    // Expose USB_BUS_INTERFACE_USBDI_GUID

    // This interface actually IS used
    USB_BUS_INTERFACE_USBDI_V1 xusbInterface;

    xusbInterface.Size = sizeof(USB_BUS_INTERFACE_USBDI_V1);
    xusbInterface.Version = USB_BUSIF_USBDI_VERSION_1;
    xusbInterface.BusContext = (PVOID)Device;

    xusbInterface.InterfaceReference = WdfDeviceInterfaceReferenceNoOp;
    xusbInterface.InterfaceDereference = WdfDeviceInterfaceDereferenceNoOp;

    xusbInterface.SubmitIsoOutUrb = UsbPdo_SubmitIsoOutUrb;
    xusbInterface.GetUSBDIVersion = UsbPdo_GetUSBDIVersion;
    xusbInterface.QueryBusTime = UsbPdo_QueryBusTime;
    xusbInterface.QueryBusInformation = UsbPdo_QueryBusInformation;
    xusbInterface.IsDeviceHighSpeed = UsbPdo_IsDeviceHighSpeed;

    WDF_QUERY_INTERFACE_CONFIG_INIT(&ifaceCfg, (PINTERFACE)&xusbInterface, &USB_BUS_INTERFACE_USBDI_GUID, NULL);

    status = WdfDeviceAddQueryInterface(Device, &ifaceCfg);
    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVERNAME "WdfDeviceAddQueryInterface failed status 0x%x\n", status));
        return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS Xusb_AssignPdoContext(WDFDEVICE Device, PPDO_IDENTIFICATION_DESCRIPTION Description)
{
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES attributes;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = Device;

    KdPrint((DRIVERNAME "Initializing XUSB context...\n"));

    PXUSB_DEVICE_DATA xusb = XusbGetData(Device);

    RtlZeroMemory(xusb, sizeof(XUSB_DEVICE_DATA));

    // Is later overwritten by actual XInput slot
    xusb->LedNumber = (UCHAR)Description->SerialNo;
    // Packet size (20 bytes = 0x14)
    xusb->Packet.Size = 0x14;

    // I/O Queue for pending IRPs
    WDF_IO_QUEUE_CONFIG usbInQueueConfig, notificationsQueueConfig, holdingInQueueConfig;

    // Create and assign queue for incoming interrupt transfer
    WDF_IO_QUEUE_CONFIG_INIT(&usbInQueueConfig, WdfIoQueueDispatchManual);

    status = WdfIoQueueCreate(Device, &usbInQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, &xusb->PendingUsbInRequests);
    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVERNAME "WdfIoQueueCreate failed 0x%x\n", status));
        return status;
    }

    // Create lock for queue
    status = WdfSpinLockCreate(&attributes, &xusb->PendingUsbInRequestsLock);
    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVERNAME "WdfSpinLockCreate failed 0x%x\n", status));
        return status;
    }

    // Create and assign queue for user-land notification requests
    WDF_IO_QUEUE_CONFIG_INIT(&notificationsQueueConfig, WdfIoQueueDispatchManual);

    status = WdfIoQueueCreate(Device, &notificationsQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, &xusb->PendingNotificationRequests);
    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVERNAME "WdfIoQueueCreate failed 0x%x\n", status));
        return status;
    }

    // Create lock for queue
    status = WdfSpinLockCreate(&attributes, &xusb->PendingNotificationRequestsLock);
    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVERNAME "WdfSpinLockCreate failed 0x%x\n", status));
        return status;
    }

    // Create and assign queue for unhandled interrupt requests
    WDF_IO_QUEUE_CONFIG_INIT(&holdingInQueueConfig, WdfIoQueueDispatchManual);

    status = WdfIoQueueCreate(Device, &holdingInQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, &xusb->HoldingUsbInRequests);
    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVERNAME "WdfIoQueueCreate failed 0x%x\n", status));
        return status;
    }

    // Create lock for queue
    status = WdfSpinLockCreate(&attributes, &xusb->HoldingUsbInRequestsLock);
    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVERNAME "WdfSpinLockCreate failed 0x%x\n", status));
        return status;
    }

    return STATUS_SUCCESS;
}

VOID Xusb_GetConfigurationDescriptorType(PUCHAR Buffer, ULONG Length)
{
    UCHAR XusbDescriptorData[XUSB_DESCRIPTOR_SIZE] =
    {
        0x09,        //   bLength
        0x02,        //   bDescriptorType (Configuration)
        0x99, 0x00,  //   wTotalLength 153
        0x04,        //   bNumInterfaces 4
        0x01,        //   bConfigurationValue
        0x00,        //   iConfiguration (String Index)
        0xA0,        //   bmAttributes Remote Wakeup
        0xFA,        //   bMaxPower 500mA

        0x09,        //   bLength
        0x04,        //   bDescriptorType (Interface)
        0x00,        //   bInterfaceNumber 0
        0x00,        //   bAlternateSetting
        0x02,        //   bNumEndpoints 2
        0xFF,        //   bInterfaceClass
        0x5D,        //   bInterfaceSubClass
        0x01,        //   bInterfaceProtocol
        0x00,        //   iInterface (String Index)

        0x11,        //   bLength
        0x21,        //   bDescriptorType (HID)
        0x00, 0x01,  //   bcdHID 1.00
        0x01,        //   bCountryCode
        0x25,        //   bNumDescriptors
        0x81,        //   bDescriptorType[0] (Unknown 0x81)
        0x14, 0x00,  //   wDescriptorLength[0] 20
        0x00,        //   bDescriptorType[1] (Unknown 0x00)
        0x00, 0x00,  //   wDescriptorLength[1] 0
        0x13,        //   bDescriptorType[2] (Unknown 0x13)
        0x01, 0x08,  //   wDescriptorLength[2] 2049
        0x00,        //   bDescriptorType[3] (Unknown 0x00)
        0x00,
        0x07,        //   bLength
        0x05,        //   bDescriptorType (Endpoint)
        0x81,        //   bEndpointAddress (IN/D2H)
        0x03,        //   bmAttributes (Interrupt)
        0x20, 0x00,  //   wMaxPacketSize 32
        0x04,        //   bInterval 4 (unit depends on device speed)

        0x07,        //   bLength
        0x05,        //   bDescriptorType (Endpoint)
        0x01,        //   bEndpointAddress (OUT/H2D)
        0x03,        //   bmAttributes (Interrupt)
        0x20, 0x00,  //   wMaxPacketSize 32
        0x08,        //   bInterval 8 (unit depends on device speed)

        0x09,        //   bLength
        0x04,        //   bDescriptorType (Interface)
        0x01,        //   bInterfaceNumber 1
        0x00,        //   bAlternateSetting
        0x04,        //   bNumEndpoints 4
        0xFF,        //   bInterfaceClass
        0x5D,        //   bInterfaceSubClass
        0x03,        //   bInterfaceProtocol
        0x00,        //   iInterface (String Index)

        0x1B,        //   bLength
        0x21,        //   bDescriptorType (HID)
        0x00, 0x01,  //   bcdHID 1.00
        0x01,        //   bCountryCode
        0x01,        //   bNumDescriptors
        0x82,        //   bDescriptorType[0] (Unknown 0x82)
        0x40, 0x01,  //   wDescriptorLength[0] 320
        0x02, 0x20, 0x16, 0x83, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x07,        //   bLength
        0x05,        //   bDescriptorType (Endpoint)
        0x82,        //   bEndpointAddress (IN/D2H)
        0x03,        //   bmAttributes (Interrupt)
        0x20, 0x00,  //   wMaxPacketSize 32
        0x02,        //   bInterval 2 (unit depends on device speed)

        0x07,        //   bLength
        0x05,        //   bDescriptorType (Endpoint)
        0x02,        //   bEndpointAddress (OUT/H2D)
        0x03,        //   bmAttributes (Interrupt)
        0x20, 0x00,  //   wMaxPacketSize 32
        0x04,        //   bInterval 4 (unit depends on device speed)

        0x07,        //   bLength
        0x05,        //   bDescriptorType (Endpoint)
        0x83,        //   bEndpointAddress (IN/D2H)
        0x03,        //   bmAttributes (Interrupt)
        0x20, 0x00,  //   wMaxPacketSize 32
        0x40,        //   bInterval 64 (unit depends on device speed)

        0x07,        //   bLength
        0x05,        //   bDescriptorType (Endpoint)
        0x03,        //   bEndpointAddress (OUT/H2D)
        0x03,        //   bmAttributes (Interrupt)
        0x20, 0x00,  //   wMaxPacketSize 32
        0x10,        //   bInterval 16 (unit depends on device speed)

        0x09,        //   bLength
        0x04,        //   bDescriptorType (Interface)
        0x02,        //   bInterfaceNumber 2
        0x00,        //   bAlternateSetting
        0x01,        //   bNumEndpoints 1
        0xFF,        //   bInterfaceClass
        0x5D,        //   bInterfaceSubClass
        0x02,        //   bInterfaceProtocol
        0x00,        //   iInterface (String Index)

        0x09,        //   bLength
        0x21,        //   bDescriptorType (HID)
        0x00, 0x01,  //   bcdHID 1.00
        0x01,        //   bCountryCode
        0x22,        //   bNumDescriptors
        0x84,        //   bDescriptorType[0] (Unknown 0x84)
        0x07, 0x00,  //   wDescriptorLength[0] 7

        0x07,        //   bLength
        0x05,        //   bDescriptorType (Endpoint)
        0x84,        //   bEndpointAddress (IN/D2H)
        0x03,        //   bmAttributes (Interrupt)
        0x20, 0x00,  //   wMaxPacketSize 32
        0x10,        //   bInterval 16 (unit depends on device speed)

        0x09,        //   bLength
        0x04,        //   bDescriptorType (Interface)
        0x03,        //   bInterfaceNumber 3
        0x00,        //   bAlternateSetting
        0x00,        //   bNumEndpoints 0
        0xFF,        //   bInterfaceClass
        0xFD,        //   bInterfaceSubClass
        0x13,        //   bInterfaceProtocol
        0x04,        //   iInterface (String Index)

        0x06,        //   bLength
        0x41,        //   bDescriptorType (Unknown)
        0x00, 0x01, 0x01, 0x03,
        // 153 bytes

        // best guess: USB Standard Descriptor
    };

    RtlCopyBytes(Buffer, XusbDescriptorData, Length);
}

VOID Xusb_GetDeviceDescriptorType(PUSB_DEVICE_DESCRIPTOR pDescriptor, PPDO_DEVICE_DATA pCommon)
{
    pDescriptor->bLength = 0x12;
    pDescriptor->bDescriptorType = USB_DEVICE_DESCRIPTOR_TYPE;
    pDescriptor->bcdUSB = 0x0200; // USB v2.0
    pDescriptor->bDeviceClass = 0xFF;
    pDescriptor->bDeviceSubClass = 0xFF;
    pDescriptor->bDeviceProtocol = 0xFF;
    pDescriptor->bMaxPacketSize0 = 0x08;
    pDescriptor->idVendor = pCommon->VendorId;
    pDescriptor->idProduct = pCommon->ProductId;
    pDescriptor->bcdDevice = 0x0114;
    pDescriptor->iManufacturer = 0x01;
    pDescriptor->iProduct = 0x02;
    pDescriptor->iSerialNumber = 0x03;
    pDescriptor->bNumConfigurations = 0x01;
}

VOID Xusb_SelectConfiguration(PUSBD_INTERFACE_INFORMATION pInfo)
{
    KdPrint((DRIVERNAME ">> >> >> URB_FUNCTION_SELECT_CONFIGURATION: Length %d, Interface %d, Alternate %d, Pipes %d\n",
        (int)pInfo->Length,
        (int)pInfo->InterfaceNumber,
        (int)pInfo->AlternateSetting,
        pInfo->NumberOfPipes));

    pInfo->Class = 0xFF;
    pInfo->SubClass = 0x5D;
    pInfo->Protocol = 0x01;

    pInfo->InterfaceHandle = (USBD_INTERFACE_HANDLE)0xFFFF0000;

    pInfo->Pipes[0].MaximumTransferSize = 0x00400000;
    pInfo->Pipes[0].MaximumPacketSize = 0x20;
    pInfo->Pipes[0].EndpointAddress = 0x81;
    pInfo->Pipes[0].Interval = 0x04;
    pInfo->Pipes[0].PipeType = 0x03;
    pInfo->Pipes[0].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0081;
    pInfo->Pipes[0].PipeFlags = 0x00;

    pInfo->Pipes[1].MaximumTransferSize = 0x00400000;
    pInfo->Pipes[1].MaximumPacketSize = 0x20;
    pInfo->Pipes[1].EndpointAddress = 0x01;
    pInfo->Pipes[1].Interval = 0x08;
    pInfo->Pipes[1].PipeType = 0x03;
    pInfo->Pipes[1].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0001;
    pInfo->Pipes[1].PipeFlags = 0x00;

    pInfo = (PUSBD_INTERFACE_INFORMATION)((PCHAR)pInfo + pInfo->Length);

    KdPrint((DRIVERNAME ">> >> >> URB_FUNCTION_SELECT_CONFIGURATION: Length %d, Interface %d, Alternate %d, Pipes %d\n",
        (int)pInfo->Length,
        (int)pInfo->InterfaceNumber,
        (int)pInfo->AlternateSetting,
        pInfo->NumberOfPipes));

    pInfo->Class = 0xFF;
    pInfo->SubClass = 0x5D;
    pInfo->Protocol = 0x03;

    pInfo->InterfaceHandle = (USBD_INTERFACE_HANDLE)0xFFFF0000;

    pInfo->Pipes[0].MaximumTransferSize = 0x00400000;
    pInfo->Pipes[0].MaximumPacketSize = 0x20;
    pInfo->Pipes[0].EndpointAddress = 0x82;
    pInfo->Pipes[0].Interval = 0x04;
    pInfo->Pipes[0].PipeType = 0x03;
    pInfo->Pipes[0].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0082;
    pInfo->Pipes[0].PipeFlags = 0x00;

    pInfo->Pipes[1].MaximumTransferSize = 0x00400000;
    pInfo->Pipes[1].MaximumPacketSize = 0x20;
    pInfo->Pipes[1].EndpointAddress = 0x02;
    pInfo->Pipes[1].Interval = 0x08;
    pInfo->Pipes[1].PipeType = 0x03;
    pInfo->Pipes[1].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0002;
    pInfo->Pipes[1].PipeFlags = 0x00;

    pInfo->Pipes[2].MaximumTransferSize = 0x00400000;
    pInfo->Pipes[2].MaximumPacketSize = 0x20;
    pInfo->Pipes[2].EndpointAddress = 0x83;
    pInfo->Pipes[2].Interval = 0x08;
    pInfo->Pipes[2].PipeType = 0x03;
    pInfo->Pipes[2].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0083;
    pInfo->Pipes[2].PipeFlags = 0x00;

    pInfo->Pipes[3].MaximumTransferSize = 0x00400000;
    pInfo->Pipes[3].MaximumPacketSize = 0x20;
    pInfo->Pipes[3].EndpointAddress = 0x03;
    pInfo->Pipes[3].Interval = 0x08;
    pInfo->Pipes[3].PipeType = 0x03;
    pInfo->Pipes[3].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0003;
    pInfo->Pipes[3].PipeFlags = 0x00;

    pInfo = (PUSBD_INTERFACE_INFORMATION)((PCHAR)pInfo + pInfo->Length);

    KdPrint((DRIVERNAME ">> >> >> URB_FUNCTION_SELECT_CONFIGURATION: Length %d, Interface %d, Alternate %d, Pipes %d\n",
        (int)pInfo->Length,
        (int)pInfo->InterfaceNumber,
        (int)pInfo->AlternateSetting,
        pInfo->NumberOfPipes));

    pInfo->Class = 0xFF;
    pInfo->SubClass = 0x5D;
    pInfo->Protocol = 0x02;

    pInfo->InterfaceHandle = (USBD_INTERFACE_HANDLE)0xFFFF0000;

    pInfo->Pipes[0].MaximumTransferSize = 0x00400000;
    pInfo->Pipes[0].MaximumPacketSize = 0x20;
    pInfo->Pipes[0].EndpointAddress = 0x84;
    pInfo->Pipes[0].Interval = 0x04;
    pInfo->Pipes[0].PipeType = 0x03;
    pInfo->Pipes[0].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0084;
    pInfo->Pipes[0].PipeFlags = 0x00;

    pInfo = (PUSBD_INTERFACE_INFORMATION)((PCHAR)pInfo + pInfo->Length);

    KdPrint((DRIVERNAME ">> >> >> URB_FUNCTION_SELECT_CONFIGURATION: Length %d, Interface %d, Alternate %d, Pipes %d\n",
        (int)pInfo->Length,
        (int)pInfo->InterfaceNumber,
        (int)pInfo->AlternateSetting,
        pInfo->NumberOfPipes));

    pInfo->Class = 0xFF;
    pInfo->SubClass = 0xFD;
    pInfo->Protocol = 0x13;

    pInfo->InterfaceHandle = (USBD_INTERFACE_HANDLE)0xFFFF0000;
}

