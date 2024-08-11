/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

//------

class CObjectsMap;

//#define VERIFY_POINTER_SET

#ifdef  VERIFY_POINTER_SET
	#include "hashmap.h"
#endif

//------

/// Helper class to hold a "visited" flag for a pointer
/// Done as a hierarchical bit buffer, assumes all the pointers are at least 8 bytes aligned
/// NOTE: only the lower 48 bits are considered here, any address outside the lower 48 bits will trigger fatal assertion
/// NOTE: please do not construct/destruct this class to often - keep a persistent instance instead and call Reset()
/// NOTE: this class is thread safe although it's interlock heavy - I'm not sure about performance on Durango
class CFastPointerSet
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_ObjectMap );

public:
	CFastPointerSet();
	~CFastPointerSet();

	// reset set to empty state
	void Reset();

	// test/mark address (to improve cache performance)
	// returns true if address was not yet visited, false if it was already visited.
	// This function IS THREAD SAFE!!
	// NOTE: address is assumed to be aligned to 8 bytes
	// NOTE: address is assumed to be in the lower 48 bits
	const Bool TestAndMark( const void* addr );

private:
	// address resolving structure:
	// 13 - 13 - 19 ( 2^16 byte entries, 2^19 bits ) - 3 (ignored)
	// total address space: 48 bits

	struct BucketHi;
	struct BucketLo;
	struct Page;

	typedef Red::Threads::AtomicOps::TAtomicPtr		AtomicPtr;
	typedef Red::Threads::CMutex					Mutex;

	static const Uint32 BIT_IGNORE = 3;

	static const Uint32 BIT_SHIFT = 5;
	static const Uint32 BIT_MASK = (1 << 5)-1;

	struct FreeMem
	{
		FreeMem*	m_next;
	};

	struct BucketHi
	{
		static const Uint32 NUM_ENTRIES = 1 << 13;
		static const Uint32 ADDR_SHIFT = 13 + 19 + BIT_IGNORE;

		BucketLo*	m_entries[ NUM_ENTRIES ];
	};

	struct BucketLo
	{
		static const Uint32 NUM_ENTRIES = 1 << 13;
		static const Uint32 ADDR_SHIFT = 19 + BIT_IGNORE;

		Page*		m_entries[ NUM_ENTRIES ];
	};

	struct Page
	{

		static const Uint32 NUM_ENTRIES = 1 << 19;
		static const Uint32 ADDR_SHIFT = BIT_IGNORE; // lower bits are not tested (alignment)

		Uint32	m_entries[ NUM_ENTRIES >> BIT_SHIFT ];

		// set address bit, returns true if the bit was set for the first time, false otherwise
		RED_INLINE const Bool TestAndMark( const Uint64 addr )
		{
			const Uint32 entry = (const Uint32)( ( addr >> ADDR_SHIFT ) % NUM_ENTRIES );

			const Uint32 index = entry >> BIT_SHIFT;
			const Uint32 mask = 1 << (entry & BIT_MASK);

			const Uint32 prevVal = Red::Threads::AtomicOps::Or32( (Red::Threads::AtomicOps::TAtomic32*) &m_entries[index], mask );
			return (prevVal & mask) == 0; // was not set - it was the first time we set it
		}
	};

	static_assert( sizeof(BucketHi) <= (64*1024), "Size of BucketHi must be less or equal than 64k" );
	static_assert( sizeof(BucketLo) <= (64*1024), "Size of BucketLo must be less or equal than 64k"  );
	static_assert( sizeof(Page) == (64*1024), "Size of Page must be equal to 64k" );

	typedef TDynArray< void*, MC_ObjectMap >	TPages;

	TPages		m_usedPages;
	TPages		m_bitPages;

	FreeMem*	m_freePages; // temp free pages
	BucketHi	m_bucketHi; // 48 bits

	Mutex		m_lock; // used only in memory allocations

#ifdef VERIFY_POINTER_SET
	typedef THashMap< void*, Uint8 >	TVisitedPtrs;

	Mutex			m_testLock; // used only in memory allocations
	TVisitedPtrs	m_testData;
#endif	

	void* AllocRawMemPage();
	void FreeRawMemPage( void* ptr );

	void* AllocBitPage();
	void* AllocUntypedMemPage();
	void FreeUntypedMemPage( void* ptr );

	void ZeroPage( void* ptr );
};

//------

/// Fast and thread safe bitmap (for storing flags for indexed objects)
/// Thread safe
class CFastObjectList
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_ObjectMap );

public:
	CFastObjectList();
	~CFastObjectList();

	// reset (to all ZERO) the list - also makes sure the internal size will be enough
	void ResetToZeros( const Uint32 maxEntryIndex );

	// reset (to all ZERO) the list - also makes sure the internal size will be enough
	void ResetToOnes( const Uint32 maxEntryIndex );

	// mark object (lock less, thread safe)
	RED_INLINE const void Set( const Uint32 entryIndex )
	{
		const Uint32 index = entryIndex >> BIT_SHIFT;
		const Uint32 mask = 1 << (entryIndex & BIT_MASK);

		Red::Threads::AtomicOps::Or32( (Red::Threads::AtomicOps::TAtomic32*) &m_entryBuffer[index], mask );
	}

	// Unmark object (lock less, thread safe)
	RED_INLINE const void Clear( const Uint32 entryIndex )
	{
		const Uint32 index = entryIndex >> BIT_SHIFT;
		const Uint32 mask = 1 << (entryIndex & BIT_MASK);

		Red::Threads::AtomicOps::And32( (Red::Threads::AtomicOps::TAtomic32*) &m_entryBuffer[index], ~mask );
	}

	// is the entry for given index set ? (not thread safe)
	RED_INLINE const Bool IsSet( const Uint32 entryIndex ) const
	{
		const Uint32 index = entryIndex >> BIT_SHIFT;
		const Uint32 mask = 1 << (entryIndex & BIT_MASK);
		return ( m_entryBuffer[ index ] & mask ) != 0;
	}

	// visit set indices 
	template< typename Visitor >
	RED_INLINE void VisitSet( const Uint32 maxIndex, const Visitor& visitor )
	{
		// group (32 in batch)
		const Uint32 maxEntry = maxIndex >> BIT_SHIFT;
		for ( Uint32 i=0; i<maxEntry; ++i )
		{
			const Uint32 entry = m_entryBuffer[i];
			if ( entry )
			{
				Uint32 mask = 1;
				Uint32 objectIndex = i << BIT_SHIFT;

				for ( Uint32 j=0; j<=BIT_MASK; ++j, mask <<= 1, ++objectIndex )
				{
					if ( entry & mask )
					{
						visitor( objectIndex );
					}
				}
			}
		}
		
		// the rest
		const Uint32 left = maxIndex & BIT_MASK;
		if ( left )
		{
			const Uint32 entry = m_entryBuffer[ maxEntry ];

			Uint32 mask = 1;
			Uint32 objectIndex = maxIndex & ~BIT_SHIFT;

			for ( Uint32 j=0; j<=left; ++j, mask <<= 1, ++objectIndex )
			{
				if ( entry & mask )
				{
					visitor( objectIndex );
				}
			}
		}
	}

private:
	static const Uint32 BIT_SHIFT = 5;
	static const Uint32 BIT_MASK = (1 << 5)-1;

	Uint32		m_numEntries;
	Uint32*		m_entryBuffer;
};

//------

/// Multi threaded list with multiple writers / multiple readers support
/// Assumption: the list itself is double buffered so at given time we are either reader or writing to it
template< typename T, typename T2, T2 ZERO >
class TFastMuiltiStreamList
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_ObjectMap );

public:
	TFastMuiltiStreamList()
		: m_readList( NULL )
		, m_writeList( NULL )
		, m_maxWrite( 0 )
		, m_writePtr( 0 )
		, m_maxRead( 0 )
		, m_readPtr( 0 )
	{

	}

	~TFastMuiltiStreamList()
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_ObjectMap, m_readList );
		RED_MEMORY_FREE( MemoryPool_Default, MC_ObjectMap, m_writeList );
	}

	// prepare list for given maximum count of objects
	void Reset( const Uint32 maxCount )
	{
		// reallocate if needed
		if ( maxCount > m_maxWrite )
		{
			m_readList = (T*)RED_MEMORY_REALLOCATE( MemoryPool_Default, m_readList, MC_ObjectMap, sizeof(T) * maxCount );
			m_writeList = (T*)RED_MEMORY_REALLOCATE( MemoryPool_Default, m_writeList, MC_ObjectMap, sizeof(T) * maxCount );
			m_maxWrite = maxCount;
		}

		// reset read/write pointers
		m_writePtr = 0;
		m_readPtr = 0;
	}

	// swap buffers
	void Swap()
	{
		::Swap( m_readList, m_writeList );

		m_maxRead = m_writePtr;
		m_writePtr = 0;
		m_readPtr = 0;
	}


	// write object (thread safe)
	RED_INLINE void Put( T ptr )
	{
		const Uint32 index = Red::Threads::AtomicOps::Increment32( &m_writePtr ) - 1;
		RED_FATAL_ASSERT( index < m_maxWrite, "CFastMuiltiStreamList fatal overflow !" );
		m_writeList[ index ] = ptr;
	}

	RED_INLINE Bool PutChecked( T ptr )
	{
		const Uint32 index = Red::Threads::AtomicOps::Increment32( &m_writePtr ) - 1;
		if( index >= m_maxWrite ) return false;
		m_writeList[ index ] = ptr;
		return true;
	}

	// read object (thread safe)
	RED_INLINE T Get()
	{
		const Uint32 index = Red::Threads::AtomicOps::Increment32( &m_readPtr ) - 1;
		return ( index < m_maxRead ) 
			? m_readList[ index ] 
			: T2( ZERO ); 
	}

	// returns true if the read list is empty
	RED_INLINE const Bool Empty() const
	{
		return (m_maxRead == 0);
	}

private:
	typedef Red::Threads::AtomicOps::TAtomic32		AtomicInt;

	T*				m_readList;
	T*				m_writeList;

	Uint32			m_maxWrite;
	AtomicInt		m_writePtr;

	Uint32			m_maxRead;
	AtomicInt		m_readPtr;
};

//------

/// Multi threaded reachability visitor
class CObjectReachability
{
public:
	CObjectReachability();

	// initialize initial reachability from root set list
	void InitializeFromRootSet( const Uint32 maxPointerCount );

	// initialize from custom object list
	void InitializeFromObjects( CObject** objects, const Uint32 numObjects );

	// collect unreachable objects
	void CollectUnreachables( CFastObjectList* unreachables, const Bool multiThreaded );

private:
	static void ProcessObjects( TFastMuiltiStreamList< void*, void*, nullptr >& inList, CFastObjectList* unreachables, CFastPointerSet& visitedSet );
	static void ProcessClassDepdendnecies( const CClass* ptr, void* object, CFastPointerSet& visitedSet, TFastMuiltiStreamList< void*, void*, nullptr >& outList );

	TFastMuiltiStreamList< void*, void*, nullptr >	m_pointerList;		// list of pointers to process
	CFastPointerSet									m_visitedSet;		// visited pointers, managed externally
	CFastObjectList*								m_unreachables;		// marks reachable objects (only unreachable objects will be left)
};

//------
