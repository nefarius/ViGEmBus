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


//
// WinAPI
// 
#include <Windows.h>
#include <SetupAPI.h>
#include <initguid.h>
#include <Dbghelp.h>

//
// Driver shared
// 
#include "ViGEm/km/BusShared.h"
#include "ViGEm/Client.h"
#include <winioctl.h>

//
// STL
// 
#include <cstdlib>
#include <climits>
#include <vector>
#include <algorithm>
#include <thread>
#include <functional>

//
// Internal
// 
#include "Internal.h"


typedef BOOL(WINAPI *MINIDUMPWRITEDUMP)(
    HANDLE hProcess,
    DWORD dwPid,
    HANDLE hFile,
    MINIDUMP_TYPE DumpType,
    CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
    CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
    CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam
    );

LONG WINAPI vigem_internal_exception_handler(struct _EXCEPTION_POINTERS* apExceptionInfo);


//
// DeviceIOControl request notification handler classes for X360 and DS4 controller types. 
// vigem_target_XXX_register_notification functions use x360 or DS4 derived class instances in a notification thread handlers.
//
class NotificationRequestPayload
{
public:
	LPVOID lpPayloadBuffer;
	DWORD  payloadBufferSize;
	DWORD  ioControlCode;

public:
	NotificationRequestPayload(DWORD _bufferSize, DWORD _ioControlCode)
	{ 
		lpPayloadBuffer = malloc(_bufferSize);
		payloadBufferSize = _bufferSize;
		ioControlCode = _ioControlCode;
	}

	virtual ~NotificationRequestPayload()
	{
		free(lpPayloadBuffer);
	}

	virtual void ProcessNotificationRequest(PVIGEM_CLIENT client, PVIGEM_TARGET target) = 0;
};

class NotificationRequestPayloadX360 : public NotificationRequestPayload
{
public:
	NotificationRequestPayloadX360(ULONG _serialNo) : NotificationRequestPayload(sizeof(XUSB_REQUEST_NOTIFICATION), IOCTL_XUSB_REQUEST_NOTIFICATION)
	{
		// Let base class to allocate required buffer size, but initialize it here with a correct type of initialization function
		XUSB_REQUEST_NOTIFICATION_INIT((PXUSB_REQUEST_NOTIFICATION)lpPayloadBuffer, _serialNo);
	}

	void ProcessNotificationRequest(PVIGEM_CLIENT client, PVIGEM_TARGET target) override
	{
		if(target->Notification != nullptr)
			PFN_VIGEM_X360_NOTIFICATION(target->Notification)(client, target, 
				((PXUSB_REQUEST_NOTIFICATION)lpPayloadBuffer)->LargeMotor, 
				((PXUSB_REQUEST_NOTIFICATION)lpPayloadBuffer)->SmallMotor,
				((PXUSB_REQUEST_NOTIFICATION)lpPayloadBuffer)->LedNumber
			);
	}
};

class NotificationRequestPayloadDS4 : public NotificationRequestPayload
{
public:
	NotificationRequestPayloadDS4(ULONG _serialNo) : NotificationRequestPayload(sizeof(DS4_REQUEST_NOTIFICATION), IOCTL_DS4_REQUEST_NOTIFICATION)
	{
		// Let base class to allocate required buffer size, but initialize it here with a correct type of initialization function
		DS4_REQUEST_NOTIFICATION_INIT((PDS4_REQUEST_NOTIFICATION)lpPayloadBuffer, _serialNo);
	}

	void ProcessNotificationRequest(PVIGEM_CLIENT client, PVIGEM_TARGET target) override
	{
		if (target->Notification != nullptr)
			PFN_VIGEM_DS4_NOTIFICATION(target->Notification)(client, target,
				((PDS4_REQUEST_NOTIFICATION)lpPayloadBuffer)->Report.LargeMotor,
				((PDS4_REQUEST_NOTIFICATION)lpPayloadBuffer)->Report.SmallMotor,
				((PDS4_REQUEST_NOTIFICATION)lpPayloadBuffer)->Report.LightbarColor
			);
	}
};


//
// Initializes a virtual gamepad object.
// 
PVIGEM_TARGET FORCEINLINE VIGEM_TARGET_ALLOC_INIT(
    _In_ VIGEM_TARGET_TYPE Type
)
{
    auto target = static_cast<PVIGEM_TARGET>(malloc(sizeof(VIGEM_TARGET)));

    if (!target)
        return nullptr;

    memset(target, 0, sizeof(VIGEM_TARGET));

    target->Size = sizeof(VIGEM_TARGET);
    target->State = VIGEM_TARGET_INITIALIZED;
    target->Type = Type;
	target->notificationThreadList = nullptr;
    return target;
}


LONG WINAPI vigem_internal_exception_handler(struct _EXCEPTION_POINTERS* apExceptionInfo)
{
    const auto mhLib = LoadLibrary(L"dbghelp.dll");
    const auto pDump = reinterpret_cast<MINIDUMPWRITEDUMP>(GetProcAddress(mhLib, "MiniDumpWriteDump"));

    const auto hFile = CreateFile(
        L"ViGEmClient.dmp",
        GENERIC_WRITE,
        FILE_SHARE_WRITE,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

	const DWORD flags = MiniDumpWithFullMemory | MiniDumpWithHandleData | MiniDumpWithUnloadedModules |
		MiniDumpWithUnloadedModules | MiniDumpWithProcessThreadData |
		MiniDumpWithFullMemoryInfo | MiniDumpWithThreadInfo |
		MiniDumpWithFullAuxiliaryState | MiniDumpIgnoreInaccessibleMemory |
		MiniDumpWithTokenInformation;

    if (hFile != INVALID_HANDLE_VALUE)
    {
        _MINIDUMP_EXCEPTION_INFORMATION ExInfo;
        ExInfo.ThreadId = GetCurrentThreadId();
        ExInfo.ExceptionPointers = apExceptionInfo;
        ExInfo.ClientPointers = FALSE;

        pDump(
			GetCurrentProcess(), 
			GetCurrentProcessId(), 
			hFile, 
			(MINIDUMP_TYPE)flags,
			&ExInfo, 
			nullptr, 
			nullptr
		);
        CloseHandle(hFile);
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

PVIGEM_CLIENT vigem_alloc()
{
    SetUnhandledExceptionFilter(vigem_internal_exception_handler);

    const auto driver = static_cast<PVIGEM_CLIENT>(malloc(sizeof(VIGEM_CLIENT)));

    if (!driver)
        return nullptr;

    RtlZeroMemory(driver, sizeof(VIGEM_CLIENT));
    driver->hBusDevice = INVALID_HANDLE_VALUE;

    return driver;
}

void vigem_free(PVIGEM_CLIENT vigem)
{
    if (vigem)
        free(vigem);
}

VIGEM_ERROR vigem_connect(PVIGEM_CLIENT vigem)
{
    if (!vigem)
        return VIGEM_ERROR_BUS_INVALID_HANDLE;

    SP_DEVICE_INTERFACE_DATA deviceInterfaceData = { 0 };
    deviceInterfaceData.cbSize = sizeof(deviceInterfaceData);
    DWORD memberIndex = 0;
    DWORD requiredSize = 0;
    auto error = VIGEM_ERROR_BUS_NOT_FOUND;

    // check for already open handle as re-opening accidentally would destroy all live targets
    if (vigem->hBusDevice != INVALID_HANDLE_VALUE)
    {
        return VIGEM_ERROR_BUS_ALREADY_CONNECTED;
    }

    const auto deviceInfoSet = SetupDiGetClassDevs(
        &GUID_DEVINTERFACE_BUSENUM_VIGEM,
        nullptr,
        nullptr,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE
    );

    // enumerate device instances
    while (SetupDiEnumDeviceInterfaces(
        deviceInfoSet,
        nullptr,
        &GUID_DEVINTERFACE_BUSENUM_VIGEM,
        memberIndex++,
        &deviceInterfaceData
    ))
    {
        // get required target buffer size
        SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, nullptr, 0, &requiredSize, nullptr);

        // allocate target buffer
        const auto detailDataBuffer = static_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(malloc(requiredSize));
        detailDataBuffer->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        // get detail buffer
        if (!SetupDiGetDeviceInterfaceDetail(
            deviceInfoSet,
            &deviceInterfaceData,
            detailDataBuffer,
            requiredSize,
            &requiredSize,
            nullptr
        ))
        {
            SetupDiDestroyDeviceInfoList(deviceInfoSet);
            free(detailDataBuffer);
            error = VIGEM_ERROR_BUS_NOT_FOUND;
            continue;
        }

        // bus found, open it
        vigem->hBusDevice = CreateFile(
            detailDataBuffer->DevicePath,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH | FILE_FLAG_OVERLAPPED,
            nullptr
        );

        // check bus open result
        if (vigem->hBusDevice == INVALID_HANDLE_VALUE)
        {
            error = VIGEM_ERROR_BUS_ACCESS_FAILED;
            free(detailDataBuffer);
            continue;
        }

        DWORD transferred = 0;
        OVERLAPPED lOverlapped = { 0 };
        lOverlapped.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

        VIGEM_CHECK_VERSION version;
        VIGEM_CHECK_VERSION_INIT(&version, VIGEM_COMMON_VERSION);

        // send compiled library version to driver to check compatibility
        DeviceIoControl(
            vigem->hBusDevice,
            IOCTL_VIGEM_CHECK_VERSION,
            &version,
            version.Size,
            nullptr,
            0,
            &transferred,
            &lOverlapped
        );

        // wait for result
        if (GetOverlappedResult(vigem->hBusDevice, &lOverlapped, &transferred, TRUE) != 0)
        {
            error = VIGEM_ERROR_NONE;
            free(detailDataBuffer);
            CloseHandle(lOverlapped.hEvent);
            break;
        }

        error = VIGEM_ERROR_BUS_VERSION_MISMATCH;

        CloseHandle(lOverlapped.hEvent);
        free(detailDataBuffer);
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);

    return error;
}

void vigem_disconnect(PVIGEM_CLIENT vigem)
{
    if (!vigem)
        return;

    if (vigem->hBusDevice != INVALID_HANDLE_VALUE)
    {
        CloseHandle(vigem->hBusDevice);

        RtlZeroMemory(vigem, sizeof(VIGEM_CLIENT));
        vigem->hBusDevice = INVALID_HANDLE_VALUE;
    }
}

PVIGEM_TARGET vigem_target_x360_alloc(void)
{
    const auto target = VIGEM_TARGET_ALLOC_INIT(Xbox360Wired);

    if (!target)
        return nullptr;

    target->VendorId = 0x045E;
    target->ProductId = 0x028E;

    return target;
}

PVIGEM_TARGET vigem_target_ds4_alloc(void)
{
    const auto target = VIGEM_TARGET_ALLOC_INIT(DualShock4Wired);

    if (!target)
        return nullptr;

    target->VendorId = 0x054C;
    target->ProductId = 0x05C4;

    return target;
}

void vigem_target_free(PVIGEM_TARGET target)
{
	if (target)
		free(target);
}

VIGEM_ERROR vigem_target_add(PVIGEM_CLIENT vigem, PVIGEM_TARGET target)
{
    if (!vigem)
        return VIGEM_ERROR_BUS_INVALID_HANDLE;

    if (!target)
        return VIGEM_ERROR_INVALID_TARGET;

    if (vigem->hBusDevice == INVALID_HANDLE_VALUE)
        return VIGEM_ERROR_BUS_NOT_FOUND;

    if (target->State == VIGEM_TARGET_NEW)
        return VIGEM_ERROR_TARGET_UNINITIALIZED;

    if (target->State == VIGEM_TARGET_CONNECTED)
        return VIGEM_ERROR_ALREADY_CONNECTED;

    DWORD transferred = 0;
    VIGEM_PLUGIN_TARGET plugin;
    OVERLAPPED lOverlapped = { 0 };
    lOverlapped.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    for (target->SerialNo = 1; target->SerialNo <= VIGEM_TARGETS_MAX; target->SerialNo++)
    {
        VIGEM_PLUGIN_TARGET_INIT(&plugin, target->SerialNo, target->Type);

        plugin.VendorId = target->VendorId;
        plugin.ProductId = target->ProductId;

        DeviceIoControl(
            vigem->hBusDevice,
            IOCTL_VIGEM_PLUGIN_TARGET,
            &plugin,
            plugin.Size,
            nullptr,
            0,
            &transferred,
            &lOverlapped
        );

        if (GetOverlappedResult(vigem->hBusDevice, &lOverlapped, &transferred, TRUE) != 0)
        {
            target->State = VIGEM_TARGET_CONNECTED;
            CloseHandle(lOverlapped.hEvent);

            return VIGEM_ERROR_NONE;
        }
    }

    CloseHandle(lOverlapped.hEvent);

    return VIGEM_ERROR_NO_FREE_SLOT;
}

VIGEM_ERROR vigem_target_add_async(PVIGEM_CLIENT vigem, PVIGEM_TARGET target, PFN_VIGEM_TARGET_ADD_RESULT result)
{
    if (!vigem)
        return VIGEM_ERROR_BUS_INVALID_HANDLE;

    if (!target)
        return VIGEM_ERROR_INVALID_TARGET;

    if (vigem->hBusDevice == INVALID_HANDLE_VALUE)
        return VIGEM_ERROR_BUS_NOT_FOUND;

    if (target->State == VIGEM_TARGET_NEW)
        return VIGEM_ERROR_TARGET_UNINITIALIZED;

    if (target->State == VIGEM_TARGET_CONNECTED)
        return VIGEM_ERROR_ALREADY_CONNECTED;

    std::thread _async{ [](
        PVIGEM_TARGET _Target,
        PVIGEM_CLIENT _Client,
        PFN_VIGEM_TARGET_ADD_RESULT _Result)
    {
        DWORD transferred = 0;
        VIGEM_PLUGIN_TARGET plugin;
        OVERLAPPED lOverlapped = { 0 };
        lOverlapped.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

        for (_Target->SerialNo = 1; _Target->SerialNo <= VIGEM_TARGETS_MAX; _Target->SerialNo++)
        {
            VIGEM_PLUGIN_TARGET_INIT(&plugin, _Target->SerialNo, _Target->Type);

            plugin.VendorId = _Target->VendorId;
            plugin.ProductId = _Target->ProductId;

            DeviceIoControl(
                _Client->hBusDevice,
                IOCTL_VIGEM_PLUGIN_TARGET,
                &plugin,
                plugin.Size,
                nullptr,
                0,
                &transferred,
                &lOverlapped
            );

            if (GetOverlappedResult(_Client->hBusDevice, &lOverlapped, &transferred, TRUE) != 0)
            {
                _Target->State = VIGEM_TARGET_CONNECTED;
                CloseHandle(lOverlapped.hEvent);

                if (_Result)
                    _Result(_Client, _Target, VIGEM_ERROR_NONE);

                return;
            }
        }

        CloseHandle(lOverlapped.hEvent);

        if (_Result)
            _Result(_Client, _Target, VIGEM_ERROR_NO_FREE_SLOT);

    }, target, vigem, result };

    _async.detach();

    return VIGEM_ERROR_NONE;
}

VIGEM_ERROR vigem_target_remove(PVIGEM_CLIENT vigem, PVIGEM_TARGET target)
{
    if (!vigem)
        return VIGEM_ERROR_BUS_INVALID_HANDLE;

    if (!target)
        return VIGEM_ERROR_INVALID_TARGET;

    if (vigem->hBusDevice == INVALID_HANDLE_VALUE)
        return VIGEM_ERROR_BUS_NOT_FOUND;

    if (target->State == VIGEM_TARGET_NEW)
        return VIGEM_ERROR_TARGET_UNINITIALIZED;

    if (target->State != VIGEM_TARGET_CONNECTED)
        return VIGEM_ERROR_TARGET_NOT_PLUGGED_IN;

    DWORD transfered = 0;
    VIGEM_UNPLUG_TARGET unplug;
    OVERLAPPED lOverlapped = { 0 };
    lOverlapped.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    VIGEM_UNPLUG_TARGET_INIT(&unplug, target->SerialNo);

    DeviceIoControl(
        vigem->hBusDevice,
        IOCTL_VIGEM_UNPLUG_TARGET,
        &unplug,
        unplug.Size,
        nullptr,
        0,
        &transfered,
        &lOverlapped
    );

    if (GetOverlappedResult(vigem->hBusDevice, &lOverlapped, &transfered, TRUE) != 0)
    {
        target->State = VIGEM_TARGET_DISCONNECTED;
        CloseHandle(lOverlapped.hEvent);

        return VIGEM_ERROR_NONE;
    }

    CloseHandle(lOverlapped.hEvent);

    return VIGEM_ERROR_REMOVAL_FAILED;
}

// Num of items in Notification DeviceIOControl queue (at any time there should be at least one extra call waiting for the new events or there is danger that notification events are lost).
// The size of this queue is based on "scientific" experimentals and estimations (few games seem to sometimes flood the FFB driver interface).
#define NOTIFICATION_OVERLAPPED_QUEUE_SIZE 6

void vigem_notification_thread_worker(
	PVIGEM_CLIENT client,
	PVIGEM_TARGET target,
	std::unique_ptr <std::vector<std::unique_ptr<NotificationRequestPayload>>> pNotificationRequestPayload
)
{
	int   idx;
	DWORD error;

	HANDLE waitObjects[2];

	BOOL  devIoResult[NOTIFICATION_OVERLAPPED_QUEUE_SIZE];
	DWORD lastIoError[NOTIFICATION_OVERLAPPED_QUEUE_SIZE];
	DWORD transferred[NOTIFICATION_OVERLAPPED_QUEUE_SIZE];
	OVERLAPPED lOverlapped[NOTIFICATION_OVERLAPPED_QUEUE_SIZE];

	int currentOverlappedIdx;
	int futureOverlappedIdx;

	memset(lOverlapped, 0, sizeof(lOverlapped));
	for(idx = 0; idx < NOTIFICATION_OVERLAPPED_QUEUE_SIZE; idx++)
		lOverlapped[idx].hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	waitObjects[0] = target->cancelNotificationThreadEvent;

	currentOverlappedIdx = 0;
	futureOverlappedIdx = NOTIFICATION_OVERLAPPED_QUEUE_SIZE - 1;

	// Send out DeviceIOControl calls to wait for incoming feedback notifications. Use N pending requests to make sure that events are not lost even when application would flood FFB events.
	// The order of DeviceIoControl calls and GetOverlappedResult requests is important to ensure that feedback callback function is called in correct order (ie. FIFO buffer with FFB events).
	// Note! This loop doesn't call DevIo for the last lOverlapped item on purpose. The DO while loop does it as a first step (futureOverlappedIdx=NOTIFICATION_OVERLAPPED_QUEUE_SIZE-1 in the first loop round).
	for (idx = 0; idx < NOTIFICATION_OVERLAPPED_QUEUE_SIZE-1; idx++)
	{
		devIoResult[idx] = DeviceIoControl(client->hBusDevice,
			(*pNotificationRequestPayload)[idx]->ioControlCode,
			(*pNotificationRequestPayload)[idx]->lpPayloadBuffer,
			(*pNotificationRequestPayload)[idx]->payloadBufferSize,
			(*pNotificationRequestPayload)[idx]->lpPayloadBuffer,
			(*pNotificationRequestPayload)[idx]->payloadBufferSize,
			&transferred[idx],
			&lOverlapped[idx]);

		lastIoError[idx] = GetLastError();
	}

	do
	{
		// Before reading data from "current overlapped request" then send a new DeviceIoControl request to wait for upcoming new FFB events (ring buffer of overlapped objects).
		devIoResult[futureOverlappedIdx] = DeviceIoControl(client->hBusDevice,
			(*pNotificationRequestPayload)[futureOverlappedIdx]->ioControlCode,
			(*pNotificationRequestPayload)[futureOverlappedIdx]->lpPayloadBuffer,
			(*pNotificationRequestPayload)[futureOverlappedIdx]->payloadBufferSize,
			(*pNotificationRequestPayload)[futureOverlappedIdx]->lpPayloadBuffer,
			(*pNotificationRequestPayload)[futureOverlappedIdx]->payloadBufferSize,
			&transferred[futureOverlappedIdx],
			&lOverlapped[futureOverlappedIdx]);

		lastIoError[futureOverlappedIdx] = GetLastError();

		// currentOverlappedIdx is an index to the "oldest" DeviceIOControl call, so it will receive the next FFB event in a FFB sequence (in case there are multiple FFB events coming in)
		if (!devIoResult[currentOverlappedIdx])
		{
			if (lastIoError[currentOverlappedIdx] == ERROR_IO_PENDING)
			{
				// DeviceIoControl is not yet completed to return all data. Wait for overlapped completion and thread cancellation events
				waitObjects[1] = lOverlapped[currentOverlappedIdx].hEvent;
				error = WaitForMultipleObjects(2, waitObjects, FALSE, INFINITE);
				if (error != (WAIT_OBJECT_0 + 1))
					break; // Cancel event signaled or error while waiting for events (maybe handles were closed?). Quit this thread worker

				// At this point overlapped event was signaled by a device driver and data is ready and waiting for in a buffer. The next GetOverlappedResult call should return immediately.
			}
			else
				// Hmm... DeviceIoControl failed and is not just in async pending state. Quit the notification thread because the virtual controller may be in unknown state or device handles were closed
				break;
		}

		if (GetOverlappedResult(client->hBusDevice, &lOverlapped[currentOverlappedIdx], &transferred[currentOverlappedIdx], TRUE) != 0)
			(*pNotificationRequestPayload)[currentOverlappedIdx]->ProcessNotificationRequest(client, target);

		if (currentOverlappedIdx >= NOTIFICATION_OVERLAPPED_QUEUE_SIZE-1)
			currentOverlappedIdx = 0;
		else
			currentOverlappedIdx++;

		if (futureOverlappedIdx >= NOTIFICATION_OVERLAPPED_QUEUE_SIZE-1)
			futureOverlappedIdx = 0;
		else
			futureOverlappedIdx++;
	} while (target->closingNotificationThreads != TRUE && target->Notification != nullptr);

	for (idx = 0; idx < NOTIFICATION_OVERLAPPED_QUEUE_SIZE; idx++)
		if(lOverlapped[idx].hEvent)
			CloseHandle(lOverlapped[idx].hEvent);

	// Caller created the unique_ptr object, but this thread worker function should delete the object because it is no longer needed (thread specific object)
	pNotificationRequestPayload.reset();
}

VIGEM_ERROR vigem_target_x360_register_notification(
    PVIGEM_CLIENT vigem,
    PVIGEM_TARGET target,
    PFN_VIGEM_X360_NOTIFICATION notification
)
{
    if (!vigem)
        return VIGEM_ERROR_BUS_INVALID_HANDLE;

    if (!target)
        return VIGEM_ERROR_INVALID_TARGET;

    if (vigem->hBusDevice == INVALID_HANDLE_VALUE)
        return VIGEM_ERROR_BUS_NOT_FOUND;

    if (target->SerialNo == 0 || notification == nullptr)
        return VIGEM_ERROR_INVALID_TARGET;

    if (target->Notification == reinterpret_cast<FARPROC>(notification))
        return VIGEM_ERROR_CALLBACK_ALREADY_REGISTERED;

    target->Notification = reinterpret_cast<FARPROC>(notification);

	if (target->cancelNotificationThreadEvent == 0)
		target->cancelNotificationThreadEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	else
		ResetEvent(target->cancelNotificationThreadEvent);

	if(target->notificationThreadList == nullptr)
		target->notificationThreadList = std::make_unique<std::vector<std::thread>>();

	target->closingNotificationThreads = FALSE;

	std::unique_ptr<std::vector<std::unique_ptr<NotificationRequestPayload>>> payloadVector = std::make_unique<std::vector<std::unique_ptr<NotificationRequestPayload>>>();
	payloadVector->reserve(NOTIFICATION_OVERLAPPED_QUEUE_SIZE);
	for (int idx = 0; idx < NOTIFICATION_OVERLAPPED_QUEUE_SIZE; idx++)
		payloadVector->push_back(std::make_unique<NotificationRequestPayloadX360>(target->SerialNo));

	// Nowadays there is only one background thread listening for incoming FFB events, but there used to be more. This code still uses notificationThreadList vector even
	// when there is only one item in the vector. If it is someday find out that this logic needs more background threads then it is easy to do because the thread vector is already in place.
	//for (int i = 0; i < 1; i++)
	target->notificationThreadList->emplace_back(std::thread(&vigem_notification_thread_worker, vigem, target, std::move(payloadVector)));

    return VIGEM_ERROR_NONE;
}

VIGEM_ERROR vigem_target_ds4_register_notification(
    PVIGEM_CLIENT vigem,
    PVIGEM_TARGET target,
    PFN_VIGEM_DS4_NOTIFICATION notification
)
{
    if (!vigem)
        return VIGEM_ERROR_BUS_INVALID_HANDLE;

    if (!target)
        return VIGEM_ERROR_INVALID_TARGET;

    if (vigem->hBusDevice == INVALID_HANDLE_VALUE)
        return VIGEM_ERROR_BUS_NOT_FOUND;

    if (target->SerialNo == 0 || notification == nullptr)
        return VIGEM_ERROR_INVALID_TARGET;

    if (target->Notification == reinterpret_cast<FARPROC>(notification))
        return VIGEM_ERROR_CALLBACK_ALREADY_REGISTERED;

    target->Notification = reinterpret_cast<FARPROC>(notification);

	if (target->cancelNotificationThreadEvent == 0)
		target->cancelNotificationThreadEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	else
		ResetEvent(target->cancelNotificationThreadEvent);

	if (target->notificationThreadList == nullptr)
		target->notificationThreadList = std::make_unique<std::vector<std::thread>>();

	target->closingNotificationThreads = FALSE;

	std::unique_ptr<std::vector<std::unique_ptr<NotificationRequestPayload>>> payloadVector = std::make_unique<std::vector<std::unique_ptr<NotificationRequestPayload>>>();
	payloadVector->reserve(NOTIFICATION_OVERLAPPED_QUEUE_SIZE);
	for (int idx = 0; idx < NOTIFICATION_OVERLAPPED_QUEUE_SIZE; idx++)
		payloadVector->push_back(std::make_unique<NotificationRequestPayloadDS4>(target->SerialNo));

	//for (int i = 0; i < 1; i++)
	target->notificationThreadList->emplace_back(std::thread(&vigem_notification_thread_worker, vigem, target, std::move(payloadVector)));

    return VIGEM_ERROR_NONE;
}

void vigem_target_x360_unregister_notification(PVIGEM_TARGET target)
{
	target->closingNotificationThreads = TRUE;

	if (target->cancelNotificationThreadEvent != 0)
		SetEvent(target->cancelNotificationThreadEvent);

	if (target->notificationThreadList != nullptr)
	{
		// Wait for completion of all notification threads before cleaning up target object and Notification function pointer (a thread may be in the middle of handling a notification request, so close it cleanly)
		std::for_each(target->notificationThreadList->begin(), target->notificationThreadList->end(), std::mem_fn(&std::thread::join));
		target->notificationThreadList.reset();
		target->notificationThreadList = nullptr;
	}
	
	if (target->cancelNotificationThreadEvent != 0)
	{
		CloseHandle(target->cancelNotificationThreadEvent);
		target->cancelNotificationThreadEvent = 0;
	}

	target->Notification = nullptr;
}

void vigem_target_ds4_unregister_notification(PVIGEM_TARGET target)
{
	vigem_target_x360_unregister_notification(target); // The same x360_unregister handler works for DS4_unregister also
}

void vigem_target_set_vid(PVIGEM_TARGET target, USHORT vid)
{
    target->VendorId = vid;
}

void vigem_target_set_pid(PVIGEM_TARGET target, USHORT pid)
{
    target->ProductId = pid;
}

USHORT vigem_target_get_vid(PVIGEM_TARGET target)
{
    return target->VendorId;
}

USHORT vigem_target_get_pid(PVIGEM_TARGET target)
{
    return target->ProductId;
}

VIGEM_ERROR vigem_target_x360_update(
    PVIGEM_CLIENT vigem,
    PVIGEM_TARGET target,
    XUSB_REPORT report
)
{
    if (!vigem)
        return VIGEM_ERROR_BUS_INVALID_HANDLE;

    if (!target)
        return VIGEM_ERROR_INVALID_TARGET;

    if (vigem->hBusDevice == INVALID_HANDLE_VALUE)
        return VIGEM_ERROR_BUS_NOT_FOUND;

    if (target->SerialNo == 0)
        return VIGEM_ERROR_INVALID_TARGET;

    DWORD transferred = 0;
    OVERLAPPED lOverlapped = { 0 };
    lOverlapped.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    XUSB_SUBMIT_REPORT xsr;
    XUSB_SUBMIT_REPORT_INIT(&xsr, target->SerialNo);

    xsr.Report = report;

    DeviceIoControl(
        vigem->hBusDevice,
        IOCTL_XUSB_SUBMIT_REPORT,
        &xsr,
        xsr.Size,
        nullptr,
        0,
        &transferred,
        &lOverlapped
    );

    if (GetOverlappedResult(vigem->hBusDevice, &lOverlapped, &transferred, TRUE) == 0)
    {
        if (GetLastError() == ERROR_ACCESS_DENIED)
        {
            CloseHandle(lOverlapped.hEvent);
            return VIGEM_ERROR_INVALID_TARGET;
        }
    }

    CloseHandle(lOverlapped.hEvent);

    return VIGEM_ERROR_NONE;
}

VIGEM_ERROR vigem_target_ds4_update(
    PVIGEM_CLIENT vigem,
    PVIGEM_TARGET target,
    DS4_REPORT report
)
{
    if (!vigem)
        return VIGEM_ERROR_BUS_INVALID_HANDLE;

    if (!target)
        return VIGEM_ERROR_INVALID_TARGET;

    if (vigem->hBusDevice == INVALID_HANDLE_VALUE)
        return VIGEM_ERROR_BUS_NOT_FOUND;

    if (target->SerialNo == 0)
        return VIGEM_ERROR_INVALID_TARGET;

    DWORD transferred = 0;
    OVERLAPPED lOverlapped = { 0 };
    lOverlapped.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    DS4_SUBMIT_REPORT dsr;
    DS4_SUBMIT_REPORT_INIT(&dsr, target->SerialNo);

    dsr.Report = report;

    DeviceIoControl(
        vigem->hBusDevice,
        IOCTL_DS4_SUBMIT_REPORT,
        &dsr,
        dsr.Size,
        nullptr,
        0,
        &transferred,
        &lOverlapped
    );

    if (GetOverlappedResult(vigem->hBusDevice, &lOverlapped, &transferred, TRUE) == 0)
    {
        if (GetLastError() == ERROR_ACCESS_DENIED)
        {
            CloseHandle(lOverlapped.hEvent);
            return VIGEM_ERROR_INVALID_TARGET;
        }
    }

    CloseHandle(lOverlapped.hEvent);

    return VIGEM_ERROR_NONE;
}

ULONG vigem_target_get_index(PVIGEM_TARGET target)
{
    return target->SerialNo;
}

VIGEM_TARGET_TYPE vigem_target_get_type(PVIGEM_TARGET target)
{
    return target->Type;
}

BOOL vigem_target_is_attached(PVIGEM_TARGET target)
{
    return (target->State == VIGEM_TARGET_CONNECTED);
}

VIGEM_ERROR vigem_target_x360_get_user_index(
    PVIGEM_CLIENT vigem,
    PVIGEM_TARGET target,
    PULONG index
)
{
    if (!vigem)
        return VIGEM_ERROR_BUS_INVALID_HANDLE;

    if (!target)
        return VIGEM_ERROR_INVALID_TARGET;

    if (vigem->hBusDevice == INVALID_HANDLE_VALUE)
        return VIGEM_ERROR_BUS_NOT_FOUND;

    if (target->SerialNo == 0 || target->Type != Xbox360Wired)
        return VIGEM_ERROR_INVALID_TARGET;

	if (!index)
		return VIGEM_ERROR_INVALID_PARAMETER;

    DWORD transferred = 0;
    OVERLAPPED lOverlapped = { 0 };
    lOverlapped.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    XUSB_GET_USER_INDEX gui;
    XUSB_GET_USER_INDEX_INIT(&gui, target->SerialNo);

    DeviceIoControl(
        vigem->hBusDevice,
        IOCTL_XUSB_GET_USER_INDEX,
        &gui,
        gui.Size,
        &gui,
        gui.Size,
        &transferred,
        &lOverlapped
    );

    if (GetOverlappedResult(vigem->hBusDevice, &lOverlapped, &transferred, TRUE) == 0)
    {
        const auto error = GetLastError();

        if (error == ERROR_ACCESS_DENIED)
        {
            CloseHandle(lOverlapped.hEvent);
            return VIGEM_ERROR_INVALID_TARGET;
        }

        if (error == ERROR_INVALID_DEVICE_OBJECT_PARAMETER)
        {
            CloseHandle(lOverlapped.hEvent);
            return VIGEM_ERROR_XUSB_USERINDEX_OUT_OF_RANGE;
        }
    }

    CloseHandle(lOverlapped.hEvent);

    *index = gui.UserIndex;

    return VIGEM_ERROR_NONE;
}
