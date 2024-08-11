/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeDecoratorHLCombatWildHorse.h"

#include "w3GenericVehicle.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeDecoratorHLWildHorseDangerDefinition )

CName CBehTreeNodeDecoratorHLWildHorseDangerInstance::SpecialWildHorseTag()
{
	return CNAME( WildHorse );
}

Bool CBehTreeNodeDecoratorHLWildHorseDangerInstance::ConditionCheck()
{
	CNewNPC *const npc = m_owner->GetNPC();
	if ( npc == nullptr )
	{
		return false;
	}
	if ( GetNeutralIsDanger() )
	{
		const TDynArray< NewNPCNoticedObject >& noticedObjects = npc->GetNoticedObjects();
		for ( auto it = noticedObjects.Begin(), end = noticedObjects.End(); it != end; ++it )
		{
			CActor* actor = it->m_actorHandle.Get();
			if( actor && actor->IsAlive() && npc->GetAttitude( actor ) != AIA_Friendly )
			{
				CName specialWildHorseTag = SpecialWildHorseTag();
				// custom WildHorses - filter out WildHorses and mounted WildHorses (by tag)
				{
					W3HorseComponent* horse = actor->FindComponent< W3HorseComponent >();
					if ( horse && horse->HasTag( specialWildHorseTag ) )
					{
						continue;
					}
				}

				CAIStorageRiderData* actorRiderData = CAIStorageRiderData::Get( actor );
				if ( actorRiderData && actorRiderData->m_sharedParams )
				{
					CActor* horseActor = actorRiderData->m_sharedParams->m_horse;
					if ( horseActor )
					{
						W3HorseComponent* horseComponent = horseActor->FindComponent< W3HorseComponent >();
						if ( horseComponent && horseComponent->HasTag( specialWildHorseTag ) )
						{
							continue;
						}
					}
				}

				return true;
			}
		}
	}
	else
	{
		if( npc->IsInDanger() )
		{
			return true;
		}
	}
	return false;
}