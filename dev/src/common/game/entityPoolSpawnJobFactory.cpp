/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "entityPoolSpawnJobFactory.h"
#include "jobSpawnFromPool.h"
#include "../engine/dynamicLayer.h"

CEntityPoolSpawnJobFactory::CEntityPoolSpawnJobFactory()
	:	m_loadingJobManager( nullptr ),
		m_layer( nullptr )
{}

CEntityPoolSpawnJobFactory::~CEntityPoolSpawnJobFactory()
{}

IJobEntitySpawn * CEntityPoolSpawnJobFactory::CreateSpawnJob( SpawnInfo & spawnInfo )
{
	RED_FATAL_ASSERT( m_loadingJobManager, "Loading Manager is null. Use CreateEntityPoolSpawnJobFactory to create factory." );

	CEntity * entity = spawnInfo.m_first;
	EntitySpawnInfo & info = spawnInfo.m_second;

	IJobEntitySpawn * job = nullptr;
	if( entity != nullptr )
	{
		job = new CJobSpawnEntityFromPool( m_layer, entity, std::move( info ) );
	}
	else
	{
		job = new CJobSpawnEntity( m_layer, std::move( info ) );	
	}

	m_loadingJobManager->Issue( job );

	return job;
}

IJobEntitySpawn * CEntityPoolSpawnJobFactory::CreateCompositeSpawnJob( SpawnInfoContainer & spawnInfoContainer )
{
	RED_FATAL_ASSERT( m_loadingJobManager, "Loading Manager is null. Use CreateEntityPoolSpawnJobFactory to create factory." );

	IJobEntitySpawn * job = new CJobSpawnEntityListFromPool( m_layer, std::move( spawnInfoContainer ) );
	m_loadingJobManager->Issue( job );
	return job;
}

void CEntityPoolSpawnJobFactory::SetInternalLoadingJobManager( CLoadingJobManager * manager )
{
	m_loadingJobManager = manager;
}

void CEntityPoolSpawnJobFactory::SetInternalLayer( CDynamicLayer * layer )
{
	m_layer = layer;
}

Red::TUniquePtr< CEntityPoolSpawnJobFactory > CreateEntityPoolSpawnJobFactory( CDynamicLayer * layer )
{
	Red::TUniquePtr< CEntityPoolSpawnJobFactory > factory( new CEntityPoolSpawnJobFactory );
	factory->SetInternalLoadingJobManager( &SJobManager::GetInstance() );
	factory->SetInternalLayer( layer );
	return factory;
}
