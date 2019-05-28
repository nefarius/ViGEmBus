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


#include "NotificationRequestPool.h"

void NotificationRequestPool::strand_dispatch_worker() const
{
    io_svc_->run();
}

NotificationRequestPool::NotificationRequestPool(
    PVIGEM_CLIENT client,
    PVIGEM_TARGET target,
    FARPROC callback
) :
    client_(client),
    target_(target),
    callback_(callback),
    stop_(false)
{
    // prepare array of handles and request wrappers
    for (auto& wait_handle : wait_handles_)
    {
        // create auto-reset event
        wait_handle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        // create async pending I/O request wrapper
        requests_.push_back(std::make_unique<XusbNotificationRequest>(
            client_,
            target_,
            wait_handle
            ));
    }

    // init ASIO
    io_svc_.reset(new boost::asio::io_service());
    worker_.reset(new boost::asio::io_service::work(*io_svc_));

    // launch notification completion thread
    thread_ = std::make_shared<boost::thread>(boost::ref(*this));

    // launch boost I/O service dispatcher thread
    worker_thread_ = std::make_shared<boost::thread>(
        boost::bind(&NotificationRequestPool::strand_dispatch_worker, this));

    // submit pending I/O to driver
    for (auto& request : requests_)
        request->request_async();
}

NotificationRequestPool::~NotificationRequestPool() noexcept(false)
{
    for (auto& wait_handle : wait_handles_)
        CloseHandle(wait_handle);

    io_svc_->stop();
    worker_thread_->join();

    thread_->join();
}

void NotificationRequestPool::operator()()
{
    // used to dispatch notification callback
    boost::asio::io_service::strand strand(*io_svc_);

    while (true)
    {
        // wait for the first pending I/O to signal its event
        const auto ret = WaitForMultipleObjects(
            requests_.size(),
            wait_handles_,
            FALSE, // first one to be signaled wins
            25 // don't block indefinitely so worker can be canceled
        );

        // timeout has occurred...
        if (ret == WAIT_TIMEOUT)
        {
            // ...check for termination request
            boost::mutex::scoped_lock lock(m_);
            if (stop_)
                // exits function (terminates thread)
                break;

            // go for another round
            continue;
        }

        // index of the request which just got completed
        const auto index = ret - WAIT_OBJECT_0;
        // grab associated request
        const auto req = requests_[index].get();

        // submit callback for async yet ordered invocation
        req->post(std::move(strand));

        // submit another pending I/O
        req->request_async();
    }
}

void NotificationRequestPool::terminate()
{
    boost::mutex::scoped_lock lock(m_);
    stop_ = true;
}
