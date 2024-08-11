#pragma once
#include "..\..\..\core\types.h"
#include "..\..\..\core\memoryMacros.h"
#include "common.h"

//////////////////////////////////////////////////////////////////////////
// Class represents simple stack allocator.
// Use it wise, cuz it is easy to 'broke' it.
// TODO: 
// - implement self-growing stack allocator
// - allocation with user-defined alignment
// - add stack markers
//////////////////////////////////////////////////////////////////////////

namespace BehaviorGraphTools
{
	class StackAllocator
	{

	private:
		Int8* m_buffer;
		Int8* m_currStackTopPtr;
		Int32 m_stackPoolSize;

_DBG_ONLY_CODE_(Int32 m_currentSizeDebug;)

	//-----------------------------------------------

	public:
		StackAllocator();
		~StackAllocator();

		// Creates a pool of given size. One have to create the pool for stack in order to use it.
		RED_INLINE void CreatePool( Int32 poolSize );

		// Allocate memory from pool and increase stack top pointer
		RED_INLINE void* Allocate( size_t size );

		// Clear stack. WARNING! This will NOT call any dtors for allocated objects! 
		RED_INLINE void RemoveAllFromStack();

		// Convenient function to create new object on top of stack. WARNING! THIS CAN RETURN NULLPTR! (in case when stack is full)
		// TODO: add forwarded params for ctor
		template<class T> RED_INLINE T* CreateOnStack();

	private:
		// can't copy, move or assign (not yet)
		StackAllocator( const StackAllocator& );
		StackAllocator& operator=( const StackAllocator& );
		StackAllocator( const StackAllocator&& );
		StackAllocator& operator=( const StackAllocator&& );

		// Release used pool
		RED_INLINE void ReleasePool();
	};

	//////////////////////////////////////////////////////////////////////////
	// INLINES:
	//////////////////////////////////////////////////////////////////////////

	RED_INLINE StackAllocator::StackAllocator()	
		: m_buffer( nullptr )
		, m_currStackTopPtr( nullptr )
		, m_stackPoolSize( 0 )
_DBG_ONLY_CODE_(, m_currentSizeDebug( 0 ) )
	{
	}

	//////////////////////////////////////////////////////////////////////////
	RED_INLINE StackAllocator::~StackAllocator()
	{
		ReleasePool();
	}

	//////////////////////////////////////////////////////////////////////////
	RED_INLINE void StackAllocator::CreatePool( Int32 poolSize )
	{
		if ( m_buffer )
		{
			ReleasePool();
		}

		m_stackPoolSize = poolSize;
		m_buffer = reinterpret_cast< Int8* >( RED_MEMORY_ALLOCATE( MemoryPool_Animation, MC_Animation, sizeof( Int8 ) * poolSize ) );
		m_currStackTopPtr = m_buffer;

		RED_FATAL_ASSERT( m_buffer != nullptr, "Stack pool not obtained!" );

_DBG_ONLY_CODE_( m_currentSizeDebug = 0; )
	}

	//////////////////////////////////////////////////////////////////////////
	RED_INLINE void StackAllocator::ReleasePool()
	{
		if ( m_buffer )
		{
			RED_MEMORY_FREE( MemoryPool_Animation, MC_Animation, m_buffer );
			m_buffer = nullptr;
			m_currStackTopPtr = nullptr;
			m_stackPoolSize = 0;
		}

_DBG_ONLY_CODE_( m_currentSizeDebug = 0; )
	}

	//////////////////////////////////////////////////////////////////////////
	RED_INLINE void* StackAllocator::Allocate( size_t size )
	{
		Bool canAllocate = ( ( m_currStackTopPtr - m_buffer ) + size ) <= m_stackPoolSize;

		RED_ASSERT( canAllocate , TXT("Stack allocator pool is full!") );

		if ( !canAllocate )
		{
			return nullptr;
		}

		Int8* tempPtr = m_currStackTopPtr;
		m_currStackTopPtr += size;

_DBG_ONLY_CODE_( m_currentSizeDebug += ( Int32 )size; )

		return tempPtr;
	}

	//////////////////////////////////////////////////////////////////////////
	RED_INLINE void StackAllocator::RemoveAllFromStack()
	{
		m_currStackTopPtr = m_buffer;

_DBG_ONLY_CODE_( m_currentSizeDebug = 0; )
	}

	//////////////////////////////////////////////////////////////////////////
	template<typename T>
	RED_INLINE T* StackAllocator::CreateOnStack()
	{
		void* ptr = Allocate( sizeof ( T ) );

		if ( ptr )
		{
			return new( ptr ) T();
		}

		return nullptr;
	}
}