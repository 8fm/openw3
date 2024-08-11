/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "partySpawnPointOrganizer.h"

#include "../engine/pathlibWorld.h"

#include "partySpawnPointComponent.h"

IMPLEMENT_ENGINE_CLASS( CPartySpawnpointOrganizer )
IMPLEMENT_ENGINE_CLASS( CPartySpreadOrganizer )


void CPartySpawnpointOrganizer::PrePartySpawn( CCreaturePartyEntry* entry, TDynArray< EntitySpawnInfo >& spawnInfo, SEncounterSpawnPoint* spDef )
{
	if ( !spDef )
	{
		return;
	}
	
	const CWayPointsCollection& collection = *spDef->m_collection;
	const SPartySpawner* const partySpawner = collection.GetPartySpawner( spDef->m_baseData->m_ownerMapping );
	if ( !partySpawner || partySpawner->m_waypointsCount == 0 )
	{
		return;
	}

	TDynArray< Bool > excludedWps( partySpawner->m_waypointsCount );
	CCreaturePartyEntry::SpawnInfoIterator subDefIterator( entry );

	for ( auto it = spawnInfo.Begin(), end = spawnInfo.End(); it != end; ++it )
	{
		subDefIterator.FetchNext();

		EntitySpawnInfo& info = *it;

		CSpawnTreeEntrySubDefinition* subDef = subDefIterator.GetSubdefinition();
		const SPartyWaypointData* partyWps = partySpawner->FindWaypoint( collection, subDef->GetPartyMemberName(), excludedWps );

		if ( partyWps )
		{
			info.m_spawnPosition = partyWps->m_position;
			info.m_spawnRotation = EulerAngles( 0.f, 0.f, partyWps->m_rotation );
		}
	}
}

void CPartySpreadOrganizer::PrePartySpawn( CCreaturePartyEntry* entry, TDynArray< EntitySpawnInfo >& spawnInfos, SEncounterSpawnPoint* sp )
{
	if ( spawnInfos.Size() <= 1 )
	{
		return;
	}

	const Float DEFAULT_PS = 0.4f;

	Vector3 centralPos = sp->m_baseData->m_position;
	CPathLibWorld* pathlib = GGame->GetActiveWorld()->GetPathLibWorld();
	PathLib::AreaId areaId = PathLib::INVALID_AREA_ID;
	auto& generator = GEngine->GetRandomNumberGenerator();
	Float angle = generator.Get< Float >( 0.f, 360.f );
	Float angleDiff = 360.f / Float( spawnInfos.Size()-1 ); 

	if ( !pathlib->TestLocation( areaId, centralPos, DEFAULT_PS, PathLib::CT_NO_ENDPOINT_TEST ) )
	{
		return;
	}

	for ( Uint32 i = 1, creatureCount = spawnInfos.Size(); i < creatureCount; ++i )
	{
		auto& info = spawnInfos[ i ];

		Float rnd = generator.Get< Float >( -1.f, 1.f );
		Bool neg = rnd < 0.f;
		Float distRatio =
			0.5f + 0.5f * (neg ? -rnd*rnd : rnd*rnd );

		Float desiredDist = m_spreadRadiusMin + ( m_spreadRadiusMax - m_spreadRadiusMin ) * distRatio;

		Vector3 desiredSpot = centralPos;
		desiredSpot.AsVector2() += EulerAngles::YawToVector2( angle ) * desiredDist;

		// increase spawn angle
		angle += angleDiff;

		Vector3 outSpot;

		if ( pathlib->GetClearLineInDirection( areaId, centralPos, desiredSpot, DEFAULT_PS, outSpot, PathLib::CT_NO_ENDPOINT_TEST ) == PathLib::CLEARLINE_INVALID_START_POINT )
		{
			continue;
		}

		info.m_spawnPosition.AsVector3() = outSpot;
		info.m_spawnRotation.Yaw = angle + generator.Get< Float >( -60.f, 60.f );
	}

}