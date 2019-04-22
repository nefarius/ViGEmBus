#pragma once
#include "XusbNotificationRequest.h"
#include "NotificationRequestPool.h"

//
// Represents a driver connection object.
// 
typedef struct _VIGEM_CLIENT_T
{
    HANDLE hBusDevice;

} VIGEM_CLIENT;

//
// Represents the (connection) state of a target device object.
// 
typedef enum _VIGEM_TARGET_STATE
{
    VIGEM_TARGET_NEW,
    VIGEM_TARGET_INITIALIZED,
    VIGEM_TARGET_CONNECTED,
    VIGEM_TARGET_DISCONNECTED
} VIGEM_TARGET_STATE, *PVIGEM_TARGET_STATE;

class NotificationRequestPool;

//
// Represents a virtual gamepad object.
// 
typedef struct _VIGEM_TARGET_T
{
    ULONG Size;
    ULONG SerialNo;
    VIGEM_TARGET_STATE State;
    USHORT VendorId;
    USHORT ProductId;
    VIGEM_TARGET_TYPE Type;
    DWORD_PTR Notification;

    std::shared_ptr<NotificationRequestPool> pool;

} VIGEM_TARGET;
