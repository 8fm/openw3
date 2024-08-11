#pragma once
#include "ridingAiStorage.h"

#include "../../common/game/spawnTreeMultiEntry.h"

class CInstantMountPartySpawnOrganizer : public CPartySpawnOrganizer
{
	DECLARE_RTTI_SIMPLE_CLASS( CInstantMountPartySpawnOrganizer );
public:
	void PostPartySpawn( const CEncounterCreaturePool::Party &party ) override;
};

BEGIN_CLASS_RTTI( CInstantMountPartySpawnOrganizer )
	PARENT_CLASS( CPartySpawnOrganizer );
END_CLASS_RTTI()