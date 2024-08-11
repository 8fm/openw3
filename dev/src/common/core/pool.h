#pragma once

/**
 *	Fixed size block allocator (aka pool).
 *
 *	Features:
 *	- very fast: constant allocation / deallocation time
 *	- zero bytes per element overhead
 *	- doesn't do any dynamic memory allocations; only uses memory given via Init() function
 */
class CPool
{
public:
	static const Uint32 INVALID_INDEX = 0xFFFFFFFF;

	CPool();
	CPool( CPool && value );

	// Initializes pool with given memory; Note: it's up to user to release that memory when not using pool
	void Init( void* memory, Uint32 size, Uint32 blockSize );
	// Deinitialized pool
	void Deinit();
	// Gets whether pool is initialized
	RED_FORCE_INLINE Bool IsInitialized() const { return m_memory != nullptr; }
	// Clears all elements
	void Clear();
	// Gets pool memory (as specified via Init() function)
	RED_FORCE_INLINE void* GetMemory() { return m_memory; }
	// Gets total number of managed blocks
	RED_FORCE_INLINE Uint32 GetNumBlocks() const { return m_numBlocks; }

	// Gets pool capacity
	RED_FORCE_INLINE Uint32 Capacity() const { return m_numBlocks; }
	// Gets whether pool has any free blocks
	RED_FORCE_INLINE Bool HasFreeBlocks() const { return m_firstFreeBlock != nullptr; }

	// Allocates new block; returns nullptr on failure
	void* AllocateBlock();
	// Allocates new block; returns new block index or INVALID_INDEX on failure
	Uint32 AllocateBlockIndex();
	// Frees block
	void FreeBlock( void* ptr );
	// Frees block by index
	void FreeBlockIndex( Uint32 index );

	Uint32 GetBlockSize() const;

	// Gets memory of the block at given index
	RED_FORCE_INLINE void* GetBlock( Uint32 index ) { ASSERT( index < m_numBlocks ); return ( ( Uint8* ) m_memory + index * m_blockSize ); }
	// Gets memory of the block at given index
	RED_FORCE_INLINE const void* GetBlock( Uint32 index ) const { ASSERT( index < m_numBlocks ); return ( ( Uint8* ) m_memory + index * m_blockSize ); }
	
	// Upsizes pool capacity while preserving indices of allocated elements; performs typed 'move' operator for all existing elements
	template < typename T >
	void Upsize( void* newMemory, Uint32 newCapacity );

	// Move operator
	CPool & operator = ( CPool&& other );

	void Swap( CPool & value );

private:
	struct FreeListEntry
	{
		FreeListEntry* m_next;
	};

	void* m_memory;						// Pool memory
	Uint32 m_numBlocks;					// Number of managed blocks
	Uint32 m_blockSize;					// Size of single block
	FreeListEntry* m_firstFreeBlock;	// List of free elements
};

template < typename T >
void CPool::Upsize( void* newMemory, Uint32 newCapacity )
{
	ASSERT( sizeof( T ) == m_blockSize );
	ASSERT( !m_firstFreeBlock ); // For now only supporting upsizing when full
	ASSERT( newCapacity > m_numBlocks );

	// Move old elements into new memory

	T* src = ( T* ) m_memory;
	T* dst = ( T* ) newMemory;
	for ( Uint32 i = 0; i < m_numBlocks; ++i )
	{
		*dst = Move( *src );
		src->~T();
		++src;
		++dst;
	}

	const Uint32 oldNumBlocks = m_numBlocks;

	m_memory = newMemory;
	m_numBlocks = newCapacity;

	// Put all new items onto free list

	FreeListEntry* prevBlock = m_firstFreeBlock = ( FreeListEntry* ) GetBlock( oldNumBlocks );
	for ( Uint32 i = oldNumBlocks + 1; i < m_numBlocks; i++ )
	{
		FreeListEntry* block = ( FreeListEntry* ) GetBlock( i );
		prevBlock->m_next = block;
		prevBlock = block;
	}
	prevBlock->m_next = nullptr;
}

RED_FORCE_INLINE void* operator new ( size_t, CPool& pool ) { return pool.AllocateBlock(); }
RED_FORCE_INLINE void operator delete ( void* ptr, CPool& pool ) { pool.FreeBlock( ptr ); }

/**
 *	Fixed size object allocator based on CPool.
 */
template < typename TYPE, EMemoryClass memoryClass = MC_Pool, RED_CONTAINER_POOL_TYPE memoryPool = MemoryPool_Default >
class TPool
{
public:
	~TPool() { Deinit(); }
	void Init( Uint32 count );
	void Deinit();
	RED_FORCE_INLINE Bool IsInitialized() const { return m_pool.IsInitialized(); }
	RED_FORCE_INLINE void* Allocate() { return m_pool.AllocateBlock(); }
	RED_FORCE_INLINE void Free( void* ptr ) { return m_pool.FreeBlock( ptr ); }
private:
	CPool m_pool;
};

// Template implementation

template < typename TYPE, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void TPool< TYPE, memoryClass, memoryPool >::Init( Uint32 count )
{
	Deinit();
	const Uint32 memorySize = count * sizeof( TYPE );
	void* memory = RED_MEMORY_ALLOCATE( memoryPool, memoryClass, memorySize );
	m_pool.Init( memory, memorySize, sizeof(TYPE) );
}

template < typename TYPE, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void TPool< TYPE, memoryClass, memoryPool >::Deinit()
{
	if ( void* memory = m_pool.GetMemory() )
	{
		RED_MEMORY_FREE( memoryPool, memoryClass, memory );
		m_pool.Deinit();
	}
}

template < typename TYPE, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE void* operator new ( size_t, TPool< TYPE, memoryClass, memoryPool >& pool ) { return pool.Allocate(); }
template < typename TYPE, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
RED_FORCE_INLINE void operator delete ( void* ptr, TPool< TYPE, memoryClass, memoryPool >& pool ) { pool.Free( ptr ); }
