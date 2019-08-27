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


#pragma once

//
// TODO: this is... not optimal. Improve in the future.
// 
#define VIGEM_TARGETS_MAX   USHRT_MAX


//
// Represents a driver connection object.
// 
typedef struct _VIGEM_CLIENT_T
{
    HANDLE hBusDevice;

} VIGEM_CLIENT;

//
// Represents the (connection) state of a target device object.
// 
typedef enum _VIGEM_TARGET_STATE
{
    VIGEM_TARGET_NEW,
    VIGEM_TARGET_INITIALIZED,
    VIGEM_TARGET_CONNECTED,
    VIGEM_TARGET_DISCONNECTED
} VIGEM_TARGET_STATE, *PVIGEM_TARGET_STATE;



//
// Represents a virtual gamepad object.
// 
typedef struct _VIGEM_TARGET_T
{
    ULONG Size;
    ULONG SerialNo;
    VIGEM_TARGET_STATE State;
    USHORT VendorId;
    USHORT ProductId;
    VIGEM_TARGET_TYPE Type;
    FARPROC Notification;

	bool closingNotificationThreads;
	HANDLE cancelNotificationThreadEvent;
	std::unique_ptr<std::vector<std::thread>> notificationThreadList;
} VIGEM_TARGET;
