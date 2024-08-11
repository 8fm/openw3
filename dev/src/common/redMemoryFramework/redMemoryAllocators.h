/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_ALLOCATORS_H
#define _RED_MEMORY_ALLOCATORS_H

#include "redMemoryThreads.h"
#include "redMemoryTlsfAllocator.h"
#include "redMemorySmallBlockAllocator.h"
#include "redMemoryGpuAllocator.h"
#include "redMemoryMultiAllocator.h"
#include "redMemoryDebugAllocator.h"
#include "redMemoryVirtualAllocWrapper.h"

///////////////////////////////////////////////////////////////////
// Define all allocators here
namespace Red { namespace MemoryFramework {

	typedef TLSFAllocatorBase<CMutex, ScopedMemory_NoProtection> TLSFAllocatorThreadSafe;
	typedef TLSFAllocatorBase<CSpinlock, ScopedMemory_NoProtection> TLSFAllocatorSpinlocked;
	typedef TLSFAllocatorBase<CNullMutex, ScopedMemory_NoProtection> TLSFAllocator;
	typedef TLSFAllocatorBase<CMutex, TLSFAllocatorImpl::ScopedGpuProtection> GpuDebugTLSFAllocatorThreadSafe;
	typedef TLSFAllocatorBase<CNullMutex, TLSFAllocatorImpl::ScopedGpuProtection> GpuDebugTLSFAllocator;
	typedef SmallBlockAllocator<CMutex> SmallBlockAllocatorThreadSafe;
	typedef SmallBlockAllocator<CSpinlock> SmallBlockAllocatorSpinlocked;
	typedef SmallBlockAllocator<CNullMutex> SmallBlockAllocatorNoLock;
	typedef GpuAllocator<CMutex> GpuAllocatorThreadSafe;
	typedef GpuAllocator<CNullMutex> GpuAllocatorNoLock;
	typedef VirtualAllocWrapperAllocator<CMutex> VirtualAllocWrapThreadSafe;
	typedef VirtualAllocWrapperAllocator<CSpinlock> VirtualAllocWrapSpinlocked;
	typedef VirtualAllocWrapperAllocator<CMutex> VirtualAllocWrapNoLock;

#ifdef RED_PLATFORM_ORBIS
	typedef TLSFAllocatorBase<CAdaptiveMutex, ScopedMemory_NoProtection> TLSFAllocatorAdaptiveLock;
	typedef SmallBlockAllocator<CAdaptiveMutex> SmallBlockAllocatorAdaptiveLock;
#endif

} }

#endif
