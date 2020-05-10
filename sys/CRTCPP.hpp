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
