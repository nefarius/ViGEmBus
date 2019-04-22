#include "XusbNotificationRequest.h"


XusbNotificationRequest::XusbNotificationRequest(
    HANDLE bus,
    ULONG serial,
    HANDLE notification
) : parent_bus(bus),
    payload(), transferred(0),
    overlapped()
{
    overlapped.hEvent = notification;
    XUSB_REQUEST_NOTIFICATION_INIT(&payload, serial);
}

bool XusbNotificationRequest::request_async()
{
    const auto ret = DeviceIoControl(
        parent_bus,
        IOCTL_XUSB_REQUEST_NOTIFICATION,
        &payload,
        payload.Size,
        &payload,
        payload.Size,
        nullptr,
        &overlapped
    );

    return (!ret && GetLastError() == ERROR_IO_PENDING);
}

UCHAR XusbNotificationRequest::get_led_number() const
{
    return payload.LedNumber;
}

UCHAR XusbNotificationRequest::get_large_motor() const
{
    return payload.LargeMotor;
}

UCHAR XusbNotificationRequest::get_small_motor() const
{
    return payload.SmallMotor;
}
