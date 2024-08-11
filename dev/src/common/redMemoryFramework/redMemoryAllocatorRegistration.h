/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_ALLOCATOR_REGISTRATION_H
#define _RED_MEMORY_ALLOCATOR_REGISTRATION_H
#pragma once

#include "redMemoryFrameworkTypes.h"
#include "../redSystem/utility.h"

namespace Red { namespace MemoryFramework {

// Any functions that can fail will return one of these
enum EAllocatorRegistrationResults
{
	AR_OK,
	AR_MaxAllocatorsReached,
	AR_OutOfMemory
};

// Forward declarations
class IAllocator;

// A central repository of any allocators that are exposed to the global alloc router
// Requires a small amount of heap to track internal allocators
class AllocatorManager : public Red::System::NonCopyable
{
public:
	AllocatorManager( );
	virtual ~AllocatorManager( );

	// Release any allocators (i.e. delete them)
	void	Release( );

	// Register an allocator. Ownership is passed to this class on registration
	EAllocatorRegistrationResults	AddAllocator( PoolLabel label, IAllocator* alloc );

	// Get an allocator based on its label. Returns nullptr on failure
	RED_INLINE IAllocator*	GetAllocatorByLabel( PoolLabel label ) const;

	// Remove an allocator (note this does NOT release the pool memory)
	void RemoveAllocatorByLabel( PoolLabel label );

	// Enumerate pool labels
	RED_INLINE PoolLabel GetLabelForIndex( Red::System::Uint16 poolIndex );
	RED_INLINE Red::System::Uint16 GetAllocatorCount();

private:
	IAllocator*							m_allocators[k_MaximumPools];					// Array of all pools
	Red::System::Uint16					m_allocatorCount;								// Number of pools registered
};

} }

#include "redMemoryAllocatorRegistration.inl"

#endif