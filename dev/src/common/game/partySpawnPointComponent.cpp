/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "partySpawnPointComponent.h"
#include "wayPointCookingContext.h"
#include "../engine/entityHandle.h"
#include "../engine/renderFrame.h"

IMPLEMENT_ENGINE_CLASS( SPartyWaypointHandle )
IMPLEMENT_ENGINE_CLASS( CPartySpawnPointComponent )

////////////////////////////////////////////////////////////////////
// SPartyWaypointHandle
////////////////////////////////////////////////////////////////////
CWayPointComponent* SPartyWaypointHandle::Get( CComponent* owner )
{
	CWayPointComponent* wp = m_cachedWaypoint.Get();
	if ( !wp )
	{
		CEntity* entity;
		if ( m_entityHandle.Empty() )
		{
			entity = owner->GetEntity();
		}
		else
		{
			entity = m_entityHandle.Get();
		}
		if ( entity )
		{
			ComponentIterator< CWayPointComponent > it( entity );
			while ( it )
			{
				CWayPointComponent* currentWp = *it;
				if ( m_componentName.Empty() || m_componentName == currentWp->GetName() )
				{
					wp = currentWp;
					m_cachedWaypoint = currentWp;
					break;
				}
				++it;
			}
		}
	}
	return wp;
}

////////////////////////////////////////////////////////////////////
// ISpawnTreeInitializer
////////////////////////////////////////////////////////////////////
CPartySpawnPointComponent::ExcludedWaypoints::ExcludedWaypoints( CPartySpawnPointComponent* partySpawnPoint )
{
	Uint32 size = partySpawnPoint->m_partyWaypoints.Size();
	if ( size > MAX_WAYPOINTS )
	{
		RED_HALT( "TOO many party waypoints in CPartySpawnPointComponent!");
		size = MAX_WAYPOINTS;
	}
	m_excluded.Resize( size );
	Red::System::MemoryZero( m_excluded.Data(), m_excluded.DataSize() );
}

////////////////////////////////////////////////////////////////////
// CPartySpawnPointComponent
////////////////////////////////////////////////////////////////////
CWayPointComponent* CPartySpawnPointComponent::FindWaypoint( CName partyMember, ExcludedWaypoints* excludedWaypoints )
{
	Uint32 matchingWaypointsFound = 0;
	Uint32 numPoints = Min( m_partyWaypoints.Size(), MAX_WAYPOINTS );
	CWayPointComponent* selectedWP = NULL;
	Uint32 selectedWPId = 0;

	auto& randomGenerator = GEngine->GetRandomNumberGenerator();

	for ( Uint32 i = 0; i < numPoints; ++i )
	{
		SPartyWaypointHandle& def = m_partyWaypoints[ i ];
		if ( def.m_partyMemberName == partyMember &&
			(!excludedWaypoints || !excludedWaypoints->m_excluded[ i ]) )
		{
			if ( matchingWaypointsFound++ == 0 || randomGenerator.Get< Uint32 >( matchingWaypointsFound ) == 0 )
			{
				// we found way point we want to use
				CWayPointComponent* wp = def.Get( this );

				// update currently selected wp
				if ( wp )
				{
					selectedWP = wp;
					selectedWPId = i;
				}
				else
				{
					// Exclude this way point definition from future searches
					if ( excludedWaypoints )
					{
						excludedWaypoints->m_excluded[ i ] = true;
					}

					// lower count of matching waypoints so invalid waypoints won't lock invalidate search
					--matchingWaypointsFound;
				}
				
			}
		}
	}

	if ( selectedWP)
	{
		// Exclude this way point definition from future searches
		if ( excludedWaypoints )
		{
			excludedWaypoints->m_excluded[ selectedWPId ] = true;
		}
		return selectedWP;
	}
	return NULL;
}

void CPartySpawnPointComponent::WaypointGenerateEditorFragments( CRenderFrame* frame )
{
	TBaseClass::WaypointGenerateEditorFragments( frame );

	const Vector& pos = GetWorldPositionRef();
	Uint32 numPoints = Min( m_partyWaypoints.Size(), MAX_WAYPOINTS );
	for ( Uint32 i = 0; i < numPoints; ++i )
	{
		SPartyWaypointHandle& def = m_partyWaypoints[ i ];
		CWayPointComponent* wp = def.Get( this );

		if ( wp )
		{
			const Vector& dest = wp->GetWorldPositionRef();

			frame->AddDebugLineWithArrow( pos, dest, 1.f, 1.f, 1.f, Color::LIGHT_BLUE, false );
		}
	}
}

void CPartySpawnPointComponent::FillWaypointsData( TDynArray< SPartySpawnPointData >& output )
{
	const Uint32 numPoints = Min( m_partyWaypoints.Size(), MAX_WAYPOINTS );

	for ( Uint32 i = 0; i < numPoints; ++i )
	{
		const SPartyWaypointHandle& def = m_partyWaypoints[i];
		const EntityHandle& entityHandle = def.m_entityHandle;
		if ( entityHandle.Empty() )
			continue;
		
		SPartySpawnPointData spawnPointData;
		const CGUID& entityGUID = entityHandle.GetEntityHandleGUID();
		if ( !entityGUID.IsZero() )
		{
			spawnPointData.m_handleType = HANDLE_GUID;
			spawnPointData.m_guid = entityGUID;
		}
		else
		{
			const IdTag& idTag = entityHandle.GetEntityHandleIdTag();
			if ( idTag.IsValid() )
			{
				spawnPointData.m_handleType = HANDLE_IDTAG;
				spawnPointData.m_idTag = idTag;
			}
		}
		spawnPointData.m_componentNameHash = def.m_componentName.CalcHash();
		spawnPointData.m_memberName = def.m_partyMemberName;
		output.PushBack( Move( spawnPointData ) );
	}
}