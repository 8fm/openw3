/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/core/memory.h"
#include "../../common/gpuApiUtils/gpuApiMemory.h"

const MemSize c_oneMegabyte = 1024u * 1024u;
const MemSize c_oneGigabyte = c_oneMegabyte * 1024u;

////////////////////////////// CORE MEMORY //////////////////////////////////

const MemSize c_umbraTomeCollectionOverhead = 1u * c_oneMegabyte;
const MemSize c_umbraTomeCollectionMaxSize = 20u * c_oneMegabyte;
const MemSize c_umbraTomeCollectionPoolSize = 2u * ( c_umbraTomeCollectionMaxSize + c_umbraTomeCollectionOverhead );
const MemSize c_umbraPoolSize = 170u * c_oneMegabyte - c_umbraTomeCollectionPoolSize;
const MemSize c_gameSavePoolSize = 10u * c_oneMegabyte;

#ifdef RED_PLATFORM_WIN32
	const MemSize c_defaultPoolInitialSize = c_oneMegabyte * 512u;
	const MemSize c_defaultPoolMaxSize = c_oneGigabyte * 2 + (c_oneGigabyte / 2);
	const MemSize c_defaultPoolGranularity = c_oneMegabyte * 32u;
#elif defined( RED_PLATFORM_WIN64 )
	const MemSize c_defaultPoolInitialSize = c_oneGigabyte;
	const MemSize c_defaultPoolMaxSize = c_oneGigabyte * 3;
	const MemSize c_defaultPoolGranularity = c_oneMegabyte * 128u;
#endif

#ifndef RED_USE_NEW_MEMORY_SYSTEM

// Memory pool initialisers
namespace R4PoolCreationParams
{
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

	RED_MEMORY_POOL_TYPE( MemoryPool_Default )::CreationParameters				DefaultPoolParameters( c_defaultPoolInitialSize, c_defaultPoolMaxSize, c_defaultPoolGranularity );
	RED_MEMORY_POOL_TYPE( MemoryPool_Umbra )::CreationParameters				UmbraPoolParameters( c_umbraPoolSize, 2 * 176u * c_oneMegabyte, 32u * c_oneMegabyte );
	RED_MEMORY_POOL_TYPE( MemoryPool_UmbraTC )::CreationParameters				UmbraTomeCollectionPoolParameters( c_umbraTomeCollectionPoolSize, c_umbraTomeCollectionPoolSize, c_umbraTomeCollectionPoolSize );
	RED_MEMORY_POOL_TYPE( MemoryPool_ScriptCompilation )::CreationParameters	ScriptCompilationPoolParameters( 128u * c_oneMegabyte, c_oneGigabyte, c_defaultPoolGranularity );
	RED_MEMORY_POOL_TYPE( MemoryPool_Physics )::CreationParameters				PhysicsPoolParameters( 128u * c_oneMegabyte, 512 * c_oneMegabyte, c_defaultPoolGranularity );
	RED_MEMORY_POOL_TYPE( MemoryPool_SmallObjects )::CreationParameters			SmallObjectPoolParameters( smallObjectAlloc, smallObjectFree, smallObjectOwnership, 1024 * 256 );
	RED_MEMORY_POOL_TYPE( MemoryPool_GameSave )::CreationParameters				GameSavePoolParameters( c_gameSavePoolSize, c_gameSavePoolSize, c_gameSavePoolSize );
	RED_MEMORY_POOL_TYPE( MemoryPool_IO )::CreationParameters					IOPoolParameters( 300 * c_oneMegabyte, 300 * c_oneMegabyte, 300 * c_oneMegabyte );

#ifdef CORE_USES_DEBUG_ALLOCATOR
	RED_MEMORY_POOL_TYPE( MemoryPool_DebugAllocator )::CreationParameters			DebugPoolParameters;
#endif
}

class R4MemoryParameters : public CoreMemory::CMemoryPoolParameters
{
public:
	R4MemoryParameters( )
	{
		SetPoolParameters( MemoryPool_Default, &R4PoolCreationParams::DefaultPoolParameters );
		SetPoolParameters( MemoryPool_Umbra, &R4PoolCreationParams::UmbraPoolParameters );
		SetPoolParameters( MemoryPool_UmbraTC, &R4PoolCreationParams::UmbraTomeCollectionPoolParameters );
		SetPoolParameters( MemoryPool_ScriptCompilation, &R4PoolCreationParams::ScriptCompilationPoolParameters );
		SetPoolParameters( MemoryPool_Physics, &R4PoolCreationParams::PhysicsPoolParameters );
		SetPoolParameters( MemoryPool_SmallObjects, &R4PoolCreationParams::SmallObjectPoolParameters );
		SetPoolParameters( MemoryPool_GameSave, &R4PoolCreationParams::GameSavePoolParameters );
		SetPoolParameters( MemoryPool_IO, &R4PoolCreationParams::IOPoolParameters );

#ifdef CORE_USES_DEBUG_ALLOCATOR
		SetPoolParameters( MemoryPool_DebugAllocator, &R4PoolCreationParams::DebugPoolParameters );
#endif
	}
};

#endif

////////////////////////////// GPU MEMORY //////////////////////////////////

namespace R4GpuPoolCreationParams
{
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_Textures )::CreationParameters			TexturePoolParameters( 72u * c_oneMegabyte, 216u * c_oneMegabyte, 72u * c_oneMegabyte );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_Buffers )::CreationParameters			BufferPoolParameters( 72u * c_oneMegabyte, 216u * c_oneMegabyte, 72u * c_oneMegabyte );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_LockedBufferData )::CreationParameters	LockedBufferPoolParameters( 8u * c_oneMegabyte, 64u * c_oneMegabyte, 8u * c_oneMegabyte );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_ShaderInclude )::CreationParameters		ShaderIncludePoolParameters( c_oneMegabyte, c_oneMegabyte, c_oneMegabyte );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_Device )::CreationParameters			DevicePoolParameters( 52u * c_oneMegabyte, 52u * c_oneMegabyte, 52u * c_oneMegabyte );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_InPlaceRenderData )::CreationParameters InPlacePoolParameters( 512u * c_oneMegabyte, 4u * c_oneMegabyte, 0, 0 );
}

class R4GpuMemoryParameters : public CoreMemory::CMemoryPoolParameters
{
public:
	R4GpuMemoryParameters( const Core::CommandLineArguments& )
	{
		SetPoolParameters( GpuApi::GpuMemoryPool_Textures, &R4GpuPoolCreationParams::TexturePoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_Buffers, &R4GpuPoolCreationParams::BufferPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_LockedBufferData, &R4GpuPoolCreationParams::LockedBufferPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_ShaderInclude, &R4GpuPoolCreationParams::ShaderIncludePoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_Device, &R4GpuPoolCreationParams::DevicePoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_InPlaceRenderData, &R4GpuPoolCreationParams::InPlacePoolParameters );
	}
};
