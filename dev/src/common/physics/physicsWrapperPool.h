/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../core/objectReachability.h"
#include "../core/idAllocator.h"

template< typename T, typename T0, RED_CONTAINER_POOL_TYPE memoryPool = MemoryPool_Physics, EMemoryClass memoryClass = MC_PhysicsWrappers >
class TWrappersPool 
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( memoryClass );

protected:
	T* m_wrappers;
	T0* m_contexts;
	Uint32 m_count;
	Int16 m_maxCountedIndex;
	Int16 m_minIndex;
	const Uint16 m_maxSize;
	Uint64 m_wrapperSize;
	bool m_flag0 : 1;
	bool m_flag1 : 1;

	TFastMuiltiStreamList< Int16, Int16, -1 > m_wrappersRemoveQue;
	TFastMuiltiStreamList< Int16, Int16, -1 > m_wrappersDirtyQue;
	TDynArray< Int16 > m_wrappersSimulationQue;

	void Init()
	{
		if( m_wrappers ) return;
		m_wrappers = ( T* ) RED_MEMORY_ALLOCATE_ALIGNED( memoryPool, memoryClass, m_maxSize * sizeof( T ), 16 );

		Red::MemorySet( &m_wrappers[ 0 ], 0xcd, sizeof( T ) * m_maxSize );
		for( Uint32 i = 0; i != m_maxSize; ++i )
		{
			m_wrappers[ i ].m_poolIndex = -1;
		}

		m_contexts = ( T0* ) RED_MEMORY_ALLOCATE_ALIGNED( memoryPool, memoryClass, m_maxSize * sizeof( T0 ), 16 );
		Red::MemorySet( &m_contexts[ 0 ], 0xcd, sizeof( T0 ) * m_maxSize );

		m_wrappersRemoveQue.Reset( m_maxSize );
		m_wrappersDirtyQue.Reset( m_maxSize );

		m_minIndex = 0;
		m_maxCountedIndex = 0;
	}

	Uint16 FindFirstHoleIndex()
	{
		Uint16 result = m_maxSize;
		for( Uint32 i = m_minIndex; i != m_maxSize; ++i )
			if( m_wrappers[ i ].m_poolIndex < 0 )
			{
				m_minIndex = i;
				result = i;
				break;
			}
		if( m_maxCountedIndex < m_minIndex + 1 )
		{
			m_maxCountedIndex = m_minIndex + 1;
		}
		return result;
	}

public:
	TWrappersPool( Uint16 maxSize, bool flag0, bool flag1 ) : m_wrappers( 0 ), m_count( 0 ), m_maxSize( maxSize ), m_wrapperSize( sizeof( T ) ), m_flag0( flag0 ), m_flag1( flag1 )
	{
	}

	~TWrappersPool()
	{
		if( m_wrappers )
		{
			RED_MEMORY_FREE( memoryPool, memoryClass, m_wrappers );
			m_wrappers = nullptr;
		}

		if( m_contexts )
		{
			RED_MEMORY_FREE( memoryPool, memoryClass, m_contexts );
			m_contexts = nullptr;
		}
	}

	bool GetFlag0() const { return m_flag0; }
	bool GetFlag1() const { return m_flag1; }

	Uint64 GetWrapperSize() const { return m_wrapperSize; }

	TDynArray< Int16 >& GetWrappersSimulationQue() { return m_wrappersSimulationQue; }

	TFastMuiltiStreamList< Int16, Int16, -1 >& GetWrappersRemoveQue() { return m_wrappersRemoveQue; }
	void PushWrapperToRemove( T* t )
	{
		m_wrappersRemoveQue.PutChecked( t->GetPoolIndex() );
	}

	TFastMuiltiStreamList< Int16, Int16, -1 >& GetWrappersDirtyQue() { return m_wrappersDirtyQue; }
	void PushDirtyWrapper( T* t )
	{
		m_wrappersDirtyQue.PutChecked( t->GetPoolIndex() );
	}

	T0* GetContextFront()
	{
		return m_contexts;
	}

	T0* GetContext( T* element )
	{
		return &m_contexts[ element->GetPoolIndex() ];
	}

	T0* GetContextAt( Int32 index )
	{
		if( index < 0 ) return nullptr;
		return m_contexts + index;
	}

	Int32 GetWrapperMaxIndex() const { return m_maxCountedIndex; }
	Bool IsEmpty() const { return m_count == 0; }

	T* GetWrapperFront()
	{
		return m_wrappers;
	}

	T* GetWrapperFirst()
	{
		if( !m_wrappers ) return nullptr;
		T* element = ( T* )m_wrappers;
		if( element->GetPoolIndex() > -1 ) return element;
		return GetWrapperNext( element );
	}
	T* GetWrapperNext( T* element )
	{
		if( !m_wrappers ) return nullptr;
		while ( element < ( m_wrappers + m_maxCountedIndex - 1 ) )
		{
			++element;
			if( element->GetPoolIndex() > -1 ) return element;
		}
		return nullptr;
	}

	template< typename T1 >
	void Destroy( T1 * t1 )
	{
		if( m_minIndex > t1->m_poolIndex )
		{
			m_minIndex = t1->m_poolIndex;
		}

		if( t1->m_poolIndex >= m_maxCountedIndex )
		{
			m_maxCountedIndex = t1->m_poolIndex;
		}

		t1->~T1();
		t1->m_poolIndex = -1;
		--m_count;
	}

	template< typename T1 >
	T* Create( T1 t1 )
	{
		if( !m_wrappers ) Init();
		Int16 holeIndex = FindFirstHoleIndex();
		if( holeIndex == m_maxSize )
		{
			return nullptr;
		}
		T* wrapper = &m_wrappers[ holeIndex ];
		wrapper->m_poolIndex = holeIndex;
		::new(wrapper->m_parentHook) T1( t1 );
		wrapper = ::new(wrapper) T();
		++m_count;
		return wrapper;
	}

	template< typename T1, typename T2 >
	T* Create( T1 t1, T2 t2 )
	{
		if( !m_wrappers ) Init();
		Int16 holeIndex = FindFirstHoleIndex();
		if( holeIndex == m_maxSize )
		{
			return nullptr;
		}
		T* wrapper = &m_wrappers[ holeIndex ];
		wrapper->m_poolIndex = holeIndex;
		::new(wrapper->m_parentHook) T1( t1 );
		wrapper = ::new(wrapper) T( t2 );
		++m_count;
		return wrapper;
	}

	template< typename T1, typename T2, typename T3 >
	T* Create( T1 t1, T2 t2, T3 t3 )
	{
		if( !m_wrappers ) Init();
		Int16 holeIndex = FindFirstHoleIndex();
		if( holeIndex == m_maxSize )
		{
			return nullptr;
		}
		T* wrapper = &m_wrappers[ holeIndex ];
		wrapper->m_poolIndex = holeIndex;
		::new(wrapper->m_parentHook) T1( t1 );
		wrapper = ::new(wrapper) T( t2, t3 );
		++m_count;
		return wrapper;
	}

	template< typename T1, typename T2, typename T3, typename T4 >
	T* Create( T1 t1, T2 t2, T3 t3, T4 t4 )
	{
		if( !m_wrappers ) Init();
		Int16 holeIndex = FindFirstHoleIndex();
		if( holeIndex == m_maxSize )
		{
			return nullptr;
		}
		T* wrapper = &m_wrappers[ holeIndex ];
		wrapper->m_poolIndex = holeIndex;
		::new(wrapper->m_parentHook) T1( t1 );
		wrapper = ::new(wrapper) T( t2, t3, t4 );
		++m_count;
		return wrapper;
	}

	template< typename T1, typename T2, typename T3, typename T4, typename T5 >
	T* Create( T1 t1, T2 t2, T3 t3, T4 t4, T5 t5 )
	{
		if( !m_wrappers ) Init();
		Int16 holeIndex = FindFirstHoleIndex();
		if( holeIndex == m_maxSize )
		{
			return nullptr;
		}
		T* wrapper = &m_wrappers[ holeIndex ];
		wrapper->m_poolIndex = holeIndex;
		::new(wrapper->m_parentHook) T1( t1 );
		wrapper = ::new(wrapper) T( t2, t3, t4, t5 );
		++m_count;
	}

	template< typename T1, typename T2, typename T3, typename T4, typename T5, typename T6 >
	T* Create( T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6 )
	{
		if( !m_wrappers ) Init();
		Int16 holeIndex = FindFirstHoleIndex();
		if( holeIndex == m_maxSize )
		{
			return nullptr;
		}
		T* wrapper = &m_wrappers[ holeIndex ];
		wrapper->m_poolIndex = holeIndex;
		::new(wrapper->m_parentHook) T1( t1 );
		wrapper = ::new(wrapper) T( t2, t3, t4, t5, t6 );
		++m_count;
		return wrapper;
	}

	template< typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7 >
	T* Create( T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7 )
	{
		if( !m_wrappers ) Init();
		Int16 holeIndex = FindFirstHoleIndex();
		if( holeIndex == m_maxSize )
		{
			return nullptr;
		}
		T* wrapper = &m_wrappers[ holeIndex ];
		wrapper->m_poolIndex = holeIndex;
		::new(wrapper->m_parentHook) T1( t1 );
		wrapper = ::new(wrapper) T( t2, t3, t4, t5, t6, t7 );
		++m_count;
		return wrapper;
	}

	template< typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8 >
	T* Create( T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8 )
	{
		if( !m_wrappers ) Init();
		Int16 holeIndex = FindFirstHoleIndex();
		if( holeIndex == m_maxSize )
		{
			return nullptr;
		}
		T* wrapper = &m_wrappers[ holeIndex ];
		wrapper->m_poolIndex = holeIndex;
		::new(wrapper->m_parentHook) T1( t1 );
		wrapper = ::new(wrapper) T( t2, t3, t4, t5, t6, t7, t8 );
		++m_count;
		return wrapper;
	}
};