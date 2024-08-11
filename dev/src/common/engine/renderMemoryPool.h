/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "renderMemoryPages.h"

/// Optimized memory allocator for rendering frame related allocations
class CRenderMemoryPool
{
protected:
	CRenderMemoryPages::Page*		m_pages;			//!< Active pages ( the top is the current one )
	
public:
	CRenderMemoryPool();
	~CRenderMemoryPool();

public:
	// Allocate memory from pool
	RED_INLINE void* Alloc( Uint32 size, Uint32 align = CSystem::DEFAULT_ALIGNMENT )
	{
		// Allocate from current page
		Uint8 *alignedTop = AlignPtr( m_pages->m_top, align );
		if ( alignedTop + size < m_pages->m_end )
		{
			m_pages->m_top = alignedTop + size;
			return alignedTop;
		}
		
		// Allocate new page
		CRenderMemoryPages::Page* newPage = GRenderMemory->AllocPage( size );
		ASSERT( (Uint32)(newPage->m_end - newPage->m_buffer) >= size );
		ASSERT( newPage->m_top == newPage->m_buffer );

		// Link in the list of frame pages
		newPage->m_next = m_pages;
		m_pages = newPage;

		// Allocate from new page
		alignedTop = newPage->m_top;
		ASSERT( alignedTop + size <= newPage->m_end );
		newPage->m_top = alignedTop + size;
		return alignedTop;
	}
};
