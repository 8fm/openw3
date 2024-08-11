#pragma once


RED_INLINE IntrusiveList::Base::Base( Base&& l )
	: m_next( l.m_next )
{
	l.m_next = NULL;
	if ( m_next )
	{
		m_next->m_prev = this;
	}
	DebugValidate();
}

RED_INLINE IntrusiveList::Node::Node( Node&& l )
	: Base( Move( l ) )
	, m_prev( l.m_prev )
{
	l.m_prev = NULL;
	if ( m_prev )
	{
		m_prev->m_next = this;
	}
	DebugValidate();
}

RED_INLINE IntrusiveList::Node* IntrusiveList::Head::ListLast()
{
	if ( !m_next )
	{
		return NULL;
	}

	Node* node = m_next;
	while ( node->m_next )
	{
		node = node->m_next;
	}
	return node;
}

RED_INLINE void IntrusiveList::Head::ListJoin( Head&& l )
{
	Node* last = l.ListLast();
	if ( last )
	{
		// fix up list end
		last->m_next = m_next;
		if ( m_next )
		{
			m_next->m_prev = last;
		}

		// fix up list start to myself
		m_next = l.m_next;
		m_next->m_prev = this;

		// trash l
		l.m_next = NULL;
	}
}

RED_INLINE void IntrusiveList::Head::ListClear()
{
	Node* node = m_next;
	while ( node )
	{
		Node* next = node->m_next;
		node->m_prev = NULL;
		node->m_next = NULL;
		node = next;
	}
}

RED_INLINE IntrusiveList::Base& IntrusiveList::Base::operator=( Base&& l )
{
	if ( m_next )
	{
		m_next->m_prev = NULL;
	}
	m_next = l.m_next;
	l.m_next = NULL;
	if ( m_next )
	{
		m_next->m_prev = this;
	}
	DebugValidate();

	return *this;
}

RED_INLINE IntrusiveList::Node& IntrusiveList::Node::operator=( Node&& l )
{
	Base::operator=( Move( l ) );

	if ( m_prev )
	{
		m_prev->m_next = NULL;
	}
	m_prev = l.m_prev;
	l.m_prev = NULL;
	if ( m_prev )
	{
		m_prev->m_next = this;
	}

	DebugValidate();

	return *this;
}

void IntrusiveList::Node::ListErase()
{
	DebugValidate();
	if ( m_prev )
	{
		m_prev->m_next = m_next;
	}
	if ( m_next )
	{
		m_next->m_prev = m_prev;
	}
	m_next = NULL;
	m_prev = NULL;
}

void IntrusiveList::Base::ListInsert( IntrusiveList::Node& e )
{
	e.ListErase();
	if ( m_next )
	{
		m_next->m_prev = &e;
	}
	e.m_prev = this;
	e.m_next = m_next;
	m_next = &e;
	DebugValidate();
}

void IntrusiveList::Base::DebugValidate()
{
	ASSERT( !m_next || m_next->m_prev == this );
}

void IntrusiveList::Node::DebugValidate()
{
	Base::DebugValidate();
	ASSERT( !m_prev || m_prev->m_next == this );
}
