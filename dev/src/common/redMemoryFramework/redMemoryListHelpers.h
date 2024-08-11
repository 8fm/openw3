/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_FRAMEWORK_LIST_HELPERS_H
#define _RED_MEMORY_FRAMEWORK_LIST_HELPERS_H
#pragma once

namespace Red { namespace MemoryFramework { namespace Utils {

	template< class NodeType >
	class ListNode
	{
	public:
		ListNode();
		~ListNode();

		NodeType* GetNext() const;
		NodeType* GetPrevious() const;
		void SetNext( NodeType* next );
		void SetPrevious( NodeType* previous );

	private:
		NodeType* m_next;
		NodeType* m_previous;
	};

	// List manipulation
	template< class NodeType >
	RED_INLINE void PushFront( NodeType*& headPtr, NodeType*& tailPtr, NodeType* toInsert );
	template< class NodeType >
	RED_INLINE void PushBack( NodeType*& headPtr, NodeType*& tailPtr, NodeType* toInsert );
	template< class NodeType >
	RED_INLINE NodeType* PopFront( NodeType*& headPtr, NodeType*& tailPtr );
	template< class NodeType >
	RED_INLINE void RemoveFromList( NodeType*& headPtr, NodeType*& tailPtr, NodeType* nodeInList );
	template< class NodeType >
	RED_INLINE void InsertBefore( NodeType*& headPtr, NodeType*& tailPtr, NodeType* anchor, NodeType* toInsert );
	template< class NodeType >
	RED_INLINE void InsertAfter( NodeType*& headPtr, NodeType*& tailPtr, NodeType* anchor, NodeType* toInsert );

} } }

#include "redMemoryListHelpers.inl"

#endif