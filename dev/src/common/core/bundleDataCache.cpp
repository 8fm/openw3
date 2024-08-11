/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "bundleDataCache.h"
#include "../redThreads/redThreadsThread.h"
#include "../redThreads/redThreadsAtomic.h"
#include "ioBufferAcquireContext.h"

// Use this to fill recycled buffers with duff data 
//#define CLEAR_RECYCLED_BUFFERS

// Maximum amount we will afford to 'waste' from recycling an existing unused buffer
const Uint32 c_recycleBufferMaximumWaste = 1024u * 1024u;
const Int32 CBundleDataCache::MAX_CACHE_SIZE = 1024u * 1024u * 128u;

BundleBufferProxy::BundleBufferProxy( void * buffer, Uint32 size, CBundleDataCache * cache )
	:	m_buffer( buffer ),
		m_size( size ),
		m_bundleDataCache( cache )
{}

BundleBufferProxy::~BundleBufferProxy()
{
	if( m_bundleDataCache )
	{
		m_bundleDataCache->InternalReleaseBuffer( m_buffer );
	}
	else
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_ResourceBuffer, m_buffer );
	}
}

void * BundleBufferProxy::GetBuffer() const
{
	return m_buffer;
}

Uint32 BundleBufferProxy::GetSize() const
{
	return m_size;
}

//////////////////////////////////////////////////////////////////////////
CBundleDataCache::CBundleDataCache()
	:	m_targetBufferCount( 0 ),
		m_cacheMemorySize( 0 ),
		m_buffer( nullptr )
{}

CBundleDataCache::CBundleDataCache( Uint32 targetBufferCount )
	: m_targetBufferCount( targetBufferCount )
	, m_cacheMemorySize( 0 )
{
	m_writeBuffers.Reserve( targetBufferCount );
	m_completeBuffers.Reserve( targetBufferCount );
}

//////////////////////////////////////////////////////////////////////////
CBundleDataCache::~CBundleDataCache()
{
	RED_MEMORY_FREE( MemoryPool_Default, MC_ResourceBuffer, m_buffer );
}

//////////////////////////////////////////////////////////////////////////
Bool CBundleDataCache::NeedsFlush() const
{
	return ( m_writeBuffers.Size() + m_completeBuffers.Size() ) >= m_targetBufferCount;
}

//////////////////////////////////////////////////////////////////////////
CBundleDataBuffer* CBundleDataCache::CreateBufferInternal( Uint32 size, Uint32 flags, EBundleBufferType bufferType )
{
	m_cacheMemorySize += size;
	void* bufferMem = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_ResourceBuffer, size );
	return new CBundleDataBuffer( bufferMem, size, flags, bufferType );
}

//////////////////////////////////////////////////////////////////////////
void CBundleDataCache::GetIOBuffer( TDynArray< CBundleDataHandle >& dataHandles, const CIOBufferAcquireContext& acquireContext )
{
	RED_FATAL_ASSERT( acquireContext.GetBufferSpanCount() > 0, "Atleast one buffer span should be provided with each acquire context object." );
	RED_FATAL_ASSERT( acquireContext.GetSize() <= MAX_IO_BUFFER_SIZE, "IO Buffer request is too large %u, IO buffer size must be less than or equal to %u", acquireContext.GetSize(), MAX_IO_BUFFER_SIZE );
	if( m_ioBuffer == nullptr )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > scopeLock( m_readWriteMutex );
		m_ioBuffer = CreateBufferInternal( acquireContext.GetSize(), acquireContext.GetFlags(), BBT_IO );
	}

	const BufferSpanCollection& bufferSpans = acquireContext.GetBufferSpans();
	BufferSpanCollection::const_iterator spanIter = bufferSpans.Begin();
	BufferSpanCollection::const_iterator spanIterEnd = bufferSpans.End();
	while( spanIter != spanIterEnd )
	{
		const CBufferSpan& bufferSpan = (*spanIter);
		const Uint32 start = bufferSpan.GetStart();
		dataHandles.PushBack( CBundleDataHandle( m_ioBuffer, start ) );
		++spanIter;
	}
}

void CBundleDataCache::FinalizeIOBuffer( CBundleDataHandle& ioBuffer )
{
	CBundleDataBuffer* dataBuffer = ioBuffer.GetBufferInternal();
	RED_FATAL_ASSERT( dataBuffer->GetBufferType() == BBT_IO, "Only IO Buffers should be finalized in this manner" );
	if( dataBuffer->Release() == 0 )
	{
		if ( dataBuffer->AddRef() == 1 )
		{
			Uint32 refcount = dataBuffer->Release();
			RED_ASSERT( refcount == 0, TXT("increment while deleting?") );
			RED_UNUSED( refcount );
			m_cacheMemorySize -= dataBuffer->GetSize();
			void* bufferMem = dataBuffer->GetDataBuffer();
			delete dataBuffer;
			RED_MEMORY_FREE( MemoryPool_Default, MC_ResourceBuffer, bufferMem );
			ioBuffer = CBundleDataHandle();
			m_ioBuffer = nullptr;
		}
	}
	else
	{
		dataBuffer->AddRef();
		ioBuffer = CBundleDataHandle();
	}
}

void CBundleDataCache::GetWriteBuffers( TDynArray< CBundleDataHandle >& dataHandles, const TDynArray< SWriteBufferAcquireContext >& acquireContext )
{
	const Uint32 contextCount = acquireContext.Size();
	dataHandles.Reserve( contextCount );

	Red::Threads::CScopedLock< Red::Threads::CMutex > scopeLock( m_readWriteMutex );
	for( Uint32 contextIndex = 0; contextIndex < contextCount; ++contextIndex )
	{
		dataHandles.PushBack( CreateWriteBuffer( acquireContext[ contextIndex ] ) );
	}
}

CBundleDataHandle CBundleDataCache::GetWriteBuffer( const SWriteBufferAcquireContext& acquireContext )
{
	if( NeedsFlush() )
	{
		// First, attempt to recycle an existing buffer
		CBundleDataHandle recycledBuffer = AttemptBufferRecycle( acquireContext.m_requiredSize, acquireContext.m_flags );
		if( recycledBuffer.GetBufferInternal() != nullptr )
		{
			return recycledBuffer;
		}

		// Couldn't recycle; flush 
		FlushUnused();
	}

	Red::Threads::CScopedLock< Red::Threads::CMutex > scopeLock( m_readWriteMutex );
	return CreateWriteBuffer( acquireContext );
}

CBundleDataHandle CBundleDataCache::CreateWriteBuffer( const SWriteBufferAcquireContext& acquireContext )
{
	// Create new buffer regardless of result of FlushUnused(), then add it to the writing list
	CBundleDataBuffer* newWriteBuffer = CreateBufferInternal( acquireContext.m_requiredSize, acquireContext.m_flags );
	RED_FATAL_ASSERT( newWriteBuffer, "Failed to create buffer for bundle read" );

	m_writeBuffers.Grow();
	CBundleDataBuffer*& entry = m_writeBuffers.Last();
	entry = newWriteBuffer;

	CBundleDataHandle handle( newWriteBuffer );
	return handle;
}

void CBundleDataCache::RegisterSingleFileBuffer( CBundleDataHandle& bufferHandle, const String& filename, Uint32 bundleOffset, Uint32 size, Uint32 bufferOffset )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > scopeLock( m_readWriteMutex );
	// Extract the buffer from the handle
	CBundleDataBuffer* dataBuffer = bufferHandle.GetBufferInternal();

	// Remove the buffer from the writing list
	m_writeBuffers.RemoveFast( dataBuffer );

	// Push it to the read list
	m_completeBuffers.Grow();
	SBundleDataCacheEntry& entry = m_completeBuffers.Last();
	entry.m_bufferData = dataBuffer;
	entry.m_id = filename;
	entry.m_readOffset = bundleOffset;
	entry.m_readSize = size;
	entry.m_bufferOffset = bufferOffset;
}

void CBundleDataCache::RegisterBufferEntry( CBundleDataHandle& bufferHandle, const String& filename, const Uint32 bundleOffset, const Uint32 size, const Uint32 bufferOffset )
{
    Red::Threads::CScopedLock< Red::Threads::CMutex > scopeLock( m_readWriteMutex );
	
	CBundleDataBuffer* dataBuffer = bufferHandle.GetBufferInternal();

	// Push it to the read list
	m_completeBuffers.Grow();
	SBundleDataCacheEntry& entry = m_completeBuffers.Last();
	entry.m_bufferData = dataBuffer;
	entry.m_id = filename;
	entry.m_readOffset = bundleOffset;
	entry.m_readSize = size;
	entry.m_bufferOffset = bufferOffset;
}

void CBundleDataCache::FinalizeWriteBuffer( CBundleDataHandle& bufferHandle )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > scopeLock( m_readWriteMutex );
	// Extract the buffer from the handle
	CBundleDataBuffer* dataBuffer = bufferHandle.GetBufferInternal();
	// Remove the buffer from the writing list
	m_writeBuffers.RemoveFast( dataBuffer );
}

Bool CBundleDataCache::HasBuffer( const String& filename, Uint32 offset, Uint32 size )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > scopeLock( m_readWriteMutex );
	for ( Uint32 i = 0; i < m_completeBuffers.Size(); ++i )
	{
		if ( m_completeBuffers[i].m_id == filename )
		{
			Uint32 bufferOffset = m_completeBuffers[i].m_readOffset;
			Uint32 bufferSize = m_completeBuffers[i].m_readSize;
			if ( bufferOffset <= offset && bufferOffset + bufferSize >= offset + size )
			{
				return true;
			}
		}
	}

	return false;
}

CBundleDataHandle CBundleDataCache::GetReadBuffer( const String& filename, Uint32 offset, Uint32 size )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > scopeLock( m_readWriteMutex );
	CBundleDataBuffer* buffer = nullptr;
	Uint32 bufferOffset = 0;
	Uint32 internalBufferOffset = 0;
	for ( Uint32 i = 0; i < m_completeBuffers.Size(); ++i )
	{
		if ( m_completeBuffers[i].m_id == filename )
		{
			bufferOffset = m_completeBuffers[i].m_readOffset;
			internalBufferOffset = m_completeBuffers[i].m_bufferOffset;
			Uint32 bufferSize = m_completeBuffers[i].m_readSize;

			if ( bufferOffset <= offset && bufferOffset + bufferSize >= offset + size )
			{
				buffer = m_completeBuffers[i].m_bufferData;
				break;
			}
		}
	}

	// Create a handle, using the correct local offset
	return ( buffer != nullptr )? CBundleDataHandle( buffer, ( offset - bufferOffset ) + internalBufferOffset ) : CBundleDataHandle();
}

// Try to find an existing cached buffer that matches the buffer params
CBundleDataHandle CBundleDataCache::AttemptBufferRecycle( Uint32 requiredSize, Uint32 flags )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > scopeLock( m_readWriteMutex );
	for ( Uint32 i = 0; i < m_completeBuffers.Size(); ++i )
	{
		CBundleDataBuffer* theBuffer = m_completeBuffers[i].m_bufferData;
		if ( theBuffer->AddRef() == 1 )
		{
			Uint32 refcount = theBuffer->Release();
			RED_ASSERT( refcount == 0, TXT("increment while recycling?") );
			RED_UNUSED( refcount );
			
			Uint32 bufferSize = theBuffer->GetSize();
			Uint32 bufferFlags = theBuffer->GetFlags();
			Bool flagsMatch = bufferFlags == flags;
			Bool sizesCompatible = ( bufferSize >= requiredSize ) && ( ( bufferSize - requiredSize ) < c_recycleBufferMaximumWaste );
			if( flagsMatch && sizesCompatible )
			{
#ifdef CLEAR_RECYCLED_BUFFERS
				Red::System::MemorySet( theBuffer->GetDataBuffer(), 0xef, bufferSize );
#endif
				m_completeBuffers.RemoveAtFast( i );
				m_writeBuffers.PushBack( theBuffer );
				return CBundleDataHandle( theBuffer );
			}
		}
		else
		{
			theBuffer->Release();
		}
	}
	return CBundleDataHandle();
}

void CBundleDataCache::FlushUnused()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > scopeLock( m_readWriteMutex );
	TDynArray< CBundleDataBuffer* > deleteBuffers;
	deleteBuffers.Reserve( m_completeBuffers.Size() );
	// evict complete buffers that are not used anymore.
	for ( Int32 i = ( m_completeBuffers.Size() - 1 ); i >= 0; --i )
	{
		if ( m_completeBuffers[i].m_bufferData->AddRef() == 1 )
		{
			Uint32 refcount = m_completeBuffers[i].m_bufferData->Release();
			RED_ASSERT( refcount == 0, TXT("increment while deleting?") );
			RED_UNUSED( refcount );

			void* bufferMem = m_completeBuffers[i].m_bufferData->GetDataBuffer();
			if( bufferMem != nullptr )
			{
				m_cacheMemorySize -= m_completeBuffers[i].m_bufferData->GetSize();
				RED_MEMORY_FREE( MemoryPool_Default, MC_ResourceBuffer, bufferMem );
				m_completeBuffers[i].m_bufferData->InvalidateDataBuffer();
				deleteBuffers.PushBackUnique( m_completeBuffers[i].m_bufferData );
			}
			m_completeBuffers.RemoveAtFast(i);
		}
		else
		{
			// Make sure we decrement the ref count again
			m_completeBuffers[i].m_bufferData->Release();
		}
	}

	for( Uint32 i = 0; i < deleteBuffers.Size(); ++i )
	{
		delete deleteBuffers[i];
	}
}

Bool CBundleDataCache::IsCacheFull( Uint32 bufferSize )
{
	RED_FATAL_ASSERT( bufferSize <= MAX_CACHE_SIZE, "Write Buffer is too large, and will lock the thread!" );
	Red::Threads::CScopedLock< Red::Threads::CMutex > scopeLock( m_readWriteMutex );
	return ( ( m_cacheMemorySize - MAX_IO_BUFFER_SIZE ) + bufferSize ) > MAX_CACHE_SIZE;
}

void CBundleDataCache::Initialize( Uint32 singleBufferSize, Uint32 bufferCount )
{
	Red::System::MemoryZero( &m_metrics, sizeof( m_metrics ) );
	m_buffer = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_ResourceBuffer, bufferCount * singleBufferSize );
	m_bufferPool.Init( m_buffer, bufferCount * singleBufferSize, singleBufferSize );
	
}

BundleDataHandle CBundleDataCache::AcquireBuffer( Uint32 size )
{
	++m_metrics.acquireBufferCount;

	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > scopeLock( m_readWriteMutex );

		if( size <= m_bufferPool.GetBlockSize() && m_bufferPool.HasFreeBlocks() )
		{
			void * buffer = m_bufferPool.AllocateBlock();
			return BundleDataHandle( new BundleBufferProxy( buffer, size, this ) );
		}
	}

	size > m_bufferPool.GetBlockSize() ? ++m_metrics.acquireBufferFailureSizeTooBig : ++m_metrics.acquireBufferFailureNoBufferAvailable;

	return BundleDataHandle( new BundleBufferProxy( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_ResourceBuffer, size ), size, nullptr ) );
}

void CBundleDataCache::InternalReleaseBuffer( void * buffer )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > scopeLock( m_readWriteMutex );
	m_bufferPool.FreeBlock( buffer );
}

const CBundleDataCache::Metrics & CBundleDataCache::GetMetrics() const
{
	return m_metrics;
}
