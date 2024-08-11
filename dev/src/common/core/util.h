/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#ifndef _UTIL_H
#define _UTIL_H

// Offset pointer by number of bytes
template< class T >
RED_INLINE RESTRICT_RETURN T* /*NOALIAS*/ OffsetPtr( T* ptr, ptrdiff_t byteOffset )
{
	return (T*)( (Uint8*) ptr + byteOffset );
}

// Calculate byte offset for two pointers
template< class T >
RED_INLINE ptrdiff_t PtrOffset( T* a, T* b )
{
	return static_cast< ptrdiff_t >( reinterpret_cast<uintptr_t>( a ) - reinterpret_cast<uintptr_t>( b ) );
}

// Align pointer
template< class T >
RED_INLINE RESTRICT_RETURN T* AlignPtr( T* ptr, size_t alignment )
{
	uintptr_t addr = ( uintptr_t ) ptr;
	addr = (addr + (alignment - 1)) & ~( alignment - 1 );
	return ( T* ) addr;
}

// Align offset in memory
RED_INLINE uintptr_t AlignOffset( uintptr_t addr, size_t alignment )
{
	if ( alignment <= 1 )
	{
		return addr;
	}
	else
	{
		return (addr + (alignment - 1)) & ~( alignment - 1 );
	}
}

RED_INLINE Uint32 PtrDiffToUint32( const void* p )
{
	return static_cast< Uint32 >( reinterpret_cast< uintptr_t >( p ) );
}

RED_INLINE Int32 PtrDiffToInt32( const void* p )
{
	return static_cast< Int32 >( reinterpret_cast< ptrdiff_t >( p ) );
}

#define CONCATENATE(x,y) x##y

#define CONCATENATE2(x,y) CONCATENATE(x,y)

//FIXME>>>>>: On Clang __COUNTER__ is unique per translation unit. So for our current use, we need to make variables declared with it static in the .cpp
#define UNIQUE_NAME(prefix) CONCATENATE2(prefix,__COUNTER__)

#endif // _UTIL_H
