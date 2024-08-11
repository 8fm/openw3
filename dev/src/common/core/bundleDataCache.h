/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "..//redSystem//utility.h"
#include "../redThreads/redThreadsThread.h"
#include "../redThreads/redThreadsAtomic.h"

#include "bundleDataHandle.h"
#include "bufferSpan.h"
#include "pool.h"
#include "atomicSharedPtr.h"

class CIOBufferAcquireContext;
struct SWriteBufferAcquireContext
{
	SWriteBufferAcquireContext()
		: m_requiredSize( 0 )
		, m_flags( 0 )
	{

	}
	
	SWriteBufferAcquireContext( Uint32 size, Uint32 flags = 0 )
		: m_requiredSize( size )
		, m_flags( flags )
	{
	}

	SWriteBufferAcquireContext( SWriteBufferAcquireContext&& acquireContext )
		: m_requiredSize( acquireContext.m_requiredSize )
		, m_flags( acquireContext.m_flags )
	{
		acquireContext.m_requiredSize = 0;
		acquireContext.m_flags = 0;
	}

	 Uint32 m_requiredSize;
	 Uint32 m_flags;
};

class CBundleDataCache;

class BundleBufferProxy
{
public:

	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_Engine );

	BundleBufferProxy( void * buffer, Uint32 size, CBundleDataCache * cache );
	~BundleBufferProxy();

	void * GetBuffer() const;
	Uint32 GetSize() const;

private:

	void * m_buffer;
	Uint32 m_size;
	CBundleDataCache * m_bundleDataCache;
};

typedef Red::TAtomicSharedPtr< BundleBufferProxy > BundleDataHandle; 

class CBundleDataCache
{
public:

	const static Uint32 MAX_IO_BUFFER_SIZE = ( 1024 * 1024 ) * 64;

	CBundleDataCache();
	CBundleDataCache( Uint32 targetBufferCount );
	~CBundleDataCache();

	// Once a buffer is registered, it is considered to be read-only, and representative of a portion of a bundle file
	void RegisterSingleFileBuffer( CBundleDataHandle& bufferHandle, const String& filename, Uint32 bundleOffset, Uint32 size, Uint32 bufferOffset = 0 );

	// Register only the buffer entry
	void RegisterBufferEntry( CBundleDataHandle& bufferHandle, const String& filename, const Uint32 bundleOffset, const Uint32 size, const Uint32 bufferOffset );

	// Finialize the write buffer
	void FinalizeWriteBuffer( CBundleDataHandle& bufferHandle );

	// Fills out the array of data handles, to the specification of the acquire context.
	void GetIOBuffer( TDynArray< CBundleDataHandle >& dataHandles, const CIOBufferAcquireContext& acquireContext );

	void FinalizeIOBuffer( CBundleDataHandle& ioBuffer );

	// Request a collection of buffers all at once.
	void GetWriteBuffers( TDynArray< CBundleDataHandle >& dataHandles, const TDynArray< SWriteBufferAcquireContext >& acquireContext );

	// Request a buffer to write into. MUST be registered on completion
	CBundleDataHandle GetWriteBuffer( const SWriteBufferAcquireContext& acquireContext ); //*Uint32 requiredSize, Uint32 flags );

	// Read buffer accessors
	Bool HasBuffer( const String& filename, Uint32 offset, Uint32 size );
	CBundleDataHandle GetReadBuffer( const String& filename, Uint32 offset, Uint32 size );

	// flush everything that has 0 refcount already
	void FlushUnused(); 

	Bool IsCacheFull( Uint32 bufferSize );

	// ctremblay, BEGIN refactoring in progress
	struct Metrics
	{
		Uint32 acquireBufferCount;
		Uint32 acquireBufferFailureSizeTooBig;
		Uint32 acquireBufferFailureNoBufferAvailable;
	};

	void Initialize( Uint32 singleBufferSize, Uint32 bufferCount );
	BundleDataHandle AcquireBuffer( Uint32 size );
	void InternalReleaseBuffer( void * buffer );	
	const Metrics & GetMetrics() const;
	// ctremblay, END refactoring in progress

private:
	const static Int32 MAX_CACHE_SIZE;
	struct SBundleDataCacheEntry
	{
		String m_id;
		Uint32 m_readOffset;		// Offset into the bundle where the buffer data starts
		Uint32 m_readSize;			// Amount actually read into the buffer (we may not read entire buffers worth of data)
		Uint32 m_bufferOffset;
		CBundleDataBuffer* m_bufferData;
	};

	Bool NeedsFlush() const;
	CBundleDataBuffer* CreateBufferInternal( Uint32 size, Uint32 flags, EBundleBufferType bufferType = BBT_NONE );
	CBundleDataHandle AttemptBufferRecycle( Uint32 requiredSize, Uint32 flags );				// attempt to recycle an unused buffer in the read cache
	CBundleDataHandle CreateWriteBuffer( const SWriteBufferAcquireContext& acquireContext );

	Red::Threads::CMutex														m_readWriteMutex;
	CBundleDataBuffer*															m_ioBuffer;
	TDynArray< CBundleDataBuffer*, MC_ResourceBuffer, MemoryPool_Default >		m_writeBuffers;			// buffers open for writing
	TDynArray< SBundleDataCacheEntry, MC_ResourceBuffer, MemoryPool_Default >	m_completeBuffers;		// buffers ready for reads
	const Uint32																m_targetBufferCount;	// this is not a hard limit because we can exceed it but if we do, we will constantly try to free up buffers until we get below this
	Uint32																		m_cacheMemorySize;

	// ctremblay, refactoring in progress
	CPool m_bufferPool;
	void * m_buffer;
	Metrics m_metrics;
};