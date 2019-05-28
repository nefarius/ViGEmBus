/*
MIT License

Copyright (c) 2017-2019 Nefarius Software Solutions e.U. and Contributors

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


#include "XusbNotificationRequest.h"
#include <winioctl.h>


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
