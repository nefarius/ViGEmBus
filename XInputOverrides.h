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

typedef enum _XINPUT_GAMEPAD_OVERRIDES
{
    XINPUT_GAMEPAD_OVERRIDE_DPAD_UP             = 0x00000001,
    XINPUT_GAMEPAD_OVERRIDE_DPAD_DOWN           = 0x00000002,
    XINPUT_GAMEPAD_OVERRIDE_DPAD_LEFT           = 0x00000004,
    XINPUT_GAMEPAD_OVERRIDE_DPAD_RIGHT          = 0x00000008,
    XINPUT_GAMEPAD_OVERRIDE_START               = 0x00000010,
    XINPUT_GAMEPAD_OVERRIDE_BACK                = 0x00000020,
    XINPUT_GAMEPAD_OVERRIDE_LEFT_THUMB          = 0x00000040,
    XINPUT_GAMEPAD_OVERRIDE_RIGHT_THUMB         = 0x00000080,
    XINPUT_GAMEPAD_OVERRIDE_LEFT_SHOULDER       = 0x00000100,
    XINPUT_GAMEPAD_OVERRIDE_RIGHT_SHOULDER      = 0x00000200,
    XINPUT_GAMEPAD_OVERRIDE_A                   = 0x00001000,
    XINPUT_GAMEPAD_OVERRIDE_B                   = 0x00002000,
    XINPUT_GAMEPAD_OVERRIDE_X                   = 0x00004000,
    XINPUT_GAMEPAD_OVERRIDE_Y                   = 0x00008000,
    XINPUT_GAMEPAD_OVERRIDE_LEFT_TRIGGER        = 0x00010000,
    XINPUT_GAMEPAD_OVERRIDE_RIGHT_TRIGGER       = 0x00020000,
    XINPUT_GAMEPAD_OVERRIDE_LEFT_THUMB_X        = 0x00040000,
    XINPUT_GAMEPAD_OVERRIDE_LEFT_THUMB_Y        = 0x00080000,
    XINPUT_GAMEPAD_OVERRIDE_RIGHT_THUMB_X       = 0x00100000,
    XINPUT_GAMEPAD_OVERRIDE_RIGHT_THUMB_Y       = 0x00200000
} XINPUT_GAMEPAD_OVERRIDES, *PXINPUT_GAMEPAD_OVERRIDES;

