/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "gpuApiUtils.h"
#include "gpuApiMemory.h"
#include "../redMemoryFramework/redMemoryFillHook.h"

#if defined( RED_PLATFORM_DURANGO )
# include "d3d11_x.h"
#endif

#ifndef GPU_API_USES_RED_MEMORY_FRAMEWORK

#include <stdlib.h>

namespace GpuApi
{
	// Public interface for allocating GPU data	
	void* AllocateTextureData( size_t size, size_t alignment )
	{
		(void)alignment;	// Unused
		return malloc( size );
	}

	void  FreeTextureData( void* dataBuffer )
	{
		free( dataBuffer );
	}
}

#else

#ifdef RED_PLATFORM_ORBIS
	extern void BindNativeOOMHandlerForAllocator( Red::MemoryFramework::MemoryManager* pool );
#endif

namespace GpuApi
{
	const Red::MemoryFramework::IAllocatorCreationParameters* GMemoryPoolCreationParameters[ MemoryPool_Max ] = { nullptr };

	// public interface for passing memory pool initialisation parameters
	void SetPoolParameters( Red::MemoryFramework::PoolLabel pool, const Red::MemoryFramework::IAllocatorCreationParameters* parameters )
	{
		GPUAPI_ASSERT( GMemoryPoolCreationParameters[ pool ] == nullptr, TXT( "Parameters already passed for this pool" ) );
		GPUAPI_ASSERT( pool < MemoryPool_Max, TXT( "Bad memory pool index" ) );
		if( pool < MemoryPool_Max )
		{
			GMemoryPoolCreationParameters[ pool ] = parameters;
		}
	}

	// Public interface for allocating GPU data	
	void* AllocateTextureData( size_t size, size_t alignment )
	{
#ifdef RED_PLATFORM_ORBIS
		const Red::MemoryFramework::PoolLabel pool = GpuMemoryPool_DefaultGarlic;
#else
		const Red::MemoryFramework::PoolLabel pool = GpuMemoryPool_Textures;
#endif
		return GPU_API_ALLOCATE( pool, MC_TextureData, size, alignment );
	}

	void  FreeTextureData( void* dataBuffer )
	{
		// This assumes that the data buffer was actually allocated from the texture pool. You will get asserts or crashes if you try to call this
		// with an address to memory allocated by a different pool!
#ifdef RED_PLATFORM_ORBIS
		const Red::MemoryFramework::PoolLabel pool = GpuMemoryPool_DefaultGarlic;
#else
		const Red::MemoryFramework::PoolLabel pool = GpuMemoryPool_Textures;
#endif
		GPU_API_FREE( pool, MC_TextureData, dataBuffer );
	}

	// Once the data is all cooked properly, we should have proper alignment values for everything and this will be redundant
	RED_INLINE Uint32 CalculateInplaceMemoryAlignment( EInPlaceResourceType type )
	{
		switch ( type )
		{
#if defined( RED_PLATFORM_DURANGO )
			// In worst case, Xbox can ask for 32k aligned texture memory, but we are using 1D tiling so 256byte should be enough
		case INPLACE_Texture:		return 0x100;
		case INPLACE_EnvProbe:		return 0x100;
		case INPLACE_DefragTemp:	return 0x100;
		case INPLACE_Buffer:		return 16;
#elif defined( RED_PLATFORM_ORBIS )
			// PS4 likes 256 byte aligned textures.
		case INPLACE_Texture:		return 0x100;
		case INPLACE_EnvProbe:		return 0x100;
		case INPLACE_DefragTemp:	return 0x100;
		case INPLACE_Buffer:		return 16;
#else
		case INPLACE_Texture:		return 16;
		case INPLACE_EnvProbe:		return 16;
		case INPLACE_Buffer:		return 16;
#endif
		default:
			GPUAPI_HALT( "Unsupported in-place resource pool" );
			return 0;
		}
	}

	// this mutex is needed because the inplace memory can be released from the decompression thread in case of the canceled streaming taks
	static Red::Threads::CMutex	st_inplacePoolLock;

	Red::MemoryFramework::MemoryRegionHandle AllocateInPlaceMemoryRegion( EInPlaceResourceType type, size_t size, Red::MemoryFramework::MemoryClass memoryClass, Uint32 alignment, Red::MemoryFramework::RegionLifetimeHint lifeTimeHint /*= Red::MemoryFramework::Region_Shortlived*/ )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_inplacePoolLock );

		if( alignment == (Uint32)-1 )
		{
			alignment = CalculateInplaceMemoryAlignment( type );
		}
		switch( type )
		{
		case INPLACE_Texture:
			return GPU_API_ALLOCATE_REGION( GpuMemoryPool_InPlaceRenderData, memoryClass, size, alignment, lifeTimeHint );		

		case INPLACE_Buffer:
#if defined( RED_PLATFORM_ORBIS )
			return GPU_API_ALLOCATE_REGION( GpuMemoryPool_Buffers, memoryClass, size, alignment, Red::MemoryFramework::Region_Shortlived );
#elif defined( RED_PLATFORM_DURANGO )
			return GPU_API_ALLOCATE_REGION( GpuMemoryPool_InPlaceMeshBuffers, memoryClass, size, alignment, Red::MemoryFramework::Region_Shortlived );
#endif

#if defined( RED_PLATFORM_CONSOLE )
		case INPLACE_DefragTemp:
			return GPU_API_ALLOCATE_REGION( GpuMemoryPool_InPlaceDefragTemp, memoryClass, size, GpuApi::CalculateInplaceMemoryAlignment( GpuApi::INPLACE_Texture ), Red::MemoryFramework::Region_Longlived );
#endif
		case INPLACE_EnvProbe:
			return GPU_API_ALLOCATE_REGION( GpuMemoryPool_EnvProbes, memoryClass, size, alignment, lifeTimeHint );

		default:
			GPUAPI_HALT( "Unsupported in-place resource pool" );
			return nullptr;
		}

		return nullptr;
	}

	void UnlockInPlaceMemoryRegion( EInPlaceResourceType type, Red::MemoryFramework::MemoryRegionHandle buffer )
	{
		switch( type )
		{
		case INPLACE_Texture:
			GPU_API_UNLOCK_REGION( GpuMemoryPool_InPlaceRenderData, buffer );
			break;
		case INPLACE_Buffer:
#if defined( RED_PLATFORM_ORBIS )
			GPU_API_UNLOCK_REGION( GpuMemoryPool_Buffers, buffer );
#elif defined( RED_PLATFORM_DURANGO )
			GPU_API_UNLOCK_REGION( GpuMemoryPool_InPlaceMeshBuffers, buffer );
#endif
			break;
#if defined( RED_PLATFORM_CONSOLE )
		case INPLACE_DefragTemp:
			GPU_API_UNLOCK_REGION( GpuMemoryPool_InPlaceDefragTemp, buffer );
			break;
#endif
		case INPLACE_EnvProbe:
			GPU_API_UNLOCK_REGION( GpuMemoryPool_EnvProbes, buffer );
			break;

		default:
			GPUAPI_HALT( "Unsupported in-place resource pool" );
		}
	}

	void LockInPlaceMemoryRegion( EInPlaceResourceType type, Red::MemoryFramework::MemoryRegionHandle buffer )
	{
		switch( type )
		{
		case INPLACE_Texture:
			GPU_API_LOCK_REGION( GpuMemoryPool_InPlaceRenderData, buffer );
			break;
		case INPLACE_Buffer:
#if defined( RED_PLATFORM_ORBIS )
			GPU_API_LOCK_REGION( GpuMemoryPool_Buffers, buffer );
#elif defined( RED_PLATFORM_DURANGO )
			GPU_API_LOCK_REGION( GpuMemoryPool_InPlaceMeshBuffers, buffer );
#endif
			break;
#if defined( RED_PLATFORM_CONSOLE )
		case INPLACE_DefragTemp:
			GPU_API_LOCK_REGION( GpuMemoryPool_InPlaceDefragTemp, buffer );
			break;
#endif
		case INPLACE_EnvProbe:
			GPU_API_LOCK_REGION( GpuMemoryPool_EnvProbes, buffer );
			break;

		default:
			GPUAPI_HALT( "Unsupported in-place resource pool" );
		}
	}

	Red::MemoryFramework::MemoryRegionHandle SplitInPlaceMemoryRegion( Red::MemoryFramework::MemoryRegionHandle& base, Uint32 splitOffset, MemSize splitBlockAlignment )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_inplacePoolLock );

#ifdef RED_PLATFORM_ORBIS
		return GPU_API_SPLIT_REGION( GpuMemoryPool_Buffers, base, splitOffset, splitBlockAlignment );
#elif defined( RED_PLATFORM_DURANGO )
		return GPU_API_SPLIT_REGION( GpuMemoryPool_InPlaceMeshBuffers, base, splitOffset, splitBlockAlignment );
#endif
		GPUAPI_HALT( "Unsupported in-place resource pool" );
		return nullptr;
	}

	void ReleaseInPlaceMemoryRegion( EInPlaceResourceType type, Red::MemoryFramework::MemoryRegionHandle buffer )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_inplacePoolLock );

		switch ( type )
		{
		case INPLACE_Texture:
			GPU_API_FREE_REGION( GpuMemoryPool_InPlaceRenderData, buffer );
			break;

		case INPLACE_Buffer:
#ifdef RED_PLATFORM_ORBIS
			GPU_API_FREE_REGION( GpuMemoryPool_Buffers, buffer );
			break;
		case INPLACE_DefragTemp:
			GPU_API_FREE_REGION( GpuMemoryPool_InPlaceDefragTemp, buffer );
			break;

#elif defined( RED_PLATFORM_DURANGO )
			GPU_API_FREE_REGION( GpuMemoryPool_InPlaceMeshBuffers, buffer );
			break;
		case INPLACE_DefragTemp:
			GPU_API_FREE_REGION( GpuMemoryPool_InPlaceDefragTemp, buffer );
			break;
#endif
		case INPLACE_EnvProbe:
			GPU_API_FREE_REGION( GpuMemoryPool_EnvProbes, buffer );
			break;

		default:
			GPUAPI_HALT( "Unsupported in-place resource pool" );
		}
	}

	void* AllocateInPlaceMemory( EInPlaceResourceType type, size_t size, Uint32 alignment )
	{
#ifdef RED_PLATFORM_CONSOLE
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_inplacePoolLock );

		// THIS FUNCTION SHOULD NEVER BE CALLED
		GPUAPI_HALT( "This should not be called!!!" );

		return nullptr;
#else
		size_t actualAlignment = alignment;
		if( alignment == -1 )
		{
			actualAlignment = CalculateInplaceMemoryAlignment( type );
		}

		switch ( type )
		{
		case INPLACE_Buffer:		return GPU_API_ALLOCATE( GpuMemoryPool_Buffers, MC_BufferObject, size, actualAlignment );
		default:
			GPUAPI_HALT( "Unsupported in-place resource pool" );
			return nullptr;
		}
#endif
	}

	void ReleaseInPlaceMemory( EInPlaceResourceType type, void* buffer )
	{
#ifdef RED_PLATFORM_CONSOLE
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_inplacePoolLock );

		// THIS FUNCTION SHOULD NEVER BE CALLED
		GPUAPI_HALT( "This should not be called!!!" );
#else
		switch ( type )
		{
		case INPLACE_Buffer:	GPU_API_FREE( GpuMemoryPool_Buffers, MC_BufferObject, buffer );		break;

		default:
			GPUAPI_HALT( "Unsupported in-place resource pool" );
		}
#endif
	}

	void FlushCpuCache( const void *address, Uint32 size )
	{
#if defined( RED_PLATFORM_DURANGO )
		D3DFlushCpuCache( address, size );
#elif defined( RED_PLATFORM_ORBIS )
		_mm_mfence(); 
		Uint8* p = (Uint8*)( (size_t)address & ~(size_t)0x3f );
		Uint8* pEndAddress = (Uint8*)( ( (size_t)( p + size ) + (size_t)0x3f ) & ~(size_t)0x3f );
		while ( p < pEndAddress )
		{
			_mm_clflush( p );
			p += 64;
		}
		_mm_mfence(); 
#endif
	}

	Red::MemoryFramework::MemoryManager* GpuApiMemory::GetInstance()
	{
		// This buffer contains the manager eventually. Force it to be aligned to 16 so any internals will not be mis-aligned
		// Since this is a DirectX lib, we can just use the Microsoft-specific alignment specifier
		static __declspec( align( 16 ) ) char s_managerBuffer[ sizeof( Red::MemoryFramework::MemoryManager ) ];

		// This pointer tracks the manager instance
		static Red::MemoryFramework::MemoryManager* s_memoryManagerInstance = nullptr;

		if( s_memoryManagerInstance != nullptr )
		{
			return s_memoryManagerInstance;
		}
		else
		{
			s_memoryManagerInstance = new ( &s_managerBuffer[0] ) Red::MemoryFramework::MemoryManager( c_GpuApiStaticMemorySize, c_GpuApiOverflowMemorySize );
			OnCreateInstance( s_memoryManagerInstance );

#ifdef RED_PLATFORM_ORBIS
			::BindNativeOOMHandlerForAllocator( s_memoryManagerInstance );
#endif

			return s_memoryManagerInstance;
		}
	}

	void GpuApiMemory::DestroyInstance()
	{
		Red::MemoryFramework::MemoryManager* manager = GetInstance();

		#define DECLARE_MEMORY_POOL( PoolName, PoolType, Flags )	INTERNAL_RED_MEMORY_DESTROY_POOL( (*manager), GpuMemory, PoolName );
		#include "gpuApiMemoryPools.h"
		#undef DECLARE_MEMORY_POOL

		#ifdef GPU_MEMORY_METRICS_DUMP_ENABLED
		manager->EndMetricsDump();
		#endif
	}

// Disable Warning #C4390 (if statement ending with ; in builds with error handling disabled)
RED_WARNING_PUSH()
RED_DISABLE_WARNING_MSC(4390)
	void GpuApiMemory::OnCreateInstance( Red::MemoryFramework::MemoryManager* manager )
	{
		// Register Pool Names
		#define DECLARE_MEMORY_POOL( PoolName, PoolType, Flags )	manager->RegisterPoolName( PoolName, #PoolName );
		#include "gpuApiMemoryPools.h"
		#undef DECLARE_MEMORY_POOL

		// Register Class Names
		#define DECLARE_MEMORY_CLASS( ClassName )	manager->RegisterClassName( ClassName, #ClassName );
		#include "gpuApiMemoryClasses.h"
		#undef DECLARE_MEMORY_CLASS

		// Create and register the pools with the manager. !!! There are no optional pools in the GPU API!
		// If the fatal asserts below get hit at any point, then either the pool parameters are not being passed (maybe a new pool?) or you are trying to 
		// allocate GPU resources too early.
		#define DECLARE_MEMORY_POOL( PoolName, PoolType, Flags )	\
		{                                                                       \
			const Red::MemoryFramework::IAllocatorCreationParameters* parameters = GMemoryPoolCreationParameters[ PoolName ];		\
			if( parameters == nullptr )																								\
			{																														\
				GPUAPI_LOG_WARNING( MACRO_TXT( "Failed to create pool '" ) MACRO_TXT( #PoolName ) MACRO_TXT( "'. No parameters passed to GPU API" ) );	\
			}																														\
			else                                                                                                                    \
			{                                                                                                                       \
				if( INTERNAL_RED_MEMORY_CREATE_POOL( (*manager), GpuMemory, PoolName, *parameters, Flags ) != ::Red::MemoryFramework::MemoryManager_OK )		\
					GPUAPI_HALT( "Failed to create pool '" MACRO_TXT( #PoolName ) MACRO_TXT("'") );																\
			}																														\
		}
		#include "gpuApiMemoryPools.h"
		#undef DECLARE_MEMORY_POOL

		manager->AnnotateSystemMemoryAreas();

		#ifdef GPU_MEMORY_METRICS_DUMP_ENABLED
		manager->BeginMetricsDump( TXT( "gpu_memory.rmm" ) );
		#endif
	}
RED_WARNING_POP()
}

#endif
