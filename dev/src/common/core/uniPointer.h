#pragma once

#include "file.h"

/// Pointer holder that does not change its size in 32-bit sytems and is always 64-bits.
template< typename T >
class TUniPointer
{
private:
	#if defined( RED_ARCH_X64 )
		T* m_ptr;
	#else
		Uint32 m_padding;	// always zeros
		T* m_ptr;
	#endif

public:
	RED_INLINE TUniPointer()
		: m_ptr( 0 )
	#if !defined( RED_ARCH_X64 )
		, m_padding( 0 )
	#endif
	{}

	RED_INLINE TUniPointer( T* ptr )
		: m_ptr( ptr )
	#if !defined( RED_ARCH_X64 )
		, m_padding( 0 )
	#endif
	{}

	RED_INLINE TUniPointer( const TUniPointer<T>& other )
		: m_ptr( other.m_ptr )
	#if !defined( RED_ARCH_X64 )
		, m_padding( 0 )
	#endif
	{}

	RED_INLINE TUniPointer<T>& operator=( const TUniPointer<T>& other )
	{
		m_ptr = other.m_ptr;
		return *this;
	}

	RED_INLINE TUniPointer<T>& operator=( T* ptr )
	{
		m_ptr = ptr;
		return *this;
	}

	RED_INLINE bool operator==( const TUniPointer<T>& other ) const
	{
		return m_ptr == other.m_ptr;
	}

	RED_INLINE bool operator==( T* ptr ) const
	{
		return m_ptr == ptr;
	}

	RED_INLINE bool operator!=( const TUniPointer<T>& other ) const
	{
		return m_ptr != other.m_ptr;
	}

	RED_INLINE bool operator!=( T* ptr ) const
	{
		return m_ptr != ptr;
	}

	RED_INLINE bool operator>( const TUniPointer<T>& other ) const
	{
		return (const Uint8*)m_ptr > (const Uint8*)other.m_ptr; // the cast is here so this works even with void*
	}

	RED_INLINE bool operator<( const TUniPointer<T>& other ) const
	{
		return (const Uint8*)m_ptr < (const Uint8*)other.m_ptr; // the cast is here so this works even with void*
	}

	RED_INLINE bool operator>=( const TUniPointer<T>& other ) const
	{
		return (const Uint8*)m_ptr >= (const Uint8*)other.m_ptr; // the cast is here so this works even with void*
	}

	RED_INLINE bool operator<=( const TUniPointer<T>& other ) const
	{
		return (const Uint8*)m_ptr <= (const Uint8*)other.m_ptr; // the cast is here so this works even with void*
	}

	RED_INLINE operator bool() const
	{
		return (m_ptr != NULL);
	}

	RED_INLINE T* operator->() const // this does not preserve the constness of the pointed object
	{
		return m_ptr;
	}

	RED_INLINE T* Get() const // this does not preserve the constness of the pointed object
	{
		return m_ptr;
	}

	RED_INLINE void** GetRef()
	{
		return (void**)&m_ptr;
	}

public:
	//! Serialization
	RED_INLINE friend IFile& operator>>( IFile& file, TUniPointer<T>& ptr )
	{
		file << ptr.m_ptr;
		return file;
	}
};