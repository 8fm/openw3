/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "depot.h"
#include "depotBundles.h"
#include "diskBundle.h"
#include "deferredDataBuffer.h"
#include "deferredDataBufferLoaders.h"
#include "deferredDataBufferKickoff.h"
#include "deferredDataBufferAsync.h"
#include "loadingJob.h"
#include "loadingJobManager.h"
#include "asyncFileAccess.h"
#include "configVar.h"

//#undef RED_FATAL_ASSERT
//#define RED_FATAL_ASSERT(x, expr) if ( !(x) ) { fwprintf( stderr, L"CRASH: %hs\n", #expr ); __debugbreak(); }

//----------

namespace Config
{
	TConfigVar< Bool >			cvDeferredBufferFakeOOM( "Streaming", "DeferredBufferFakeOOM", false );
}


//----------

class CBufferAsyncData_LoadingJob : public ILoadJob
{
public:
	CBufferAsyncData_LoadingJob( BufferAsyncDataJobBased* task )
		: ILoadJob( JP_SpawnEntity /* TODO: priorities !! */, false )
		, m_task( task )
	{}

	virtual const Char* GetDebugName() const override { return TXT("BufferAsyncData_LoadingJob"); }

	virtual EJobResult Process()
	{
		m_task->AddRef();
		const EJobResult result = m_task->ProcessLoading() ? JR_Finished : JR_Failed;
		if ( 0 == m_task->Release() )
		{
			delete m_task;
		}

		return result;
	}

protected:
	BufferAsyncDataJobBased*	m_task;
};

BufferAsyncDataJobBased::BufferAsyncDataJobBased( const DeferredDataAccess::AsyncAccess& accessData )
	: m_accessData( accessData )
	, m_job( nullptr )
{
}

BufferAsyncDataJobBased::~BufferAsyncDataJobBased()
{
	Red::Threads::CScopedLock< Red::Threads::CLightMutex > lock( m_lock );

	// finish and/or cancel the loading job
	if ( m_job != nullptr )
	{
		m_job->Cancel();
		m_job->Release();
		m_job = nullptr;
	}
}

void BufferAsyncDataJobBased::IssueJob()
{
	Red::Threads::CScopedLock< Red::Threads::CLightMutex > lock( m_lock );

	// create loading job
	if ( !m_job )
	{
		m_job = new CBufferAsyncData_LoadingJob( this );
		SJobManager::GetInstance().Issue( m_job );
	}
}

Bool BufferAsyncDataJobBased::ProcessLoading()
{
	PC_SCOPE( BufferAsyncDataJobBased_ProcessLoading );

	Red::Threads::CScopedLock< Red::Threads::CLightMutex > lock( m_lock );

	RED_FATAL_ASSERT( m_accessData.m_alloc, "Logic Error. DEBUG!" );
	RED_FATAL_ASSERT( m_accessData.m_delloc, "Logic Error DEBUG!" );

	// allocate memory
	void* mem = m_accessData.m_alloc( m_accessData.m_size, m_accessData.m_alignment );
	if ( !mem )
	{
		m_failed = true;
		return false; // OOM
	}

	// load data
	if ( !LoadDataIntoMemory( mem, m_accessData.m_size ) )
	{
		m_accessData.m_delloc( mem );

		// failed to load
		m_failed = true;
		return false;
	}

	// setup buffer
	m_data.Reset( new BufferProxy( mem, m_accessData.m_size, m_accessData.m_delloc ) );
	m_failed = false;

	// call the loading callback (async!! - called from the job thread)
	if ( m_accessData.m_callback )
	{
		m_accessData.m_callback( m_data );
	}

	// done
	return true;
}

BufferAsyncDataJobBased::EResult BufferAsyncDataJobBased::GetData( BufferHandle& outData ) const
{
	Red::Threads::CScopedLock< Red::Threads::CLightMutex > lock( m_lock );

	// check if the job has finished
	if ( m_job && m_job->HasEnded() )
	{
		m_failed = !m_job->HasFinishedWithoutErrors();

		m_job->Release();
		m_job = nullptr;
	}

	// check if data is already loaded
	if ( m_data )
	{
		outData = m_data;
		return eResult_OK;
	}

	// we are either still loading of we have failed
	if ( m_failed )
		return eResult_Failed;
	else
		return eResult_NotReady;
}

//----------

BufferAsyncData_PhysicalFile::BufferAsyncData_PhysicalFile( const DeferredDataAccess::AsyncAccess& accessData, const String& path )
	: BufferAsyncDataJobBased( accessData )
	, m_filePath( path )
{
	// start only after we are fully initialized
	IssueJob();
}

Bool BufferAsyncData_PhysicalFile::LoadDataIntoMemory( void* memory, const Uint32 size ) const
{
	// open the file
	Red::TScopedPtr< IFile > file( GFileManager->CreateFileReader( m_filePath, 0 ) );
	if ( !file )
	{
		ERR_CORE( TXT("Unable to open file '%ls'. Deferred data buffer async loading failed."), m_filePath.AsChar() );
		return false;
	}

	// check size
	if ( file->GetSize() != size )
	{
		ERR_CORE( TXT("Size of the file '%ls' = %d and differs from expected %d. Deferred data buffer async loading failed."), 
			m_filePath.AsChar(), file->GetSize(), size );
		return false;
	}

	// load data
	file->ClearError();
	file->Serialize( memory, size );

	// failed ?
	if ( file->HasErrors() )
	{
		ERR_CORE( TXT("IO errors when loading data from '%ls' (size=%d). Deferred data buffer async loading failed."), 
			m_filePath.AsChar(), size );
		return false;
	}

	// loaded
	return true;
}

//----------

BufferAsyncData_LatentToken::BufferAsyncData_LatentToken( const DeferredDataAccess::AsyncAccess& accessData, IFileLatentLoadingToken* tokenCloned )
	: BufferAsyncDataJobBased( accessData )
	, m_token( tokenCloned )
{
	// start only after we are fully initialized
	IssueJob();
}

BufferAsyncData_LatentToken::~BufferAsyncData_LatentToken()
{
	delete m_token;
}

Bool BufferAsyncData_LatentToken::LoadDataIntoMemory( void* memory, const Uint32 size ) const
{
	// open the file
	Red::TScopedPtr< IFile > file( m_token->Resume(0) );
	if ( !file )
	{
		ERR_CORE( TXT("Unable to resume loading from latent data token. Deferred data buffer async loading failed.") );
		return false;
	}

	// load data
	file->ClearError();
	file->Serialize( memory, size );

	// failed ?
	if ( file->HasErrors() )
	{
		ERR_CORE( TXT("IO errors when loading data from '%ls' (size=%d). Deferred data buffer async loading failed."), 
			file->GetFileNameForDebug(), size );
		return false;
	}

	// loaded
	return true;
}

//----------

BufferAsyncData_BundledBuffer::BufferAsyncData_BundledBuffer( const DeferredDataAccess::AsyncAccess& accessData, Red::Core::Bundle::FileID fileID )
	: m_accessData( accessData )
	, m_asyncFile( nullptr )
	, m_fileID( fileID )
	, m_state( eState_NotStated )
	, m_kickRegistered( false )
{
	// try to start reading of the buffered data immediately
	TryStart();

	// if we have a callback we need to register for "nagging"
	if ( m_accessData.m_callback )
	{
		m_kickRegistered = true;
		SDeferredDataBufferKickOffList::GetInstance().RegisterCallback( this );
	}
}

BufferAsyncData_BundledBuffer::~BufferAsyncData_BundledBuffer()
{
	// state error
	RED_FATAL_ASSERT( m_state == eState_Failed || m_state == eState_Finished, "We are ending in invalid state" );

	// in all of the other cases it's illegal to be registered in the kickoff while being deleted
	RED_FATAL_ASSERT( m_kickRegistered == false, "Still in the kick off list! This should not happen even if we are not started yet." );

	// close any opened access to the async file
	if ( m_asyncFile != nullptr )
	{
		m_asyncFile->Release();
		m_asyncFile = nullptr;
	}
}

Uint32 BufferAsyncData_BundledBuffer::GetSize() const
{
	return m_accessData.m_size;
}

BufferAsyncData_BundledBuffer::EResult BufferAsyncData_BundledBuffer::GetData( BufferHandle& outData ) const
{
	Red::Threads::CScopedLock< Red::Threads::CLightMutex > lock( m_lock );

	// advance internal state
	UpdateState();

	// simple states
	if ( m_state == eState_Failed )
	{
		UnregisterKicker_NoLock();
		return eResult_Failed;
	}
	else if ( m_state == eState_Finished )
	{
		RED_FATAL_ASSERT( m_data, "Finished without data" );
		UnregisterKicker_NoLock();
		outData = m_data;
		return eResult_OK;
	}

	// still loading
	return eResult_NotReady;
}

void BufferAsyncData_BundledBuffer::UpdateState() const
{
	Red::Threads::CScopedLock< Red::Threads::CLightMutex > lock( m_lock );
	UpdateState_NoLock();
}

void BufferAsyncData_BundledBuffer::UpdateState_NoLock() const
{
	// start if not started yet
	// this is required because starting of the decompression tasks and AsyncIO is throttled
	// throttling is required because we have limited resources in the system (mostly async handles)
	if ( m_state == eState_NotStated )
	{
		TryStart();
	}

	// simple states
	if ( m_state == eState_Started )
	{
		RED_FATAL_ASSERT( m_asyncFile, "Started without async file" );

		// check if the async file has finished loading
		IFile* reader = nullptr;
		const auto ret = m_asyncFile->GetReader( reader );

		// we are still not ready
		if ( ret == IAsyncFile::eResult_Failed )
		{
			// loading of the async file has failed
			ERR_CORE( TXT("Async loading of '%ls' has failed. Deferred data buffer will not be loaded."), m_asyncFile->GetFileNameForDebug() );
			m_state = eState_Failed;

			// close async file
			m_asyncFile->Release();
			m_asyncFile = nullptr;
		}
		else if ( ret == IAsyncFile::eResult_OK )
		{
			BufferHandle localData;

			// in callback case load only into local data buffer
			BufferHandle& targetData = m_data;
			if ( m_accessData.m_callback )
			{
				targetData = localData;
			}

			// allocate destination memory
			void* mem = Config::cvDeferredBufferFakeOOM.Get() ? nullptr : m_accessData.m_alloc( m_accessData.m_size, m_accessData.m_alignment );
			if ( !mem )
			{
				// we've failed due to OOM
				ERR_CORE( TXT("Async loading of DDB at bundle file ID %d failed due to internal OOM, size=%d"), 
					m_fileID, m_accessData.m_size );

				// put in the OOM queue and wait to be rescheduled
				// W3 Hack: we break const correctness
				const_cast< BufferAsyncData_BundledBuffer* >(this)->RegisterInOOMQueue();

				// revert to a non started state since we will be releasing the file resource
				m_state = eState_NotStated;
			}
			else
			{
				// load data
				reader->Serialize( mem, m_accessData.m_size );

				// setup buffer
				targetData.Reset( new BufferProxy( mem, m_accessData.m_size, m_accessData.m_delloc ) );

				// done
				m_state = eState_Finished;
			}

			// close the reader
			delete reader;

			// close async file
			m_asyncFile->Release();
			m_asyncFile = nullptr;
			
			// call the optional callback
			if ( m_state == eState_Finished )
			{
				// ensure the callback is called only once
				if ( m_accessData.m_callback )
				{
					// ctremblay some very rare crash are occuring deep in std::function and related to TSharedPtr dtor.
					// To get better picture, and easier to debug callstack, I'm storing a temp value of data to make sure it's not related to TSharedPtr.
					// If callstack is same, it's either a memory stomp, some fuckup in mem alloc of std::function, or some nasty bug in std::function.
					BufferHandle resultData = targetData; 

					m_accessData.m_callback( resultData );
				}
			}

			// we are done, unregister from kick list
			UnregisterKicker_NoLock();
		}
	}
}

void BufferAsyncData_BundledBuffer::TryStart() const
{
	RED_FATAL_ASSERT( m_state == eState_NotStated, "Invalid state logic" );
	RED_FATAL_ASSERT( m_asyncFile == nullptr, "Invalid state logic" );

	const auto ret = GDepot->GetBundles()->CreateAsyncFileReader( m_fileID, m_accessData.m_ioTag, m_asyncFile );
	if ( ret == eAsyncReaderResult_Failed )
	{
		ERR_CORE( TXT("Async loading of DDB at bundle file ID %d cannot be started: failed to create async file"), m_fileID );
		m_state = eState_Failed;
	}
	else if ( ret == eAsyncReaderResult_OK )
	{
		m_state = eState_Started;
	}
}

void BufferAsyncData_BundledBuffer::UnregisterKicker_NoLock() const
{
	// unregister the kicker
	if ( m_kickRegistered )
	{
		m_kickRegistered = false;
		SDeferredDataBufferKickOffList::GetInstance().UnregisterCallback( const_cast< BufferAsyncData_BundledBuffer* >( this ) );
	}
}

Uint32 BufferAsyncData_BundledBuffer::Release()
{
	if ( m_kickRegistered )
	{
		RED_FATAL_ASSERT( m_refCount.GetValue() > 1, "Invalid ref count - one ref is held by the kickoff list, we must have at least one more" );
	}
	return SDeferredDataBufferKickOffList::GetInstance().ReleaseCallbackRef( &m_refCount, this );
}

void BufferAsyncData_BundledBuffer::Kick()
{
	Red::Threads::CScopedLock< Red::Threads::CLightMutex > lock( m_lock );

	// update internal state
	if ( m_state == eState_NotStated || m_state == eState_Started )
	{
		UpdateState();
	}
}

//----------

BufferAsyncData_SyncData::BufferAsyncData_SyncData( BufferHandle data, const Uint32 size )
	: m_data( data )
	, m_size( size )
{
}

Uint32 BufferAsyncData_SyncData::GetSize() const
{
	return m_size;
}

BufferAsyncData_SyncData::EResult BufferAsyncData_SyncData::GetData( BufferHandle& outData ) const
{
	outData = m_data;
	return eResult_OK;
}

//----------
