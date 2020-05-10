#pragma once

#include <ntddk.h>
#include <wdf.h>
#include <ntintsafe.h>

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

} PDO_IDENTIFICATION_DESCRIPTION, * PPDO_IDENTIFICATION_DESCRIPTION;
