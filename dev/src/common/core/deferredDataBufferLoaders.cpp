/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "depot.h"
#include "depotBundles.h"
#include "diskBundle.h"
#include "deferredDataBuffer.h"
#include "deferredDataBufferLoaders.h"
#include "deferredDataBufferAsync.h"

//----------

namespace Helper
{
	static BufferHandle LoadBufferDataFromFile( IFile& file, const DeferredDataAccess::SyncAccess& accessInfo )
	{
		BufferHandle ret;

		RED_FATAL_ASSERT( accessInfo.m_alloc != nullptr, "Incomplete sync access information" );
		RED_FATAL_ASSERT( accessInfo.m_delloc != nullptr, "Incomplete sync access information" );

		// allocate memory
		void* mem = accessInfo.m_alloc( accessInfo.m_size, accessInfo.m_alignment );
		if ( mem != nullptr )
		{
			// load data
			file.ClearError();
			file.Serialize( mem, accessInfo.m_size );
			if ( !file.HasErrors() )
			{
				ret.Reset( new BufferProxy( mem, accessInfo.m_size, accessInfo.m_delloc ) );
			}
			else
			{
				ERR_CORE( TXT("Failed to load full deferred data from %ls (size=%d)"), file.GetFileNameForDebug(), accessInfo.m_size );
				accessInfo.m_delloc( mem );
			}
		}
		else
		{
			ERR_CORE( TXT("Failed to allocate memory for deferred data from %ls (size=%d)"), file.GetFileNameForDebug(), accessInfo.m_size );
		}

		return ret;
	}
}

//----------

DeferredDataAccess_LatentToken::DeferredDataAccess_LatentToken( IFileLatentLoadingToken* latentToken )
	: m_token( latentToken )
{}

DeferredDataAccess_LatentToken::~DeferredDataAccess_LatentToken()
{
	delete m_token;
}

DeferredDataAccess* DeferredDataAccess_LatentToken::Clone() const
{
	return new DeferredDataAccess_LatentToken( m_token->Clone() );
}

BufferHandle DeferredDataAccess_LatentToken::LoadSync( const SyncAccess& accessInfo ) const
{
	RED_FATAL_ASSERT( m_token != nullptr, "DeferedDataBuffer access with NULL latent token" );

	BufferHandle ret;

	// open file
	Red::TScopedPtr< IFile > file( m_token->Resume(0) );
	if ( file )
	{
		ret = Helper::LoadBufferDataFromFile( *file, accessInfo );
	}
	else
	{
		ERR_CORE( TXT("Failed to restore access to deferred data, file unknown (size=%d)"), accessInfo.m_size );
	}

	return ret;
}

BufferAsyncDataHandle DeferredDataAccess_LatentToken::LoadAsync( const AsyncAccess& accessInfo ) const
{
	RED_FATAL_ASSERT( m_token != nullptr, "DeferedDataBuffer access with NULL latent token" );

	BufferAsyncDataHandle ret;
	ret.Reset( new  BufferAsyncData_LatentToken( accessInfo, m_token->Clone() ) );
	return ret;
}

//----------

DeferredDataAccess_LatentTokenPatchable::DeferredDataAccess_LatentTokenPatchable()
{}

DeferredDataAccess_LatentTokenPatchable::~DeferredDataAccess_LatentTokenPatchable()
{
	delete m_token;
}

BufferHandle DeferredDataAccess_LatentTokenPatchable::LoadSync( const SyncAccess& accessInfo ) const
{
	RED_FATAL_ASSERT( m_token != nullptr, "DeferedDataBuffer access with NULL latent token" );

	BufferHandle ret;

	// open file
	Red::TScopedPtr< IFile > file( m_token->Resume(0) );
	if ( file )
	{
		ret = Helper::LoadBufferDataFromFile( *file, accessInfo );
	}
	else
	{
		ERR_CORE( TXT("Failed to restore access to deferred data, file unknown (size=%d)"), accessInfo.m_size );
	}

	return ret;
}

BufferAsyncDataHandle DeferredDataAccess_LatentTokenPatchable::LoadAsync( const AsyncAccess& accessInfo ) const
{
	RED_FATAL_ASSERT( m_token != nullptr, "DeferedDataBuffer access with NULL latent token" );

	BufferAsyncDataHandle ret;
	ret.Reset( new  BufferAsyncData_LatentToken( accessInfo, m_token->Clone() ) );
	return ret;
}

void DeferredDataAccess_LatentTokenPatchable::PatchAccess( IFileLatentLoadingToken* token )
{
	RED_FATAL_ASSERT( m_token == nullptr, "Trying to patch already patched access token" );
	m_token = token;
}

DeferredDataAccess* DeferredDataAccess_LatentTokenPatchable::Clone() const
{
	RED_FATAL_ASSERT( m_token != nullptr, "Buffered was accessed before it was patched - loading during saving ?" );
	return new DeferredDataAccess_LatentToken( m_token->Clone() );
}

//----------

DeferredDataAccess_BundledFile::DeferredDataAccess_BundledFile( Red::Core::Bundle::FileID fileId, const Uint32 expectedSize )
	: m_fileId( fileId )
	, m_expectedSize( expectedSize )
{}

DeferredDataAccess* DeferredDataAccess_BundledFile::Clone() const
{
	return new DeferredDataAccess_BundledFile( m_fileId, m_expectedSize );
}

BufferHandle DeferredDataAccess_BundledFile::LoadSync( const SyncAccess& accessInfo ) const
{
	BufferHandle ret;

	if ( m_expectedSize != accessInfo.m_size )
	{
		ERR_CORE( TXT("Requested loading on deferred data buffer (fileID %d) with different size (%d) than expected (%d). NOT USING."),
			m_fileId, accessInfo.m_size, m_expectedSize );
	}
	else
	{
		Red::TScopedPtr< IFile > file( GDepot->GetBundles()->CreateFileReader( m_fileId ) );
		if ( file )
		{
			// file size is different, WTF ?
			if ( file->GetSize() == m_expectedSize )
			{
				ret = Helper::LoadBufferDataFromFile( *file, accessInfo );
			}
			else
			{
				ERR_CORE( TXT("Restored deferred data buffer access (fileID %d) has different size (%d) than expected (%d). NOT USING."),
					m_fileId , file->GetSize(), m_expectedSize );
			}
		}
		else
		{
			ERR_CORE( TXT("Unable to restore deferred data buffer access (FileID %d)"), m_fileId );
		}
	}
	
	return ret;
}

BufferAsyncDataHandle DeferredDataAccess_BundledFile::LoadAsync( const AsyncAccess& accessInfo ) const
{
	BufferAsyncDataHandle ret;
	ret.Reset( new  BufferAsyncData_BundledBuffer( accessInfo, m_fileId ) );
	return ret;
}

//----------

DeferredDataAccess_PhysicalFile::DeferredDataAccess_PhysicalFile( const String& depotPath, const Uint32 expectedSize )
	: m_depotPath( depotPath )
	, m_expectedSize( expectedSize )
{}

DeferredDataAccess* DeferredDataAccess_PhysicalFile::Clone() const
{
	return new DeferredDataAccess_PhysicalFile( m_depotPath, m_expectedSize );
}

BufferHandle DeferredDataAccess_PhysicalFile::LoadSync( const SyncAccess& accessInfo ) const
{
	BufferHandle ret;

	Red::TScopedPtr< IFile > file( GFileManager->CreateFileReader( m_depotPath ) );
	if ( file )
	{
		if ( file->GetSize() != m_expectedSize )
		{
			ERR_CORE( TXT("Restored deferred data buffer access (file '%ls') has different size (%d) than expected (%d). NOT USING."), 
				m_depotPath.AsChar(), file->GetSize(), m_expectedSize );
		}
		else
		{
			ret = Helper::LoadBufferDataFromFile( *file, accessInfo );
		}
	}
	else
	{
		ERR_CORE( TXT("Unable to restore deferred data buffer access (file '%ls')"), m_depotPath.AsChar() );
	}

	return ret;
}

BufferAsyncDataHandle DeferredDataAccess_PhysicalFile::LoadAsync( const AsyncAccess& accessInfo ) const
{
	BufferAsyncDataHandle ret;
	ret.Reset( new  BufferAsyncData_PhysicalFile( accessInfo, m_depotPath ) );
	return ret;
}


//----------
