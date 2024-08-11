/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "wayPointsCollection.h"
#include "partySpawnPointComponent.h"

#include "wayPointCookingContext.h"

IMPLEMENT_ENGINE_CLASS( SPartyWaypointData );
IMPLEMENT_ENGINE_CLASS( SPartySpawner );
IMPLEMENT_ENGINE_CLASS( CWayPointsCollection );

CWayPointsCollection::CWayPointsCollection()
	: m_waypointsCount( 0 )
	, m_componentsMappingsCount( 0 )
	, m_waypointsGroupsCount( 0 )
	, m_indexesCount( 0 )
{
}

CWayPointsCollection::~CWayPointsCollection()
{
}

void CWayPointsCollection::OnSerialize( IFile& file )
{
	if ( file.IsWriter() )
	{
		m_version = CURRENT_BINARIES_VERSION;
	}

	TBaseClass::OnSerialize( file );

	m_dataBuffer.Serialize( file );

	if ( !file.IsGarbageCollector() && file.IsReader() )
	{
		if ( !RestoreDataPostSerialization() )
		{
			m_waypointsCount = 0;
			m_componentsMappingsCount = 0;
			m_waypointsGroupsCount = 0;
			RED_HALT( "Problem while reading CWayPointsCollection %s", GetFriendlyName().AsChar() );
		}
	}
}

void CWayPointsCollection::CreateWaypoint( const SWayPointCookingData& wpInput, SWayPointHandle& outWp )
{
	outWp.m_position = wpInput.m_position;
	outWp.m_yaw = wpInput.m_orientation.Yaw;
	outWp.m_ownerMapping = -1;
}

void CWayPointsCollection::CreateParty( const SComponentMapping& wpMapping, const CWayPointCookingContext& context, GroupId mappingIndex )
{
	if ( const SPartyCookingData* partyData = context.GetParty( wpMapping ) )
	{
		SPartySpawner party;
		party.m_mappingIndex = mappingIndex;
		party.m_waypointsCount = 0;
		party.m_firstIndex = m_partyWaypoints.Size();
		
		for ( Uint32 i = 0, n = partyData->m_partySpawnPoints.Size(); i < n; ++i )
		{
			const SPartySpawnPointData& spawnPointData = partyData->m_partySpawnPoints[i];
			if ( spawnPointData.m_handleType != HANDLE_NONE )
			{
				const SWayPointCookingData* wpData = spawnPointData.m_handleType == HANDLE_IDTAG ? context.GetWaypoint( spawnPointData.m_idTag ) : context.GetWaypoint( spawnPointData.m_guid, spawnPointData.m_componentNameHash );
				if ( wpData )
				{
					party.m_waypointsCount += 1;
					SPartyWaypointData wpHandle;
					wpHandle.m_position = wpData->m_position;
					wpHandle.m_rotation = wpData->m_orientation.Yaw;
					wpHandle.m_memberName = spawnPointData.m_memberName;
					m_partyWaypoints.PushBack( Move( wpHandle ) );
				}
			}
		}
		m_parties.PushBack( Move( party ) );
	}
}

Bool CWayPointsCollection::Create( const Input& input )
{
	const CWayPointCookingContext& context = input.m_context;

	// precompute input
	// setup algorithm data
	TDynArray< SWayPointHandle > waypoints;
	TDynArray< SComponentMapping > mappings;
	THashMap< SComponentMapping, Uint32 > indices;

	m_parties.ClearFast();
	m_partyWaypoints.ClearFast();
	
	struct GroupDef
	{
		Uint32 m_groupId;
		TDynArray< Uint16 > m_aps;

		Bool operator<( const GroupDef& g ) const { return m_groupId < g.m_groupId; }
	};

	// iterate through input groups
	TDynArray< GroupDef > groups;
	Bool b = false;
	for ( const Input::Group& g : input.m_groups )
	{
		if ( g.m_list.Empty() )
		{
			continue;
		}
		b = true;
		GroupDef def;
		def.m_groupId = g.m_groupId;
		def.m_aps.Resize( g.m_list.Size() );
		// iterate through group waypoints
		for ( Uint32 i = 0, n = g.m_list.Size(); i != n; ++i )
		{
			const SComponentMapping& mapping = g.m_list[ i ];

			Uint16 apIndex;

			auto itFind = indices.Find( mapping );
			if ( itFind == indices.End() )
			{
				// lazy create waypoint
				const SWayPointCookingData* wpInput = context.GetWaypoint( mapping );
				SWayPointHandle wp;
				CreateWaypoint( *wpInput, wp );
				if ( wpInput->m_waypointClass->IsA< CActionPointComponent >() )
				{
					wp.m_ownerMapping = mappings.Size();
					mappings.PushBack( wpInput->m_id );
				}
				else if ( wpInput->m_waypointClass->IsA< CPartySpawnPointComponent >() )
				{
					wp.m_ownerMapping = mappings.Size();
					mappings.PushBack( wpInput->m_id );
					CreateParty( mapping, context, wp.m_ownerMapping );
				}
				apIndex = waypoints.Size();
				waypoints.PushBack( wp );
			}
			else
			{
				apIndex = itFind->m_second;
			}
			def.m_aps[ i ] = apIndex;
		}

		groups.PushBack( def );
	}

	// shouldn't be necessary, but lets just make sure
	::Sort( groups.Begin(), groups.End() );

	// compute data size
	m_waypointsCount = waypoints.Size();
	m_componentsMappingsCount = mappings.Size();
	m_waypointsGroupsCount = groups.Size();
	m_indexesCount = 0;

	for ( const GroupDef& g : groups )
	{
		m_indexesCount += g.m_aps.Size();
	}

	// allocate & fill up buffer data
	const Uint32 waypointBufferSize = m_waypointsCount * sizeof( SWayPointHandle );
	const Uint32 mappingsBufferSize = m_componentsMappingsCount * sizeof( SComponentMapping );
	const Uint32 groupsBufferSize = m_waypointsGroupsCount * sizeof( SWayPointsGroup );
	const Uint32 indexesBufferSize = m_indexesCount * sizeof( Uint16 );

	const Uint32 bufferSize = 
		waypointBufferSize
		+ mappingsBufferSize
		+ groupsBufferSize
		+ indexesBufferSize;

	m_dataBuffer.Allocate( bufferSize );

	Uint8* data = static_cast< Uint8* >( m_dataBuffer.GetData() );

	m_waypoints = reinterpret_cast< SWayPointHandle* >( data );
	Red::System::MemoryCopy( m_waypoints, waypoints.TypedData(), waypointBufferSize );
	data += waypointBufferSize;

	m_componentMappings = reinterpret_cast< SComponentMapping* >( data );
	Red::System::MemoryCopy( m_componentMappings, mappings.TypedData(), mappingsBufferSize );
	data += mappingsBufferSize;

	m_waypointsGroups = reinterpret_cast< SWayPointsGroup* >( data );
	data += groupsBufferSize;

	m_indexes = reinterpret_cast< Uint16* >( data );

	Uint32 assignedIndexes = 0;
	for ( Uint32 i = 0, n = groups.Size(); i != n; ++i )
	{
		const GroupDef& def = groups[ i ];
		SWayPointsGroup& g = m_waypointsGroups[ i ];

		// copy indexes
		Red::System::MemoryCopy( &m_indexes[ assignedIndexes ], def.m_aps.TypedData(), def.m_aps.DataSize() );

		// fill up group structure
		g.m_firstIndex = assignedIndexes;
		g.m_groupId = def.m_groupId;
		g.m_waypointsCount = def.m_aps.Size();
		assignedIndexes += g.m_waypointsCount;
	}

	return b;
}

Bool CWayPointsCollection::RestoreDataPostSerialization()
{
	const Uint32 baseBufferSize = 
		m_waypointsCount * sizeof( SWayPointHandle )
		+ m_componentsMappingsCount * sizeof( SComponentMapping )
		+ m_waypointsGroupsCount * sizeof( SWayPointsGroup )
		+ m_indexesCount * sizeof( Uint16 );

	const Uint32 dataSize = static_cast< Uint32 >( m_dataBuffer.GetSize() );
	if ( dataSize != baseBufferSize )
	{
		return false;
	}
	
	Uint8* data = static_cast< Uint8* >( m_dataBuffer.GetData() );

	m_waypoints = reinterpret_cast< SWayPointHandle* >( data );
	data += m_waypointsCount * sizeof( SWayPointHandle );
	m_componentMappings = reinterpret_cast< SComponentMapping* >( data );
	data += m_componentsMappingsCount * sizeof( SComponentMapping );
	m_waypointsGroups = reinterpret_cast< SWayPointsGroup* >( data );
	data += m_waypointsGroupsCount * sizeof( SWayPointsGroup );
	m_indexes = reinterpret_cast< Uint16* >( data );

	return true;
}

CWayPointsCollection::GroupIndex CWayPointsCollection::GetWPGroupIndexById( GroupId groupId ) const
{
	for ( Uint16 i = 0; i < m_waypointsGroupsCount; ++i )
	{
		if ( m_waypointsGroups[ i ].m_groupId == groupId )
		{
			return i;
		}
	}
	return -1;
}

const SPartySpawner* CWayPointsCollection::GetPartySpawner( GroupId mappingIndex ) const
{
	for ( Uint16 i = 0, n = m_parties.Size(); i < n; ++i )
	{
		if ( m_parties[i].m_mappingIndex == mappingIndex )
		{
			return &m_parties[i];
		}
	}
	return nullptr;
}

const SPartyWaypointData* SPartySpawner::FindWaypoint( const CWayPointsCollection& collection, const CName& partyMemberName, TDynArray< Bool >& excludedWaypoints ) const
{
	Uint32 matchingWaypointsFound = 0;
	const SPartyWaypointData* selectedWP = nullptr;
	Uint32 selectedWPId = 0;

	auto& randomGenerator = GEngine->GetRandomNumberGenerator();

	for ( Uint32 i = 0; i < m_waypointsCount; ++i )
	{
		const SPartyWaypointData* wp = collection.GetPartyWaypoints( m_firstIndex + i );
		if ( wp && wp->m_memberName == partyMemberName && ( excludedWaypoints.Empty() || !excludedWaypoints[i] ) )
		{
			if ( matchingWaypointsFound++ == 0 || randomGenerator.Get< Uint32 >( matchingWaypointsFound ) == 0 )
			{
				const SPartyWaypointData* currentWp = wp;

				// update currently selected wp
				if ( wp )
				{
					selectedWP = currentWp;
					selectedWPId = i;
				}
				else
				{
					// Exclude this waypoint definition from future searches
					excludedWaypoints[i] = true;

					// lower count of matching waypoints so invalid waypoints won't lock invalidate search
					--matchingWaypointsFound;
				}
			}
		}
	}

	if ( selectedWP )
	{
		// Exclude this way point definition from future searches
		excludedWaypoints[selectedWPId] = true;
		return selectedWP;
	}

	return nullptr;
}