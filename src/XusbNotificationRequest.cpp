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
#include <ViGEm/Client.h>
#include "Internal.h"
#include <boost/bind/bind.hpp>
#include <boost/function.hpp>


XusbNotificationRequest::XusbNotificationRequest(
    PVIGEM_CLIENT client,
    PVIGEM_TARGET target,
    HANDLE notification
) : client_(client),
    target_(target),
    payload_(),
    overlapped_()
{
    memset(&overlapped_, 0, sizeof(OVERLAPPED));
    overlapped_.hEvent = notification;
    XUSB_REQUEST_NOTIFICATION_INIT(&payload_, target_->SerialNo);
}

bool XusbNotificationRequest::request_async()
{
    // queue request in driver
    const auto ret = DeviceIoControl(
        client_->hBusDevice,
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

void XusbNotificationRequest::post(boost::asio::io_service::strand strand) const
{
    // prepare queueing library caller notification callback
    const boost::function<void(
        PVIGEM_CLIENT,
        PVIGEM_TARGET,
        UCHAR,
        UCHAR,
        UCHAR)> pfn = PFN_VIGEM_X360_NOTIFICATION(target_->Notification);

    // submit callback for async yet ordered invocation
    strand.post(boost::bind(pfn,
        client_,
        target_,
        payload_.LargeMotor,
        payload_.SmallMotor,
        payload_.LedNumber
    ));
}
