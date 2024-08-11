/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "redMemoryAllocatorRegistration.h"
#include "redMemoryAssert.h"

namespace Red { namespace MemoryFramework {

////////////////////////////////////////////////////////////////////
// CTor
//
AllocatorManager::AllocatorManager( )
	: m_allocatorCount(0)
{

}

////////////////////////////////////////////////////////////////////
// DTor
//	Asserts if allocators have not been destroyed correctly
AllocatorManager::~AllocatorManager( )
{
	RED_MEMORY_ASSERT( m_allocatorCount==0, "AllocatorManager has not been released!" );
}

////////////////////////////////////////////////////////////////////
// addAllocator
// Register an allocator. Ownership is passed to this class on registration
EAllocatorRegistrationResults	AllocatorManager::AddAllocator( PoolLabel label, IAllocator* alloc )
{
	// sanity check the state of the manager and the allocator
	RED_MEMORY_ASSERT( alloc != nullptr, "Registering a NULL allocator!");
	RED_MEMORY_ASSERT( label < k_MaximumPools, "Label is out of range" );
	RED_MEMORY_ASSERT( m_allocators[label] == nullptr, "An allocator is already registered for this label" );
	RED_MEMORY_ASSERT( m_allocatorCount < k_MaximumPools, "Too many allocators are registered already" );
	
	if( m_allocatorCount >= k_MaximumPools )
		return AR_MaxAllocatorsReached;

	if( alloc != nullptr && m_allocatorCount < k_MaximumPools && m_allocators[label] == nullptr )
	{
		m_allocators[label] = alloc;
		++m_allocatorCount;
		return AR_OK;
	}

	return AR_OutOfMemory;
}

////////////////////////////////////////////////////////////////////
// RemoveAllocatorByLabel
//	Remove an allocator (note this does NOT release the pool memory)
void	AllocatorManager::RemoveAllocatorByLabel( PoolLabel label )
{
	RED_MEMORY_ASSERT( label < k_MaximumPools, "Label is out of range" );
	RED_MEMORY_ASSERT( m_allocators[label] != nullptr, "Allocator does not exist" );

	if( label >= k_MaximumPools )
	{
		return ;
	}

	m_allocators[ label ] = nullptr;
	m_allocatorCount--;
}


////////////////////////////////////////////////////////////////////
// release
//	Release internal lists.
void	AllocatorManager::Release( )
{
	RED_MEMORY_ASSERT( m_allocatorCount == 0, "Releasing AllocatorManager but some pools still exist!" );
	m_allocatorCount = 0;
}

} }