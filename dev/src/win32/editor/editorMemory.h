/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/core/memory.h"
#include "../../common/gpuApiUtils/gpuApiMemory.h"
#include "terrainEditTool.h"		// Required to calculate correct pool size

const MemSize c_oneMegabyte = 1024u * 1024u;
const MemSize c_oneGigabyte = c_oneMegabyte * 1024u;

// The x86 build of the editor has less memory available in the default pool to ensure D3D has enough virtual memory breathing room
#ifdef RED_PLATFORM_WIN32
	const MemSize c_defaultPoolInitialSize = c_oneMegabyte * 512u;
	const MemSize c_defaultPoolMaxSize = c_oneGigabyte * 2 + (c_oneGigabyte / 2);
	const MemSize c_defaultPoolGranularity = c_oneMegabyte * 32u;
#elif defined( RED_PLATFORM_WIN64 )
	const MemSize c_defaultPoolInitialSize = c_oneGigabyte;
	const MemSize c_defaultPoolMaxSize = c_oneGigabyte * 5;
	const MemSize c_defaultPoolGranularity = c_oneMegabyte * 256u;
#endif

const MemSize c_umbraTomeCollectionOverhead = 1u * c_oneMegabyte;
const MemSize c_umbraTomeCollectionMaxSize = 25u * c_oneMegabyte;
const MemSize c_umbraTomeCollectionPoolSize = 2u * ( c_umbraTomeCollectionMaxSize + c_umbraTomeCollectionOverhead );
const MemSize c_umbraPoolSize = 180u * c_oneMegabyte - c_umbraTomeCollectionPoolSize;
const MemSize c_gameSavePoolSize = 10u * c_oneMegabyte;

////////////////////////////// CORE MEMORY //////////////////////////////////

#ifndef RED_USE_NEW_MEMORY_SYSTEM

// Memory pool initialisers
namespace EditorPoolCreationParams
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
	RED_MEMORY_POOL_TYPE( MemoryPool_Umbra )::CreationParameters				UmbraPoolParameters( c_umbraPoolSize, c_umbraPoolSize, c_umbraPoolSize );
	RED_MEMORY_POOL_TYPE( MemoryPool_UmbraTC )::CreationParameters				UmbraTomeCollectionPoolParameters( c_umbraTomeCollectionPoolSize, c_umbraTomeCollectionPoolSize, c_umbraTomeCollectionPoolSize );
	RED_MEMORY_POOL_TYPE( MemoryPool_TerrainEditor )::CreationParameters		TerrainEditorPoolParameters( c_oneMegabyte * 128u, c_oneMegabyte * 256u, c_defaultPoolGranularity );
	RED_MEMORY_POOL_TYPE( MemoryPool_ScriptCompilation )::CreationParameters	ScriptCompilationPoolParameters( 128u * c_oneMegabyte, c_oneGigabyte, c_defaultPoolGranularity );
	RED_MEMORY_POOL_TYPE( MemoryPool_Physics )::CreationParameters				PhysicsPoolParameters( 128u * c_oneMegabyte, c_oneGigabyte, c_defaultPoolGranularity );
	RED_MEMORY_POOL_TYPE( MemoryPool_SmallObjects )::CreationParameters			SmallObjectPoolParameters( smallObjectAlloc, smallObjectFree, smallObjectOwnership, 1024 * 256 );
	RED_MEMORY_POOL_TYPE( MemoryPool_GameSave )::CreationParameters				GameSavePoolParameters( c_gameSavePoolSize, c_gameSavePoolSize, c_gameSavePoolSize );

#ifdef CORE_USES_DEBUG_ALLOCATOR
	RED_MEMORY_POOL_TYPE( MemoryPool_DebugAllocator )::CreationParameters			DebugPoolParameters;
#endif
}

class EditorMemoryParameters : public CoreMemory::CMemoryPoolParameters
{
public:
	EditorMemoryParameters()
	{
		SetPoolParameters( MemoryPool_Default, &EditorPoolCreationParams::DefaultPoolParameters );
		SetPoolParameters( MemoryPool_Umbra, &EditorPoolCreationParams::UmbraPoolParameters );
		SetPoolParameters( MemoryPool_UmbraTC, &EditorPoolCreationParams::UmbraTomeCollectionPoolParameters );
		SetPoolParameters( MemoryPool_TerrainEditor, &EditorPoolCreationParams::TerrainEditorPoolParameters );
		SetPoolParameters( MemoryPool_ScriptCompilation, &EditorPoolCreationParams::ScriptCompilationPoolParameters );
		SetPoolParameters( MemoryPool_Physics, &EditorPoolCreationParams::PhysicsPoolParameters );
		SetPoolParameters( MemoryPool_SmallObjects, &EditorPoolCreationParams::SmallObjectPoolParameters );
		SetPoolParameters( MemoryPool_GameSave, &EditorPoolCreationParams::GameSavePoolParameters );
	
#ifdef CORE_USES_DEBUG_ALLOCATOR
		SetPoolParameters( MemoryPool_DebugAllocator, &EditorPoolCreationParams::DebugPoolParameters );
#endif
	}
};

#endif


////////////////////////////// GPU MEMORY //////////////////////////////////

namespace EditorGpuPoolCreationParams
{
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_Textures )::CreationParameters			TexturePoolParameters( 128u * c_oneMegabyte, 512u * c_oneMegabyte, 128u * c_oneMegabyte );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_Buffers )::CreationParameters			BufferPoolParameters( 72u * c_oneMegabyte, 216u * c_oneMegabyte, 72u * c_oneMegabyte );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_LockedBufferData )::CreationParameters	LockedBufferPoolParameters( 8u * c_oneMegabyte, 64u * c_oneMegabyte, 8u * c_oneMegabyte );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_ShaderInclude )::CreationParameters		ShaderIncludePoolParameters( c_oneMegabyte, c_oneMegabyte, c_oneMegabyte );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_Device )::CreationParameters			DevicePoolParameters( 52u * c_oneMegabyte, 52u * c_oneMegabyte, 52u * c_oneMegabyte );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_InPlaceRenderData )::CreationParameters InPlacePoolParameters( 512u * c_oneMegabyte, 4u * c_oneMegabyte, 0, 0 );
}

class EditorGpuMemoryParameters : public CoreMemory::CMemoryPoolParameters
{
public:
	EditorGpuMemoryParameters()
	{
		SetPoolParameters( GpuApi::GpuMemoryPool_Textures, &EditorGpuPoolCreationParams::TexturePoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_Buffers, &EditorGpuPoolCreationParams::BufferPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_LockedBufferData, &EditorGpuPoolCreationParams::LockedBufferPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_ShaderInclude, &EditorGpuPoolCreationParams::ShaderIncludePoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_Device, &EditorGpuPoolCreationParams::DevicePoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_InPlaceRenderData, &EditorGpuPoolCreationParams::InPlacePoolParameters );
	}
};