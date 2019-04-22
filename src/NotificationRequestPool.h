#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <vector>
#include "XusbNotificationRequest.h"
#include "ViGEm/Client.h"

#define VIGEM_INVERTED_CALL_THREAD_COUNT    20


class NotificationRequestPool
{
    HANDLE wait_handles_[VIGEM_INVERTED_CALL_THREAD_COUNT]{};
    PFN_VIGEM_X360_NOTIFICATION callback_;
    std::vector<std::reference_wrapper<XusbNotificationRequest>> requests_;
    std::shared_ptr<boost::thread> thread_;
    boost::mutex m_;
    boost::condition_variable cv_;
    bool stop_;

    boost::shared_ptr<boost::asio::io_service> io_svc_;
    boost::shared_ptr<boost::asio::io_service::work> worker_;
    boost::shared_ptr<boost::thread_group> worker_threads_;

public:
    NotificationRequestPool(HANDLE bus, ULONG serial, PFN_VIGEM_X360_NOTIFICATION callback);
    ~NotificationRequestPool();

    void operator()();
    void terminate();
};
