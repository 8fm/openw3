/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../gpuApiUtils/gpuApiErrorHandling.h"

// Use this to enable GPU memory metrics dumps
//#define GPU_MEMORY_METRICS_DUMP_ENABLED

namespace GpuApi
{
	////////////////////////////////////////////////////////////////////////////
	// Auto-generate memory pool label enum
	#define DECLARE_MEMORY_POOL( PoolName, PoolType, Flags )	PoolName,
	enum EMemoryPoolLabel
	{
		#include "gpuApiMemoryPools.h"
		MemoryPool_Max
	};
	#undef DECLARE_MEMORY_POOL

	///////////////////////////////////////////////////////////////////////////
	// Auto-generate memory class enum
	#define DECLARE_MEMORY_CLASS( ClassName )	ClassName,
	enum EMemoryClass
	{
		#include "gpuApiMemoryClasses.h"
	};
	#undef DECLARE_MEMORY_CLASS
}

#ifdef GPU_API_USES_RED_MEMORY_FRAMEWORK

//////////////////////////////////////////////////////////////////
// Red Memory Setup

#include "../redMemoryFramework/redMemoryFramework.h"
#include <new>	// For in-place new

#define GPU_API_ALLOCATE( PoolName, MemoryClass, Size, Alignment )		INTERNAL_RED_MEMORY_ALLOCATE_ALIGNED( (*GpuApi::GpuApiMemory::GetInstance()), GpuApi::GpuMemory, PoolName, MemoryClass, Size, Alignment )
#define GPU_API_FREE( PoolName, MemoryClass, Address )					INTERNAL_RED_MEMORY_FREE( (*GpuApi::GpuApiMemory::GetInstance()), GpuApi::GpuMemory, PoolName, MemoryClass, Address )
#define GPU_API_ALLOCATE_REGION( PoolName, MemoryClass, Size, Alignment, Lifetime )	INTERNAL_RED_MEMORY_ALLOCATE_REGION( (*GpuApi::GpuApiMemory::GetInstance()), GpuApi::GpuMemory, PoolName, MemoryClass, Size, Alignment, Lifetime )
#define GPU_API_SPLIT_REGION( PoolName, Block, Size, Alignment )		INTERNAL_RED_MEMORY_SPLIT_REGION( (*GpuApi::GpuApiMemory::GetInstance()), GpuApi::GpuMemory, PoolName, Block, Size, Alignment )
#define GPU_API_FREE_REGION( PoolName, Region )							INTERNAL_RED_MEMORY_FREE_REGION( (*GpuApi::GpuApiMemory::GetInstance()), GpuApi::GpuMemory, PoolName, Region )
#define GPU_API_UNLOCK_REGION( PoolName, Region )						INTERNAL_RED_MEMORY_UNLOCK_REGION( (*GpuApi::GpuApiMemory::GetInstance()), GpuApi::GpuMemory, PoolName, Region )
#define GPU_API_LOCK_REGION( PoolName, Region )							INTERNAL_RED_MEMORY_LOCK_REGION( (*GpuApi::GpuApiMemory::GetInstance()), GpuApi::GpuMemory, PoolName, Region )

namespace GpuApi
{
	// Pool types
	INTERNAL_RED_MEMORY_BEGIN_POOL_TYPES( GpuMemory )
		#define DECLARE_MEMORY_POOL( PoolName, PoolType, Flags )	INTERNAL_RED_MEMORY_DECLARE_POOL( PoolName, PoolType )
			#include "gpuApiMemoryPools.h"
		#undef DECLARE_MEMORY_POOL
	INTERNAL_RED_MEMORY_END_POOL_TYPES

	const Red::System::MemSize c_GpuApiStaticMemorySize = 0;		// GPU Api should allocate no memory before the manager is created
	const Red::System::MemSize c_GpuApiOverflowMemorySize = 0;		// GPU Api should be strict about budgets. Do not use an overflow pool for now

	// Wrapper for the Gpu API Memory Manager interface
	// Acts as a singleton and controls manager creation / destruction
	class GpuApiMemory
	{
	public:
		static Red::MemoryFramework::MemoryManager* GetInstance();
		static void DestroyInstance();

	private:
		static void OnCreateInstance( Red::MemoryFramework::MemoryManager* manager );
	};

	// public interface for setting up memory pools. the params are not copied, so the caller must keep the params memory in-scope at least until the gpu api memory is initialised
	void SetPoolParameters( Red::MemoryFramework::PoolLabel pool, const Red::MemoryFramework::IAllocatorCreationParameters* parameters );

	// Public interface for allocating GPU data
	// TODO : Get rid of these, either use GPU_API_[ALLOCATE|FREE] directly, or the "InPlace" versions below
	void* AllocateTextureData( size_t size, size_t alignment );
	void  FreeTextureData( void* dataBuffer );


	// Memory will be appropriately aligned, based on pool/class.
	enum EInPlaceResourceType : Uint8
	{
		INPLACE_None,
		INPLACE_Texture,
		INPLACE_Buffer,
		INPLACE_EnvProbe,
		INPLACE_DefragTemp,

		INPLACE_MAX
	};

	// If alignment of -1 is passed, a safe default will be used
	void* AllocateInPlaceMemory( EInPlaceResourceType type, size_t size, Uint32 alignment );
	void  ReleaseInPlaceMemory( EInPlaceResourceType type, void* buffer );

	Red::MemoryFramework::MemoryRegionHandle AllocateInPlaceMemoryRegion( EInPlaceResourceType type, size_t size, Red::MemoryFramework::MemoryClass memoryClass, Uint32 alignment=(Uint32)-1, Red::MemoryFramework::RegionLifetimeHint lifeTimeHint = Red::MemoryFramework::Region_Shortlived );
	Red::MemoryFramework::MemoryRegionHandle SplitInPlaceMemoryRegion( Red::MemoryFramework::MemoryRegionHandle& base, Uint32 splitOffset, MemSize splitBlockAlignment );
	void ReleaseInPlaceMemoryRegion( EInPlaceResourceType type, Red::MemoryFramework::MemoryRegionHandle buffer );
	void UnlockInPlaceMemoryRegion( EInPlaceResourceType type, Red::MemoryFramework::MemoryRegionHandle buffer );
	void LockInPlaceMemoryRegion( EInPlaceResourceType type, Red::MemoryFramework::MemoryRegionHandle buffer );

	// Flushes a CPU cache
	void FlushCpuCache( const void *address, Uint32 size );
}

#else

/////////////////////////////////////////////////////////////////////////
// Non-Red Memory builds just use malloc / free for everything

#define GPU_API_ALLOCATE( PoolName, MemoryClass, Size, Alignment )		malloc( Size );
#define GPU_API_FREE( PoolName, MemoryClass, Address )					free( Address );

namespace GpuApi
{
	// Public interface for allocating GPU data	
	void* AllocateTextureData( size_t size, size_t alignment );
	void  FreeTextureData( void* dataBuffer );
}

#endif