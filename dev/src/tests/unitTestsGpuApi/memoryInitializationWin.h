#pragma once
#ifndef _MEMORY_INIT_HACK
#define _MEMORY_INIT_HACK

#include "../../common/core/memory.h"
#include "../../common/gpuApiUtils/gpuApiMemory.h"

const MemSize c_oneMegabyte = 1024u * 1024u;
const MemSize c_oneGigabyte = c_oneMegabyte * 1024u;

#ifdef RED_PLATFORM_WIN32
const MemSize c_defaultPoolInitialSize = c_oneMegabyte * 512u;
const MemSize c_defaultPoolMaxSize = c_oneGigabyte * 2 + (c_oneGigabyte / 2);
const MemSize c_defaultPoolGranularity = c_oneMegabyte * 32u;
#elif defined( RED_PLATFORM_WIN64 )
const MemSize c_defaultPoolInitialSize = c_oneGigabyte;
const MemSize c_defaultPoolMaxSize = c_oneGigabyte * 3;
const MemSize c_defaultPoolGranularity = c_oneMegabyte * 64u;
#endif

#ifndef RED_USE_NEW_MEMORY_SYSTEM

class UnitTestMemoryParameters : public CoreMemory::CMemoryPoolParameters
{
public:
	UnitTestMemoryParameters()
	{
		static RED_MEMORY_POOL_TYPE( MemoryPool_Default )::CreationParameters		DefaultPoolParameters( c_defaultPoolInitialSize, c_defaultPoolMaxSize, c_defaultPoolGranularity );

		SetPoolParameters( MemoryPool_Default, &DefaultPoolParameters );
	}
};

#endif

class UnitTestGpuMemoryParameters : public CoreMemory::CMemoryPoolParameters
{
public:
	UnitTestGpuMemoryParameters()
	{
		static INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_Textures )::CreationParameters			TexturePoolParameters( 72u * c_oneMegabyte, 216u * c_oneMegabyte, 72u * c_oneMegabyte );
		static INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_Buffers )::CreationParameters			BufferPoolParameters( 72u * c_oneMegabyte, 216u * c_oneMegabyte, 72u * c_oneMegabyte );
		static INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_LockedBufferData )::CreationParameters	LockedBufferPoolParameters( 8u * c_oneMegabyte, 64u * c_oneMegabyte, 8u * c_oneMegabyte );
		static INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_ShaderInclude )::CreationParameters		ShaderIncludePoolParameters( c_oneMegabyte, c_oneMegabyte, c_oneMegabyte );
		static INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_Device )::CreationParameters				DevicePoolParameters( 52u * c_oneMegabyte, 52u * c_oneMegabyte, 52u * c_oneMegabyte );
		static INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_InPlaceRenderData )::CreationParameters	InPlacePoolParameters( 512u * c_oneMegabyte, 4u * c_oneMegabyte, 0, 0 );

		SetPoolParameters( GpuApi::GpuMemoryPool_Textures, &TexturePoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_Buffers, &BufferPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_LockedBufferData, &LockedBufferPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_ShaderInclude, &ShaderIncludePoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_Device, &DevicePoolParameters );
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
			RED_MEMORY_INITIALIZE( UnitTestMemoryParameters );

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

