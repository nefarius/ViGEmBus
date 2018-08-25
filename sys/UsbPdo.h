/*
* Virtual Gamepad Emulation Framework - Windows kernel-mode bus driver
* Copyright (C) 2016-2018  Benjamin Höglinger-Stelzer
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
