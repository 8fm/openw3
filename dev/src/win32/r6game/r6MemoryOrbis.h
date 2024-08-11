/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/core/memory.h"
#include "../../common/gpuApiUtils/gpuApiMemory.h"

const MemSize c_oneMegabyte = 1024u * 1024u;
const MemSize c_oneGigabyte = c_oneMegabyte * 1024u;

////////////////////////////// CORE MEMORY //////////////////////////////////

// Memory pool initialisers
namespace R6PoolCreationParams
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

	RED_MEMORY_POOL_TYPE( MemoryPool_Default )::CreationParameters			DefaultPoolParameters( c_oneGigabyte, c_oneGigabyte, c_oneGigabyte );
	RED_MEMORY_POOL_TYPE( MemoryPool_SpeedTree )::CreationParameters		SpeedTreePoolParameters( 384u * c_oneMegabyte, 384u * c_oneMegabyte, 384u * c_oneMegabyte );
	RED_MEMORY_POOL_TYPE( MemoryPool_SmallObjects )::CreationParameters		SmallObjectPoolParameters( smallObjectAlloc, smallObjectFree, smallObjectOwnership, 1024 * 256 );
}

class R6MemoryParameters : public CoreMemory::CMemoryPoolParameters
{
public:
	R6MemoryParameters()
	{
		SetPoolParameters( MemoryPool_Default, &R6PoolCreationParams::DefaultPoolParameters );
		SetPoolParameters( MemoryPool_SpeedTree, &R6PoolCreationParams::SpeedTreePoolParameters );
		SetPoolParameters( MemoryPool_SmallObjects, &R6PoolCreationParams::SmallObjectPoolParameters );
	}
};

////////////////////////////// GPU MEMORY //////////////////////////////////

namespace R6GpuPoolCreationParams
{
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_DisplayBuffer_Onion )::CreationParameters	DisplayBufferOnionPoolParameters( 2u * c_oneMegabyte, 128u * c_oneMegabyte, 2u * c_oneMegabyte );	
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_DisplayBuffer_Garlic )::CreationParameters	DisplayBufferGarlicPoolParameters( 2u * c_oneMegabyte, 128u * c_oneMegabyte, 2u * c_oneMegabyte );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_GPUInternal )::CreationParameters			GpuInternalPoolParameters( 2u * c_oneMegabyte, 32u * c_oneMegabyte, 2u * c_oneMegabyte );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_Shaders )::CreationParameters				ShadersPoolParameters( 2u * c_oneMegabyte, 32u * c_oneMegabyte, 2u * c_oneMegabyte );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_ShaderInclude )::CreationParameters			ShaderIncludePoolParameters( c_oneMegabyte, c_oneMegabyte, c_oneMegabyte );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_Misc )::CreationParameters					MiscPoolParameters( 2u * c_oneMegabyte, 32u * c_oneMegabyte, 2u * c_oneMegabyte );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_Textures )::CreationParameters				TexturePoolParameters( 32u * c_oneMegabyte, 960u * c_oneMegabyte, 64u * c_oneMegabyte );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_Buffers )::CreationParameters				BufferPoolParameters( 32u * c_oneMegabyte, 512u * c_oneMegabyte, 32u * c_oneMegabyte );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_BuffersDynamic )::CreationParameters		DynamicBufferPoolParameters( 32u * c_oneMegabyte, 128u * c_oneMegabyte, 32u * c_oneMegabyte );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_LockedBufferData )::CreationParameters		LockedBuffersPoolParameters( 8u * c_oneMegabyte, 64u * c_oneMegabyte, 8u * c_oneMegabyte );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_Device )::CreationParameters				DevicePoolParameters( 120u * c_oneMegabyte, 120u * c_oneMegabyte, 120u * c_oneMegabyte );
}

class R6GpuMemoryParameters : public CoreMemory::CMemoryPoolParameters
{
public:
	R6GpuMemoryParameters()
	{
		SetPoolParameters( GpuApi::GpuMemoryPool_DisplayBuffer_Onion, &R6GpuPoolCreationParams::DisplayBufferOnionPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_DisplayBuffer_Garlic, &R6GpuPoolCreationParams::DisplayBufferGarlicPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_GPUInternal, &R6GpuPoolCreationParams::GpuInternalPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_Shaders, &R6GpuPoolCreationParams::ShadersPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_ShaderInclude, &R6GpuPoolCreationParams::ShaderIncludePoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_Misc, &R6GpuPoolCreationParams::MiscPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_Textures, &R6GpuPoolCreationParams::TexturePoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_Buffers, &R6GpuPoolCreationParams::BufferPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_BuffersDynamic, &R6GpuPoolCreationParams::DynamicBufferPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_LockedBufferData, &R6GpuPoolCreationParams::LockedBuffersPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_Device, &R6GpuPoolCreationParams::DevicePoolParameters );
	}
};