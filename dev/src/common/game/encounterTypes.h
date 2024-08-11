#pragma once

enum ESpawnTreeCreatureRemovalReason
{
	SPAWN_TREE_REMOVAL_KILLED,
	SPAWN_TREE_REMOVAL_DESPAWN,
	SPAWN_TREE_REMOVAL_POOL
};

enum ESpawnType
{
	EST_NormalSpawn,
	EST_PoolSpawn,
	EST_GameIsRestored
};


typedef Uint16 SpawnTreeDespawnerId;
static const SpawnTreeDespawnerId SPAWN_TREE_INVALID_DESPAWNER_ID = 0xffff;

