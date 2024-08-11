#include "build.h"
#include "pool.h"

CPool::CPool()
	:	m_memory( nullptr ),
		m_numBlocks( 0 ),
		m_blockSize( 0 ),
		m_firstFreeBlock( nullptr )
{}

CPool::CPool( CPool && value )
	:	m_memory( std::move( value.m_memory ) ),
		m_numBlocks( std::move( value.m_numBlocks ) ),
		m_blockSize( std::move( value.m_blockSize ) ),
		m_firstFreeBlock( std::move( value.m_firstFreeBlock ) )
{
	value.m_memory = nullptr;
	value.m_numBlocks = 0;
	value.m_blockSize = 0;
	value.m_firstFreeBlock = nullptr;
}

void CPool::Init( void* memory, Uint32 size, Uint32 blockSize )
{
	RED_FATAL_ASSERT( blockSize >= sizeof( FreeListEntry ), "" );
	RED_FATAL_ASSERT( memory || !size, "Size > 0 requires valid memory" );

	m_memory = memory;
	m_blockSize = blockSize;
	m_numBlocks = size / blockSize;
	m_firstFreeBlock = nullptr;

	Clear();
}

void CPool::Deinit()
{
	m_memory = nullptr;
	m_blockSize = 0;
	m_numBlocks = 0;
	m_firstFreeBlock = nullptr;
}

void CPool::Clear()
{
	if ( !m_memory )
	{
		return;
	}

	// Arrange all blocks into singly linked list

	FreeListEntry* prevBlock = m_firstFreeBlock = ( FreeListEntry* ) GetBlock( 0 );
	for ( Uint32 i = 1; i < m_numBlocks; i++ )
	{
		FreeListEntry* block = ( FreeListEntry* ) GetBlock( i );
		prevBlock->m_next = block;
		prevBlock = block;
	}
	prevBlock->m_next = nullptr;
}

void* CPool::AllocateBlock()
{
	if ( !m_firstFreeBlock )
	{
		return nullptr;
	}
	FreeListEntry* allocatedBlock = m_firstFreeBlock;
	m_firstFreeBlock = m_firstFreeBlock->m_next;
	return allocatedBlock;
}

Uint32 CPool::AllocateBlockIndex()
{
	if ( !m_firstFreeBlock )
	{
		return INVALID_INDEX;
	}
	FreeListEntry* allocatedBlock = m_firstFreeBlock;
	m_firstFreeBlock = m_firstFreeBlock->m_next;
	return ( Uint32 ) ( ( ( const Uint8* ) allocatedBlock - ( const Uint8* ) m_memory ) / m_blockSize );
}

void CPool::FreeBlock( void* ptr )
{
	RED_FATAL_ASSERT( m_memory <= ptr && ptr < ( const Uint8* ) m_memory + m_numBlocks * m_blockSize, "" );
	FreeListEntry* releasedBlock = ( FreeListEntry* ) ptr;
	releasedBlock->m_next = m_firstFreeBlock;
	m_firstFreeBlock = releasedBlock;
}

void CPool::FreeBlockIndex( Uint32 index )
{
	FreeBlock( GetBlock( index ) );
}

CPool & CPool::operator = ( CPool&& other )
{
	CPool( std::move( other ) ).Swap( *this );
	return *this;
}

void CPool::Swap( CPool & value )
{
	::Swap( m_memory, value.m_memory );
	::Swap( m_numBlocks, value.m_numBlocks );
	::Swap( m_blockSize, value.m_blockSize );
	::Swap( m_firstFreeBlock, value.m_firstFreeBlock );
}

Uint32 CPool::GetBlockSize() const
{
	return m_blockSize;
}
