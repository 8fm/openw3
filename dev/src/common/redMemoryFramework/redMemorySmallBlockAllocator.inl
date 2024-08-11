/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
//
#include "redMemorySmallBlockAllocatorImpl.inl"
#include "../redSystem/bitUtils.h"

namespace Red { namespace MemoryFramework {

	/////////////////////////////////////////////////////////////////
	// CTor
	//
	template< typename TSyncLock >
	SmallBlockAllocator< TSyncLock >::SmallBlockAllocator()
		: IAllocator()
		, m_chunkAddressMask( 0 )
		, m_bucketArray( nullptr )
		, m_bucketCount( 0 )
		, m_permanentMemoryBlock( nullptr )
	{
	}

	/////////////////////////////////////////////////////////////////
	// DTor
	//
	template< typename TSyncLock >
	SmallBlockAllocator< TSyncLock >::~SmallBlockAllocator()
	{
	}

	/////////////////////////////////////////////////////////////////
	// Allocate
	//	Note! The alignment is actually ignored; don't use this for objects with strict alignment requirements!
	template< typename TSyncLock >
	void*	SmallBlockAllocator< TSyncLock >::Allocate( Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::Uint16 memoryClass )
	{
		RED_MEMORY_ASSERT( allocSize <= SmallBlockAllocatorImpl::k_MaximumAllocationAllowed, "This allocator only supports blocks of up to '%ld' bytes", SmallBlockAllocatorImpl::k_MaximumAllocationAllowed );
		RED_MEMORY_ASSERT( allocAlignment <= SmallBlockAllocatorImpl::k_MaximumAllocationAllowed, "This allocator only supports alignment up to '%ld' bytes", SmallBlockAllocatorImpl::k_MaximumAllocationAllowed );
		RED_MEMORY_ASSERT( ( allocAlignment & ( allocAlignment - 1 ) ) == 0, "Alignment must be power of two" );

		// Size must be rounded up to alignment in order to select the correct bucket
		allocSize = ( allocSize + allocAlignment - 1 ) & ~( allocAlignment - 1 );

		Red::System::Int32 bucketIndex = m_bucketSizeLookup[ allocSize ];
		RED_MEMORY_ASSERT( bucketIndex != -1, "A bucket was not defined for size '%ld'", allocSize );
		SmallBlockAllocatorImpl::Bucket< TSyncLock >& sourceBucket = m_bucketArray[ bucketIndex ];

		void* memory = nullptr;
		{
			// Fast path
			typename TSyncLock::TScopedLock lock( &sourceBucket.GetLock() );
			memory = sourceBucket.GetFreeEntry();
		}

		if( !memory )
		{
			SmallBlockAllocatorImpl::Chunk* newChunksHead = nullptr, *newChunksTail = nullptr;
			GetEmptyChunks( newChunksHead, newChunksTail, 1 );		// This locks the chunks pool

			if( newChunksHead != nullptr )
			{
				typename TSyncLock::TScopedLock lock( &sourceBucket.GetLock() );
				for( auto chnk = newChunksHead; chnk != nullptr; chnk = chnk->GetNext() )
				{
					sourceBucket.AddNewChunk( chnk, m_creationParameters.m_chunkSizeBytes );
				}

				memory = sourceBucket.GetFreeEntry();	// Final attempt to allocate
			}
		}

		allocatedSize = sourceBucket.GetEntrySize();

		return memory;
	}

	/////////////////////////////////////////////////////////////////
	// Reallocate
	//	Not many optimisations can be done here, try to avoid reallocating from this pool
	template< typename TSyncLock >
	void*	SmallBlockAllocator< TSyncLock >::Reallocate( void* ptr, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::MemSize& freedSize, Red::System::Uint16 memoryClass )
	{
		// First of all, test if this allocator actually owns the pointer
		if( ptr != nullptr && !OwnsPointer( ptr ) )
		{
			allocatedSize = 0;
			freedSize = 0;
			return nullptr;
		}

		RED_MEMORY_ASSERT( allocSize <= SmallBlockAllocatorImpl::k_MaximumAllocationAllowed, "This allocator only supports blocks of up to '%ld' bytes", SmallBlockAllocatorImpl::k_MaximumAllocationAllowed );
		if( allocSize == 0 && ptr != nullptr )
		{
			allocatedSize = 0;
			freedSize = GetAllocationSize( ptr );
			EAllocatorFreeResults freeOk = Free( ptr );
			RED_UNUSED( freeOk );
			RED_MEMORY_ASSERT( freeOk == Free_OK, "Failed to free ptr passed to reallocate" );

			return nullptr;
		}
		else if( allocSize > 0 )
		{
			Red::System::MemSize sourceSize = ptr != nullptr ? GetAllocationSize( ptr ) : 0;
			void* newPtr = Allocate( allocSize, allocAlignment, allocatedSize, memoryClass );

			if( ptr != nullptr && newPtr != nullptr )
			{
				Red::System::MemoryCopy( newPtr, ptr, Red::Math::NumericalUtils::Min( sourceSize, allocSize ) );
			}

			if( ptr )
			{
				freedSize = GetAllocationSize( ptr );
				EAllocatorFreeResults freeOk = Free( ptr );
				RED_UNUSED( freeOk );
				RED_MEMORY_ASSERT( freeOk == Free_OK, "Failed to free ptr passed to reallocate" );
			}

			return newPtr;
		}

		return nullptr;
	}

	/////////////////////////////////////////////////////////////////
	// Free
	//
	template< typename TSyncLock >
	EAllocatorFreeResults SmallBlockAllocator< TSyncLock >::Free( const void* ptr )
	{
		// Chop off the first bits to get the chunk header for this pointer
		Red::System::MemUint chunkHeadAddress = reinterpret_cast< Red::System::MemUint >( ptr ) & m_chunkAddressMask;
		SmallBlockAllocatorImpl::Chunk* theChunk = reinterpret_cast< SmallBlockAllocatorImpl::Chunk* >( chunkHeadAddress );
		if( !theChunk->IsValid() )
		{
			return Free_NotOwned;
		}

		auto* parentBucket = static_cast< SmallBlockAllocatorImpl::Bucket< TSyncLock >* >( theChunk->GetParentBucket() );	
		SmallBlockAllocatorImpl::Chunk* releasedChunkHead = nullptr;
		SmallBlockAllocatorImpl::Chunk* releasedChunkTail = nullptr;

		// If there are now empty chunks, release them from the bucket
		{
			typename TSyncLock::TScopedLock lock( &parentBucket->GetLock() );
			parentBucket->ReleaseEntryFromChunk( ptr, theChunk );
			if( parentBucket->EmptyChunkCount() > 0 )
			{
				parentBucket->ReleaseEmptyChunks( releasedChunkHead, releasedChunkTail );
			}
		}

		// By default, we return all release chunks to the internal pool, and only free them properly if we hit an OOM situation
		if( releasedChunkHead != nullptr )
		{
			typename TSyncLock::TScopedLock lock( &m_chunkPoolLock );
			while( releasedChunkHead != nullptr )
			{
				SmallBlockAllocatorImpl::Chunk* nextChunk = releasedChunkHead->GetNext();
				releasedChunkHead->Cleanup();

				Utils::PushFront( m_chunkPoolHead, m_chunkPoolTail, releasedChunkHead );
				releasedChunkHead = nextChunk;
			}
		}

		return Free_OK;
	}

	/////////////////////////////////////////////////////////////////
	// GetAllocationSize
	//
	template< typename TSyncLock >
	Red::System::MemSize SmallBlockAllocator< TSyncLock >::GetAllocationSize( const void* ptr ) const
	{
		// Chop off the first bits to get the chunk header for this pointer
		Red::System::MemUint chunkHeadAddress = reinterpret_cast< Red::System::MemUint >( ptr ) & m_chunkAddressMask;
		SmallBlockAllocatorImpl::Chunk* theChunk = reinterpret_cast< SmallBlockAllocatorImpl::Chunk* >( chunkHeadAddress );
		if( theChunk->IsValid() )
		{
			return theChunk->GetEntrySize();
		}
		else
		{
			return 0;	// block not owned by this allocator (or memory is corrupted)
		}
	}

	/////////////////////////////////////////////////////////////////
	// OwnsPointer
	//
	template< typename TSyncLock >
	Red::System::Bool SmallBlockAllocator< TSyncLock >::OwnsPointer( const void* ptr ) const
	{
		if( !ptr )
		{
			return false;
		}

		// Chop off the first bits to get the chunk header for this pointer
		Red::System::MemUint chunkHeadAddress = reinterpret_cast< Red::System::MemUint >( ptr ) & m_chunkAddressMask;
		SmallBlockAllocatorImpl::Chunk* theChunk = reinterpret_cast< SmallBlockAllocatorImpl::Chunk* >( chunkHeadAddress );
		
		// test permanent area first as its most common case and faster
		Red::System::MemUint permanentAreaHead = reinterpret_cast< Red::System::MemUint >( m_permanentMemoryBlock );
		Red::System::MemSize permanentAreaSize = m_creationParameters.m_permanentChunksToPreallocate * m_creationParameters.m_chunkSizeBytes;
		if( ( chunkHeadAddress >= permanentAreaHead ) && ( chunkHeadAddress < ( permanentAreaHead + permanentAreaSize ) ) )
		{
			if( !theChunk->IsPermanent() )	// Just make sure the chunk is set up properly
			{
				return false;
			}
		}
		else if( !m_creationParameters.m_ownershipFn( theChunk ) )
		{
			// Its not from permanent block, and parent allocator doesn't own it either
			return false;
		}

		if( !theChunk->IsValid() )
		{
			return false;
		}

		// Test that the chunk's parent bucket is in our list
		Red::System::MemUint chunkBucketAddr = reinterpret_cast< Red::System::MemUint >( theChunk->GetParentBucket() );
		Red::System::MemUint bucketArrayAddr = reinterpret_cast< Red::System::MemUint >( m_bucketArray );

		return ( chunkBucketAddr >= bucketArrayAddr && chunkBucketAddr < ( bucketArrayAddr + ( m_bucketCount * sizeof( *m_bucketArray ) ) ) );
	}

	/////////////////////////////////////////////////////////////////
	// Initialise
	//	Flags are pretty much ignored as they are inherited from the 'parent' allocator
	template< typename TSyncLock >
	EAllocatorInitResults SmallBlockAllocator< TSyncLock >::Initialise( const IAllocatorCreationParameters& parameters, Red::System::Uint32 flags )
	{
		const CreationParameters& params = static_cast< const CreationParameters& >( parameters );
		RED_MEMORY_ASSERT( params.IsValid(), "Invalid parameters" );
		if( !params.IsValid() )
		{
			return AllocInit_BadParameters;
		}
		m_creationParameters = params;
		m_bucketCount = params.m_bucketDefinitionCount;
		m_chunkAddressMask = ~( params.m_chunkSizeBytes - 1 );

		if( !InitialiseBookeepingMemory( params ) )
		{
			RED_MEMORY_ASSERT( false, "Failed to allocate memory for bookkeeping data" );
			return AllocInit_OutOfMemory;
		}

		InitialiseBucketSizeLookup( params );

		// Allocate initial chunks for all buckets
		Uint32 chunksToAllocateAtOnce = params.m_permanentChunksToPreallocate;
		if( chunksToAllocateAtOnce > 0 )
		{
			void* permanentChunkMemory = m_creationParameters.m_allocFn( m_creationParameters.m_chunkSizeBytes * chunksToAllocateAtOnce, m_creationParameters.m_chunkSizeBytes );
			RED_FATAL_ASSERT( permanentChunkMemory, "Failed to allocate initial chunks" );
			if( permanentChunkMemory != nullptr )
			{
				for( Uint32 i=0; i < chunksToAllocateAtOnce; ++i )
				{
					void* thisChunkMemory = reinterpret_cast< void* >( reinterpret_cast< MemUint >( permanentChunkMemory ) + ( m_creationParameters.m_chunkSizeBytes * i ) );
					SmallBlockAllocatorImpl::Chunk* newChunk = new( thisChunkMemory ) SmallBlockAllocatorImpl::Chunk( true );	// true = permanent chunks
					Utils::PushFront( m_chunkPoolHead, m_chunkPoolTail, newChunk );
				}
			}
			m_permanentMemoryBlock = permanentChunkMemory;
		}

		return AllocInit_OK;
	}

	/////////////////////////////////////////////////////////////////
	// AllocateNewChunk
	//	Allocate memory for a single chunk, set up header + add to pool
	template< typename TSyncLock >
	SmallBlockAllocatorImpl::Chunk* SmallBlockAllocator< TSyncLock >::AllocateNewChunk()
	{
		void* chunkMemory = m_creationParameters.m_allocFn( m_creationParameters.m_chunkSizeBytes, m_creationParameters.m_chunkSizeBytes );
		RED_MEMORY_ASSERT( chunkMemory, "Failed to allocate chunk memory" );
		if( !chunkMemory )
		{
			return nullptr;
		}

		SmallBlockAllocatorImpl::Chunk* newChunk = new( chunkMemory ) SmallBlockAllocatorImpl::Chunk( false );	// false = chunk memory can be freed
		return newChunk;
	}

	/////////////////////////////////////////////////////////////////
	// GetEmptyChunks
	//	Note, this allocates more chunk memory if required
	template< typename TSyncLock >
	void SmallBlockAllocator< TSyncLock >::GetEmptyChunks( SmallBlockAllocatorImpl::Chunk*& listHead, SmallBlockAllocatorImpl::Chunk*& listTail, Uint32 chunkCount )
	{
		// 2 passes to avoid locks
		SmallBlockAllocatorImpl::Chunk* newChunksHead = nullptr;
		SmallBlockAllocatorImpl::Chunk* newChunksTail = nullptr;
		{
			typename TSyncLock::TScopedLock lock( &m_chunkPoolLock );
			for( Uint32 i=0; i < chunkCount; ++i )
			{
				// Get chunks from the pool if possible. Pop from back to try and re-use recently touched chunks
				SmallBlockAllocatorImpl::Chunk* newChunk = Utils::PopFront( m_chunkPoolHead, m_chunkPoolTail );
				if( newChunk == nullptr )
				{
					newChunk = AllocateNewChunk();	// No free chunks, make some new ones
				}

				if( newChunk != nullptr )
				{
					Utils::PushBack( newChunksHead, newChunksTail, newChunk );
				}
			}
		}
		listHead = newChunksHead;
		listTail = newChunksTail;
	}

	/////////////////////////////////////////////////////////////////
	// InitialiseBookeepingMemory
	//	Allocate memory for bucket structures, and any internal data from the source allocator
	//  Assume params are valid at this point
	template< typename TSyncLock >
	Bool SmallBlockAllocator< TSyncLock >::InitialiseBookeepingMemory( const CreationParameters& parameters )
	{
		// Allocate heap for bucket header array 
		Red::System::MemSize bucketArraySize = sizeof( SmallBlockAllocatorImpl::Bucket< TSyncLock > ) * parameters.m_bucketDefinitionCount;
		m_bucketArray = static_cast< SmallBlockAllocatorImpl::Bucket< TSyncLock >* >( parameters.m_allocFn( bucketArraySize, 16u ) );
		RED_MEMORY_ASSERT( m_bucketArray != nullptr, "Failed to allocate heap for bucket array" );
		if( !m_bucketArray )
		{
			return false;
		}

		// Initialise each bucket header
		for( Uint32 bucket = 0; bucket < parameters.m_bucketDefinitionCount; ++bucket )
		{
			new( &m_bucketArray[ bucket ] ) SmallBlockAllocatorImpl::Bucket< TSyncLock >();
			m_bucketArray[ bucket ].SetEntrySize( parameters.m_bucketDefinitions[ bucket ].m_maximumAllocationSize );
		}

		return true;
	}

	/////////////////////////////////////////////////////////////////
	// InitialiseBucketSizeLookup
	//	Build map of size -> bucket index
	template< typename TSyncLock >
	void SmallBlockAllocator< TSyncLock >::InitialiseBucketSizeLookup( const CreationParameters& parameters )
	{
		Red::System::MemorySet( m_bucketSizeLookup, (Int8)-1, sizeof( m_bucketSizeLookup ) );

		for( Uint32 bucket = 0; bucket < parameters.m_bucketDefinitionCount; ++bucket )
		{
			for( Uint32 size = parameters.m_bucketDefinitions[ bucket ].m_minimumAllocationSize; 
				 size <= parameters.m_bucketDefinitions[ bucket ].m_maximumAllocationSize;
				 ++size )
			{
				m_bucketSizeLookup[ size ] = bucket;
			}
		}
	}

	/////////////////////////////////////////////////////////////////
	// Release
	//	Free up any memory still used
	template< typename TSyncLock >
	void SmallBlockAllocator< TSyncLock >::Release( )
	{
		// Destroy the buckets + release all chunk memory
		for( Uint32 bucket = 0; bucket < m_bucketCount; ++bucket )
		{
			typename TSyncLock::TScopedLock lock( &m_bucketArray[ bucket ].GetLock() );
			m_bucketArray[ bucket ].ReleaseAllChunks( m_creationParameters.m_freeFn );
			m_bucketArray[ bucket ].~Bucket();
		}
		m_creationParameters.m_freeFn( m_bucketArray );

		// Destroy all empty chunks in the pool
		{
			typename TSyncLock::TScopedLock lock( &m_chunkPoolLock );
			SmallBlockAllocatorImpl::Chunk* freeChunk = m_chunkPoolHead;
			while( freeChunk )
			{
				SmallBlockAllocatorImpl::Chunk* nextFreeChunk = freeChunk->GetNext();
				Bool canFreeThisChunk = !freeChunk->IsPermanent();
				freeChunk->~Chunk();
				if( canFreeThisChunk )
				{
					m_creationParameters.m_freeFn( freeChunk );
				}
				freeChunk = nextFreeChunk;
			}
			m_chunkPoolHead = m_chunkPoolTail = nullptr;
			if( m_permanentMemoryBlock != nullptr )
			{
				m_creationParameters.m_freeFn( m_permanentMemoryBlock );
			}
		}
	}

	/////////////////////////////////////////////////////////////////
	// IncreaseMemoryFootprint
	//	All we can do is attempt to grow the correct bucket. WE don't call into the area callback as it is generally reserved for system-level allocations
	template< typename TSyncLock >
	Red::System::Bool SmallBlockAllocator< TSyncLock >::IncreaseMemoryFootprint( AllocatorAreaCallback& areaCallback, Red::System::MemSize sizeRequired )
	{
		RED_MEMORY_ASSERT( sizeRequired <= SmallBlockAllocatorImpl::k_MaximumAllocationAllowed, "This allocator only supports blocks of up to '%ld' bytes", SmallBlockAllocatorImpl::k_MaximumAllocationAllowed );

		Red::System::Int32 bucketIndex = static_cast< Red::System::Int32 >( m_bucketSizeLookup[ sizeRequired ] );
		RED_MEMORY_ASSERT( bucketIndex != -1, "A bucket was not defined for size '%ld'", sizeRequired );

		SmallBlockAllocatorImpl::Chunk* newChunksHead = nullptr, *newChunksTail = nullptr;
		GetEmptyChunks( newChunksHead, newChunksTail, 1 );		// This locks the chunks pool
		{
			typename TSyncLock::TScopedLock lock( &m_bucketArray[ bucketIndex ].GetLock() );
			for( auto chnk = newChunksHead; chnk != nullptr; chnk = chnk->GetNext() )
			{
				m_bucketArray[ bucketIndex ].AddNewChunk( chnk, m_creationParameters.m_chunkSizeBytes );
			}
		}

		return newChunksHead != nullptr;
	}

	/////////////////////////////////////////////////////////////////
	// ReleaseFreeMemoryToSystem
	//	We can try to free any chunks that are completely free
	template< typename TSyncLock >
	Red::System::MemSize SmallBlockAllocator< TSyncLock >::ReleaseFreeMemoryToSystem( AllocatorAreaCallback& areaCallback )
	{
		typename TSyncLock::TScopedLock lock( &m_chunkPoolLock );

		Uint32 chunksReleased = 0;
		SmallBlockAllocatorImpl::Chunk* freeChunk = m_chunkPoolHead;
		while( freeChunk )
		{
			SmallBlockAllocatorImpl::Chunk* nextFreeChunk = freeChunk->GetNext();
			if( !freeChunk->IsPermanent() )
			{
				freeChunk->~Chunk();
				m_creationParameters.m_freeFn( freeChunk );
				++chunksReleased;
			}
			freeChunk = nextFreeChunk;
		}
		m_chunkPoolHead = m_chunkPoolTail = nullptr;

		return chunksReleased * m_creationParameters.m_chunkSizeBytes;
	}

	/////////////////////////////////////////////////////////////////
	// OnOutOfMemory
	//	Output proper debug info
	template< typename TSyncLock >
	void SmallBlockAllocator< TSyncLock >::OnOutOfMemory()
	{
		DumpDebugOutput();
	}

	/////////////////////////////////////////////////////////////////
	// RequestAllocatorInfo
	//
	template< typename TSyncLock >
	void SmallBlockAllocator< TSyncLock >::RequestAllocatorInfo( AllocatorInfo& info )
	{
		info.SetAllocatorTypeName( TXT( "SmallBlockAllocator" ) );
		info.SetAllocatorBudget( 0 );
		info.SetPerAllocationOverhead( 0 );
	}

	/////////////////////////////////////////////////////////////////
	// WalkAllocator
	//	Returns each area that has been allocated from the chunk allocator. They are NOT in address order
	template< typename TSyncLock >
	void SmallBlockAllocator< TSyncLock >::WalkAllocator( AllocatorWalker* theWalker )
	{
		for( Red::System::Uint32 bucket = 0; bucket < m_bucketCount; ++bucket )
		{
			typename TSyncLock::TScopedLock lock( &m_bucketArray[ bucket ].GetLock() );

			SmallBlockAllocatorImpl::Chunk* chunkPtr = m_bucketArray[ bucket ].GetFreeChunksHead();
			while( chunkPtr )
			{
				Red::System::MemUint chunkAddress = reinterpret_cast< Red::System::MemUint >( chunkPtr );
				theWalker->OnMemoryArea( chunkAddress, m_creationParameters.m_chunkSizeBytes );
				chunkPtr = chunkPtr->GetNext();
			}

			chunkPtr = m_bucketArray[ bucket ].GetFullChunksHead();
			while( chunkPtr )
			{
				Red::System::MemUint chunkAddress = reinterpret_cast< Red::System::MemUint >( chunkPtr );
				theWalker->OnMemoryArea( chunkAddress, m_creationParameters.m_chunkSizeBytes );
				chunkPtr = chunkPtr->GetNext();
			}
		}

		{
			typename TSyncLock::TScopedLock lock( &m_chunkPoolLock );
			SmallBlockAllocatorImpl::Chunk* freeChunks = m_chunkPoolHead;
			while( freeChunks )
			{
				theWalker->OnMemoryArea( reinterpret_cast< Red::System::MemUint >( freeChunks ), m_creationParameters.m_chunkSizeBytes );
				freeChunks = freeChunks->GetNext();
			}
		}
	}

	/////////////////////////////////////////////////////////////////
	// WalkPoolArea
	//	Build a bitmap for the chunk passed in. This is required in order to walk the addresses in order
	//	without having to walk the free list for each entry - o(2n) rather than o(n^2)
	template< typename TSyncLock >
	void SmallBlockAllocator< TSyncLock >::WalkPoolArea( Red::System::MemUint startAddress, Red::System::MemSize size, PoolAreaWalker* theWalker )
	{
		// Chop off the first bits to get the chunk header for this pointer
		Red::System::MemUint chunkHeadAddress = startAddress & m_chunkAddressMask;
		SmallBlockAllocatorImpl::Chunk* theChunk = reinterpret_cast< SmallBlockAllocatorImpl::Chunk* >( chunkHeadAddress );
		if( !theChunk->IsValid() )
		{
			typename TSyncLock::TScopedLock lock( &m_chunkPoolLock );

			// If the chunk is in the free pool, we are done
			SmallBlockAllocatorImpl::Chunk* freeChunks = m_chunkPoolHead;
			while( freeChunks )
			{
				if( freeChunks == theChunk )
				{
					theWalker->OnFreeArea( reinterpret_cast< Red::System::MemUint >( freeChunks ), m_creationParameters.m_chunkSizeBytes );
					return;
				}
				freeChunks = freeChunks->GetNext();
			}
		}

		// Bitmap that can hold the largest block
		Red::System::MemSize debugBitmapSizeBytes = m_creationParameters.m_maximumEntriesPerChunk / 8;
		Red::System::Uint8* debugBitmap = (Red::System::Uint8*)m_creationParameters.m_allocFn( debugBitmapSizeBytes, 16 );
		if( !debugBitmap )
		{
			return;
		}

		// Build the bitmap by walking the chunk free list
		Red::System::MemorySet( debugBitmap, 0, debugBitmapSizeBytes );
		{
			auto parentBucket = static_cast< SmallBlockAllocatorImpl::Bucket< TSyncLock >* >( theChunk->GetParentBucket() );
			typename TSyncLock::TScopedLock lock( &parentBucket->GetLock() );

			SmallBlockAllocatorImpl::ChunkEntryHeader* freePtr = theChunk->GetFreelistHead();
			Red::System::MemUint dataStartAddress = reinterpret_cast< Red::System::MemUint >( theChunk + 1 );
			while( freePtr )
			{
				Red::System::MemUint entryIndex = ( reinterpret_cast< Red::System::MemUint >( freePtr ) - dataStartAddress ) / theChunk->GetEntrySize();
				debugBitmap[ entryIndex / 8 ] |= ( 1 << ( entryIndex & 7 ) );
				freePtr = freePtr->m_nextFreeEntry;
			}

			// Finally, walk the chunk memory, using the bitmap to determine used / free status
			Red::System::MemUint chunkEndAddr = chunkHeadAddress + m_creationParameters.m_chunkSizeBytes;
			Red::System::MemUint entryIndex = 0;
			for( Red::System::MemUint entryAddr = dataStartAddress; entryAddr < chunkEndAddr; entryAddr += theChunk->GetEntrySize() )
			{
				if( debugBitmap[ entryIndex / 8 ] & ( 1 << ( entryIndex & 7 ) ) )
				{
					theWalker->OnFreeArea( entryAddr, theChunk->GetEntrySize() );
				}
				else
				{
					theWalker->OnUsedArea( entryAddr, theChunk->GetEntrySize(), 0 );
				}
				++entryIndex;
			}
		}

		m_creationParameters.m_freeFn( debugBitmap );
	}

	/////////////////////////////////////////////////////////////////
	// RuntimeAllocate
	//	Just call through to 'static' version
	template< typename TSyncLock >
	void*  SmallBlockAllocator< TSyncLock >::RuntimeAllocate( Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::Uint16 memoryClass )
	{
		return Allocate( allocSize, allocAlignment, allocatedSize, memoryClass );
	}

	/////////////////////////////////////////////////////////////////
	// RuntimeReallocate
	//	Just call through to 'static' version
	template< typename TSyncLock >
	void*  SmallBlockAllocator< TSyncLock >::RuntimeReallocate( void* ptr, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::MemSize& freedSize, Red::System::Uint16 memoryClass )
	{
		return Reallocate( ptr, allocSize, allocAlignment, allocatedSize, freedSize, memoryClass );
	}

	/////////////////////////////////////////////////////////////////
	// RuntimeFree
	//	Just call through to 'static' version
	template< typename TSyncLock >
	EAllocatorFreeResults  SmallBlockAllocator< TSyncLock >::RuntimeFree( void* ptr )
	{
		return Free( ptr );
	}

	/////////////////////////////////////////////////////////////////
	// RuntimeGetAllocationSize
	//	Just call through to 'static' version
	template< typename TSyncLock >
	Red::System::MemSize  SmallBlockAllocator< TSyncLock >::RuntimeGetAllocationSize( void* ptr ) const
	{
		return GetAllocationSize( ptr );
	}

	/////////////////////////////////////////////////////////////////
	// RuntimeOwnsPointer
	//	Just call through to 'static' version
	template< typename TSyncLock >
	Red::System::Bool  SmallBlockAllocator< TSyncLock >::RuntimeOwnsPointer( void* ptr ) const
	{
		return SmallBlockAllocator< TSyncLock >::OwnsPointer( ptr );
	}

	/////////////////////////////////////////////////////////////////
	// DumpDebugInfoVerbose
	//	Logs loads of debug info in case something goes wrong
	template< typename TSyncLock >
	void SmallBlockAllocator< TSyncLock >::DumpDebugOutput()
	{
		RED_MEMORY_LOG( TXT( "--------------------------------------------------------------------" ) );
		RED_MEMORY_LOG( TXT( "SmallBlockAllocator Debug Info" ) );
		RED_MEMORY_LOG( TXT( "\tChunk Size: %d. Maximum Entry / Chunk: %d" ), m_creationParameters.m_chunkSizeBytes, m_creationParameters.m_maximumEntriesPerChunk );
		for( Red::System::Uint32 bucket = 0; bucket < m_bucketCount; ++bucket )
		{
			RED_MEMORY_LOG( TXT( "\tBucket %d: %d - %d bytes" ),
							bucket,
							m_creationParameters.m_bucketDefinitions[ bucket ].m_minimumAllocationSize,
							m_creationParameters.m_bucketDefinitions[ bucket ].m_maximumAllocationSize );
		}

		for( Red::System::Uint32 bucket = 0; bucket < m_bucketCount; ++bucket )
		{
			typename TSyncLock::TScopedLock lock( &m_bucketArray[ bucket ].GetLock() );

			RED_MEMORY_LOG( TXT( "------------------------------------" ) );
			RED_MEMORY_LOG( TXT( "Bucket: %d. Entry Size: %d. Current cached empty chunks: %d" ), 
							bucket, 
							m_bucketArray[ bucket ].GetEntrySize(),
							m_bucketArray[ bucket ].EmptyChunkCount() );
			SmallBlockAllocatorImpl::Chunk* chunk = m_bucketArray[ bucket ].GetFreeChunksHead();
			if( chunk )
			{
				RED_MEMORY_LOG( TXT( "Free Chunks:" ) );
				while( chunk )
				{
					Red::System::MemUint chunkAddr = reinterpret_cast< Red::System::MemUint >( chunk );
					RED_MEMORY_LOG( TXT( "Chunk %p. Full? %ls. Empty? %ls. On Free List? %ls. Used Entries (Cached): %d" ),
									chunkAddr, 
									chunk->IsFull() ? TXT( "true" ) : TXT( "false" ),
									chunk->IsEmpty() ? TXT( "true" ) : TXT( "false" ),
									chunk->IsOnFreeList() ? TXT( "true" ) : TXT( "false" ),
									chunk->GetUsedEntryCount() );

					Red::System::Int32 freeCount = 0;
					SmallBlockAllocatorImpl::ChunkEntryHeader* freeEntry = chunk->GetFreelistHead();
					while( freeEntry )
					{
						++freeCount;
						freeEntry = freeEntry->m_nextFreeEntry;
					}

					// Calculate the total entries
					Red::System::MemUint entryDataSize =  ( chunkAddr + m_creationParameters.m_chunkSizeBytes ) - reinterpret_cast< Red::System::MemUint >( chunk->GetFirstEntry() );
					Red::System::MemUint entryCount = entryDataSize / m_bucketArray[ bucket ].GetEntrySize();
					RED_UNUSED( entryCount );
					RED_MEMORY_LOG( TXT( "\tActual: Full? %ls. Empty? %ls. Free Entries: %d. Used Entries: %d" ),
									freeCount == 0 ? TXT( "true" ) : TXT( "false" ),
									freeCount == entryCount ? TXT( "true" ) : TXT( "false" ),
									freeCount,
									entryCount - freeCount );
					chunk = chunk->GetNext();
				}
			}

			chunk = m_bucketArray[ bucket ].GetFullChunksHead();
			if( chunk )
			{
				RED_MEMORY_LOG( TXT( "------------------------------------" ) );
				RED_MEMORY_LOG( TXT( "Full Chunks:" ) );
				while( chunk )
				{
					Red::System::MemUint chunkAddr = reinterpret_cast< Red::System::MemUint >( chunk );
					RED_MEMORY_LOG( TXT( "Chunk %p. Full? %ls. Empty? %ls. On Free List? %ls. Used Entries (Cached): %d" ),
						chunkAddr, 
						chunk->IsFull() ? TXT( "true" ) : TXT( "false" ),
						chunk->IsEmpty() ? TXT( "true" ) : TXT( "false" ),
						chunk->IsOnFreeList() ? TXT( "true" ) : TXT( "false" ),
						chunk->GetUsedEntryCount() );

					Red::System::Int32 freeCount = 0;
					SmallBlockAllocatorImpl::ChunkEntryHeader* freeEntry = chunk->GetFreelistHead();
					while( freeEntry )
					{
						++freeCount;
						freeEntry = freeEntry->m_nextFreeEntry;
					}

					// Calculate the total entries
					Red::System::MemUint entryDataSize =  ( chunkAddr + m_creationParameters.m_chunkSizeBytes ) - reinterpret_cast< Red::System::MemUint >( chunk->GetFirstEntry() );
					Red::System::MemUint entryCount = entryDataSize / m_bucketArray[ bucket ].GetEntrySize();
					RED_UNUSED( entryCount );
					RED_MEMORY_LOG( TXT( "\tActual: Full? %ls. Empty? %ls. Free Entries: %d. Used Entries: %d" ),
						freeCount == 0 ? TXT( "true" ) : TXT( "false" ),
						freeCount == entryCount ? TXT( "true" ) : TXT( "false" ),
						freeCount,
						entryCount - freeCount );
					chunk = chunk->GetNext();
				}
			}
		}

		RED_MEMORY_LOG( TXT( "Free chunk pool:" ) );
		{
			typename TSyncLock::TScopedLock lock( &m_chunkPoolLock);
			SmallBlockAllocatorImpl::Chunk* freeChunks = m_chunkPoolHead;
			while( freeChunks )
			{
				if( freeChunks->IsPermanent() )
				{
					RED_MEMORY_LOG( TXT( "\tPermanent chunk %p" ), freeChunks );
				}
				else
				{
					RED_MEMORY_LOG( TXT( "\tLoose chunk %p" ), freeChunks );
				}
				freeChunks = freeChunks->GetNext();
			}
		}
	}
} }