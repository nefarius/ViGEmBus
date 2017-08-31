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

#define HID_GET_FEATURE_REPORT_SIZE_0                   0x31
#define HID_GET_FEATURE_REPORT_SIZE_1                   0x25
#define HID_GET_FEATURE_REPORT_MAC_ADDRESSES_SIZE       0x10

#define HID_SET_FEATURE_REPORT_SIZE_0                   0x17
#define HID_SET_FEATURE_REPORT_SIZE_1                   0x11

#define HID_REPORT_ID_0                                 0xA3
#define HID_REPORT_ID_1                                 0x02
#define HID_REPORT_MAC_ADDRESSES_ID                     0x12
#define HID_REPORT_ID_3                                 0x13
#define HID_REPORT_ID_4                                 0x14

#define DS4_DESCRIPTOR_SIZE	                            0x0029
#define DS4_CONFIGURATION_SIZE                          0x0070
#define DS4_HID_REPORT_DESCRIPTOR_SIZE                  0x01D3

#define DS4_MANUFACTURER_NAME_LENGTH                    0x38
#define DS4_PRODUCT_NAME_LENGTH                         0x28
#define DS4_OUTPUT_BUFFER_OFFSET                        0x04
#define DS4_OUTPUT_BUFFER_LENGTH                        0x05

#define DS4_REPORT_SIZE                                 0x40
#define DS4_QUEUE_FLUSH_PERIOD                          0x05


//
// DS4-specific device context data.
// 
typedef struct _DS4_DEVICE_DATA
{
    //
    // HID Input Report buffer
    //
    UCHAR Report[DS4_REPORT_SIZE];

    //
    // Output report cache
    //
    DS4_OUTPUT_REPORT OutputReport;

    //
    // Queue for incoming interrupt transfer
    //
    WDFQUEUE PendingUsbInRequests;

    //
    // Timer for dispatching interrupt transfer
    //
    WDFTIMER PendingUsbInRequestsTimer;

    //
    // Queue for inverted calls
    //
    WDFQUEUE PendingNotificationRequests;

    //
    // Auto-generated MAC address of the target device
    //
    MAC_ADDRESS TargetMacAddress;

    //
    // Default MAC address of the host (not used)
    //
    MAC_ADDRESS HostMacAddress;
} DS4_DEVICE_DATA, *PDS4_DEVICE_DATA;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DS4_DEVICE_DATA, Ds4GetData)


EVT_WDF_TIMER Ds4_PendingUsbRequestsTimerFunc;

NTSTATUS
Bus_Ds4SubmitReport(
    WDFDEVICE Device,
    ULONG SerialNo,
    PDS4_SUBMIT_REPORT Report,
    _In_ BOOLEAN FromInterface
);

//
// DS4-specific functions
// 
NTSTATUS Ds4_PreparePdo(PWDFDEVICE_INIT DeviceInit, PUNICODE_STRING DeviceId, PUNICODE_STRING DeviceDescription);
NTSTATUS Ds4_PrepareHardware(WDFDEVICE Device);
NTSTATUS Ds4_AssignPdoContext(WDFDEVICE Device, PPDO_IDENTIFICATION_DESCRIPTION Description);
VOID Ds4_GetConfigurationDescriptorType(PUCHAR Buffer, ULONG Length);
VOID Ds4_GetDeviceDescriptorType(PUSB_DEVICE_DESCRIPTOR pDescriptor, PPDO_DEVICE_DATA pCommon);
VOID Ds4_SelectConfiguration(PUSBD_INTERFACE_INFORMATION pInfo);

