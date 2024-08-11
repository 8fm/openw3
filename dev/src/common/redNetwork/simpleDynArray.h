/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __RED_NETWORK_SIMPLE_DYNAMIC_ARRAY_H__
#define __RED_NETWORK_SIMPLE_DYNAMIC_ARRAY_H__

#include "../redSystem/types.h"
#include "../redSystem/compilerExtensions.h"

#include "memory.h"

namespace Red
{
	namespace Network
	{
		template< typename T >
		class SimpleDynArray
		{
			typedef System::Uint32 Uint32;

			static const Uint32 DEFAULT_CAPACITY = 8;

		public:
			RED_INLINE SimpleDynArray()
			:	m_data( nullptr )
			,	m_capacity( 0 )
			{
			}

			RED_INLINE ~SimpleDynArray()
			{
				RED_ASSERT( m_capacity != 0 || m_data == nullptr, TXT( "Capacity should be 0 with no data" ) );
				
				if( m_data != nullptr )
				{				
					Memory::Free( m_data );
					m_data = nullptr;
				}
				m_capacity = 0;
			}

			RED_INLINE void Initialize( Uint32 InitialCapacity = DEFAULT_CAPACITY )
			{
				Expand( InitialCapacity );
			}

			RED_INLINE T& operator[]( Uint32 index )
			{
				RED_ASSERT( index < m_capacity, TXT( "Trying to access invalid array index" ) );
				return m_data[ index ];
			}

			RED_INLINE const T& operator[]( Uint32 index ) const
			{
				RED_ASSERT( index < m_capacity, TXT( "Trying to access invalid array index" ) );
				return m_data[ index ];
			}

			RED_INLINE Uint32 GetCapacity() const
			{
				return m_capacity;
			}

			RED_INLINE void Expand()
			{
				Expand( m_capacity );
			}

			RED_INLINE void Expand( Uint32 sizeToExpandBy )
			{
				m_capacity += sizeToExpandBy;

				if( m_capacity > 0 )
				{
					m_data = static_cast< T* >( Memory::Realloc( m_data, m_capacity * sizeof( T ) ) );
					System::MemoryZero( m_data + ( m_capacity - sizeToExpandBy ), sizeToExpandBy * sizeof( T ) );
				}
			}

		private:
			T* m_data;
			Uint32 m_capacity;
		};
	}
}

#endif // __RED_NETWORK_SIMPLE_DYNAMIC_ARRAY_H__
