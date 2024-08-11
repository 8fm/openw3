/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "sectorData.h"
#include "sectorDataGlobals.h"
#include "sectorDataStreamingContext.h"
#include "sectorDataResourceLoader.h"
#include "sectorDataObjects.h"
#include "sectorDataMerged.h"
#include "renderer.h"

#include "../core/configVar.h"
#include "../core/streamingGrid.h"
#include "../core/streamingGridHelpers.h"

//#define PARANOID_STREAMING_CHECKS

namespace Config
{
	TConfigVar< Bool >	cvSectorAllowStreaming( "Streaming/Sectors", "AllowStreaming", true );
	TConfigVar< Bool >	cvSectorForceUnstream( "Streaming/Sectors", "ForceUnstream", false );
	TConfigVar< Bool >	cvSectorShowAllSectors( "Streaming/Sectors", "ForceShowAllSectors", false );
	TConfigVar< Bool >	cvSectorForceRefreshVisibilityMask( "Streaming/Sectors", "ForceRefreshVisibilityMask", false );
	TConfigVar< Int32 >	cvSectorMinVisibleSectorID( "Streaming/Sectors", "SectorMinVisibleSectorID", 0 );
	TConfigVar< Int32 >	cvSectorMaxVisibleSectorID( "Streaming/Sectors", "SectorMaxVisibleSectorID", 65535 );
}

CSectorDataStreamingContext::CSectorDataStreamingContext( const Float worldSize, class IRenderScene* renderScene, class CPhysicsWorld* physicsScene, class CDynamicLayer* dynamicLayer, const Bool hasPrefetchData, class CClipMap* terrainClipMap )
	: m_worldSize( worldSize )
	, m_renderScene( renderScene )
	, m_physicsScene( physicsScene )
	, m_dynamicLayer( dynamicLayer )
	, m_loadingLocked( false )
	, m_terrainClipMap( terrainClipMap )
{
	// create data management
	m_runtimeData = new CSectorDataMerged( worldSize, hasPrefetchData );
	m_runtimeResourceLoader = new CSectorDataResourceLoader( m_runtimeData );
	m_runtimeObjectManager = new CSectorDataObjectWrapper( m_runtimeData );	
}

CSectorDataStreamingContext::~CSectorDataStreamingContext()
{
	UnstreamAll();		// Ensure all objects are destroyed

	delete m_runtimeResourceLoader;
	m_runtimeResourceLoader = nullptr;

	delete m_runtimeObjectManager;
	m_runtimeObjectManager = nullptr;

	delete m_runtimeData;
	m_runtimeData = nullptr;
}

#define USE_DURANGO_CERTHACK
#if defined( RED_PLATFORM_DURANGO ) && defined( USE_DURANGO_CERTHACK )
# define STREAMING_DURANGO_CERTHACK() PUMP_MESSAGES_DURANGO_CERTHACK()
#else
# define STREAMING_DURANGO_CERTHACK()
#endif

void CSectorDataStreamingContext::UnstreamAll()
{
	// processing context
	ISectorDataObject::Context streamContext;
	streamContext.m_dynamicLayer = m_dynamicLayer;
	streamContext.m_physicsScene = m_physicsScene;
	streamContext.m_terrainClipMap = m_terrainClipMap;
	streamContext.m_renderScene = m_renderScene;
	streamContext.m_resourceLoader = m_runtimeResourceLoader;
	streamContext.m_instantUnload = true;		// Unstream assets without delay

	for ( Uint32 i=0; i<m_inRangeObjects.Size(); ++i )
	{
		const auto& info = m_inRangeObjects[i];

		// Clean up the in-range mask
		m_inRangeMask.Clear( info.m_id );

		// is object streamed ? if so, we need to unstream it
		if ( m_streamedMask.Get( info.m_id ) )
		{
			// perform synchronous unstreaming, this should ALWAYS succeed, or we will leak
			const auto ret = info.m_object->Unstream( streamContext, false );
			RED_FATAL_ASSERT( ret == ISectorDataObject::eResult_Finished, "Failed to unstream an object" );

			// remove from streaming set
			m_streamedMask.Clear( info.m_id );
		}

		// delete the wrapper
		delete info.m_object;
	}

#ifndef RED_FINAL_BUILD
	// Test that everything was removed properly
	for( Uint32 i=0; i < SectorData::MAX_OBJECTS; ++i )
	{
		if( m_streamedMask.Get( i ) )
		{
			RED_FATAL_ASSERT( false, "Failed to clear streamed object mask" );
		}
	}
#endif

	m_inRangeObjects.ClearFast();
	m_streamedObjects.ClearFast();
}

void CSectorDataStreamingContext::FlushResourceLoading()
{
	m_runtimeResourceLoader->Flush();
}

void CSectorDataStreamingContext::PreloadResources( const Vector& referenceCameraPosition, const Float range )
{
	PC_SCOPE_PIX( PreloadResources );

	// make sure visibility mask is up to date
	RefreshVisibilityMask();

	// get all resources in given area
	TDynArray< SectorData::PackedResourceRef > resourceIndices;
	m_runtimeData->CollectResourcesFromLocation( referenceCameraPosition, range, resourceIndices );

	// load resources
	m_runtimeResourceLoader->PrefetchResources( resourceIndices.TypedData(), resourceIndices.Size() );
	m_runtimeResourceLoader->Flush();
}

void CSectorDataStreamingContext::SetLoadingLock( const Bool isLoadingLocked )
{
	m_loadingLocked = isLoadingLocked;
}

void CSectorDataStreamingContext::ProcessAsync( CSectorDataStreamingContextThreadData& threadData, const Vector& referenceCameraPosition, Bool instantUnloads, const Bool forceStream/* = false*/ )
{
	PC_SCOPE_PIX( StreamingAsync );

	// make sure visibility mask is up to date
	RefreshVisibilityMask();

	STREAMING_DURANGO_CERTHACK();

	// processing context
	ISectorDataObject::Context streamContext;
	streamContext.m_dynamicLayer = m_dynamicLayer;
	streamContext.m_physicsScene = m_physicsScene;
	streamContext.m_terrainClipMap = m_terrainClipMap;
	streamContext.m_renderScene = m_renderScene;
	streamContext.m_resourceLoader = m_runtimeResourceLoader;
	streamContext.m_instantUnload = instantUnloads;
	streamContext.m_forceStream = forceStream;

	// Query the grid
	TStreamingGridCollectorStack< SectorData::MAX_STREAMED_OBJECTS > entitiesInRange;
	if ( !Config::cvSectorForceUnstream.Get() )
	{
		PC_SCOPE_PIX( Query );
		m_runtimeData->CollectForPoint( referenceCameraPosition, entitiesInRange );
		STREAMING_DURANGO_CERTHACK();
	}

	// Sort objects by distance from camera
	{
		PC_SCOPE_PIX( Sort );
		entitiesInRange.Sort();
		STREAMING_DURANGO_CERTHACK();
	}

	// Build query bitfield - a bit for every object in the range
	threadData.m_queryMask.ClearAll(); // 80KB memset
	for ( Uint32 i=0; i<entitiesInRange.Size(); ++i )
	{
		const Uint32 id = entitiesInRange[i];
		threadData.m_queryMask.Set( id );
		STREAMING_DURANGO_CERTHACK();
	}

	// Add locked object to the query (to make sure there are streamed REGARDLESS if they are in range or not)
	if ( !m_lockedObjectIndices.Empty() )
	{
		PC_SCOPE_PIX( ApplyStreamingLock );

		for ( Uint32 i=0; i<m_lockedObjectIndices.Size(); ++i )
		{
			const Uint32 id = m_lockedObjectIndices[i];
			if ( !threadData.m_queryMask.Get( id ) )
			{
				// add to query result
				threadData.m_queryMask.Set( id );
				entitiesInRange.Add( id, 0 ); // forced distance 0 - stream ASAP
			}
		}
		STREAMING_DURANGO_CERTHACK();
	}

	// Create new wrappers for objects that just got in range
	{
		PC_SCOPE_PIX( CreateWrappers );
		for ( Uint32 i=0; i<entitiesInRange.Size(); ++i )
		{
			const Uint32 id = entitiesInRange[i];

			// get/create the object wrapper but only for valid objects
			ISectorDataObject* object = nullptr;
			if ( !m_inRangeMask.Get( id ) && m_visibilityMask.Get( id ) )
			{
				// create new wrapper
				object = m_runtimeObjectManager->CreateObjectWrapper( id );
				RED_FATAL_ASSERT( object != nullptr, "Failed to create object wrapper for %d", id );

				// make sure we are not adding a duplicate
#if !defined(RED_FINAL_BUILD) && defined(PARANOID_STREAMING_CHECKS)
				for ( Uint32 i=0; i<m_inRangeObjects.Size(); ++i )
				{
					const auto& a = m_inRangeObjects[i];
					RED_FATAL_ASSERT( a.m_id != id, "Duplicated object in the inrange!" );
				}
#endif

				// register wrapper
				m_inRangeMask.Set( id );
				m_inRangeObjects.PushBack( CSectorDataObjectInfo( id, object ) );
			}
		}
		STREAMING_DURANGO_CERTHACK();
	}

	// reset temp lists
	threadData.m_tempInRangeObjectList.ClearFast();
	threadData.m_tempStreamedObjectList.ClearFast();
	threadData.m_syncStreamObjects.ClearFast();
	threadData.m_syncUnstreamObjects.ClearFast();
	STREAMING_DURANGO_CERTHACK();

	// Perform streaming - process all of the objects in range (with wrappers) that are not yet stream and are in the query distance
	// build the new list of object in range - we only should have there objects that are truly in range
	{
		PC_SCOPE_PIX( ProcessObjects );
		for ( Uint32 i=0; i<m_inRangeObjects.Size(); ++i )
		{
			const auto& info = m_inRangeObjects[i];

			RED_FATAL_ASSERT( m_inRangeMask.Get( info.m_id ), "In range object is not marked as in range" );

			// process only objects that are truly in range
			if ( threadData.m_queryMask.Get( info.m_id ) && m_visibilityMask.Get( info.m_id ) )
			{
				// this object is in range of the old list and current query, keep it
				threadData.m_tempInRangeObjectList.PushBack( info );

				// should we stream this object ? it should be in range of the query and not yet streamed
				if ( !m_streamedMask.Get( info.m_id ))
				{
					// load only if allowed
					if ( !m_loadingLocked && Config::cvSectorAllowStreaming.Get() )
					{
						// having the wrapper, try to perform streaming		
						const auto ret = info.m_object->Stream( streamContext, /*async*/ true );
						RED_FATAL_ASSERT( ret == ISectorDataObject::eResult_Finished || ret == ISectorDataObject::eResult_RequiresSync || ret == ISectorDataObject::eResult_NotReady || ret == ISectorDataObject::eResult_Failed, "Unsupported return code" );

						// we have successfully performed streaming
						if ( ret == ISectorDataObject::eResult_Finished || ret == ISectorDataObject::eResult_RequiresSync )
						{
							// make sure we are not adding a duplicate
	#if !defined(RED_FINAL_BUILD) && defined(PARANOID_STREAMING_CHECKS)
							for ( Uint32 i=0; i<m_streamedObjects.Size(); ++i )
							{
								const auto& a = m_streamedObjects[i];
								RED_FATAL_ASSERT( a.m_id != info.m_id, "Duplicated object in the streaming list!" );
							}
	#endif

							// mark as streamed
							m_streamedMask.Set( info.m_id );
							m_streamedObjects.PushBack( info );

							// add object to new list of streamed objects
							threadData.m_tempStreamedObjectList.PushBack( info );

							// we will have to perform a little bit more of processing on the main thread
							if ( ret == ISectorDataObject::eResult_RequiresSync )
							{
								threadData.m_syncStreamObjects.PushBack( info );
							}
						}
					}
				}
				else
				{
					// object is still in range and is already streamed, keep it
					threadData.m_tempStreamedObjectList.PushBack( info );
				}
			}
			else
			{
				// object is not visible or in the query range, we should unstream and delete it
				m_inRangeMask.Clear( info.m_id );

				// is object streamed ? if so, we need to unstream it first
				Bool canDelete = true;
				if ( m_streamedMask.Get( info.m_id ) )
				{
					// perform async unstreaming, this may require finishing at the sync stage
					const auto ret = info.m_object->Unstream( streamContext, /*async*/ true );
					RED_FATAL_ASSERT( ret == ISectorDataObject::eResult_Finished || ret == ISectorDataObject::eResult_RequiresSync, "Unsupported return code" );

					// remove from streaming set
					m_streamedMask.Clear( info.m_id );

					// we have a request to finish unstreaming on the main thread
					if ( ret == ISectorDataObject::eResult_RequiresSync )
					{
						threadData.m_syncUnstreamObjects.PushBack( info );
						canDelete = false;
					}
				}

				// delete the wrapper
				if ( canDelete )
				{
					// it's free to delete the wrapper now, it's not going to be touched any more
					delete info.m_object;
				}
			}
		}
		STREAMING_DURANGO_CERTHACK();
	}

	// Swap the list to valid ones
	{
		PC_SCOPE_PIX( SwapLists );

		// swap the in-range list to valid state again
		m_inRangeObjects = threadData.m_tempInRangeObjectList;
		threadData.m_tempInRangeObjectList.ClearFast();

		m_streamedObjects = threadData.m_tempStreamedObjectList;
		threadData.m_tempStreamedObjectList.ClearFast();
		STREAMING_DURANGO_CERTHACK();
	}

	// Validate lists consistency
#ifndef RED_FINAL_BUILD
	for ( Uint32 i=0; i<m_streamedObjects.Size(); ++i )
	{
		const auto info = m_streamedObjects[i];
		RED_FATAL_ASSERT( m_streamedMask.Get(info.m_id) == true, "Streamed object not flag as streamed" );
		RED_FATAL_ASSERT( m_inRangeMask.Get(info.m_id) == true, "Streamed object not flag as in-range" );
	}

	for ( Uint32 i=0; i<m_inRangeObjects.Size(); ++i )
	{
		const auto info = m_inRangeObjects[i];
		RED_FATAL_ASSERT( m_inRangeMask.Get(info.m_id) == true, "Inrange mask not flag as in-range" );
	}

	RED_FATAL_ASSERT( m_streamedObjects.Size() <= m_inRangeObjects.Size(), "More inrange objects that streamed objects" );
#endif
}

void CSectorDataStreamingContext::ProcessSync( CSectorDataStreamingContextThreadData& threadData, Bool instantUnloads, const Bool forceStream/* = false*/ )
{
	PC_SCOPE_PIX( StreamingSync );

	// processing context
	ISectorDataObject::Context streamContext;
	streamContext.m_dynamicLayer = m_dynamicLayer;
	streamContext.m_physicsScene = m_physicsScene;
	streamContext.m_terrainClipMap = m_terrainClipMap;
	streamContext.m_renderScene = m_renderScene;
	streamContext.m_resourceLoader = m_runtimeResourceLoader;
	streamContext.m_instantUnload = instantUnloads;
	streamContext.m_forceStream = forceStream;

	// finalize synchronous streaming
	for ( Uint32 i=0; i<threadData.m_syncStreamObjects.Size(); ++i )
	{
		const auto info = threadData.m_syncStreamObjects[i];

		// validate state
		RED_FATAL_ASSERT( m_inRangeMask.Get( info.m_id ), "Streamed object is not in the inrange list" );
		RED_FATAL_ASSERT( m_streamedMask.Get( info.m_id ), "Streamed object is not in the streamed list" );

		// finalize streaming call for the objects that returned sResult_RequiresSync
		const auto ret = info.m_object->Stream( streamContext, /*async*/ false );
		RED_FATAL_ASSERT( ret == ISectorDataObject::eResult_Finished || ret == ISectorDataObject::eResult_NotReady, "Synchronous call scheduled but returned invalid code when processed" );
		RED_ASSERT( ret == ISectorDataObject::eResult_Finished, TXT("Synchronous call scheduled but was unable to finish") );

		// we are still not ready even though we said we will, LIES, LIES, EVERYBODY LIES
		// example: adding physical object to tiles, the GetTerrainAtPoint is like a woman - it will tell you she's ready one frame and on the next it will tell you her head hurts.
		if ( ret == ISectorDataObject::eResult_NotReady )
		{
			PC_SCOPE_PIX( RevertingStreamingObject );

			// remove from the mask of streamed objects, we were lied to
			m_streamedMask.Clear( info.m_id );

			// linear search is needed to back out the object from the list of streamed objects
			for ( Uint32 i=0; i<m_streamedObjects.Size(); ++i )
			{
				if ( m_streamedObjects[i].m_object == info.m_object )
				{
					// we don't need to keep the list ordered so a RemoveAtFast will do
					m_streamedObjects.RemoveAtFast( i );
					break;
				}
			}
		}
	}

	// finalize synchronous unstreaming
	for ( Uint32 i=0; i<threadData.m_syncUnstreamObjects.Size(); ++i )
	{
		const auto info = threadData.m_syncUnstreamObjects[i];

		// validate state
		RED_FATAL_ASSERT( !m_inRangeMask.Get( info.m_id ), "Streamed object is still in the inrange list" );
		RED_FATAL_ASSERT( !m_streamedMask.Get( info.m_id ), "Streamed object is still in the streamed list" );

		// finalize unstreaming call for the objects that returned sResult_RequiresSync
		const auto ret = info.m_object->Unstream( streamContext, /*async*/ false );
		RED_FATAL_ASSERT( ret == ISectorDataObject::eResult_Finished, "Synchronous call scheduled but was unable to finish" );

		// delete the object now
		delete info.m_object;
	}
}

void CSectorDataStreamingContext::RefreshVisibilityMask()
{
	// refresh only if it's dirty
	if ( m_visibilityMaskDirty.Exchange( false ) || Config::cvSectorForceRefreshVisibilityMask.Get() )
	{
		PC_SCOPE_PIX( RefreshVisibilityMask );

		// clear current mask
		m_visibilityMask.ClearAll();

		// mark objects from valid sector
		Uint32 numVisibleObjects = 0;
		const Uint32 numAllObjects = m_runtimeData->GetNumObjects();
		for ( Uint32 objectId=1; objectId<numAllObjects; ++objectId )
		{
			const auto& object = m_runtimeData->GetObject(objectId);

			// basic visibility test
			Bool isVisible = (object.m_sectorID && m_visibleSectors.Get( object.m_sectorID ));

#ifndef RED_FINAL_BUILD
			// additional visibility test
			isVisible &= ((Int32)object.m_sectorID >= Config::cvSectorMinVisibleSectorID.Get());
			isVisible &= ((Int32)object.m_sectorID <= Config::cvSectorMaxVisibleSectorID.Get());

			// forced visibility
			if ( Config::cvSectorShowAllSectors.Get() )
				isVisible = true;
#endif

			// add to visibility list only if sector is visible
			if ( isVisible )
			{
				m_visibilityMask.Set( objectId );
				numVisibleObjects += 1;
			}
		}

		// stats
		LOG_ENGINE( TXT("Filtered %d visible objects (out of %d)"), 
			numVisibleObjects, numAllObjects );
	}
}

Uint32 CSectorDataStreamingContext::AttachSectorData( const Uint64 contentID, const CSectorData* sectorData, const Bool isVisible )
{
	RED_FATAL_ASSERT( sectorData != nullptr, "NULL sector data" );
	RED_FATAL_ASSERT( sectorData->HasData(), "Sector data with no data should be filtered earlier" );

	// find existing content
	Uint32 sectorId = 0;
	if ( !m_sectorDataIds.Find( contentID, sectorId ) )
	{
		sectorId = m_runtimeData->AppendSectorData( sectorData );
		if ( !sectorId )
			return 0;

		m_sectorDataIds.Insert( contentID, sectorId );
	}

	// set sector visibility mask
	if ( isVisible )
		m_visibleSectors.Set( sectorId );
	else
		m_visibleSectors.Clear( sectorId );

	// request visibility mask to be refreshed
	m_visibilityMaskDirty.SetValue( true );

	// return id of the new sector
	return sectorId;
}

void CSectorDataStreamingContext::RemoveSectorData( const Uint32 sectorId )
{
	// remove sector data
	m_runtimeData->RemoveSectorData( sectorId );

	// remove stuff from visibility mask (this will unstream the objects on next pass)
	m_visibleSectors.Clear( sectorId );
	m_visibilityMaskDirty.Exchange( true );
}

void CSectorDataStreamingContext::ToggleSectorDataVisibility( const Uint32 sectorId, const Bool isVisible )
{
	if ( m_visibleSectors.Get( sectorId ) != isVisible )
	{
		// set sector visibility mask
		if ( isVisible )
			m_visibleSectors.Set( sectorId );
		else
			m_visibleSectors.Clear( sectorId );

		// print stats
#if !defined(RED_FINAL_BUILD) && !defined(RED_PLATFORM_CONSOLE)
		LOG_ENGINE( TXT("SectorID[%d].Visible: %ls"), sectorId, isVisible ? TXT("visible") : TXT("hidden") );
#endif

		// request visibility mask to be refreshed
		m_visibilityMaskDirty.SetValue( true );
	}
}

const Uint32 CSectorDataStreamingContext::GetNumStreamingResources() const
{
	return 0;
}

const Uint32 CSectorDataStreamingContext::GetNumStreamedObjects() const
{
	return m_streamedObjects.Size();
}

const Uint32 CSectorDataStreamingContext::GetNumStreamingObjects() const
{
	if ( m_inRangeObjects.Size() > m_streamedObjects.Size() )
		return m_inRangeObjects.Size() - m_streamedObjects.Size();

	return 0;
}

const Uint32 CSectorDataStreamingContext::GetNumLockedObjects() const
{
	return m_lockedObjectIndices.Size();
}

const Uint32 CSectorDataStreamingContext::GetNumInrangeObjects() const
{
	return m_inRangeObjects.Size();
}

const Uint32 CSectorDataStreamingContext::GetNumTotalObjects() const
{
	return m_runtimeData->GetNumObjects();
}

void CSectorDataStreamingContext::SetStreamingLock( const Box* bounds, const Uint32 numBounds )
{
	// collect objects from all areas and create one big merged list
	TStreamingGridCollectorStack< 65535 > entitiesInRange;
	m_runtimeData->CollectForAreas( bounds, numBounds, entitiesInRange );

	// clear current lock
	m_lockedMask.ClearAll();
	m_lockedObjectIndices.ClearFast();

	// add the entries to locked list
	if ( numBounds == 1 )
	{
		for ( Uint32 i=0; i<entitiesInRange.Size(); ++i )
		{
			const Uint32 id = entitiesInRange[i];
			m_lockedMask.Set( id );
			m_lockedObjectIndices.PushBack( id );
		}
	}
	else
	{
		// merge the results
		for ( Uint32 i=0; i<entitiesInRange.Size(); ++i )
		{
			const Uint32 id = entitiesInRange[i];
			if ( !m_lockedMask.Get( id ) )
			{
				m_lockedMask.Set( id );
				m_lockedObjectIndices.PushBack( id );
			}
		}
	}

	// set streaming lock
	LOG_ENGINE( TXT("Sector data streaming lock update: %d areas, %d object"), numBounds, m_lockedObjectIndices.Size() );
}