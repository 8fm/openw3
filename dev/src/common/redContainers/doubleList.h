/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#ifndef RED_CONTAINERS_DOUBLE_LIST_H
#define RED_CONTAINERS_DOUBLE_LIST_H

namespace Red { namespace Containers {

	template< class ElementType, class Allocator > class DoubleList;

	// Iterators / Nodes kept in namespace to avoid having big nested classes
	namespace DoubleLinkedList
	{
		template< class ElementType >
		class Node : Red::System::NonCopyable
		{
		public:
			Node();
			~Node();
			ElementType m_data;
			Node*		m_next;
			Node*		m_previous;
		};

		template< class ElementType >
		class Iterator
		{
		public:
			template< class ElType, class ListAllocator > friend class Red::Containers::DoubleList;
			template< class ElType > friend class ConstIterator;

			Iterator();
			Iterator( const Iterator& other );
			~Iterator();
			const Iterator& operator=( const Iterator& other );
			
			ElementType& operator*();
			ElementType* operator->();

			Iterator operator++();							
			Iterator operator++( Red::System::Int32 );	

			Iterator operator--();							
			Iterator operator--( Red::System::Int32 );	

			Red::System::Bool operator==( const Iterator& other );
			Red::System::Bool operator!=( const Iterator& other );

		private:
			Node< ElementType >* m_node;
		};

		template< class ElementType >
		class ConstIterator
		{
		public:
			template< class ElType, class ListAllocator > friend class Red::Containers::DoubleList;

			ConstIterator();
			ConstIterator( const ConstIterator& other );
			ConstIterator( const Iterator< ElementType >& other );
			~ConstIterator();
			const ConstIterator& operator=( const ConstIterator& other );

			const ElementType& operator*();
			const ElementType* operator->();

			ConstIterator operator++();							
			ConstIterator operator++( Red::System::Int32 );		

			ConstIterator operator--();							
			ConstIterator operator--( Red::System::Int32 );		

			Red::System::Bool operator==( const ConstIterator& other );
			Red::System::Bool operator!=( const ConstIterator& other );

		private:
			Node< ElementType >* m_node;
		};
	}

	// Non-intrusive double-linked list
	// Uses a link node allocator policy class with 2 static functions, NodeType* AllocateNode< NodeType > and FreeNode< NodeType >( NodeType* )
	template< class ElementType, class LinkNodeAllocatorPolicy >
	class DoubleList
	{
	public:
		DoubleList();
		DoubleList( const DoubleList& other );
		DoubleList( const DoubleList&& other );
		~DoubleList();
		const DoubleList& operator=( const DoubleList& other );
		const DoubleList& operator=( DoubleList&& other );

		// Iterators
		typedef DoubleLinkedList::ConstIterator< ElementType > const_iterator;
		typedef DoubleLinkedList::Iterator< ElementType > iterator;		

		// DoubleList properties
		Red::System::Uint32 Size();			// Node Count
		Red::System::MemSize DataSize();	// Node count * element size (NOT node size!)
		Red::System::Bool Empty();

		// Element manipulation
		void PushFront( const ElementType& data );
		void PushBack( const ElementType& data );
		void PopFront();
		void PopBack();
		void Clear();
		Red::System::Bool Exist( const ElementType& data );
		Red::System::Bool Remove( const ElementType& data );

		// Comparison ops
		Red::System::Bool operator==( const DoubleList& other );
		Red::System::Bool operator!=( const DoubleList& other );

		// Iterator manipulation
		const const_iterator Begin() const;
		const const_iterator End() const;
		iterator Begin();
		iterator End();		

		Red::System::Bool Insert( iterator position, const ElementType& data );
		void Erase( iterator& it );		// iterator is invalid after this operation

	private:
		typedef DoubleLinkedList::Node< ElementType > ListNode;
		ListNode* m_head;								
		ListNode* m_tail;								// Maintain a tail node pointer so push/pop from back is O(1) 
	};

} }

#include "doubleList.inl"

#endif