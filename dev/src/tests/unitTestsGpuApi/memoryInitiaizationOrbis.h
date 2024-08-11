/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef _MEMORY_INIT_HACK
#define _MEMORY_INIT_HACK

#include "../../common/core/memory.h"
#include "../../common/gpuApiUtils/gpuApiMemory.h"

const MemSize c_oneMegabyte = 1024u * 1024u;
const MemSize c_oneGigabyte = c_oneMegabyte * 1024u;

////////////////////////////// CORE MEMORY //////////////////////////////////

const MemSize c_defaultPoolSize = ( c_oneGigabyte * 2 ) - ( c_oneMegabyte * 120 );		// 2gb - 120mb from gpu api device

#ifndef RED_USE_NEW_MEMORY_SYSTEM

class UnitTestsMemoryParameters : public CoreMemory::CMemoryPoolParameters
{
public:
	UnitTestsMemoryParameters()
	{
		static 	RED_MEMORY_POOL_TYPE( MemoryPool_Default )::CreationParameters			DefaultPoolParameters( c_defaultPoolSize, c_defaultPoolSize, c_defaultPoolSize );
		static  RED_MEMORY_POOL_TYPE( MemoryPool_CObjects )::CreationParameters			CObjectPoolParameters( c_oneMegabyte * 128u, c_oneMegabyte * 512u, c_oneMegabyte * 64u );

		SetPoolParameters( MemoryPool_Default, &DefaultPoolParameters );
		SetPoolParameters( MemoryPool_CObjects, &CObjectPoolParameters );
	}
};

#endif

////////////////////////////// GPU MEMORY //////////////////////////////////

const MemSize c_defaultOnion		= 186 * c_oneMegabyte;
const MemSize c_smallOnion			= 2 * c_oneMegabyte;
const MemSize c_constantBuffers		= 60 * c_oneMegabyte;
const MemSize c_defaultGarlic		= 24u * c_oneMegabyte;
const MemSize c_GPUInternal			= 16u * c_oneMegabyte;
const MemSize c_swapChain			= 20u * c_oneMegabyte;
const MemSize c_RenderTargets		= 400u * c_oneMegabyte;
const MemSize c_renderData			= 884u * c_oneMegabyte;
const MemSize c_inplaceData			= 560u * c_oneMegabyte;

class UnitTestGpuMemoryParameters : public CoreMemory::CMemoryPoolParameters
{
public:
	UnitTestGpuMemoryParameters()
	{

		const Uint32 c_gpuFlags = Red::MemoryFramework::Allocator_DirectMemory | Red::MemoryFramework::Allocator_UseGarlicBus | Red::MemoryFramework::Allocator_AccessGpuReadWrite | Red::MemoryFramework::Allocator_AccessCpuWrite | Red::MemoryFramework::Allocator_TextureMemory;
		const Uint32 c_cpuFlags = Red::MemoryFramework::Allocator_DirectMemory | Red::MemoryFramework::Allocator_UseOnionBus | Red::MemoryFramework::Allocator_AccessCpuReadWrite;

		static INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_DefaultOnion )::CreationParameters			DefaultOnionPoolParameters( c_defaultOnion, c_defaultOnion, c_defaultOnion );	
		static INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_SmallOnion )::CreationParameters				SmallOnionPoolParameters( c_smallOnion, c_smallOnion, c_smallOnion );
		static INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_ConstantBuffers )::CreationParameters		ConstantBuffersPoolParameters( c_constantBuffers, c_constantBuffers, c_constantBuffers );
		static INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_DefaultGarlic )::CreationParameters			DefaultGarlicPoolParameters( c_defaultGarlic, c_defaultGarlic, c_defaultGarlic );
		static INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_GPUInternal )::CreationParameters			GpuInternalPoolParameters( c_GPUInternal, c_GPUInternal, c_GPUInternal );
		static INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_SwapChain )::CreationParameters				SwapChainPoolParameters( c_swapChain, c_swapChain, c_swapChain );
		static INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_RenderTargets )::CreationParameters			RenderTargetsPoolParameters( c_RenderTargets, c_oneMegabyte * 4, c_gpuFlags, c_cpuFlags );
		static INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_RenderData )::CreationParameters				RenderDataPoolParameters( c_renderData, c_oneMegabyte * 4, c_gpuFlags, c_cpuFlags );
		static INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_InPlaceRenderData )::CreationParameters		InPlacePoolParameters( c_inplaceData, c_oneMegabyte * 4, c_gpuFlags, c_cpuFlags );

		SetPoolParameters( GpuApi::GpuMemoryPool_DefaultOnion, &DefaultOnionPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_SmallOnion, &SmallOnionPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_ConstantBuffers, &ConstantBuffersPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_DefaultGarlic, &DefaultGarlicPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_GPUInternal, &GpuInternalPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_SwapChain, &SwapChainPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_RenderTargets, &RenderTargetsPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_RenderData, &RenderDataPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_InPlaceRenderData, &InPlacePoolParameters );
	}
};

class MemoryInitializer
{
public:
	static void InitializeMemory()
	{
		static Bool memInitialized = false;
		if( memInitialized == false )
		{
			RED_MEMORY_INITIALIZE( UnitTestsMemoryParameters );

			// Pass pool parameters to gpu memory system
			auto poolParameterFn = []( Red::MemoryFramework::PoolLabel l, const Red::MemoryFramework::IAllocatorCreationParameters* p ) 
			{ 
				GpuApi::SetPoolParameters( l, p ); 
			};
			UnitTestGpuMemoryParameters gpuMemoryPoolParameters;
			gpuMemoryPoolParameters.ForEachPoolParameter( poolParameterFn );

			memInitialized = true;
		}
	}
};

#endif