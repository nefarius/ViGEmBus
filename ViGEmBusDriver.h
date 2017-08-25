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


// {A77BC4D5-6AF7-4E69-8DC4-6B88A6028CE6}
// ReSharper disable once CppMissingIncludeGuard
DEFINE_GUID(GUID_VIGEM_INTERFACE_STANDARD,
    0xA77BC4D5, 0x6AF7, 0x4E69, 0x8D, 0xC4, 0x6B, 0x88, 0xA6, 0x02, 0x8C, 0xE6);

#pragma once

DECLARE_GLOBAL_CONST_UNICODE_STRING(VigemNtDeviceName, L"\\Device\\ViGEmBus");
DECLARE_GLOBAL_CONST_UNICODE_STRING(VigemDosDeviceName, L"\\DosDevices\\ViGEmBus");

#include "ViGEmBusShared.h"
