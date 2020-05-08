#pragma once
#include <ntifs.h>

constexpr auto cpp_pool_tag = 'EGiV';

#ifdef _AMD64_

static void* operator new(size_t lBlockSize)
{
	return ExAllocatePoolWithTag(NonPagedPoolNx, lBlockSize, cpp_pool_tag);
}

static void operator delete(void* p)
{
	if (p == nullptr)
	{
		return;
	}
	ExFreePoolWithTag(p, cpp_pool_tag);
}

#else

static void* __CRTDECL operator new(size_t lBlockSize)
{
	return ExAllocatePoolWithTag(NonPagedPoolNx, lBlockSize, CPP_POOL_TAG);
}

static void __CRTDECL operator delete(void* p)
{
	if (!p)
	{
		return;
	}
	ExFreePoolWithTag(p, CPP_POOL_TAG);
}

#endif
