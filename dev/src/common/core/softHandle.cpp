/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "softHandle.h"
#include "resource.h"
#include "jobGenericJobs.h"
#include "depot.h"
#include "loadingJobManager.h"
#include "softHandleProcessor.h"

BaseSoftHandle::BaseSoftHandle()
	: m_asyncLoad( NULL )
{
}

BaseSoftHandle::BaseSoftHandle( CResource* resource )
	: m_asyncLoad( NULL )
{
	if ( resource )
	{
		if ( resource->GetFile() )
		{
			THandle< CResource > resHandle( resource );
			m_resource = (BaseSafeHandle&)resHandle;
			m_path = resource->GetDepotPath();
		}
		else
		{
			HALT( "Unable to initialize soft handle with a fileless resource" );
		}
	}
}

BaseSoftHandle::BaseSoftHandle( const BaseSoftHandle& other )
	: m_resource( other.m_resource )
	, m_asyncLoad( NULL )
	, m_path( other.m_path )
{
}

BaseSoftHandle::BaseSoftHandle( BaseSoftHandle&& other )
	: m_resource( Move( other.m_resource ) )
	, m_asyncLoad( other.m_asyncLoad )
	, m_path( Move( other.m_path ) )
{
	other.m_asyncLoad = nullptr;
}

BaseSoftHandle::BaseSoftHandle( const String& path )
	: m_asyncLoad( NULL )
	, m_path( path )
{
}

BaseSoftHandle::~BaseSoftHandle()
{
	// Release current token
	if ( m_asyncLoad )
	{
		m_asyncLoad->Release();
		m_asyncLoad = NULL;
	}
}

BaseSoftHandle& BaseSoftHandle::operator=( const BaseSoftHandle& other )
{
	if ( this != &other )
	{
		// Release current token
		if ( m_asyncLoad )
		{
			m_asyncLoad->Release();
			m_asyncLoad = NULL;
		}

		// Copy data
		m_path = other.m_path;
		m_resource = other.m_resource;
		m_asyncLoad = NULL;
	}

	return *this;
}

BaseSoftHandle& BaseSoftHandle::operator=( BaseSoftHandle&& other )
{
	if ( this != &other )
	{
		// Release current token
		if ( m_asyncLoad )
		{
			m_asyncLoad->Release();
			m_asyncLoad = nullptr;
		}

		// Move data
		m_path = Move( other.m_path );
		m_resource = Move( other.m_resource );
		m_asyncLoad = other.m_asyncLoad;
		other.m_asyncLoad = nullptr;
	}
	return *this;
}

THandle< CResource > BaseSoftHandle::Get() const
{
	// No path
	if ( m_path.Empty() )
	{
		return NULL;
	}

	// Load object
	if ( !Load() )
	{
		WARN_CORE( TXT("Unable to load soft handle object '%ls'"), m_path.AsChar() );
		return NULL;
	}

	// Get object
	return (const THandle< CResource >&) m_resource;
}

BaseSoftHandle::EAsyncLoadingResult BaseSoftHandle::GetAsync( Bool immediate ) const
{
	// No path
	if ( m_path.Empty() )
	{
		return ALR_Loaded;
	}

	// Already valid, not loading anything new
	if ( m_resource.Get() )
	{
		return ALR_Loaded;
	}

	// Check if resource is loaded in depot
	if ( !m_asyncLoad )
	{
		CResource* depotResource = GDepot->FindResource( m_path );
		if ( depotResource )
		{
			THandle< CResource > depotResourceHandle( depotResource );
			m_resource = (BaseSafeHandle&) depotResourceHandle;

			return ALR_Loaded;
		}
	}
	
	// Loading Token already created
	if ( m_asyncLoad )
	{
		// Failed
		if ( m_asyncLoad->HasEnded() && !m_asyncLoad->HasFinishedWithoutErrors() )
		{
			// Release token
			m_asyncLoad->Release();
			m_asyncLoad = NULL;

			// Report error
			WARN_CORE( TXT("Unable to load soft handle object '%ls'"), m_path.AsChar() );

			return ALR_Failed;
		}

		// Finished
		if ( m_asyncLoad->HasEnded() && m_asyncLoad->HasFinishedWithoutErrors() )
		{
			// Update
			CResource* res = m_asyncLoad->GetResource();
			if ( res )
			{
				// Set loaded
				THandle< CResource > resHandle( res );
				m_resource = (BaseSafeHandle&)resHandle;
	
				// Release token
				m_asyncLoad->Release();
				m_asyncLoad = NULL;

				// Return loaded resource
				return ALR_Loaded;
			}
			else
			{
				// Error, res is NULL?
				m_asyncLoad->Release();
				m_asyncLoad = NULL;

				return ALR_Failed;
			}
		}

		// Not loaded yet
		return ALR_InProgress;
	}

	// Create loading job
	m_asyncLoad = new CSoftHandleLoadJob( m_path, immediate ? JP_Immediate : JP_Resources, false, nullptr, 
		immediate ? eResourceLoadingPriority_High : eResourceLoadingPriority_Normal );

	// Issue this job to the job manager
	SJobManager::GetInstance().Issue( m_asyncLoad );

	// Not loaded
	return ALR_InProgress;
}

Bool BaseSoftHandle::Clear()
{
	m_path = String::EMPTY;
	return Release();
}

Bool BaseSoftHandle::Load() const
{
	// Already loaded
	if ( m_resource.Get() )
	{
		return true;
	}

	// Unable to load empty shit
	if ( m_path.Empty() )
	{
		return false;
	}

	Bool actuallyLoading = false;
	if( GetHandle().Get() == nullptr )		// Are we *really* loading anything?
	{
		actuallyLoading = true;
	}

	// Load resource
	m_resource = (const BaseSafeHandle&) GDepot->LoadResource( m_path );

	// Return true if resource was loaded
	return m_resource.IsValid();
}

Bool BaseSoftHandle::Release() const
{
	// Clear handle
	m_resource.Clear();
	// Release current token
	if ( m_asyncLoad )
	{
		m_asyncLoad->Release();
		m_asyncLoad = NULL;
	}
	return true;
}


Bool BaseSoftHandle::ReleaseJob() const
{
	if ( m_asyncLoad )
	{
		m_asyncLoad->Release();
		m_asyncLoad = NULL;
	}
	return true;
}


void BaseSoftHandle::Serialize( IFile& file )
{
	// GC
	if ( file.IsGarbageCollector() )
	{
		file << (THandle< CResource >&)m_resource;
		return;
	}

	// Save handle directly
	file << *this;
}

Bool BaseSoftHandle::IsLoading() const
{
	// Do not use m_asyncLoad member
#ifndef NO_EDITOR
	if ( const CDiskFile* df = GDepot->FindFileUseLinks( GetPath(), 0 ) )
#else
	if ( const CDiskFile* df = GDepot->FindFile( GetPath() ) )
#endif
	{
		return df->IsLoading();
	}

	return false;
}
