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


#pragma once

#include <Windows.h>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <vector>
#include "XusbNotificationRequest.h"
#include "ViGEm/Client.h"
#include "Internal.h"


class NotificationRequestPool
{
    static const int thread_count = 20;
    HANDLE wait_handles_[thread_count]{};

    PVIGEM_CLIENT client_;
    PVIGEM_TARGET target_;

    std::vector<std::unique_ptr<XusbNotificationRequest>> requests_;
    std::shared_ptr<boost::thread> thread_;
    boost::mutex m_;
    boost::condition_variable cv_;
    bool stop_;

    boost::shared_ptr<boost::asio::io_service> io_svc_;
    boost::shared_ptr<boost::asio::io_service::work> worker_;
    std::shared_ptr<boost::thread> worker_thread_;

    void strand_dispatch_worker() const;

public:
    NotificationRequestPool(
        PVIGEM_CLIENT client, 
        PVIGEM_TARGET target, 
        FARPROC callback
    );
    ~NotificationRequestPool() noexcept(false);

    void operator()();
    void terminate();
};
