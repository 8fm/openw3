// Macro Usage:
// To add a new pool, add DECLARE_MEMORY_POOL(Pool Label, Allocator Type, Pool flags)

#define TLSF_ALLOCATOR_TYPE Red::MemoryFramework::TLSFAllocatorThreadSafe
#define SMALL_ALLOCATOR_TYPE Red::MemoryFramework::SmallBlockAllocatorThreadSafe

#ifdef DECLARE_MEMORY_POOL

	// Default memory pool used by most things in the engine
	DECLARE_MEMORY_POOL( MemoryPool_Default, TLSF_ALLOCATOR_TYPE, 0 )

	// Umbra pool
	DECLARE_MEMORY_POOL( MemoryPool_Umbra, TLSF_ALLOCATOR_TYPE, 0 )
	DECLARE_MEMORY_POOL( MemoryPool_UmbraTC, TLSF_ALLOCATOR_TYPE, 0 )

	// Terrain editor pool. Editor / Windows only
	DECLARE_MEMORY_POOL( MemoryPool_TerrainEditor, TLSF_ALLOCATOR_TYPE, 0 )

	// Script compilation pool. (Only when script compilation is enabled)
	DECLARE_MEMORY_POOL( MemoryPool_ScriptCompilation, TLSF_ALLOCATOR_TYPE, 0 )

	// Physics pool 
	DECLARE_MEMORY_POOL( MemoryPool_Physics, TLSF_ALLOCATOR_TYPE, 0 )

	// Small object allocator
	DECLARE_MEMORY_POOL( MemoryPool_SmallObjects, SMALL_ALLOCATOR_TYPE, 0 )

	DECLARE_MEMORY_POOL( MemoryPool_GameSave, TLSF_ALLOCATOR_TYPE, 0 )

	// Loading/IO pool 
	DECLARE_MEMORY_POOL( MemoryPool_IO, TLSF_ALLOCATOR_TYPE, Red::MemoryFramework::Allocator_NoBreakOnOOM )

#ifdef CORE_USES_DEBUG_ALLOCATOR
	// Debug allocator used for catching memory corruption. Requires lots of memory
	DECLARE_MEMORY_POOL( MemoryPool_DebugAllocator, Red::MemoryFramework::DebugAllocator, 0 )
#endif

	// Task manager pool defers to the default memory pool.
	#define MemoryPool_Task				MemoryPool_Default
	#define MemoryPool_SpeedTree		MemoryPool_Default
	#define MemoryPool_OperatorNew		MemoryPool_Default
	#define MemoryPool_FoliageData		MemoryPool_Default
	#define MemoryPool_Audio			MemoryPool_Default
	#define MemoryPool_Animation		MemoryPool_Default
	#define MemoryPool_Strings			MemoryPool_Default
	#define MemoryPool_RedGui			MemoryPool_Default
	#define MemoryPool_CObjects			MemoryPool_Default
	#define MemoryPool_Debug			MemoryPool_Default

#else
	#error DECLARE_MEMORY_POOL must be defined to include memoryPools.h
#endif
