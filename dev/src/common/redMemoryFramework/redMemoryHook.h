/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_HOOK_H
#define _RED_MEMORY_HOOK_H
#pragma once

#include "redMemoryFrameworkTypes.h"

namespace Red { namespace MemoryFramework {

	// Used to add custom functionality on allocate / free per-manager. Fill in ALL the function ptrs!
	class MemoryHook
	{
	public:
		// Called just before we call into an allocator. Returns the adjusted size to be allocated.
		typedef Red::System::MemSize (*PreAllocateHook)( PoolLabel label, MemoryClass memoryClass, size_t allocSize, size_t allocAlignment );

		// Called after a successful allocation. 
		typedef void (*PostAllocateHook)( PoolLabel label, MemoryClass memoryClass, Red::System::MemSize allocatedSize, void* allocatedBuffer );

		// Called just before we reallocate from an allocator. Returns the adjusted size to be allocated.
		typedef Red::System::MemSize (*PreReallocateHook)( PoolLabel label, MemoryClass memoryClass, Red::System::MemSize size, Red::System::MemSize allocAlignment, void* ptr );

		// Called after a successful reallocation
		typedef void (*PostReallocateHook)( PoolLabel label, MemoryClass memoryClass, Red::System::MemSize freedSize, Red::System::MemSize newSize, void* ptr );

		// Called just before we free memory from an allocator
		typedef void (*OnFreeHook)( PoolLabel label, MemoryClass memoryClass, Red::System::MemSize sizeToFree, const void* ptr );

		PreAllocateHook OnPreAllocate;
		PostAllocateHook OnPostAllocate;
		PreReallocateHook OnPreReallocate;
		PostReallocateHook OnPostReallocate;
		OnFreeHook OnFree;

		// Default ctor just ensures the fptrs are null
		MemoryHook()
			: OnPreAllocate( nullptr )
			, OnPostAllocate( nullptr )
			, OnPreReallocate( nullptr )
			, OnPostReallocate( nullptr )
			, OnFree( nullptr )
		{
		}

		RED_INLINE void SetNextHook( MemoryHook* next )
		{
			m_nextHook = next;
		}

		RED_INLINE MemoryHook* GetNextHook() const
		{
			return m_nextHook;
		}

	private:
		MemoryHook* m_nextHook;
	};

} }

#endif