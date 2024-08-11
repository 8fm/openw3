/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "memoryInitializer.h"
#include "../../common/redSystem/types.h"
#include "../../common/core/memory.h"
#include "../../common/redMemoryFramework/redMemoryFillHook.h"
#include "../../common/redMemoryFramework/redMemoryFrameworkTypes.h"

namespace Red
{
namespace UnitTest
{
	const Red::System::MemSize c_oneMegabyte = 1024u * 1024u;
	const Red::System::MemSize c_oneGigabyte = c_oneMegabyte * 1024u;

#if defined( RED_PLATFORM_WIN64 )
	const Red::System::MemSize c_defaultPoolInitialSize = c_oneGigabyte;
	const Red::System::MemSize c_defaultPoolMaxSize = c_oneGigabyte * 3;
	const Red::System::MemSize c_defaultPoolGranularity = c_oneMegabyte * 64u;
	const Uint32 c_defaultFlags = 0;
#elif defined( RED_PLATFORM_DURANGO )
	const Red::System::MemSize c_defaultPoolInitialSize = c_oneGigabyte * 2;
	const Red::System::MemSize c_defaultPoolMaxSize = c_oneGigabyte * 2;
	const Red::System::MemSize c_defaultPoolGranularity = c_oneMegabyte * 512u;
	const Uint32 c_defaultFlags = ( Red::MemoryFramework::Allocator_AccessCpu | Red::MemoryFramework::Allocator_Cached | Red::MemoryFramework::Allocator_StaticSize );
#elif defined( RED_PLATFORM_ORBIS )
	const Red::System::MemSize c_defaultPoolInitialSize = c_oneGigabyte;
	const Red::System::MemSize c_defaultPoolMaxSize = c_oneGigabyte;
	const Red::System::MemSize c_defaultPoolGranularity = c_oneGigabyte;
	const Uint32 c_defaultFlags = ( Red::MemoryFramework::Allocator_DirectMemory | Red::MemoryFramework::Allocator_UseOnionBus | Red::MemoryFramework::Allocator_AccessCpuReadWrite | Red::MemoryFramework::Allocator_StaticSize );
#endif

#ifndef RED_USE_NEW_MEMORY_SYSTEM

	// Small object pool chunk allocators 
	auto smallObjectAlloc = [] ( Red::System::MemSize size, Red::System::MemSize align ) 
	{ 
		return RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_SmallObjectPool, size, align );
	};
	auto smallObjectFree = [] ( void* ptr ) 
	{ 
		RED_MEMORY_FREE( MemoryPool_Default, MC_SmallObjectPool, ptr );
	};
	auto smallObjectOwnership = [] ( void* ptr )
	{
#ifdef CORE_USES_DEBUG_ALLOCATOR
		return RED_MEMORY_GET_ALLOCATOR( MemoryPool_DebugAllocator )->OwnsPointer( ptr );
#else
		return RED_MEMORY_GET_ALLOCATOR( MemoryPool_Default )->OwnsPointer( ptr );
#endif
	};

	struct UnitTestMemoryFillSelector
	{
		// Used to filter on pool / memory class for zero-fill on allocate
		RED_INLINE static Bool ShouldFillMemory( Red::System::Uint32 label, Red::System::Uint32	 memoryClass ) 
		{ 
			return true;
		}
	};

	void MemoryInitializer::SetUp()
	{
		typedef Red::MemoryFramework::MemoryFiller< UnitTestMemoryFillSelector > SelectiveMemoryFiller;
		static SelectiveMemoryFiller s_memoryFiller( SelectiveMemoryFiller::ZeroOnAllocate | SelectiveMemoryFiller::ZeroOnReallocate );
		SRedMemory::GetInstance().RegisterUserPoolHook( &s_memoryFiller );

		static RED_MEMORY_POOL_TYPE( MemoryPool_Default )::CreationParameters DefaultPoolParameters( c_defaultPoolInitialSize, c_defaultPoolMaxSize, c_defaultPoolGranularity );
		static RED_MEMORY_POOL_TYPE( MemoryPool_SmallObjects )::CreationParameters SmallObjectPoolParameters( smallObjectAlloc, smallObjectFree, smallObjectOwnership, 1024 * 256 );

		INTERNAL_RED_MEMORY_CREATE_POOL( SRedMemory::GetInstance(), CoreMemory, MemoryPool_Default, DefaultPoolParameters,  c_defaultFlags );
		INTERNAL_RED_MEMORY_CREATE_POOL( SRedMemory::GetInstance(), CoreMemory, MemoryPool_SmallObjects, SmallObjectPoolParameters, c_defaultFlags );
	}

#else
	void MemoryInitializer::SetUp()
	{}

#endif

	void MemoryInitializer::TearDown()
	{}
}
}
