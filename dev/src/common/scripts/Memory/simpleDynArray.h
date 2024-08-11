/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __RED_SIMPLE_DYNAMIC_ARRAY_H__
#define __RED_SIMPLE_DYNAMIC_ARRAY_H__

#include "../../redSystem/types.h"
#include "../../redSystem/compilerExtensions.h"

#include "allocatorProxy.h"

namespace Red
{
	template< typename T >
	class SimpleDynArray
	{
		typedef System::Uint32 Uint32;

		static const Uint32 DEFAULT_CAPACITY = 8;

	public:
		RED_INLINE SimpleDynArray()
		:	m_allocator( nullptr )
		,	m_data( nullptr )
		,	m_size( 0 )
		,	m_capacity( 0 )
		{
		}

		RED_INLINE ~SimpleDynArray()
		{
			m_allocator->Free( m_data );

			m_allocator = nullptr;
			m_data = nullptr;
			m_size = 0;
			m_capacity = 0;
		}

		RED_INLINE void Initialize( AllocatorProxy* allocProxy, Uint32 InitialCapacity = DEFAULT_CAPACITY )
		{
			m_allocator = allocProxy;
			Expand( InitialCapacity );
		}

		RED_INLINE void PushBack( const T& item )
		{
			if( m_size == m_capacity )
			{
				Expand( m_capacity );
			}

			m_data[ m_size ] = item;
			++m_size;
		}

		RED_INLINE void PopBack()
		{
			RED_ASSERT( m_size > 0, TXT( "Trying to Pop empty array" ) );
			--m_size;
		}

		RED_INLINE T& operator[]( Uint32 index )
		{
			RED_ASSERT( index < m_size, TXT( "Trying to access invalid array index" ) );
			return m_data[ index ];
		}

		RED_INLINE const T& operator[]( Uint32 index ) const
		{
			RED_ASSERT( index < m_size, TXT( "Trying to access invalid array index" ) );
			return m_data[ index ];
		}

		RED_INLINE Uint32 GetSize() const
		{
			return m_size;
		}

	private:

		RED_INLINE void Expand( Uint32 sizeToExpandBy )
		{
			RED_ASSERT( m_allocator, TXT( "Simple dynamic array uninitialised" ) );
			m_capacity += sizeToExpandBy;

			if( m_capacity > 0 )
			{
				m_data = static_cast< T* >( m_allocator->Realloc( m_data, m_capacity * sizeof( T ) ) );
			}
		}

	private:
		AllocatorProxy* m_allocator;
		T* m_data;
		Uint32 m_size;
		Uint32 m_capacity;
	};
}

#endif // __RED_SIMPLE_DYNAMIC_ARRAY_H__
