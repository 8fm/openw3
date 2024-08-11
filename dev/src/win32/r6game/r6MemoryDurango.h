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

	RED_MEMORY_POOL_TYPE( MemoryPool_Default )::CreationParameters			DefaultPoolParameters( c_oneGigabyte * 2, c_oneGigabyte * 2, c_oneGigabyte / 2 );
	RED_MEMORY_POOL_TYPE( MemoryPool_SpeedTree )::CreationParameters		SpeedTreePoolParameters( c_oneMegabyte * 128u, c_oneMegabyte * 256u, c_oneMegabyte * 64u );
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
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_Textures )::CreationParameters			TexturePoolParameters( 72u * c_oneMegabyte, 216u * c_oneMegabyte, 72u * c_oneMegabyte );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_Buffers )::CreationParameters			BufferPoolParameters( 72u * c_oneMegabyte, 216u * c_oneMegabyte, 72u * c_oneMegabyte );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_ShaderInclude )::CreationParameters		ShaderIncludePoolParameters( 4u * c_oneMegabyte, 4u * c_oneMegabyte, 4u * c_oneMegabyte );
	INTERNAL_RED_MEMORY_POOL_TYPE( GpuApi::GpuMemory, GpuApi::GpuMemoryPool_Device )::CreationParameters			DevicePoolParameters( 40u * c_oneMegabyte, 40u * c_oneMegabyte, 40u * c_oneMegabyte );
}

class R6GpuMemoryParameters : public CoreMemory::CMemoryPoolParameters
{
public:
	R4GpuMemoryParameters()
	{
		SetPoolParameters( GpuApi::GpuMemoryPool_Textures, &R6GpuPoolCreationParams::TexturePoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_Buffers, &R6GpuPoolCreationParams::BufferPoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_ShaderInclude, &R6GpuPoolCreationParams::ShaderIncludePoolParameters );
		SetPoolParameters( GpuApi::GpuMemoryPool_Device, &R6GpuPoolCreationParams::DevicePoolParameters );
	}
};