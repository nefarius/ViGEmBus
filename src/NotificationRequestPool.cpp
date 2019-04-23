#include "NotificationRequestPool.h"

void NotificationRequestPool::strand_dispatch_worker() const
{
    io_svc_->run();
}

NotificationRequestPool::NotificationRequestPool(
    PVIGEM_CLIENT client,
    PVIGEM_TARGET target,
    PFN_VIGEM_X360_NOTIFICATION callback
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
            client_->hBusDevice,
            target_->SerialNo,
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

NotificationRequestPool::~NotificationRequestPool()
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

        // prepare queueing library caller notification callback
        const boost::function<void(
            PVIGEM_CLIENT,
            PVIGEM_TARGET,
            UCHAR,
            UCHAR,
            UCHAR)> pfn = callback_;

        // submit callback for async yet ordered invocation
        strand.post(boost::bind(pfn,
                                client_,
                                target_,
                                req->get_large_motor(),
                                req->get_small_motor(),
                                req->get_led_number()
        ));

        // submit another pending I/O
        req->request_async();
    }
}

void NotificationRequestPool::terminate()
{
    boost::mutex::scoped_lock lock(m_);
    stop_ = true;
}
