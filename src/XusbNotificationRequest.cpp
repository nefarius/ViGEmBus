#include "XusbNotificationRequest.h"


XusbNotificationRequest::XusbNotificationRequest(
    HANDLE bus,
    ULONG serial,
    HANDLE wait
) : parent_bus(bus),
    payload(), transferred(0),
    overlapped()
{
    overlapped.hEvent = wait;
    XUSB_REQUEST_NOTIFICATION_INIT(&payload, serial);
}

void XusbNotificationRequest::requestAsync()
{
    DeviceIoControl(
        parent_bus,
        IOCTL_XUSB_REQUEST_NOTIFICATION,
        &payload,
        payload.Size,
        &payload,
        payload.Size,
        &transferred,
        &overlapped
    );
}
