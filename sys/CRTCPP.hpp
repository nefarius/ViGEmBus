/*
* Virtual Gamepad Emulation Framework - Windows kernel-mode bus driver
*
* MIT License
*
* Copyright (c) 2016-2020 Nefarius Software Solutions e.U. and Contributors
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#pragma once

constexpr auto cpp_pool_tag = 'EGiV';

#ifdef _AMD64_

void* operator new
(
    size_t size
    )
{
    return ExAllocatePoolWithTag(NonPagedPoolNx, size, cpp_pool_tag);
}

void* operator new[]
(
    size_t size
    )
{
    return ExAllocatePoolWithTag(NonPagedPoolNx, size, cpp_pool_tag);
}

void operator delete
(
    void* what
    )
{
    if (what == nullptr)
    {
        return;
    }
	
    ExFreePoolWithTag(what, cpp_pool_tag);
}

void operator delete
(
    void* what,
    size_t size
    )
{
    UNREFERENCED_PARAMETER(size);
	
    if (what == nullptr)
    {
        return;
    }

    ExFreePoolWithTag(what, cpp_pool_tag);
}

void operator delete[]
(
    void* what,
    size_t size
    )
{
    UNREFERENCED_PARAMETER(size);
	
    if (what == nullptr)
    {
        return;
    }
	
    ExFreePoolWithTag(what, cpp_pool_tag);
}

#else

void* __CRTDECL operator new
(
    size_t size
    )
{
    return ExAllocatePoolWithTag(NonPagedPoolNx, size, cpp_pool_tag);
}

void* __CRTDECL operator new[]
(
    size_t size
    )
{
    return ExAllocatePoolWithTag(NonPagedPoolNx, size, cpp_pool_tag);
}

void __CRTDECL operator delete
(
    void* what
    )
{
    if (what == nullptr)
    {
        return;
    }

    ExFreePoolWithTag(what, cpp_pool_tag);
}

void __CRTDECL operator delete
(
    void* what,
    size_t size
    )
{
    UNREFERENCED_PARAMETER(size);

    if (what == nullptr)
    {
        return;
    }

    ExFreePoolWithTag(what, cpp_pool_tag);
}

void __CRTDECL operator delete[]
(
    void* what,
    size_t size
    )
{
    UNREFERENCED_PARAMETER(size);

    if (what == nullptr)
    {
        return;
    }

    ExFreePoolWithTag(what, cpp_pool_tag);
}

#endif
