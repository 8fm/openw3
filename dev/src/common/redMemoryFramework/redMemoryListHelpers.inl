/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

namespace Red { namespace MemoryFramework { namespace Utils {

	///////////////////////////////////////////////////////////////
	// List Node
	//
	template< class NodeType >
	RED_INLINE ListNode< NodeType >::ListNode()
		: m_next( nullptr )
		, m_previous( nullptr )
	{
	}

	template< class NodeType >
	ListNode< NodeType >::~ListNode()
	{
	}

	template< class NodeType >
	RED_INLINE NodeType* ListNode< NodeType >::GetNext() const
	{
		return m_next;
	}

	template< class NodeType >
	RED_INLINE NodeType* ListNode< NodeType >::GetPrevious() const
	{
		return m_previous;
	}

	template< class NodeType >
	RED_INLINE void ListNode< NodeType >::SetNext( NodeType* next )
	{
		m_next = next;
	}

	template< class NodeType >
	RED_INLINE void ListNode< NodeType >::SetPrevious( NodeType* previous )
	{
		m_previous = previous;
	}

	///////////////////////////////////////////////////////////////
	// PushFront
	//	Add node to front of list
	template< class NodeType >
	RED_INLINE void PushFront( NodeType*& headPtr, NodeType*& tailPtr, NodeType* toInsert )
	{
		toInsert->SetNext( headPtr );
		toInsert->SetPrevious( nullptr );
		if( headPtr )
		{
			headPtr->SetPrevious( toInsert );
		}
		else
		{
			tailPtr = toInsert;
		}
		headPtr = toInsert;
	}

	///////////////////////////////////////////////////////////////
	// PushBack
	//	Add node to back of list
	template< class NodeType >
	RED_INLINE void PushBack( NodeType*& headPtr, NodeType*& tailPtr, NodeType* toInsert )
	{
		toInsert->SetNext( nullptr );
		toInsert->SetPrevious( tailPtr );
		if( tailPtr )
		{
			tailPtr->SetNext( toInsert );
		}
		else
		{
			headPtr = toInsert;
		}
		tailPtr = toInsert;
	}

	///////////////////////////////////////////////////////////////
	// PopFront
	//	Remove node from head
	template< class NodeType >
	RED_INLINE NodeType* PopFront( NodeType*& headPtr, NodeType*& tailPtr )
	{
		NodeType* popped = headPtr;
		if( headPtr != nullptr )
		{
			headPtr = headPtr->GetNext();
			if( headPtr != nullptr )
			{
				headPtr->SetPrevious( nullptr );
			}
			else
			{
				tailPtr = nullptr;
			}

			popped->SetNext( nullptr );
			popped->SetPrevious( nullptr );
		}

		return popped;
	}

	///////////////////////////////////////////////////////////////
	// RemoveFromList
	//	Remove node from any point in a list. 
	template< class NodeType >
	RED_INLINE void RemoveFromList( NodeType*& headPtr, NodeType*& tailPtr, NodeType* nodeInList )
	{
		if( nodeInList->GetPrevious() != nullptr )
		{
			nodeInList->GetPrevious()->SetNext( nodeInList->GetNext() );
		}
		else
		{
			RED_MEMORY_ASSERT( headPtr == nodeInList, "Bad head ptr - list is corrupt" );
			headPtr = nodeInList->GetNext();
		}

		if( nodeInList->GetNext() != nullptr )
		{
			nodeInList->GetNext()->SetPrevious( nodeInList->GetPrevious() );
		}
		else
		{
			RED_MEMORY_ASSERT( tailPtr == nodeInList, "Bad tail ptr - list is corrupt" );
			tailPtr = nodeInList->GetPrevious();
		}

		nodeInList->SetNext( nullptr );
		nodeInList->SetPrevious( nullptr );
	}

	///////////////////////////////////////////////////////////////
	// InsertBefore
	//	Insert node immediately before the 'anchor' node
	template< class NodeType >
	RED_INLINE void InsertBefore( NodeType*& headPtr, NodeType*& tailPtr, NodeType* anchor, NodeType* toInsert )
	{
		toInsert->SetPrevious( anchor->GetPrevious() );
		toInsert->SetNext( anchor );
		if( anchor->GetPrevious() != nullptr )
		{
			anchor->GetPrevious()->SetNext( toInsert );
		}
		else
		{
			RED_MEMORY_ASSERT( headPtr == anchor, "Bad head ptr" );
			headPtr = toInsert;
		}
		anchor->SetPrevious( toInsert );
	}

	///////////////////////////////////////////////////////////////
	// InsertAfter
	//	Insert node immediately after the 'anchor' node
	template< class NodeType >
	RED_INLINE void InsertAfter( NodeType*& headPtr, NodeType*& tailPtr, NodeType* anchor, NodeType* toInsert )
	{
		toInsert->SetPrevious( anchor );
		toInsert->SetNext( anchor->GetNext() );
		if( anchor->GetNext() != nullptr )
		{
			anchor->GetNext()->SetPrevious( toInsert );
		}
		else
		{
			RED_MEMORY_ASSERT( tailPtr == anchor, "Bad tail ptr" );
			tailPtr = toInsert;
		}
		anchor->SetNext( toInsert );
	}


} } } 