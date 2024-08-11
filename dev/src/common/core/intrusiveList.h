#pragma once


// Implementation of intrusive lists. Its just initial implementation, so it needs more testing and so on.
// Basically we can combine with it any time of data using TIntrusiveListElement template, and
// keep pointer to list as TIntrusiveListHead.
// NOTICE: Empty destructors! (clear list elements by your own)
class CIntrusiveListElement;

class IntrusiveList
{
public:
	class Node;
	class Head;

	class Base : public Red::System::NonCopyable
	{
		friend class Node;

	public:
		Node*			m_next;

		RED_INLINE Base()
			: m_next( NULL )													{}
		RED_INLINE Base( Base&& l );
		RED_INLINE Base& operator=( Base&& l );

		RED_INLINE void		ListInsert( Node& e );						// insert new element on the front

		RED_INLINE void		DebugValidate();
	};
	
	class Head : public Base
	{
	public:
		RED_INLINE Head()
			: Base()															{}

		RED_INLINE Head( Head&& l )
			: Base( Move( l ) )													{}
		RED_INLINE ~Head()													{ ListClear(); }

		RED_INLINE Head& operator=( Head&& l )								{ Base::operator=( Move( l ) ); return *this; }

		RED_INLINE Node*		ListLast();
		RED_INLINE void		ListJoin( Head&& l );
		RED_INLINE void		ListClear();
	};

	class Node : public Base
	{
		friend class Base;
	public:
		Base*				m_prev;

	public:
		RED_INLINE Node()
			: Base()
			, m_prev( NULL )													{}

		RED_INLINE Node( Node&& l );
		RED_INLINE Node& operator=( Node&& l );
		RED_INLINE ~Node()													{ ListErase(); }

		RED_INLINE void		ListErase();									// erase element from list its currently on
		RED_INLINE Bool		ListIsAttached() const							{ return m_prev != nullptr; }

		RED_INLINE void		DebugValidate();
	};
};

template < class TData >
class TIntrusiveList : public IntrusiveList
{
public:
	typedef IntrusiveList::Node Node;

	class Head : public IntrusiveList::Head
	{
	private:
		typedef IntrusiveList::Head Super;
	public:
		RED_INLINE Head()
			: Super()															{}

		RED_INLINE Head( Head&& l )
			: Super( Move( l ) )												{}
		RED_INLINE Head& operator=( Head&& l )
		{
			Super::operator=( Move( l ) );
			return *this;
		}

		class Iterator
		{
			Node*										m_p;

		public:
			Iterator( Head* h )
				: m_p( static_cast< Node* >( h->m_next ) )						{}
			Iterator( Node* p )
				: m_p( p )														{}
			

			Bool operator==( const Iterator it ) const							{ return it.m_p == m_p; }
			Bool operator!=( const Iterator it ) const							{ return it.m_p != m_p; }

			Iterator& operator++()												{ m_p = static_cast< Node* >( m_p->m_next ); return *this; }

			TData* operator->() const											{ return static_cast< TData* >( m_p ); }
			TData& operator*() const											{ return *static_cast< TData* >( m_p ); }

			Iterator Erase()													{ Iterator ret( static_cast< Node* >( m_p->m_next ) ); m_p->ListErase(); return ret; }
		};

		class ConstIterator
		{
			const Node*									m_p;

		public:
			ConstIterator( const Head* h )
				: m_p( static_cast< const Node* >( h->m_next ) )				{}
			ConstIterator( const Node* p )
				: m_p( p )														{}
			

			Bool operator==( const ConstIterator it ) const						{ return it.m_p == m_p; }
			Bool operator!=( const ConstIterator it ) const						{ return it.m_p != m_p; }

			ConstIterator& operator++()											{ m_p = static_cast< const Node* >( m_p->m_next ); return *this; }

			const TData* operator->() const										{ return static_cast< const TData* >( m_p ); }
			const TData& operator*() const										{ return *static_cast< const TData* >( m_p ); }
		};

		Bool Empty() const												{ return m_next == nullptr; }
		TData* First()													{ return static_cast< TData* >( static_cast< Node* >( m_next ) ); }

		Iterator Begin()												{ return Iterator( this ); }
		Iterator End()													{ return Iterator( (Node*)(nullptr) ) ; }
		ConstIterator CBegin() const									{ return ConstIterator( this ); }
		ConstIterator CEnd() const										{ return ConstIterator( (Node*)(nullptr) ) ; }
	};
};

#include "intrusiveList.inl"