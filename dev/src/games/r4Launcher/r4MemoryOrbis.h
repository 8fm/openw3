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
const MemSize c_objectPoolSize = 300u * c_oneMegabyte;
const MemSize c_gameSavePoolSize = 10u * c_oneMegabyte;
const MemSize c_ioPoolSize = 35u * c_oneMegabyte; // we need less memory for PS4
const MemSize c_defaultPoolSize = c_oneGigabyte + 834 * c_oneMegabyte;

#ifdef RED_CONFIGURATION_NOPTS
const MemSize c_audioPoolSize = 220u * c_oneMegabyte;	// ON FLEXIBLE
#else
const MemSize c_audioPoolSize = 240u * c_oneMegabyte;	// ON FLEXIBLE
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
		return RED_MEMORY_GET_ALLOCATOR( MemoryPool_Default )->OwnsPointer( ptr );
	};

	RED_MEMORY_POOL_TYPE( MemoryPool_Default )::CreationParameters			DefaultPoolParameters( c_defaultPoolSize, c_defaultPoolSize, c_defaultPoolSize );
	RED_MEMORY_POOL_TYPE( MemoryPool_Umbra )::CreationParameters			UmbraPoolParameters( c_umbraPoolSize, c_umbraPoolSize, c_umbraPoolSize );
	RED_MEMORY_POOL_TYPE( MemoryPool_UmbraTC )::CreationParameters			UmbraTomeCollectionPoolParameters( c_umbraTomeCollectionPoolSize, c_umbraTomeCollectionPoolSize, c_umbraTomeCollectionPoolSize );
	RED_MEMORY_POOL_TYPE( MemoryPool_SpeedTree )::CreationParameters		SpeedTreePoolParameters( c_oneMegabyte * 128u, c_oneMegabyte * 128u, c_oneMegabyte * 128u );
	RED_MEMORY_POOL_TYPE( MemoryPool_SmallObjects )::CreationParameters		SmallObjectPoolParameters( smallObjectAlloc, smallObjectFree, smallObjectOwnership, 1024 * 256, 840 );	// 840x256k = 210mb reserved
	RED_MEMORY_POOL_TYPE( MemoryPool_CObjects )::CreationParameters			CObjectPoolParameters( c_objectPoolSize, c_objectPoolSize, c_objectPoolSize );
	RED_MEMORY_POOL_TYPE( MemoryPool_GameSave )::CreationParameters			GameSavePoolParameters( c_gameSavePoolSize, c_gameSavePoolSize, c_gameSavePoolSize );
	RED_MEMORY_POOL_TYPE( MemoryPool_IO )::CreationParameters				IOPoolParameters( c_ioPoolSize, c_ioPoolSize, c_ioPoolSize );


	/*THIS IS ON FLEXIBLE!!!*/
	RED_MEMORY_POOL_TYPE( MemoryPool_Audio )::CreationParameters			AudioPoolParameters( c_audioPoolSize, c_audioPoolSize, c_audioPoolSize );
	/* - */

}

class R4MemoryParameters : public CoreMemory::CMemoryPoolParameters
{
public:
	R4MemoryParameters( )
	{
		SetPoolParameters( MemoryPool_Default, &R4PoolCreationParams::DefaultPoolParameters );
		SetPoolParameters( MemoryPool_Umbra, &R4PoolCreationParams::UmbraPoolParameters );
		SetPoolParameters( MemoryPool_UmbraTC, &R4PoolCreationParams::UmbraTomeCollectionPoolParameters );
		SetPoolParameters( MemoryPool_SpeedTree, &R4PoolCreationParams::SpeedTreePoolParameters );
		SetPoolParameters( MemoryPool_SmallObjects, &R4PoolCreationParams::SmallObjectPoolParameters );
		SetPoolParameters( MemoryPool_CObjects, &R4PoolCreationParams::CObjectPoolParameters );
		SetPoolParameters( MemoryPool_GameSave, &R4PoolCreationParams::GameSavePoolParameters );
		SetPoolParameters( MemoryPool_IO, &R4PoolCreationParams::IOPoolParameters );
		SetPoolParameters( MemoryPool_Audio, &R4PoolCreationParams::AudioPoolParameters );
	}
};

#endif

////////////////////////////// RENDERER MEMORY //////////////////////////////////


const MemSize c_defaultOnion		= 186 * c_oneMegabyte;
const MemSize c_smallOnion			= 2 * c_oneMegabyte;
const MemSize c_constantBuffers		= 12 * c_oneMegabyte;	 // FLEXIBLE MEMORY
const MemSize c_defaultGarlic		= 32u * c_oneMegabyte;
const MemSize c_GPUInternal			= 16u * c_oneMegabyte;
const MemSize c_swapChain			= 20u * c_oneMegabyte;
const MemSize c_RenderTargets		= 420u * c_oneMegabyte;
const MemSize c_envProbes			= 13u * c_oneMegabyte;
const MemSize c_renderData			= 858u * c_oneMegabyte;
const MemSize c_inplaceData			= 539u * c_oneMegabyte;
const MemSize c_inPlaceDefragTemp	= ( 8 * c_oneMegabyte ) + ( 1024 * 4 );

const MemSize c_looseFileRenderData = 1758u * c_oneMegabyte;
const MemSize c_looseFileInplaceData = 32u * c_oneMegabyte;

namespace R4GpuPoolCreationParams
{
	const Uint32 c_gpuFlags = Red::MemoryFramework::Allocator_DirectMemory | Red::MemoryFramework::Allocator_UseGarlicBus | Red::MemoryFramework::Allocator_AccessGpuReadWrite | Red::MemoryFramework::Allocator_AccessCpuWrite | Red::MemoryFramework::Allocator_TextureMemory;
	const Uint32 c_cpuFlags = Red::MemoryFramework::Allocator_DirectMemory | Red::MemoryFramework::Allocator_UseOnionBus | Red::MemoryFramework::Allocator_AccessCpuReadWrite;

	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_DefaultOnion )::CreationParameters			DefaultOnionPoolParameters( c_defaultOnion, c_defaultOnion, c_defaultOnion );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_SmallOnion )::CreationParameters			SmallOnionPoolParameters( c_smallOnion, c_smallOnion, c_smallOnion );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_ConstantBuffers )::CreationParameters		ConstantBuffersPoolParameters( c_constantBuffers, c_constantBuffers, c_constantBuffers );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_DefaultGarlic )::CreationParameters			DefaultGarlicPoolParameters( c_defaultGarlic, c_defaultGarlic, c_defaultGarlic );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_GPUInternal )::CreationParameters			GpuInternalPoolParameters( c_GPUInternal, c_GPUInternal, c_GPUInternal );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_SwapChain )::CreationParameters				SwapChainPoolParameters( c_swapChain, c_swapChain, c_swapChain );

	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_RenderTargets )::CreationParameters			RenderTargetsPoolParameters( c_RenderTargets, c_oneMegabyte * 4, c_gpuFlags, c_cpuFlags );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_EnvProbes )::CreationParameters				EnvProbesPoolParameters( c_envProbes, 2 * 1024, c_gpuFlags, c_cpuFlags );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_InPlaceDefragTemp )::CreationParameters		InPlaceDefragTempPoolParameters( c_inPlaceDefragTemp, 1024, c_gpuFlags, c_cpuFlags );

	namespace LooseFileConfiguration
	{
		INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_RenderData )::CreationParameters			RenderDataPoolParameters( c_looseFileRenderData, c_oneMegabyte * 4, c_gpuFlags, c_cpuFlags );
		INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_InPlaceRenderData )::CreationParameters		InPlacePoolParameters( c_looseFileInplaceData, c_oneMegabyte * 4, c_gpuFlags, c_cpuFlags );
	}
	
	namespace CookedFileConfiguration
	{
		INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_RenderData )::CreationParameters			RenderDataPoolParameters( c_renderData, c_oneMegabyte * 4, c_gpuFlags, c_cpuFlags );
		INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_InPlaceRenderData )::CreationParameters		InPlacePoolParameters( c_inplaceData, c_oneMegabyte * 4, c_gpuFlags, c_cpuFlags );
	}
}

class R4GpuMemoryParameters : public CoreMemory::CMemoryPoolParameters
{
public:
	R4GpuMemoryParameters( const Core::CommandLineArguments& commandLine )
	{
		SetPoolParameters( GpuApi::GpuMemoryPool_DefaultOnion, &R4GpuPoolCreationParams::DefaultOnionPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_SmallOnion, &R4GpuPoolCreationParams::SmallOnionPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_ConstantBuffers, &R4GpuPoolCreationParams::ConstantBuffersPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_DefaultGarlic, &R4GpuPoolCreationParams::DefaultGarlicPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_GPUInternal, &R4GpuPoolCreationParams::GpuInternalPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_SwapChain, &R4GpuPoolCreationParams::SwapChainPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_RenderTargets, &R4GpuPoolCreationParams::RenderTargetsPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_EnvProbes, &R4GpuPoolCreationParams::EnvProbesPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_InPlaceDefragTemp, &R4GpuPoolCreationParams::InPlaceDefragTempPoolParameters );

		if( commandLine.m_useBundles )
		{
			SetPoolParameters( GpuApi::GpuMemoryPool_RenderData, &R4GpuPoolCreationParams::CookedFileConfiguration::RenderDataPoolParameters );
			SetPoolParameters( GpuApi::GpuMemoryPool_InPlaceRenderData, &R4GpuPoolCreationParams::CookedFileConfiguration::InPlacePoolParameters );
		}
		else
		{	
			SetPoolParameters( GpuApi::GpuMemoryPool_RenderData, &R4GpuPoolCreationParams::LooseFileConfiguration::RenderDataPoolParameters );
			SetPoolParameters( GpuApi::GpuMemoryPool_InPlaceRenderData, &R4GpuPoolCreationParams::LooseFileConfiguration::InPlacePoolParameters );
		}
	}
};
