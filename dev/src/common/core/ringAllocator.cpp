#include "build.h"
#include "ringAllocator.h"

//----

CRingBufferBlock::CRingBufferBlock()
	: m_allocator( nullptr )
	, m_data( nullptr )
	, m_size( 0 )
	, m_next( nullptr )
{
}

CRingBufferBlock::~CRingBufferBlock()
{
}

void CRingBufferBlock::Setup( class CRingBufferAllocator* allocator, const Uint32 size, void* ptr )
{
	m_allocator = allocator;
	m_data = ptr;
	m_size = size;
}

void CRingBufferBlock::Link( CRingBufferBlock*& list )
{
	RED_FATAL_ASSERT( m_next == nullptr, "Trying to link already linked block" );
	m_next = list;
	list = this;
}

CRingBufferBlock* CRingBufferBlock::Unlink()
{
	CRingBufferBlock* next = m_next;
	m_next = nullptr;
	return next;
}

void CRingBufferBlock::Release()
{
	if ( m_allocator )
	{
		m_allocator->ReleaseBlock( this );
	}
	else if ( m_data )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_Default, m_data );
		m_data = nullptr;

		delete this;
	}
}

CRingBufferBlock* CRingBufferBlock::CreateManualBlock( const Uint32 size, const Uint32 alignment )
{
	void* mem = RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_Default, size, alignment );
	if ( !mem )
		return nullptr;

	CRingBufferBlock* block = new CRingBufferBlock();
	block->Setup( nullptr, size, mem );
	return block;
}

//----

CRingBufferAllocator::CRingBufferAllocator( )
	: m_buffer( nullptr )
	, m_size( 0 )
	, m_usedBlocks( nullptr )
	, m_numUsedBlocks( 0 )
	, m_numFlushes( 0 )
	, m_numAllocated( 0 )
	, m_maxAllocated( 0 )
{
	
}

void CRingBufferAllocator::Initialize( void* ptr, Uint32 size, Uint32 maxBlocks )
{
	m_buffer = ptr;
	m_size = size;

	// setup allocation
	m_allocPos = (Uint8*)ptr;
	m_endPos = (Uint8*)ptr + size;

	// allocate the blocks
	m_allBlocks = (CRingBufferBlock*) RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Default, sizeof(CRingBufferBlock) * maxBlocks );
	Red::MemoryZero( m_allBlocks, sizeof(CRingBufferBlock) * maxBlocks );

	// create the initial free list
	m_freeBlocks = nullptr;
	for ( Uint32 i=0; i<maxBlocks; ++i )
	{
		CRingBufferBlock* block = &m_allBlocks[i];
		block->Link( m_freeBlocks );
	}
}

CRingBufferAllocator::~CRingBufferAllocator()
{
	// free blocks
	if ( m_allBlocks )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_Default, m_allBlocks );
		m_allBlocks = nullptr;
	}

	m_freeBlocks = nullptr;
	m_size = 0;	
}

void CRingBufferAllocator::GetStats( Stats& outStats ) const
{
	Red::Threads::CScopedLock< Red::Threads::CSpinLock > lock( m_lock );

	outStats.m_maxAllocated = m_maxAllocated;
	outStats.m_numAllocated = m_numAllocated;
	outStats.m_numFlushes = m_numFlushes;
}

Bool CRingBufferAllocator::CanAllocateBlock( const Uint32 size, const Uint32 alignment ) const
{
	// note: we are taking the alignment of the m_buffer into consideration
	const Uint8* basePos = AlignPtr<Uint8>( (Uint8*) m_buffer, alignment );
	return ( basePos + size <= m_endPos );
}

CRingBufferBlock* CRingBufferAllocator::TryAllocateBlock( const Uint32 size, const Uint32 alignment )
{
	Red::Threads::CScopedLock< Red::Threads::CSpinLock > lock( m_lock );

	// to big
	if ( !CanAllocateBlock( size, alignment ) )
		return nullptr;

	// do we have space left ?
	Uint8* basePos = AlignPtr<Uint8>( m_allocPos, alignment );
	if ( basePos + size > m_endPos )
	{
		// we can't fit into rest of this buffer, if there are still buffers in use we cannot allocate now
		if ( m_numUsedBlocks > 0 )
			return nullptr;

		// we can allocate
		m_allocPos = AlignPtr<Uint8>( (Uint8*) m_buffer, alignment );
		basePos = m_allocPos;

		// stats
		m_numFlushes += 1;
	}

	// get free block
	CRingBufferBlock* block = m_freeBlocks;
	if ( !block )
		return nullptr;
	m_freeBlocks = block->Unlink();

	// prepare block info
	block->Setup( this, size, basePos );
	block->Link( m_usedBlocks );
	m_numUsedBlocks += 1;

	// update stats
	m_numAllocated += size;
	if ( m_numAllocated > m_maxAllocated )
		m_maxAllocated = m_numAllocated;

	// advance position
	m_allocPos = basePos + size;

	// return allocated block
	return block;
}

void CRingBufferAllocator::ReleaseBlock( CRingBufferBlock* block )
{
	Red::Threads::CScopedLock< Red::Threads::CSpinLock > lock( m_lock );

	RED_FATAL_ASSERT( block != nullptr, "Trying to release invalid block" );
	RED_FATAL_ASSERT( block->m_allocator == this, "Trying to release block from another allocator" );
	RED_FATAL_ASSERT( m_numUsedBlocks > 0, "Trying to release block when no blocks are allocated" );
	RED_FATAL_ASSERT( block->GetSize() <= m_numAllocated, "Trying to release block that is larger than currently allocated size" );

	m_usedBlocks = block->Unlink();
	block->Link( m_freeBlocks );

	m_numUsedBlocks -= 1;
	m_numAllocated -= block->GetSize(); 
}

//----
