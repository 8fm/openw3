/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#if 0

template< class tFirst, class tSecond > class TFragPool;

/// Double linked fragment
template< class tFirst, class tSecond >
class TFragElem
{
	friend class TFragPool< tFirst, tSecond >;

protected:
	tFirst			m_objectA;		//!< First linked object
	tSecond			m_objectB;		//!< Second linked object
	TFragElem*		m_nextA;		//!< Next object A fragment
	TFragElem**		m_prevA;		//!< Previous object A fragment
	TFragElem*		m_nextB;		//!< Next object B fragment
	TFragElem**		m_prevB;		//!< Previous object B fragment
	void*			m_owner;		//!< Owner (pool block)
	Uint32			m_pad;			//!< Padding to fit inside 32 bytes

private:
	//! Hidden destructor and constructor as they are not called directly
	RED_INLINE TFragElem() {};
	RED_INLINE ~TFragElem() {};

public:
	//! Link to object A list
	RED_INLINE void LinkA( tFirst objectA, TFragElem *&fragList )
	{
		m_objectA = objectA;
		if ( fragList )
		{
			fragList->m_prevA = &m_nextA;
		}
		m_nextA = fragList;
		m_prevA = &fragList;
		fragList = this;
	}

	//! Link to object B list
	RED_INLINE void LinkB( tSecond objectB, TFragElem *&fragList )
	{
		m_objectB = objectB;
		if ( fragList )
		{
			fragList->m_prevB = &m_nextB;
		}
		m_nextB = fragList;
		m_prevB = &fragList;
		fragList = this;
	}

	//! Unlink from actor list
	RED_INLINE void UnlinkA()
	{
		if ( m_nextA )
		{
			m_nextA->m_prevA = m_prevA;
		}
		if ( m_prevA )
		{
			*m_prevA = m_nextA; 
		}
		m_nextA = NULL;
		m_prevA = NULL;
		m_objectA = NULL; 		
	}

	//! Unlink from zone list
	RED_INLINE void UnlinkB()
	{
		if ( m_nextB )
		{
			m_nextB->m_prevB = m_prevB;
		}
		if ( m_prevB )
		{
			*m_prevB = m_nextB; 
		}
		m_nextB = NULL;
		m_prevB = NULL;
		m_objectB = NULL; 
	}

	//! Free this fragment back to pool
	RED_INLINE void Release();

	//! Free fragments of object A in object B
	RED_INLINE void FreeListA()
	{
		TFragElem *cur, *next;
		for ( cur=this; cur; cur=next )
		{
			next = cur->m_nextA;
			cur->UnlinkA();
			cur->UnlinkB();
			cur->Release();
		}
	}

	//! Free fragments of object B in object A
	RED_INLINE void FreeListB()
	{
		TFragElem *cur, *next;
		for ( cur=this; cur; cur=next )
		{
			next = cur->m_nextB;
			cur->UnlinkA();
			cur->UnlinkB();
			cur->Release();
		}
	}

public:
	//! Access object A
	RED_INLINE tFirst GetObjectA() const { return m_objectA; }

	//! Access object B
	RED_INLINE tSecond GetObjectB() const { return m_objectB; }

	//! Access next fragment in chain for object A
	RED_INLINE TFragElem *GetNextFragA() const { return m_nextA; }

	//! Access next fragment in chain for object A
	RED_INLINE TFragElem *GetNextFragB() const { return m_nextB; }
};

/// General fragment pool
template< typename tFirst, typename tSecond >
class TFragPool
{
	typedef TFragElem< tFirst, tSecond > tFrag;
	friend class TFragElem< tFirst, tSecond >;

private:
	/// Pool block
	struct PoolBlock
	{
		void*		m_memory;		//!< Memory block
		void*		m_memoryEnd;	//!< End of memory block
		tFrag*		m_freeFrags;	//!< Free fragments in this block
		Uint32		m_freeCount;	//!< Number of free fragments in block
		Uint32		m_maxCount;		//!< Pool capacity
		TFragPool*	m_owner;

		//! Create pool block
		RED_INLINE PoolBlock( Uint32 maxCapacity, TFragPool* owner )
			: m_freeCount( maxCapacity )
			, m_maxCount( maxCapacity )
			, m_owner( owner )
		{
			// Allocate memory
			const Uint32 memorySize = sizeof( tFrag ) * maxCapacity;
			m_memory = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Fragments, memorySize );
			m_memoryEnd = OffsetPtr( m_memory, memorySize );

			// Initialize memory to zeros
			Red::System::MemorySet( m_memory, 0, memorySize );

			// Initialize fragment chain, move back to front so the created list is in natural order
			for ( Int32 i = (INT)( m_maxCount-1 ); i>=0; --i )
			{	
				tFrag *frag = (tFrag*)m_memory + i;
				frag->m_nextA = m_freeFrags;
				m_freeFrags = frag;
			}
		}

		//! Destructor
		RED_INLINE ~PoolBlock()
		{
			// Free pool memory
			if ( m_memory )
			{
				RED_MEMORY_FREE( MemoryPool_Default, MC_Fragments, m_memory );
				m_memory = NULL;
			}
		}

		//! Allocate fragment from this pool block
		RED_INLINE tFrag* AllocateFrag()
		{
			// Make sure we are not allocating from empty pool
			RED_FATAL_ASSERT( m_freeCount > 0, "" );

			// Allocate fragment
			tFrag *frag = m_freeFrags;
			frag->m_owner = this;
			m_freeFrags = m_freeFrags->m_nextA;
			m_freeCount--;
			return frag;
		}

		//! Free fragment, fragment should be from this pool block
		RED_INLINE void FreeFrag( tFrag* frag )
		{
			// Make sure we are freeing valid fragment
			RED_FATAL_ASSERT( frag, "" );
			RED_FATAL_ASSERT( m_freeCount < m_maxCount, "" );
			RED_FATAL_ASSERT( (Uint8*) frag >= ( Uint8* ) m_memory, "" );
			RED_FATAL_ASSERT( (Uint8*) frag < ( Uint8* ) m_memoryEnd, "" );

			// Free fragment
			frag->m_nextA = m_freeFrags;
			m_freeFrags = frag;
			m_freeCount++;
		}
	};

private:
	TDynArray< PoolBlock* >		m_blocks;		//!< Active pool blocks
	TDynArray< PoolBlock* >		m_fullBlocks;	//!< Block that are totaly used up
	Uint32						m_blockSize;	//!< Single pool block capacity

public:
	TFragPool( Uint32	singleBlockCapacity )
		: m_blockSize( singleBlockCapacity )
	{
		// Create block
		m_blocks.Reserve( 64 );
		m_fullBlocks.Reserve( 4096 );

		m_blocks.PushBack( new PoolBlock( singleBlockCapacity, this ) );
	}

	~TFragPool()
	{
		// Free blocks
		m_blocks.ClearPtr();
		m_fullBlocks.ClearPtr();
	}

	//! Allocate scene fragment
	tFrag* AllocateFrag()
	{
		// Allocate from nonempty pool
		for ( Uint32 i=0; i<m_blocks.Size(); i++ )
		{
			PoolBlock *block = m_blocks[i];
			if ( block->m_freeCount > 0 )
			{
				// Allocate fragment
				tFrag* frag = block->AllocateFrag();

				// If this was the last block allocated move the pool to the list of used up pools
				if ( block->m_freeCount == 0 )
				{
					m_blocks.RemoveFast( block );
					m_fullBlocks.PushBack( block );
				}

				// Return allocated fragment
				return frag;
			}
		}

		// No free blocks, create new pool
		PoolBlock *block = new PoolBlock( m_blockSize, this );
		m_blocks.PushBack( block );

		// Allocate fragment from created pool block
		return block->AllocateFrag();
	}
	RED_INLINE void FreeFrag( tFrag* frag )
	{
		TFragPool< tFirst, tSecond >::PoolBlock* pool = static_cast< TFragPool< tFirst, tSecond >::PoolBlock* >( frag->m_owner );
		pool->FreeFrag( frag );

		if( pool->m_freeCount == 1 )
		{
			// ok, so full block is now partially usable again
			m_blocks.PushBack( pool );
			m_fullBlocks.RemoveFast( pool );
		}

		// Mark frag as freed
		frag->m_owner = NULL;

		if( pool->m_freeCount == m_blockSize )
		{
			// whole pool is not used
			m_blocks.RemoveFast( pool );
			delete pool;
		}
	}
};

//! Free this fragment back to pool
template< class tFirst, class tSecond >
RED_INLINE void TFragElem< tFirst, tSecond >::Release()
{
	RED_FATAL_ASSERT( m_owner, "" );
	if ( m_owner )
	{
		// Free fragment back to its pool
		TFragPool< tFirst, tSecond >::PoolBlock* pool = static_cast< TFragPool< tFirst, tSecond >::PoolBlock* >( m_owner );
		pool->m_owner->FreeFrag( this );
	}
}
#endif