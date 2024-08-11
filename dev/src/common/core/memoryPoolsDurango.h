// Macro Usage:
// To add a new pool, add DECLARE_MEMORY_POOL(Pool Label, Allocator Type, Pool flags)

// Default flags for physical memory (cached, mapped to CPU only)
#define DEFAULT_POOL_FLAGS	Red::MemoryFramework::Allocator_AccessCpu | Red::MemoryFramework::Allocator_Cached | Red::MemoryFramework::Allocator_StaticSize

#ifdef DECLARE_MEMORY_POOL

	// Default memory pool used by most things in the engine
	DECLARE_MEMORY_POOL( MemoryPool_Default, Red::MemoryFramework::TLSFAllocatorThreadSafe, DEFAULT_POOL_FLAGS | RED_MEMORY_DURANGO_ALLOCID( 0 ) )

	// Umbra pool
	DECLARE_MEMORY_POOL( MemoryPool_Umbra, Red::MemoryFramework::TLSFAllocatorSpinlocked, DEFAULT_POOL_FLAGS | RED_MEMORY_DURANGO_ALLOCID( 1 ) )
	DECLARE_MEMORY_POOL( MemoryPool_UmbraTC, Red::MemoryFramework::TLSFAllocator, DEFAULT_POOL_FLAGS | RED_MEMORY_DURANGO_ALLOCID( 1 ) )

	// Terrain editor pool. Editor / Windows only. Don't initialise it!
	DECLARE_MEMORY_POOL( MemoryPool_TerrainEditor, Red::MemoryFramework::TLSFAllocatorSpinlocked, DEFAULT_POOL_FLAGS | RED_MEMORY_DURANGO_ALLOCID( 2 ) )

	// Loading/IO pool 
	DECLARE_MEMORY_POOL( MemoryPool_IO, Red::MemoryFramework::TLSFAllocatorSpinlocked, DEFAULT_POOL_FLAGS | RED_MEMORY_DURANGO_ALLOCID( 0 ) | Red::MemoryFramework::Allocator_NoBreakOnOOM )

	// Script compilation pool (Use default pool on console)
	#define MemoryPool_ScriptCompilation MemoryPool_Default

	// Small object allocator
	DECLARE_MEMORY_POOL( MemoryPool_SmallObjects, Red::MemoryFramework::SmallBlockAllocatorThreadSafe, DEFAULT_POOL_FLAGS )

	DECLARE_MEMORY_POOL( MemoryPool_GameSave, Red::MemoryFramework::TLSFAllocatorSpinlocked, DEFAULT_POOL_FLAGS )

	// CObject allocator
	DECLARE_MEMORY_POOL( MemoryPool_CObjects, Red::MemoryFramework::TLSFAllocatorSpinlocked, DEFAULT_POOL_FLAGS )

	// All pools go to the main one on consoles for now
	#define MemoryPool_Task				MemoryPool_Default
	#define MemoryPool_SpeedTree		MemoryPool_Default
	#define MemoryPool_OperatorNew		MemoryPool_Default
	#define MemoryPool_FoliageData		MemoryPool_Default
	#define MemoryPool_Audio			MemoryPool_Default
	#define MemoryPool_Strings			MemoryPool_Default
	#define MemoryPool_RedGui			MemoryPool_Default
	#define MemoryPool_Animation		MemoryPool_Default
	#define MemoryPool_Physics			MemoryPool_Default
	#define MemoryPool_Debug			MemoryPool_Default

#else
	#error DECLARE_MEMORY_POOL must be defined to include memoryPoolsDurango.h
#endif
