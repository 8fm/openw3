/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "..\engine\appearanceComponent.h"
#include "jobSpawnFromPool.h"


////////////////////////////////////////////////////////////////////////
// IJobEntityPoolSpawn
////////////////////////////////////////////////////////////////////////
void IJobEntityPoolSpawn::RestoreEntityAsync( CEntity* entity, const EntitySpawnInfo& spawnInfo )
{
	CAppearanceComponent* appearanceComponent = CAppearanceComponent::GetAppearanceComponent( entity );
	if( appearanceComponent )
	{
		appearanceComponent->ApplyAppearance( spawnInfo );
	}

	entity->OnRestoredFromPoolAsync( spawnInfo );
}

void IJobEntityPoolSpawn::RestoreEntitySync( CEntity* entity, const EntitySpawnInfo& spawnInfo )
{
	CDynamicLayer* layer = GetDynamicLayer();

	entity->OnRestoredFromPool( layer, spawnInfo );

	layer->AttachEntity( entity );

	spawnInfo.OnPostAttach( entity );
}

////////////////////////////////////////////////////////////////////////
// CJobSpawnEntityFromPool
////////////////////////////////////////////////////////////////////////

CJobSpawnEntityFromPool::CJobSpawnEntityFromPool( CDynamicLayer* layer, CEntity * entity, EntitySpawnInfo&& spawnInfo )
	: IJobEntityPoolSpawn( layer )
	, m_restoredEntity( entity )
	, m_spawnInfo( std::move( spawnInfo ) )
{
	m_restoredEntity->AddToRootSet(); // At this point, Entity do not have any owner. But it can't be gc either!
}

CJobSpawnEntityFromPool::~CJobSpawnEntityFromPool()
{
	m_restoredEntity->RemoveFromRootSet();
}

Uint32 CJobSpawnEntityFromPool::GetEntitiesCount()
{
	return m_restoredEntity ? 1 : 0;
}

EJobResult CJobSpawnEntityFromPool::Process()
{
	if ( m_restoredEntity )
	{
		RestoreEntityAsync( m_restoredEntity, m_spawnInfo );
	}

	return JR_Finished;
}

Bool CJobSpawnEntityFromPool::ValidateSpawn() const
{
	return true;
}

void CJobSpawnEntityFromPool::LinkEntities()
{
	if ( m_restoredEntity )
	{
		RestoreEntitySync( m_restoredEntity, m_spawnInfo );
	}
	
}

CEntity* CJobSpawnEntityFromPool::GetSpawnedEntity( Uint32 n )
{
	ASSERT( n == 0 );
	return m_restoredEntity;
}

Bool CJobSpawnEntityFromPool::IsSpawnedFromPool( Uint32 n ) const
{
	ASSERT( n == 0 );
	return true;
}
Bool CJobSpawnEntityFromPool::AreEntitiesReady() const
{
	if ( m_restoredEntity )
	{
		if ( !m_restoredEntity->IsRenderingReady() )
		{
			return false;
		}
	}
	return true;
}

const Char* CJobSpawnEntityFromPool::GetDebugName() const
{
	return TXT("SpawnEntityFromPool");
}

////////////////////////////////////////////////////////////////////////
// CJobSpawnEntityListFromPool
////////////////////////////////////////////////////////////////////////

CJobSpawnEntityListFromPool::CJobSpawnEntityListFromPool( CDynamicLayer* layer, SpawnInfoContainer&& spawnInfos )
	: IJobEntityPoolSpawn( layer )
	, m_spawnInfos( std::move( spawnInfos ) )
	, m_wasEntitiesLinked( false )
{
	m_createdEntities.Resize( m_spawnInfos.Size() );

	for( Uint32 i = 0, n = m_spawnInfos.Size(); i < n; ++i )
	{
		CEntity * entity = m_spawnInfos[ i ].m_first;
		const EntitySpawnInfo & info = m_spawnInfos[ i ].m_second;

		if ( entity )
		{
			entity->AddToRootSet(); // At this point, Entity do not have any owner. But it can't be gc either!
		}

		m_createdEntities[ i ] = entity; 
	}
}

CJobSpawnEntityListFromPool::~CJobSpawnEntityListFromPool()
{
	for ( Uint32 i = 0, n = m_createdEntities.Size(); i != n; ++i )
	{
		CEntity * entity = m_createdEntities[ i ];

		if ( entity && entity->IsInRootSet() )
		{
			entity->RemoveFromRootSet();
		}
	}
}

Uint32 CJobSpawnEntityListFromPool::GetEntitiesCount()
{
	return m_createdEntities.Size();
}

CEntity* CJobSpawnEntityListFromPool::GetSpawnedEntity( Uint32 n )
{
	return m_createdEntities[ n ];
}

Bool CJobSpawnEntityListFromPool::IsSpawnedFromPool( Uint32 n ) const
{
	return m_spawnInfos[ n ].m_first != nullptr;
}

Bool CJobSpawnEntityListFromPool::AreEntitiesReady() const
{
	for ( CEntity* entity : m_createdEntities )
	{
		if ( entity && !entity->IsRenderingReady() )
		{
			return false;
		}
	}
	return true;
}

EJobResult CJobSpawnEntityListFromPool::Process()
{
	Bool createdSomething = false;
	m_createdEntities.Resize( m_spawnInfos.Size() );
	for ( Uint32 i = 0, n = m_spawnInfos.Size(); i != n; ++i )
	{
		const SpawnInfo& spawnInfo = m_spawnInfos[ i ];
		const EntitySpawnInfo & info = spawnInfo.m_second;
		if( !spawnInfo.m_first )
		{
			
			CEntity* entity = SpawnEntity( info);
			m_createdEntities[ i ] = entity;
			if ( entity )
			{
				// Apply initial appearance
				CAppearanceComponent* appearanceComponent = CAppearanceComponent::GetAppearanceComponent( entity );
				if ( appearanceComponent )
				{
					appearanceComponent->ApplyInitialAppearance( info );
				}

				entity->PostComponentsInitializedAsync();

				createdSomething = true;
			}
		}
		else if ( m_createdEntities[ i ] )
		{
			RestoreEntityAsync( m_createdEntities[ i ], info );

			createdSomething = true;
		}
	}
	return createdSomething ? JR_Finished : JR_Failed;
}

void CJobSpawnEntityListFromPool::LinkEntities()
{
	if ( !m_wasEntitiesLinked )
	{
		ASSERT( m_createdEntities.Size() == m_spawnInfos.Size() );

		m_wasEntitiesLinked = true;
		for( Uint32 i = 0, n = m_createdEntities.Size(); i < n; ++i )
		{
			if ( m_createdEntities[ i ] )
			{
				const SpawnInfo& spawnInfo = m_spawnInfos[ i ];
				const EntitySpawnInfo & info = spawnInfo.m_second;
				if ( !spawnInfo.m_first )
				{
					LinkEntity( m_createdEntities[ i ], info );
				}
				else
				{
					RestoreEntitySync( m_createdEntities[ i ], info );
				}
			}
		}
	}
}

Bool CJobSpawnEntityListFromPool::ValidateSpawn() const
{
	return true;
}

const Char* CJobSpawnEntityListFromPool::GetDebugName() const
{
	return TXT("SpawnEntityListFromPool");
}
