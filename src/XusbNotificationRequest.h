#pragma once

#include <Windows.h>
#include "ViGEm/km/BusShared.h"

class XusbNotificationRequest
{
private:
    HANDLE parent_bus;
    XUSB_REQUEST_NOTIFICATION payload;
    DWORD transferred;
    OVERLAPPED overlapped;

public:
    XusbNotificationRequest(HANDLE bus, ULONG serial, HANDLE wait);

    void requestAsync();
};
