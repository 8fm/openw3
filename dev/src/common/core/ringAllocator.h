#pragma once

#ifndef __RING_ALLOCATOR_H___
#define __RING_ALLOCATOR_H___

/// Allocated buffer from ring allocator
class CRingBufferBlock : public Red::NonCopyable
{
public:
	// get the buffer size
	RED_INLINE const Uint32 GetSize() const { return m_size; }

	// get the buffer data pointer
	RED_INLINE void* GetData() const { return m_data; }

	// release the block
	void Release();

	// Create manual block
	static CRingBufferBlock* CreateManualBlock( const Uint32 size, const Uint32 alignment );

private:
	CRingBufferBlock();
	~CRingBufferBlock();

	void Setup( class CRingBufferAllocator* allocator, const Uint32 size, void* ptr );
	void Link( CRingBufferBlock*& list );
	CRingBufferBlock* Unlink();

	class CRingBufferAllocator*		m_allocator;
	void*							m_data;
	Uint32							m_size;
	CRingBufferBlock*				m_next;

	friend class CRingBufferAllocator;
};

/// Very lame ring buffer based allocator
class CRingBufferAllocator
{
public:
	CRingBufferAllocator();
	~CRingBufferAllocator();

	struct Stats
	{
		Uint32		m_numAllocated;
		Uint32		m_maxAllocated;
		Uint32		m_numFlushes;
	};

	void Initialize( void* ptr, Uint32 size, Uint32 maxBlocks );

	// get maximum block size
	RED_INLINE const Uint32 GetMaxSize() const { return m_size; }

	// get buffer statistics
	void GetStats( Stats& outStats ) const;

	// can we allocate a block of given size from this allocator
	// this may fail due to the following conditions:
	//   - block size is to large (always fails)
	Bool CanAllocateBlock( const Uint32 size, const Uint32 alignment ) const;

	// allocate block of given size from this allocator
	// this may fail due to the following conditions:
	//   - block size is to large (always fails)
	//   - not enough free block entries (you should retry)
	//   - buffer flush required but there are still blocks in use (you should retry)
	CRingBufferBlock* TryAllocateBlock( const Uint32 size, const Uint32 alignment );

private:
	mutable Red::Threads::CSpinLock	m_lock;

	void*					m_buffer;
	Uint32					m_size;
	Uint8*					m_allocPos;
	Uint8*					m_endPos;

	CRingBufferBlock*		m_allBlocks;
	CRingBufferBlock*		m_freeBlocks;
	CRingBufferBlock*		m_usedBlocks;

	Uint32					m_numUsedBlocks;

	// statistics
	Uint32					m_numFlushes;
	Uint32					m_numAllocated;
	Uint32					m_maxAllocated;

	friend class CRingBufferBlock;
	void ReleaseBlock( CRingBufferBlock* block );
};

#endif