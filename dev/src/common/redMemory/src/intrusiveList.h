/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_UTILITY_INTRUSIVE_LIST_H_
#define _RED_MEMORY_UTILITY_INTRUSIVE_LIST_H_

namespace red
{
namespace memory
{
	class IntrusiveSingleLinkedList
	{
	public:

		IntrusiveSingleLinkedList();

		IntrusiveSingleLinkedList * GetNext() const;
		void SetNext( IntrusiveSingleLinkedList * next );

	private:

		IntrusiveSingleLinkedList * m_next;
	};

	class IntrusiveDoubleLinkedList
	{
	public:

		IntrusiveDoubleLinkedList();

		void PushFront( IntrusiveDoubleLinkedList * node );
		void Remove();

		IntrusiveDoubleLinkedList * GetNext() const;
		IntrusiveDoubleLinkedList * GetPrevious() const;

		bool Empty() const;

	private:

		IntrusiveDoubleLinkedList * m_next;
		IntrusiveDoubleLinkedList * m_previous;

	};
}
}

#include "intrusiveList.hpp"

#endif
