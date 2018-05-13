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

//
// Used to identify children in the device list of the bus.
// 
typedef struct _PDO_IDENTIFICATION_DESCRIPTION
{
    WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER Header; // should contain this header

    //
    // Unique serial number of the device on the bus
    // 
    ULONG SerialNo;

    // 
    // PID of the process creating this PDO
    // 
    DWORD OwnerProcessId;

    //
    // Device type this PDO is emulating
    // 
    VIGEM_TARGET_TYPE TargetType;

    //
    // If set, the vendor ID the emulated device is reporting
    // 
    USHORT VendorId;

    //
    // If set, the product ID the emulated device is reporting
    // 
    USHORT ProductId;

    //
    // Is the current device owner another driver?
    // 
    BOOLEAN OwnerIsDriver;

    //
    // SessionId associated with file handle.  Used to map file handles to emulated gamepad devices
    // 
    LONG SessionId;

} PDO_IDENTIFICATION_DESCRIPTION, *PPDO_IDENTIFICATION_DESCRIPTION;

//
// The PDO device-extension (context).
//
typedef struct _PDO_DEVICE_DATA
{
    //
    // Unique serial number of the device on the bus
    // 
    ULONG SerialNo;

    // 
    // PID of the process creating this PDO
    // 
    DWORD OwnerProcessId;

    //
    // Device type this PDO is emulating
    // 
    VIGEM_TARGET_TYPE TargetType;

    //
    // If set, the vendor ID the emulated device is reporting
    // 
    USHORT VendorId;

    //
    // If set, the product ID the emulated device is reporting
    // 
    USHORT ProductId;

    //
    // Interface for PDO to FDO communication
    // 
    VIGEM_BUS_INTERFACE BusInterface;

    //
    // Queue for incoming data interrupt transfer
    //
    WDFQUEUE PendingUsbInRequests;

    //
    // Queue for inverted calls
    //
    WDFQUEUE PendingNotificationRequests;

} PDO_DEVICE_DATA, *PPDO_DEVICE_DATA;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(PDO_DEVICE_DATA, PdoGetData)

//
// FDO (bus device) context data
// 
typedef struct _FDO_DEVICE_DATA
{
    //
    // Counter of interface references
    // 
    LONG InterfaceReferenceCounter;

    //
    // Next SessionId to assign to a file handle
    // 
    LONG NextSessionId;

    //
    // Collection holding pending plugin requests
    // 
    WDFCOLLECTION PendingPluginRequests;

    //
    // Sync lock for pending request collection
    // 
    WDFSPINLOCK PendingPluginRequestsLock;

    //
    // Periodic timer sweeping up orphaned requests
    // 
    WDFTIMER PendingPluginRequestsCleanupTimer;

} FDO_DEVICE_DATA, *PFDO_DEVICE_DATA;

#define FDO_FIRST_SESSION_ID 100

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FDO_DEVICE_DATA, FdoGetData)

// 
// Context data associated with file objects created by user mode applications
// 
typedef struct _FDO_FILE_DATA
{
    //
    // SessionId associated with file handle.  Used to map file handles to emulated gamepad devices
    // 
    LONG SessionId;

} FDO_FILE_DATA, *PFDO_FILE_DATA;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FDO_FILE_DATA, FileObjectGetData)

//
// Context data for plugin requests
// 
typedef struct _FDO_PLUGIN_REQUEST_DATA
{
    //
    // Unique serial number of the device on the bus
    // 
    ULONG Serial;

    //
    // High resolution timestamp taken when this request got moved to pending state
    // 
    LARGE_INTEGER Timestamp;

    //
    // Performance counter system frequency taken upon fetching timestamp
    // 
    LARGE_INTEGER Frequency;

} FDO_PLUGIN_REQUEST_DATA, *PFDO_PLUGIN_REQUEST_DATA;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FDO_PLUGIN_REQUEST_DATA, PluginRequestGetData)

