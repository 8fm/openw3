/**
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_INCLUDE_UTILS_HPP_
#define _RED_MEMORY_INCLUDE_UTILS_HPP_

#include "../src/assert.h"
#include "../src/block.h"

namespace red
{
namespace memory
{
	RED_MEMORY_INLINE bool IsPowerOf2( u64 value )
	{
		return ( value & ( value - 1 ) ) == 0;
	}

	RED_MEMORY_INLINE bool IsAligned( u64 value, u32 alignment )
	{
		RED_MEMORY_ASSERT( IsPowerOf2( alignment ), "Provided alignment need to be power of 2." );
		return ( value & ( alignment - 1 ) ) == 0;
	}

	RED_MEMORY_INLINE bool IsAligned( const void * memory, u32 alignment )
	{
		return IsAligned( reinterpret_cast< u64 >( memory ), alignment );  
	}

	RED_MEMORY_INLINE u64 AlignAddress( u64 address, u32 alignment )
	{
		RED_MEMORY_ASSERT( IsPowerOf2( alignment ), "Provided alignment need to be power of 2." );
		const u64 mask = -static_cast< i64 >( alignment ); 
		const u64 result = ( address + ( alignment - 1 ) ) & mask; 
		return result;
	}

	RED_MEMORY_INLINE void * AlignAddress( const void * address, u32 alignment )
	{
		u64 value = reinterpret_cast< u64 >( address );
		const u64 result = AlignAddress( value, alignment );
		return reinterpret_cast< void* >( result );
	}

	// Only for power of two. Same than AlignAddress, just for being clear. 
	RED_MEMORY_INLINE u64 RoundUp( u64 value, u64 roundUpTo )
	{
		RED_MEMORY_ASSERT( IsPowerOf2( roundUpTo ), "Function can only be use for power of two value." );
		return ( value + ( roundUpTo - 1 ) ) & ~( roundUpTo - 1 );
	}

	RED_MEMORY_INLINE u32 RoundUp( u32 value, u32 roundUpTo )
	{
		RED_MEMORY_ASSERT( IsPowerOf2( roundUpTo ), "Function can only be use for power of two value." );
		return ( value + ( roundUpTo - 1 ) ) & ~( roundUpTo - 1 );
	} 

	template< typename T >
	RED_MEMORY_INLINE u64 AddressOf( T & value )
	{
		return reinterpret_cast< u64 >( std::addressof( value ) );
	}

	template< typename T >
	RED_MEMORY_INLINE u64 AddressOf( T * value )
	{
		return reinterpret_cast< u64 >( value );
	}

	RED_MEMORY_INLINE void MemsetBlock( const Block & block, i32 value )
	{
		void * addr = reinterpret_cast< void* >( block.address );
		Memset( addr, value, block.size );
	}

	RED_MEMORY_INLINE void MemsetBlock( u64 block, i32 value, u64 size )
	{
		void * addr = reinterpret_cast< void* >( block );
		Memset( addr, value, size );
	}
}
}

#endif
