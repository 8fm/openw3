/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_GAME_ENTITY_POOL_SPAWN_JOB_FACTORY_H_
#define _RED_GAME_ENTITY_POOL_SPAWN_JOB_FACTORY_H_

class CEntity;
class IJobEntitySpawn;

class CEntityPoolSpawnJobFactory
{
	DECLARE_CLASS_MEMORY_POOL_ALIGNED( MemoryPool_Default, MC_EntityPool, __alignof( CEntityPoolSpawnJobFactory ) );
public:

	typedef TPair< CEntity*, EntitySpawnInfo > SpawnInfo;
	typedef TDynArray< SpawnInfo > SpawnInfoContainer;

	CEntityPoolSpawnJobFactory();
	RED_MOCKABLE ~CEntityPoolSpawnJobFactory();
	RED_MOCKABLE IJobEntitySpawn * CreateSpawnJob( SpawnInfo & spawnInfo ); 
	RED_MOCKABLE IJobEntitySpawn * CreateCompositeSpawnJob( SpawnInfoContainer & spawnInfoContainer ); // Better name?

	void SetInternalLoadingJobManager( CLoadingJobManager * manager );
	void SetInternalLayer( CDynamicLayer * layer );

private:

	CLoadingJobManager * m_loadingJobManager;
	CDynamicLayer * m_layer;
};

Red::TUniquePtr< CEntityPoolSpawnJobFactory > CreateEntityPoolSpawnJobFactory( CDynamicLayer * layer );

#endif
