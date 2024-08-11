/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

namespace Red
{
	RED_INLINE UniqueBuffer::UniqueBuffer()
		:	m_buffer( nullptr ),
			m_size( 0 ),
			m_memoryClass( MC_DataBlob )
	{}

	RED_INLINE UniqueBuffer::UniqueBuffer( void * buffer, Uint32 size, MemoryFramework::MemoryClass memClass )
		:	m_buffer( buffer ),
			m_size( size ),
			m_memoryClass( memClass )
	{}
	
	RED_INLINE UniqueBuffer::UniqueBuffer( UniqueBuffer && rvalue )
		:	m_buffer( rvalue.Release() ),
			m_size( std::move( rvalue.m_size ) ),
			m_memoryClass( std::move( rvalue.m_memoryClass ) )
	{}
	
	RED_INLINE UniqueBuffer::~UniqueBuffer()
	{
		RED_MEMORY_FREE( MemoryPool_Default, m_memoryClass, m_buffer );
	}

	RED_INLINE void * UniqueBuffer::Get() const
	{
		return m_buffer;
	}
	
	RED_INLINE Uint32 UniqueBuffer::GetSize() const
	{
		return m_size;
	}
	
	RED_INLINE Red::MemoryFramework::MemoryClass UniqueBuffer::GetMemoryClass() const
	{
		return m_memoryClass;
	}

	RED_INLINE void UniqueBuffer::Reset()
	{
		UniqueBuffer().Swap( *this );
	}
	
	RED_INLINE void UniqueBuffer::Reset( void * buffer, Uint32 size, MemoryFramework::MemoryClass memClass )
	{
		UniqueBuffer( buffer, size, memClass ).Swap( *this );
	}

	RED_INLINE void * UniqueBuffer::Release()
	{
		void * buffer = m_buffer;
		m_buffer = nullptr;
		return buffer;
	}

	RED_INLINE void UniqueBuffer::Swap( UniqueBuffer & value )
	{
		::Swap( m_buffer, value.m_buffer );
		::Swap( m_size, value.m_size );
		::Swap( m_memoryClass, value.m_memoryClass );
	}

	RED_INLINE UniqueBuffer & UniqueBuffer::operator=( UniqueBuffer && rvalue )
	{
		Reset( rvalue.Release(), std::move( rvalue.m_size ), std::move( rvalue.m_memoryClass ) );
		return *this;
	}

	RED_INLINE UniqueBuffer::operator UniqueBuffer::bool_operator () const
	{
		return m_buffer != nullptr ? &BoolConversion::valid : 0;
	}
	
	RED_INLINE bool UniqueBuffer::operator!() const
	{
		return m_buffer == nullptr;
	}

	RED_INLINE UniqueBuffer CreateUniqueBuffer(Uint32 size, Uint32 alignment, MemoryFramework::MemoryClass memClass )
	{
		void * buffer = RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, memClass, size, alignment );
		return UniqueBuffer( buffer, size, memClass );
	}
}
