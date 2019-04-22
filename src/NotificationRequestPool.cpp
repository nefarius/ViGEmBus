#include "NotificationRequestPool.h"

NotificationRequestPool::NotificationRequestPool(
    HANDLE bus,
    const ULONG serial,
    PFN_VIGEM_X360_NOTIFICATION callback) :
    callback_(callback),
    stop_(false)
{
    for (auto& wait_handle : wait_handles_)
    {
        wait_handle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        XusbNotificationRequest req(bus, serial, wait_handle);
        requests_.push_back(std::ref(req));
    }

    io_svc_.reset(new boost::asio::io_service());
    worker_.reset(new boost::asio::io_service::work(*io_svc_));
    worker_threads_.reset(new boost::thread_group());

    thread_ = std::make_shared<boost::thread>(boost::ref(*this));

    for (auto& request : requests_)
        request.get().request_async();
}

NotificationRequestPool::~NotificationRequestPool()
{
    for (auto& wait_handle : wait_handles_)
        CloseHandle(wait_handle);

    thread_->join();
}

void NotificationRequestPool::operator()()
{
    boost::asio::io_service::strand strand(*io_svc_);

    while (true)
    {
        const auto ret = WaitForMultipleObjects(
            requests_.size(),
            wait_handles_,
            FALSE,
            INFINITE
        );

        const auto index = ret - WAIT_OBJECT_0;
        auto& req = requests_[index].get();

        //const boost::function<void(
        //    PVIGEM_CLIENT,
        //    PVIGEM_TARGET,
        //    UCHAR,
        //    UCHAR,
        //    UCHAR
        //    )> pfn = callback_;

        //strand.post(boost::bind(pfn,
        //    client,
        //    target,
        //    notify.LargeMotor,
        //    notify.SmallMotor,
        //    notify.LedNumber
        //));

        req.request_async();

        boost::mutex::scoped_lock lock(m_);
        if (stop_)
            break;
    }
}

void NotificationRequestPool::terminate()
{
    boost::mutex::scoped_lock lock(m_);
    stop_ = true;
}
