/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_FRAMEWORK_SMALL_BLOCK_ALLOCATOR_H
#define _RED_MEMORY_FRAMEWORK_SMALL_BLOCK_ALLOCATOR_H
#pragma once

//#define NEED_MORE_MEMORY_TO_DEBUG_MEMORY_STOMPS

#include "../redSystem/types.h"
#include "redMemoryAllocator.h"
#include "redMemoryListHelpers.h"

namespace Red { namespace MemoryFramework { namespace SmallBlockAllocatorImpl {

	/////////////////////////////////////////////////////////////////////////////
	// Constants
	static const Red::System::Uint32 k_MaximumBuckets = 32;
	static const Red::System::MemSize k_SmallestAllocationAllowed = sizeof( Red::System::MemInt );
	static const Red::System::MemSize k_DefaultChunkSize = 1024 * 1024;

#ifdef NEED_MORE_MEMORY_TO_DEBUG_MEMORY_STOMPS
	static const Red::System::MemSize k_MaximumAllocationAllowed = 256;
#else
	static const Red::System::MemSize k_MaximumAllocationAllowed = 128;
#endif

	/////////////////////////////////////////////////////////////////////////////
	// Internal types

	// Chunk allocation handled via callbacks (can be lambdas, statics, whatever)
	typedef void* ( *ChunkAllocate )( Red::System::MemSize, Red::System::MemSize );		// Size, Alignment
	typedef void ( *ChunkFree )( void* );
	typedef Bool ( *ChunkOwnership )( void* );		// Test ownership of chunk

	// Entry header, used to make the freelists more readable
	struct ChunkEntryHeader
	{
		union
		{
			ChunkEntryHeader* m_nextFreeEntry;
			void* m_data;
		};
	};

	class BucketBase	{ };		// Used as a base class proxy

	// Chunks contain fixed-size entries (allocations) and are tracked in intrusive lists
	class Chunk : public Utils::ListNode< Chunk >
	{
	public:
		Chunk( Bool isPermanent );
		~Chunk();

		void Initialise( BucketBase* parentBucket, Red::System::MemSize entrySize, Red::System::MemSize chunkSize );
		void Cleanup();
		void* AllocateEntry();
		void FreeEntry( void* entry );
		void SetOnFreeList( Bool onFreeList );

		Bool IsPermanent() const;					// Can we free this chunk's memory
		Bool IsFull() const;						// Free list empty?
		Bool IsEmpty() const;						// All memory free?
		Bool IsValid() const;
		Bool IsOnFreeList() const;
		Red::System::Uint32 GetEntrySize() const;
		BucketBase* GetParentBucket() const;
		ChunkEntryHeader* GetFreelistHead() const;
		Red::System::Uint32 GetUsedEntryCount() const;
		ChunkEntryHeader* GetFirstEntry() const;

	private:
		ChunkEntryHeader* m_freeEntryListHead;		// Entry free-list head
		Red::System::Uint32 m_entrySize;			// Size of an allocation
		Red::System::Uint32 m_usedEntryCount;		// Track # of free entries (so we can test when we can free the entire thing)

		ChunkEntryHeader* m_firstEntryAddress;		// Data ptr
		Red::System::MemSize m_usableSize;

		BucketBase* m_parentBucket;

		// Sentinel block
		Red::System::Uint32 m_chunkSentinel : 30;
		Red::System::Bool m_isOnFreeList : 1;		// Use a single bit to track free-list status. This gives us o(1) free()
		Red::System::Bool m_isPermanent : 1;		// Use a single bit to track allocation status. 1 = permanent chunk (no free possible)
	};

	static_assert( sizeof( Chunk ) % 16 == 0, "Chunk struct MUST be a multiple of 16 to ensure correct alignment" );

	// Buckets contain a list of chunks for a particular allocation size
	template< typename TSyncLock >
	class Bucket : public BucketBase
	{
	public:
		Bucket();
		~Bucket();

		void SetEntrySize( Red::System::Uint32 size );
		void AddNewChunk( Chunk* newChunk, Red::System::MemSize chunkSize );
		void* GetFreeEntry();
		void ReleaseEntryFromChunk( const void* ptr, Chunk* theChunk );
		void ReleaseEmptyChunks( Chunk*& releasedChunksHead, Chunk*& releasedChunksTail );
		void ReleaseAllChunks( ChunkFree freeFn );

		Red::System::Uint32 GetEntrySize() const;
		Red::System::Uint32 EmptyChunkCount() const;

		Chunk* GetFreeChunksHead() const;
		Chunk* GetFullChunksHead() const;

		RED_INLINE TSyncLock& GetLock()				{ return m_lock; }

	private:
		Red::System::Uint32 m_entrySize;			// Size of largest allocation supported by this bucket
		Red::System::Uint32 m_emptyChunks;			// Number of completely empty (free) chunks

		Chunk* m_freeChunksHead;					// Headptr of list of chunks with at least 1 free entry
		Chunk* m_freeChunksTail;					// Last free chunk

		Chunk* m_fullChunksHead;					// Chunks with no free entries
		Chunk* m_fullChunksTail;					// Last with no free entries

		TSyncLock m_lock;							// Note! The bucket does not touch its own lock, it is handled externally
	};

	// Allocator creation parameters. Contains a list of bucket definitions
	class CreationParameters : public IAllocatorCreationParameters
	{
	public:
		struct BucketDefinition
		{
			BucketDefinition();
			BucketDefinition( Red::System::Uint32 minAllocSize, Red::System::Uint32 maxAllocSize );
			Red::System::Uint32 m_minimumAllocationSize;
			Red::System::Uint32 m_maximumAllocationSize;
		};

		// Default CTor creates a generic bucket size list that should be good for most situations
		CreationParameters();
		CreationParameters( ChunkAllocate allocfn, ChunkFree freefn, ChunkOwnership ownerFn, 
							Red::System::MemSize chunkSize = k_DefaultChunkSize, 
							Red::System::Uint32 permanentChunksToPreallocate = 0 );	// These will be allocated in one huge block, and *never freed*

		// Use this if you want to use custom bucket definitions
		void SetBuckets( BucketDefinition* definitions, Red::System::Uint32 bucketCount );

		// Test bucket definitions are valid
		Bool IsValid() const;

		// Default bucket setup
		void InitialiseDefaultBuckets();
		void RecalculateMaxEntriesPerChunk();

		Red::System::MemSize m_chunkSizeBytes;								// Size of a single chunk in bytes (must be power-of-two!)
		Red::System::Uint32 m_permanentChunksToPreallocate;				// Number of chunk buffers to pre-allocate as one large permanent allocation
		ChunkAllocate m_allocFn;											// Chunk allocate function
		ChunkFree m_freeFn;													// Chunk free function
		ChunkOwnership m_ownershipFn;										// Test chunk owned by parent allocator
		Red::System::Uint32 m_bucketDefinitionCount;						// Number of bucket definitions
		Red::System::Uint32 m_maximumEntriesPerChunk;						// Max entries per chunk. Generated from bucket definitions
		BucketDefinition m_bucketDefinitions[ k_MaximumBuckets ];			// Bucket definitions ( smallestAllocation, largestAllocation, default chunk count )
	};
} } }

namespace Red { namespace MemoryFramework {

	//////////////////////////////////////////////////////////////////
	// Small block allocator
	//	Contains multiple buckets, each handling fixed-size allocations
	//	Each bucket contains multiple 'chunks', that contain  a free-list per chunk
	//	Minimum allocation size = sizeof( intptr_t ) so free-list can be intrusive to reduce bookkeeping footprint

	namespace SmallBlockAllocatorImpl { 
		class CreationParameters;
	}

	template< typename TSyncLock >
	class SmallBlockAllocator : public IAllocator 
	{
	public:
		SmallBlockAllocator();
		virtual ~SmallBlockAllocator();

		virtual EAllocatorInitResults Initialise( const IAllocatorCreationParameters& parameters, Red::System::Uint32 flags );
		virtual void Release( );

		virtual Red::System::Bool IncreaseMemoryFootprint( AllocatorAreaCallback& areaCallback, Red::System::MemSize sizeRequired );
		virtual Red::System::MemSize ReleaseFreeMemoryToSystem( AllocatorAreaCallback& areaCallback );

		virtual void RequestAllocatorInfo( AllocatorInfo& info );
		virtual void WalkAllocator( AllocatorWalker* theWalker );
		virtual void WalkPoolArea( Red::System::MemUint startAddress, Red::System::MemSize size, PoolAreaWalker* theWalker );

		void*	Allocate( Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::Uint16 memoryClass );
		void*	Reallocate( void* ptr, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::MemSize& freedSize, Red::System::Uint16 memoryClass );
		EAllocatorFreeResults Free( const void* ptr );
		Red::System::MemSize GetAllocationSize( const void* ptr ) const;
		Red::System::Bool OwnsPointer( const void* ptr ) const;

		typedef SmallBlockAllocatorImpl::CreationParameters CreationParameters;

		virtual void	DumpDebugOutput();

	private:
		virtual void* RuntimeAllocate( Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::Uint16 memoryClass );
		virtual void* RuntimeReallocate( void* ptr, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::MemSize& freedSize, Red::System::Uint16 memoryClass );
		virtual EAllocatorFreeResults RuntimeFree( void* ptr );
		virtual Red::System::MemSize RuntimeGetAllocationSize( void* ptr ) const;
		virtual Red::System::Bool RuntimeOwnsPointer( void* ptr ) const;
		virtual void OnOutOfMemory();

		Bool InitialiseBookeepingMemory( const CreationParameters& parameters );
		void InitialiseBucketSizeLookup( const CreationParameters& parameters );

		SmallBlockAllocatorImpl::Chunk* AllocateNewChunk();													// Allocate memory for chunk, set up header + add to pool
		void GetEmptyChunks( SmallBlockAllocatorImpl::Chunk*& listHead, SmallBlockAllocatorImpl::Chunk*& listTail, Uint32 chunkCount );

		// ++ No locking on access of this data (its pretty much const) ...
		Red::System::MemUint m_chunkAddressMask;															// ptr & address mask = chunk ptr
		SmallBlockAllocatorImpl::Bucket< TSyncLock >* m_bucketArray;										// Array of bucket headers
		Red::System::Uint32 m_bucketCount;																	// Number of bucket headers
		Red::System::Int8 m_bucketSizeLookup[ SmallBlockAllocatorImpl::k_MaximumAllocationAllowed + 1 ];	// Array mapping size (index) to bucket index. -1 = no bucket
		CreationParameters m_creationParameters;															// Cache parameters
		// -- No locking on access of this data ...

		// Empty chunks are kept in a pool so we can pre-allocate them, and add them to buckets as needed
		mutable TSyncLock m_chunkPoolLock;																	// Chunk pool is protected
		SmallBlockAllocatorImpl::Chunk* m_chunkPoolHead;
		SmallBlockAllocatorImpl::Chunk* m_chunkPoolTail;
		void* m_permanentMemoryBlock;																		// Pre-allocated chunks go in one huge block
	};

} }


#endif