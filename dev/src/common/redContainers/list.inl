/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

namespace Red { namespace Containers {

	namespace SingleLinkedList
	{
		//////////////////////////////////////////////////////////////////////
		// Node CTor
		//
		template< class ElementType >
		Node<ElementType>::Node()
			: m_next( nullptr )
		{

		}

		//////////////////////////////////////////////////////////////////////
		// Node DTor
		//
		template< class ElementType >
		Node<ElementType>::~Node()
		{

		}

		//////////////////////////////////////////////////////////////////////
		// Iterator CTor
		//
		template< class ElementType >
		Iterator<ElementType>::Iterator()
			: m_node( nullptr )
		{

		}

		//////////////////////////////////////////////////////////////////////
		// Iterator copy ctor
		//
		template< class ElementType >
		Iterator<ElementType>::Iterator( const Iterator<ElementType>& other )
			: m_node( other.m_node )
		{

		}

		//////////////////////////////////////////////////////////////////////
		// Iterator dtor 
		//
		template< class ElementType >
		Iterator<ElementType>::~Iterator()
		{

		}

		//////////////////////////////////////////////////////////////////////
		// Iterator copy assignment 
		//
		template< class ElementType >
		const Iterator<ElementType>& Iterator<ElementType>::operator=( const Iterator<ElementType>& other )
		{
			m_node = other.m_node;
			return *this;
		}

		//////////////////////////////////////////////////////////////////////
		// Iterator indirection 
		//
		template< class ElementType >
		ElementType& Iterator<ElementType>::operator*()
		{
			return m_node->m_data;
		}

		//////////////////////////////////////////////////////////////////////
		// Iterator deref
		//
		template< class ElementType >
		ElementType* Iterator<ElementType>::operator->()
		{
			return &( m_node->m_data );
		}

		//////////////////////////////////////////////////////////////////////
		// Pre-increment
		//
		template< class ElementType >
		Iterator<ElementType> Iterator<ElementType>::operator++()
		{
			m_node = m_node->m_next;
			return *this;
		}

		//////////////////////////////////////////////////////////////////////
		// Post-increment
		//
		template< class ElementType >
		Iterator<ElementType> Iterator<ElementType>::operator++( Red::System::Int32 )
		{
			Iterator<ElementType> oldIt = *this;
			++(*this);
			return oldIt;
		}

		//////////////////////////////////////////////////////////////////////
		// Iterator equality
		//
		template< class ElementType >
		Red::System::Bool Iterator<ElementType>::operator==( const Iterator<ElementType>& other )
		{
			return m_node == other.m_node;
		}

		//////////////////////////////////////////////////////////////////////
		// Iterator inequality
		//
		template< class ElementType >
		Red::System::Bool Iterator<ElementType>::operator!=( const Iterator<ElementType>& other )
		{
			return m_node != other.m_node;
		}

		//////////////////////////////////////////////////////////////////////
		// ConstIterator Ctor
		//
		template< class ElementType >
		ConstIterator<ElementType>::ConstIterator()
			: m_node( nullptr )
		{

		}

		//////////////////////////////////////////////////////////////////////
		// ConstIterator copy ctor
		//
		template< class ElementType >
		ConstIterator<ElementType>::ConstIterator( const ConstIterator<ElementType>& other )
			: m_node( other.m_node )
		{

		}
		
		//////////////////////////////////////////////////////////////////////
		// ConstIterator copy ctor with Iterator
		//
		template< class ElementType >
		ConstIterator<ElementType>::ConstIterator( const Iterator< ElementType >& other )
		{
			m_node = other.m_node;
		}


		//////////////////////////////////////////////////////////////////////
		// ConstIterator dtor
		//
		template< class ElementType >
		ConstIterator<ElementType>::~ConstIterator()
		{

		}

		//////////////////////////////////////////////////////////////////////
		// ConstIterator copy ctor
		//
		template< class ElementType >
		const ConstIterator<ElementType>& ConstIterator<ElementType>::operator=( const ConstIterator<ElementType>& other )
		{
			m_node = other.m_node;
			return *this;
		}

		//////////////////////////////////////////////////////////////////////
		// ConstIterator indirection
		//
		template< class ElementType >
		const ElementType& ConstIterator<ElementType>::operator*()
		{
			return m_node->m_data;
		}

		//////////////////////////////////////////////////////////////////////
		// ConstIterator deref
		//
		template< class ElementType >
		const ElementType* ConstIterator<ElementType>::operator->()
		{
			return &( m_node->m_data );
		}

		//////////////////////////////////////////////////////////////////////
		// ConstIterator pre-increment
		//
		template< class ElementType >
		ConstIterator<ElementType> ConstIterator<ElementType>::operator++()
		{
			m_node = m_node->m_next;
			return *this;
		}

		//////////////////////////////////////////////////////////////////////
		// ConstIterator post-increment
		//
		template< class ElementType >
		ConstIterator<ElementType> ConstIterator<ElementType>::operator++( Red::System::Int32 )
		{
			ConstIterator<ElementType> oldIt = *this;
			++(*this);
			return oldIt;
		}

		//////////////////////////////////////////////////////////////////////
		// ConstIterator equality
		//
		template< class ElementType >
		Red::System::Bool ConstIterator<ElementType>::operator==( const ConstIterator<ElementType>& other )
		{
			return m_node == other.m_node;
		}

		//////////////////////////////////////////////////////////////////////
		// ConstIterator inequality
		//
		template< class ElementType >
		Red::System::Bool ConstIterator<ElementType>::operator!=( const ConstIterator<ElementType>& other )
		{
			return m_node != other.m_node;
		}
	}

	//////////////////////////////////////////////////////////////////////
	// CTor
	//
	template< class ElementType, class LinkNodeAllocatorPolicy >
	List< ElementType, LinkNodeAllocatorPolicy >::List()
		: m_head( nullptr )
		, m_tail( nullptr )
	{

	}

	//////////////////////////////////////////////////////////////////////
	// Copy CTor
	//	Copy each element individually
	template< class ElementType, class LinkNodeAllocatorPolicy >
	List< ElementType, LinkNodeAllocatorPolicy >::List( const List< ElementType, LinkNodeAllocatorPolicy >& other )
		: m_head( nullptr )
		, m_tail( nullptr )
	{
		for( const_iterator it = other.Begin(); it != other.End(); ++it )
		{
			PushBack( *it );
		}
	}

	//////////////////////////////////////////////////////////////////////
	// Move Ctor
	//
	template< class ElementType, class LinkNodeAllocatorPolicy >
	List< ElementType, LinkNodeAllocatorPolicy >::List( const List< ElementType, LinkNodeAllocatorPolicy >&& other )
		: m_head( other.m_head )
		, m_tail( other.m_tail )
	{
		other.m_head = nullptr;
		other.m_tail = nullptr;
	}

	//////////////////////////////////////////////////////////////////////
	// DTor
	//
	template< class ElementType, class LinkNodeAllocatorPolicy >
	List< ElementType, LinkNodeAllocatorPolicy >::~List()
	{
		Clear();
	}

	//////////////////////////////////////////////////////////////////////
	// Copy assignment
	//	Copy each element individually
	template< class ElementType, class LinkNodeAllocatorPolicy >
	const List< ElementType, LinkNodeAllocatorPolicy >& List< ElementType, LinkNodeAllocatorPolicy >::operator=( const List< ElementType, LinkNodeAllocatorPolicy >& other )
	{
		Clear();
		for( const_iterator it = other.Begin(); it != other.End(); ++it )
		{
			PushBack( *it );
		}
	}

	//////////////////////////////////////////////////////////////////////
	// Move assignment
	//
	template< class ElementType, class LinkNodeAllocatorPolicy >
	const List< ElementType, LinkNodeAllocatorPolicy >& List< ElementType, LinkNodeAllocatorPolicy >::operator=( List< ElementType, LinkNodeAllocatorPolicy >&& other )
	{
		Clear();
		m_head = other.m_head;
		m_tail = other.m_tail;
		other.m_head = nullptr;
		other.m_tail = nullptr;
	}

	//////////////////////////////////////////////////////////////////////
	// Size
	//
	template< class ElementType, class LinkNodeAllocatorPolicy >
	Red::System::Uint32 List< ElementType, LinkNodeAllocatorPolicy >::Size()
	{
		// Walk the list to find the size
		Red::System::Uint32 linkCount = 0;
		const_iterator it = Begin();
		const_iterator end = End();
		while( it != end )
		{
			++linkCount;
			++it;
		}

		return linkCount;
	}

	//////////////////////////////////////////////////////////////////////
	// DataSize
	//	Size of node data (not including nodes themselves!)
	template< class ElementType, class LinkNodeAllocatorPolicy >
	Red::System::MemSize List<ElementType, LinkNodeAllocatorPolicy>::DataSize()
	{
		return Size() * sizeof( ElementType );
	}

	//////////////////////////////////////////////////////////////////////
	// Empty
	//
	template< class ElementType, class LinkNodeAllocatorPolicy >
	Red::System::Bool List<ElementType, LinkNodeAllocatorPolicy>::Empty()
	{
		return m_head == nullptr;
	}

	//////////////////////////////////////////////////////////////////////
	// PushFront
	//
	template< class ElementType, class LinkNodeAllocatorPolicy >
	void List<ElementType, LinkNodeAllocatorPolicy>::PushFront( const ElementType& data )
	{
		ListNode* newNode = LinkNodeAllocatorPolicy::template AllocateNode< ListNode >();
		RED_ASSERT( newNode, TXT( "Failed to allocate linked list node" ) );
		newNode->m_data = data;

		if( m_head )
		{
			newNode->m_next = m_head;
			m_head = newNode;
		}
		else
		{
			m_head = newNode;
			m_tail = newNode;
		}
	}

	//////////////////////////////////////////////////////////////////////
	// PushBack
	//
	template< class ElementType, class LinkNodeAllocatorPolicy >
	void List<ElementType, LinkNodeAllocatorPolicy>::PushBack( const ElementType& data )
	{
		ListNode* newNode = LinkNodeAllocatorPolicy::template AllocateNode< ListNode >();
		RED_ASSERT( newNode, TXT( "Failed to allocate linked list node" ) );
		newNode->m_data = data;

		if( m_tail )
		{
			m_tail->m_next = newNode;
			m_tail = newNode;
		}
		else
		{
			m_head = newNode;
			m_tail = newNode;
		}
	}

	//////////////////////////////////////////////////////////////////////
	// PopFront
	//
	template< class ElementType, class LinkNodeAllocatorPolicy >
	void List<ElementType, LinkNodeAllocatorPolicy>::PopFront()
	{
		RED_ASSERT( m_head != nullptr && m_tail != nullptr, TXT( "Cannot pop from empty list" ) );

		if( m_tail == m_head )
		{
			m_tail = nullptr;
		}

		ListNode* nextNode = m_head->m_next;
		LinkNodeAllocatorPolicy::template FreeNode< ListNode >( m_head );
		m_head = nextNode;
	}

	//////////////////////////////////////////////////////////////////////
	// PopBack
	//	Slowest op due to linear search through all nodes
	template< class ElementType, class LinkNodeAllocatorPolicy >
	void List<ElementType, LinkNodeAllocatorPolicy>::PopBack()
	{
		RED_ASSERT( m_head != nullptr && m_tail != nullptr, TXT( "Cannot pop from empty list" ) );
		
		if( m_tail != m_head )
		{
			ListNode* prevNode = m_head;
			while( prevNode != nullptr )
			{
				if( prevNode->m_next == m_tail )
				{
					// Unlink the tail node
					prevNode->m_next = nullptr;

					// Destroy the tail
					LinkNodeAllocatorPolicy::template FreeNode< ListNode >( m_tail );
					m_tail = prevNode;

					return;
				}
				prevNode = prevNode->m_next;
			}
		}
		else
		{
			// Fast path for single item
			LinkNodeAllocatorPolicy::template FreeNode< ListNode >( m_head );
			m_head = nullptr;
			m_tail = nullptr;
		}
	}

	//////////////////////////////////////////////////////////////////////
	// Clear
	//	Delete all nodes, clear the list
	template< class ElementType, class LinkNodeAllocatorPolicy >
	void List<ElementType, LinkNodeAllocatorPolicy>::Clear()
	{
		ListNode* theNode = m_head;
		while( theNode != nullptr )
		{
			ListNode* nextNode = theNode->m_next;
			LinkNodeAllocatorPolicy::template FreeNode< ListNode >( theNode );
			theNode = nextNode;
		}
		m_head = nullptr;
		m_tail = nullptr;
	}

	//////////////////////////////////////////////////////////////////////
	// Exist
	//
	template< class ElementType, class LinkNodeAllocatorPolicy >
	Red::System::Bool List<ElementType, LinkNodeAllocatorPolicy>::Exist( const ElementType& data )
	{
		for( const_iterator it = Begin(); it != End(); ++it )
		{
			if( *it == data )
			{
				return true;
			}
		}

		return false;
	}

	//////////////////////////////////////////////////////////////////////
	// Remove
	//  Find a node with the matching element and remove it
	template< class ElementType, class LinkNodeAllocatorPolicy >
	Red::System::Bool List<ElementType, LinkNodeAllocatorPolicy>::Remove( const ElementType& data )
	{
		ListNode* toRemove = m_head;
		ListNode* prevNode = nullptr;
		while( toRemove != nullptr )
		{
			if( toRemove->m_data == data )
			{
				// Fix up previous link
				if( prevNode != nullptr )
				{
					prevNode->m_next = toRemove->m_next;
				}

				// Fix up head / tail
				if( m_tail == toRemove )
				{
					m_tail = prevNode;
				}
				if( m_head == toRemove )
				{
					m_head = toRemove->m_next;
				}
				LinkNodeAllocatorPolicy::template FreeNode< ListNode >( toRemove );
				return true;
			}
			prevNode = toRemove;
			toRemove = toRemove->m_next;
		}

		return false;
	}

	//////////////////////////////////////////////////////////////////////
	// Erase
	//	Find a node matching the iterator and delete it
	template< class ElementType, class LinkNodeAllocatorPolicy >
	void List<ElementType, LinkNodeAllocatorPolicy>::Erase( iterator& it )
	{
		Remove( *it );
	}

	//////////////////////////////////////////////////////////////////////
	// Equality op
	//
	template< class ElementType, class LinkNodeAllocatorPolicy >
	Red::System::Bool List<ElementType, LinkNodeAllocatorPolicy>::operator==( const List& other )
	{
		const_iterator it1, it2;

		// Compare elements on each list
		for( it1 = Begin(), it2 = other.Begin(); it1 != End() && it2 != other.End(); ++it1, ++it2 )
		{
			if ( (*it1) != (*it2) )
			{
				return false;
			}
		}

		// End of one list, but the end of other, so not equal
		if( it1 != End() || it2 != other.End() )
		{
			return false;
		}

		return true;
	}

	//////////////////////////////////////////////////////////////////////
	// Inequality op
	//
	template< class ElementType, class LinkNodeAllocatorPolicy >
	Red::System::Bool List<ElementType, LinkNodeAllocatorPolicy>::operator!=( const List& other )
	{
		return !( *this == other );
	}

	//////////////////////////////////////////////////////////////////////
	// Begin
	//
	template< class ElementType, class LinkNodeAllocatorPolicy >
	typename List<ElementType, LinkNodeAllocatorPolicy>::iterator List<ElementType, LinkNodeAllocatorPolicy>::Begin()
	{
		iterator itBegin;
		itBegin.m_node = m_head;
		return itBegin;
	}

	//////////////////////////////////////////////////////////////////////
	// End
	//
	template< class ElementType, class LinkNodeAllocatorPolicy >
	typename List<ElementType, LinkNodeAllocatorPolicy>::iterator List<ElementType, LinkNodeAllocatorPolicy>::End()
	{
		return iterator();
	}

	//////////////////////////////////////////////////////////////////////
	// Begin
	//
	template< class ElementType, class LinkNodeAllocatorPolicy >
	const typename List<ElementType, LinkNodeAllocatorPolicy>::const_iterator List<ElementType, LinkNodeAllocatorPolicy>::Begin() const
	{
		const_iterator itBegin;
		itBegin.m_node = m_head;
		return itBegin;
	}

	//////////////////////////////////////////////////////////////////////
	// End 
	//
	template< class ElementType, class LinkNodeAllocatorPolicy >
	const typename List<ElementType, LinkNodeAllocatorPolicy>::const_iterator List<ElementType, LinkNodeAllocatorPolicy>::End() const
	{
		return const_iterator();
	}

	//////////////////////////////////////////////////////////////////////
	// Insert
	//
	template< class ElementType, class LinkNodeAllocatorPolicy >
	Red::System::Bool List<ElementType, LinkNodeAllocatorPolicy>::Insert( iterator position, const ElementType& data )
	{
		if( position == Begin() )
		{
			PushFront( data );
			return true;
		}
		else
		{
			for( iterator it = Begin(); it != End(); ++it )
			{
				if( it.m_node->m_next == position.m_node )
				{
					ListNode* newNode = LinkNodeAllocatorPolicy::template AllocateNode< ListNode >();
					newNode->m_data = data;
					newNode->m_next = position.m_node;
					it.m_node->m_next = newNode;
					return true;
				}
			}
		}

		return false;
	}

} }