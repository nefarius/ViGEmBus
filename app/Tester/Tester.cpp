// Tester.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ViGEm/Client.h>

#include <mutex>
#include <iostream>

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

    static int count = 1;

    std::cout.width(3);
    std::cout << count++ << " ";
    std::cout.width(3);
    std::cout << (int)LargeMotor << " ";
    std::cout.width(3);
    std::cout << (int)SmallMotor << std::endl;

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

    while(!(GetAsyncKeyState(VK_ESCAPE) & 0x8000))
    {
        ret = vigem_target_x360_update(client, x360, report);
        report.bLeftTrigger++;
        Sleep(10);
    }

    vigem_target_x360_unregister_notification(x360);
    vigem_target_remove(client, x360);
    vigem_target_free(x360);
    vigem_free(client);
}
