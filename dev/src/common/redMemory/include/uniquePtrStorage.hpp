/**
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_UNIQUE_PTR_STORAGE_HPP_
#define _RED_MEMORY_UNIQUE_PTR_STORAGE_HPP_

#include "operators.h"

namespace red
{
namespace memory
{
	//////////////////////////////////////////////////////////////////////////

	template< typename T >
	RED_MEMORY_INLINE void DefaultUniquePtrDestructor::operator()( T * ptr ) const
	{
		RED_DELETE( ptr );
	}

	//////////////////////////////////////////////////////////////////////////

	template< typename T, typename DestructorFunctor >
	RED_MEMORY_INLINE UniquePtrStorage_EmptyDestructor< T, DestructorFunctor >::UniquePtrStorage_EmptyDestructor()
		:	m_pointer( nullptr )
	{}
	
	template< typename T, typename DestructorFunctor >
	RED_MEMORY_INLINE UniquePtrStorage_EmptyDestructor< T, DestructorFunctor >::UniquePtrStorage_EmptyDestructor( T * ptr )
		:	m_pointer( ptr )
	{}
	
	template< typename T, typename DestructorFunctor >
	RED_MEMORY_INLINE UniquePtrStorage_EmptyDestructor< T, DestructorFunctor >::UniquePtrStorage_EmptyDestructor( T * ptr, const DestructorFunctor & destructor )
		:	DestructorFunctor( destructor ),
			m_pointer( ptr )
	{}
	
	template< typename T, typename DestructorFunctor >
	RED_MEMORY_INLINE UniquePtrStorage_EmptyDestructor< T, DestructorFunctor >::UniquePtrStorage_EmptyDestructor( T * ptr, DestructorFunctor && destructor )
		:	DestructorFunctor( std::forward< DestructorFunctor >( destructor ) ),
			m_pointer( ptr )
	{}
	
	template< typename T, typename DestructorFunctor >
	RED_MEMORY_INLINE UniquePtrStorage_EmptyDestructor< T, DestructorFunctor >::UniquePtrStorage_EmptyDestructor( UniquePtrStorage_EmptyDestructor && storage )
		:	DestructorFunctor( std::forward< DestructorFunctor >( storage ) ),
			m_pointer( storage.Release() )
	{}

	template< typename T, typename DestructorFunctor >
	RED_MEMORY_INLINE UniquePtrStorage_EmptyDestructor< T, DestructorFunctor >::~UniquePtrStorage_EmptyDestructor()
	{
		GetDestructor()( m_pointer );
	}

	template< typename T, typename DestructorFunctor >
	RED_MEMORY_INLINE T * UniquePtrStorage_EmptyDestructor< T, DestructorFunctor >::Get() const
	{
		return m_pointer;
	}

	template< typename T, typename DestructorFunctor >
	RED_MEMORY_INLINE T * UniquePtrStorage_EmptyDestructor< T, DestructorFunctor >::Release()
	{
		T* pointer = m_pointer;
		m_pointer = nullptr;
		return pointer;
	}

	template< typename T, typename DestructorFunctor >
	RED_MEMORY_INLINE void UniquePtrStorage_EmptyDestructor< T, DestructorFunctor >::Swap( UniquePtrStorage_EmptyDestructor & storage )
	{
		std::swap( m_pointer, storage.m_pointer );
	}
	
	template< typename T, typename DestructorFunctor >
	RED_MEMORY_INLINE DestructorFunctor & UniquePtrStorage_EmptyDestructor< T, DestructorFunctor >::GetDestructor()
	{
		return *this;
	}
	
	template< typename T, typename DestructorFunctor >
	RED_MEMORY_INLINE const DestructorFunctor & UniquePtrStorage_EmptyDestructor< T, DestructorFunctor >::GetDestructor() const
	{
		return *this;
	}

	//////////////////////////////////////////////////////////////////////////

	template< typename T, typename DestructorFunctor >
	RED_MEMORY_INLINE UniquePtrStorage_AggregateDestructor< T, DestructorFunctor >::UniquePtrStorage_AggregateDestructor()
		:	m_pointer( nullptr )
	{}

	template< typename T, typename DestructorFunctor >
	RED_MEMORY_INLINE UniquePtrStorage_AggregateDestructor< T, DestructorFunctor >::UniquePtrStorage_AggregateDestructor( T * ptr )
		:	m_pointer( ptr )
	{}

	template< typename T, typename DestructorFunctor >
	RED_MEMORY_INLINE UniquePtrStorage_AggregateDestructor< T, DestructorFunctor >::UniquePtrStorage_AggregateDestructor( T * ptr, const DestructorFunctor & destructor )
		:	m_pointer( ptr ),
			m_destructor( destructor )
	{}

	template< typename T, typename DestructorFunctor >
	RED_MEMORY_INLINE UniquePtrStorage_AggregateDestructor< T, DestructorFunctor >::UniquePtrStorage_AggregateDestructor( T * ptr, DestructorFunctor && destructor )
		:	m_pointer( ptr ),
			m_destructor( std::forward< DestructorFunctor >( destructor ) )
	{}

	template< typename T, typename DestructorFunctor >
	RED_MEMORY_INLINE UniquePtrStorage_AggregateDestructor< T, DestructorFunctor >::UniquePtrStorage_AggregateDestructor( UniquePtrStorage_AggregateDestructor && storage )
		:	m_pointer( storage.Release() ),
			m_destructor( std::forward< DestructorFunctor >( storage.m_destructor ) )
	{}

	template< typename T, typename DestructorFunctor >
	RED_MEMORY_INLINE UniquePtrStorage_AggregateDestructor< T, DestructorFunctor >::~UniquePtrStorage_AggregateDestructor()
	{
		if( m_pointer )
		{
			m_destructor( m_pointer );
		}
	}

	template< typename T, typename DestructorFunctor >
	RED_MEMORY_INLINE T * UniquePtrStorage_AggregateDestructor< T, DestructorFunctor >::Get() const
	{
		return m_pointer;
	}

	template< typename T, typename DestructorFunctor >
	RED_MEMORY_INLINE T * UniquePtrStorage_AggregateDestructor< T, DestructorFunctor >::Release()
	{
		T* pointer = m_pointer;
		m_pointer = nullptr;
		return pointer;
	}

	template< typename T, typename DestructorFunctor >
	RED_MEMORY_INLINE void UniquePtrStorage_AggregateDestructor< T, DestructorFunctor >::Swap( UniquePtrStorage_AggregateDestructor & storage )
	{
		std::swap( m_pointer, storage.m_pointer );
		std::swap( m_destructor, storage.m_destructor );
	}

	template< typename T, typename DestructorFunctor >
	RED_MEMORY_INLINE DestructorFunctor & UniquePtrStorage_AggregateDestructor< T, DestructorFunctor >::GetDestructor()
	{
		return m_destructor;
	}

	template< typename T, typename DestructorFunctor >
	RED_MEMORY_INLINE const DestructorFunctor & UniquePtrStorage_AggregateDestructor< T, DestructorFunctor >::GetDestructor() const
	{
		return m_destructor;
	}
}
}

#endif
