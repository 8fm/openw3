/**
 * Copyright © 2007 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../core/loadingJob.h"
#include "layer.h"

class IJobEntitySpawn : public ILoadJob
{
protected:
	THandle< CLayer >	m_layer;				//!< Layer to spawn on

	CEntity* SpawnEntity( const EntitySpawnInfo& spawnInfo );
	void LinkEntity( CEntity* entity, const EntitySpawnInfo& spawnInfo );
public:
	IJobEntitySpawn( CLayer* layer );

	virtual Bool ValidateSpawn() const = 0;
	virtual void LinkEntities() = 0;
	virtual Uint32 GetEntitiesCount() = 0;
	virtual CEntity* GetSpawnedEntity( Uint32 n ) = 0;
	virtual Bool IsSpawnedFromPool( Uint32 n ) const;
	virtual Bool AreEntitiesReady() const;
};

/// Job for spawning entity on a layer in background
class CJobSpawnEntity : public IJobEntitySpawn
{
private:
	EntitySpawnInfo		m_spawnInfo;			//!< Where to spawn
	CEntity*			m_createdEntity;		//!< Created entity
	Bool				m_wasEntityLinked;		//!< Was the entity linked to the layer ?
	CDiskFile*			m_layerFile;			//!< Layer file (debug)
	String				m_layerFilePath;		//!< Path to the layer file (debug)

public:
	CJobSpawnEntity( CLayer* layer, EntitySpawnInfo&& spawnInfo );
	~CJobSpawnEntity();

	//! Get spawned entity
	CEntity* GetSpawnedEntity( Bool link = true );

	// IJobEntitySpawn interface
	void LinkEntities() override;
	Uint32 GetEntitiesCount() override;
	CEntity* GetSpawnedEntity( Uint32 n ) override;
	Bool AreEntitiesReady() const override;

protected:
	//! Process the job, is called from job thread
	virtual EJobResult Process() override;
	virtual Bool ValidateSpawn() const override;
	const Char* GetDebugName() const override;

};

class CJobSpawnEntityList : public IJobEntitySpawn
{
protected:
	TDynArray< EntitySpawnInfo >	m_spawnInfos;			//!< Where to spawn
	TDynArray< CEntity* >			m_createdEntities;		//!< Created entity
	Bool							m_wasEntitiesLinked;	//!< Was the entity linked to the layer ?

public:
	CJobSpawnEntityList( CLayer* layer, TDynArray< EntitySpawnInfo >&& spawnInfos );
	~CJobSpawnEntityList();

	// IJobEntitySpawn interface
	void LinkEntities() override;
	Uint32 GetEntitiesCount() override;
	CEntity* GetSpawnedEntity( Uint32 n ) override;
	Bool AreEntitiesReady() const override;

protected:
	//! Process the job, is called from job thread
	virtual EJobResult Process() override;
	virtual Bool ValidateSpawn() const override;
	const Char* GetDebugName() const override;
};