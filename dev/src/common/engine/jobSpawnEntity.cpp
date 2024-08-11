/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "appearanceComponent.h"
#include "jobSpawnEntity.h"
#include "layer.h"
#include "entity.h"
#include "../core/diskFile.h"

////////////////////////////////////////////////////////////////////////
// IJobEntitySpawn
////////////////////////////////////////////////////////////////////////
IJobEntitySpawn::IJobEntitySpawn( CLayer* layer )
	: ILoadJob( JP_SpawnEntity, true )
	, m_layer( layer )
{
}

CEntity* IJobEntitySpawn::SpawnEntity( const EntitySpawnInfo& spawnInfo )
{
	// No template to spawn from
	if ( !spawnInfo.m_template )
	{
		return NULL;
	}

	// Setup instancing mode
	EntityTemplateInstancingInfo instanceInfo;
	instanceInfo.m_detachFromTemplate = spawnInfo.m_detachTemplate;
	instanceInfo.m_previewOnly = false;
	instanceInfo.m_async = true;
	instanceInfo.m_entityClass = spawnInfo.m_entityClass;

	// Use specified template to create entity	
	CEntity* createdEntity = spawnInfo.m_template->CreateInstance( m_layer.Get(), instanceInfo );
	if ( !createdEntity )
	{
		return NULL;
	}

	if( spawnInfo.m_forceNonStreamed )
	{
		// We need the entity to not be attached so it wont fire streaming events and modify its internal state
		// If this assumption changes, the "nonstreamification" will need to change to make sure the streamed components
		// are converted to non-streamed components before the entity is attached to the world
		RED_ASSERT( !createdEntity->IsAttached(), TXT("Trying to force an entity to be non-streamed while it is attached") );
		createdEntity->CreateStreamedComponents( SWN_DoNotNotifyWorld, true, nullptr, false );
		createdEntity->SetStreamed( false );
	}

#ifndef NO_EDITOR
	// Set name
	if( spawnInfo.m_name.Empty() )
	{
		CDiskFile* file = spawnInfo.m_template->GetFile();
		if( file )
		{
			CFilePath path( file->GetFileName() );
			String name;
			CLayer::GenerateUniqueEntityNameCheap( createdEntity, path.GetFileName(), name );
			createdEntity->SetName( name );
		}
	}
#endif

	createdEntity->OnCreatedAsync( spawnInfo );

	// Keep the reference to this entity, we need to do that until we add the entity to the layer
	createdEntity->AddToRootSet();

	// Job was processed
	return createdEntity;
}
void IJobEntitySpawn::LinkEntity( CEntity* createdEntity, const EntitySpawnInfo& spawnInfo )
{
	// Now we can safely remove the reference to the entity
	createdEntity->RemoveFromRootSet();

	//createdEntity->ApplyMeshComponentColoring();

	// Link entity with layer
	m_layer.Get()->LinkEntityWithLayer( createdEntity, spawnInfo );

	// Call post-attach handler
	spawnInfo.OnPostAttach( createdEntity );
}

Bool IJobEntitySpawn::IsSpawnedFromPool( Uint32 n ) const
{
	return false;
}

Bool IJobEntitySpawn::AreEntitiesReady() const
{
	return true;
}

////////////////////////////////////////////////////////////////////////
// CJobSpawnEntity
////////////////////////////////////////////////////////////////////////
CJobSpawnEntity::CJobSpawnEntity( CLayer* layer, EntitySpawnInfo&& spawnInfo )
	: IJobEntitySpawn( layer )
	, m_createdEntity( NULL )
	, m_wasEntityLinked( false )
	, m_layerFile( layer ? layer->GetFile() : nullptr )
	, m_layerFilePath( layer ? layer->GetFriendlyName() : String::EMPTY )
{
	RED_FATAL_ASSERT( layer != nullptr, "Trying to create a job to spawn an entity on a DELETED layer" );
	m_spawnInfo = Move( spawnInfo );
	// Keep the template alive
	if ( m_spawnInfo.m_template )
	{
		m_spawnInfo.m_template->AddToRootSet();
	}
}

CJobSpawnEntity::~CJobSpawnEntity()
{
	// Release the template reference
	if ( m_spawnInfo.m_template )
	{
		m_spawnInfo.m_template->RemoveFromRootSet();
	}

	// Entity was not linked
	if ( !m_wasEntityLinked && m_createdEntity )
	{
		// Only if task was not canceled 
		if ( HasEnded() && !IsCanceled() )
		{
			WARN_ENGINE( TXT("Entity spawned by async job was not picked up by main thread. FAIL!.") );
		}

		// Leave the cleanup to the garbage collector
		m_createdEntity->RemoveFromRootSet();
	}


}

void CJobSpawnEntity::LinkEntities()
{
	// Link entity to dynamic layer
	if ( !m_wasEntityLinked )
	{
		m_wasEntityLinked = true;
		if ( m_createdEntity )
		{
			PC_SCOPE( GetSpawnedEntity );

			PC_CHECKER_SCOPE( 0.005f, TXT("SPAWN"), TXT("GetSpawnedEntity '%ls'"), m_spawnInfo.m_template->GetDepotPath().AsChar() );

			LinkEntity( m_createdEntity, m_spawnInfo );
		}
		else
		{
			RED_HALT("Wait for the job to FINISH!!!");
		}
	}
	
}

CEntity* CJobSpawnEntity::GetSpawnedEntity( Bool link )
{
	if( link )
	{
		LinkEntities();

		if ( m_createdEntity->IsInRootSet() )
		{
			RED_HALT("Wait for the job to FINISH!!!");
		}
	}

	// Return entity
	return m_createdEntity;
}

Uint32 CJobSpawnEntity::GetEntitiesCount()
{
	return m_createdEntity ? 1 : 0;
}
CEntity* CJobSpawnEntity::GetSpawnedEntity( Uint32 n )
{
	ASSERT( n == 0 );
	return m_createdEntity;
}

Bool CJobSpawnEntity::AreEntitiesReady() const
{
	if ( m_createdEntity )
	{
		if ( !m_createdEntity->IsRenderingReady() )
		{
			return false;
		}
	}
	return true;
}

EJobResult CJobSpawnEntity::Process()
{
	m_createdEntity = SpawnEntity( m_spawnInfo );
	if ( !m_createdEntity )
	{
		return JR_Failed;
	}

	// Apply initial appearance
	CAppearanceComponent* appearanceComponent = CAppearanceComponent::GetAppearanceComponent( m_createdEntity );
	if ( appearanceComponent )
	{
		appearanceComponent->ApplyInitialAppearance( m_spawnInfo );
	}

	m_createdEntity->PostComponentsInitializedAsync();

	// Job was processed
	return JR_Finished;
}

Bool CJobSpawnEntity::ValidateSpawn() const
{
	if ( !m_layer )
	{
		ERR_ENGINE( TXT("Layer was deleted BEFORE spawn job has finished. WTF ?. Layer path: '%ls'"), m_layerFilePath.AsChar() );
		return false;
	}

	if ( !m_createdEntity )
	{
		return false;
	}

	return true;
}

const Char* CJobSpawnEntity::GetDebugName() const
{
	return TXT("SpawnEntity");
}

////////////////////////////////////////////////////////////////////////
// CJobSpawnEntityList
////////////////////////////////////////////////////////////////////////

CJobSpawnEntityList::CJobSpawnEntityList( CLayer* layer, TDynArray< EntitySpawnInfo >&& spawnInfos )
	: IJobEntitySpawn( layer )
	, m_spawnInfos( Move( spawnInfos ) )
	, m_wasEntitiesLinked( false )
{}
CJobSpawnEntityList::~CJobSpawnEntityList()
{
	if ( !m_wasEntitiesLinked )
	{
		for ( Uint32 i = 0, n = m_createdEntities.Size(); i != n; ++i )
		{
			if ( m_createdEntities[ i ] )
			{
				m_createdEntities[ i ]->RemoveFromRootSet();
			}
		}
	}
}

void CJobSpawnEntityList::LinkEntities()
{
	if ( !m_wasEntitiesLinked )
	{
		m_wasEntitiesLinked = true;
		for( Uint32 i = 0, n = m_createdEntities.Size(); i < n; ++i )
		{
			if ( m_createdEntities[ i ] )
			{
				LinkEntity( m_createdEntities[ i ], m_spawnInfos[ i ] );
			}
		}
	}
}
Uint32 CJobSpawnEntityList::GetEntitiesCount()
{
	return m_createdEntities.Size();
}
CEntity* CJobSpawnEntityList::GetSpawnedEntity( Uint32 n )
{
	return m_createdEntities[ n ];
}

Bool CJobSpawnEntityList::AreEntitiesReady() const
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

EJobResult CJobSpawnEntityList::Process()
{
	Bool createdSomething = false;
	m_createdEntities.Resize( m_spawnInfos.Size() );
	for ( Uint32 i = 0, n = m_spawnInfos.Size(); i != n; ++i )
	{
		const EntitySpawnInfo& spawnInfo = m_spawnInfos[ i ];

		CEntity* entity = SpawnEntity( spawnInfo );
		m_createdEntities[ i ] = entity;
		if ( entity )
		{
			// Apply initial appearance
			CAppearanceComponent* appearanceComponent = CAppearanceComponent::GetAppearanceComponent( entity );
			if ( appearanceComponent )
			{
				appearanceComponent->ApplyInitialAppearance( spawnInfo );
			}

			entity->PostComponentsInitializedAsync();

			createdSomething = true;
		}
	}
	return createdSomething ? JR_Finished : JR_Failed;
}

const Char* CJobSpawnEntityList::GetDebugName() const
{
	return TXT("SpawnEntityList");
}

Bool CJobSpawnEntityList::ValidateSpawn() const
{
	return m_layer != nullptr;
}
