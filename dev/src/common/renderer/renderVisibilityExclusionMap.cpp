/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderVisibilityExclusionMap.h"
#include "../../common/engine/mergedMeshBuilder.h"

//-------------------------------------------

CRenderVisibilityExclusionList::CRenderVisibilityExclusionList( const GlobalVisID* ids, const Uint32 numIDs, const Uint8 renderMask, const Bool isEnabled )
	: m_isEnabled( isEnabled )
	, m_renderMask( renderMask )
{
	// prepare the global object list
	m_globalObjects.Resize( numIDs );
	for ( Uint32 i=0; i<numIDs; ++i )
		m_globalObjects[i] = ids[i];

	// local object IDs are not mapped yet
	m_localObjects.Resize( numIDs );
	Red::MemoryZero( m_localObjects.Data(), m_localObjects.DataSize() );

	// reserve and build the hashmap
	m_globalIndexMap.Reserve( numIDs );
	for ( Uint32 i=0; i<numIDs; ++i )
	{
		m_globalIndexMap.Insert( ids[i], i );
	}
}

CRenderVisibilityExclusionList::~CRenderVisibilityExclusionList()
{
}

void CRenderVisibilityExclusionList::SetState( const Bool isEnabled )
{
	m_isEnabled = isEnabled;
}

IRenderVisibilityExclusion* CRenderInterface::CreateVisibilityExclusion( const GlobalVisID* ids, const Uint32 numIDs, const Uint8 renderMask, const Bool isEnabled )
{
	if ( !numIDs || !ids )
		return nullptr;

	return new CRenderVisibilityExclusionList( ids, numIDs, renderMask, isEnabled );
}

Int32 CRenderVisibilityExclusionList::RegisterObject( const GlobalVisID globalId, const LocalVisObjectID localId )
{
	Uint16 index = 0;
	if ( !m_globalIndexMap.Find( globalId, index ) )
		return -1;

	// register new version regardless if we have the previos object (this handles the fade out of old proxies)
	m_localObjects[ index ] = localId;
	return index;
}

Bool CRenderVisibilityExclusionList::UnegisterObject( const GlobalVisID globalId, const LocalVisObjectID localId )
{
	Uint16 index = 0;
	if ( m_globalIndexMap.Find( globalId, index ) )
	{
		// unregister only the right object (this handles the fade out of old proxies)
		if ( m_localObjects[index] == localId )
		{
			m_localObjects[index] = 0;
		}

		return true;
	}

	return false;
}

//-------------------------------------------

CRenderVisibilityExclusionMap::CRenderVisibilityExclusionMap()
{
#ifdef NO_EDITOR
	const Uint32 maxProxies = 128*1024;
#else
	const Uint32 maxProxies = 512*1024;
#endif

	/// prepare ID allocator, we should not have more than 64K active proxies in this system
	m_idAllocator.Resize( maxProxies );

	/// prepare proxy tables
	m_objectProxies.Resize( maxProxies ); // 1 MB in cooked game
	m_objectFilterMasks.Resize( maxProxies ); // 0.256 MB in cooked game

	/// prepare initial data
	m_maxValidLocalId = 1;
	m_idAllocator.Alloc();

	m_globalToLocalMap.Reserve( 30 * 1024 );
}

CRenderVisibilityExclusionMap::~CRenderVisibilityExclusionMap()
{	
}

LocalVisObjectID CRenderVisibilityExclusionMap::RegisterProxy( const IRenderProxyBase* proxy, const GlobalVisID globalID )
{
	// NOTE: no mutex here, caller is responsible for thread safety of this code

	RED_FATAL_ASSERT( globalID.IsValid(), "Trying to register vis object with no global ID" );
	RED_FATAL_ASSERT( proxy != nullptr, "Trying to register vis object with no proxy" );

	/// allocate object ID
	LocalVisObjectID localId = m_idAllocator.Alloc();
	RED_ASSERT( localId != 0, TXT("Out of entries for local object IDs in the visibility system - to many static proxies in range ?") );
	if ( localId == 0 )
		return 0;

	// keep track of maximum valid ID
	if ( localId >= m_maxValidLocalId )
		m_maxValidLocalId = localId + 1;

	/// add to map - note that we may already have a proxy for that globalID (fadeIn/fadeOut case) that's why we use Set instead of Insert
	m_globalToLocalMap.Set( globalID, localId );

	/// map the object if it occurs in any of the attached visibility lists
	Uint8 filterMask = 0xFF;
	for ( CRenderVisibilityExclusionList* list : m_exclusionLists )
	{
		// register object
		const Int32 index = list->RegisterObject( globalID, localId );
		if ( index != -1 )
		{
			// exclude from rendering in given group if list is enabled
			if ( list->IsEnabled() )
			{
				filterMask &= ~list->GetRenderMask();
			}

			break;
		}
	}

	/// insert the object information
	RED_FATAL_ASSERT( m_objectProxies[ localId ] == nullptr, "Unexpected content" );
	m_objectProxies[ localId ] = proxy;
	m_objectFilterMasks[ localId ] = filterMask;

	/// return local object ID
	return localId;
}

void CRenderVisibilityExclusionMap::UnregisterProxy( const IRenderProxyBase* proxy, const GlobalVisID globalID, const LocalVisObjectID localId )
{
	// NOTE: no mutex here, caller is responsible for thread safety of this code

	RED_FATAL_ASSERT( localId != 0, "Trying to unregister vis object with no local ID" );
	RED_FATAL_ASSERT( proxy != nullptr, "Trying to unregister vis object with no proxy" );

	/// validate entry
	RED_FATAL_ASSERT( localId < m_objectProxies.Size(), "Local vis object ID out of range" );
	RED_FATAL_ASSERT( m_objectProxies[ localId ] == proxy, "Mismatched local vis object ID (reused by another proxy)" );

	/// unregister from map but only if the pair matches
	LocalVisObjectID currentLocalId = 0;
	if ( m_globalToLocalMap.Find( globalID, currentLocalId ) )
	{
		if ( currentLocalId == localId )
		{
			m_globalToLocalMap.Erase( globalID );
		}
	}

	/// remove from visibility lists
	for ( auto* list : m_exclusionLists )
	{
		list->UnegisterObject( globalID, localId );
	}

	/// remove entry
	m_objectProxies[ localId ] = nullptr;
	m_objectFilterMasks[ localId ] = 0;

	/// reuse the ID
	m_idAllocator.Release( localId );
}

void CRenderVisibilityExclusionMap::AddList( CRenderVisibilityExclusionList* objectList )
{
	// NOTE: no mutex here, caller is responsible for thread safety of this code

	RED_FATAL_ASSERT( !m_exclusionLists.Exist( objectList ), "Object list already registered" );
	
	// add to lists
	m_exclusionLists.PushBack( objectList );

	// map all object from visibility list using current global->local map
	const Uint8 filterMask = ~objectList->GetRenderMask();
	const Uint32 numObjects = objectList->GetNumObjects();
	const auto* globalObjects = objectList->GetGlobalObjects();
	auto* localObjects = objectList->GetLocalObjects();
	Uint32 numObjectsFound = 0;
	Uint32 numObjectsFilteredAway = 0;
	for ( Uint32 i=0; i<numObjects; ++i )
	{
		RED_FATAL_ASSERT( localObjects[i] == 0, "Dirty list." );

		LocalVisObjectID localId = 0;
		if ( m_globalToLocalMap.Find( globalObjects[i], localId ) && localId )
		{
			if ( 0 != (m_objectFilterMasks[ localId ] & filterMask) )
				numObjectsFilteredAway += 1;

			m_objectFilterMasks[ localId ] &= filterMask;
			localObjects[i] = localId;
			numObjectsFound += 1;
		}
	}

	// stats
	LOG_RENDERER( TXT("Visibility list: %d objects found, %d not found, %d filtered away"), 
		numObjectsFound, objectList->GetNumObjects() - numObjectsFound, numObjectsFilteredAway );

	// add to list
	objectList->AddRef();
}

void CRenderVisibilityExclusionMap::RemoveList( CRenderVisibilityExclusionList* objectList )
{
	// NOTE: no mutex here, caller is responsible for thread safety of this code

	// remove form lists
	if ( m_exclusionLists.Remove( objectList ) )
	{
		// unmap all local objects
		const Uint32 numObjects = objectList->GetNumObjects();
		Red::MemoryZero( objectList->GetLocalObjects(), sizeof(LocalVisObjectID) * numObjects );

		// remove from dirty list but force global recompute
		m_fullRecompute = true;

		// remove reference
		objectList->Release();
	}
}

void CRenderVisibilityExclusionMap::MarkDirty( CRenderVisibilityExclusionList* objectList )
{
	// NOTE: no mutex here, caller is responsible for thread safety of this code
	m_fullRecompute = true;
}

void CRenderVisibilityExclusionMap::PrepareForQueries()
{
	// NOTE: no mutex here, caller is responsible for thread safety of this code
	// NOTE: full recompute for now only

	// nothing dirty :)
	if ( !m_fullRecompute )
		return;

	// reset masks
	Uint32 maxUsedObject = 0;
	for ( Uint32 i=0; i<m_objectProxies.Size(); ++i )
	{
		if ( m_objectProxies[i] != 0 )
		{
			m_objectFilterMasks[i] = 0xFF; // reset
			maxUsedObject = i + 1;
		}
	}

	// update local ID
	m_maxValidLocalId = maxUsedObject;

	// apply visibility masking
	for ( const CRenderVisibilityExclusionList* list : m_exclusionLists )
	{
		if ( list->IsEnabled() )
		{
			const Uint8 filterMask = ~list->GetRenderMask();

			const Uint32 numObjects = list->GetNumObjects();
			const LocalVisObjectID* localIds = list->GetLocalObjects();
			for ( Uint32 i=0; i<numObjects; ++i )
			{
				if ( localIds[i] )
				{
					m_objectFilterMasks[ localIds[i] ] &= filterMask; // visibility masking
				}
			}
		}
	}

	// cleanup state
	m_fullRecompute = false;
}

//-------------------------------------------

CRenderVisibilityExclusionTester::CRenderVisibilityExclusionTester()
	: m_mask( nullptr )
	, m_maxObjects( 0 )
{
}

CRenderVisibilityExclusionTester::~CRenderVisibilityExclusionTester()
{
}

void CRenderVisibilityExclusionTester::Setup( const CRenderVisibilityExclusionMap& visibilityMap )
{
	m_mask = visibilityMap.GetFilterMasks();
	m_maxObjects = visibilityMap.GetMaxValidLocalId();
}

//-------------------------------------------
