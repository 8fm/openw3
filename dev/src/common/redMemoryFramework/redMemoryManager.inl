/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_MANAGER_INL
#define _RED_MEMORY_MANAGER_INL

#include "redMemoryLog.h"
#include "redMemoryHook.h"

namespace Red { namespace MemoryFramework {

////////////////////////////////////////////////////////////////////////
// CallPreAllocateHooks
//
RED_FORCE_INLINE Red::System::MemSize MemoryManager::CallPreAllocateHooks( MemoryHook* hooks, PoolLabel label, MemoryClass memoryClass, size_t allocSize, size_t allocAlignment ) const
{
	Red::System::MemSize newSize = allocSize;
	MemoryHook* hook = hooks;
	while( hook )
	{
		if( hook->OnPreAllocate )
		{
			newSize = hook->OnPreAllocate( label, memoryClass, newSize, allocAlignment );
		}
		hook = hook->GetNextHook();
	}
	return newSize;
}

////////////////////////////////////////////////////////////////////////
// CallPostAllocateHooks
//
RED_FORCE_INLINE void MemoryManager::CallPostAllocateHooks( MemoryHook* hooks, PoolLabel label, MemoryClass memoryClass, Red::System::MemSize allocatedSize, void* allocatedBuffer ) const
{
	MemoryHook* hook = hooks;
	while( hook )
	{
		if( hook->OnPostAllocate )
		{
			hook->OnPostAllocate( label, memoryClass, allocatedSize, allocatedBuffer );
		}
		hook = hook->GetNextHook();
	}
}

////////////////////////////////////////////////////////////////////////
// CallPreReallocateHooks
//
RED_FORCE_INLINE Red::System::MemSize MemoryManager::CallPreReallocateHooks( MemoryHook* hooks, PoolLabel label, MemoryClass memoryClass, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, void* ptr ) const
{
	Red::System::MemSize newAllocSize = allocSize;
	MemoryHook* hook = hooks;
	while( hook )
	{
		if( hook->OnPreReallocate )
		{
			newAllocSize = hook->OnPreReallocate( label, memoryClass, allocSize, allocAlignment, ptr );
		}
		hook = hook->GetNextHook();
	}
	return newAllocSize;
}

////////////////////////////////////////////////////////////////////////
// CallPostReallocateHooks
//
RED_FORCE_INLINE void MemoryManager::CallPostReallocateHooks( MemoryHook* hooks, PoolLabel label, MemoryClass memoryClass, Red::System::MemSize freedSize, Red::System::MemSize newSize, void* ptr ) const
{
	MemoryHook* hook = hooks;
	while( hook )
	{
		if( hook->OnPostReallocate )
		{
			hook->OnPostReallocate( label, memoryClass, freedSize, newSize, ptr );
		}
		hook = hook->GetNextHook();
	}
}

////////////////////////////////////////////////////////////////////////
// CallOnFreeHooks
//
RED_FORCE_INLINE void MemoryManager::CallOnFreeHooks( MemoryHook* hooks, PoolLabel label, MemoryClass memoryClass, Red::System::MemSize sizeToFree, const void* ptr ) const
{
	MemoryHook* hook = hooks;
	while( hook )
	{
		if( hook->OnFree )
		{
			hook->OnFree( label, memoryClass, sizeToFree, ptr );
		}
		hook = hook->GetNextHook();
	}
}

////////////////////////////////////////////////////////////////////////
// RegisterOOMCallback
//	Register a callback that gets hit when an allocation fails
void MemoryManager::RegisterOOMCallback( OutOfMemoryCallback callback )
{
	m_oomCallbacks.RegisterCallback( callback );
}

////////////////////////////////////////////////////////////////////////
// UnregisterOOMCallback
//	Register a callback that gets hit when an allocation fails
void MemoryManager::UnregisterOOMCallback( OutOfMemoryCallback callback )
{
	m_oomCallbacks.UnRegisterCallback( callback );
}

////////////////////////////////////////////////////////////////////////
// AddNamedAllocator
//	Creates an allocator of the specified type and registers it
template < class ALLOCATOR_TYPE >
EMemoryManagerResults MemoryManager::AddNamedPool( PoolLabel label, const IAllocatorCreationParameters& parameters, Red::System::Uint32 flags )
{
	RED_UNUSED( flags );

	// Instantiate the allocator
	ALLOCATOR_TYPE* theAllocator = m_allocatorCreator.CreateAllocator< ALLOCATOR_TYPE >();
	RED_MEMORY_ASSERT( theAllocator != NULL, "Failed to create a named allocator" );
	if( theAllocator == NULL )
	{
		return MemoryManager_OutOfMemory;
	}

	// Initialise the allocator
	EAllocatorInitResults initResults = theAllocator->Initialise( parameters, flags );
	if( initResults == AllocInit_OutOfMemory )
	{
		m_allocatorCreator.DestroyAllocator< ALLOCATOR_TYPE >( theAllocator );
		return MemoryManager_OutOfMemory;
	}
	
	// Try to register the allocator
	EAllocatorRegistrationResults registrationResult = m_allocators.AddAllocator( label, theAllocator );
	if( registrationResult != AR_OK )
	{
		m_allocatorCreator.DestroyAllocator< ALLOCATOR_TYPE >( theAllocator );
	}

	// Process registration errors
	switch( registrationResult )
	{
	case AR_OutOfMemory:
		return MemoryManager_OutOfMemory;
	case AR_MaxAllocatorsReached:
		return MemoryManager_MaxAllocatorsReached;
	case AR_OK:
		return MemoryManager_OK;
	}

	return MemoryManager_OK;
}

///////////////////////////////////////////////////////////////////
// ShouldBreakOnOOM
//	Returns true if the specified allocator should break/force crash on OOM
Red::System::Bool MemoryManager::ShouldBreakOnOOM( IAllocator* allocator )
{
	return !allocator || !( allocator->GetFlags() & Allocator_NoBreakOnOOM );
}

///////////////////////////////////////////////////////////////////
// RuntimeAllocate
//
void* MemoryManager::RuntimeAllocate( PoolLabel label, MemoryClass memoryClass, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment )
{
	// If you pass in "-size_t(1)" then it can overflow and wrap around later with alignment padding. Just stop crazy allocation requests here.
	RED_MEMORY_ASSERT( allocSize <= 512 * 1024 * 1024, "Refusing to allocate more than half a gig of memory at once!" );
	RED_MEMORY_ASSERT( allocAlignment != 0 && (allocAlignment & ( allocAlignment-1)) == 0, "Alignment must be a power of two" );

	void* thePtr = nullptr;
	Red::System::MemSize actualAllocSize = 0;
	if( allocSize == 0 )
	{
		return thePtr;
	}

	IAllocator* allocator = GetPool( label );
	if( allocator == nullptr )	// If this pool does not exist, redirect to statics pool
	{
		RED_MEMORY_ASSERT( m_staticPool != nullptr, "Trying to allocate to the statics pool but none exists for this manager. Consider giving this memory manager a statics budget" );
		Red::System::MemSize hookAllocSize = CallPreAllocateHooks( m_defaultHookList, label, memoryClass, allocSize, allocAlignment );
		thePtr = m_staticPool->Allocate( hookAllocSize, allocAlignment, actualAllocSize, memoryClass );
		CallPostAllocateHooks( m_defaultHookList, label, memoryClass, actualAllocSize, thePtr );
		m_metricsCollector.OnStaticAllocation( label, memoryClass, thePtr, actualAllocSize, allocAlignment );
	}
	else
	{
		Red::System::MemSize hookAllocSize = CallPreAllocateHooks( m_userPoolHookList, label, memoryClass, allocSize, allocAlignment );
		thePtr = (allocator->RuntimeAllocate)( hookAllocSize, allocAlignment, actualAllocSize, memoryClass );
		if( thePtr == nullptr )
		{
			if( allocator->IncreaseMemoryFootprint( m_metricsCollector.GetAllocatorAreaCallback( label ), allocSize ) )
			{
				thePtr = allocator->RuntimeAllocate( hookAllocSize, allocAlignment, actualAllocSize, memoryClass );
			}
		}
		CallPostAllocateHooks( m_userPoolHookList, label, memoryClass, actualAllocSize, thePtr );
		m_metricsCollector.OnAllocation( label, memoryClass, thePtr, actualAllocSize, allocAlignment );
	}

	if( thePtr == nullptr )
	{
		thePtr = OnAllocationFailed( label, memoryClass, allocSize, allocAlignment );
	}

	return thePtr;
}

///////////////////////////////////////////////////////////////////
// RuntimeReallocate
//
void* MemoryManager::RuntimeReallocate( PoolLabel label, void* ptr, MemoryClass memoryClass, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment )
{
	// If you pass in "-size_t(1)" then it can overflow and wrap around later with alignment padding. Just stop crazy allocation requests here.
	RED_MEMORY_ASSERT( allocSize <= 512 * 1024 * 1024, "Refusing to reallocate more than half a gig of memory at once!" );
	RED_MEMORY_ASSERT( allocAlignment != 0 && (allocAlignment & ( allocAlignment-1)) == 0, "Alignment must be a power of two" );

#ifndef RED_FINAL_BUILD
	// Invalid size handling
	if ( allocSize > 512 * 1024 * 1024 )
		return nullptr;
#endif
	void* thePtr = nullptr;
	Red::System::MemSize freedSize = 0;
	Red::System::MemSize allocatedSize = 0;

	IAllocator* allocator = GetPool( label );

	// If the allocator does not exist, or the original allocation came from the static pool, we need to use the statics pool
	if( ( m_staticPool != nullptr && m_staticPool->OwnsPointer( ptr ) ) || allocator == nullptr )
	{
		RED_MEMORY_ASSERT( m_staticPool != nullptr, "Trying to reallocate to the statics pool but none exists for this manager" );
		Red::System::MemSize hookAllocSize = CallPreReallocateHooks( m_defaultHookList, label, memoryClass, allocSize, allocAlignment, ptr );
		thePtr = m_staticPool->Reallocate( ptr, hookAllocSize, allocAlignment, allocatedSize, freedSize, memoryClass );
		CallPostReallocateHooks( m_defaultHookList, label, memoryClass, freedSize, allocatedSize, thePtr );
		m_metricsCollector.OnStaticReallocation( label, memoryClass, ptr, thePtr, freedSize, allocatedSize, allocAlignment );
	}
	else
	{
		Red::System::MemSize hookAllocSize = CallPreReallocateHooks( m_userPoolHookList, label, memoryClass, allocSize, allocAlignment, ptr );
		thePtr = allocator->RuntimeReallocate( ptr, hookAllocSize, allocAlignment, allocatedSize, freedSize, memoryClass );
		if( allocSize > 0 && ( thePtr == nullptr || allocatedSize == 0 ) )	// Failed to allocate
		{
			if( allocator->IncreaseMemoryFootprint( m_metricsCollector.GetAllocatorAreaCallback( label ), allocSize ) )
			{
				thePtr = allocator->RuntimeReallocate( ptr, hookAllocSize, allocAlignment, allocatedSize, freedSize, memoryClass );
			}
		}
		CallPostReallocateHooks( m_userPoolHookList, label, memoryClass, freedSize, allocatedSize, thePtr );
		m_metricsCollector.OnReallocation( label, memoryClass, ptr, thePtr, freedSize, allocatedSize, allocAlignment );
	}

	// Realloc has complex failure conditions. Handle them all!
	Red::System::Bool allocationFailed = ( allocSize > 0 && ( thePtr == nullptr || allocatedSize == 0 ) );
	Red::System::Bool freeFailed = ( ptr != nullptr && allocSize == 0 && freedSize == 0 );
	if( allocationFailed || freeFailed )
	{
		thePtr = OnReallocateFailed( label, ptr, memoryClass, allocSize, allocAlignment );
	}

	return thePtr;
}

///////////////////////////////////////////////////////////////////
// RuntimeFree
//
EAllocatorFreeResults MemoryManager::RuntimeFree( PoolLabel label, MemoryClass memoryClass, void* ptr, Red::System::MemSize* freedSizeReturn )
{
	if( ptr == nullptr )
	{
		freedSizeReturn = 0;
		return Free_OK;
	}

	EAllocatorFreeResults freeResult = Free_OK;
	Red::System::MemSize freedSize = 0;
	IAllocator* allocator = GetPool( label );

	if( ( m_staticPool != nullptr && m_staticPool->OwnsPointer( ptr ) ) || allocator == nullptr )
	{
		freedSize = m_staticPool->GetAllocationSize( ptr );
		CallOnFreeHooks( m_defaultHookList, label, memoryClass, freedSize, ptr );
		freeResult = m_staticPool->Free( ptr );
		m_metricsCollector.OnStaticFree( label, memoryClass, ptr, freeResult, freedSize );
	}
	else if( allocator )
	{
		freedSize = allocator->RuntimeGetAllocationSize( ptr );
		CallOnFreeHooks( m_userPoolHookList, label, memoryClass, freedSize, ptr );
		freeResult = allocator->RuntimeFree( ptr );
		m_metricsCollector.OnFree( label, memoryClass, ptr, freeResult, freedSize );
	}

	// Handle the case where neither the static or named pool own the pointer (last ditch attempt to free the memory!)
	if( freeResult == Free_NotOwned && m_overflowPool != nullptr )
	{
		freedSize = m_overflowPool->GetAllocationSize( ptr );
		CallOnFreeHooks( m_defaultHookList, label, memoryClass, freedSize, ptr );
		freeResult = m_overflowPool->Free( ptr );
		m_metricsCollector.OnOverflowFree( label, memoryClass, ptr, freeResult, freedSize );
	}

	RED_MEMORY_ASSERT( freeResult == Free_OK, "Major error! Failed to free the memory at this address" );
	if( freeResult != Free_OK )
	{
		OnFreeFailed( label, memoryClass, ptr, freeResult );
	}

	if( freedSizeReturn )
	{
		*freedSizeReturn = freedSize;
	}

	return freeResult;
}

////////////////////////////////////////////////////////////////////////
// AllocateRegion
//	Returns a (potentially) moveable region of memory. No overflow / static available here!
template < class ALLOCATOR_TYPE >
MemoryRegionHandle MemoryManager::AllocateRegion( PoolLabel label, MemoryClass memoryClass, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, RegionLifetimeHint lifetimeHint )
{
	RED_MEMORY_ASSERT( allocAlignment != 0 && (allocAlignment & ( allocAlignment-1)) == 0, "Alignment must be a power of two" );
	if( allocSize == 0 )
	{
		return nullptr;
	}

	ALLOCATOR_TYPE* allocator = static_cast< ALLOCATOR_TYPE* >( GetPool( label ) );
	RED_MEMORY_ASSERT( allocator, "Pool does not exist with this label" );

	MemoryRegionHandle handleResult = allocator->AllocateRegion( allocSize, allocAlignment, lifetimeHint );
	if( !handleResult.IsValid() )
	{
		handleResult = OnAllocateRegionFailed( label, memoryClass, allocSize, allocAlignment, lifetimeHint );
	}

	if( handleResult.IsValid() )
	{
		m_metricsCollector.OnAllocation( label, memoryClass, handleResult.GetRawPtr(), handleResult.GetSize(), allocAlignment );
	}
	handleResult.InternalSetMemoryClass( memoryClass );
	
	return handleResult;
}

////////////////////////////////////////////////////////////////////////
// SplitRegion
//	Chop a region in half, returns the newly created region
template < class ALLOCATOR_TYPE >
MemoryRegionHandle MemoryManager::SplitRegion( PoolLabel label, MemoryRegionHandle& baseRegion, Red::System::MemSize splitPosition, Red::System::MemSize splitAlignment )
{
	ALLOCATOR_TYPE* allocator = static_cast< ALLOCATOR_TYPE* >( GetPool( label ) );
	RED_MEMORY_ASSERT( allocator, "Pool does not exist with this label" );

	const Red::System::MemSize prevSize = baseRegion.GetSize();
	const MemoryClass memoryClass = baseRegion.GetMemoryClass();

	MemoryRegionHandle handleResult = allocator->SplitRegion( baseRegion, splitPosition, splitAlignment );
	if( handleResult.IsValid() )
	{
		m_metricsCollector.OnFree( label, memoryClass, baseRegion.GetRawPtr(), Free_OK, prevSize );
		m_metricsCollector.OnAllocation( label, memoryClass, baseRegion.GetRawPtr(), baseRegion.GetSize(), splitAlignment );
		m_metricsCollector.OnAllocation( label, memoryClass, handleResult.GetRawPtr(), handleResult.GetSize(), splitAlignment );
		handleResult.InternalSetMemoryClass( memoryClass );
	}

	return handleResult;
}

////////////////////////////////////////////////////////////////////////
// UnlockRegion
//	Unlock memory region so it can be freed/defragged
template < class ALLOCATOR_TYPE >
void MemoryManager::UnlockRegion( PoolLabel label, MemoryRegionHandle baseRegion )
{
	ALLOCATOR_TYPE* allocator = static_cast< ALLOCATOR_TYPE* >( GetPool( label ) );
	RED_MEMORY_ASSERT( allocator, "Pool does not exist with this label" );

	allocator->UnlockRegion( baseRegion );
}

////////////////////////////////////////////////////////////////////////
// LockRegion
//	Lock memory region so it can't be defragged
template < class ALLOCATOR_TYPE >
void MemoryManager::LockRegion( PoolLabel label, MemoryRegionHandle baseRegion )
{
	ALLOCATOR_TYPE* allocator = static_cast< ALLOCATOR_TYPE* >( GetPool( label ) );
	RED_MEMORY_ASSERT( allocator, "Pool does not exist with this label" );

	allocator->LockRegion( baseRegion );
}

////////////////////////////////////////////////////////////////////////
// FreeRegion
//	Return a region back to its pool
template < class ALLOCATOR_TYPE >
EAllocatorFreeResults MemoryManager::FreeRegion( PoolLabel label, MemoryRegionHandle& handle )
{
	if( !handle.IsValid() )
	{
		return Free_OK;
	}

	void* regionAddress = handle.GetRawPtr();
	Red::System::MemSize regionSize = handle.GetSize();

	ALLOCATOR_TYPE* allocator = static_cast< ALLOCATOR_TYPE* >( GetPool( label ) );
	RED_MEMORY_ASSERT( allocator, "Pool does not exist with this label" );

	const MemoryClass memoryClass = handle.GetMemoryClass();
	EAllocatorFreeResults freeResult = allocator->FreeRegion( handle );
	m_metricsCollector.OnFree( label, memoryClass, regionAddress, freeResult, regionSize );
	if( freeResult != Free_OK )
	{
		freeResult = OnFreeRegionFailed( label, memoryClass, handle );
	}
	else
	{
		handle = MemoryRegionHandle();
	}

	return freeResult;
}

////////////////////////////////////////////////////////////////////////
// Allocate
//	Allocate from a pool. Fallback to static / overflow pools
template < class ALLOCATOR_TYPE >
void* MemoryManager::Allocate( PoolLabel label, MemoryClass memoryClass, size_t allocSize, size_t allocAlignment )
{
	// If you pass in "-size_t(1)" then it can overflow and wrap around later with alignment padding. Just stop crazy allocation requests here.
	RED_MEMORY_ASSERT( allocSize <= 512 * 1024 * 1024, "Refusing to allocate more than half a gig of memory at once!" );
	RED_MEMORY_ASSERT( allocAlignment != 0 && (allocAlignment & ( allocAlignment-1)) == 0, "Alignment must be a power of two" );

	void* thePtr = nullptr;
	Red::System::MemSize actualAllocSize = 0;
	if( allocSize == 0 )
	{
		return thePtr;
	}

	ALLOCATOR_TYPE* allocator = static_cast< ALLOCATOR_TYPE* >( GetPool( label ) );
	if( allocator == nullptr )	// If this pool does not exist, redirect to statics pool
	{
		RED_MEMORY_ASSERT( m_staticPool != nullptr, "Trying to allocate to the statics pool but none exists for this manager. Consider giving this memory manager a statics budget" );
		Red::System::MemSize hookAllocSize = CallPreAllocateHooks( m_defaultHookList, label, memoryClass, allocSize, allocAlignment );
		thePtr = m_staticPool->Allocate( hookAllocSize, allocAlignment, actualAllocSize, memoryClass );
		CallPostAllocateHooks( m_defaultHookList, label, memoryClass, actualAllocSize, thePtr );
		m_metricsCollector.OnStaticAllocation( label, memoryClass, thePtr, actualAllocSize, allocAlignment );
	}
	else
	{
		Red::System::MemSize hookAllocSize = CallPreAllocateHooks( m_userPoolHookList, label, memoryClass, allocSize, allocAlignment );
		thePtr = allocator->Allocate( hookAllocSize, allocAlignment, actualAllocSize, memoryClass );
		if( thePtr == nullptr )
		{
			if( allocator->IncreaseMemoryFootprint( m_metricsCollector.GetAllocatorAreaCallback( label ), allocSize ) )
			{
				thePtr = allocator->Allocate( hookAllocSize, allocAlignment, actualAllocSize, memoryClass );
			}
		}
		CallPostAllocateHooks( m_userPoolHookList, label, memoryClass, actualAllocSize, thePtr );
		m_metricsCollector.OnAllocation( label, memoryClass, thePtr, actualAllocSize, allocAlignment );
	}

	if( thePtr == nullptr )
	{
		thePtr = OnAllocationFailed( label, memoryClass, allocSize, allocAlignment );
	}

	return thePtr;
}

////////////////////////////////////////////////////////////////////////
// Reallocate
//	Reallocate from a pool
template < class ALLOCATOR_TYPE >
void* MemoryManager::Reallocate( PoolLabel label, void* ptr, MemoryClass memoryClass, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment )
{
	// If you pass in "-size_t(1)" then it can overflow and wrap around later with alignment padding. Just stop crazy allocation requests here.
	RED_MEMORY_ASSERT( allocSize <= 512 * 1024 * 1024, "Refusing to reallocate more than half a gig of memory at once!" );
	RED_MEMORY_ASSERT( allocAlignment != 0 && (allocAlignment & ( allocAlignment-1)) == 0, "Alignment must be a power of two" );

	void* thePtr = nullptr;
	Red::System::MemSize freedSize = 0;
	Red::System::MemSize allocatedSize = 0;

	ALLOCATOR_TYPE* allocator = static_cast< ALLOCATOR_TYPE* >( GetPool( label ) );

	// If the allocator does not exist, or the original allocation came from the static pool, we need to use the statics pool
	if( ( m_staticPool != nullptr && m_staticPool->OwnsPointer( ptr ) ) || allocator == nullptr )
	{
		RED_MEMORY_ASSERT( m_staticPool != nullptr, "Trying to reallocate to the statics pool but none exists for this manager" );
		Red::System::MemSize hookAllocSize = CallPreReallocateHooks( m_defaultHookList, label, memoryClass, allocSize, allocAlignment, ptr );
		thePtr = m_staticPool->Reallocate( ptr, hookAllocSize, allocAlignment, allocatedSize, freedSize, memoryClass );
		CallPostReallocateHooks( m_defaultHookList, label, memoryClass, freedSize, allocatedSize, thePtr );
		m_metricsCollector.OnStaticReallocation( label, memoryClass, ptr, thePtr, freedSize, allocatedSize, allocAlignment );
	}
	else
	{
		Red::System::MemSize hookAllocSize = CallPreReallocateHooks( m_userPoolHookList, label, memoryClass, allocSize, allocAlignment, ptr );
		thePtr = allocator->Reallocate( ptr, hookAllocSize, allocAlignment, allocatedSize, freedSize, memoryClass );
		if( allocSize > 0 && ( thePtr == nullptr || allocatedSize == 0 ) )	// Failed to allocate
		{
			if( allocator->IncreaseMemoryFootprint( m_metricsCollector.GetAllocatorAreaCallback( label ), allocSize ) )
			{
				thePtr = allocator->Reallocate( ptr, hookAllocSize, allocAlignment, allocatedSize, freedSize, memoryClass );
			}
		}
		CallPostReallocateHooks( m_userPoolHookList, label, memoryClass, freedSize, allocatedSize, thePtr );
		m_metricsCollector.OnReallocation( label, memoryClass, ptr, thePtr, freedSize, allocatedSize, allocAlignment );
	}

	// Realloc has complex failure conditions. Handle them all!
	Red::System::Bool allocationFailed = ( allocSize > 0 && ( thePtr == nullptr || allocatedSize == 0 ) );
	Red::System::Bool freeFailed = ( ptr != nullptr && allocSize == 0 && freedSize == 0 );
	if( allocationFailed || freeFailed )
	{
		thePtr = OnReallocateFailed( label, ptr, memoryClass, allocSize, allocAlignment );
	}

	return thePtr;
}

////////////////////////////////////////////////////////////////////////
// Free
//	Free from a pool (has fallbacks to static/overflow pool)
template < class ALLOCATOR_TYPE >
EAllocatorFreeResults MemoryManager::Free( PoolLabel label, MemoryClass memoryClass, const void* ptr, Red::System::MemSize* freedSizeReturn )
{
	if( ptr == nullptr )
	{
		freedSizeReturn = 0;
		return Free_OK;
	}

	EAllocatorFreeResults freeResult = Free_NotOwned;
	Red::System::MemSize freedSize = 0;
	ALLOCATOR_TYPE* allocator = static_cast< ALLOCATOR_TYPE* >( GetPool( label ) );
	
	if( ( m_staticPool != nullptr && m_staticPool->OwnsPointer( ptr ) ) || allocator == nullptr )
	{
		freedSize = m_staticPool->GetAllocationSize( ptr );
		CallOnFreeHooks( m_defaultHookList, label, memoryClass, freedSize, ptr );
		freeResult = m_staticPool->Free( ptr );
		m_metricsCollector.OnStaticFree( label, memoryClass, ptr, freeResult, freedSize );
	}
	else if( allocator )
	{
		freedSize = allocator->GetAllocationSize( ptr );
		CallOnFreeHooks( m_userPoolHookList, label, memoryClass, freedSize, ptr );
		freeResult = allocator->Free( ptr );
		m_metricsCollector.OnFree( label, memoryClass, ptr, freeResult, freedSize );
	}

	// Handle the case where neither the static or named pool own the pointer (last ditch attempt to free the memory!)
	if( freeResult == Free_NotOwned && m_overflowPool != nullptr )
	{
		freedSize = m_overflowPool->GetAllocationSize( ptr );
		CallOnFreeHooks( m_defaultHookList, label, memoryClass, freedSize, ptr );
		freeResult = m_overflowPool->Free( ptr );
		m_metricsCollector.OnOverflowFree( label, memoryClass, ptr, freeResult, freedSize );
	}

	RED_MEMORY_ASSERT( freeResult == Free_OK, "Major error! Failed to free the memory at this address" );
	if( freeResult != Free_OK )
	{
		OnFreeFailed( label, memoryClass, ptr, freeResult );
	}

	if( freedSizeReturn )
	{
		*freedSizeReturn = freedSize;
	}

	return freeResult;
}

///////////////////////////////////////////////////////////////////
// OnAllocationFailed
//	Called whenever an allocation fails, this can report the allocation, and try to throw it into the overflow pool
void* MemoryManager::OnAllocationFailed( PoolLabel label, MemoryClass memoryClass, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment )
{
	// Kick out oom callbacks, if anything freed memory, attempt to allocate again
	if( FireOutOfMemoryCallbacks( label ) )
	{
		RED_MEMORY_LOG( TXT( "Allocate Failed. OOM Callbacks freed some memory. Attempting allocate again" ) );
		void* newPtr = RuntimeAllocate( label, memoryClass, allocSize, allocAlignment );
		if( newPtr )
		{
			return newPtr;
		}
	}

	// First, try to allocate to the overflow pool
	RED_MEMORY_LOG_ONCE( TXT( "Allocate failed. Attempting to allocate %d bytes from the overflow pool!" ), allocSize );

	Red::System::MemSize actualSizeAllocated = 0;
	if( m_overflowPool != nullptr )
	{
		Red::System::MemSize newAllocSize = CallPreAllocateHooks( m_defaultHookList, label, memoryClass, allocSize, allocAlignment );
		void* thePtr = m_overflowPool->Allocate( newAllocSize, allocAlignment, actualSizeAllocated, memoryClass );
		CallPostAllocateHooks( m_defaultHookList, label, memoryClass, actualSizeAllocated, thePtr );
		m_metricsCollector.OnOverflowAllocation( label, memoryClass, thePtr, allocSize, allocAlignment );
		if( thePtr == nullptr )
		{
			OutOfMemory( label, memoryClass, allocSize, allocAlignment );
		}
		return thePtr;
	}

	OutOfMemory( label, memoryClass, allocSize, allocAlignment );
	return nullptr;
}

////////////////////////////////////////////////////////////////////////
// OnReallocateFailed
//	Called when a reallocation failed.
void* MemoryManager::OnReallocateFailed( PoolLabel label, void* ptr, MemoryClass memoryClass, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment )
{
	// Kick out oom callbacks, if anything freed memory, attempt to reallocate again
	Red::System::MemSize freedSize = 0;
	Red::System::MemSize allocatedSize = 0;

	if( FireOutOfMemoryCallbacks( label ) )
	{
		RED_MEMORY_LOG( TXT( "Reallocate Failed in pool %d, memory class %d. OOM Callbacks freed some memory. Attempting reallocate again" ), label, memoryClass );
		void* newPtr = RuntimeReallocate( label, ptr, memoryClass, allocSize, allocAlignment );

		return newPtr;
	}

	RED_MEMORY_LOG_ONCE( TXT( "Reallocate failed. Attempting to reallocate %u bytes from the overflow pool!" ), allocSize );

	// Worst case. We can't really do a proper reallocate; so try and 'simulate' one
	void* thePtr = nullptr;
	if( m_overflowPool != nullptr )
	{
		// Try to get the 'source' pool for this memory
		IAllocator* sourceAllocator = GetPool( label );
		if( ptr != nullptr && !sourceAllocator->RuntimeOwnsPointer( ptr ) )
		{
			sourceAllocator = m_overflowPool;
			RED_MEMORY_ASSERT( sourceAllocator->RuntimeOwnsPointer( ptr ), "Failed to find owner pool" );
		}

		// Allocate from overflow
		Red::System::MemSize newAllocSize = CallPreAllocateHooks( m_defaultHookList, label, memoryClass, allocSize, allocAlignment );
		thePtr = m_overflowPool->Allocate( newAllocSize, allocAlignment, allocatedSize, memoryClass );
		CallPostAllocateHooks( m_defaultHookList, label, memoryClass, allocatedSize, thePtr );

		// Copy old data to new data if required
		Red::System::MemSize oldAllocSize = sourceAllocator->RuntimeGetAllocationSize( ptr );
		if( newAllocSize > 0 && oldAllocSize > 0 && thePtr != nullptr )
		{
			Red::System::MemSize copySize = Red::Math::NumericalUtils::Min( oldAllocSize, newAllocSize );
			Red::System::MemoryCopy( thePtr, ptr, copySize );
		}

		// Free the old memory
		if( ptr )
		{
			if( sourceAllocator->RuntimeFree( ptr ) == Free_OK )
			{
				freedSize  = oldAllocSize;
			}
		}

		m_metricsCollector.OnOverflowReallocation( label, memoryClass, ptr, thePtr, freedSize, allocatedSize, allocAlignment );
	}

	// Realloc has complex failure conditions. Handle them all!
	Red::System::Bool allocationFailed = ( allocSize > 0 && thePtr == nullptr );
	Red::System::Bool freeFailed = ( allocSize == 0 && thePtr != nullptr && freedSize == 0 );
	if( allocationFailed || freeFailed )
	{
		// By this point, we have failed to reallocate from any pool. This means that either non of the pools own the pointer (unlikely)
		// or no pools had enough space left. All we can do it return OutOfMemory
		OutOfMemory( label, memoryClass, allocSize, allocAlignment );
	}

	return thePtr;
}

////////////////////////////////////////////////////////////////////////
// FireOutOfMemoryCallbacks
//	Returns true if any callbacks indicate that memory was freed
Red::System::Bool MemoryManager::FireOutOfMemoryCallbacks( PoolLabel poolLabel )
{
	Red::System::Bool memoryFreed = false;
	for( auto it = m_oomCallbacks.Begin(); it != m_oomCallbacks.End(); ++it )
	{
		memoryFreed |= (*it)( poolLabel );
	}
	return memoryFreed;
}

} }


#endif
