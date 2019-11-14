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

#if X360
    const auto x360 = vigem_target_x360_alloc();

	ret = vigem_target_add(client, x360);

	ret = vigem_target_x360_register_notification(client, x360, &notification);
#endif
	
	const auto ds4 = vigem_target_ds4_alloc();
	
	ret = vigem_target_add(client, ds4);

#if X360
    XUSB_REPORT report;
    XUSB_REPORT_INIT(&report);

    while(!(GetAsyncKeyState(VK_ESCAPE) & 0x8000))
    {
        ret = vigem_target_x360_update(client, x360, report);
        report.bLeftTrigger++;
        Sleep(10);
    }

#endif

	DS4_REPORT report;
	DS4_REPORT_INIT(&report);

	while (!(GetAsyncKeyState(VK_ESCAPE) & 0x8000))
	{
		ret = vigem_target_ds4_update(client, ds4, report);
		report.bThumbLX++;
		report.wButtons |= DS4_BUTTON_CIRCLE;
		Sleep(10);
	}

#if X360
    vigem_target_x360_unregister_notification(x360);
    vigem_target_remove(client, x360);
    vigem_target_free(x360);
#endif

	vigem_target_remove(client, ds4);
	vigem_target_free(ds4);
	
    vigem_free(client);
}
