// Tester.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ViGEm/Client.h>

#include <mutex>
#include <iostream>
#include "../../../vcpkg/installed/x86-windows-static/include/boost/thread/thread_only.hpp"
#include "../../../vcpkg/installed/x86-windows-static/include/boost/thread/once.hpp"

static std::mutex m;

VOID CALLBACK notification(
    PVIGEM_CLIENT Client,
    PVIGEM_TARGET Target,
    UCHAR LargeMotor,
    UCHAR SmallMotor,
    UCHAR LedNumber
)
{
    m.lock();

    //std::cout.width(3);
    //std::cout << (int)LargeMotor << " ";
    //std::cout.width(3);
    //std::cout << (int)SmallMotor << std::endl;

    m.unlock();
}

int main()
{
    const auto client = vigem_alloc();

    auto ret = vigem_connect(client);

    const auto x360 = vigem_target_x360_alloc();

    ret = vigem_target_add(client, x360);

    ret = vigem_target_x360_register_notification(client, x360, &notification);

    XUSB_REPORT report;
    XUSB_REPORT_INIT(&report);

    while(true)
    {
        ret = vigem_target_x360_update(client, x360, report);
        report.bLeftTrigger++;
        boost::detail::win32::sleep(10);
    }

    vigem_target_remove(client, x360);
    vigem_target_free(x360);
    vigem_free(client);
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
