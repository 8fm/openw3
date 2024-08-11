
#pragma once

/**
 *	Alloc array class
 *
 *	Local array is based on alloca function so be careful.
 *	You can use local array only with POD-types ( see Resize function );
 *
 *	Example:
 *	{
 *		TAllocArrayCreate( Int32, locArrayInt, 3 );
 *
 *		locArrayInt.PushBack( 0 );
 *		locArrayInt.PushBack( 1 );
 *		locArrayInt.PushBack( 2 );
 *
 *		for ( Uint32 i=0; i<locArrayInt.Size(); ++i )
 *		{
 *			Int32 var = locArrayInt[ i ];
 *		}
 *	}
 */

#define TAllocArrayCreate( _classT, _name, _numElements ) void* buf##_name = RED_ALLOCA( sizeof( _classT ) * _numElements ); RED_FATAL_ASSERT( buf##_name, "TAllocArray creation fatal assert" ); TAllocArray< _classT > _name( buf##_name, _numElements );

template< typename T >
class TAllocArray
{
protected:
	T*			m_buf;
	Uint32		m_size;
	Uint32		m_numElements;

public:
	TAllocArray( void* buf, Uint32 numElements )
		: m_buf( reinterpret_cast< T* >( buf ) ) 
		, m_size( 0 )
		, m_numElements( numElements )
	{
		
	}

public:
	Uint32 Size() const
	{
		return m_size;
	}

	Int32 SizeInt() const
	{
		return (Int32)m_size;
	}

	void Resize( Uint32 newSize )
	{
		RED_FATAL_ASSERT( newSize <= Capacity(), "" );
		m_size = newSize;
	}

	Bool Empty() const
	{
		return Size() == 0;
	}

	Uint32 Capacity() const
	{
		return m_numElements;
	}

	Bool Full() const
	{
		return Size() == Capacity();
	}

	const T& operator[]( Int32 i ) const
	{
		RED_FATAL_ASSERT( ( i >= 0 ) && ( i < SizeInt() ), "" );
		return TypedData()[i];
	}

	T& operator[]( Int32 i )
	{
		RED_FATAL_ASSERT( ( i >= 0 ) && ( i < SizeInt() ), "" );
		return TypedData()[i];
	}

public:
	void PushBack( const T& element )
	{	
		RED_FATAL_ASSERT( Size() + 1 <= Capacity() , "Memory corruption!" );
		m_buf[ m_size ] = element;
		m_size += 1;
	}

	Bool Exist( const T& element ) const
	{
		const Uint32 num = Size();
		for ( Uint32 i=0; i<num; ++i )
		{
			if ( element == m_buf[ i ] )
			{
				return true;
			}
		}

		return false;
	}

public:
	T* TypedData()
	{
		return m_buf;
	}

	const T* TypedData() const 
	{
		return m_buf;
	}

private:
	TAllocArray( const TAllocArray< T >& );
	TAllocArray& operator=( const TAllocArray< T >& );
};
