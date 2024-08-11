/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "encounterSpawnPoints.h"

#include "encounter.h"
#include "gameWorld.h"
#include "wayPointsCollectionsSet.h"

template< class T >
class TDeleteGuard
{
	T* m_t;
public:
	TDeleteGuard( T* t ) : m_t( t )												{}
	~TDeleteGuard()																{ delete m_t; }
};

///////////////////////////////////////////////////////////////////////////////
// SEncounterSpawnPointData
///////////////////////////////////////////////////////////////////////////////
CWayPointComponent* SEncounterSpawnPointData::GetWaypoint( CWorld* world, const CWayPointsCollection& collection, const CWayPointsCollection::SWayPointHandle& baseData )
{
	{
		CWayPointComponent* wp = m_waypoint.Get();
		if ( wp )
		{
			return wp;
		}
	}
	

	CWayPointsCollection::SComponentMapping* componentMapping = collection.GetComponentMapping( baseData );
	if ( !componentMapping )
	{
		return nullptr;
	}

	CEntity* entity = world->FindEntity( componentMapping->m_entityGuid );
	if ( !entity )
	{
		return nullptr;
	}

	CComponent* component = entity->FindComponent( componentMapping->m_componentGuid );
	if ( !component || !component->IsA< CWayPointComponent >() )
	{
		return nullptr;
	}

	CWayPointComponent* wp = static_cast< CWayPointComponent* >( component );
	m_waypoint = wp;
	return wp;
}


///////////////////////////////////////////////////////////////////////////////
// SEncounterSpawnGroupIterator
///////////////////////////////////////////////////////////////////////////////
SEncounterSpawnGroupIterator::SEncounterSpawnGroupIterator( CEncounterSpawnPoints& sp, CWayPointsCollection::GroupIndex groupIndex )
	: m_collection( *sp.GetCollection() )
	, m_runtimeData( sp.GetRuntimeData() )
{
	const CWayPointsCollection::SWayPointsGroup& group = m_collection.GetWPGroup( groupIndex );
	m_baseWP = group.GetIndexes( m_collection );
	m_waypointsCount = group.m_waypointsCount;
	Reset();
}

void SEncounterSpawnGroupIterator::Reset()
{
	m_waypointsLeft = m_waypointsCount;
	m_wp = m_baseWP;
}


///////////////////////////////////////////////////////////////////////////////
// SRandomizedEncounterSpawnGroupIterator
///////////////////////////////////////////////////////////////////////////////
SRandomizedEncounterSpawnGroupIterator::SRandomizedEncounterSpawnGroupIterator( CEncounterSpawnPoints& sp, CWayPointsCollection::GroupIndex groupIndex )
	: SEncounterSpawnGroupIterator( sp, groupIndex )
	, m_isLooped( false )
{
	m_randomizedPoint = GEngine->GetRandomNumberGenerator().Get< Uint32 >( m_waypointsCount );
	m_waypointsLeft -= m_randomizedPoint;
	m_wp += m_randomizedPoint;
}

void SRandomizedEncounterSpawnGroupIterator::Reset()
{
	SEncounterSpawnGroupIterator::Reset();
	Uint32 r = GEngine->GetRandomNumberGenerator().Get< Uint32 >( m_waypointsCount );
	m_waypointsLeft -= r;
	m_wp += r;
}

///////////////////////////////////////////////////////////////////////////////
// CEncounterSpawnPoints
///////////////////////////////////////////////////////////////////////////////
void CEncounterSpawnPoints::ComputeResourceFileName( CEncounter* encounter, String& outPath )
{
	const Char* fileExt = CWayPointsCollection::GetFileExtension();
	const CGUID& guid = encounter->GetGUID();
	outPath = String::Printf( TXT("enc_%04x_%04x_%04x_%04x.%s")
		, guid.parts.A
		, guid.parts.B
		, guid.parts.C
		, guid.parts.D
		, fileExt
		);
}
void CEncounterSpawnPoints::ComputeResourceDepotPath( CWorld* world, CEncounter* encounter, String& outPath )
{
	const String& depotPath = world->GetDepotPath();
	const Char* fileExt = CWayPointsCollection::GetFileExtension();
	const CGUID& guid = encounter->GetGUID();
	outPath = String::Printf( TXT("%s/navi_cooked/enc_%04x_%04x_%04x_%04x.%s")
		, depotPath.AsChar()
		, guid.parts.A
		, guid.parts.B
		, guid.parts.C
		, guid.parts.D
		, fileExt
		);
}

Bool CEncounterSpawnPoints::Initialize( CGameWorld* world, CEncounter* encounter )
{
	if ( m_collection.IsValid() )
	{
		return true;
	}
#ifdef NO_RUNTIME_WAYPOINT_COOKING
	CWayPointsCollectionsSet* cookedWaypoints = world->GetCookedWaypoints();
	if ( !cookedWaypoints )
	{
		return false;
	}
	CWayPointsCollection* collection = cookedWaypoints->GetWayPointsCollection( encounter->GetGUID() );
	if ( !collection )
	{
		return false;
	}
#else
	CWayPointCookingContext* cookingContext = world->GetWaypointCookingContext();
	if ( !cookingContext )
	{
		return false;
	}
	CWayPointsCollection::Input* input = encounter->ComputeWaypointCollectionCookingInput( world, cookingContext );
	if ( !input )
	{
		return false;
	}
	TDeleteGuard< CWayPointsCollection::Input > g( input );
	THandle< CWayPointsCollection > collection = CreateObject< CWayPointsCollection >( encounter );

	if ( !collection->Create( *input ) )
	{
		collection->Discard();
		return false;
	}
#endif
	m_collection = collection;

	// initialize runtime data!
	m_runtimeData.Resize( collection->GetWaypointsCount() );

	// notice every encounter entry of SpawnPoints collection
	encounter->OnSpawnPointsCollectionLoaded();

	return true;
}

CEncounterSpawnPoints::GroupRuntimeIndex CEncounterSpawnPoints::GetGroupRuntimeIndex( GroupId groupId )
{
	CWayPointsCollection* collection = m_collection.Get();
	return collection->GetWPGroupIndexById( groupId );
}

//Bool CEncounterSpawnPoints::Request()
//{
//	if ( !m_isLoaded )
//	{
//		if ( m_collection.GetAsync() != BaseSoftHandle::ALR_Loaded )
//		{
//			return false;
//		}
//
//		CWayPointsCollection* collection = m_collection.Get();
//
//		m_runtimeData.Resize( collection->GetWaypointsCount() );
//		
//	}
//	return true;
//}
//
//void CEncounterSpawnPoints::Release()
//{
//	m_collection.Release();
//	m_runtimeData.Clear();
//	m_isLoaded = false;
//}

SEncounterSpawnPoint CEncounterSpawnPoints::GetRandomSpawnPoint()
{
	CWayPointsCollection* collection = m_collection.Get();

	Uint32 r = GEngine->GetRandomNumberGenerator().Get< Uint32 >( collection->GetWaypointsCount() );

	const CWayPointsCollection::SWayPointHandle& wp = collection->GetWaypoint( r );

	SEncounterSpawnPoint p( &wp, &m_runtimeData[ r ], collection );
	return p;
}

void CEncounterSpawnPoints::OnSerialize( IFile& file )
{
#ifndef NO_RUNTIME_WAYPOINT_COOKING
	if ( file.IsGarbageCollector() )
	{
		file << m_collection;
	}
#endif
}


