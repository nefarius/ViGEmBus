#pragma once

constexpr auto cpp_pool_tag = 'EGiV';

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
