/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/core/memory.h"
#include "../../common/gpuApiUtils/gpuApiMemory.h"

const MemSize c_oneMegabyte = 1024u * 1024u;
const MemSize c_oneGigabyte = c_oneMegabyte * 1024u;

const MemSize c_defaultPoolSize = c_oneGigabyte * 2;

////////////////////////////// CORE MEMORY //////////////////////////////////

class UnitTestMemoryParameters : public CoreMemory::CMemoryPoolParameters
{
public:
	UnitTestMemoryParameters()
	{
		static RED_MEMORY_POOL_TYPE( MemoryPool_Default )::CreationParameters			DefaultPoolParameters( c_defaultPoolSize, c_defaultPoolSize, c_defaultPoolSize );
		static RED_MEMORY_POOL_TYPE( MemoryPool_SpeedTree )::CreationParameters			SpeedTreePoolParameters( c_oneMegabyte * 128u, c_oneMegabyte * 192u, c_oneMegabyte * 64u );
		static RED_MEMORY_POOL_TYPE( MemoryPool_CObjects )::CreationParameters			CObjectPoolParameters( c_oneMegabyte * 128u, c_oneMegabyte * 512u, c_oneMegabyte * 64u );

		SetPoolParameters( MemoryPool_Default, &DefaultPoolParameters );
		SetPoolParameters( MemoryPool_SpeedTree, &SpeedTreePoolParameters );
		SetPoolParameters( MemoryPool_CObjects, &CObjectPoolParameters );
	}
};

////////////////////////////// GPU MEMORY //////////////////////////////////

class UnitTestGpuMemoryParameters : public CoreMemory::CMemoryPoolParameters
{
public:
	UnitTestGpuMemoryParameters()
	{
		static INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_Textures )::CreationParameters			TexturePoolParameters( 128u * c_oneMegabyte, 128u * c_oneMegabyte, 64u * c_oneMegabyte );
		static INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_Buffers )::CreationParameters			BufferPoolParameters( 32u * c_oneMegabyte, 32u * c_oneMegabyte, 32u * c_oneMegabyte );
		static INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_ShaderInclude )::CreationParameters		ShaderIncludePoolParameters( 4u * c_oneMegabyte, 4u * c_oneMegabyte, 4u * c_oneMegabyte );
		static INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_Device )::CreationParameters			DevicePoolParameters( 40u * c_oneMegabyte, 40u * c_oneMegabyte, 40u * c_oneMegabyte );

		SetPoolParameters( GpuApi::GpuMemoryPool_Textures, &TexturePoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_Buffers, &BufferPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_ShaderInclude, &ShaderIncludePoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_Device, &DevicePoolParameters );
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
			UnitTestMemoryParameters memoryParams;
			CoreMemory::Initialise( &memoryParams );

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