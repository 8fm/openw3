/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_NETWORK_LIST_H_
#define _RED_NETWORK_LIST_H_

#include "../redSystem/error.h"

#include "memory.h"
#include "simpleDynArray.h"

namespace Red
{
	namespace Network
	{
		template< typename T >
		class List
		{
		private:
			typedef System::Uint32 Uint32;
			static const Uint32 INVALID_INDEX = 0xffffffff;

			struct Container
			{
				Uint32 m_next;
				Uint32 m_prev;
				T m_item;
				System::Bool m_inUse;

				Container()
					:	m_next( INVALID_INDEX )
					,	m_prev( INVALID_INDEX )
					,	m_inUse( false )
				{
				}
			};

		public:

			template< typename ContainerType, typename ListType >
			class IteratorBase
			{
			public:
				RED_INLINE IteratorBase()
				:	m_pool( nullptr )
				,	m_position( INVALID_INDEX )
				,	m_start( INVALID_INDEX )
				,	m_end( INVALID_INDEX )
				{
				}

				RED_INLINE IteratorBase( ListType* pool )
					:	m_pool( pool )
					,	m_position( INVALID_INDEX )
					,	m_start( INVALID_INDEX )
					,	m_end( INVALID_INDEX )
				{
				}

				RED_INLINE IteratorBase( ListType* pool, Uint32 position, Uint32 start, Uint32 end )
				:	m_pool( pool )
				,	m_position( position )
				,	m_start( start )
				,	m_end( end )
				{
				}

				// Assignment
				RED_INLINE void operator=( const IteratorBase& other )
				{
					m_pool = other.m_pool;
					m_position = other.m_position;
					m_start = other.m_start;
					m_end = other.m_end;
				}

				// Comparison
				RED_INLINE System::Bool operator==( const IteratorBase& other )
				{
					return m_pool == other.m_pool && m_position == other.m_position;
				}

				RED_INLINE System::Bool operator!=( const IteratorBase& other )
				{
					return !( *this == other );
				}

				// Pre increment
				RED_INLINE void operator++()
				{
					if( m_position != INVALID_INDEX )
					{
						m_position = (*m_pool)[ m_position ].m_next;
					}
					else
					{
						m_position = m_start;
					}
				}

				// Pre decrement
				RED_INLINE void operator--()
				{
					if( m_position != INVALID_INDEX )
					{
						m_position = (*m_pool)[ m_position ].m_prev;
					}
					else
					{
						m_position = m_end;
					}
				}

				RED_INLINE T& operator*()
				{
					return (*m_pool)[ m_position ].m_item;
				}

				RED_INLINE T* operator->()
				{
					return &((*m_pool)[ m_position ].m_item);
				}

				RED_INLINE const T& operator*() const
				{
					return (*m_pool)[ m_position ].m_item;
				}

				RED_INLINE const T* operator->() const
				{
					return &((*m_pool)[ m_position ].m_item);
				}

				RED_INLINE System::Bool IsValid() const { return m_position != INVALID_INDEX; }

			private:
				SimpleDynArray< Container >* m_pool;
				Uint32 m_position;
				Uint32 m_start;
				Uint32 m_end;

				friend class List;
			};

			typedef IteratorBase< Container*, SimpleDynArray< Container > > Iterator;
			typedef IteratorBase< const Container*, const SimpleDynArray< Container > > ConstIterator;

		public:
			RED_INLINE List()
			:	m_size( 0 )
			,	m_start( INVALID_INDEX )
			,	m_end( INVALID_INDEX )
			{}

			RED_INLINE ~List() {}

			RED_INLINE void Initialize()
			{
				m_pool.Initialize();
			}

			RED_INLINE void Initialize( Uint32 initialCapacity )
			{
				m_pool.Initialize( initialCapacity );
			}

			// Add a new item to the end of the list and return a pointer to the new item
			T* Add()
			{
				Uint32 index = FindFree();

				Container& container = m_pool[ index ];

				container.m_inUse = true;

				if( m_end != INVALID_INDEX )
				{
					m_pool[ m_end ].m_next = index;
					container.m_prev = m_end;
				}

				if( m_start == INVALID_INDEX )
				{
					m_start = index;
				}

				m_end = index;

				container.m_item = T();

				++m_size;

				return &container.m_item;
			}

			// Remove item from any point in the list
			// Iterator will be decremented to the previous item in the list
			void Remove( Iterator& iter )
			{
				RED_ASSERT( iter.m_pool == &m_pool );

				Uint32 indexOfItemToBeRemoved = iter.m_position;

				Container* itemToRemove = m_pool[ indexOfItemToBeRemoved ];

				--iter;

				if( itemToRemove->m_next != INVALID_INDEX )
				{
					Container next = m_pool[ itemToRemove->m_next ];
					next.m_prev = itemToRemove->m_prev;
				}
				else
				{
					if( indexOfItemToBeRemoved == m_end )
					{
						m_end = m_pool[ m_end ].m_prev;
					}
				}

				if( itemToRemove->m_prev != INVALID_INDEX )
				{
					Container prev = m_pool[ itemToRemove->m_prev ];
					prev.m_next = itemToRemove->m_next;
				}
				else
				{
					if( indexOfItemToBeRemoved == m_start )
					{
						m_start = m_pool[ m_start ].m_next;
					}
				}

				--m_size;
				itemToRemove->m_next	= INVALID_INDEX;
				itemToRemove->m_prev	= INVALID_INDEX;
				itemToRemove->m_inUse	= false;
			}

			template< typename TSearchType >
			Iterator Find( TSearchType& item )
			{
				List< T >::Iterator iter;
				for( iter = this->Begin(); iter != this->End(); ++iter )
				{
					if( *iter == item )
					{
						return iter;
					}
				}

				return Iterator();
			}

			RED_INLINE void Clear()
			{
				m_size = 0;
				m_start = nullptr;
				m_end = nullptr;

				for( System::Uint32 i = 0; i < m_pool.GetCapacity(); ++i )
				{
					m_pool[ i ].m_next	= nullptr;
					m_pool[ i ].m_prev	= nullptr;
					m_pool[ i ].m_inUse	= false;
				}
			}

			RED_INLINE const T* Front() const
			{
				if( m_start != INVALID_INDEX )
				{
					return &m_pool[ m_start ].m_item;
				}
				return nullptr;
			}

			RED_INLINE T* Front()
			{
				return const_cast< T* >( static_cast< const List< T >* >( this )->Front() );
			}

			RED_INLINE const T* Back() const
			{
				if( m_end != INVALID_INDEX )
				{
					return &m_pool[ m_end ].m_item;
				}
				return nullptr;
			}

			RED_INLINE T* Back()
			{
				return const_cast< T* >( static_cast< const List< T >* >( this )->Back() );
			}

			RED_INLINE void PopFront()
			{
				if( m_start != INVALID_INDEX )
				{
					Container& old = m_pool[ m_start ];
					m_start = old.m_next;

					old.m_next	= INVALID_INDEX;
					old.m_prev	= INVALID_INDEX;
					old.m_inUse	= false;

					if( m_start != INVALID_INDEX )
					{
						m_pool[ m_start ].m_prev = INVALID_INDEX;
					}

					--m_size;

					if( m_size == 0 )
					{
						m_start = INVALID_INDEX;
						m_end = INVALID_INDEX;
					}
				}
			}

			RED_INLINE Iterator Begin()
			{
				return Iterator( &m_pool, m_start, m_start, m_end );
			}

			RED_INLINE Iterator End()
			{
				return Iterator( &m_pool );
			}

			RED_INLINE ConstIterator Begin() const
			{
				return ConstIterator( &m_pool, m_start, m_start, m_end );
			}

			RED_INLINE ConstIterator End() const
			{
				return ConstIterator( &m_pool );
			}

			RED_INLINE Uint32 GetSize() const { return m_size; }

		private:

			Uint32 FindFree()
			{
				Uint32 capacity = m_pool.GetCapacity();
				for( Uint32 i = 0; i < capacity; ++i )
				{
					if( !m_pool[ i ].m_inUse )
					{
						return i;
					}
				}

				m_pool.Expand();
				return capacity;
			}

		private:
			SimpleDynArray< Container > m_pool;
			Uint32 m_size;
			Uint32 m_start;
			Uint32 m_end;
		};
	}
}

#endif //_RED_NETWORK_LIST_H_
