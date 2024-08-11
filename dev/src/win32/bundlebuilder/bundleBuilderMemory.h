/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/core/memory.h"

const MemSize c_oneMegabyte = 1024u * 1024u;
const MemSize c_oneGigabyte = c_oneMegabyte * 1024u;

#ifdef RED_PLATFORM_WIN32
	const MemSize c_defaultPoolInitialSize		= c_oneMegabyte * 512u;
	const MemSize c_defaultPoolMaxSize			= c_oneGigabyte * 2 + (c_oneGigabyte / 2);
	const MemSize c_defaultPoolGranularity		= c_oneMegabyte * 32u;
#elif defined( RED_PLATFORM_WIN64 )
	const MemSize c_defaultPoolInitialSize		= c_oneGigabyte;
	const MemSize c_defaultPoolMaxSize			= c_oneGigabyte * 3;
	const MemSize c_defaultPoolGranularity		= c_oneMegabyte * 512u;
#endif

const MemSize c_BundleBuilderPoolGranularity	= c_defaultPoolGranularity;
const MemSize c_umbraPoolSize = 176u * c_oneMegabyte;

#ifndef RED_USE_NEW_MEMORY_SYSTEM

// Memory pool initialisers
namespace BundleBuilderPoolCreationParams
{
	// Small object pool chunk allocators 
	const auto smallObjectAlloc = [] ( Red::System::MemSize size, Red::System::MemSize align ) 
	{ 
		return RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_SmallObjectPool, size, align );
	};
	const auto smallObjectFree = [] ( void* ptr ) 
	{ 
		RED_MEMORY_FREE( MemoryPool_Default, MC_SmallObjectPool, ptr );
	};
	const auto smallObjectOwnership = [] ( void* ptr )
	{
		return RED_MEMORY_GET_ALLOCATOR( MemoryPool_Default )->OwnsPointer( ptr );
	};

	const RED_MEMORY_POOL_TYPE( MemoryPool_Default )::CreationParameters		DefaultPoolParameters( c_defaultPoolInitialSize, c_defaultPoolMaxSize, c_defaultPoolGranularity );
	const RED_MEMORY_POOL_TYPE( MemoryPool_Umbra )::CreationParameters			UmbraPoolParameters( c_umbraPoolSize, 2 * 160u * c_oneMegabyte, 32u * c_oneMegabyte );
	const RED_MEMORY_POOL_TYPE( MemoryPool_Physics )::CreationParameters		PhysicsPoolParameters( 128u * c_oneMegabyte, c_oneGigabyte / 2, c_defaultPoolGranularity );
	const RED_MEMORY_POOL_TYPE( MemoryPool_SmallObjects )::CreationParameters	SmallObjectPoolParameters( smallObjectAlloc, smallObjectFree, smallObjectOwnership, 1024 * 256 );
}

class BundleBuilderMemoryParameters : public CoreMemory::CMemoryPoolParameters
{
public:
	BundleBuilderMemoryParameters()
	{
		SetPoolParameters( MemoryPool_Default, &BundleBuilderPoolCreationParams::DefaultPoolParameters );
		SetPoolParameters( MemoryPool_Umbra, &BundleBuilderPoolCreationParams::UmbraPoolParameters );
		SetPoolParameters( MemoryPool_Physics, &BundleBuilderPoolCreationParams::PhysicsPoolParameters );
		SetPoolParameters( MemoryPool_SmallObjects, &BundleBuilderPoolCreationParams::SmallObjectPoolParameters );
	}
};


#define MemoryPool_BundleBuilder MemoryPool_Max + 1

INTERNAL_RED_MEMORY_BEGIN_POOL_TYPES( BundlePools )
	INTERNAL_RED_MEMORY_DECLARE_POOL( MemoryPool_BundleBuilder, Red::MemoryFramework::TLSFAllocatorThreadSafe );
INTERNAL_RED_MEMORY_END_POOL_TYPES

#define BUNDLER_MEMORY_ALLOCATE( MemoryClass, Size )						INTERNAL_RED_MEMORY_ALLOCATE_ALIGNED	( SRedMemory::GetInstance(), BundlePools, MemoryPool_BundleBuilder, MemoryClass, Size, CalculateDefaultAlignment( Size ) )
#define BUNDLER_MEMORY_REALLOCATE( Ptr, MemoryClass, Size )					INTERNAL_RED_MEMORY_REALLOCATE_ALIGNED	( SRedMemory::GetInstance(), BundlePools, MemoryPool_BundleBuilder, Ptr, MemoryClass, Size, CalculateDefaultAlignment( Size ) )
#define BUNDLER_MEMORY_ALLOCATE_ALIGNED( MemoryClass, Size, Align )			INTERNAL_RED_MEMORY_ALLOCATE_ALIGNED	( SRedMemory::GetInstance(), BundlePools, MemoryPool_BundleBuilder, MemoryClass, Size, Align )
#define BUNDLER_MEMORY_REALLOCATE_ALIGNED( Ptr, MemoryClass, Size, Align )	INTERNAL_RED_MEMORY_REALLOCATE_ALIGNED	( SRedMemory::GetInstance(), BundlePools, MemoryPool_BundleBuilder, Ptr, MemoryClass, Size, Align )
#define BUNDLER_MEMORY_FREE( MemoryClass, Ptr )								INTERNAL_RED_MEMORY_FREE				( SRedMemory::GetInstance(), BundlePools, MemoryPool_BundleBuilder, MemoryClass, Ptr )

#else

RED_MEMORY_POOL_STATIC( MemoryPool_BundleBuilder, red::memory::DefaultAllocator );

#define BUNDLER_MEMORY_ALLOCATE( MemoryClass, Size )						RED_ALLOCATE( MemoryPool_BundleBuilder, Size )
#define BUNDLER_MEMORY_REALLOCATE( Ptr, MemoryClass, Size )					RED_REALLOCATE( MemoryPool_BundleBuilder, Ptr, Size )
#define BUNDLER_MEMORY_ALLOCATE_ALIGNED( MemoryClass, Size, Align )			RED_ALLOCATE_ALIGNED( MemoryPool_BundleBuilder, Size, Align )
#define BUNDLER_MEMORY_REALLOCATE_ALIGNED( Ptr, MemoryClass, Size, Align )	RED_REALLOCATE( MemoryPool_BundleBuilder, Ptr, Size )
#define BUNDLER_MEMORY_FREE( MemoryClass, Ptr )								RED_FREE( MemoryPool_BundleBuilder, Ptr )						

#endif
