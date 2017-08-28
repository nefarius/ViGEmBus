/*
MIT License

Copyright (c) 2017 Benjamin "Nefarius" Höglinger

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


#ifndef ViGEmClient_h__
#define ViGEmClient_h__

#include "ViGEmBusShared.h"


typedef enum _VIGEM_ERRORS
{
    VIGEM_ERROR_NONE = 0x20000000,
    VIGEM_ERROR_BUS_NOT_FOUND = 0xE0000001,
    VIGEM_ERROR_NO_FREE_SLOT = 0xE0000002,
    VIGEM_ERROR_INVALID_TARGET = 0xE0000003,
    VIGEM_ERROR_REMOVAL_FAILED = 0xE0000004,
    VIGEM_ERROR_ALREADY_CONNECTED = 0xE0000005,
    VIGEM_ERROR_TARGET_UNINITIALIZED = 0xE0000006,
    VIGEM_ERROR_TARGET_NOT_PLUGGED_IN = 0xE0000007,
    VIGEM_ERROR_BUS_VERSION_MISMATCH = 0xE0000008,
    VIGEM_ERROR_BUS_ACCESS_FAILED = 0xE0000009,
    VIGEM_ERROR_CALLBACK_ALREADY_REGISTERED = 0xE0000010,
    VIGEM_ERROR_CALLBACK_NOT_FOUND = 0xE0000011,
    VIGEM_ERROR_BUS_ALREADY_CONNECTED = 0xE0000012
} VIGEM_ERROR;

#define VIGEM_SUCCESS(_val_) (_val_ == VIGEM_ERROR_NONE)


typedef struct _VIGEM_CLIENT_T *PVIGEM_CLIENT;
typedef struct _VIGEM_TARGET_T *PVIGEM_TARGET;

typedef VOID(CALLBACK* PVIGEM_X360_NOTIFICATION)(
    PVIGEM_TARGET Target,
    UCHAR LargeMotor,
    UCHAR SmallMotor,
    UCHAR LedNumber);

typedef VOID(CALLBACK* PVIGEM_DS4_NOTIFICATION)(
    PVIGEM_TARGET Target,
    UCHAR LargeMotor,
    UCHAR SmallMotor,
    DS4_LIGHTBAR_COLOR LightbarColor);


PVIGEM_CLIENT vigem_alloc(void);

void vigem_free(PVIGEM_CLIENT vigem);

VIGEM_ERROR vigem_connect(PVIGEM_CLIENT vigem);

void vigem_disconnect(PVIGEM_CLIENT vigem);

PVIGEM_TARGET vigem_target_x360_alloc(void);

PVIGEM_TARGET vigem_target_ds4_alloc(void);

void vigem_target_free(PVIGEM_TARGET target);

VIGEM_ERROR vigem_target_add(PVIGEM_CLIENT vigem, PVIGEM_TARGET target);

VIGEM_ERROR vigem_target_remove(PVIGEM_CLIENT vigem, PVIGEM_TARGET target);

VIGEM_ERROR vigem_target_x360_register_notification(PVIGEM_CLIENT vigem, PVIGEM_TARGET target, PVIGEM_X360_NOTIFICATION notification);

VIGEM_ERROR vigem_target_ds4_register_notification(PVIGEM_CLIENT vigem, PVIGEM_TARGET target, PVIGEM_DS4_NOTIFICATION notification);

void vigem_target_x360_unregister_notification(PVIGEM_TARGET target);

void vigem_target_ds4_unregister_notification(PVIGEM_TARGET target);

void vigem_target_set_vid(PVIGEM_TARGET target, USHORT vid);

void vigem_target_set_pid(PVIGEM_TARGET target, USHORT pid);

VIGEM_ERROR vigem_target_x360_update(PVIGEM_CLIENT vigem, PVIGEM_TARGET target, XUSB_REPORT report);

VIGEM_ERROR vigem_target_ds4_update(PVIGEM_CLIENT vigem, PVIGEM_TARGET target, DS4_REPORT report);

ULONG vigem_target_get_index(PVIGEM_TARGET target);

#endif // ViGEmClient_h__
