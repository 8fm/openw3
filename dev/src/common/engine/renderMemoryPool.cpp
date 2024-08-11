/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderMemoryPool.h"

CRenderMemoryPool::CRenderMemoryPool()
	: m_pages( NULL )
{
	// Get first page
	m_pages = GRenderMemory ? GRenderMemory->AllocPage( GRenderMemory->GetPageSize() ) : NULL;
}

CRenderMemoryPool::~CRenderMemoryPool()
{
	// Release pages to the pool
	CRenderMemoryPages::Page* next = NULL;
	for ( CRenderMemoryPages::Page* cur=m_pages; cur; cur=next )
	{
		next = cur->m_next;
		GRenderMemory->RelasePage( cur );
	}
}

