#include "build.h"
#include "instantMountPartySpawnOrganizer.h"
#include "w3GenericVehicle.h"
#include "../../common/game/encounter.h"
#include "../../common/game/behTreeInstance.h"


#define MAX_HORSE_COUNT 20

////////////////////////////////////////////////////////////
// CInstantMountPartySpawnOrganizer
IMPLEMENT_ENGINE_CLASS( CInstantMountPartySpawnOrganizer );
void CInstantMountPartySpawnOrganizer::PostPartySpawn( const CEncounterCreaturePool::Party &party )
{
	TStaticArray< CActor *, MAX_HORSE_COUNT > horseArray;
	// [Step] gather all horses
	for ( auto it = party.Begin(), end = party.End(); it != end; ++it )
	{
		if ( it->m_memberName == CNAME( horse ) )
		{
			if ( horseArray.Size() < MAX_HORSE_COUNT )
			{
				horseArray.PushBack( it->m_actor );
			}
		}
	}

	// [Step] for each rider instant mount rider on horse
	Uint32 currentHorseIndex = 0;
	for ( auto it = party.Begin(), end = party.End(); it != end; ++it )
	{
		if ( it->m_memberName == CNAME( rider ) && currentHorseIndex < horseArray.Size() )
		{
			CActor *const horse = horseArray[ currentHorseIndex++ ];
			CActor *const rider = it->m_actor;
	
			if ( horse == nullptr || rider == nullptr )
			{
				continue;
			}
			CAIStorageRiderData *const riderData = rider->GetScriptAiStorageData<CAIStorageRiderData>( CNAME( RiderData ) );
			if ( riderData == nullptr )
			{
				continue;
			}
			W3HorseComponent* horseComponent = horse->FindComponent< W3HorseComponent>();
			if ( horseComponent == nullptr )
			{
				continue;
			}
			const Bool result = horseComponent->PairWithRider( horse, riderData->m_sharedParams.Get() );
			if ( result == false )
			{
				RED_LOG_ERROR( RED_LOG_CHANNEL( HorseRiding ), TXT("Could not pair rider with horse using Party") );
				continue;
			}
			rider->SignalGameplayEvent( CNAME( RidingManagerMountHorse ), MT_instant | MT_fromScript );
		}
	}	
}