#pragma once

#define WIN32_LEAN_AND_MEAN
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
    FARPROC callback_;

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
