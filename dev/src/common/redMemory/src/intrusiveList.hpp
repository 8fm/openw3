/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_UTILITY_INTRUSIVE_LIST_HPP_
#define _RED_MEMORY_UTILITY_INTRUSIVE_LIST_HPP_

#include "assert.h"

namespace red
{
namespace memory
{
	RED_MEMORY_INLINE IntrusiveSingleLinkedList * IntrusiveSingleLinkedList::GetNext() const
	{
		return m_next;
	}

	RED_MEMORY_INLINE void IntrusiveSingleLinkedList::SetNext( IntrusiveSingleLinkedList * next )
	{
		m_next = next;
	}

	RED_MEMORY_INLINE IntrusiveDoubleLinkedList * IntrusiveDoubleLinkedList::GetNext() const
	{
		return m_next;
	}

	RED_MEMORY_INLINE bool IntrusiveDoubleLinkedList::Empty() const
	{
		return m_next == this;
	}

	RED_MEMORY_INLINE void IntrusiveDoubleLinkedList::PushFront( IntrusiveDoubleLinkedList * node )
	{
		RED_MEMORY_ASSERT( node, "Node cannot be null." );

		IntrusiveDoubleLinkedList *currentNextNode = m_next;
		node->m_next = currentNextNode;
		node->m_previous = this;
		currentNextNode->m_previous = node;
		m_next = node;
	}

	RED_MEMORY_INLINE void IntrusiveDoubleLinkedList::Remove()
	{
		IntrusiveDoubleLinkedList * previous = m_previous;
		IntrusiveDoubleLinkedList * next = m_next;
		next->m_previous = previous;
		previous->m_next = next;
		m_next = this;
		m_previous = this;
	}
}
}

#endif
