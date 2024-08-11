/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef CORE_DYNAMIC_ARRAY_H
#define CORE_DYNAMIC_ARRAY_H

/// Define this macro to use detailed memory metrics of array buffers
/// Note: uses ~300 MB in game, slow but not very slow
//#define USE_ARRAY_METRICS

#include "../redSystem/error.h"
#include "../redSystem/typetraits.h"
#include "memory.h"
#include "file.h"
#include "compressedNumSerializer.h"
#include "algorithms.h"
#include "memoryHelpers.h"

class CName;

#ifdef USE_ARRAY_METRICS

// Internal metrics header
#include "../redMemoryFramework/redMemoryArrayMetrics.h"

// Red IO
#include "../redIO/redIO.h"

// Instantiate the array metrics data as a singleton
typedef TSingleton< Red::MemoryFramework::ArrayMetrics, TNoDestructionLifetime > SArrayMetrics;

#endif

template <typename T, Bool is_copyable_type>
class TCopyPolicy
{
public:
	static RED_INLINE void MoveBackwards(T* buf, Int32 offset, Int32 num)
	{
		// move backwards num elements from (buf+offset) address to (buf) address
		T* from	= buf + offset;
		T* to	= buf;
		for (Int32 it	= num; it; --it)
			*to++ = Move( *from++ );

		// destruct offset elements in reverse order starting from (buf+offset+num) ending at (buf+num)
		T* ptr	= buf + offset + num - 1;
		for (Int32 it	= offset; it; --it)
			ptr--->~T();
	}

	static RED_INLINE void MoveForwardsInsert(T* dstBuf, Int32 dstNum, const T* srcBuf, Int32 srcNum)
	{
		// copy dst buffer by srcNum elements in reverse order
		while(dstNum-->0)
			*(dstBuf+dstNum+srcNum) = Move(*(dstBuf+dstNum));

		// copy src buffer into dst buffer
		while(srcNum-->0)
			*dstBuf++ = *srcBuf++;
	}

	static RED_INLINE void MoveForwardsInsertMove(T* dstBuf, Int32 dstNum, T* srcBuf, Int32 srcNum)
	{
		// copy dst buffer by srcNum elements in reverse order
		while(dstNum-->0)
			*(dstBuf+dstNum+srcNum) = Move(*(dstBuf+dstNum));

		// copy src buffer into dst buffer
		while(srcNum-->0)
			*dstBuf++ = Move(*srcBuf++);
	}

	static RED_INLINE void CopyNonOverlapping(T* dstBuf, const T* srcBuf, Int32 num )
	{
		// copy src buffer into dst buffer
		while( num-- > 0 )
			*dstBuf++ = *srcBuf++;
	}

	static RED_FORCE_INLINE void CopyConstruct( T* dstBuf, const T* srcBuf, size_t num )
	{
		while(num-- > 0)
		{
			::new(dstBuf++) T(*srcBuf++);
		}
	}

	static RED_FORCE_INLINE void CopyAssign( T* dstBuf, const T* srcBuf, size_t num)
	{
		while(num-- > 0)
		{
			*(dstBuf++) = *(srcBuf++);
		}
	}
};

template <typename T>
class TCopyPolicy<T, true>
{
public:
	static RED_INLINE void MoveBackwards(T* buf, Int32 offset, Int32 num)
	{
		Red::System::MemoryCopy(buf, buf + offset, num*sizeof(T));
	}

	static RED_INLINE void MoveForwardsInsert(T* dstBuf, Int32 dstNum, const T* srcBuf, Int32 srcNum)
	{
		Red::System::MemoryMove(dstBuf + srcNum, dstBuf, dstNum*sizeof(T));
		Red::System::MemoryCopy(dstBuf, srcBuf, srcNum*sizeof(T));
	}
	static RED_INLINE void MoveForwardsInsertMove(T* dstBuf, Int32 dstNum, T* srcBuf, Int32 srcNum)
	{
		MoveForwardsInsert( dstBuf, dstNum, srcBuf, srcNum );
	}

	static RED_INLINE void CopyNonOverlapping(T* dstBuf, const T* srcBuf, Int32 num )
	{
		std::copy( srcBuf, srcBuf + num, dstBuf );
	}

	static RED_INLINE void CopyConstruct(  T* __restrict dstBuf, const T* __restrict srcBuf, size_t num)
	{
		std::copy( srcBuf, srcBuf + num, dstBuf );
	}

	static RED_INLINE void CopyAssign( T* __restrict  dstBuf, const T* __restrict  srcBuf, size_t num)
	{
		std::copy( srcBuf, srcBuf + num, dstBuf );
	}
};

template <typename T, Bool is_pod_type>
class TInitPolicy
{
public:
	static RED_INLINE void Construct( T* buf, size_t num )
	{
		// @todo: placement array new doesn#t work here, as it adds constant implementation 
		// dependent offset to memory pointer, for array size
		// new(element) T[num];
		while( num-- > 0 )
		{
			::new(buf++) T;
		}
	}

	static RED_INLINE void Destruct( T* buf, size_t num )
	{
		// loop uses a descending order to preserve the canonical destruction order of C++ 
		if( num == 0 )
		{
			return;
		}
		buf += num - 1;
		while(num-- > 0)
		{
			buf--->~T();
		}
	}

	static RED_INLINE void MoveConstruct( T* dstBuf, T* srcBuf, size_t num )
	{
		while(num-- > 0)
		{
			::new(dstBuf++) T( Move(*srcBuf++) );
		}
	}

	static RED_INLINE Bool Equal( const T* buf1, const T* buf2, size_t num )
	{
		while (num-- > 0)
		{
			if ( !(*buf1++ == *buf2++)  )
			{
				return false;
			}
		}
		return true;
	}

	static RED_INLINE Bool Less( const T* buf1, size_t num1, const T* buf2, size_t num2 )
	{
		while (num1-- > 0 && num2-- > 0)
		{
			if (*buf1 < *buf2)
			{
				return true;
			}
			else if (*buf2 < *buf1)
			{
				return false;
			}
			buf1 ++;
			buf2 ++;
		}

		return (num1 <= 0 && num2 > 0);
	}

	static RED_INLINE void SwapElements(T* elem1, T* elem2)
	{
		T tmp = *elem1;
		*elem1 = *elem2;
		*elem2 = tmp;
	}
};

template <typename T>
class TInitPolicy<T, true>
{
public:
	static RED_INLINE void Construct(T*, size_t)
	{
		// do nothing
	}

	static RED_INLINE void MoveConstruct(T* dstBuf, T* srcBuf, size_t num)
	{
		Red::System::MemoryCopy(dstBuf, srcBuf, num*sizeof(T));
	}

	static RED_INLINE void Destruct(T*, size_t)
	{
		// do nothing
	}

	static RED_INLINE Bool Equal(const T* buf1, const T* buf2, size_t num)
	{
		return 0 == Red::System::MemoryCompare( buf1, buf2, num*sizeof(T) );
	}

	static RED_INLINE Bool Less(const T* buf1, size_t num1, const T* buf2, size_t num2)
	{
		Int32 cmp = Red::System::MemoryCompare(buf1, buf2, num1 < num2 ? num1*sizeof(T) : num2*sizeof(T));
		return (cmp < 0 || (cmp == 0 && num1 < num2));
	}

	static RED_INLINE void SwapElements(T* elem1, T* elem2)
	{
		Uint8 tmpBuf[sizeof(T)];
		Red::System::MemoryCopy(tmpBuf, elem1, sizeof(T));
		Red::System::MemoryCopy(elem1, elem2, sizeof(T));
		Red::System::MemoryCopy(elem2, tmpBuf, sizeof(T));
	}
};

/// Base array interface
#pragma pack (push, 4)
class CBaseArray
{
#ifdef USE_ARRAY_METRICS
protected:
	Red::MemoryFramework::TArrayID						m_arrayID;
	static Uint32 s_arrayMetricsPreallocSize;
	static Uint32 s_tempMetricsPreallocBufferSize;
	static void* s_tempMetricsPreallocBuffer;
	static Red::MemoryFramework::DynamicArrayMetrics*	s_arrayMetricsPreallocated;
#endif

protected:
	void*				m_buf;
	Uint32				m_size;

public:
	RED_INLINE CBaseArray()
		: m_buf( NULL )
		, m_size( 0 )
	{}

	RED_INLINE ~CBaseArray()
	{
		// Data should be freed by now but if not release it anyway
		if ( m_buf )
		{
			RED_FATAL_ASSERT( false, "Destroying a base array, but it still contains elements that cannot be freed" );
			m_buf = NULL;

#ifdef USE_ARRAY_METRICS
			SArrayMetrics::GetInstance().UpdateArrayElementCount( m_arrayID, 0 );
			SArrayMetrics::GetInstance().UpdateArrayMaximumCount( m_arrayID, 0 );
#endif
		}

		UnregisterMetrics();
	}

	void UnregisterMetrics()
	{
#ifdef USE_ARRAY_METRICS
		// Release array metrics data
		if ( m_arrayID != 0 )
		{
			SArrayMetrics::GetInstance().UnregisterArray( m_arrayID );
			m_arrayID = 0;
		}
#endif
	}

	RED_INLINE const void* Data() const
	{		
		return m_buf;
	}

	RED_INLINE void* Data()
	{
		return m_buf;
	}

	RED_INLINE Uint32 Size() const
	{
		return m_size;
	}

	RED_INLINE Int32 SizeInt() const
	{
		return (Int32)m_size;
	}

	RED_INLINE Bool Empty() const
	{
		return m_size == 0;
	}

	RED_INLINE Uint32 Capacity( Uint32 itemSize ) const
	{
		return static_cast< Uint32 >( Memory::GetBlockSize< MemoryPool_Default >( m_buf ) ) / itemSize;
	}

	// Use this when you want to grow the buffer to a known size and will not add more later
	template< typename Type >
	RED_INLINE size_t GrowBufferExact( size_t amount, EMemoryClass memClass )
	{
		RED_FATAL_ASSERT( (size_t)m_size + amount < 0xffffffff, "TDynArray is growing over maximum capacity" );
	
		size_t oldSize = m_size;
		m_size += (Uint32)amount;

		if( m_size > Capacity( sizeof( Type ) ) )
		{
			ResizeBuffer< Type >( m_size, memClass );
		}

#ifdef USE_ARRAY_METRICS
		// Size / allocated changed
		SArrayMetrics::GetInstance().UpdateArrayElementCount( m_arrayID, m_size );
		SArrayMetrics::GetInstance().UpdateArrayMaximumCount( m_arrayID, Capacity() );
#endif

		return oldSize;
	}

	template< typename T >
	RED_INLINE size_t GrowBuffer( size_t amount, EMemoryClass memClass )
	{	
		RED_FATAL_ASSERT( (size_t)m_size + amount < 0xffffffff, "TDynArray is growing over maximum capacity" );

		size_t oldSize = m_size;
		m_size += (Uint32)amount;

		if( m_size > Capacity( sizeof( T ) ) )
		{
			// Increase the size of the buffer by 1.5 each time. This is better than increasing by 2x the size, as it allows 
			// the holes (from freeing the old buffer) to eventually get big enough to fit the new buffer (assuming the allocations happen in contiguous,
			// linear address space)
			// The highest bound we should use is phi (1.618...) as anything over phi would give a decreasing fibbonaci sequence of sizes,
			// and we would never be able to re-use the old holes
			// e.g.
			// grow(32) - alloc(32) - no hole
			// grow(48) - alloc(48), free(32) - 32 byte hole
			// grow(72) - alloc(72), free(48) - 80 byte hole
			// grow(120) - alloc(120), free(72) - 152 byte hole
			// grow(180) - alloc(180), free(120) - 272 byte hole
			// grow(270) - alloc(270) -> HOLE FILLED, free(120)
			RED_FATAL_ASSERT( (size_t)m_size + ((size_t)m_size / 2) < 0xffffffff, "TDynArray is growing over maximum capacity" );

			ResizeBuffer< T >( m_size + (m_size / 2), memClass );
		}

#ifdef USE_ARRAY_METRICS
		// Size / allocated changed
		SArrayMetrics::GetInstance().UpdateArrayElementCount( m_arrayID, m_size );
		SArrayMetrics::GetInstance().UpdateArrayMaximumCount( m_arrayID, Capacity() );
#endif

		return oldSize;
	}

	template< typename T >
	RED_INLINE void Shrink( EMemoryClass memClass )
	{
		if ( m_size != Capacity( sizeof( T ) ) )
		{
			ResizeBuffer< T >( m_size, memClass );

#ifdef USE_ARRAY_METRICS
			SArrayMetrics::GetInstance().UpdateArrayMaximumCount( m_arrayID, Capacity() );
#endif
		}
	}

	RED_INLINE void ShrinkBuffer( size_t count )
	{
		if ( m_size >= count )
		{
			m_size -= (Uint32)count;
		}
		else
		{
			m_size = 0;
		}

#ifdef USE_ARRAY_METRICS
		SArrayMetrics::GetInstance().UpdateArrayElementCount( m_arrayID, m_size );
#endif
	}

	RED_INLINE void Clear( EMemoryClass memClass ) 
	{
		RED_MEMORY_FREE_HYBRID( MemoryPool_Default, memClass, m_buf );
		m_size = 0;
		m_buf = 0;
	}

	RED_INLINE void ClearNoReallocate() // DO NOT USE THAT!!!!! Only for RTTI system
	{
		m_size = 0;
#ifdef USE_ARRAY_METRICS
		SArrayMetrics::GetInstance().UpdateArrayElementCount( m_arrayID, m_size );
#endif
	}

	template< typename T >
	RED_INLINE void Resize( Uint32 newElementCount, EMemoryClass memClass )
	{
		m_size = newElementCount;
		ResizeBuffer< T >( newElementCount, memClass );
	}

	static Red::Threads::CAtomic< int > s_smallArrayCount;

	RED_INLINE void Resize( Uint32 newElementCount, Uint32 itemSize, Uint32 alignment, EMemoryClass memClass )
	{
		m_size = newElementCount;
		const Uint32 allocSize = newElementCount * itemSize;
		m_buf = RED_MEMORY_REALLOCATE_ALIGNED_HYBRID( MemoryPool_Default, m_buf, memClass, allocSize, alignment );
	}

	template< typename T >
	RED_INLINE void ResizeBuffer( Uint32 newElementCount, EMemoryClass memClass )
	{
		const MemSize toAllocate = newElementCount * sizeof( T );
		m_buf = RED_MEMORY_REALLOCATE_ALIGNED_HYBRID( MemoryPool_Default, m_buf, memClass, toAllocate, __alignof( T ) );
	}

	// swap the array data with other array - can be used to fast copy something. Memory pool and class MUST match
	RED_INLINE void SwapWith( CBaseArray& arr2 )
	{
		::Swap( m_buf, arr2.m_buf );
		::Swap( m_size, arr2.m_size );
	}

#ifdef USE_ARRAY_METRICS

	class TArrayDumpToLog
	{
	public:
		void DumpLine( const Char* line );
	};

	class TArrayDumpToFile
	{
	public:
		TArrayDumpToFile( const Char* filePath );
		~TArrayDumpToFile();

		void DumpLine( const Char* line );

	private:
		Red::IO::CNativeFileHandle m_fileHandle;
	};

	template< class OutputType >
	static void DumpArrayMetricsSummary( OutputType& outputter );

	template< class OutputType >
	static void DumpArrayMetricsDetailed( OutputType& outputter );
#endif

protected:
	RED_INLINE void MoveInternals( CBaseArray& from, CBaseArray& to )
	{
		to.m_buf = from.m_buf;
		to.m_size = from.m_size;
		from.m_size = 0;
		from.m_buf = nullptr;
	}

private:
	CBaseArray( const CBaseArray& a );
	CBaseArray& operator=( const CBaseArray& a );
};
#pragma pack(pop)

template 
	< typename T
	, EMemoryClass memoryClass = MC_DynArray
	, RED_CONTAINER_POOL_TYPE memoryPool = MemoryPool_Default
	>
class TDynArray : public CBaseArray
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_DynArray );

public:
	typedef TInitPolicy<T,std::is_scalar<T>::value> TInit;
	typedef TCopyPolicy<T,TCopyableType<T>::Value> TCopy;

public:
	typedef T			value_type;
	typedef T*			iterator;
	typedef const T*	const_iterator;

public:
	RED_INLINE TDynArray()
		: CBaseArray()
	{
	}

	RED_INLINE TDynArray(const TDynArray<T,memoryClass,memoryPool>& arr)
		: CBaseArray()
	{
		m_size = arr.Size();
		ResizeBuffer( m_size );
		TCopy::CopyConstruct( TypedData(), arr.TypedData(), m_size );

#ifdef USE_ARRAY_METRICS
		// Size / allocated size have changed
		SArrayMetrics::GetInstance().UpdateArrayElementCount( m_arrayID, m_size );
		SArrayMetrics::GetInstance().UpdateArrayMaximumCount( m_arrayID, Capacity() );
#endif
	}

	template< EMemoryClass MoveMemoryClass >
	RED_INLINE TDynArray(TDynArray<T,MoveMemoryClass,memoryPool>&& arr)
		:	CBaseArray( )
	{
		static_assert( MoveMemoryClass == memoryClass, "Cannot move to different memory class" );

		MoveInternals( arr, *this );

#ifdef USE_ARRAY_METRICS
		// Update metrics for the moved array and this one
		SArrayMetrics::GetInstance().UpdateArrayElementCount( arr.m_arrayID, arr.m_size );
		SArrayMetrics::GetInstance().UpdateArrayMaximumCount( arr.m_arrayID, arr.Capacity() );

		SArrayMetrics::GetInstance().UpdateArrayElementCount( m_arrayID, m_size );
		SArrayMetrics::GetInstance().UpdateArrayMaximumCount( m_arrayID, Capacity() );
#endif

	}

	explicit RED_INLINE TDynArray( size_t initialSize )
		: CBaseArray( )
	{
		Resize( initialSize );
	}

	RED_INLINE ~TDynArray()
	{
		// Destroy objects
		TInit::Destruct( TypedData(), m_size );

		// Free memory
		if ( m_buf )
		{
			CBaseArray::Clear( memoryClass );
		}

		// Unregister array metrics
		UnregisterMetrics();
	}

	RED_INLINE T* TypedData()
	{
		return reinterpret_cast<T*>( m_buf );
	}

	RED_INLINE const T* TypedData() const 
	{
		return reinterpret_cast<const T*>( m_buf );
	}

	RED_INLINE size_t DataSize() const
	{
		return m_size * sizeof(T);
	}

	RED_INLINE void Clear()
	{
		Resize(0);
	}

	RED_INLINE void ClearFast()
	{
		TInit::Destruct( TypedData(), m_size );
		m_size = 0;
#ifdef USE_ARRAY_METRICS
		SArrayMetrics::GetInstance().UpdateArrayElementCount( m_arrayID, m_size );
#endif
	}

	RED_INLINE void ClearPtr()
	{
		for ( size_t i = 0; i < Size(); ++i )
		{
			delete TypedData()[i];
		}
		Resize(0);
	}

	RED_INLINE void ClearPtrFast()
	{
		for ( size_t i = 0; i < Size(); ++i )
		{
			delete TypedData()[i];
		}
		ResizeFast(0);
	}

	RED_INLINE void ClearPtrRev()
	{
		const Int32 size = (Int32) Size();
		for ( Int32 i = size-1; i>=0; --i )
		{
			delete TypedData()[i];
		}
		Resize(0);
	}

	RED_INLINE const TDynArray<T,memoryClass,memoryPool>& CopyFast(const TDynArray<T,memoryClass,memoryPool>& arr)
	{
		if ( &arr != this )
		{
			InternalCopy( arr );
		}

		return *this;
	}

	RED_INLINE const T& operator[](Int32 i) const
	{
		RED_FATAL_ASSERT( (i >= 0) && (i < (Int32)m_size), "Array: Out of bounds. Cannot access item %i as the array is only size %u", i, m_size );
		return TypedData()[i];
	}

	RED_INLINE T& operator[](Int32 i)
	{
		RED_FATAL_ASSERT( (i >= 0) && (i < (Int32)m_size), "Array: Out of bounds. Cannot access item %i as the array is only size %u\n This is very bad as the program could be about to write to invalid memory location %p", i, m_size, TypedData() + i );
		return TypedData()[i];
	}

	// Copy assignment can use any other array type; move cannot
	RED_INLINE const TDynArray<T,memoryClass,memoryPool>& operator=(const TDynArray<T,memoryClass,memoryPool>& arr)
	{
		if ( &arr != this )
		{
			InternalCopy( arr );
		}

		return *this;
	}

	// Copy assignment can use any other array type; move cannot
	template< EMemoryClass otherMemClass, RED_CONTAINER_POOL_TYPE otherMemPool >
	RED_INLINE const TDynArray<T,memoryClass,memoryPool>& operator=(const TDynArray<T,otherMemClass,otherMemPool>& arr)
	{
		InternalCopy( arr );
		return *this;
	}

	template< EMemoryClass MoveMemoryClass >
	RED_INLINE TDynArray<T,memoryClass,memoryPool>& operator=(TDynArray<T,MoveMemoryClass,memoryPool>&& arr)
	{
		static_assert( MoveMemoryClass == memoryClass, "Cannot move dynarray of different memclass" );
		TDynArray( std::move( arr ) ).SwapWith( *this );
		return *this;
	}

	RED_INLINE Bool operator==(const TDynArray<T,memoryClass,memoryPool>& arr) const
	{
		return (Size() == arr.Size() && TInit::Equal(TypedData(), arr.TypedData(), Size()) );
	}

	RED_INLINE Bool operator!=(const TDynArray<T,memoryClass,memoryPool>& arr) const
	{
		return !(*this == arr);
	}

	RED_INLINE Bool operator<(const TDynArray<T,memoryClass,memoryPool>& arr) const
	{
		return TInit::Less(TypedData(), Size(), arr.TypedData(), arr.Size());
	}

	RED_INLINE Bool operator>(const TDynArray<T,memoryClass,memoryPool>& arr) const
	{
		return TInit::Less(arr.TypedData(), arr.Size(), TypedData(), Size());
	}

	RED_INLINE Bool operator>=(const TDynArray<T,memoryClass,memoryPool>& arr) const
	{
		return !TInit::Less(TypedData(), Size(), arr.TypedData(), arr.Size());
	}

	RED_INLINE Bool operator<=(const TDynArray<T,memoryClass,memoryPool>& arr) const
	{
		return !TInit::Less(arr.TypedData(), arr.Size(), TypedData(), Size());
	}

	RED_INLINE void PushBack(const T& element)
	{	
		size_t oldSize = GrowBuffer(1);
		TCopy::CopyConstruct( TypedData() + oldSize, &element, 1 );
	}

	RED_INLINE void PushBack(T&& element)
	{	
		size_t oldSize = GrowBuffer(1);
		TInit::MoveConstruct( TypedData() + oldSize, &element, 1 );
	}

	RED_INLINE void PushBack(const TDynArray<T,memoryClass,memoryPool>& arr)
	{	
		size_t arrSize = arr.Size();
		size_t oldSize = GrowBuffer(arrSize);

		TCopy::CopyConstruct( TypedData() + oldSize, arr.TypedData(), arrSize );
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

	RED_INLINE Bool PushBackUnique(T&& element)
	{	
		if ( Find( Begin(), End(), element ) == End() )
		{
			PushBack( Move( element ) );
			return true;
		}
		else
		{
			return false;
		}
	}

	RED_INLINE void PushBackUnique(const TDynArray<T,memoryClass,memoryPool>& arr)
	{
		for( Uint32 i = 0; i < arr.Size(); ++i ) 
		{
			if ( Find( Begin(), End(), arr[i] ) == End() )
			{
				PushBack( arr[i] );
			}
		}
	}

	RED_INLINE T& Front()
	{
		RED_FATAL_ASSERT( Size() > 0, "Array: Cannot access last item - array is empty!\nThis is very bad as the program could just happily start writing to incorrect address space %p", ( TypedData() - 1 ) );
		return TypedData()[0];
	}

	RED_INLINE const T& Front() const
	{
		RED_FATAL_ASSERT( Size() > 0, "Array: Cannot access last item - array is empty!\nThis is very bad as the program could just happily start writing to incorrect address space %p", ( TypedData() - 1 ) );
		return TypedData()[0];
	}

	RED_INLINE T& Back()
	{
		RED_FATAL_ASSERT( Size() > 0, "Array: Cannot access last item - array is empty!\nThis is very bad as the program could just happily start writing to incorrect address space %p", ( TypedData() - 1 ) );
		return TypedData()[ Size() - 1 ];
	}

	RED_INLINE const T& Back() const
	{
		RED_FATAL_ASSERT( Size() > 0, "Array: Cannot access last item - array is empty!" );
		return TypedData()[ Size() - 1 ];
	}

	RED_INLINE T PopBack()
	{
		RED_FATAL_ASSERT( Size() > 0, "Array: Cannot access last item - array is empty!\nThis is very bad as you're about to request 4 gigabytes of memory" );
		--m_size;
		T elem = std::move( TypedData()[ m_size ] );
		TInit::Destruct( TypedData() + m_size, 1 );

#ifdef USE_ARRAY_METRICS
		SArrayMetrics::GetInstance().UpdateArrayElementCount( m_arrayID, m_size );
#endif

		return std::move( elem );
	}

	RED_INLINE T PopBackFast()
	{
		return PopBack();
	}

	RED_INLINE T& Last()
	{
		RED_FATAL_ASSERT( Size() > 0, "" );
		return TypedData()[ m_size - 1 ];
	}

	RED_INLINE void Resize(size_t size)
	{	
		RED_FATAL_ASSERT( size < 0xffffffff, "TDynArray is growing over maximum capacity" );
		if ( size != m_size )
		{
			const size_t oldSize = m_size;
			m_size = (Uint32)size;

			if(oldSize >= m_size)
			{
				TInit::Destruct(reinterpret_cast< T* >( m_buf ) + m_size, oldSize - m_size);
			}

			ResizeBuffer( (Uint32)size );

			if(oldSize <= m_size)
			{
				TInit::Construct(reinterpret_cast< T* >( m_buf ) + oldSize, m_size - oldSize);
			}

#ifdef USE_ARRAY_METRICS
			// Size / allocated have changed
			SArrayMetrics::GetInstance().UpdateArrayElementCount( m_arrayID, m_size );
			SArrayMetrics::GetInstance().UpdateArrayMaximumCount( m_arrayID, Capacity() );
#endif
		}
	}

	RED_INLINE void ResizeFast(size_t size)
	{	
		RED_FATAL_ASSERT( size < 0xffffffff, "TDynArray is growing over maximum capacity" );
		if ( size <= Capacity() )
		{
			const size_t oldSize = m_size;
			m_size = (Uint32)size;

			if(oldSize > m_size)
			{
				TInit::Destruct(TypedData() + m_size, Int32(oldSize) - Int32(m_size));
			}
			// Do not resize the buffer - no realloc
			if(oldSize < m_size)
			{
				TInit::Construct(TypedData() + oldSize, Int32(m_size) - Int32(oldSize));
			}

#ifdef USE_ARRAY_METRICS
			SArrayMetrics::GetInstance().UpdateArrayElementCount( m_arrayID, m_size );
#endif
		}
		else
		{
			Resize( size );
		}
	}

	RED_INLINE Uint32 Capacity() const
	{
		return CBaseArray::Capacity( sizeof( T ) );
	}

	RED_INLINE void Rewind(size_t size)
	{	
		RED_FATAL_ASSERT( (size_t)size < 0xffffffff, "TDynArray is rewinding over maximum capacity" );

		RED_FATAL_ASSERT( size < m_size, "" );
		TInit::Destruct( TypedData() + size, Int32(m_size) - Int32(size) );
		m_size = size;

#ifdef USE_ARRAY_METRICS
		SArrayMetrics::GetInstance().UpdateArrayElementCount( m_arrayID, m_size );
#endif
	}

	RED_INLINE void Reserve(size_t size)
	{	
		if ( Capacity() < size)
		{
			ResizeBuffer( (Uint32)size );

#ifdef USE_ARRAY_METRICS
			SArrayMetrics::GetInstance().UpdateArrayMaximumCount( m_arrayID, Capacity() );
#endif
		}
	}

	RED_INLINE void Shrink()
	{
		if ( Capacity() > m_size )
		{
			CBaseArray::Shrink< T >( memoryClass );
		}
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
		if ( where != ( End() - 1 ))
		{
			*where = Move(*( End() - 1 ));
		}
		TInit::Destruct( End()-1, 1 );
		--m_size;

#ifdef USE_ARRAY_METRICS
		SArrayMetrics::GetInstance().UpdateArrayElementCount( m_arrayID, m_size );
#endif
	}

	RED_INLINE void Erase( iterator where )
	{
		TCopy::MoveBackwards( &(*where), 1, PtrDiffToInt32((void*)(End() - where - 1)) );
		--m_size;

#ifdef USE_ARRAY_METRICS
		SArrayMetrics::GetInstance().UpdateArrayElementCount( m_arrayID, m_size );
#endif
	}

	RED_INLINE void Erase( iterator first, iterator last )
	{
		Int32 num = PtrDiffToInt32( (void*)(last - first) );
		TCopy::MoveBackwards( &(*first), PtrDiffToInt32((void*)(last - first)), PtrDiffToInt32( (void*)( End() - first - num ) ) );
		m_size -= num;
#ifdef USE_ARRAY_METRICS
		SArrayMetrics::GetInstance().UpdateArrayElementCount( m_arrayID, m_size );
#endif
	}

	RED_INLINE void RemoveAtFast( size_t index )
	{
		RED_FATAL_ASSERT( index < m_size, "Array: Out of bounds. Cannot remove item %i as the array is only size %u", index, m_size );

		//TInit::Destruct( TypedData() + index, 1 );
		if (index < (m_size-1))
		{
			TypedData()[ index ] = Move( Back() );
		}
		TInit::Destruct( TypedData() + (m_size-1), 1 );
		--m_size;

#ifdef USE_ARRAY_METRICS
		SArrayMetrics::GetInstance().UpdateArrayElementCount( m_arrayID, m_size );
#endif
	}

	RED_INLINE void RemoveAt( size_t index )
	{
		RED_FATAL_ASSERT( index < m_size, "Array: Out of bounds. Cannot remove item %i as the array is only size %u", index, m_size );

		TCopy::MoveBackwards( TypedData() + index, 1, static_cast< Int32 >(m_size - index - 1) );
		--m_size;

#ifdef USE_ARRAY_METRICS
		SArrayMetrics::GetInstance().UpdateArrayElementCount( m_arrayID, m_size );
#endif
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

	RED_INLINE ptrdiff_t GetIndex( const T& element ) const
	{
		const_iterator i = Find( Begin(), End(), element );
		if ( i != End() )
		{
			return i - Begin();
		}
		else
		{
			return -1;
		}
	}

	RED_INLINE ptrdiff_t GetIndex( const_iterator i ) const
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

	RED_INLINE const T* FindPtr( const T& element ) const
	{
		const_iterator it = Find( Begin(), End(), element );

		if( it != End() )
		{
			return it;
		}
		else
		{
			return NULL;
		}
	}

	RED_INLINE T* FindPtr( const T& element )
	{
		iterator it = Find( Begin(), End(), element );

		if( it != End() )
		{
			return it;
		}
		else
		{
			return NULL;
		}
	}

	RED_INLINE Bool Insert( const Uint32 position, const T& element )
	{
		if ( position <= m_size )
		{
			Uint32 oldSize = Uint32( m_size );
			Grow( 1 );

			TCopy::MoveForwardsInsert( TypedData() + position, oldSize - position, &element, 1 );

			return true;
		}
		else
		{
			return false;
		}
	}

	RED_INLINE Bool Insert( const Uint32 position, T&& element )
	{
		if ( position <= m_size )
		{
			Uint32 oldSize = Uint32( m_size );
			Grow( 1 );

			TCopy::MoveForwardsInsertMove( TypedData() + position, oldSize - position, &element, 1 );

			return true;
		}
		else
		{
			return false;
		}
	}

	RED_INLINE size_t Grow( size_t amount = 1 )
	{
		size_t oldSize = GrowBuffer(amount);

		TInit::Construct(TypedData() + oldSize, amount);

		return oldSize;
	}

	RED_INLINE TDynArray<T,memoryClass,memoryPool> SubArray( size_t start, size_t count = 0  ) const
	{
		size_t size = static_cast< size_t >( Size() );
		if( start > size)
		{
			start = size;
		}
		size_t end = ( count > 0 ) ? Min(start + count, size) : size;

		TDynArray<T,memoryClass, memoryPool> subArray;
		subArray.Reserve( end - start );
		for ( ; start != end; ++start )
		{
			subArray.PushBack( operator[]( static_cast< Int32 >( start ) ) );
		}
		return subArray;
	}

	RED_INLINE size_t SizeOfAllElements() const
	{
		return sizeof( T ) * Capacity();
	}

	RED_INLINE size_t GetInternalMemSize() const
	{
		size_t total( 0 );
		for ( size_t i=0; i<m_size; ++i )
			total += TypedData()[ i ].GetInternalMemSize();
		return total;
	}

	RED_INLINE size_t GetPointedMemSize() const
	{
		size_t total( 0 );
		for ( size_t i=0; i<m_size; ++i )
			total += TypedData()[ i ]->GetInternalMemSize();
		return total;
	}

	RED_INLINE void SwapWith( TDynArray<T,memoryClass,memoryPool>& arr2 )
	{
		CBaseArray::SwapWith( arr2 );
	}

protected:
	RED_INLINE size_t GrowBuffer(size_t amount)
	{
		return CBaseArray::GrowBuffer< T >( amount, memoryClass );
	}

	RED_INLINE void ResizeBuffer(Uint32 newElementCount)
	{
		CBaseArray::ResizeBuffer< T >( newElementCount, memoryClass );
	}	

	// Why protected ? To make sure implicit conversion do not happen.
	template< EMemoryClass otherMemClass, RED_CONTAINER_POOL_TYPE otherMemPool >
	RED_INLINE TDynArray(const TDynArray<T,otherMemClass,otherMemPool>& arr)
		: CBaseArray()
	{
		m_size = arr.Size();
		ResizeBuffer( arr.Size() );
		TInit::CopyConstruct( TypedData(), arr.TypedData(), m_size );

#ifdef USE_ARRAY_METRICS
		// Size / allocated size have changed
		SArrayMetrics::GetInstance().UpdateArrayElementCount( m_arrayID, m_size );
		SArrayMetrics::GetInstance().UpdateArrayMaximumCount( m_arrayID, Capacity() );
#endif
	}

	RED_FORCE_INLINE size_t GetAlignment() const
	{
		return __alignof( T );
	}

private:

	template< EMemoryClass otherMemClass, RED_CONTAINER_POOL_TYPE otherMemPool >
	RED_INLINE void InternalCopy( const TDynArray< T, otherMemClass, otherMemPool > & arr )
	{
		if( arr.Empty() )
		{
			ClearFast();
		}
		else
		{
			const Uint32 newSize = arr.Size();
			const Uint32 oldSize = m_size;

			// There are enough element, copy what is needed, destroy the rest.
			if( newSize <= oldSize ) 
			{
				TCopy::CopyAssign( TypedData(), arr.TypedData(), newSize );
				TInit::Destruct( TypedData() + newSize, oldSize - newSize );
			}
			else if( newSize <=  Capacity() ) // Enough space allocated. Copy over existing and construct the rest.
			{
				TCopy::CopyAssign( TypedData(), arr.TypedData(), oldSize );
				TCopy::CopyConstruct(  TypedData() + oldSize, arr.TypedData() + oldSize, newSize - oldSize );
			}
			else // Worst case, buffer has to be reallocated. Destroy everything, and reconstruct everything. 
			{
				TInit::Destruct( TypedData(), oldSize );
				ResizeBuffer( newSize );
				TCopy::CopyConstruct( TypedData(), arr.TypedData(), newSize );
			}

			m_size = newSize;
		}	

#ifdef USE_ARRAY_METRICS
		// Size / allocated size have changed
		SArrayMetrics::GetInstance().UpdateArrayElementCount( m_arrayID, m_size );
		SArrayMetrics::GetInstance().UpdateArrayMaximumCount( m_arrayID, Capacity() );
#endif
	}

public:
	// Serialization
	friend IFile& operator<<( IFile& file, TDynArray<T,memoryClass, memoryPool> &ar )
	{
		ar.Serialize( file );
		return file;
	}

	// Serialize array data in bulk
	void SerializeBulk( IFile& file )
	{
		if ( file.IsReader() )
		{
			// Read the number of elements in the array
			Uint32 size = 0;
			file << CCompressedNumSerializer( size );

			// Initialize array
			Resize( static_cast< size_t >( size ) );
		}
		else if ( file.IsWriter() )
		{
			// Write number of elements in the array
			file << CCompressedNumSerializer( m_size );
		}

		if ( m_size )
		{
			file.Serialize( m_buf, m_size * sizeof(T) );
		}

#ifdef USE_ARRAY_METRICS
		// Size changed
		SArrayMetrics::GetInstance().UpdateArrayElementCount( m_arrayID, m_size );
#endif
	}

	void SerializeBulkFast( IFile& file )
	{
		if ( file.IsReader() )
		{
			// Read the number of elements in the array
			Uint32 size = 0;
			file << CCompressedNumSerializer( size );

			// Initialize array
			ResizeFast( static_cast< size_t >( size ) );
		}
		else if ( file.IsWriter() )
		{
			// Write number of elements in the array
			file << CCompressedNumSerializer( m_size );
		}

		if ( m_size )
		{
			file.Serialize( m_buf, m_size * sizeof(T) );
		}

#ifdef USE_ARRAY_METRICS
		// Size changed
		SArrayMetrics::GetInstance().UpdateArrayElementCount( m_arrayID, m_size );
#endif
	}

	// Serialize array data
	void Serialize( IFile& file )
	{
		if ( file.IsReader() )
		{
			// Read the number of elements in the array
			Uint32 size = 0;

			file << CCompressedNumSerializer( size );

			// Initialize array
			Resize( (size_t)size );
		}
		else if ( file.IsWriter() )
		{
			// Write number of elements in the array
			file << CCompressedNumSerializer( m_size );
		}

		// Serialize elements
		Bool isPlainType = TPlainType<T>::Value;
#ifdef RED_ENDIAN_SWAP_SUPPORT_DEPRECATED
		if ( isPlainType && !file.IsByteSwapping() )
#else
		if ( isPlainType )
#endif // RED_ENDIAN_SWAP_SUPPORT_DEPRECATED
		{
			// Serialize whole buffer
			if ( m_size )
			{
				file.Serialize( m_buf, m_size * sizeof(T) );
			}
		}
		else
		{
			// Serialize each element
			for ( size_t i=0; i<m_size; i++ )
			{
				file << TypedData()[i];
			}
		}

#ifdef USE_ARRAY_METRICS
		// Size changed
		SArrayMetrics::GetInstance().UpdateArrayElementCount( m_arrayID, m_size );
#endif
	}

public:
	// Get the RTTI system type name for this array, inclused the _initial_ memory class and pool information
	RED_INLINE static const class CName& GetTypeName();

	// Get the raw, c++ name of the array type, usefull for debugging
	RED_INLINE static const AnsiChar* GetRawTypeName()
	{
#ifdef USE_ARRAY_METRICS
		return typeid(T).name();
#else
		return "unknown";
#endif
	}
};

//! Remove empty pointers from array
template< class T, enum EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void RemoveEmptyPointers( TDynArray< T*, memoryClass, memoryPool >& ar )
{
	// Remove all empty elements from array
	Int32 size = ( Int32 ) ar.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( !ar[i] )
		{
			ar.EraseFast( ar.Begin() + i  );
		}
	}

	// Shrink to reduce memory
	ar.Shrink();
}

//! Allocate new element and place it at the end of the array
template <class T, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void *operator new ( size_t, TDynArray<T,memoryClass, memoryPool> &ar )
{
	Uint32 index = static_cast< Uint32 >( ar.CBaseArray::template GrowBuffer< T >( 1, memoryClass ) );
	return &ar[ index ];
}

template <class T, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void operator delete( void *, TDynArray<T,memoryClass, memoryPool> & )
{ 
	RED_FATAL_ASSERT(0, "This should never happen, we don't use exceptions!"); 
} 

template< typename T, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_INLINE void Swap( TDynArray<T,memoryClass,memoryPool>& arr1, TDynArray<T,memoryClass,memoryPool>& arr2 )
{
	arr1.SwapWith( arr2 );
}

// Enable c++11 range-based for loop

template < typename T, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
typename TDynArray< T, memoryClass, memoryPool >::iterator begin( TDynArray< T, memoryClass, memoryPool >& arr ) { return arr.Begin(); }

template < typename T, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
typename TDynArray< T, memoryClass, memoryPool >::iterator end( TDynArray< T, memoryClass, memoryPool >& arr ) { return arr.End(); }

template < typename T, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
typename TDynArray< T, memoryClass, memoryPool >::const_iterator begin( const TDynArray< T, memoryClass, memoryPool >& arr ) { return arr.Begin(); }

template < typename T, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
typename TDynArray< T, memoryClass, memoryPool >::const_iterator end( const TDynArray< T, memoryClass, memoryPool >& arr ) { return arr.End(); }

#endif //CORE_DYNAMIC_ARRAY_H
