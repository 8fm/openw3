/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "wayPointCookingContext.h"

#include "wayPointsCollectionsSet.h"
#include "partySpawnPointComponent.h"

CWayPointCookingContext::CWayPointCookingContext()
	: m_queryStructuresInitialized( false )
{
}

CWayPointCookingContext::~CWayPointCookingContext()
{
	for ( const SCollections2Save& c : m_collections2Save )
	{
		delete c.m_collectionInput;
	}
}

void CWayPointCookingContext::LazyInitializeQueryStructures() const
{
	if ( m_queryStructuresInitialized )
	{
		return;
	}

	for ( const SWayPointCookingData& wp : m_allWaypoints )
	{
		const TagList::TTagList& tags = wp.m_tagList.GetTags();
		for ( CName tag : tags )
		{
			m_tagMapping[ tag ].PushBack( wp.m_id );
		}
	}

	m_queryStructuresInitialized = true;
}

void CWayPointCookingContext::RegisterWaypoint( CWayPointComponent* waypoint )
{
	CEntity* entity = waypoint->GetEntity();

	SComponentMapping idMapping;
	idMapping.m_entityGuid = entity->GetGUID();
	idMapping.m_componentGuid = waypoint->GetGUID();
	if ( GetWaypoint( idMapping ) != nullptr )
	{
		// We are re-collecting same object.
		// Its possible in editor mode as we can hide/show layers
		return;
	}

	SWayPointCookingData data;
	data.m_id = idMapping;
	data.m_position = waypoint->GetWorldPositionRef();
	data.m_orientation = waypoint->GetWorldRotation();
	data.m_waypointClass = waypoint->GetClass();
	data.m_tagList = waypoint->GetTags();
	data.m_tagList.AddTags( entity->GetTags() );
	data.m_actionPointDataIndex = -1;
	data.m_componentNameHash = waypoint->GetName().CalcHash();
	data.m_idTag = IdTag::Empty();

	if ( const CPeristentEntity* const persistentEntity = Cast< CPeristentEntity >( entity ) )
	{
		data.m_idTag = persistentEntity->GetIdTag();
	}

	CActionPointComponent* actionPointComponent = Cast< CActionPointComponent >( waypoint );
	if ( actionPointComponent )
	{
		data.m_actionPointDataIndex = m_actionPoints.Size();
		SActionPointCookingData apData;
		apData.m_categories = actionPointComponent->GetActionCategories();
		m_actionPoints.PushBack( Move( apData ) );
	}
	else if ( CPartySpawnPointComponent* const partyPointComponent = Cast< CPartySpawnPointComponent >( waypoint ) )
	{
		SPartyCookingData partyData;
		partyData.m_spawnerMapping.m_entityGuid = entity->GetGUID();
		partyData.m_spawnerMapping.m_componentGuid = waypoint->GetGUID();
		partyPointComponent->FillWaypointsData( partyData.m_partySpawnPoints );
		
		m_parties.PushBack( Move( partyData ) );
	}

	m_allWaypoints.Insert( Move( data ) );

	m_queryStructuresInitialized = false;
}

void CWayPointCookingContext::Clear()
{
	m_allWaypoints.ClearFast();
	m_actionPoints.ClearFast();
	m_parties.ClearFast();
	m_collections2Save.ClearFast();
	m_tagMapping.ClearFast();
	m_queryStructuresInitialized = false;
}

void CWayPointCookingContext::HandleWaypoints( Handler& handler, const Box& bbox )
{
	for ( const SWayPointCookingData& wp : m_allWaypoints )
	{
		if ( bbox.Contains( wp.m_position ) )
		{
			handler.Handle( this, wp );
		}
	}
}

void CWayPointCookingContext::ScheduleCollectionToSave( const CGUID& guid, CWayPointsCollection::Input* collection )
{
	SCollections2Save c;
	c.m_guid = guid;
	c.m_collectionInput = collection;
	m_collections2Save.PushBack( c );
}

const SWayPointCookingData* CWayPointCookingContext::GetWaypoint( const SComponentMapping& id ) const
{
	auto itFind = m_allWaypoints.Find( id );
	if ( itFind == m_allWaypoints.End() )
	{
		return nullptr;
	}
	return &(*itFind);
}

const SPartyCookingData* CWayPointCookingContext::GetParty( const SComponentMapping& id ) const
{
	for ( const SPartyCookingData& wp : m_parties )
	{
		if ( wp.m_spawnerMapping == id )
		{
			return &wp;
		}
	}
	return nullptr;
}

const SWayPointCookingData*	CWayPointCookingContext::GetWaypoint( const CGUID& entityGUID, Uint32 componentNameHash ) const
{
	for ( const SWayPointCookingData& wp : m_allWaypoints )
	{
		if ( wp.m_id.m_entityGuid == entityGUID && ( componentNameHash == 0 || ( wp.m_componentNameHash == componentNameHash ) ) )
		{
			return &wp;
		}
	}
	return nullptr;
}

const SWayPointCookingData*	CWayPointCookingContext::GetWaypoint( const IdTag& idTag ) const
{
	for ( const SWayPointCookingData& wp : m_allWaypoints )
	{
		if ( wp.m_idTag.IsValid() && wp.m_idTag == idTag )
		{
			return &wp;
		}
	}
	return nullptr;
}

const CWayPointCookingContext::WaypointsMappingList* CWayPointCookingContext::GetWaypointsByTag( CName tag ) const
{
	LazyInitializeQueryStructures();

	auto itFind = m_tagMapping.Find( tag );
	if ( itFind == m_tagMapping.End() )
	{
		return nullptr;
	}

	return &itFind->m_second;
}

#ifndef NO_RESOURCE_COOKING

Bool CWayPointCookingContext::CommitOutput()
{
	THandle< CWayPointsCollectionsSet > collectionsSet = CreateObject< CWayPointsCollectionsSet >();

	for ( const SCollections2Save& c : m_collections2Save )
	{
		THandle< CWayPointsCollection > collection = CreateObject< CWayPointsCollection >( collectionsSet );
		
		if ( !collection->Create( *c.m_collectionInput ) )
		{
			continue;
		}

		collectionsSet->StoreWayPointsCollection( c.m_guid, collection.Get() );
	}
	m_collections2Save.Clear();

	String fileName = m_context->GetWorld()->GetFile()->GetFileName().StringBefore( TXT(".w2w"), true );
	
	return collectionsSet->SaveAs( m_context->GetOutputDir(), fileName, false );
}

#endif
