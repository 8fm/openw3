/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

namespace Red { namespace MemoryFramework { namespace SmallBlockAllocatorImpl {

	const Red::System::Uint32 k_ChunkSentinelValue = 0x0043686b;		// 'Chk'
	const Red::System::Uint32 k_ChunkSentinelBadValue = 0x004e6f70;		// 'Nop'

	///////////////////////////////////////////////////////////////////
	// Chunk Default CTor
	//
	Chunk::Chunk( Bool isPermanent )
		: m_entrySize( 0 )
		, m_usedEntryCount( 0 )
		, m_usableSize( 0 )
		, m_isOnFreeList( false )
		, m_isPermanent( isPermanent )
	{
		m_chunkSentinel = k_ChunkSentinelBadValue;
	}

	///////////////////////////////////////////////////////////////////
	// Chunk DTor
	//
	Chunk::~Chunk()
	{
		m_chunkSentinel = k_ChunkSentinelBadValue;
	}

	///////////////////////////////////////////////////////////////////
	// Cleanup
	//	Remove internal lists, etc, but don't fully destroy self. opposite of Initialise
	void Chunk::Cleanup()
	{
		m_entrySize = 0;
		m_usedEntryCount = 0;
		m_freeEntryListHead = nullptr;
		m_firstEntryAddress = nullptr;
		m_usableSize = 0;
		m_parentBucket = nullptr;
		m_chunkSentinel = k_ChunkSentinelBadValue;
	}

	///////////////////////////////////////////////////////////////////
	// Initialise
	//	Prepare internal list as all free, ready for use
	void Chunk::Initialise( BucketBase* parentBucket, Red::System::MemSize entrySize, Red::System::MemSize chunkSize )
	{
		const Red::System::MemSize c_minimumAlignment = sizeof( void* );

		m_parentBucket = parentBucket;
		m_entrySize = static_cast< Red::System::Uint32 >( entrySize );
		m_usedEntryCount = 0;

		// We align all entries in a chunk to the entry size (gives us natural alignment for free)
		// Can't use fast align since entry size is not always power of two
		// For safety we need to also make sure entries are aligned to word size to avoid misaligned data
		auto baseAddress = reinterpret_cast< Red::System::MemUint >( this ) + sizeof( *this );
		auto entryAlignment = ( entrySize + c_minimumAlignment - 1 ) & ~( c_minimumAlignment - 1 );
		auto alignedDataAddr = baseAddress;
		if( ( baseAddress % entryAlignment ) != 0 )
		{
			alignedDataAddr = alignedDataAddr + entryAlignment - ( baseAddress % entryAlignment );
		}
		m_usableSize = chunkSize - sizeof( *this ) - ( alignedDataAddr - baseAddress );
		m_firstEntryAddress = reinterpret_cast< ChunkEntryHeader* >( alignedDataAddr );

		// Initialise the free-list entries. This may be a bottleneck - need to profile more
		Red::System::Uint32 entryCount = static_cast< Red::System::Uint32 >( m_usableSize / entrySize );
		ChunkEntryHeader* headPtr = m_firstEntryAddress;
		for( Uint32 e = 0; e < entryCount - 1; ++e )
		{
			Red::System::MemUint baseAddr = reinterpret_cast< Red::System::MemUint >( headPtr );
			headPtr->m_nextFreeEntry =		reinterpret_cast< ChunkEntryHeader* >( baseAddr + entrySize );
			headPtr = reinterpret_cast< ChunkEntryHeader* >( baseAddr + entrySize );
		}
		headPtr->m_nextFreeEntry = nullptr;
		m_freeEntryListHead = m_firstEntryAddress;

		m_chunkSentinel = k_ChunkSentinelValue;

		RED_FATAL_ASSERT( reinterpret_cast< Red::System::MemUint >( headPtr + 1 ) <= reinterpret_cast< Red::System::MemUint >( this ) + chunkSize, "Buffer overrun" );
	}

	///////////////////////////////////////////////////////////////////
	// IsEmpty
	//	Is the chunk totally free (no allocations)
	RED_INLINE Bool Chunk::IsEmpty() const
	{
		return m_usedEntryCount == 0;
	}

	///////////////////////////////////////////////////////////////////
	// IsValid
	//	Check sentinel value to ensure a chunk ptr is valid
	RED_INLINE Bool Chunk::IsValid() const
	{
		return m_chunkSentinel == k_ChunkSentinelValue;
	}

	///////////////////////////////////////////////////////////////////
	// GetParentBucket
	//
	RED_INLINE BucketBase* Chunk::GetParentBucket() const
	{
		return m_parentBucket;
	}

	///////////////////////////////////////////////////////////////////
	// GetFreelistHead
	//
	RED_INLINE ChunkEntryHeader* Chunk::GetFreelistHead() const
	{
		return m_freeEntryListHead;
	}

	///////////////////////////////////////////////////////////////////
	// GetUsedEntryCount
	//
	RED_INLINE Red::System::Uint32 Chunk::GetUsedEntryCount() const
	{
		return m_usedEntryCount;
	}

	///////////////////////////////////////////////////////////////////
	// GetFirstEntry
	//
	RED_INLINE ChunkEntryHeader* Chunk::GetFirstEntry() const
	{
		return m_firstEntryAddress;
	}

	///////////////////////////////////////////////////////////////////
	// SetOnFreeList
	//
	RED_INLINE void Chunk::SetOnFreeList( Bool onFreeList )
	{
		m_isOnFreeList = onFreeList;
	}

	///////////////////////////////////////////////////////////////////
	// IsOnFreeList
	//
	RED_INLINE Bool Chunk::IsOnFreeList() const
	{
		return m_isOnFreeList;
	}

	///////////////////////////////////////////////////////////////////
	// GetEntrySize
	//
	RED_INLINE Red::System::Uint32 Chunk::GetEntrySize() const
	{
		return m_entrySize;
	}

	///////////////////////////////////////////////////////////////////
	// IsFull
	//	Return true if there are no more free entries
	RED_INLINE Bool Chunk::IsFull() const
	{
		return m_freeEntryListHead == nullptr;
	}

	///////////////////////////////////////////////////////////////////
	// IsPermanent
	//	Return true if this chunk owns its memory
	RED_INLINE Bool Chunk::IsPermanent() const
	{
		return m_isPermanent;
	}

	///////////////////////////////////////////////////////////////////
	// AllocateEntry
	//	Return first free entry; does nothing if free list is empty
	RED_INLINE void* Chunk::AllocateEntry()
	{
		void* result = m_freeEntryListHead;
		if( m_freeEntryListHead )
		{
			m_freeEntryListHead = m_freeEntryListHead->m_nextFreeEntry;
		}
		++m_usedEntryCount;
		return result;
	}

	///////////////////////////////////////////////////////////////////
	// FreeEntry
	//	Push the entry to the front of the free list
	RED_INLINE void Chunk::FreeEntry( void* entry )
	{
		ChunkEntryHeader* entryHeader = static_cast< ChunkEntryHeader* >( entry );
		entryHeader->m_nextFreeEntry = m_freeEntryListHead;
		m_freeEntryListHead = entryHeader;
		--m_usedEntryCount;
	}

	///////////////////////////////////////////////////////////////////
	// Bucket Default CTor
	//	
	template< typename TSyncLock >
	Bucket< TSyncLock >::Bucket()
		: m_entrySize( 0 )
		, m_emptyChunks( 0 )
		, m_freeChunksHead( nullptr )
		, m_freeChunksTail( nullptr )
		, m_fullChunksHead( nullptr )
		, m_fullChunksTail( nullptr )
	{
	}

	///////////////////////////////////////////////////////////////////
	// Bucket DTor
	//	
	template< typename TSyncLock >
	Bucket< TSyncLock >::~Bucket()
	{

	}

	///////////////////////////////////////////////////////////////////
	// ReleaseEmptyChunks
	//	Remove all empty chunks from internal free list and call releaseFn so parent caller can deal with them
	template< typename TSyncLock >
	void Bucket< TSyncLock >::ReleaseEmptyChunks( Chunk*& releasedChunksHead, Chunk*& releasedChunksTail )
	{
		Chunk* freeChunk = m_freeChunksTail;
		while( freeChunk != nullptr )
		{
			Chunk* prevChunk = freeChunk->GetPrevious();
			if( freeChunk->IsEmpty() )
			{
				Utils::RemoveFromList( m_freeChunksHead, m_freeChunksTail, freeChunk);
				Utils::PushBack( releasedChunksHead, releasedChunksTail, freeChunk );
			}
			freeChunk = prevChunk;
		}
		m_emptyChunks = 0;
	}

	///////////////////////////////////////////////////////////////////
	// ReleaseAllChunks
	//	Remove all chunks regardless of internal state. Be careful if there are still things allocated in here!
	template< typename TSyncLock >
	void Bucket< TSyncLock >::ReleaseAllChunks( ChunkFree freeFn )
	{
		// Release the 'free' chunks first
		Chunk* theChunk = m_freeChunksHead;
		while( theChunk != nullptr )
		{
			Chunk* nextChunk = theChunk->GetNext();
			Bool isPermanent = theChunk->IsPermanent();
			theChunk->~Chunk();
			if( !isPermanent )
			{
				freeFn( theChunk );
			}
			theChunk = nextChunk;
		}

		// Release the 'used' chunks
		theChunk = m_fullChunksHead;
		while( theChunk != nullptr )
		{
			Chunk* nextChunk = theChunk->GetNext();
			Bool isPermanent = theChunk->IsPermanent();
			theChunk->~Chunk();
			if( !isPermanent )
			{
				freeFn( theChunk );
			}
			theChunk = nextChunk;
		}
		m_freeChunksHead = m_freeChunksTail = nullptr;
		m_fullChunksHead = m_fullChunksTail = nullptr;
		m_emptyChunks = 0;
	}

	///////////////////////////////////////////////////////////////////
	// AddNewChunk
	//	Add a new chunk for the memory passed in. Initialise bookkeeping, etc
	template< typename TSyncLock >
	void Bucket< TSyncLock >::AddNewChunk( Chunk* theChunk, Red::System::MemSize chunkSize )
	{
		RED_MEMORY_ASSERT( chunkSize > sizeof( Chunk ), "Chunk memory block is too small!" );

		theChunk->Initialise( this, m_entrySize, chunkSize );

		// Push the free chunk to the back of the free chunk list (so we try and use partially free chunks first by popping the head on alloc)
		Utils::PushBack( m_freeChunksHead, m_freeChunksTail, theChunk );
		theChunk->SetOnFreeList( true );

		++m_emptyChunks;
	}

	///////////////////////////////////////////////////////////////////
	// GetFreeEntry
	//	Return a free block if one exists. Does NOT allocate new chunks from the system if it fails
	template< typename TSyncLock >
	RED_INLINE void* Bucket< TSyncLock >::GetFreeEntry()
	{
		if( m_freeChunksHead )
		{
			if( m_freeChunksHead->IsEmpty() )
			{
				--m_emptyChunks;
			}
			void* foundEntry = m_freeChunksHead->AllocateEntry();
			if( m_freeChunksHead->IsFull() )
			{
				// Chunk is full, remove from free list, add to full list
				Chunk* fullChunk = Utils::PopFront( m_freeChunksHead, m_freeChunksTail );
				Utils::PushFront( m_fullChunksHead, m_fullChunksTail, fullChunk );
				fullChunk->SetOnFreeList( false );
			}
			return foundEntry;
		}
		else
		{
			// No chunks
			return nullptr;
		}
	}

	///////////////////////////////////////////////////////////////////
	// ReleaseEntryFromChunk
	//	Essentially frees the memory at ptr back to the chunk, updates internal lists if required
	template< typename TSyncLock >
	RED_INLINE void Bucket< TSyncLock >::ReleaseEntryFromChunk( const void* ptr, Chunk* theChunk )
	{
		// Free the entry for the chunk
		theChunk->FreeEntry( const_cast< void* >( ptr ) );

		// If the chunk is not on the free list, we need to do it now (assume it's on the 'used' list)
		if( !theChunk->IsOnFreeList() )
		{
			// Assume its on the 'full' list
			Utils::RemoveFromList( m_fullChunksHead, m_fullChunksTail, theChunk );
			Utils::PushFront( m_freeChunksHead, m_freeChunksTail, theChunk );
			theChunk->SetOnFreeList( true );
		}

		if( theChunk->IsEmpty() )
		{
			++m_emptyChunks;
		}
	}

	///////////////////////////////////////////////////////////////////
	// SetEntrySize
	//	set individual allocation size
	template< typename TSyncLock >
	void Bucket< TSyncLock >::SetEntrySize( Red::System::Uint32 size )
	{
		RED_MEMORY_ASSERT( size <= k_MaximumAllocationAllowed, "Bucket size is too big!" );
		m_entrySize = size;
	}

	///////////////////////////////////////////////////////////////////
	// GetEntrySize
	//	
	template< typename TSyncLock >
	RED_INLINE Red::System::Uint32 Bucket< TSyncLock >::GetEntrySize() const
	{
		return m_entrySize;
	}

	///////////////////////////////////////////////////////////////////
	// EmptyChunkCount
	//	Returns number of completely free chunks
	template< typename TSyncLock >
	RED_INLINE Red::System::Uint32 Bucket< TSyncLock >::EmptyChunkCount() const
	{
		return m_emptyChunks;
	}

	///////////////////////////////////////////////////////////////////
	// GetFreeChunksHead
	//
	template< typename TSyncLock >
	RED_INLINE Chunk* Bucket< TSyncLock >::GetFreeChunksHead() const
	{
		return m_freeChunksHead;
	}

	///////////////////////////////////////////////////////////////////
	// GetFullChunksHead
	//
	template< typename TSyncLock >
	RED_INLINE Chunk* Bucket< TSyncLock >::GetFullChunksHead() const
	{
		return m_fullChunksHead;
	}

	///////////////////////////////////////////////////////////////////
	// BucketDefinition Default CTor
	//	
	CreationParameters::BucketDefinition::BucketDefinition()
		: m_minimumAllocationSize( 0 )
		, m_maximumAllocationSize( 0 ) 
	{

	}

	///////////////////////////////////////////////////////////////////
	// BucketDefinition CTor
	//	
	CreationParameters::BucketDefinition::BucketDefinition( Red::System::Uint32 minAllocSize, Red::System::Uint32 maxAllocSize )
		: m_minimumAllocationSize( minAllocSize )
		, m_maximumAllocationSize( maxAllocSize )
	{ 
	}

	///////////////////////////////////////////////////////////////////
	// CreationParameters CTor
	//	Sets up buckets with some reasonable defaults
	CreationParameters::CreationParameters( ChunkAllocate allocfn, ChunkFree freefn, ChunkOwnership ownerFn, 
											Red::System::MemSize chunkSize, Red::System::Uint32 permanentChunksToPreallocate )
		: m_chunkSizeBytes( chunkSize )
		, m_permanentChunksToPreallocate( permanentChunksToPreallocate )
		, m_allocFn( allocfn )
		, m_freeFn( freefn )
		, m_ownershipFn( ownerFn )
		, m_bucketDefinitionCount( 0 )
		, m_maximumEntriesPerChunk( 0 )
	{
		InitialiseDefaultBuckets();
		RecalculateMaxEntriesPerChunk();

		RED_MEMORY_ASSERT( IsValid(), "Bad bucket parameters" );
	}

	///////////////////////////////////////////////////////////////////
	// CreationParameters default CTor
	//
	CreationParameters::CreationParameters()
		: m_chunkSizeBytes( 0 )
		, m_allocFn( nullptr )
		, m_freeFn( nullptr )
		, m_bucketDefinitionCount( 0 )
		, m_maximumEntriesPerChunk( 0 )
	{
	}

	///////////////////////////////////////////////////////////////////
	// InitialiseDefaultBuckets
	//	
	void CreationParameters::InitialiseDefaultBuckets()
	{
		m_bucketDefinitionCount = 0;
		m_bucketDefinitions[ m_bucketDefinitionCount++ ] = BucketDefinition( 0, 8 );
		m_bucketDefinitions[ m_bucketDefinitionCount++ ] = BucketDefinition( 9, 16 );
		m_bucketDefinitions[ m_bucketDefinitionCount++ ] = BucketDefinition( 17, 24 );
		m_bucketDefinitions[ m_bucketDefinitionCount++ ] = BucketDefinition( 25, 32 );
		m_bucketDefinitions[ m_bucketDefinitionCount++ ] = BucketDefinition( 33, 48 );
		m_bucketDefinitions[ m_bucketDefinitionCount++ ] = BucketDefinition( 49, 64 );
		m_bucketDefinitions[ m_bucketDefinitionCount++ ] = BucketDefinition( 65, 80 );
		m_bucketDefinitions[ m_bucketDefinitionCount++ ] = BucketDefinition( 81, 96 );
		m_bucketDefinitions[ m_bucketDefinitionCount++ ] = BucketDefinition( 97, 128 );

#ifdef NEED_MORE_MEMORY_TO_DEBUG_MEMORY_STOMPS
		m_bucketDefinitions[ m_bucketDefinitionCount++ ] = BucketDefinition( 129 , 256 );
#endif

		RecalculateMaxEntriesPerChunk();
	}

	///////////////////////////////////////////////////////////////////
	// RecalculateMaxEntriesPerChunk
	//	
	void CreationParameters::RecalculateMaxEntriesPerChunk()
	{
		Red::System::Uint32 smallestEntry = static_cast< Red::System::Uint32 >( m_chunkSizeBytes );
		for( Red::System::Uint32 i = 0; i < m_bucketDefinitionCount; ++i )
		{
			smallestEntry = Red::Math::NumericalUtils::Min( smallestEntry, m_bucketDefinitions[ i ].m_maximumAllocationSize );
		}
		m_maximumEntriesPerChunk = (Red::System::Uint32)m_chunkSizeBytes / smallestEntry;
	}

	///////////////////////////////////////////////////////////////////
	// SetBuckets
	//  Use this if you want custom bucket definitions
	void CreationParameters::SetBuckets( BucketDefinition* definitions, Red::System::Uint32 bucketCount )
	{
		RED_MEMORY_ASSERT( bucketCount < k_MaximumBuckets, "Too many buckets wanted - maximum is %d", bucketCount );
		RED_MEMORY_ASSERT( definitions, "Bad definition ptr" );

		for( Red::System::Uint32 i=0; i< bucketCount; ++i )
		{
			m_bucketDefinitions[ i ] = definitions[ i ];
		}
		m_bucketDefinitionCount = bucketCount;

		RED_MEMORY_ASSERT( IsValid(), "Bad bucket parameters" );
	}

	///////////////////////////////////////////////////////////////////
	// IsValid
	//	Test that bucket definitions are ok (sizes good, no overlaps)
	Bool CreationParameters::IsValid() const
	{
		if( m_chunkSizeBytes == 0 )
		{
			RED_MEMORY_LOG( TXT( "Chunk size must be > 0" ) );
			return false;
		}

		if( ( m_chunkSizeBytes & ( m_chunkSizeBytes - 1 ) ) != 0 )
		{
			RED_MEMORY_LOG( TXT( "Chunk size must be a power-of-two" ) );
		}

		// Populate bucket list. Bucket parameters are validated here (should be smallest -> largest, no overlaps)
		Red::System::Int32 lastMax = -1;
		for( Red::System::Uint32 i=0; i< m_bucketDefinitionCount; ++i )
		{
			if( m_bucketDefinitions[ i ].m_maximumAllocationSize == 0 )
			{
				RED_MEMORY_LOG( TXT( "Bucket %d entry size cannot be 0" ), i );
				return false;
			}

			if( m_bucketDefinitions[ i ].m_minimumAllocationSize >  k_MaximumAllocationAllowed )
			{
				RED_MEMORY_LOG( TXT( "Bucket %d Minimum alloc size too big" ), i );
				return false;
			}

			if( m_bucketDefinitions[ i ].m_minimumAllocationSize >= m_bucketDefinitions[ i ].m_maximumAllocationSize )
			{
				RED_MEMORY_LOG( TXT( "Bucket %d invalid!" ), i );
				return false;
			}

			if( static_cast< Red::System::Int32 >( m_bucketDefinitions[ i ].m_minimumAllocationSize ) <= lastMax )
			{
				RED_MEMORY_LOG( TXT( "Bucket %d Overlapping buckets!" ), i );
				return false;
			}

			if( m_bucketDefinitions[ i ].m_maximumAllocationSize >  k_MaximumAllocationAllowed )
			{
				RED_MEMORY_LOG( TXT( "Bucket %d Maximum alloc size too big" ), i );
				return false;
			}

			if( m_bucketDefinitions[ i ].m_maximumAllocationSize & ( k_SmallestAllocationAllowed-1 ) )
			{
				RED_MEMORY_LOG( TXT( "Bucket max sizes must be a multiple of %d" ), k_SmallestAllocationAllowed );
				return false;
			}

			lastMax = static_cast< Red::System::Int32 >( m_bucketDefinitions[ i ].m_maximumAllocationSize );
		}

		return true;
	}


} } }
