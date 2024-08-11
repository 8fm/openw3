#pragma once

// This is prototype implementation for cycled list. The cool thing about it, is that is intrusive list where any element can be threated like container itself.
// You can threat lik
// Iterator has awkward interface, but I couldn't find easy way to apply standard iterator interface to this type of iteration (where you finish iteration in starting node).

class CCycleList
{
protected:
	CCycleList*	m_prev;
	CCycleList*	m_next;

public:
	RED_INLINE CCycleList()												{ m_prev = this; m_next = this;  }
	RED_INLINE CCycleList( CCycleList&& l );
	RED_INLINE ~CCycleList();
	RED_INLINE CCycleList& operator=( CCycleList&& l );

	RED_INLINE void		ListJoin( CCycleList& l );						// join two lists
	RED_INLINE void		ListIsDetached();								// erase element from list its currently on
	RED_INLINE Bool		ListEmpty() const								{ return m_prev == this; }
};

template < class TData >
class TCycleList : public CCycleList
{
public:
	RED_INLINE TCycleList()
		: CCycleList()														{}
	RED_INLINE TCycleList( TCycleList&& l )
		: CCycleList( Move( l ) )											{}
	RED_INLINE TCycleList& operator=( TCycleList&& l )					{ CCycleList::operator=( Move( l ) ); return *this; }

	struct Iterator
	{
		Iterator( TCycleList& list )
			: m_it( &list )
			, m_base( &list )												{}
		Bool operator++()													{ return ( m_list = m_it->m_next ) != m_base; }

		TData* operator->() const											{ return static_cast< TData* >( m_it ); }
		TData& operator*() const											{ return *static_cast< TData* >( m_it ); }

		CCycleList*			m_it;
		CCycleList*			m_base;
	};
};



#include "cycleList.inl"