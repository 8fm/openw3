/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifdef RED_PLATFORM_CONSOLE
// Whether content is expected to be attached at runtime or during startup
# define RED_ASYNC_LAZY_CACHE_CHAIN
#endif

/// Intended for a single producer thread, with multiple consumer threads, on PC/next-gen console architecture (otherwise needs some mem barriers)
/// The expectation is that the consumer might not see the latest values right away, but shouldn't see an increased count before
/// the item is placed in the deque. With x86/64 writes and reads to the same memory location (count variables) aren't going to be 
/// reordered, and the count variable is a data dependency for accessing a pointer to a specific item
/// (vs a flag guarding access to an already known pointer). We use volatile access on writes
/// to guard against compiler reordering. Consumer reads are non-volatile (could be, if really needed),
/// so don't expect to poll in a loop off-thread waiting for an update.
/// Deletes are not supported, since it's expected that we iterating within a safe range at all times.
namespace Helper
{

template< class T, Int32 N >
class CLazyCacheChain
{
private:
#ifdef RED_ASYNC_LAZY_CACHE_CHAIN
	T*		m_list[ N * 2 ];
	Int32	m_backCount;
	Int32	m_frontCount;
#else
	TDynArray< T* > m_syncList; // Allow it to grow to support as many mods as work
#endif

public:
	enum { Size = N };

public:
#ifdef RED_ASYNC_LAZY_CACHE_CHAIN
	typedef T*			value_type;
	typedef T**			iterator;
	typedef const T**	const_iterator;
#else
	typedef typename TDynArray< T* >::iterator iterator;
	typedef typename TDynArray< T* >::const_iterator const_iterator;
	typedef typename TDynArray< T* >::value_type value_type;
#endif

public:
	CLazyCacheChain()
#ifdef RED_ASYNC_LAZY_CACHE_CHAIN
		: m_backCount( 0 )
		, m_frontCount( 0 )
#endif
	{
#ifndef RED_ASYNC_LAZY_CACHE_CHAIN
	m_syncList.Reserve( Size );
#endif
	};

	void ClearPtr()
	{
#ifdef RED_ASYNC_LAZY_CACHE_CHAIN
		// First clear the back count then front count so nothing can see older content before newer content
		// Doesn't need atomic exchange since only supports single threaded producer
		const Int32 oldBackCount = const_cast< volatile Int32& >( m_backCount );
		const Int32 oldFrontCount = const_cast< volatile Int32& >( m_frontCount );
		const_cast< volatile Int32& >( m_backCount ) = 0;
		const_cast< volatile Int32& >( m_frontCount ) = 0;

		const iterator beg = &m_list[Size] - oldFrontCount;
		const iterator end = &m_list[Size] + oldBackCount;
		for ( iterator it = beg; it != end; ++it )
		{
			T* ptr = *it;
			delete ptr;
		}
		Red::System::MemoryZero( m_list, sizeof(m_list) );
#else
		m_syncList.ClearPtrFast();
#endif
	}

	Bool PushFront( T* node )
	{
#ifdef RED_ASYNC_LAZY_CACHE_CHAIN
		const Int32 localFrontCount = const_cast< volatile Int32& >( m_frontCount );
		if ( localFrontCount >= N )
		{
			return false;
		}

		const_cast< T* volatile& >( m_list[Size - 1 - localFrontCount] ) = node;
		const_cast< volatile Int32& >( m_frontCount ) = localFrontCount + 1;
#else
		m_syncList.Insert( 0, node );
#endif

		return true;
	}

	Bool PushBack( T* node )
	{
#ifdef RED_ASYNC_LAZY_CACHE_CHAIN
		const Int32 localBackCount = const_cast< volatile Int32& >( m_backCount );
		if ( localBackCount >= N )
		{
			return false;
		}

		const_cast< T* volatile& >( m_list[Size + localBackCount] ) = node;
		const_cast< volatile Int32& >( m_backCount ) = localBackCount + 1;

#else
		m_syncList.PushBack( node );
#endif

		return true;
	}

	const_iterator Begin() const
	{
#ifdef RED_ASYNC_LAZY_CACHE_CHAIN
		return const_cast< const_iterator >( &m_list[Size] - m_frontCount );
#else
		return m_syncList.Begin();
#endif
	}

	const_iterator End() const
	{
#ifdef RED_ASYNC_LAZY_CACHE_CHAIN
		return const_cast< const_iterator >( &m_list[Size] + m_backCount );
#else
		return m_syncList.End();
#endif
	}

	iterator Begin()
	{
#ifdef RED_ASYNC_LAZY_CACHE_CHAIN
		return &m_list[Size] - m_frontCount;
#else
		return m_syncList.Begin();
#endif
	}

	iterator End()
	{
#ifdef RED_ASYNC_LAZY_CACHE_CHAIN
		return &m_list[Size] + m_backCount;
#else
		return m_syncList.End();
#endif
	}
};

} // namespace Helper

template < class T, Int32 N >
typename Helper::CLazyCacheChain< T, N >::iterator begin( Helper::CLazyCacheChain< T, N >& list ) { return list.Begin(); }

template < class T, Int32 N >
typename Helper::CLazyCacheChain< T, N >::iterator end( Helper::CLazyCacheChain< T, N >& list ) { return list.End(); }

template < class T, Int32 N >
typename Helper::CLazyCacheChain< T, N >::const_iterator begin( const Helper::CLazyCacheChain< T, N >& list ) { return list.Begin(); }

template < class T, Int32 N >
typename Helper::CLazyCacheChain< T, N >::const_iterator end( const Helper::CLazyCacheChain< T, N >& list ) { return list.End(); }
