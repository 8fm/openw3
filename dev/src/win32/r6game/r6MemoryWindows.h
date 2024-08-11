/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

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

// Memory pool initialisers
namespace R6PoolCreationParams
{
	RED_MEMORY_POOL_TYPE( MemoryPool_Default )::CreationParameters			DefaultPoolParameters( c_defaultPoolInitialSize, c_defaultPoolMaxSize, c_defaultPoolGranularity );
	RED_MEMORY_POOL_TYPE( MemoryPool_SpeedTree )::CreationParameters		SpeedTreePoolParameters( 128u * c_oneMegabyte, c_oneGigabyte / 2, c_defaultPoolGranularity );
	RED_MEMORY_POOL_TYPE( MemoryPool_ScriptCompilation )::CreationParameters	ScriptCompilationPoolParameters( 128u * c_oneMegabyte, c_oneGigabyte, c_defaultPoolGranularity );
}

class R6MemoryParameters : public CoreMemory::CMemoryPoolParameters
{
public:
	R6MemoryParameters()
	{
		SetPoolParameters( MemoryPool_Default, &R6PoolCreationParams::DefaultPoolParameters );
		SetPoolParameters( MemoryPool_SpeedTree, &R6PoolCreationParams::SpeedTreePoolParameters );
		SetPoolParameters( MemoryPool_ScriptCompilation, &R6PoolCreationParams::ScriptCompilationPoolParameters );
	}
};

////////////////////////////// GPU MEMORY //////////////////////////////////

namespace R6GpuPoolCreationParams
{
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_Textures )::CreationParameters			TexturePoolParameters( 72u * c_oneMegabyte, 216u * c_oneMegabyte, 72u * c_oneMegabyte );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_Buffers )::CreationParameters			BufferPoolParameters( 72u * c_oneMegabyte, 216u * c_oneMegabyte, 72u * c_oneMegabyte );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_LockedBufferData )::CreationParameters	LockedBufferPoolParameters( 8u * c_oneMegabyte, 64u * c_oneMegabyte, 8u * c_oneMegabyte );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_ShaderInclude )::CreationParameters		ShaderIncludePoolParameters( c_oneMegabyte, c_oneMegabyte, c_oneMegabyte );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_Device )::CreationParameters			DevicePoolParameters( 52u * c_oneMegabyte, 52u * c_oneMegabyte, 52u * c_oneMegabyte );
}

class R6GpuMemoryParameters : public CoreMemory::CMemoryPoolParameters
{
public:
	R6GpuMemoryParameters()
	{
		SetPoolParameters( GpuApi::GpuMemoryPool_Textures, &R6GpuPoolCreationParams::TexturePoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_Buffers, &R6GpuPoolCreationParams::BufferPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_LockedBufferData, &R6GpuPoolCreationParams::LockedBufferPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_ShaderInclude, &R6GpuPoolCreationParams::ShaderIncludePoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_Device, &R6GpuPoolCreationParams::DevicePoolParameters );
	}
};