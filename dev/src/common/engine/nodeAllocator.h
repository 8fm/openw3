/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

//---------------------------------------------------------------------------
//
// This file contains some simple pool based allocator of aligned structures
// Allocator is not thread safe (assumption is that the structures are allocated from one thread only).
// As a feature this code can allocate more than one structure at once (in bulk)
// which is prefferable for trees (hence the name).
//
// Assumption in order to make things faster: 
//  - Type T has trivial constructor and destructor
//
// TODO: this should use system page allocator rather than general allocator.
//
//---------------------------------------------------------------------------

template < Red::MemoryFramework::MemoryClass MemClass = MC_GlobalSpatialTree, 
	RED_CONTAINER_POOL_TYPE MemPool = MemoryPool_Default >
class TDefaultPageAllocator
{
public:
	//! Page size does depend on the system, usually 64K
	static Uint32 GetPageSize()
	{
		return 65536;
	}

	//! Allocate single page of memory
	static void* AllocatePage()
	{
		return RED_MEMORY_ALLOCATE( MemPool, MemClass, GetPageSize() );
	}

	//! Free allocated page memory
	static void FreePage(void* page)
	{
		RED_MEMORY_FREE( MemPool, MemClass, page );
	}
};

template < typename T, 
	class PageAllocator = TDefaultPageAllocator<>, 
	Uint32 DefaultAlignment = 16, 
	Uint32 BulkCount = 1 >
class TNodeAllocator
{
	struct Link
	{
		Link* m_next;
	};

	// Allocated pages
	TDynArray< void* > m_pages;

	// Free list - FIFO style
	Link* m_freeListHead;
	Link* m_freeListTail;

	// Current linear allocator
	T* m_currentPool;
	Uint32 m_currentPoolLeft;

	// Number of allocated nodes so far, debug mostly
	Uint32 m_numAllocatedNodes;

public:
	RED_INLINE TNodeAllocator()
		: m_freeListHead(NULL)
		, m_freeListTail(NULL)
		, m_numAllocatedNodes(0)
		, m_currentPool(NULL)
		, m_currentPoolLeft(0)
	{};

	// Destructor frees the pool
	RED_INLINE ~TNodeAllocator()
	{
		Clear();
	}

	// Free all allocated memory
	RED_INLINE void Clear()
	{
		for (Uint32 i=0; i<m_pages.Size(); ++i)
		{
			PageAllocator::FreePage(m_pages[i]);
		}

		m_pages.ClearFast();

		m_freeListHead = NULL;
		m_freeListTail = NULL;
		m_numAllocatedNodes = 0;

		m_currentPool = NULL;
		m_currentPoolLeft = 0;
	}

	// Allocate an element from node allocator
	// If the BulkCount is more than one then a block of elements is allocated
	RED_INLINE T* Allocate()
	{
		// Try the linear allocator first, it's the fastest
		if (m_currentPoolLeft > 0)
		{
			m_currentPoolLeft -= BulkCount;
			m_numAllocatedNodes += BulkCount;

			T* ptr = m_currentPool;
			m_currentPool += BulkCount;
			return ptr;
		}

		// Allocate items from free list if possible
		if (NULL != m_freeListHead)
		{
			Link* ptr = m_freeListHead;

			// unlink from the free list
			m_freeListHead = ptr->m_next;
			if (NULL == m_freeListHead)
			{
				m_freeListTail = NULL;
			}

			m_numAllocatedNodes += BulkCount;

			return reinterpret_cast<T*>(ptr);
		}

		// There are no more elements in the linear allocator and the free list is empty
		// We need to allocate a page
		void* pagePtr = PageAllocator::AllocatePage();
		const Uint32 pageSize = PageAllocator::GetPageSize();
		m_pages.PushBack( pagePtr );

		// initialize the linear allocator (leave the space for first element)
		m_currentPool = reinterpret_cast<T*>(pagePtr) + BulkCount;
		m_currentPoolLeft = ((pageSize / (sizeof(T) * BulkCount)) - 1) * BulkCount;

		// get the first bulk item 
		m_numAllocatedNodes += BulkCount;
		return reinterpret_cast<T*>(pagePtr);
	}

	// Free element (or group of elements) back to the node allocator
	RED_INLINE void Free(T* ptr)
	{
		ASSERT(m_numAllocatedNodes > 0);

		// Add to the TAIL of the free list (so it's a FIFO - helps with debugging)
		Link* ptrLink = reinterpret_cast<Link*>(ptr);
		if (m_freeListTail)
		{
			ptrLink->m_next = NULL;
			m_freeListTail->m_next = ptrLink;
			m_freeListTail = ptrLink;
		}
		else
		{
			ptrLink->m_next = NULL;
			m_freeListTail = ptrLink;
			m_freeListHead = ptrLink;
		}

		// Bookkeeping
		m_numAllocatedNodes -= BulkCount;
	}
};

