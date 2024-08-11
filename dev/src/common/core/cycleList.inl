#pragma once



RED_INLINE CCycleList::CCycleList( CCycleList&& l )
	: m_prev( l.m_prev )
	, m_next( l.m_next )
{
	// fix pointers
	m_next->m_prev = this;
	m_prev->m_next = this;

	// detach l
	l.m_prev = &l;
	l.m_next = &l;
}

RED_INLINE CCycleList::~CCycleList()
{
	m_prev->m_next = m_next;
	m_next->m_prev = m_prev;
}
RED_INLINE CCycleList& CCycleList::operator=( CCycleList&& l )
{
	// detach myself
	m_prev->m_next = m_next;
	m_next->m_prev = m_prev;

	// steal pointers
	m_prev = l.m_prev;
	m_next = l.m_next;

	// fix pointers
	m_next->m_prev = this;
	m_prev->m_next = this;

	// detach l
	l.m_prev = &l;
	l.m_next = &l;

	return *this;
}

RED_INLINE void CCycleList::ListJoin( CCycleList& l )
{
	// store entry point
	CCycleList* prev = l.m_next;
	l.m_next = this;
	// find last element of this list
	CCycleList* next;
	for ( next = this; next->m_next != this; next = next->m_next );
	// connect lists
	next->m_next = prev;
	prev->m_prev = next;
}

RED_INLINE void CCycleList::ListIsDetached()
{
	m_prev->m_next = m_next;
	m_next->m_prev = m_prev;

	m_prev = this;
	m_next = this;
}