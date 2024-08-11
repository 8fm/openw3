/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "jobGenericJobs.h"
#include "depot.h"
#include "diskFile.h"
#include "fileLoadingStats.h"
#include "fileLatentLoadingToken.h"
#include "memoryFileReader.h"
#include "dependencyLoader.h"
#include "profiler.h"
#include "dependencyCache.h"
#include "softHandleProcessor.h"
#include "genericFence.h"

//////////////////////////////////////////////////////////////////////////////

CJobLoadResource::CJobLoadResource( const String& resourceToLoad, EJobPriority prio /*= JP_Resources*/, Bool willBeElementOfBatch /*= false*/, class CGenericCountedFence* loadingFence /*= nullptr*/, EResourceLoadingPriority rprio /*= EResourceLoadingPriority::eResourceLoadingPriority_Normal*/, CCallback* callback )
	: ILoadJob( prio, true )
	, m_resourceToLoad( resourceToLoad )
	, m_loadingFence( loadingFence )
	, m_callback( callback )
	, m_loadingPriority( rprio )
{
	// Check if this resource already exists
	m_loadedResource = GDepot->FindResource( m_resourceToLoad );
	if ( m_loadedResource )
	{
		m_loadedResource->AddToRootSet();
	}
}

CJobLoadResource::~CJobLoadResource()
{
	// Release link to resource if isn't released already
	if ( m_loadedResource && m_loadedResource->IsInRootSet() )
	{
		m_loadedResource->RemoveFromRootSet();
	}

	// Release the loading fence
	if ( m_loadingFence )
	{
		WARN_CORE( TXT("Ending loading job '%ls' without signalling fence '%ls'"),
			m_resourceToLoad.AsChar(), ANSI_TO_UNICODE( m_loadingFence->GetDebugName() ) );

		m_loadingFence->Release();
		m_loadingFence = nullptr;
	}
}

void CJobLoadResource::SignalLoadingFence()
{
	if ( m_loadingFence )
	{
		m_loadingFence->Signal();
		m_loadingFence->Release();
		m_loadingFence = nullptr;
	}
}

EJobResult CJobLoadResource::Process()
{
	// Resource is already loaded
	if ( m_loadedResource )
	{
		if ( m_callback )
		{
			m_callback->OnJobFinished( this, JR_Finished );
		}
		SignalLoadingFence();
		return JR_Finished;
	}

	// No source to load from
	if ( m_resourceToLoad.Empty() )
	{
		if ( m_callback )
		{
			m_callback->OnJobFinished( this, JR_Failed );
		}
		WARN_CORE( TXT("IO Job created with no source file to load from") );
		SignalLoadingFence();
		return JR_Failed;
	}

	// Load resource
	ResourceLoadingContext context;
	context.m_priority = m_loadingPriority;
	m_loadedResource = GDepot->LoadResource( m_resourceToLoad, context );
	if ( !m_loadedResource )
	{
		if ( m_callback )
		{
			m_callback->OnJobFinished( this, JR_Failed );
		}
		// Resource could not be loaded
		WARN_CORE( TXT("IO Job failed to load resource '%ls'"), m_resourceToLoad.AsChar() );
		SignalLoadingFence();
		return JR_Failed;
	}
	else
	{
		// Keep reference
		m_loadedResource->AddToRootSet();
	}

	if ( m_callback )
	{
		m_callback->OnJobFinished( this, JR_Finished );
	}

	SignalLoadingFence();
	return JR_Finished;
}

//////////////////////////////////////////////////////////////////////////////

CJobLoadData::CJobLoadData( const JobLoadDataInitInfo& initData, EJobPriority priority /*= JP_Default*/, Red::MemoryFramework::MemoryClass memoryClass /*= MC_BufferFlash */, const Bool isGCBlocker /*= false*/ )
	: ILoadJob( priority, isGCBlocker )
	, m_offset( initData.m_offset )
	, m_size( initData.m_size )
	, m_sourceFile( NULL )
	, m_sourceFileToken( NULL )
	, m_buffer( NULL )
	, m_memoryClass( memoryClass )
{
	// Determine source
	if ( initData.m_sourceFileToken )
	{
		// We load from file token
		m_sourceFileToken = initData.m_sourceFileToken->Clone();
		ASSERT( m_sourceFileToken );
	}
	else if ( initData.m_sourceFile )
	{
		if ( initData.m_closeFile )
		{
			// We load from a file that we should close after finishing
			m_sourceFile = initData.m_sourceFile;
			m_ownsFileHandle = true;
		}
		else
		{
			// We load from a file that was opened elsewhere and we should not touch it
			m_sourceFile = initData.m_sourceFile;
			m_ownsFileHandle = false;
		}
	}

	// Determine target
	if ( initData.m_buffer )
	{
		// We do have a preallocated memory buffer
		m_buffer = initData.m_buffer;
		m_ownsBufferHandle = false;
	}
	else
	{
		// We need to allocate memory for loading ourselves
		m_ownsBufferHandle = true;
	}
}

CJobLoadData::~CJobLoadData()
{
	// Close file
	if ( m_sourceFile && m_ownsFileHandle )
	{
		delete m_sourceFile;
		m_sourceFile = NULL;
	}

	// Free memory
	if ( m_buffer && m_ownsBufferHandle )
	{
		RED_MEMORY_FREE( MemoryPool_Default, m_memoryClass, m_buffer );
		m_buffer = NULL;
	}

	// Always free the latent token, it's cloned
	if ( m_sourceFileToken )
	{
		delete m_sourceFileToken;
		m_sourceFileToken = NULL;
	}
}

size_t CJobLoadData::GetDataSize()
{
	// Make sure we can use the data handle
	if ( HasFinishedWithoutErrors() )
	{
		return m_size;
	}

	// Not valid
	return 0;
}

const void* CJobLoadData::GetDataBuffer()
{
	// Make sure we can use the data handle
	if ( HasFinishedWithoutErrors() )
	{
		return m_buffer;
	}

	// Not valid
	return NULL;
}

EJobResult CJobLoadData::Process()
{
	// There's nothing to load
	if ( m_size == 0 )
	{
		WARN_CORE( TXT("IO Job created with 0 data to load") );
		return JR_Failed;
	}

	// No source
	if ( !m_sourceFile && !m_sourceFileToken )
	{
		WARN_CORE( TXT("IO Job created with no source file to load from") );
		return JR_Failed;
	}

	// Open the file
	IFile* file = m_sourceFile;
	IFile* fileToDelete = NULL;
	if ( m_sourceFileToken )
	{
		// Resume the loading
		file = m_sourceFileToken->Resume( 0 );
		if ( !file )
		{
			WARN_CORE( TXT("IO Job failed because we cannot restore loading") );
			return JR_Failed;
		}

		// Close this file when exiting
		fileToDelete = file;
	}

	// Determine size
	if ( m_size == NumericLimits< size_t >::Max() )
	{
		const size_t fileSize = static_cast< size_t >( file->GetSize() );
		RED_ASSERT( (Uint64)fileSize == file->GetSize(), TXT("Unexpectedly large file '%ls'"), file->GetFileNameForDebug() );
		m_size = fileSize - m_offset;
	}

	// Allocate memory
	if ( !m_buffer && m_ownsBufferHandle )
	{
		m_buffer = RED_MEMORY_ALLOCATE( MemoryPool_Default, m_memoryClass, m_size );
		if ( !m_buffer )
		{
			delete fileToDelete;
			WARN_CORE( TXT("IO Job failed because we cannot allocate target buffer (%i)"), m_size );
			return JR_Failed;
		}
	}

	// Get to file position
	file->Seek( m_offset );

	// Load directly to specified buffer
	file->Serialize( m_buffer, m_size );

	// Close the source file in case it was opened by us
	delete fileToDelete;

	// Finished with success
	return JR_Finished;
}

//////////////////////////////////////////////////////////////////////////////

CJobLoadDataCreateObject::CJobLoadDataCreateObject( const JobLoadDataInitInfo& initData, EJobPriority priority /*= JP_Default*/, Red::MemoryFramework::MemoryClass memoryClass /*= MC_BufferFlash */ )
	: CJobLoadData( initData, priority, memoryClass, true )
{
	m_object = initData.m_object;
}

CJobLoadDataCreateObject::~CJobLoadDataCreateObject()
{
}

EJobResult CJobLoadDataCreateObject::Process()
{
	// There's nothing to load
	if ( m_size == 0 )
	{
		WARN_CORE( TXT("IO Job created with 0 data to load") );
		return JR_Failed;
	}

	// No source
	if ( !m_sourceFileToken )
	{
		WARN_CORE( TXT("IO Job created with no source file to load from") );
		return JR_Failed;
	}

	// Open the file
	IFile* file = NULL;
	IFile* fileToDelete = NULL;
	if ( m_sourceFileToken )
	{
		// Resume the loading
		file = m_sourceFileToken->Resume( 0 );
		if ( !file )
		{
			WARN_CORE( TXT("IO Job failed because we cannot restore loading") );
			return JR_Failed;
		}

		// Close this file when exiting
		fileToDelete = file;
	}
	
	// Allocate memory
	if ( !m_buffer )
	{
		delete fileToDelete;
		WARN_CORE( TXT("IO Job failed because we cannot allocate target buffer ( %i )"), m_size );
		return JR_Failed;
	}

	// Load directly to specified buffer
	file->Serialize( m_buffer, m_size );

	// Close the source file in case it was opened by us
	delete fileToDelete;

	{
		TDynArray<Uint8> tempBuf;
		tempBuf.Resize( m_size );
		Red::System::MemoryCopy( tempBuf.Data(), m_buffer, m_size );

		// Deserialize cached buffer
		TDynArray< CObject* > loadedObjects;
		CMemoryFileReader reader( tempBuf, 0 );
		CDependencyLoader loader( reader, NULL );

		// Load
		DependencyLoadingContext loadingContext;
		loadingContext.m_parent = NULL; 
		{	
			PC_SCOPE( Load );
			if ( !loader.LoadObjects( loadingContext ) )
			{
				WARN_CORE( TXT("Deserialization failed") );
				return JR_Failed;
			}
		}

		// Post load them
		{
			PC_SCOPE( PostLoad );
			loader.PostLoad();
		}

		/// object issuing this job MUST release ref
		loadingContext.m_loadedRootObjects[0]->AddToRootSet();
		*m_object = loadingContext.m_loadedRootObjects[0];
	}

	// Finished with success
	return JR_Finished;
}

//////////////////////////////////////////////////////////////////////////////
