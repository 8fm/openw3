/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _CORE_DEFERRED_DATA_BUFFER_H_
#define _CORE_DEFERRED_DATA_BUFFER_H_

#include "scopedPtr.h"
#include "atomicSharedPtr.h"
#include "atomicWeakPtr.h"
#include "intrusivePtr.h"
#include "rttiType.h"
#include "classBuilder.h"
#include "../redThreads/readWriteSpinLock.h"

class IFileLatentLoadingToken;
class ILoadJob;
class BufferProxy;
class BufferAsyncData;

typedef Red::TAtomicSharedPtr< BufferProxy > BufferHandle;
typedef Red::TIntrusivePtr< BufferAsyncData > BufferAsyncDataHandle;
typedef std::function< void( BufferHandle ) > BufferAsyncCallback;

typedef std::function< void * ( Uint32 size, Uint16 alignment ) > AllocateFunction;
typedef std::function< void( void* ptr ) > DeallocateFunction;

class BufferProxy
{
	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_Engine );
public:

	BufferProxy( void * buffer, Uint32 size, const DeallocateFunction & dealloc );
	~BufferProxy();

	void * GetData() const;
	Uint32 GetSize() const;

private:
	void * m_buffer;
	Uint32 m_size;
	DeallocateFunction m_dealloc;
};

class BufferAsyncData
{
	//DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_Engine );  - to big for small objects pool - mostly due to the size of the AllocateFunction and DeallocateFunction
	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_Default, MC_Engine );

public:
	BufferAsyncData();
	virtual ~BufferAsyncData();

	enum EResult
	{
		eResult_OK,			//!< Data was read and is avaiable
		eResult_Failed,		//!< Something failed internally (CRC, decompression, IO, etc)
		eResult_NotReady,	//!< Content is not yet ready
	};

	// wait for the data to be ready, can return invalid handle if we failed to load it
	BufferHandle WaitForData() const;

	// are we in the OOM queue ?
	RED_FORCE_INLINE const Bool IsInOOMQueue_Async() const { return m_inOOMQueue.GetValue(); }

	// get size of the data we are waiting for
	virtual Uint32 GetSize() const = 0;

	// request the data, can return eResult_NotReady
	virtual EResult GetData( BufferHandle& outData ) const = 0;

	// kick data loading
	virtual void Kick() = 0;

	// reference counting
	void AddRef();
	virtual Uint32 Release();

	// reset the OOM flag, returns true if reset
	Bool ResetOOMFlag();

protected:
	// OOM handling, returns true upon sucessful registration
	Bool RegisterInOOMQueue();

	Red::Threads::CAtomic< Int32 >	m_refCount;
	Red::Threads::CAtomic< Bool >	m_inOOMQueue;
};

/// buffer data access - allows to create an access handler to the buffer data
class DeferredDataAccess
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Engine );
	
public:
	virtual ~DeferredDataAccess() {};

	// sync access info
	struct SyncAccess
	{
		AllocateFunction	m_alloc;
		DeallocateFunction	m_delloc;
		Uint32				m_size;
		Uint16				m_alignment;
	};

	// async access info
	struct AsyncAccess : public SyncAccess
	{
		BufferAsyncCallback		m_callback;
		Uint8					m_ioTag;
	};

	// load internal data synchronously
	virtual BufferHandle LoadSync( const SyncAccess& accessInfo ) const = 0;

	// request asynchronous access
	virtual BufferAsyncDataHandle LoadAsync( const AsyncAccess& accessInfo ) const = 0;

	// clone access point
	virtual DeferredDataAccess* Clone() const = 0;
};

/// buffer data access that can be later patched with new offset - ONLY FOR SAVING IN EDITOR
class DeferredDataAccessPatchable : public DeferredDataAccess
{
public:
	virtual ~DeferredDataAccessPatchable() {};
	virtual void PatchAccess( IFileLatentLoadingToken* token ) = 0;
};

class DeferredDataBuffer
{
	DECLARE_RTTI_STRUCT( DeferredDataBuffer );

public:
	typedef Uint64 FileOffsetType;

	DeferredDataBuffer();
	~DeferredDataBuffer();

	// Is this buffer valid (non empty) ?
	bool IsValid() const;

	// Is this buffer loaded (has persistent data) ?
	bool IsLoaded() const;
	
	// Get size of the buffer's data
	// This is always valid, regardless if the buffer is loaded or not
	Uint32 GetSize() const;

	// Set the internal allocator/deallocator functions to use specified memory class
	void SetMemoryClass( Red::MemoryFramework::MemoryClass memClass );

	// Set custom memory allocation/deallocation functions
	void SetAllocateFunction( const AllocateFunction & value );
	void SetDeallocateFunction( const DeallocateFunction & value );

	// Synchronous loading of the data
	BufferHandle AcquireBufferHandleSync() const;
	BufferHandle AcquireBufferHandleForWritingSync(); 

	// request async loading access to the data inside this data buffer
	// each call will open a new access request so you need to limit them at the higher level
	// NOTE: deleting the file OR modifying the file content while the async request is in flight has UNDEFINED behavior
	// NOTE: if the buffer is already loaded the existing content will be returned ASAP
	// The allocator/deallocator functions from this DDB will be used to allocate memory for the storage.
	// Internal decompression memory is allocated internally from budgeted pool.
	BufferAsyncDataHandle AcquireBufferHandleAsync( Uint8 ioTag ) const;

	// Variation of OpenAsyncAccess that accepts a callback to be used when we are finished
	BufferAsyncDataHandle AcquireBufferHandleAsync( Uint8 ioTag, const BufferAsyncCallback& callback ) const;

	// Variation of OpenAsyncAccess that accepts a callback to be used when we are finished AND taking custom alloc/dealloc callbacks
	BufferAsyncDataHandle AcquireBufferHandleAsync( Uint8 ioTag, const BufferAsyncCallback& callback, const AllocateFunction& allocFunc, const DeallocateFunction& deallocFunc ) const;

	// Clear buffer content, will also delete async access 
	void Clear();

	// Resize buffer
	void ReallocateBuffer( Uint32 size );

	// configure access methods to the data of DeferredDataBuffer
	// there are multiple sources of data that can be used with DeferredDataBuffer
	void SetBufferContent( const void * inputBuffer, Uint32 size );
	void SetAccessFromLatentToken( const Uint32 bufferSize, IFileLatentLoadingToken* token );
	void SetAccessFromBundleFile( const Uint32 bufferSize, const Red::Core::Bundle::FileID fileId );
	void SetAccessFromFileData( const Uint32 bufferSize, const Uint64 dataOffset, IFile& fileToReadFrom );
	void SetAccessFromPhysicalFile( const Uint32 bufferSize, const String& depotPath );
	
	// used only for saving to the same file - we may be required to patch our access
	Red::TAtomicSharedPtr< DeferredDataAccessPatchable > SetAccessFromLatentTokenForLaterPatching();

	// write (serialize) an object into the content of this DDB
	template< typename T >
	void WriteObject( T& inputObject );
	
	// load (deserialize) an object from the content of this DDB
	template< typename T >
	Red::TUniquePtr< T > ReadObject( const Uint32 fileVersion ) const;

	// save in a trivial stream - useful for non dependency-mapped serialization
	void SerializeDirectlyAsRawBuffer( IFile& file );

	// DeferredDataBuffer are not copyable. But RTTI system need this.
	DeferredDataBuffer & operator=( const DeferredDataBuffer & value ); 

	// Force all asynchronous access to be immediate, helps with cooking data
	static void ForceImmediateAsyncLoading();

public:
	// HACK++ - to be removed once the mesh resave is done
	RED_FORCE_INLINE const Bool HACK_WasDeserializedFromFile() const { return m_wasDeserializedFromFileHACK; }
	RED_FORCE_INLINE void HACK_SetWasDeserializedFromFile() { m_wasDeserializedFromFileHACK = true; }
	// HACK--

	// HACK++ - save ID stuff
	RED_FORCE_INLINE const Int32 GetSaveID() const { return m_saveID; }
	RED_FORCE_INLINE const void SetSaveID(const Int32 id) { m_saveID = id; }

private:
	// allocate buffer memory
	void *AllocateBuffer() const;

	// load content of the buffer, synchronously
	BufferHandle ReadBufferSync() const;

	// allocation policy
	AllocateFunction		m_alloc;
	DeallocateFunction		m_dealloc;

	// size of the data in buffer
	Uint32					m_size;

	// access helper to saved buffer data (NULL for buffers that are not yet saved to disk)
	Red::TAtomicSharedPtr< DeferredDataAccess > m_access;

	// buffer data (weak)
	mutable Red::TAtomicWeakPtr< BufferProxy > m_buffer;

	// buffer data (when writing new content)
	BufferHandle m_bufferWriting;

	// HACK++ - to be removed once the mesh resave is done
	Bool			m_wasDeserializedFromFileHACK;
	// HACK--

	// HACK++ - needed for saving the buffers
	Int32			m_saveID;
	// HACK--
	 
	typedef Red::Threads::CScopedLock< Red::Threads::CRWSpinLock > ScopedWriteLock;
	typedef Red::Threads::CScopedSharedLock< Red::Threads::CRWSpinLock > ScopedReadLock;
	mutable Red::Threads::CRWSpinLock m_lock;

	static const Uint16 DEFAULT_ALIGNMNET;

	static Bool st_immediateAsyncLoading;
};

DEFINE_SIMPLE_RTTI_TYPE( DeferredDataBuffer );

bool operator==( const DeferredDataBuffer & left, const DeferredDataBuffer & right );

void SerializeDeferredDataBufferFromLatentLoaderData( IFile& file, DeferredDataBuffer & buffer );
void SerializeDeferredDataBufferAsLatentDataBufferData( IFile & file, DeferredDataBuffer & buffer );

#include "deferredDataBuffer.inl"

#endif
