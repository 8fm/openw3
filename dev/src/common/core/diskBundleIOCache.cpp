/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "diskBundleIOCache.h"

CDiskBundleIOCache::CDiskBundleIOCache()
	:	m_memory( nullptr ),
		m_bufferSize( 0 )
{}

CDiskBundleIOCache::~CDiskBundleIOCache()
{
	RED_MEMORY_FREE( MemoryPool_Default, MC_BundleIO, m_memory );
}

void CDiskBundleIOCache::Initialize( Uint32 bufferSize )
{
	m_bufferSize = bufferSize;
	m_memory = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_BundleIO, bufferSize );
	m_allocator.Initialize( m_memory, bufferSize, MAX_BLOCKS );
}

CRingBufferBlock* CDiskBundleIOCache::AllocateBlock( const Uint32 size, const Uint32 alignment )
{
	// block is to big for the allocator - allocate directly from memory
	if ( !m_allocator.CanAllocateBlock( size, alignment ) )
	{
		WARN_CORE( TXT("IO error: requested a buffer of size %d that is bigger than the internal IO ring buffer. Allocating from main memory. OOM can occur."), size, m_bufferSize );
		return CRingBufferBlock::CreateManualBlock( size, alignment );
	}

	// try to allocate new block - if not possible, yield and retry
	CRingBufferBlock* block = nullptr;
	do
	{
		block = m_allocator.TryAllocateBlock( size, alignment );
		if ( !block )
		{
			ERR_CORE( TXT("IO error: internal IO ring buffer is full - probably there are multiple threads accessing the bundles. This is not wise and will slow down rendering." ) );
			Red::Threads::YieldCurrentThread();
		}
	}
	while ( block == nullptr );

	// return allocated block (always valid at this point)
	return block;
}
