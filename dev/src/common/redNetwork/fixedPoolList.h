/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_NETWORK_FIXED_POOL_LIST_H_
#define _RED_NETWORK_FIXED_POOL_LIST_H_

#include "../redSystem/error.h"

namespace Red
{
	namespace Network
	{
		template< typename T, System::Uint32 Capacity >
		class FixedPoolList
		{
		private:
			struct Container
			{
				Container* m_next;
				Container* m_prev;
				T m_item;
				System::Bool m_inUse;

				Container()
					:	m_next( nullptr )
					,	m_prev( nullptr )
					,	m_inUse( false )
				{
				}
			};

		public:

			template< typename ContainerType >
			class IteratorBase
			{
			private:
				RED_INLINE IteratorBase( ContainerType position, ContainerType* start, ContainerType* end )
				:	m_position( position )
				,	m_start( start )
				,	m_end( end )
				{
				}

			public:
				RED_INLINE IteratorBase()
					:	m_position( nullptr )
					,	m_start( nullptr )
					,	m_end( nullptr )
				{
				}

				RED_INLINE IteratorBase( const IteratorBase& other )
				:	m_position( other.m_position )
				,	m_start( other.m_start )
				,	m_end( other.m_end )
				{
				}

				// Assignment
				RED_INLINE void operator=( const IteratorBase& other )
				{
					m_position = other.m_position;
					m_start = other.m_start;
					m_end = other.m_end;
				}

				// Comparison
				RED_INLINE System::Bool operator==( const IteratorBase& other ) const
				{
					return m_position == other.m_position;
				}

				RED_INLINE System::Bool operator!=( const IteratorBase& other ) const
				{
					return m_position != other.m_position;
				}

				// Pre increment
				RED_INLINE void operator++()
				{
					if( m_position )
					{
						m_position = m_position->m_next;
					}
					else
					{
						m_position = *m_start;
					}
				}

				// Pre decrement
				RED_INLINE void operator--()
				{
					if( m_position )
					{
						m_position = m_position->m_prev;
					}
					else
					{
						m_position = *m_end;
					}
				}

				RED_INLINE T& operator*()
				{
					return m_position->m_item;
				}

				RED_INLINE T* operator->()
				{
					return &(m_position->m_item);
				}

				RED_INLINE const T& operator*() const
				{
					return m_position->m_item;
				}

				RED_INLINE const T* operator->() const
				{
					return &(m_position->m_item);
				}

				RED_INLINE System::Bool IsValid() const { return m_position != nullptr; }

			private:
				ContainerType m_position;
				ContainerType* m_start;
				ContainerType* m_end;

				friend class FixedPoolList;
			};

			typedef IteratorBase< Container* > Iterator;
			typedef IteratorBase< const Container* > ConstIterator;

		public:
			RED_INLINE FixedPoolList()
			:	m_size( 0 )
			,	m_start( nullptr )
			,	m_end( nullptr )
			{}

			RED_INLINE ~FixedPoolList() {}

			// Add a new item to the end of the list and return a pointer to the new item
			T* Add()
			{
				Container* container = FindFree();
				if( container )
				{
					container->m_inUse = true;

					if( m_end )
					{
						m_end->m_next = container;

						container->m_prev = m_end;
					}

					if( !m_start )
					{
						m_start = container;
					}

					m_end = container;

					container->m_item = T();

					++m_size;

					return &container->m_item;
				}

				return nullptr;
			}

			// Remove item from any point in the list
			// Iterator will be decremented to the previous item in the list
			void Remove( Iterator& iter )
			{
				Container* itemToRemove = iter.m_position;
				--iter;

				if( itemToRemove->m_next )
				{
					itemToRemove->m_next->m_prev = itemToRemove->m_prev;
				}
				else
				{
					if( itemToRemove == m_end )
					{
						m_end = m_end->m_prev;
					}
				}

				if( itemToRemove->m_prev )
				{
					itemToRemove->m_prev->m_next = itemToRemove->m_next;
				}
				else
				{
					if( itemToRemove == m_start )
					{
						m_start = m_start->m_next;
					}
				}

				--m_size;
				itemToRemove->m_next	= nullptr;
				itemToRemove->m_prev	= nullptr;
				itemToRemove->m_inUse	= false;
			}

			template< typename TSearchType >
			Iterator Find( TSearchType& item )
			{
				FixedPoolList< T, Capacity >::Iterator iter;
				for( iter = this->Begin(); iter != this->End(); ++iter )
				{
					if( iter.m_position->m_item == item )
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

				for( System::Uint32 i = 0; i < Capacity; ++i )
				{
					m_pool[ i ].m_next	= nullptr;
					m_pool[ i ].m_prev	= nullptr;
					m_pool[ i ].m_inUse	= false;
				}
			}

			RED_INLINE const T* Front() const
			{
				if( m_start )
				{
					return &m_start->m_item;
				}
				return nullptr;
			}

			RED_INLINE T* Front()
			{
				return const_cast< T* >( static_cast< const FixedPoolList< T, Capacity >* >( this )->Front() );
			}

			RED_INLINE const T* Back() const
			{
				if( m_end )
				{
					return &m_end->m_item;
				}
				return nullptr;
			}

			RED_INLINE T* Back()
			{
				return const_cast< T* >( static_cast< const FixedPoolList< T, Capacity >* >( this )->Back() );
			}

			RED_INLINE void PopFront()
			{
				if( m_start )
				{
					Container* old = m_start;
					m_start = m_start->m_next;

					old->m_next		= nullptr;
					old->m_prev		= nullptr;
					old->m_inUse	= false;

					if( m_start )
					{
						m_start->m_prev = nullptr;
					}

					--m_size;

					if( m_size == 0 )
					{
						m_end = nullptr;
					}
				}
			}

			RED_INLINE Iterator Begin()
			{
				return Iterator( m_start, &m_start, &m_end );
			}

			RED_INLINE Iterator End()
			{
				return Iterator( nullptr, &m_start, &m_end );
			}

			RED_INLINE ConstIterator Begin() const
			{
				return ConstIterator( m_start, &m_start, &m_end );
			}

			RED_INLINE ConstIterator End() const
			{
				return ConstIterator( nullptr, &m_start, &m_end );
			}

			RED_INLINE System::Uint32 GetSize() const { return m_size; }
			RED_INLINE System::Uint32 GetSpaceRemaining() const { return Capacity - m_size; }

		private:

			Container* FindFree()
			{
				for( System::Uint32 i = 0; i < Capacity; ++i )
				{
					if( !m_pool[ i ].m_inUse )
					{
						return &m_pool[ i ];
					}
				}

				return nullptr;
			}

		private:

			Container m_pool[ Capacity ];
			System::Uint32 m_size;

			Container* m_start;
			Container* m_end;
		};
	}
}

#endif //_RED_NETWORK_FIXED_POOL_LIST_H_
