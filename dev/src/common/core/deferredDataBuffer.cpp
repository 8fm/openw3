/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "deferredDataBuffer.h"
#include "deferredDataBufferLoaders.h"
#include "deferredDataBufferAsync.h"
#include "deferredDataBufferOOMQueue.h"
#include "fileLatentLoadingToken.h"
#include "fileSys.h"
#include "profiler.h"
#include "configVar.h"

namespace Config
{
	TConfigVar< Bool >		cvForceImmediateDeferredData( "ResourceLoading", "ForceImmediateDeferredData", false );
}

IMPLEMENT_SIMPLE_RTTI_TYPE( DeferredDataBuffer );

void * DefaultAllocate( Red::MemoryFramework::MemoryClass memClass, Uint32 size, Uint16 alignment )
{
	return RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, memClass, size, alignment );
}

void DefaultDeallocate( Red::MemoryFramework::MemoryClass memClass, void * ptr )
{
	RED_MEMORY_FREE( MemoryPool_Default, memClass, ptr );
}

const AllocateFunction defaultAllocateFunction = []( Uint32 size, Uint16 alignment ){ return DefaultAllocate( MC_DataBlob, size, alignment ); };
const DeallocateFunction defaultDeallocateFunction = []( void * ptr ){ DefaultDeallocate( MC_DataBlob, ptr ); };

const Uint16 DeferredDataBuffer::DEFAULT_ALIGNMNET = 16;

bool DeferredDataBuffer::st_immediateAsyncLoading = false;

//---------------------------------------------------------

BufferAsyncData::BufferAsyncData()
	: m_refCount( 1 )
	, m_inOOMQueue( false )
{
}

BufferAsyncData::~BufferAsyncData()
{
	RED_FATAL_ASSERT( m_inOOMQueue.GetValue() == 0, "Trying to destroy a async request that is still in the OOM queue" );
	RED_FATAL_ASSERT( m_refCount.GetValue() == 0, "Invalid ref count" );
}

void BufferAsyncData::AddRef()
{
	m_refCount.Increment();
}

Uint32 BufferAsyncData::Release()
{
	return m_refCount.Decrement();
}

Bool BufferAsyncData::ResetOOMFlag()
{
	return m_inOOMQueue.Exchange( false ) == true;
}

Bool BufferAsyncData::RegisterInOOMQueue()
{
	if ( m_inOOMQueue.Exchange( true ) == false )
	{
		SDeferredDataOOMQueue::GetInstance().Register( this ); // this addrefs
		return true;
	}

	return false;
}

//---------------------------------------------------------

BufferProxy::BufferProxy( void * buffer, Uint32 size, const DeallocateFunction & dealloc )
	:	m_buffer( buffer ),
		m_size( size ),
		m_dealloc( dealloc )
{}

BufferProxy::~BufferProxy()
{
	m_dealloc( m_buffer );
}

void * BufferProxy::GetData() const 
{ 
	return m_buffer; 
}

Uint32 BufferProxy::GetSize() const 
{ 
	return m_size; 
}

//---------------------------------------------------------

BufferHandle BufferAsyncData::WaitForData() const
{
	PC_SCOPE(BufferAsyncDataWait);

	// wait for the data to be there
	BufferHandle data;
	for ( ;; )
	 {
		const auto ret = GetData( data );
		if ( ret != eResult_NotReady )
			break;

		Red::Threads::YieldCurrentThread(); 
	}

	return data;
}

//---------------------------------------------------------

DeferredDataBuffer::DeferredDataBuffer()
	:	m_size( 0 ),
		m_alloc( defaultAllocateFunction ),
		m_dealloc( defaultDeallocateFunction ),
		// HACK++
		m_wasDeserializedFromFileHACK(false),
		m_saveID( -1 )
		// HACK--
{}

DeferredDataBuffer::~DeferredDataBuffer()
{
}

bool DeferredDataBuffer::IsValid() const
{
	return (m_size != 0);
}

bool DeferredDataBuffer::IsLoaded() const
{
	return m_buffer.GetRefCount() != 0;
}

void DeferredDataBuffer::SetMemoryClass( Red::MemoryFramework::MemoryClass memClass )
{
	m_alloc = [=]( Uint32 size, Uint16 alignment ){ return DefaultAllocate( memClass, size, alignment ); };
	m_dealloc = [=]( void * ptr ){ DefaultDeallocate( memClass, ptr ); };
}

void DeferredDataBuffer::SetAllocateFunction( const AllocateFunction & value )
{
	m_alloc = value;
}

void DeferredDataBuffer::SetDeallocateFunction( const DeallocateFunction & value )
{
	m_dealloc = value;
}

void DeferredDataBuffer::SerializeDirectlyAsRawBuffer( IFile& file )
{
	if ( file.IsGarbageCollector() )
		return;

	// dump the content of the deferred data buffer directly - this way it can be preserved even on copy-paste :)

	if ( file.IsReader() )
	{
		Uint32 size = 0;
		file << size;

		if ( size )
		{
			ReallocateBuffer( size );

			// load data
			BufferHandle handle = AcquireBufferHandleForWritingSync();
			if ( handle && handle->GetData() )
			{
				file.Serialize( handle->GetData(), size );
			}
			else
			{
				file.Seek( file.GetOffset() + size );
			}
		}
		else
		{
			Clear();
		}
	}
	else
	{
		BufferHandle data = AcquireBufferHandleSync();

		if ( data && data->GetData() )
		{
			Uint32 size = GetSize();
			file << size;

			file.Serialize( data->GetData(), size );
		}
		else
		{
			Uint32 zero = 0;
			file << zero;
		}
	}
}

BufferHandle DeferredDataBuffer::AcquireBufferHandleSync() const
{
	BufferHandle handle;
	{
		ScopedReadLock scopedRead( m_lock );
		handle = m_buffer.Lock();
	}
	
	if( !handle && m_size != 0 )
	{
		handle = ReadBufferSync();
	}

	return handle;
}

BufferHandle DeferredDataBuffer::AcquireBufferHandleForWritingSync()
{
	{
		ScopedReadLock scopedRead( m_lock );
		m_bufferWriting = m_buffer.Lock();
	}

	if( !m_bufferWriting && m_size != 0 )
	{
		m_bufferWriting = ReadBufferSync();
	}

	return m_bufferWriting;
}

BufferHandle DeferredDataBuffer::ReadBufferSync() const
{
	BufferHandle handle;
	{
		ScopedReadLock scopedLock( m_lock );
		handle = m_buffer.Lock();
	}

	if ( !handle && m_access && m_size>0 )
	{
		DeferredDataAccess::SyncAccess info;
		info.m_alignment = DEFAULT_ALIGNMNET;
		info.m_size = m_size;
		info.m_alloc = m_alloc;
		info.m_delloc = m_dealloc;

		handle = m_access->LoadSync( info );

		ScopedWriteLock scopedLock( m_lock );
		BufferHandle alreadyAssignedhandle = m_buffer.Lock();
		if( alreadyAssignedhandle )
		{
			return alreadyAssignedhandle;
		}	

		m_buffer = handle;
	}

	return handle;
}

BufferAsyncDataHandle DeferredDataBuffer::AcquireBufferHandleAsync( Uint8 ioTag ) const
{
	// cooking mode, create sync data
	if ( st_immediateAsyncLoading  || Config::cvForceImmediateDeferredData.Get() )
	{
		BufferAsyncDataHandle ret;
		BufferHandle handle = AcquireBufferHandleSync();
		ret.Reset( new BufferAsyncData_SyncData( handle, m_size ) );
		return ret;
	}

	BufferHandle handle;
	{
		ScopedReadLock scopedLock( m_lock );
		handle = m_buffer.Lock();
	}

	// we already have sync data, use it
	if ( handle )
	{
		BufferAsyncDataHandle ret;
		ret.Reset( new BufferAsyncData_SyncData( handle, m_size ) );
		return ret;
	}

	// create async access
	BufferAsyncDataHandle ret;
	if ( m_access )
	{
		DeferredDataAccess::AsyncAccess info;
		info.m_alignment = DEFAULT_ALIGNMNET;
		info.m_alloc = m_alloc;
		info.m_delloc = m_dealloc;
		info.m_size = m_size;
		info.m_ioTag = ioTag;

		// request async access to data
		ret = m_access->LoadAsync( info );
	}

	return ret;
}

BufferAsyncDataHandle DeferredDataBuffer::AcquireBufferHandleAsync( Uint8 ioTag, const BufferAsyncCallback& callback ) const
{
	return AcquireBufferHandleAsync( ioTag, callback, m_alloc, m_dealloc );
}

BufferAsyncDataHandle DeferredDataBuffer::AcquireBufferHandleAsync( Uint8 ioTag, const BufferAsyncCallback& callback, const AllocateFunction& allocFunc, const DeallocateFunction& deallocFunc) const
{
	// cooking mode, create sync data
	if ( st_immediateAsyncLoading || Config::cvForceImmediateDeferredData.Get() )
	{
		BufferAsyncDataHandle ret;
		BufferHandle handle = AcquireBufferHandleSync();
		ret.Reset( new BufferAsyncData_SyncData( handle, m_size ) );

		// call callback now
		if ( callback )
		{
			callback( handle );
		}

		return ret;
	}

	BufferHandle handle;
	{
		ScopedReadLock scopedLock( m_lock );
		handle = m_buffer.Lock();
	}

	// we already have sync data, use it
	if ( handle )
	{
		BufferAsyncDataHandle ret;
		ret.Reset( new BufferAsyncData_SyncData( handle, m_size ) );

		// call callback now
		if ( callback )
		{
			callback( handle );
		}

		return ret;
	}

	// create async access
	BufferAsyncDataHandle ret;
	if ( m_access )
	{
		DeferredDataAccess::AsyncAccess info;
		info.m_alignment = DEFAULT_ALIGNMNET;
		info.m_alloc = allocFunc;
		info.m_delloc = deallocFunc;
		info.m_callback = callback;
		info.m_size = m_size;
		info.m_ioTag = ioTag;

		// request async access to data
		ret = m_access->LoadAsync( info );
	}

	return ret;
}

void DeferredDataBuffer::SetBufferContent( const void * inputBuffer, Uint32 size )
{
	if ( size == 0 )
	{
		Clear();
	}
	else
	{
		ScopedWriteLock scopedLock( m_lock );

		m_size = size;
		m_bufferWriting.Reset();
		void * buffer = AllocateBuffer();
		Red::System::MemoryCopy( buffer, inputBuffer, m_size );
		m_bufferWriting.Reset( new BufferProxy( buffer, m_size, m_dealloc ) );
		m_buffer = m_bufferWriting;
	}
}

void DeferredDataBuffer::Clear()
{
	ScopedWriteLock scopedLock( m_lock );
	m_size = 0;
	m_access.Reset();
	m_bufferWriting.Reset();
	m_buffer.Reset();
}

void DeferredDataBuffer::ReallocateBuffer( Uint32 size )
{
	ScopedWriteLock scopedLock( m_lock );
	m_size = size;
	m_bufferWriting.Reset();
	void * buffer = AllocateBuffer();
	m_bufferWriting.Reset( new BufferProxy( buffer, m_size, m_dealloc ) );
	m_buffer = m_bufferWriting;
}

void * DeferredDataBuffer::AllocateBuffer() const
{
	return m_alloc( m_size, DEFAULT_ALIGNMNET );
}

DeferredDataBuffer & DeferredDataBuffer::operator=( const DeferredDataBuffer &other )
{
	if( &other != this )
	{
		m_size = other.m_size;
		m_access.Reset( other.m_access ? other.m_access->Clone() : nullptr );
		m_alloc = other.m_alloc;
		m_dealloc = other.m_dealloc;
		m_buffer = other.m_buffer;
		m_bufferWriting = other.m_bufferWriting;
		// HACK++
		m_wasDeserializedFromFileHACK = other.m_wasDeserializedFromFileHACK;
		m_saveID = -1;
		// HACK--

	}

	return *this;
}

bool operator==( const DeferredDataBuffer & /*left*/, const DeferredDataBuffer & /*right*/ )
{
	return false;
} 

void DeferredDataBuffer::SetAccessFromLatentToken( const Uint32 bufferSize, IFileLatentLoadingToken* token )
{
	RED_FATAL_ASSERT( token != nullptr, "Unable to create latent access to file data" );

	ScopedWriteLock lock( m_lock );

	m_buffer.Reset();
	m_bufferWriting.Reset();
	m_access.Reset( new DeferredDataAccess_LatentToken( token ) );
	m_size = bufferSize;
}

Red::TAtomicSharedPtr< DeferredDataAccessPatchable > DeferredDataBuffer::SetAccessFromLatentTokenForLaterPatching()
{
	Red::TAtomicSharedPtr< DeferredDataAccessPatchable > access( new DeferredDataAccess_LatentTokenPatchable() );
	ScopedWriteLock lock( m_lock );
	m_access = access;

	return access;
}

void DeferredDataBuffer::SetAccessFromBundleFile( const Uint32 bufferSize, const Red::Core::Bundle::FileID fileId )
{
	RED_FATAL_ASSERT( fileId != 0, "Unable to create access to defered buffer data with an non existing buffer file" );

	ScopedWriteLock lock( m_lock );

	m_buffer.Reset();
	m_bufferWriting.Reset();
	m_access.Reset( new DeferredDataAccess_BundledFile( fileId, bufferSize ) );
	m_size = bufferSize;
}

void DeferredDataBuffer::SetAccessFromFileData( const Uint32 bufferSize, const Uint64 dataOffset, IFile& fileToReadFrom )
{
	RED_FATAL_ASSERT( bufferSize != 0, "Unable to create access to defered buffer data from a zero sized file" );

	ReallocateBuffer( bufferSize );

	// reset access - file is initialized directly
	{
		ScopedWriteLock lock( m_lock );
		m_access.Reset();
	}

	// acquire buffer for writing
	BufferHandle buffer = AcquireBufferHandleForWritingSync();
	if ( buffer && buffer->GetData() )
	{
		// keep the current offset so we can restore it after reading the content
		const Uint64 savedOffset = fileToReadFrom.GetOffset();

		// load the content
		fileToReadFrom.Seek( dataOffset );
		fileToReadFrom.Serialize( buffer->GetData(), buffer->GetSize() );

		// move back
		fileToReadFrom.Seek( savedOffset );
	}
}

void DeferredDataBuffer::SetAccessFromPhysicalFile( const Uint32 bufferSize, const String& depotPath )
{
	RED_FATAL_ASSERT( !depotPath.Empty(), "Unable to create access to defered buffer data from a empty physical file" );
	RED_FATAL_ASSERT( !GFileManager->IsReadOnly(), "Cannot set direct physical access in read only file system" );

	m_buffer.Reset();
	m_bufferWriting.Reset();
	m_access.Reset( new DeferredDataAccess_PhysicalFile( depotPath, bufferSize) );
	m_size = bufferSize;
}

void SerializeDeferredDataBufferFromLatentLoaderData( IFile& file, DeferredDataBuffer & buffer )
{
	if( file.IsReader() )
	{
		const Uint64 position = file.GetOffset();

		// read buffer data size
		Uint32 skipOffset = 0;
		file << skipOffset;

		// get current position in file
		const Uint32 bufferSize = skipOffset ? static_cast< Uint32 >( skipOffset - sizeof( Uint32 ) ) : 0;
		if ( bufferSize )
		{
			// setup buffer access point
			IFileLatentLoadingToken* token = file.CreateLatentLoadingToken( static_cast< Uint32 >( position ) );
			buffer.SetAccessFromLatentToken( bufferSize, token );
		}
		else
		{
			// no data
			buffer.Clear();
		}

		// skip to the end of the data
		file.Seek( position + skipOffset );
	}
	else if ( file.IsWriter() )
	{
		RED_FATAL( "This path is not supported for writing" );
	}
}

void SerializeDeferredDataBufferAsLatentDataBufferData( IFile & file, DeferredDataBuffer & buffer )
{
	if( file.IsReader() )
	{
		// get data buffer size
		Uint32 bufferSize = 0;
		file << bufferSize;

		// setup buffer access point
		const Uint64 position = file.GetOffset();
		if ( bufferSize )
		{
			IFileLatentLoadingToken* token = file.CreateLatentLoadingToken( static_cast< Uint32 >( position ) );
			if ( token )
			{
				// Got a loading token from the file, so we can just set up that and not load the data now.
				buffer.SetAccessFromLatentToken( bufferSize, token );

				// skip data
				file.Seek( position + bufferSize );
			}
			else
			{
				// Couldn't get loading token, so need to read in the data now. Later loading will not be possible (maybe the
				// IFile is some temporary memory buffer, for duplicating objects or something).
				buffer.ReallocateBuffer( bufferSize );
				BufferHandle handle = buffer.AcquireBufferHandleForWritingSync();
				if ( handle && handle->GetData() )
				{
					// If we got a handle, read in the data.
					file.Serialize( handle->GetData(), handle->GetSize() );
				}
				else
				{
					// Couldn't even get buffer handle, so just skip the data.
					file.Seek( position + bufferSize );
				}
			}
		}
		else
		{
			buffer.Clear();
		}
	}
	else if( file.IsWriter() )
	{
		// Save as LatentDataBuffer. Mostly use for migration to DeferredDataBuffer. 
		// Editor can use the conversion code for a while until we resave/convert all data
		
		// save buffer size
		Uint32 size = buffer.GetSize();
		file << size;

		// save buffer data, raw mode
		BufferHandle handle = buffer.AcquireBufferHandleSync();

		// resave ? if so, recreate the access point
		if ( file.IsResourceResave() )
		{
			const Uint64 position = file.GetOffset();
			IFileLatentLoadingToken* token = file.CreateLatentLoadingToken( static_cast< Uint32 >( position ) );
			buffer.SetAccessFromLatentToken( size, token );
		}

		// save
		if ( handle )
		{
			file.Serialize( handle->GetData(), handle->GetSize() );
		}
	}
}

void DeferredDataBuffer::ForceImmediateAsyncLoading()
{
	st_immediateAsyncLoading = true;
}
