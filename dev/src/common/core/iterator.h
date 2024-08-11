/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

//template< class Content, template <typename> class Collection >
template< class Content, template <typename> class Collection >
class CollectionIterator
{
	typename Collection< Content* >::iterator m_iter;
	typename Collection< Content* >::iterator m_end;

public:
	CollectionIterator( Collection< Content* >& collection )
	{
		m_iter = collection.Begin();
		m_end = collection.End();
	}

	RED_INLINE operator Bool () const
	{
		return m_iter != m_end;
	}

	RED_INLINE void operator++ ()
	{
		++m_iter;
	}

	RED_INLINE Content* operator*()
	{
		return *m_iter;
	}
};

template< typename Content, typename Collection = TDynArray< Content* > >
class ArrayIterator
{
	typename Collection::iterator	m_iter;
	typename Collection::iterator	m_end;

public:
	ArrayIterator( Collection& collection )
	{
		m_iter = collection.Begin();
		m_end = collection.End();
	}

	RED_INLINE operator Bool () const
	{
		return m_iter != m_end;
	}

	RED_INLINE void operator++ ()
	{
		++m_iter;
	}

	RED_INLINE Content* operator*()
	{
		return *m_iter;
	}
};

template< class Content, template <class, typename> class Collection, typename CompareFunc >
class SortedCollectionIterator
{
protected:
	typename Collection< Content, CompareFunc >::iterator m_iter;
	typename Collection< Content, CompareFunc >::iterator m_end;

public:
	SortedCollectionIterator( Collection< Content, CompareFunc >& collection )
	{
		m_iter = collection.Begin();
		m_end = collection.End();
	}

	RED_INLINE operator Bool () const
	{
		return m_iter != m_end;
	}

	RED_INLINE void operator++ ()
	{
		++m_iter;
	}

	RED_INLINE Content& operator*()
	{
		return *m_iter;
	}
};

