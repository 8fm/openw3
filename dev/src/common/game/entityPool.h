/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_GAME_ENTITY_POOL_H_
#define _RED_GAME_ENTITY_POOL_H_

#include "../core/uniquePtr.h"

class CEntity;
class CEntityTemplate;
class EntitySpawnInfo;
class CDynamicLayer;
class CEntityPoolSpawnJobFactory;
class CRenderFrame;

// CEntityPool class is the same than CEntitiesPool. class was rename as the interface and code changed so drastically than P4 diff or timelapse is useless.
// Future refactor should push spawning to be more and more Fire And Forget. 
// Also, total Spawned Entity should not be manage by the Pool ...
class CEntityPool
{
	DECLARE_CLASS_MEMORY_POOL_ALIGNED( MemoryPool_Default, MC_EntityPool, __alignof( CEntityPool ) );

public:

	enum SpawnType : Uint8
	{
		SpawnType_Encounter,
		SpawnType_Community
	};

	CEntityPool();
	RED_MOCKABLE ~CEntityPool();

	void Update();

	void AddEntity( CEntity * entity );

	IJobEntitySpawn * SpawnEntity( EntitySpawnInfo && spawnInfo, SpawnType type );
	IJobEntitySpawn * SpawnEntities( TDynArray< EntitySpawnInfo > && spawnInfos, SpawnType type );
	
	CEntity * SpawnEntity_Sync( EntitySpawnInfo & spawnInfo, SpawnType type );

	void SignalEntityUnspawn( CEntity * entity );	  
	void RegisterCommunityEntity( CEntity * entity );

	void Shutdown();

	Int32 GetEntityCount() const;
	Int32 GetMaxEntityCount() const;
	Int32 GetEntitySpawnedCount() const;
	Int32 GetMaxEntitySpawnCount() const;
	Bool IsSpawnLimitExceeded() const;	

	void Flush();

	void StartFastForward();
	void StopFastForward();

	void CollectForGC( IFile & gc );
	void AddDebugScreenText( CRenderFrame * frame );

	// Do not use those functions. Mainly for UnitTest.
	void SetInternalDynamicLayer( CDynamicLayer * layer );
	void SetInternalSpawnJobFactory( Red::TUniquePtr< CEntityPoolSpawnJobFactory > factory );
	void SetInternalMaxEntityCount( Int32 value );
	void SetInternalMaxEntitySpawnCount( Int32 value );
	Uint32 GetInternalPendingEntityCount() const;
	Uint32 GetInternalBucketEntityCount( CEntityTemplate * key ) const;
	Uint32 GetInternalEntityPendingDestructionCount() const;
	void ForceAddEntity( CEntity * entity );

private:

	struct Bucket
	{
		typedef TPair< CEntity *, CName > Entry;
		typedef TDynArray< Entry > EntryContainer;

		Bucket(){}
		Bucket( Bucket && value );
		Bucket & operator=( Bucket && other );

		EntryContainer entryContainer;
		
		friend IFile& operator<<( IFile& file, Bucket & bucket ) { file << bucket.entryContainer; return file; }
	};

	struct BucketFinder;

	typedef Red::Threads::CSpinLock LockType;
	
	typedef TDynArray< THandle< CEntity >, MC_EntityPool > PendingEntityContainer;
	typedef TDynArray< THandle< CEntity >, MC_EntityPool > EntityContainer;

	typedef TPair< THandle< CEntityTemplate >, Bucket > BucketTableEntry;
	typedef TDynArray< BucketTableEntry > BucketContainer; 
	
	void SetupEntityToProcess();
	void TryAddingEntityToPool( CEntity * entity );
	void AddEntityToPool( CEntity * entity  );
	void AddEntityToBucket( CEntity * entity );
	void RemoveEntityFromPool( CEntity * entity );
	void RemoveDanglingBucket( Int32 & entityToRemove );
	
	Bucket & AcquireBucket( const THandle< CEntityTemplate > & entityTemplate );
	IJobEntitySpawn * SpawnEntityFromBucket( Bucket & bucket, EntitySpawnInfo & spawnInfo );
	CEntity * AcquireMatchingEntity( EntitySpawnInfo & spawnInfo );
	CEntity * AcquireMatchingEntityFromBucket( Bucket & bucket, EntitySpawnInfo & spawnInfo );
	CEntity * AcquireFirstEntityFromBucket( Bucket & bucket, EntitySpawnInfo & spawnInfo );

	Bool CanEntityBeAddedToPool( CEntity * entity ) const;
	Bool CanEntityBeSpawned( const EntitySpawnInfo & info ) const;
	Bool CanEntitiesBeSpawned( const TDynArray< EntitySpawnInfo > & spawnInfos ) const;
	
	void Shrink();
	void Sanitize();

	void DestroyPendingEntity();
	void DestroyAllEntity( EntityContainer & container );

	RED_MOCKABLE void UpdateEntitySpawnLimit();

	PendingEntityContainer m_pendingEntityContainer;
	PendingEntityContainer m_processEntityContainer;
	BucketContainer m_bucketContainer;
	EntityContainer m_entityContainer;
	EntityContainer m_entityPendingDestruction;

	CDynamicLayer * m_layer;

	Red::TUniquePtr< CEntityPoolSpawnJobFactory > m_factory;

	Red::Threads::CAtomic< Int32 > m_entityCount;
	Red::Threads::CAtomic< Int32 > m_entitySpawnedCount;
	Int32 m_maxEntityCount;
	Int32 m_maxEntitySpawnCount;

	Bool m_fastForwardMode;
	Bool m_entityAddedThisFrame;

	LockType m_lock;

#ifndef NO_EDITOR // HACK only for debug and editor metrics .........
	THashSet< CEntity* > m_communityEntity; 
	Red::Threads::CAtomic< Int32 > m_communityEntitySpawnedCount;
#endif
};

Red::TUniquePtr< CEntityPool > CreateEntityPool( const CWorld * world );

#endif