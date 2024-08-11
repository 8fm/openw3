/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "intrusiveList.h"
#include "assert.h"

namespace red
{
namespace memory
{
	IntrusiveSingleLinkedList::IntrusiveSingleLinkedList()
		: m_next( nullptr )
	{}

	IntrusiveDoubleLinkedList::IntrusiveDoubleLinkedList()
		:	m_next( nullptr ),
			m_previous( nullptr )
	{
		m_next = this;
		m_previous = this;
	}

	

	IntrusiveDoubleLinkedList * IntrusiveDoubleLinkedList::GetPrevious() const
	{
		return m_previous;
	}
}	
}
