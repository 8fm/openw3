/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_MANAGER_H
#define _RED_MEMORY_MANAGER_H
#pragma once

#include "redMemoryFrameworkTypes.h"
#include "redMemoryAllocatorRegistration.h"
#include "redMemoryAllocators.h"
#include "redMemoryMetricsCollector.h"
#include "redMemoryAllocatorCreator.h"
#include "redMemoryCallbackList.h"
#include "redMemoryMetricsClassGroup.h"
#include "../redSystem/nameHash.h"

namespace Red { namespace MemoryFramework {

class IAllocator;
class IAllocatorCreationParameters;
class AllocationMetricsCollector;
class MemoryHook;

////////////////////////////////////////////////////////////////////////////
// Callback definitions

// Out of memory Callback. Returns true to indicate that some memory was released
typedef Red::System::Bool (*OutOfMemoryCallback)( PoolLabel poolLabel );

////////////////////////////////////////////////////////////////////////////
// Default Memory Pools
//	Use these typedefs to control the allocator types of the 'default' pools
typedef TLSFAllocatorThreadSafe RedMemoryStaticPoolAllocator;
typedef TLSFAllocatorThreadSafe RedMemoryOverflowAllocator;

////////////////////////////////////////////////////////////////////////////
// Allocator initialisation / registration function results
enum EMemoryManagerResults
{
	MemoryManager_OK,
	MemoryManager_OutOfMemory,
	MemoryManager_MaxAllocatorsReached
};

////////////////////////////////////////////////////////////////////////////
// MemoryManager
//	This class is responsible for pool creation / destruction as well as
//	collating metrics
class MemoryManager : public Red::System::NonCopyable
{
public:
	// Default pool budgets
	static const Red::System::MemSize c_DefaultStaticPoolSize = 1024;
	static const Red::System::MemSize c_DefaultOverflowPoolSize = 1024 * 1024;

	MemoryManager( Red::System::MemSize staticPoolSize = c_DefaultStaticPoolSize, Red::System::MemSize overflowPoolSize = c_DefaultOverflowPoolSize );
	~MemoryManager();

	// Register a new hook for all user pools
	void RegisterUserPoolHook( MemoryHook* theHook );

	// Instantiate a new pool
	template < class ALLOCATOR_TYPE >
	EMemoryManagerResults AddNamedPool( PoolLabel label, const IAllocatorCreationParameters& parameters, Red::System::Uint32 flags );

	// Destroy a pool 
	void DestroyNamedPool( PoolLabel label );

	// Direct allocator access
	RED_INLINE IAllocator* GetPool( PoolLabel label );
	RED_INLINE IAllocator* GetStaticPool();
	RED_INLINE IAllocator* GetOverflowPool();

	////////////////////////////////////////////////////////////////////////
	// Statically-resolved allocation functions (fast)

	// Allocate from a pool (has fallbacks to static pool)
	template < class ALLOCATOR_TYPE >
	void* Allocate( PoolLabel label, MemoryClass memoryClass, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment );
	
	// Reallocate from a pool
	template < class ALLOCATOR_TYPE >
	void* Reallocate( PoolLabel label, void* ptr, MemoryClass memoryClass, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment );

	// Free from a pool (has fallbacks to static pool)
	template < class ALLOCATOR_TYPE >
	EAllocatorFreeResults Free( PoolLabel label, MemoryClass memoryClass, const void* ptr, Red::System::MemSize* freedSizeReturn=nullptr );

	// Region allocation (region internal memory can move!)
	template < class ALLOCATOR_TYPE >
	MemoryRegionHandle AllocateRegion( PoolLabel label, MemoryClass memoryClass, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, RegionLifetimeHint lifetimeHint = Region_Longlived );

	// Region free
	template < class ALLOCATOR_TYPE >
	EAllocatorFreeResults FreeRegion( PoolLabel label, MemoryRegionHandle& handle );

	// Split memory region
	template < class ALLOCATOR_TYPE >
	MemoryRegionHandle SplitRegion( PoolLabel label, MemoryRegionHandle& baseRegion, Red::System::MemSize splitPosition, Red::System::MemSize splitAlignment );

	// Unlock memory region
	template < class ALLOCATOR_TYPE >
	void UnlockRegion( PoolLabel label, MemoryRegionHandle baseRegion );

	template < class ALLOCATOR_TYPE >
	void LockRegion( PoolLabel label, MemoryRegionHandle handle );

	////////////////////////////////////////////////////////////////////////
	// Runtime-resolved allocation functions (costs a virtual call, slower)
	void* RuntimeAllocate( PoolLabel label, MemoryClass memoryClass, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment );
	void* RuntimeReallocate( PoolLabel label, void* ptr, MemoryClass memoryClass, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment );
	EAllocatorFreeResults RuntimeFree( PoolLabel label, MemoryClass memoryClass, void* ptr, Red::System::MemSize* freedSizeReturn=nullptr );

	// Attempt to free memory back to the OS. This should be called in low-memory situations
	Red::System::MemSize ReleaseFreeMemoryToSystem();

	// Attempt to free memory back from a particular pool
	Red::System::MemSize ReleaseFreeMemoryToSystem( PoolLabel label );

	// Callback registration
	void RegisterOOMCallback( OutOfMemoryCallback callback );
	void UnregisterOOMCallback( OutOfMemoryCallback callback );

	// Register a pool name (for metrics)
	void RegisterPoolName( PoolLabel label, const Red::System::AnsiChar* poolName );

	// Register a class name (for metrics)
	void RegisterClassName( MemoryClass memoryClass, const Red::System::AnsiChar* className );

	// Register a memory class metrics group
	void RegisterClassGroup( const AnsiChar* groupName, MemoryClass* memClasses, Uint32 count );

	// Get memory class name, or "Unknwon" if not known
	const Red::System::AnsiChar* GetMemoryClassName( const MemoryClass memoryClass ) const;

	// Get memory pool name, or "Unknwon" if not known
	const Red::System::AnsiChar* GetMemoryPoolName( const PoolLabel memoryPool ) const;

	// Metrics
	AllocationMetricsCollector& GetMetricsCollector();
	void BeginMetricsDump( const Red::System::Char* filename );		// Write metrics to a file
	void EndMetricsDump( );											// Stop writing metrics to a file
	Red::System::Bool IsDumpingMetrics();							// Are metrics being written
	void OnFrameStart( );
	void OnFrameEnd( );
	void ResetMetrics( );

	// Pool enumeration
	Red::System::Uint16 GetRegisteredPoolCount();
	PoolLabel GetPoolLabelForIndex( Red::System::Uint16 poolIndex );

	// Debug function that can label memory on some systems to aid debugging
	void AnnotateSystemMemoryAreas();		

private:
	// Default pool creation / shutdown
	void InitialiseDefaultPools( Red::System::MemSize staticPoolSize, Red::System::MemSize overflowPoolSize );
	void ReleaseDefaultPools();
	void DestroyAllNamedPools();
	Bool ShouldCreateOverflowPool();

	// Region handle allocation failed
	MemoryRegionHandle OnAllocateRegionFailed( PoolLabel label, MemoryClass memoryClass, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, RegionLifetimeHint lifetimeHint );

	// Region free failed
	EAllocatorFreeResults OnFreeRegionFailed( PoolLabel label, MemoryClass memoryClass, MemoryRegionHandle& handle );

	// Called whenever an allocation fails, this can report the allocation, and try to throw it into the overflow pool. Uses runtime resolver
	void* OnAllocationFailed( PoolLabel label, MemoryClass memoryClass, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment );

	// Called when a reallocation failed. Note that this assumes the original data was NOT freed. Uses runtime resolver
	void* OnReallocateFailed( PoolLabel label, void* ptr, MemoryClass memoryClass, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment );

	// Called when a free fails. This usually means a dangling pointer to a destroyed pool
	void OnFreeFailed( PoolLabel label, MemoryClass memoryClass, const void* ptr, EAllocatorFreeResults reason );

	// Called when all attempts to allocate memory fail, this will most likely precede a hard crash
	void OutOfMemory( PoolLabel label, MemoryClass memoryClass, Red::System::MemSize allocSize, Red::System::MemSize alignment );

	// Returns true if the specified allocator should break/force crash on OOM
	Red::System::Bool ShouldBreakOnOOM( IAllocator* allocator );

	// Kicks out OOM callbacks and returns true if anything was freed
	Red::System::Bool FireOutOfMemoryCallbacks( PoolLabel poolLabel );

	// Default pool hooks
	void AddDefaultPoolHook( MemoryHook* hook );

	// Hook calling helpers
	Red::System::MemSize CallPreAllocateHooks( MemoryHook* hooks, PoolLabel label, MemoryClass memoryClass, size_t allocSize, size_t allocAlignment ) const;
	void CallPostAllocateHooks( MemoryHook* hooks, PoolLabel label, MemoryClass memoryClass, Red::System::MemSize allocatedSize, void* allocatedBuffer ) const;
	Red::System::MemSize CallPreReallocateHooks( MemoryHook* hooks, PoolLabel label, MemoryClass memoryClass, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, void* ptr ) const;
	void CallPostReallocateHooks( MemoryHook* hooks, PoolLabel label, MemoryClass memoryClass, Red::System::MemSize freedSize, Red::System::MemSize newSize, void* ptr ) const;
	void CallOnFreeHooks( MemoryHook* hooks, PoolLabel label, MemoryClass memoryClass, Red::System::MemSize sizeToFree, const void* ptr ) const;

	AllocatorCreator m_allocatorCreator;									// Responsible for ownership and construction / destruction of allocator objects
	AllocatorManager m_allocators;											// This manager tracks all available allocators
	RedMemoryStaticPoolAllocator* m_staticPool;								// The static pool
	RedMemoryOverflowAllocator*	m_overflowPool;								// The overflow pool
	AllocationMetricsCollector m_metricsCollector;							// Metrics collector for this manager
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	PoolNamesList m_memoryPoolNames;										// Lookup table for memory pool names
	MemoryClassNamesList m_memoryClassNames;								// Lookup table for memory class names
	MemoryClassGroups< k_MaximumMemoryClassGroups > m_memoryClassGroups;	// Memory class groups for metrics
#endif
	CallbackList< OutOfMemoryCallback, 8 > m_oomCallbacks;					// OutOfMemory callbacks
	MemoryHook* m_defaultHookList;											// Hooks for default pools (overflow / statics)
	MemoryHook* m_userPoolHookList;											// Hooks for user pools
};

///////////////////////////////////////////////////////////////////////
// ::GetPool
//	Direct allocator access
RED_INLINE IAllocator* MemoryManager::GetPool( PoolLabel label )
{
	return m_allocators.GetAllocatorByLabel( label );
}

///////////////////////////////////////////////////////////////////////
// GetStaticPool
//	Direct allocator access
RED_INLINE IAllocator* MemoryManager::GetStaticPool()
{
	return m_staticPool;
}

///////////////////////////////////////////////////////////////////////
// GetOverflowPool
//	Direct allocator access
RED_INLINE IAllocator* MemoryManager::GetOverflowPool()
{
	return m_overflowPool;
}

} }

#endif
