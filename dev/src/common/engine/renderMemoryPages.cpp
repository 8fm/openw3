/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderMemoryPages.h"

// The render memory master allocator
CRenderMemoryPages* GRenderMemory = NULL;

CRenderMemoryPages::Page::Page( Uint32 size )
	: m_next( NULL )
{
	// Allocate render page memory
	m_buffer = m_top = (Uint8*) RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_RenderFragment, size );
	m_end = m_top + size;
}

CRenderMemoryPages::Page::~Page()
{
	// Free memory
	if ( m_buffer )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_RenderFragment, m_buffer );
		m_buffer = NULL;
	}
}

CRenderMemoryPages::CRenderMemoryPages( Uint32 pageSize, Uint32 initialFreePages )
	: m_pageSize( pageSize )
	, m_freePages( NULL )
{
	// Allocate initial pages
	TDynArray< Page* > pages;
	for ( Uint32 i=0; i<initialFreePages; i++ )
	{
		pages.PushBack( AllocPage( pageSize ) );
	}

	// Return pages to the pool
	for ( Uint32 i=0; i<pages.Size(); i++ )
	{
		RelasePage( pages[i] );
	}
}

CRenderMemoryPages::~CRenderMemoryPages()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_listMutex );

	// Release pages
	Page* next = NULL;
	for ( Page* cur=m_freePages; cur; cur=next )
	{
		next = cur->m_next;
		delete cur;
	}

	// Free list
	m_freePages = NULL;
}

CRenderMemoryPages::Page* CRenderMemoryPages::AllocPage( Uint32 minimalSize )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_listMutex );

	// Try to get from the free page
	if ( m_freePages )
	{
		// Try to use page with exact size
		{
			Page** prevPtr = &m_freePages;
			for ( Page* cur=m_freePages; cur; cur=cur->m_next )
			{
				ASSERT( *prevPtr == cur );

				const Uint32 pageSize = PtrDiffToUint32( ( void* ) ( cur->m_end - cur->m_buffer ) );
				if ( pageSize == minimalSize )
				{
					// Unlink from list of free pages
					*prevPtr = cur->m_next;
					cur->m_next = NULL;

					return cur;
				}

				prevPtr = &cur->m_next;
			}
		}

		// Try to use any matching page that will fit the buffer
		{
			Page** prevPtr = &m_freePages;
			for ( Page* cur=m_freePages; cur; cur=cur->m_next )
			{
				ASSERT( *prevPtr == cur );

				const Uint32 pageSize = PtrDiffToUint32( (void*)(cur->m_end - cur->m_buffer) );
				if ( pageSize >= minimalSize )
				{
					// Unlink from list of free pages
					*prevPtr = cur->m_next;
					cur->m_next = NULL;

					return cur;
				}

				prevPtr = &cur->m_next;
			}
		}
	}

	// No page reused, alloc new page
	const Uint32 pageSizeToAlloc = minimalSize > m_pageSize ? minimalSize : m_pageSize;
	Page* page = new Page( pageSizeToAlloc );
	return page;
}

void CRenderMemoryPages::RelasePage( Page* page )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_listMutex );
	ASSERT( page );

	// Rewind memory
	page->m_top = page->m_buffer;

	// Add to free list
	page->m_next = m_freePages;
	m_freePages = page;
}