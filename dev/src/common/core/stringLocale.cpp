/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

CLocalTextBufferManager::LocalBuffer::LocalBuffer( Uint32 size )
	: m_size( size )
	, m_next( NULL )
{
	// Allocate memory
	const Uint32 memorySize = size * sizeof( Char );
	m_buffer = ( Char* ) RED_MEMORY_ALLOCATE( MemoryPool_Strings, MC_String, memorySize );
}

CLocalTextBufferManager::LocalBuffer::~LocalBuffer()
{
	// Deallocate memory
	if ( m_buffer )
	{
		RED_MEMORY_FREE( MemoryPool_Strings, MC_String, m_buffer );
		m_buffer = NULL;
	}
}

CLocalTextBufferManager::CLocalTextBufferManager()
	: m_free( NULL )
	, m_used( NULL )
{
	// Preallocate some buffers, 8 should be enough for start
	const Uint32 numBuffers = 8;
	for ( Uint32 i=0; i<numBuffers; ++i )
	{
		// Allocate buffer
		LocalBuffer* buffer = new LocalBuffer( 1024 );

		// Add to free list
		buffer->m_next = m_free;
		m_free = buffer;
	}
}

CLocalTextBufferManager::~CLocalTextBufferManager()
{
	// Something still in used
	if ( m_used )
	{
		WARN_CORE( TXT("Some temporary text buffers that were allocated are still in use. Please debug.") );
	}

	// Release the free buffers
	LocalBuffer* cur = m_free;
	while ( cur )
	{
		LocalBuffer* next = cur->m_next;
		delete cur;
		cur = next;
	}
}

Char* CLocalTextBufferManager::AllocateTemporaryBuffer( Uint32 neededSize /*=1024*/ )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_accessMutex );

	// No buffer
	if ( neededSize == 0 )
	{
		return NULL;
	}

	// Try in the free list
	LocalBuffer** prev = &m_free;
	for ( LocalBuffer* cur=m_free; cur; prev = &cur->m_next, cur=cur->m_next )
	{
		if ( cur->m_size >= neededSize )
		{
			// Unlink from old list
			*prev = cur->m_next;

			// Link to new list
			cur->m_next = m_used;
			m_used = cur;

			// Return memory buffer
			return cur->m_buffer;
		}
	}

	// Allocate new buffers
	LocalBuffer* newBuffer = new LocalBuffer( neededSize );

	// Add to list
	newBuffer->m_next = m_used;
	m_used = newBuffer;

	// Return memory
	return newBuffer->m_buffer;
}

void CLocalTextBufferManager::DeallocateTemporaryBuffer( Char* buffer )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_accessMutex );

	// Not allocated
	if ( !buffer )
	{
		return;
	}

	// Find the block in in used list
	LocalBuffer** prev = &m_used;
	for ( LocalBuffer* cur=m_used; cur; prev = &cur->m_next, cur=cur->m_next )
	{
		if ( cur->m_buffer == buffer )
		{
			// Unlink from old list
			*prev = cur->m_next;

			// Link to free list
			cur->m_next = m_free;
			m_free = cur;

			// Done
			return;
		}
	}

	// This memory was not allocated from temporary pool
	WARN_CORE( TXT("Text memory at %p was not allocated from temporary text pool. Please debug."), buffer );
}

//////////////////////////////////////////////////////////////////////////////////////////////////
