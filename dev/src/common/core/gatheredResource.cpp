/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "gatheredResource.h"
#include "directory.h"
#include "depot.h"

CGatheredResource::CGatheredResource( const Char* path, Uint32 flags )
	: m_flags( flags )
	, m_path( path )
	, m_isInvalid( false )
	, m_resource( NULL )
{
	// Validate the path passed to the gathered resource
	DepotHelpers::ValidatePathString( path );

	// Add to resource list
	GetGatheredResources().PushBack( this );
}

CGatheredResource::~CGatheredResource()
{
	if ( !GIsClosing )
	{
		// Release resource handle
		Release();

		// Remove from list of resources
		GetGatheredResources().Remove( this );
	}
}

void CGatheredResource::ReleaseAllGatheredResources()
{
	const TDynArray< CGatheredResource* >& resList = GetGatheredResources();
	for ( Uint32 i=0; i<resList.Size(); i++ )
	{
		resList[i]->Release();
	}
}

TDynArray< CGatheredResource* >& CGatheredResource::GetGatheredResources()
{
	static TDynArray< CGatheredResource* > s_resources;
	return s_resources;
}

// #define GATHERED_LOADING_STATS

Bool CGatheredResource::Load()
{
	// Failed already...
	if ( m_isInvalid )
	{
		return false;
	}

	// Resource is already loaded
	if ( m_resource )
	{
		return true;
	}

	// Get path
	String resourcePath = m_path.ToString();
	ASSERT( !resourcePath.Empty() );

#ifdef GATHERED_LOADING_STATS
	// Stats
	CFileLoadingStats loadingStats;

	// Load resource
	CTimeCounter loadingTimer;
	ResourceLoadingContext loadingContext;
	loadingContext.m_stats = &loadingStats;
	m_resource = GDepot->LoadResource( resourcePath, loadingContext  );
#else
	// Load resource, no stats
	m_resource = GDepot->LoadResource( resourcePath );
#endif

	// Not loaded
	if ( !m_resource )
	{
		m_isInvalid = true;
		WARN_CORE( TXT("Unable to load cooked '%ls'. Make sure resource was gathered and check log for details."), resourcePath.AsChar() );
		return false;
	}

	// Keep reference
	m_resource->AddToRootSet();

#ifdef GATHERED_LOADING_STATS
	// Stats
	const Float loadingTime = loadingTimer.GetTimePeriod();
	if ( !GIsCooker )
	{
		LOG_CORE( TXT("Gathered resource '%ls' loaded in %1.2fs"), resourcePath.AsChar(), loadingTime );
	}

	// Stats
	if ( loadingTime > 0.002f )
	{
		LOG_CORE( TXT("Loading stats of '%ls'"), resourcePath.AsChar() );
		loadingStats.Dump();
	}
#endif

	// Resource loaded
	return true;
}

void CGatheredResource::Release()
{
	// Release, only works in cooked builds
	if ( m_resource )
	{
		m_resource->RemoveFromRootSet();
		m_resource = nullptr;
	}
}
