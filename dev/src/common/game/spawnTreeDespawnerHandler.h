#pragma once
#include "spawnTreeDespawnConfiguration.h"

class CSpawnTreeDespawnerHandler
{
protected:
	TDynArray< CSpawnTreeDespawnAction >	m_despawners;
	SpawnTreeDespawnerId					m_nextDespawnId;

public:
	CSpawnTreeDespawnerHandler() {}
	void Update();
	void RegisterDespawner( CSpawnTreeDespawnAction&& despawner );
	Bool RemoveDespawner( SpawnTreeDespawnerId id );
	SpawnTreeDespawnerId GetNextDespawnId();
};
