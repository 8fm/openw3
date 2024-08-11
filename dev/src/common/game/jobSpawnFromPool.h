#pragma once

#include "../engine/dynamicLayer.h"
#include "../engine/jobSpawnEntity.h"

class IJobEntityPoolSpawn : public IJobEntitySpawn
{
protected:

	CDynamicLayer* GetDynamicLayer() const													{ return static_cast< CDynamicLayer* >( m_layer.Get() ); }

	void RestoreEntityAsync( CEntity* entity, const EntitySpawnInfo& spawnInfo );
	void RestoreEntitySync( CEntity* entity, const EntitySpawnInfo& spawnInfo );

	
public:
	IJobEntityPoolSpawn( CDynamicLayer* layer )
		: IJobEntitySpawn( layer )
	{}
};

class CJobSpawnEntityFromPool : public IJobEntityPoolSpawn
{
private:
	EntitySpawnInfo	m_spawnInfo;
	
	CEntity*		m_restoredEntity;

public:
	CJobSpawnEntityFromPool( CDynamicLayer * layer, CEntity * entity, EntitySpawnInfo&& spawnInfo );
	virtual ~CJobSpawnEntityFromPool();

	// IJobEntitySpawn interface
	void LinkEntities() override;
	Uint32 GetEntitiesCount() override;
	CEntity* GetSpawnedEntity( Uint32 n ) override;
	Bool IsSpawnedFromPool( Uint32 n ) const override;
	Bool AreEntitiesReady() const override;

protected:
	//! Process the job, is called from job thread
	virtual EJobResult Process() override;
	virtual Bool ValidateSpawn() const override;
	const Char* GetDebugName() const override;
};

class CJobSpawnEntityListFromPool : public IJobEntityPoolSpawn
{
public:
	typedef TPair< CEntity*, EntitySpawnInfo > SpawnInfo;
	typedef TDynArray< SpawnInfo > SpawnInfoContainer;

private:
	SpawnInfoContainer		m_spawnInfos;			//!< Where to spawn
	TDynArray< CEntity* >	m_createdEntities;		//!< Created entity
	Bool					m_wasEntitiesLinked;	//!< Was the entity linked to the layer ?

public:
	
	CJobSpawnEntityListFromPool( CDynamicLayer* layer, SpawnInfoContainer&& spawnInfos );
	~CJobSpawnEntityListFromPool();

	// IJobEntitySpawn interface
	void LinkEntities() override;
	Uint32 GetEntitiesCount() override;
	CEntity* GetSpawnedEntity( Uint32 n ) override;
	Bool IsSpawnedFromPool( Uint32 n ) const override;
	Bool AreEntitiesReady() const override;

protected:
	//! Process the job, is called from job thread
	virtual EJobResult Process() override;
	virtual Bool ValidateSpawn() const override;

	const Char* GetDebugName() const override;
};