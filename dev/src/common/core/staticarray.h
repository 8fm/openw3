/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifdef USE_NEW_STATIC_ARRAY

#include "staticarray_new.h"

#else

#include <typeinfo>

#ifdef _DEBUG
	#define STATIC_ARRAY_DEBUG
#endif

/// Access wrapper for static arrays (used by RTTI)
/// Memory layout of this class should be compatible with TStaticArray
class IBaseStaticArray
{
private:
	Uint32			m_size;

#ifdef STATIC_ARRAY_DEBUG
	Uint32			m_underflowCheck;
	RED_ALIGNED_VAR( Int8, 16 )			m_array[ 4 ];
#else
	RED_ALIGNED_VAR( Int8, 16 )			m_array[ 4 ];
#endif

public:
	IBaseStaticArray();

	RED_INLINE Uint32 GetSize() const
	{
		return m_size;
	}

	RED_INLINE const void* GetElement( const Uint32 typeSize, const Uint32 index ) const
	{
		return &m_array[ typeSize * index ];
	}

	RED_INLINE void* GetElement( const Uint32 typeSize, const Uint32 index )
	{
		return &m_array[ typeSize * index ];
	}

	RED_INLINE Uint32 Grow( const Uint32 count )
	{
		const Uint32 index = m_size;
		m_size += count;
		return index;
	}

	RED_INLINE void Remove( const Uint32 count )
	{
		RED_FATAL_ASSERT( count <= m_size, "" );
		m_size -= count;
	}

	RED_INLINE void Clear()
	{
		m_size = 0;
	}

public:
	//! Calculate size of the TStaticArray type
	static Uint32 CalcTypeSize( const Uint32 innerTypeSize, const Uint32 maxSize )
	{
		Uint32 memSize = sizeof( Uint32 );
		memSize += innerTypeSize * maxSize;

#ifdef STATIC_ARRAY_DEBUG
		memSize += sizeof( Uint32 ) * 2;
#endif

		Uint32 correctSize = (memSize + 15) & ~(15);
		return correctSize;
	}
};

/// TDynArray like wrapper for an array with predefined capacity
template < typename T, size_t MaxSize >
class TStaticArray
{
private:
	
	Uint32			m_size;

#ifdef STATIC_ARRAY_DEBUG
	Uint32			m_underflowCheck;
	Int8			m_array[ MaxSize * sizeof( T ) ];
	Uint32			m_overflowCheck;
	static const Uint32 MAGIC_NUMBER1 = 0x12345678;
	static const Uint32 MAGIC_NUMBER2 = 0x90ABCDEF;
#else
	RED_ALIGNED_VAR( Int8, 16 )	m_array[ MaxSize * sizeof( T ) ];
#endif

	

public:
	typedef TInitPolicy<T,TPlainType<T>::Value> TInit;
	typedef TCopyPolicy<T,TCopyableType<T>::Value> TCopy;

public:
	typedef T			value_type;
	typedef T*			iterator;
	typedef const T*	const_iterator;

public:
	RED_INLINE TStaticArray()
		: m_size( 0 )
#ifdef STATIC_ARRAY_DEBUG
		, m_underflowCheck( MAGIC_NUMBER1 )
		, m_overflowCheck( MAGIC_NUMBER2 )
#endif
	{
	}

	RED_INLINE TStaticArray( const TStaticArray<T, MaxSize>& arr )
		: m_size( 0 )
#ifdef STATIC_ARRAY_DEBUG
		, m_underflowCheck( MAGIC_NUMBER1 )
		, m_overflowCheck( MAGIC_NUMBER2 )
#endif
	{
		*this = arr;
	}

	template< Uint32 OtherMaxSize >
	RED_INLINE TStaticArray( const TStaticArray<T, OtherMaxSize>& arr )
		: m_size( 0 )
#ifdef STATIC_ARRAY_DEBUG
		, m_underflowCheck( MAGIC_NUMBER1 )
		, m_overflowCheck( MAGIC_NUMBER2 )
#endif
	{
		*this = arr;
	}

	template< EMemoryClass memoryClass >
	RED_INLINE TStaticArray( const TDynArray<T, memoryClass>& arr )
		: m_size( 0 )
#ifdef STATIC_ARRAY_DEBUG
		, m_underflowCheck( MAGIC_NUMBER1 )
		, m_overflowCheck( MAGIC_NUMBER2 )
#endif
	{
		*this = arr;
	}

	explicit RED_INLINE TStaticArray( Uint32 initialSize )
		: m_size( 0 )
#ifdef STATIC_ARRAY_DEBUG
		, m_underflowCheck( MAGIC_NUMBER1 )
		, m_overflowCheck( MAGIC_NUMBER2 )
#endif
	{
		Resize( initialSize );
	}

	RED_INLINE ~TStaticArray()
	{
		// Destroy objects
		TInit::Destruct( TypedData(), m_size );

#ifdef STATIC_ARRAY_DEBUG
		RED_FATAL_ASSERT( m_underflowCheck == MAGIC_NUMBER1, "" );
		RED_FATAL_ASSERT( m_overflowCheck == MAGIC_NUMBER2, "" );
#endif
	}

	RED_INLINE T* TypedData()
	{
		return reinterpret_cast<T*>( Data() );
	}

	RED_INLINE const T* TypedData() const 
	{
		return reinterpret_cast<const T*>( Data() );
	}

		RED_INLINE const void* Data() const
	{
		return (const void*)&m_array;
	}

	RED_INLINE void* Data()
	{
		return (void*)&m_array;
	}

	RED_INLINE Uint32 Size() const
	{
		return m_size;
	}

	RED_INLINE Bool Empty() const
	{
		return m_size == 0;
	}

	RED_INLINE Bool Full() const
	{
		return m_size == MaxSize;
	}

	RED_INLINE Uint32 Capacity() const
	{
		return MaxSize;
	}

	RED_INLINE Uint32 GetElemSize() const
	{
		return sizeof( T );
	}

	RED_INLINE Uint32 DataSize() const
	{
		return m_size * sizeof(T);
	}

	RED_INLINE void Clear()
	{
		Resize(0);
	}

	RED_INLINE void Clear( Uint32 size )
	{
		RED_UNUSED( size );
		TInit::Destruct( TypedData(), m_size );
		m_size = 0;
	}

	RED_INLINE void ClearFast()
	{
		TInit::Destruct( TypedData(), m_size );
		m_size = 0;
	}

	RED_INLINE void ClearPtr()
	{
		for ( size_t i = 0; i < Size(); ++i )
		{
			delete TypedData()[i];
		}
		Resize(0);
	}

	RED_INLINE void ClearPtrRev()
	{
		const size_t size = Size();
		for ( size_t i = size-1; i != 0; --i )
		{
			delete TypedData()[i];
		}
		Resize(0);
	}

	RED_INLINE const T& operator[](Int32 i) const
	{
		RED_FATAL_ASSERT( (i >= 0) && (i < (Int32)m_size), "" );
		return TypedData()[i];
	}

	RED_INLINE T& operator[](Int32 i)
	{
		RED_FATAL_ASSERT( (i >= 0) && (i < (Int32)m_size), "" );
		return TypedData()[i];
	}

	RED_INLINE const TStaticArray<T,MaxSize>& operator=(const TStaticArray<T,MaxSize> & arr )
	{
		if ( &arr != this )
		{
			InternalCopy( arr.TypedData(), arr.Size() );
		}

		return *this;
	}

	template< size_t OtherMaxSize >
	RED_INLINE const TStaticArray<T,MaxSize>& operator=(const TStaticArray<T,OtherMaxSize> & arr )
	{
		InternalCopy( arr.TypedData(), arr.Size() );

		return *this;
	}

	template< EMemoryClass memoryClass >
	RED_INLINE const TStaticArray<T,MaxSize>& operator=(const TDynArray<T,memoryClass>& arr )
	{
		InternalCopy( arr.TypedData(), arr.Size() );

		return *this;
	}

	template< size_t OtherMaxSize >
	RED_INLINE Bool operator==(const TStaticArray<T,OtherMaxSize>& arr) const
	{
		return (Size() == arr.Size() && TInit::Equal(TypedData(), arr.TypedData(), Size()) );
	}

	template< size_t OtherMaxSize >
	RED_INLINE Bool operator!=(const TStaticArray<T,OtherMaxSize>& arr) const
	{
		return !(*this == arr);
	}

	template< size_t OtherMaxSize >
	RED_INLINE Bool operator<(const TStaticArray<T,OtherMaxSize>& arr) const
	{
		return TInit::Less(TypedData(), Size(), arr.TypedData(), arr.Size());
	}

	template< size_t OtherMaxSize >
	RED_INLINE Bool operator>(const TStaticArray<T,OtherMaxSize>& arr) const
	{
		return TInit::Less(arr.TypedData(), arr.Size(), TypedData(), Size());
	}

	template< size_t OtherMaxSize >
	RED_INLINE Bool operator>=(const TStaticArray<T,OtherMaxSize>& arr) const
	{
		return !TInit::Less(TypedData(), Size(), arr.TypedData(), arr.Size());
	}

	template< size_t OtherMaxSize >
	RED_INLINE Bool operator<=(const TStaticArray<T,OtherMaxSize>& arr) const
	{
		return !TInit::Less(arr.TypedData(), arr.Size(), TypedData(), Size());
	}

	template< EMemoryClass memoryClass >
	void ToDynArray( TDynArray<T,memoryClass>& dynArr )
	{
		dynArr.Resize( Size() );
		TCopy::CopyNonOverlapping( dynArr.TypedData(), TypedData(), Size() );
	}
	
	template< EMemoryClass memoryClass >
	friend TDynArray<T, memoryClass>& operator<<( TDynArray<T,memoryClass>& dynArr, const TStaticArray< T, MaxSize >& arr )
	{
		dynArr.Resize( arr.Size() );
		TCopy::CopyNonOverlapping( dynArr.TypedData(), arr.TypedData(), arr.Size() );
		return dynArr;
	}

	RED_INLINE void PushBack(const T& element)
	{	
		RED_FATAL_ASSERT( m_size + 1 <= MaxSize , "Memory corruption!" );
		TCopy::CopyConstruct( TypedData() + m_size, &element, 1 );
		m_size += 1;
	}

	RED_INLINE void PushBack(T&& element)
	{	
		RED_FATAL_ASSERT( m_size + 1 <= MaxSize , "Memory corruption!" );
		TInit::MoveConstruct( TypedData() + m_size, &element, 1 );
		m_size += 1;
	}

	template< EMemoryClass memoryClass >
	RED_INLINE void PushBack(const TDynArray<T,memoryClass>& arr)
	{	
		size_t arrSize = arr.Size();
		RED_FATAL_ASSERT( m_size + arrSize <= MaxSize, "Memory corruption" );
		TCopy::CopyConstruct( TypedData() + m_size, arr.TypedData(), arrSize );
		m_size += static_cast< Uint32 >( arrSize );
	}

	template< size_t OtherMaxSize >
	RED_INLINE void PushBack(const TStaticArray<T,OtherMaxSize>& arr)
	{
		size_t arrSize = arr.Size();
		ASSERT( m_size + arrSize <= MaxSize );
		TCopy::CopyConstruct( TypedData() + m_size, arr.TypedData(), arrSize );
		m_size += static_cast< Uint32 >( arrSize );
	}

	RED_INLINE Bool PushBackUnique(const T& element)
	{	
		if ( Find( Begin(), End(), element ) == End() )
		{
			PushBack( element );
			return true;
		}
		else
		{
			return false;
		}
	}

	template< EMemoryClass memoryClass >
	RED_INLINE void PushBackUnique(const TDynArray<T,memoryClass>& arr)
	{
		for( size_t i = 0; i < arr.Size(); ++i ) 
		{
			if ( Find( Begin(), End(), arr[i] ) == End() )
			{
				PushBack( arr[i] );
			}
		}
	}

	template< size_t OtherMaxSize >
	RED_INLINE void PushBackUnique(const TStaticArray<T,OtherMaxSize>& arr)
	{
		for( size_t i = 0; i < arr.Size(); ++i ) 
		{
			if ( Find( Begin(), End(), arr[i] ) == End() )
			{
				PushBack( arr[i] );
			}
		}
	}

	RED_INLINE T& Back()
	{
		RED_FATAL_ASSERT( Size() > 0, "" );
		return TypedData()[ Size() - 1 ];
	}

	RED_INLINE const T& Back() const
	{
		RED_FATAL_ASSERT( Size() > 0, "" );
		return TypedData()[ Size() - 1 ];
	}

	RED_INLINE T PopBack()
	{
		RED_FATAL_ASSERT( Size() > 0, "" );
		T elem = TypedData()[ Size() - 1 ];
		Resize( Size() - 1 );
		return elem;
	}

	RED_INLINE T PopBackFast()
	{
		RED_FATAL_ASSERT( Size() > 0, "" );
		T elem = TypedData()[ Size() - 1 ];
		--m_size;
		TInit::Destruct(TypedData() + m_size, 1);
		return elem;
	}

	RED_INLINE void Resize(size_t size)
	{	
		if ( size != m_size )
		{
			const size_t oldSize = m_size;
			m_size = static_cast< Uint32 >( size );
			if(oldSize >= m_size)
			{
				// Call the destructors of any items removed by the resize
				TInit::Destruct(TypedData() + m_size, oldSize - m_size);
			}

			if(oldSize <= m_size)
			{
				// Call the default constructor for any items being added by the resize
				TInit::Construct(TypedData() + oldSize, m_size - oldSize);
			}
		}
	}

	RED_INLINE void Rewind(size_t size)
	{	
		RED_FATAL_ASSERT( size < m_size, "" );
		TInit::Destruct( TypedData() + size, m_size - size );
		m_size = size;
	}

	RED_INLINE void Swap( size_t elem1, size_t elem2 )
	{
		TInit::SwapElements( &TypedData()[elem1], &TypedData()[elem2] );
	}

	RED_INLINE void Swap( iterator elem1, iterator elem2 )
	{
		TInit::SwapElements( elem1, elem2 );
	}

	RED_INLINE const_iterator Begin() const
	{
		return TypedData();
	}

	RED_INLINE const_iterator End() const
	{
		return TypedData() + m_size; 
	}

	RED_INLINE iterator Begin()
	{
		return TypedData();
	}

	RED_INLINE iterator End()
	{
		return TypedData() + m_size;
	}

	RED_INLINE void EraseFast( iterator where )
	{
		*where = *( End() - 1 );
		--m_size;
	}

	RED_INLINE void Erase( iterator where )
	{
		TCopy::MoveBackwards( &(*where), 1, PtrDiffToInt32( (void*)(End() - where - 1) ) );
		--m_size;
	}

	RED_INLINE void Erase( iterator first, iterator last )
	{
		Int32 num = PtrDiffToInt32( (void*)(last - first) );
		TCopy::MoveBackwards( &(*first), num, PtrDiffToInt32( (void*)(End() - first - num) ) );
		m_size -= num;
	}

	RED_INLINE void RemoveAtFast( size_t index )
	{
		RED_FATAL_ASSERT( (index < (Int32)m_size), "" );
		TypedData()[ index ] = Back();
		--m_size;
	}

	RED_INLINE void RemoveAt( size_t index )
	{
		RED_FATAL_ASSERT( (index < (Int32)m_size), "" );
		TCopy::MoveBackwards( TypedData() + index, 1, static_cast< Int32 >( m_size - index - 1 ) );
		--m_size;
	}

	RED_INLINE Bool RemoveFast( const T& element )
	{
		iterator i = Find( Begin(), End(), element );
		if ( i != End() )
		{
			EraseFast( i );
			return true;
		}
		else
		{
			return false;
		}
	}

	RED_INLINE Bool Remove( const T& element )
	{
		iterator i = Find( Begin(), End(), element );
		if ( i != End() )
		{
			Erase( i );
			return true;
		}
		else
		{
			return false;
		}
	}

	RED_INLINE Int32 GetIndex( const T& element ) const
	{
		const_iterator i = Find( Begin(), End(), element );
		if ( i != End() )
		{
			return PtrDiffToInt32( ( void* )( i - Begin() ) );
		}
		else
		{
			return -1;
		}
	}

	RED_INLINE Int32 GetIndex( const_iterator i ) const
	{
		if ( i != End() )
		{
			return i - Begin();
		}
		else
		{
			return -1;
		}
	}

	RED_INLINE Bool Exist( const T& element ) const
	{
		return Find( Begin(), End(), element ) != End();
	}

	RED_INLINE Bool Insert( const Uint32 position, const T& element )
	{
		if ( position <= m_size )
		{
			Uint32 oldSize = static_cast< Uint32 >( Grow(1) );

			TCopy::MoveForwardsInsert( TypedData() + position, oldSize - position, &element, 1 );

			return true;
		}
		else
		{
			return false;
		}
	}

	RED_INLINE size_t Grow(size_t amount = 1)
	{
		size_t oldSize = m_size;
		m_size += static_cast< Uint32 >( amount );
		RED_FATAL_ASSERT( m_size <= MaxSize, "Static array overflow, old size %u, new size %u, max size %u", oldSize, m_size, MaxSize );

		TInit::Construct(TypedData() + oldSize, amount);

		return oldSize;
	}

	template< EMemoryClass memoryClass >
	RED_INLINE TDynArray<T,memoryClass> SubArray( Uint32 start, Int32 count = 0 /* zero means whole */ ) const
	{
		start    = Clamp<Uint32>( start, 0, Size() );
		Uint32 end = ( count > 0 ) ? Min(start + count, Size()) : Size();

		TDynArray<T,memoryClass> subArray;
		subArray.Reserve( end - start );
		for ( ; start != end; ++start )
			subArray.PushBack( operator[]( start ) );
		return subArray;
	}

	RED_INLINE TMemSize SizeOfAllElements() const
	{
		return sizeof( T ) * Capacity();
	}

	RED_INLINE TMemSize GetInternalMemSize() const
	{
		size_t total( 0 );
		for ( size_t i=0; i<m_size; ++i )
			total += TypedData()[ i ].GetInternalMemSize();
		return total;
	}

	RED_INLINE TMemSize GetPointedMemSize() const
	{
		size_t total( 0 );
		for ( size_t i=0; i<m_size; ++i )
			total += TypedData()[ i ]->GetInternalMemSize();
		return total;
	}

	RED_INLINE static const CName& GetTypeName();

private:
	RED_INLINE void InternalCopy( const T* arr, const Uint32 newSize )
	{
		if( newSize == 0 )
		{
			ClearFast();
		}
		else
		{
			const Uint32 oldSize = m_size;

			// There are enough element, copy what is needed, destroy the rest.
			if( newSize <= oldSize ) 
			{
				TCopy::CopyAssign( TypedData(), arr, newSize );
				TInit::Destruct( TypedData() + newSize, oldSize - newSize );
			}
			else 
			{
				RED_FATAL_ASSERT( newSize <= Capacity(), "Memory corruption!" );

				TCopy::CopyAssign( TypedData(), arr, oldSize );
				TCopy::CopyConstruct( TypedData() + oldSize, arr + oldSize, newSize - oldSize );
			}

			m_size = newSize;
		}
	}
};

// PushBack static array to dynarray
template< class T, EMemoryClass memoryClass, size_t MaxSize >
void PushBack( TDynArray<T,memoryClass>& dynArr, const TStaticArray<T,MaxSize>& arr )
{
	size_t prevSize = dynArr.Grow( arr.Size() );
	TStaticArray<T,MaxSize>::TCopy::CopyNonOverlapping( dynArr.TypedData() + prevSize, arr.TypedData(), arr.Size() );
}

// PushBackUnique static array to dynarray
template< class T, EMemoryClass memoryClass, size_t MaxSize >
void PushBackUnique( TDynArray<T,memoryClass>& dynArr, const TStaticArray<T,MaxSize>& arr )
{
	for( size_t i = 0; i < arr.Size(); ++i ) 
	{
		if ( Find( dynArr.Begin(), dynArr.End(), arr[static_cast< Int32 >( i )] ) == dynArr.End() )
		{
			dynArr.PushBack( arr[ static_cast< Int32 >( i )] );
		}
	}
}

//! Remove empty pointers from array
template< class T, size_t MaxSize >
void RemoveEmptyPointers( TStaticArray< T*, MaxSize >& ar )
{
	// Remove all empty elements from array
	size_t size = ar.Size();
	for ( size_t i = size-1; i!=0; --i )
	{
		if ( !ar[i] )
		{
			ar.Erase( ar.Begin() + i );
		}
	}
}

//! Allocate new element and place it at the end of the array
template <class T, size_t MaxSize >
void *operator new ( size_t size, TStaticArray<T,MaxSize> &ar )
{
	RED_UNUSED( size );
	size_t index = ar.Grow( 1 );
	return &ar[ index ];
}

#endif
