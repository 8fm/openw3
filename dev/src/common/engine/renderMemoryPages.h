/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Block base memory allocator for renderer
class CRenderMemoryPages
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_RenderData );

	friend class CRenderMemoryPool;

protected:
	struct Page
	{
		DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_RenderData );

		Uint8*		m_buffer;		//!< Memory buffer
		Uint8*		m_top;			//!< Allocation offset
		Uint8*		m_end;			//!< End of the buffer
		Page*		m_next;			//!< Next page

		Page( Uint32 size );
		~Page();
	};

protected:
	Page*					m_freePages;		//!< Free ( unused ) pages
	Uint32					m_pageSize;			//!< Size of page
	Red::Threads::CMutex	m_listMutex;		//!< Array access mutex

public:
	//! Get the default page size
	RED_INLINE Uint32 GetPageSize() const { return m_pageSize; }

public:
	CRenderMemoryPages( Uint32 pageSize, Uint32 initialFreePages );
	~CRenderMemoryPages();

	// Allocate page
	Page* AllocPage( Uint32 minimalSize );
	
	// Release page
	void RelasePage( Page* page );
};

/// Page base memory allocator for renderer
extern CRenderMemoryPages* GRenderMemory;
