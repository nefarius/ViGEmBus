#include "XusbNotificationRequest.h"


XusbNotificationRequest::XusbNotificationRequest(
    HANDLE bus,
    ULONG serial,
    HANDLE notification
) : parent_bus_(bus),
    payload_(),
    overlapped_()
{
    memset(&overlapped_, 0, sizeof(OVERLAPPED));
    overlapped_.hEvent = notification;
    XUSB_REQUEST_NOTIFICATION_INIT(&payload_, serial);
}

bool XusbNotificationRequest::request_async()
{
    // queue request in driver
    const auto ret = DeviceIoControl(
        parent_bus_,
        IOCTL_XUSB_REQUEST_NOTIFICATION,
        &payload_,
        payload_.Size,
        &payload_,
        payload_.Size,
        nullptr,
        &overlapped_
    );

    const auto error = GetLastError();

    return (!ret && error == ERROR_IO_PENDING);
}

UCHAR XusbNotificationRequest::get_led_number() const
{
    return payload_.LedNumber;
}

UCHAR XusbNotificationRequest::get_large_motor() const
{
    return payload_.LargeMotor;
}

UCHAR XusbNotificationRequest::get_small_motor() const
{
    return payload_.SmallMotor;
}
