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

//
// Uncomment to compile in crash dump handler
// 
//#define VIGEM_USE_CRASH_HANDLER


#ifdef VIGEM_USE_CRASH_HANDLER
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
#endif


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
    return target;
}

#ifdef VIGEM_USE_CRASH_HANDLER
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
#endif

PVIGEM_CLIENT vigem_alloc()
{
#ifdef VIGEM_USE_CRASH_HANDLER
    SetUnhandledExceptionFilter(vigem_internal_exception_handler);
#endif

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
    VIGEM_ERROR error = VIGEM_ERROR_NO_FREE_SLOT;
    DWORD transferred = 0;
    VIGEM_PLUGIN_TARGET plugin;
    VIGEM_WAIT_DEVICE_READY devReady;
    OVERLAPPED olPlugIn = { 0 };
    olPlugIn.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    OVERLAPPED olWait = { 0 };
    olWait.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    do {
        if (!vigem)
        {
            error = VIGEM_ERROR_BUS_INVALID_HANDLE;
        	break;
        }

        if (!target)
        {
            error = VIGEM_ERROR_INVALID_TARGET;
        	break;
        }

        if (vigem->hBusDevice == INVALID_HANDLE_VALUE)
        {
            error = VIGEM_ERROR_BUS_NOT_FOUND;
        	break;
        }

        if (target->State == VIGEM_TARGET_NEW)
        {
            error = VIGEM_ERROR_TARGET_UNINITIALIZED;
        	break;
        }

        if (target->State == VIGEM_TARGET_CONNECTED)
        {
            error = VIGEM_ERROR_ALREADY_CONNECTED;
        	break;
        }       

    	//
    	// TODO: this is mad stupid, redesign, so that the bus fills the assigned slot
    	// 
        for (target->SerialNo = 1; target->SerialNo <= VIGEM_TARGETS_MAX; target->SerialNo++)
        {
	        VIGEM_PLUGIN_TARGET_INIT(&plugin, target->SerialNo, target->Type);

	        plugin.VendorId = target->VendorId;
	        plugin.ProductId = target->ProductId;

        	/*
        	 * Request plugin of device. This is an inherently asynchronous operation,
        	 * which is addressed differently through the history of the driver design.
        	 * Pre-v1.17 this request was kept pending until the child was deemed operational
        	 * which unfortunately causes synchronization issues on some systems.
        	 * Starting with v1.17 "waiting" for full power-up is done with an additional 
        	 * IOCTL that is sent immediately after and kept pending until the driver
        	 * reports that the device can receive report updates. The following section
        	 * and error handling is designed to achieve transparent backwards compatibility
        	 * to not break applications using the pre-v1.17 client SDK. This is not a 100%
        	 * perfect and can cause other functions to fail if called too soon but
        	 * hopefully the applications will just ignore these errors and retry ;)
        	 */
	        DeviceIoControl(
		        vigem->hBusDevice,
		        IOCTL_VIGEM_PLUGIN_TARGET,
		        &plugin,
		        plugin.Size,
		        nullptr,
		        0,
		        &transferred,
		        &olPlugIn
	        );

        	//
        	// This should return fairly immediately >=v1.17
        	// 
	        if (GetOverlappedResult(vigem->hBusDevice, &olPlugIn, &transferred, TRUE) != 0)
	        {
	        	/*
	        	 * This function is announced to be blocking/synchronous, a concept that 
	        	 * doesn't reflect the way the bus driver/PNP manager bring child devices
	        	 * to life. Therefore, we send another IOCTL which will be kept pending 
	        	 * until the bus driver has been notified that the child device has
	        	 * reached a state that is deemed operational. This request is only 
	        	 * supported on drivers v1.17 or higher, so gracefully cause errors
	        	 * of this call as a potential success and keep the device plugged in.
	        	 */
		        VIGEM_WAIT_DEVICE_READY_INIT(&devReady, plugin.SerialNo);

		        DeviceIoControl(
			        vigem->hBusDevice,
			        IOCTL_VIGEM_WAIT_DEVICE_READY,
			        &devReady,
			        devReady.Size,
			        nullptr,
			        0,
			        &transferred,
			        &olWait
		        );

		        if (GetOverlappedResult(vigem->hBusDevice, &olWait, &transferred, TRUE) != 0)
		        {
			        target->State = VIGEM_TARGET_CONNECTED;

			        error = VIGEM_ERROR_NONE;
			        break;
		        }
	        	
		        //
		        // Backwards compatibility with version pre-1.17, where this IOCTL doesn't exist
		        // 
		        if (GetLastError() == ERROR_INVALID_PARAMETER)
		        {
			        target->State = VIGEM_TARGET_CONNECTED;

			        error = VIGEM_ERROR_NONE;
			        break;
		        }

		        //
		        // Don't leave device connected if the wait call failed
		        // 
		        error = vigem_target_remove(vigem, target);
		        break;
	        }
        }
    } while (false);

    if (olPlugIn.hEvent)
	    CloseHandle(olPlugIn.hEvent);

    if (olWait.hEvent)
	    CloseHandle(olWait.hEvent);

    return error;
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

	std::thread _async{
		[](
		PVIGEM_TARGET _Target,
		PVIGEM_CLIENT _Client,
		PFN_VIGEM_TARGET_ADD_RESULT _Result)
		{
			const auto error = vigem_target_add(_Client, _Target);

			_Result(_Client, _Target, error);
		},
		target, vigem, result
	};

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

VIGEM_ERROR vigem_target_x360_register_notification(
    PVIGEM_CLIENT vigem,
    PVIGEM_TARGET target,
    PFN_VIGEM_X360_NOTIFICATION notification,
    LPVOID userData
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
    target->NotificationUserData = userData;

    if (target->cancelNotificationThreadEvent == nullptr)
	    target->cancelNotificationThreadEvent = CreateEvent(
            nullptr, 
            TRUE, 
            FALSE, 
            nullptr
        );
    else
	    ResetEvent(target->cancelNotificationThreadEvent);

    std::thread _async{
	    [](
	    PVIGEM_TARGET _Target,
	    PVIGEM_CLIENT _Client,
	    LPVOID _UserData)
	    {
		    DWORD transferred = 0;
		    OVERLAPPED lOverlapped = {0};
		    lOverlapped.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

		    XUSB_REQUEST_NOTIFICATION xrn;
		    XUSB_REQUEST_NOTIFICATION_INIT(&xrn, _Target->SerialNo);

		    do
		    {
			    DeviceIoControl(
				    _Client->hBusDevice,
				    IOCTL_XUSB_REQUEST_NOTIFICATION,
				    &xrn,
				    xrn.Size,
				    &xrn,
				    xrn.Size,
				    &transferred,
				    &lOverlapped
			    );

			    if (GetOverlappedResult(_Client->hBusDevice, &lOverlapped, &transferred, TRUE) != 0)
			    {
				    if (_Target->Notification == nullptr)
				    {
					    CloseHandle(lOverlapped.hEvent);
					    return;
				    }

				    reinterpret_cast<PFN_VIGEM_X360_NOTIFICATION>(_Target->Notification)(
					    _Client, _Target, xrn.LargeMotor, xrn.SmallMotor, xrn.LedNumber, _UserData
				    );

				    continue;
			    }

			    if (GetLastError() == ERROR_ACCESS_DENIED || GetLastError() == ERROR_OPERATION_ABORTED)
			    {
				    CloseHandle(lOverlapped.hEvent);
				    return;
			    }
		    }
		    while (TRUE);
	    },
	    target, vigem, userData
    };

    _async.detach();
	
    return VIGEM_ERROR_NONE;
}

VIGEM_ERROR vigem_target_ds4_register_notification(
    PVIGEM_CLIENT vigem,
    PVIGEM_TARGET target,
    PFN_VIGEM_DS4_NOTIFICATION notification,
    LPVOID userData
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
    target->NotificationUserData = userData;

	if (target->cancelNotificationThreadEvent == 0)
		target->cancelNotificationThreadEvent = CreateEvent(
            nullptr, 
            TRUE, 
            FALSE,
            nullptr
        );
	else
		ResetEvent(target->cancelNotificationThreadEvent);

    std::thread _async{
	    [](
	    PVIGEM_TARGET _Target,
	    PVIGEM_CLIENT _Client,
	    PFN_VIGEM_DS4_NOTIFICATION _Notification,
	    LPVOID _UserData)
	    {
		    DWORD transferred = 0;
		    OVERLAPPED lOverlapped = {0};
		    lOverlapped.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

		    DS4_REQUEST_NOTIFICATION ds4rn;
		    DS4_REQUEST_NOTIFICATION_INIT(&ds4rn, _Target->SerialNo);

		    do
		    {
			    DeviceIoControl(
				    _Client->hBusDevice,
				    IOCTL_DS4_REQUEST_NOTIFICATION,
				    &ds4rn,
				    ds4rn.Size,
				    &ds4rn,
				    ds4rn.Size,
				    &transferred,
				    &lOverlapped
			    );

			    if (GetOverlappedResult(_Client->hBusDevice, &lOverlapped, &transferred, TRUE) != 0)
			    {
				    _Notification(
                        _Client, 
                        _Target, 
                        ds4rn.Report.LargeMotor, 
                        ds4rn.Report.SmallMotor, 
                        ds4rn.Report.LightbarColor, 
                        _UserData
                    );
				    continue;
			    }

			    if (GetLastError() == ERROR_ACCESS_DENIED || GetLastError() == ERROR_OPERATION_ABORTED)
			    {
				    CloseHandle(lOverlapped.hEvent);
				    return VIGEM_ERROR_INVALID_TARGET;
			    }
		    }
		    while (TRUE);
	    },
	    target, vigem, notification, userData
    };

    _async.detach();
	
    return VIGEM_ERROR_NONE;
}

void vigem_target_x360_unregister_notification(PVIGEM_TARGET target)
{	
	if (target->cancelNotificationThreadEvent != 0)
		SetEvent(target->cancelNotificationThreadEvent);
    
	if (target->cancelNotificationThreadEvent != nullptr)
	{
		CloseHandle(target->cancelNotificationThreadEvent);
		target->cancelNotificationThreadEvent = nullptr;
	}

	target->Notification = nullptr;
	target->NotificationUserData = nullptr;
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

VIGEM_ERROR vigem_target_ds4_update_ex(PVIGEM_CLIENT vigem, PVIGEM_TARGET target, DS4_REPORT_EX report)
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
	OVERLAPPED lOverlapped = {0};
	lOverlapped.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	DS4_SUBMIT_REPORT_EX dsr;
	DS4_SUBMIT_REPORT_EX_INIT(&dsr, target->SerialNo);

	dsr.Report = report;

	DeviceIoControl(
		vigem->hBusDevice,
		IOCTL_DS4_SUBMIT_REPORT, // Same IOCTL, just different size
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

		/*
		 * NOTE: this will not happen on v1.16 due to NTSTATUS accidentally been set
		 * as STATUS_SUCCESS when the submitted buffer size wasn't the expected one.
		 * For backwards compatibility this function will silently fail (not cause
		 * report updates) when run with the v1.16 driver. This API was introduced 
		 * with v1.17 so it won't affect existing applications built before.
		 */
		if (GetLastError() == ERROR_INVALID_PARAMETER)
		{
			CloseHandle(lOverlapped.hEvent);
			return VIGEM_ERROR_NOT_SUPPORTED;
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
