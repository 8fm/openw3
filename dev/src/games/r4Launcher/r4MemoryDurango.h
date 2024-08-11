/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/core/memory.h"
#include "../../common/gpuApiUtils/gpuApiMemory.h"

const MemSize c_oneMegabyte = 1024u * 1024u;
const MemSize c_oneGigabyte = c_oneMegabyte * 1024u;

  const MemSize c_umbraTomeCollectionOverhead = 1u * c_oneMegabyte;
  const MemSize c_umbraTomeCollectionMaxSize = 20u * c_oneMegabyte;

const MemSize c_umbraTomeCollectionPoolSize = 2u * ( c_umbraTomeCollectionMaxSize + c_umbraTomeCollectionOverhead );
const MemSize c_umbraPoolSize = 170u * c_oneMegabyte - c_umbraTomeCollectionPoolSize;
const MemSize c_objectPoolSize = 300u * c_oneMegabyte;
const MemSize c_gameSavePoolSize = 10u * c_oneMegabyte;
const MemSize c_ioPoolSize = 35u * c_oneMegabyte; // we need less memory for Durango
const MemSize c_envProbes = 13u * c_oneMegabyte;
const MemSize c_defaultPoolSize = c_oneGigabyte + 895u * c_oneMegabyte;
const MemSize c_inPlaceDefragTemp = ( 8 * c_oneMegabyte ) + ( 1024 * 4 );
const MemSize c_inPlaceRenderData = (507 * c_oneMegabyte);

////////////////////////////// CORE MEMORY //////////////////////////////////

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
		return RED_MEMORY_GET_ALLOCATOR( MemoryPool_Default )->OwnsPointer( ptr );
	};

	RED_MEMORY_POOL_TYPE( MemoryPool_Default )::CreationParameters			DefaultPoolParameters( c_defaultPoolSize, c_defaultPoolSize, c_defaultPoolSize );
	RED_MEMORY_POOL_TYPE( MemoryPool_Umbra )::CreationParameters			UmbraPoolParameters( c_umbraPoolSize, c_umbraPoolSize, c_umbraPoolSize );
	RED_MEMORY_POOL_TYPE( MemoryPool_UmbraTC )::CreationParameters			UmbraTomeCollectionPoolParameters( c_umbraTomeCollectionPoolSize, c_umbraTomeCollectionPoolSize, c_umbraTomeCollectionPoolSize );
	RED_MEMORY_POOL_TYPE( MemoryPool_SmallObjects )::CreationParameters		SmallObjectPoolParameters( smallObjectAlloc, smallObjectFree, smallObjectOwnership, 1024 * 256, 840 );	// 840x256k = 210mb reserved
	RED_MEMORY_POOL_TYPE( MemoryPool_CObjects )::CreationParameters			CObjectPoolParameters( c_objectPoolSize, c_objectPoolSize, c_objectPoolSize );
	RED_MEMORY_POOL_TYPE( MemoryPool_GameSave )::CreationParameters			GameSavePoolParameters( c_gameSavePoolSize, c_gameSavePoolSize, c_gameSavePoolSize );
	RED_MEMORY_POOL_TYPE( MemoryPool_IO )::CreationParameters				IOPoolParameters( c_ioPoolSize, c_ioPoolSize, c_ioPoolSize );
}

class R4MemoryParameters : public CoreMemory::CMemoryPoolParameters
{
public:
	R4MemoryParameters( )
	{
		SetPoolParameters( MemoryPool_Default, &R4PoolCreationParams::DefaultPoolParameters );
		SetPoolParameters( MemoryPool_Umbra, &R4PoolCreationParams::UmbraPoolParameters );
		SetPoolParameters( MemoryPool_UmbraTC, &R4PoolCreationParams::UmbraTomeCollectionPoolParameters );
		SetPoolParameters( MemoryPool_SmallObjects, &R4PoolCreationParams::SmallObjectPoolParameters );
		SetPoolParameters( MemoryPool_CObjects, &R4PoolCreationParams::CObjectPoolParameters );
		SetPoolParameters( MemoryPool_GameSave, &R4PoolCreationParams::GameSavePoolParameters );
		SetPoolParameters( MemoryPool_IO, &R4PoolCreationParams::IOPoolParameters );		
	}
};

#endif

////////////////////////////// GPU MEMORY //////////////////////////////////

namespace R4GpuPoolCreationParams
{
	const Uint32 c_gpuFlags = Red::MemoryFramework::Allocator_AccessCpuGpu | Red::MemoryFramework::Allocator_Cached | Red::MemoryFramework::Allocator_GpuNonCoherent | RED_MEMORY_DURANGO_ALLOCID( 7 );
	const Uint32 c_cpuFlags = Red::MemoryFramework::Allocator_AccessCpu | Red::MemoryFramework::Allocator_Cached | RED_MEMORY_DURANGO_ALLOCID( 8 );
	
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_Buffers )::CreationParameters			BufferPoolParameters( 32u * c_oneMegabyte, 32u * c_oneMegabyte, 32u * c_oneMegabyte );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_ConstantBuffers )::CreationParameters	ConstantBufferPoolParameters( 50u * c_oneMegabyte, 50u * c_oneMegabyte, 50u * c_oneMegabyte );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_ShaderInclude )::CreationParameters		ShaderIncludePoolParameters( 4u * c_oneMegabyte, 4u * c_oneMegabyte, 4u * c_oneMegabyte );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_Device )::CreationParameters			DevicePoolParameters( 40u * c_oneMegabyte, 40u * c_oneMegabyte, 40u * c_oneMegabyte );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_EnvProbes )::CreationParameters			EnvProbesPoolParameters( c_envProbes, 2 * 1024, c_gpuFlags, c_cpuFlags );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_InPlaceMeshBuffers )::CreationParameters InPlaceMeshParameters( 458u * c_oneMegabyte, 4u * c_oneMegabyte, c_gpuFlags, c_cpuFlags );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_InPlaceDefragTemp )::CreationParameters	InPlaceDefragTempPoolParameters( c_inPlaceDefragTemp, 1024, c_gpuFlags, c_cpuFlags | Red::MemoryFramework::Allocator_Use64kPages );
	
	// We need to deal with textures allocating into different pools depending on if we are loading cooked or uncooked files
	namespace LooseFileConfiguration
	{
		INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_Textures )::CreationParameters			TexturePoolParameters( 800u * c_oneMegabyte, 800u * c_oneMegabyte, 800u * c_oneMegabyte );
		INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_InPlaceRenderData )::CreationParameters	InPlacePoolParameters( 32u * c_oneMegabyte, 4u * c_oneMegabyte, c_gpuFlags, c_cpuFlags );
	}
	namespace CookedFileConfiguration
	{
		INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_Textures )::CreationParameters			TexturePoolParameters( 4u * c_oneMegabyte, 4u * c_oneMegabyte, 4u * c_oneMegabyte );
		INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_InPlaceRenderData )::CreationParameters	InPlacePoolParameters( c_inPlaceRenderData, 4u * c_oneMegabyte, c_gpuFlags, c_cpuFlags );
	}
}

class R4GpuMemoryParameters : public CoreMemory::CMemoryPoolParameters
{
public:
	R4GpuMemoryParameters( const Core::CommandLineArguments& commandLine )
	{
		SetPoolParameters( GpuApi::GpuMemoryPool_Buffers, &R4GpuPoolCreationParams::BufferPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_ConstantBuffers, &R4GpuPoolCreationParams::ConstantBufferPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_ShaderInclude, &R4GpuPoolCreationParams::ShaderIncludePoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_Device, &R4GpuPoolCreationParams::DevicePoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_EnvProbes, &R4GpuPoolCreationParams::EnvProbesPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_InPlaceMeshBuffers, &R4GpuPoolCreationParams::InPlaceMeshParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_InPlaceDefragTemp, &R4GpuPoolCreationParams::InPlaceDefragTempPoolParameters );

		if( commandLine.m_useBundles )
		{
			SetPoolParameters( GpuApi::GpuMemoryPool_Textures, &R4GpuPoolCreationParams::CookedFileConfiguration::TexturePoolParameters );
			SetPoolParameters( GpuApi::GpuMemoryPool_InPlaceRenderData, &R4GpuPoolCreationParams::CookedFileConfiguration::InPlacePoolParameters );
		}
		else
		{
			SetPoolParameters( GpuApi::GpuMemoryPool_Textures, &R4GpuPoolCreationParams::LooseFileConfiguration::TexturePoolParameters );
			SetPoolParameters( GpuApi::GpuMemoryPool_InPlaceRenderData, &R4GpuPoolCreationParams::LooseFileConfiguration::InPlacePoolParameters );
		}
	}
};
