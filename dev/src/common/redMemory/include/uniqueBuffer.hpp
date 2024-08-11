/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_UNIQUE_BUFFER_HPP_
#define _RED_MEMORY_UNIQUE_BUFFER_HPP_

namespace red
{
	RED_MEMORY_INLINE UniqueBuffer::UniqueBuffer()
		:	m_buffer( nullptr ),
			m_size( 0 )
	{}

	RED_MEMORY_INLINE UniqueBuffer::UniqueBuffer( void * buffer, Uint32 size, FreeCallback && callback )
		:	m_buffer( buffer ),
			m_freeCallback( std::forward< FreeCallback >( callback ) ),
			m_size( size )
	{}

	RED_MEMORY_INLINE UniqueBuffer::UniqueBuffer( UniqueBuffer && rvalue )
		:	m_buffer( rvalue.Release() ),
			m_freeCallback( std::forward< FreeCallback >( rvalue.m_freeCallback ) ),
			m_size( std::move( rvalue.m_size ) )
	{}
	
	RED_MEMORY_INLINE UniqueBuffer::~UniqueBuffer()
	{
		if( m_buffer )
		{
			m_freeCallback( m_buffer );
		}
	}

	RED_MEMORY_INLINE void * UniqueBuffer::Get() const
	{
		return m_buffer;
	}
	
	RED_MEMORY_INLINE Uint32 UniqueBuffer::GetSize() const
	{
		return m_size;
	}
	
	RED_MEMORY_INLINE void UniqueBuffer::Reset()
	{
		UniqueBuffer().Swap( *this );
	}
	
	RED_MEMORY_INLINE void * UniqueBuffer::Release()
	{
		void * buffer = m_buffer;
		m_buffer = nullptr;
		return buffer;
	}

	RED_MEMORY_INLINE void UniqueBuffer::Swap( UniqueBuffer & value )
	{
		std::swap( m_buffer, value.m_buffer );
		std::swap( m_freeCallback, value.m_freeCallback );
		std::swap( m_size, value.m_size );
	}

	RED_MEMORY_INLINE UniqueBuffer & UniqueBuffer::operator=( UniqueBuffer && rvalue )
	{
		UniqueBuffer( std::move( rvalue ) ).Swap( *this );
		return *this; 
	}

	RED_MEMORY_INLINE UniqueBuffer::operator UniqueBuffer::bool_operator () const
	{
		return m_buffer != nullptr ? &BoolConversion::valid : 0;
	}
	
	RED_MEMORY_INLINE bool UniqueBuffer::operator!() const
	{
		return m_buffer == nullptr;
	}

	template< typename Pool >
	RED_MEMORY_INLINE UniqueBuffer CreateUniqueBuffer( Uint32 size, Uint32 alignment )
	{
		void * buffer = RED_ALLOCATE_ALIGNED( Pool, size, alignment );
		return MakeUniqueBuffer< Pool >( buffer, size );
	}

	template< typename Proxy >
	RED_MEMORY_INLINE UniqueBuffer CreateUniqueBuffer( Proxy & proxy, Uint32 size, Uint32 alignment )
	{
		void * buffer = RED_ALLOCATE_ALIGNED( proxy, size, alignment );
		auto callback = [&]( const void* buffer ) { RED_FREE( proxy, buffer ); };
		return UniqueBuffer( buffer, size, std::move( callback ) );
	}

	template< typename Pool >
	RED_MEMORY_INLINE UniqueBuffer MakeUniqueBuffer( void * buffer, Uint32 size )
	{
		void (*func)(const void*) = memory::Free< Pool >;
		return UniqueBuffer( buffer, size, func );
	}

	template< typename Proxy >
	RED_MEMORY_INLINE UniqueBuffer MakeUniqueBuffer( Proxy & proxy, void * buffer, Uint32 size )
	{
		auto callback = [&]( void* buffer ) { RED_FREE( proxy, buffer ); };
		return UniqueBuffer( buffer, size, std::move( callback ) );
	}
}

#endif
