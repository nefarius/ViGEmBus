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


#pragma once

BOOLEAN USB_BUSIFFN UsbPdo_IsDeviceHighSpeed(IN PVOID BusContext);
NTSTATUS USB_BUSIFFN UsbPdo_QueryBusInformation(
    IN PVOID BusContext,
    IN ULONG Level,
    IN OUT PVOID BusInformationBuffer,
    IN OUT PULONG BusInformationBufferLength,
    OUT PULONG BusInformationActualLength
);
NTSTATUS USB_BUSIFFN UsbPdo_SubmitIsoOutUrb(IN PVOID BusContext, IN PURB Urb);
NTSTATUS USB_BUSIFFN UsbPdo_QueryBusTime(IN PVOID BusContext, IN OUT PULONG CurrentUsbFrame);
VOID USB_BUSIFFN UsbPdo_GetUSBDIVersion(
    IN PVOID BusContext,
    IN OUT PUSBD_VERSION_INFORMATION VersionInformation,
    IN OUT PULONG HcdCapabilities
);
NTSTATUS UsbPdo_GetDeviceDescriptorType(PURB urb, PPDO_DEVICE_DATA pCommon);
NTSTATUS UsbPdo_GetConfigurationDescriptorType(PURB urb, PPDO_DEVICE_DATA pCommon);
NTSTATUS UsbPdo_GetStringDescriptorType(PURB urb, PPDO_DEVICE_DATA pCommon);
NTSTATUS UsbPdo_SelectConfiguration(PURB urb, PPDO_DEVICE_DATA pCommon);
NTSTATUS UsbPdo_SelectInterface(PURB urb, PPDO_DEVICE_DATA pCommon);
NTSTATUS UsbPdo_BulkOrInterruptTransfer(PURB urb, WDFDEVICE Device, WDFREQUEST Request);
NTSTATUS UsbPdo_AbortPipe(WDFDEVICE Device);
NTSTATUS UsbPdo_ClassInterface(PURB urb, WDFDEVICE Device, PPDO_DEVICE_DATA pCommon);
NTSTATUS UsbPdo_GetDescriptorFromInterface(PURB urb, PPDO_DEVICE_DATA pCommon);
