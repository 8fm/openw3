/**
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../redMemory/include/pool.h"
#include "../redMemory/include/defaultAllocator.h"
#include "../redMemory/include/fixedSizeAllocator.h"

namespace red
{
namespace memory
{
	class DefaultAllocator;
	class DynamicFixedSizeAllocator;
	class LockingDynamicTLSFAllocator;
}
}

typedef red::memory::PoolDefault MemoryPool_Default;
typedef MemoryPool_Default MemoryPool_SmallObjects;

RED_MEMORY_POOL_STATIC( MemoryPool_Umbra, red::memory::LockingDynamicTLSFAllocator );
RED_MEMORY_POOL_STATIC( MemoryPool_UmbraTC, red::memory::DynamicFixedSizeAllocator );

RED_MEMORY_POOL_STATIC( MemoryPool_IO, red::memory::LockingDynamicTLSFAllocator );

RED_MEMORY_POOL_STATIC( MemoryPool_GameSave, red::memory::DefaultAllocator );

RED_MEMORY_POOL_STATIC( MemoryPool_CObjects, red::memory::DefaultAllocator );

#ifdef RED_PLATFORM_ORBIS
RED_MEMORY_POOL_STATIC( MemoryPool_Audio, red::memory::LockingDynamicTLSFAllocator );
#else
RED_MEMORY_POOL_STATIC( MemoryPool_Audio, red::memory::DefaultAllocator );
#endif

RED_MEMORY_POOL_STATIC( MemoryPool_Strings, red::memory::DefaultAllocator );
RED_MEMORY_POOL_STATIC( MemoryPool_Task, red::memory::DefaultAllocator );
RED_MEMORY_POOL_STATIC( MemoryPool_Physics, red::memory::DefaultAllocator );
RED_MEMORY_POOL_STATIC( MemoryPool_Animation, red::memory::DefaultAllocator );
RED_MEMORY_POOL_STATIC( MemoryPool_FoliageData, red::memory::DefaultAllocator );
RED_MEMORY_POOL_STATIC( MemoryPool_RedGui, red::memory::DefaultAllocator );
RED_MEMORY_POOL_STATIC( MemoryPool_SpeedTree, red::memory::DefaultAllocator );

RED_MEMORY_POOL_STATIC( MemoryPool_Debug, red::memory::DefaultAllocator );
RED_MEMORY_POOL_STATIC( MemoryPool_TerrainEditor, red::memory::DefaultAllocator );
RED_MEMORY_POOL_STATIC( MemoryPool_ScriptCompilation, red::memory::DefaultAllocator );
