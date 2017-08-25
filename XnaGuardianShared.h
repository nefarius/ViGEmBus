/*
MIT License

Copyright (c) 2016 Benjamin "Nefarius" Höglinger

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

#define XNA_GUARDIAN_DEVICE_PATH    TEXT("\\\\.\\XnaGuardian")

#define XINPUT_MAX_DEVICES          0x04

#define VALID_USER_INDEX(_index_)   ((_index_ >= 0) && (_index_ < XINPUT_MAX_DEVICES))

//
// Custom extensions
// 
#define XINPUT_EXT_TYPE                         0x8001
#define XINPUT_EXT_CODE                         0x801

#define IOCTL_XINPUT_EXT_OVERRIDE_GAMEPAD_STATE CTL_CODE(XINPUT_EXT_TYPE, XINPUT_EXT_CODE + 0x01, METHOD_BUFFERED, FILE_WRITE_DATA)
#define IOCTL_XINPUT_EXT_PEEK_GAMEPAD_STATE     CTL_CODE(XINPUT_EXT_TYPE, XINPUT_EXT_CODE + 0x02, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)


//
// State of the gamepad (compatible to XINPUT_GAMEPAD)
// 
typedef struct _XINPUT_GAMEPAD_STATE {
    USHORT wButtons;
    BYTE   bLeftTrigger;
    BYTE   bRightTrigger;
    SHORT  sThumbLX;
    SHORT  sThumbLY;
    SHORT  sThumbRX;
    SHORT  sThumbRY;
} XINPUT_GAMEPAD_STATE, *PXINPUT_GAMEPAD_STATE;

//
// Context data for IOCTL_XINPUT_EXT_OVERRIDE_GAMEPAD_STATE I/O control code
// 
typedef struct _XINPUT_EXT_OVERRIDE_GAMEPAD
{
    IN ULONG Size;

    IN UCHAR UserIndex;

    IN ULONG Overrides;

    IN XINPUT_GAMEPAD_STATE Gamepad;

} XINPUT_EXT_OVERRIDE_GAMEPAD, *PXINPUT_EXT_OVERRIDE_GAMEPAD;

VOID FORCEINLINE XINPUT_EXT_OVERRIDE_GAMEPAD_INIT(
    _Out_ PXINPUT_EXT_OVERRIDE_GAMEPAD OverrideGamepad,
    _In_ UCHAR UserIndex
)
{
    RtlZeroMemory(OverrideGamepad, sizeof(XINPUT_EXT_OVERRIDE_GAMEPAD));

    OverrideGamepad->Size = sizeof(XINPUT_EXT_OVERRIDE_GAMEPAD);
    OverrideGamepad->UserIndex = UserIndex;
}

//
// Context data for IOCTL_XINPUT_EXT_PEEK_GAMEPAD_STATE I/O control code
// 
typedef struct _XINPUT_EXT_PEEK_GAMEPAD
{
    IN ULONG Size;

    IN UCHAR UserIndex;

} XINPUT_EXT_PEEK_GAMEPAD, *PXINPUT_EXT_PEEK_GAMEPAD;

VOID FORCEINLINE XINPUT_EXT_PEEK_GAMEPAD_INIT(
    _Out_ PXINPUT_EXT_PEEK_GAMEPAD PeekGamepad,
    _In_ UCHAR UserIndex
)
{
    RtlZeroMemory(PeekGamepad, sizeof(XINPUT_EXT_PEEK_GAMEPAD));

    PeekGamepad->Size = sizeof(XINPUT_EXT_PEEK_GAMEPAD);
    PeekGamepad->UserIndex = UserIndex;
}


