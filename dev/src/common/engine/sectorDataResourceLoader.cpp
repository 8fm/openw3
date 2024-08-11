/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "sectorData.h"
#include "sectorDataMerged.h"
#include "sectorDataResourceLoader.h"

#include "mesh.h"
#include "bitmapTexture.h"

#include "../core/depot.h"
#include "../core/jobGenericJobs.h"
#include "../core/resourceLoading.h"

CSectorDataResourceLoader::CSectorDataResourceLoader( const CSectorDataMerged* baseData )
	: m_sourceData( baseData )
	, m_numStreamingResources( 0 )
	, m_numLoadedResources( 0 )
{
	Red::MemoryZero( &m_resources, sizeof(m_resources) );
}

CSectorDataResourceLoader::~CSectorDataResourceLoader()
{
	// release all loaded resources
	for ( auto& it : m_resources )
	{
		// stop streaming
		if ( it.m_loadingJob != nullptr )
		{
			it.m_loadingJob->Cancel();
			it.m_loadingJob->Release();
			it.m_loadingJob = nullptr;
		}

		// cleanup resource
		it.m_collision.Reset();
		it.m_resource = nullptr;
	}
}

const Uint32 CSectorDataResourceLoader::GetNumStreamingResources() const
{
	return m_numStreamingResources.GetValue();
}

const Uint32 CSectorDataResourceLoader::GetNumLoadedResources() const
{
	return m_numLoadedResources.GetValue();
}

void CSectorDataResourceLoader::Flush()
{
	PC_SCOPE_PIX( FlushResourceStreaming );

	CTimeCounter timer;

	RED_FATAL_ASSERT( ::SIsMainThread(), "This should be called only from main thread" );

	// wait for all of the streaming jobs to finish
	Uint32 numResourcesFinished = 0;
	Uint32 numResourcesFailed = 0;
	Uint32 numResourcesLoaded = 0;
	Uint32 numCollisionFinished = 0;
	Uint32 numCollisionFailed = 0;
	Uint32 numCollisionLoaded = 0;
	for ( Uint32 i=0; i<MAX_RESOURCES; ++i )
	{
		auto& info = m_resources[i];

		// loading a resource
		if ( info.m_loadingJob != nullptr )
		{
			while ( !info.m_loadingJob->HasEnded() )
			{
				Red::Threads::YieldCurrentThread();
			}

			// valid resource loaded, extract the resource
			if ( info.m_loadingJob->HasFinishedWithoutErrors() )
			{
				CJobLoadResource* loadJob = static_cast< CJobLoadResource* >( info.m_loadingJob );
				info.m_resource = loadJob->GetResource();
				RED_FATAL_ASSERT( info.m_resource != nullptr, "Job succeeded but there's no resource" );
				numResourcesFinished += 1;
			}
			else
			{
				numResourcesFailed += 1;
			}

			// release the finished job
			info.m_loadingJob->Release();
			info.m_loadingJob = nullptr;
		}

		// wait for collision data
		if ( info.m_loadingCollision )
		{
			const String depotPath = info.m_file->GetDepotPath();

			// load collision
			CompiledCollisionPtr ptr;
			Red::System::DateTime emptyTime;
			const auto ret = GCollisionCache->FindCompiled_Sync( ptr, depotPath, emptyTime );
			if ( ret == ICollisionCache::eResult_Valid )
			{
				info.m_collision = ptr;
				info.m_loadingCollision = false;
				numCollisionFinished += 1;
			}
			else
			{
				info.m_loadingCollision = false;
				numCollisionFailed += 1;
			}
		}

		// count loaded resources
		if ( info.m_resource )
		{
			numResourcesLoaded += 1;
		}

		// count loaded collision data
		if ( info.m_collision )
		{
			numCollisionLoaded += 1;
		}
	}

	// stats
	LOG_ENGINE( TXT("Sector data resource streaming flushed in %1.3fs"),
		timer.GetTimePeriod() );
	LOG_ENGINE( TXT("Sector data resources: (%d finished, %d failed), total %d loaded"), 
		numResourcesFinished, numResourcesFailed, numResourcesLoaded );
	LOG_ENGINE( TXT("Sector data collision: (%d finished, %d failed), total %d loaded"), 
		numCollisionFinished, numCollisionFailed, numCollisionLoaded );
}

void CSectorDataResourceLoader::PrefetchResources( const SectorData::PackedResourceRef* resources, const Uint32 numResources )
{
	PC_SCOPE_PIX( PrefetchResources );

	CTimeCounter timer;

	// generate list of files to load
	TDynArray< CDiskFile* > filesToLoad;
	filesToLoad.Reserve( numResources );
	for ( Uint32 i=0; i<numResources; ++i )
	{
		CDiskFile* file = GetResourceFileNoRef( resources[i] );
		if ( file != nullptr && !file->IsLoaded() )
		{
			filesToLoad.PushBack( file );
		}
	}

	// load files using batch load
	SResourceLoader::GetInstance().Load( nullptr, filesToLoad.TypedData(), filesToLoad.Size(), eResourceLoadingPriority_High, nullptr );

	// stats
	LOG_ENGINE( TXT("Sector data resource prefetch: loaded %d resources (out of %d) in %1.3fs"), 
		filesToLoad.Size(), numResources, timer.GetTimePeriod() );
}

CSectorDataResourceLoader::EResult CSectorDataResourceLoader::PrefetchResource( const SectorData::PackedResourceRef resInfo, const Int32 priorityBias/*=0*/ )
{
	// Prepare resource (map the file), this will also fail on common cases (invalid index, etc)
	const Uint32 mergedResourceID = resInfo.m_resourceIndex;
	if ( !PrepareResource( mergedResourceID ) )
		return eResult_Failed;

	// We must have a valid file reference here
	auto& info = m_resources[ mergedResourceID ];
	RED_FATAL_ASSERT( info.m_file != nullptr, "Resource has invalid file pointer" );

	// Do we need to load the resource ?
	if ( info.m_resource )
		return eResult_Loaded;

	// HACK - get the depot path
	// TODO: push the path hashes further down the pipeline
	const String depotPath = info.m_file->GetDepotPath();

	// Start streaming
	if ( info.m_loadingJob == nullptr )
	{
		info.m_loadingJob = new CJobLoadResource( depotPath, JP_StreamingResource );
		SJobManager::GetInstance().Issue( info.m_loadingJob );

		// we are not ready yet
		return eResult_NotReady;
	}

	// still running
	if ( !info.m_loadingJob->HasEnded() )
		return eResult_NotReady;

	// finished, is it valid ?
	if ( !info.m_loadingJob->HasFinishedWithoutErrors() )
	{
		info.m_loadingJob->Release();
		info.m_loadingJob = nullptr;
		info.m_numRefs = INVALID_RESOURCE;
		return eResult_Failed;
	}

	// valid resource loaded, extract the resource
	CJobLoadResource* loadJob = static_cast< CJobLoadResource* >( info.m_loadingJob );
	info.m_resource = loadJob->GetResource();
	RED_FATAL_ASSERT( info.m_resource != nullptr, "Job succeeded but there's no resource" );

	// release the finished job
	info.m_loadingJob->Release();
	info.m_loadingJob = nullptr;

	// resource is loaded
	RED_FATAL_ASSERT( info.m_resource.IsValid(), "Well, no valid resource" );
	return eResult_Loaded;
}

CSectorDataResourceLoader::EResult CSectorDataResourceLoader::PrefetchCollision( const SectorData::PackedResourceRef resInfo, const Int32 priorityBias/*=0*/ )
{
	// Prepare resource (map the file), this will also fail on common cases (invalid index, etc)
	const Uint32 mergedResourceID = resInfo.m_resourceIndex;
	if ( !PrepareResource( mergedResourceID ) )
		return eResult_Failed;
	
	// We must have a valid file reference here
	auto& info = m_resources[ mergedResourceID ];
	RED_FATAL_ASSERT( info.m_file != nullptr, "Resource has invalid file pointer" );

	// use loaded collision
	if ( info.m_collision )
	{
		info.m_loadingCollision = false; // finished
		return eResult_Loaded;
	}

	// HACK - get the depot path
	// TODO: push the path hashes further down the pipeline
	const String depotPath = info.m_file->GetDepotPath();

	// load collision
	CompiledCollisionPtr ptr;
	Red::System::DateTime emptyTime;
	const auto ret = GCollisionCache->FindCompiled( ptr, depotPath, emptyTime );
	if ( ret == ICollisionCache::eResult_Invalid )
	{
		// store it
		info.m_loadingCollision = false; // finished
		info.m_collision = CompiledCollisionPtr();
		return eResult_Failed;
	}
	else if ( ret != ICollisionCache::eResult_Valid )
	{
		info.m_loadingCollision = true;
		return eResult_NotReady; // we are still loading the data
	}

	// store it
	info.m_loadingCollision = false; // finished
	info.m_collision = ptr;
	return eResult_Loaded;
}

CDiskFile* CSectorDataResourceLoader::GetResourceFileNoRef( const SectorData::PackedResourceRef resInfo )
{
	// Prepare resource (map the file), this will also fail on common cases (invalid index, etc)
	const Uint32 mergedResourceID = resInfo.m_resourceIndex;
	if ( !PrepareResource( mergedResourceID ) )
		return nullptr;

	// Return mapped file
	const auto& info = m_resources[ mergedResourceID ];
	return info.m_file;
}

Box CSectorDataResourceLoader::GetResourceBoxNoRef( const SectorData::PackedResourceRef res )
{
	if ( res.m_resourceIndex )
		return m_sourceData->GetResourceLocalBounds( res.m_resourceIndex );

	return Box::EMPTY;
}

THandle< CResource > CSectorDataResourceLoader::GetResourceAddRef( const SectorData::PackedResourceRef resInfo )
{
	// Prepare resource (map the file), this will also fail on common cases (invalid index, etc)
	const Uint32 mergedResourceID = resInfo.m_resourceIndex;
	if ( !PrepareResource( mergedResourceID ) )
		return nullptr;

	// Resource not yet loaded
	auto& res = m_resources[ mergedResourceID ];
	if ( !res.m_resource )
		return nullptr;

	// Add reference
	res.m_numRefs += 1;
	return res.m_resource;
}

CompiledCollisionPtr CSectorDataResourceLoader::GetCollisionAddRef( const SectorData::PackedResourceRef resInfo )
{
	// Prepare resource (map the file), this will also fail on common cases (invalid index, etc)
	const Uint32 mergedResourceID = resInfo.m_resourceIndex;
	if ( !PrepareResource( mergedResourceID ) )
		return CompiledCollisionPtr();

	// Resource not yet loaded
	auto& res = m_resources[ mergedResourceID ];
	if ( !res.m_collision )
		return CompiledCollisionPtr();

	// Add reference
	res.m_numRefs += 1;
	return res.m_collision;
}

void CSectorDataResourceLoader::CancelPrefetch( const SectorData::PackedResourceRef resInfo )
{
	// We must have a valid file reference here
	const Uint32 mergedResourceID = resInfo.m_resourceIndex;
	auto& info = m_resources[ mergedResourceID ];
	RED_FATAL_ASSERT( info.m_file != nullptr, "Resource has invalid file pointer" );

	// resource is in use, do nothing
	if ( info.m_numRefs > 0 )
		return;

	// if resource is not loaded yet we don't have to load it any more
	if ( info.m_loadingJob )
	{
		info.m_loadingJob->Cancel();
		info.m_loadingJob->Release();
		info.m_loadingJob = nullptr;
	}
}

void CSectorDataResourceLoader::Release( const SectorData::PackedResourceRef resInfo )
{
	const Uint32 mergedResourceID = resInfo.m_resourceIndex;
	RED_FATAL_ASSERT( mergedResourceID < MAX_RESOURCES, "Resource index (%d) is out of bounds (%d)", mergedResourceID, MAX_RESOURCES );
	auto& info = m_resources[ mergedResourceID ];

	// resource is invalid (failed to load), there's no way to release it
	if ( info.m_numRefs == INVALID_RESOURCE )
		return;

	RED_FATAL_ASSERT( info.m_numRefs > 0, "Resource index (%d) is already released", mergedResourceID );
	if ( info.m_numRefs == 0  )
		return;

	info.m_numRefs -= 1;

	// cleanup local stuff
	if ( !info.m_numRefs )
	{
		// potential fix for the growing resoure cache ?
		if ( CMesh* mesh = Cast< CMesh >( info.m_resource ) )
		{
			LOG_CORE( TXT("Render resource '%ls' released"), mesh->GetDepotPath().AsChar() );
			mesh->ReleaseRenderResource();
		}
		/*else if ( CBitmapTexture* tex = Cast< CBitmapTexture >( info.m_resource ) )
		{
			LOG_CORE( TXT("Render resource '%ls' released"), tex->GetDepotPath().AsChar() );
			mesh->ReleaseRenderResource();
		}*/

		info.m_resource = nullptr;
		info.m_collision.Reset();
	}
}

Bool CSectorDataResourceLoader::PrepareResource( const Uint32 mergedResourceID )
{
	// Empty resource is considered invalid
	if ( !mergedResourceID )
		return false;

	// Resource index is out of bounds
	RED_FATAL_ASSERT( mergedResourceID < MAX_RESOURCES, "Resource index (%d) is out of bounds (%d)",
		mergedResourceID, MAX_RESOURCES );

	// already failed
	auto& info = m_resources[ mergedResourceID ];
	if ( info.m_numRefs == INVALID_RESOURCE )
		return false;

	// map if not mapped
	if ( info.m_file == nullptr )
	{
		const auto pathHash = m_sourceData->GetResourcePathHash( mergedResourceID );
		info.m_file = GDepot->FindFile( pathHash );
		RED_ASSERT( info.m_file != nullptr, TXT("Invalid file - how was this shit cooked then ?") );

		// mark as invalid resource
		if ( !info.m_file )
		{
			info.m_numRefs = INVALID_RESOURCE;
			return false;
		}
	}

	// valid resource
	return true;
}
