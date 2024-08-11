/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "layerStorage.h"
#include "appearanceComponent.h"
#include "../core/gameSave.h"
#include "gameSaveManager.h"
#include "game.h"
#include "persistentEntity.h"
#include "layer.h"
#include "dynamicLayer.h"
#include "gameDataStorage.h"

/////////////////////////////////////////////////////////////////////////////////////////////

CLayerStorage::SpawnData::SpawnData()
	: m_position( 0,0,0,0 )
	, m_rotation( 0,0,0 )
	, m_scale( 1,1,1,1 )
{
}

void CLayerStorage::SpawnData::StoreData( IGameSaver* saver ) const
{
	//CGameSaverBlock block( saver, CNAME(s) );
	saver->WriteValue( CNAME(t), m_template );
	saver->WriteValue( CNAME(p), m_position );
	saver->WriteValue( CNAME(r), m_rotation );
	saver->WriteValue( CNAME(s), m_scale );
	saver->WriteValue( CNAME(a), m_appearance );
	saver->WriteValue( CNAME(e), m_entityFlags );
	saver->WriteValue( CNAME(ts), m_tags );
	saver->WriteValue( CNAME(n), m_name );
}

void CLayerStorage::SpawnData::RestoreData( IGameLoader* loader )
{
	if ( loader->GetSaveVersion() < SAVE_VERSION_DIRECT_STREAM_SAVES )
	{
		CGameSaverBlock block( loader, CNAME(s) );
		loader->ReadValue( CNAME(t), m_template );
		loader->ReadValue( CNAME(p), m_position );
		loader->ReadValue( CNAME(r), m_rotation );
		loader->ReadValue( CNAME(s), m_scale );
		loader->ReadValue( CNAME(a), m_appearance );
		loader->ReadValue( CNAME(e), m_entityFlags );
		loader->ReadValue( CNAME(ts), m_tags );
		loader->ReadValue( CNAME(n), m_name );
	}
	else
	{
		loader->ReadValue( CNAME(t), m_template );
		loader->ReadValue( CNAME(p), m_position );
		loader->ReadValue( CNAME(r), m_rotation );
		loader->ReadValue( CNAME(s), m_scale );
		loader->ReadValue( CNAME(a), m_appearance );
		loader->ReadValue( CNAME(e), m_entityFlags );
		loader->ReadValue( CNAME(ts), m_tags );
		loader->ReadValue( CNAME(n), m_name );
	}
}

void CLayerStorage::SpawnData::PullData( CEntity* entity )
{
	ASSERT( entity );

	// Pull the spawn data
	m_template = entity->GetEntityTemplate()->GetDepotPath();
	m_position = entity->GetPosition();
	m_rotation = entity->GetRotation();
	m_scale = entity->GetScale();
	CAppearanceComponent* appearanceComponent = CAppearanceComponent::GetAppearanceComponent( entity );
	m_appearance = appearanceComponent ? appearanceComponent->GetAppearance() : CName::NONE;
	m_entityFlags = entity->GetEntityFlags();
	m_tags = entity->GetTags();
	m_name = entity->GetName();
}

void CLayerStorage::SpawnData::PushData( EntitySpawnInfo& spawnInfo )
{
	spawnInfo.m_template = m_template.Get();
	spawnInfo.m_detachTemplate = false;
	spawnInfo.m_tags = m_tags;
	spawnInfo.m_spawnPosition = m_position;
	spawnInfo.m_spawnRotation = m_rotation;
	spawnInfo.m_spawnScale = m_scale;
	spawnInfo.m_entityFlags = m_entityFlags;
	spawnInfo.m_name = m_name;

	if ( m_appearance )
	{
		spawnInfo.m_appearances.PushBack( m_appearance );
	}
}

Uint32 CLayerStorage::SpawnData::GetDataSize() const
{
	return sizeof( CLayerStorage::SpawnData ) + 2 * (m_template.GetPath().Size() + m_name.Size() + ( m_tags.Empty() ? 0 : m_tags.ToString().Size() ) );
}

/////////////////////////////////////////////////////////////////////////////////////////////
CLayerStorage::ComponentData::ComponentData()
	: m_storedData( nullptr )
	, m_next( nullptr )
{
}

CLayerStorage::ComponentData::~ComponentData()
{
	if ( m_storedData )
	{
		delete m_storedData;
		m_storedData = nullptr;	
	}
}

Uint32 CLayerStorage::ComponentData::GetDataSize() const
{
	return sizeof( *this ) + m_storedData ? m_storedData->GetSize() : 0;
}

CLayerStorage::EntityData::EntityData()
	: m_data( NULL )
	, m_spawnData( NULL )
	, m_componentsData( nullptr )
{
}

CLayerStorage::EntityData::~EntityData()
{
	delete m_spawnData;
	if(m_data)
	{
		delete m_data;
		m_data = NULL;
	}
	ClearComponentData();
}

void CLayerStorage::EntityData::ClearComponentData()
{
	while ( m_componentsData )
	{
		ComponentData* compData = m_componentsData->m_next;
		delete m_componentsData;
		m_componentsData = compData;
	}
}

void CLayerStorage::EntityData::StoreData( IGameSaver* saver ) const
{
	CGameSaverBlock block( saver, CNAME(entityData) );

	// Save the Id
	saver->WriteValue( CNAME(idTag), m_idTag );

	// Save serialized data
	saver->AddStorageStream( m_data );

	// Save spawn data
	const Bool hasSpawnData = m_spawnData != NULL;
	saver->WriteValue( CNAME(hasSpawnData), hasSpawnData );
	if ( hasSpawnData )
	{
		// Save spawn data
		m_spawnData->StoreData( saver );
	}

	// Save component data
	Uint8 compDataSize = (Uint8) GetNumComponents();
	saver->WriteValue( CNAME(compDataSize), compDataSize );
	ComponentData* compData = m_componentsData;
	while ( compData )
	{
		CGameSaverBlock block( saver, CNAME(entityComponentData) );

		saver->WriteValue< CGUID >( CNAME(compGuid), compData->m_guid );
		saver->AddStorageStream( compData->m_storedData );

		compData = compData->m_next;
	}
}

void CLayerStorage::EntityData::RestoreData( IGameLoader* loader )
{
	CGameSaverBlock block( loader, CNAME(entityData) );

	// Load the Id
	loader->ReadValue( CNAME(idTag), m_idTag );

	if (m_data)
	{
		delete m_data;
		m_data = NULL;
	}

	// Load the entity data
	m_data = loader->ExtractDataStorage();

	// Load spawn info
	const Bool hasSpawnData = loader->ReadValue< Bool >( CNAME(hasSpawnData) );
	if ( hasSpawnData )
	{
		// Load spawn data
		m_spawnData = new SpawnData;
		m_spawnData->RestoreData( loader );
	}

	ClearComponentData();

	Uint8 compDataSize = loader->ReadValue< Uint8 >( CNAME(compDataSize) );
	for ( Uint8 i = 0; i < compDataSize; ++i )
	{
		CGameSaverBlock block( loader, CNAME(entityComponentData) );

		ComponentData* compData = new ComponentData();
		loader->ReadValue< CGUID >( CNAME(compGuid), compData->m_guid );
		compData->m_storedData = loader->ExtractDataStorage();
		compData->m_next = m_componentsData;
		m_componentsData = compData;
	}
}

void CLayerStorage::EntityData::CaptureStateFrom( CPeristentEntity* entity )
{
	ASSERT( entity );

	// Do not allow to capture state of unattached entity																																		// P.S Paxas is gay
	ASSERT( entity->IsAttached() );
	if ( !entity->IsAttached() )
	{
		WARN_ENGINE( TXT("SaveGame: Trying to pull data from non attached entity '%ls'. That is not allowed."), entity->GetFriendlyName().AsChar() );
		return;
	}

	// Create state dump of entity
	if ( !m_data )
	{
		m_data = CGameDataStorage< MC_Gameplay, MemoryPool_GameSave >::Create( 1024, 0 );
	}

	if ( m_data )
	{
		// Save state
		ISaveFile* directStream = nullptr;
		IGameSaver* saver = SGameSaveManager::GetInstance().CreateSaver( m_data, &directStream );
		if ( saver )
		{
			entity->SaveState( saver, directStream );
			delete saver;
		}

		// Pull the spawn data
		if ( m_spawnData )
		{
			m_spawnData->PullData( entity );
		}
	}
}

void CLayerStorage::EntityData::ApplyStateTo( CPeristentEntity* entity ) const
{
	ASSERT( entity );

	// Do not allow to push state to attached entity																																		// P.S Paxas is gay
	ASSERT( !entity->IsAttached() );
	if ( entity->IsAttached() )
	{
		WARN_ENGINE( TXT("SaveGame: Trying to push data to already attached entity '%ls'. That is not allowed."), entity->GetFriendlyName().AsChar() );
		return;
	}

	// Get game loader
	Uint32 version = 0;
	ISaveFile* directStream = nullptr;
	IGameLoader* loader = SGameSaveManager::GetInstance().CreateLoader( m_data, &directStream, &version );
	if ( loader )
	{
		// Set restored flag
		entity->SetDynamicFlag( EDF_RestoredFromLayerStorage, true );

		// Load state
		entity->RestoreState( loader, directStream, version );

		// Destroy loaded
		delete loader;
	}
}

CLayerStorage::ComponentData* CLayerStorage::EntityData::GetComponentData( const CGUID& guid )
{
	ComponentData* compData = m_componentsData;
	while ( compData )
	{
		if ( compData->m_guid == guid )
		{
			return compData;
		}
		compData = compData->m_next;
	}
	return nullptr;
}

IGameSaver* CLayerStorage::EntityData::GetComponentSaver( const CGUID& guid )
{
	ComponentData* compData = GetComponentData( guid );
	if ( !compData )
	{
		compData = new ComponentData();
		compData->m_guid = guid;
		compData->m_next = m_componentsData;
		m_componentsData = compData;
	}

	IGameDataStorage* storage = compData->m_storedData;
	if ( !storage )
	{
		storage = compData->m_storedData = CGameDataStorage< MC_Gameplay, MemoryPool_GameSave >::Create( 1024, 0 );
	}

	return storage ? SGameSaveManager::GetInstance().CreateSaver( storage, nullptr ) : nullptr;
}

IGameLoader* CLayerStorage::EntityData::GetComponentLoader( const CGUID& guid )
{
	ComponentData* compData = GetComponentData( guid );
	if ( !compData )
	{
		return nullptr;
	}

	if ( compData->m_storedData )
	{
		IGameLoader* loader = SGameSaveManager::GetInstance().CreateLoader( compData->m_storedData, nullptr, nullptr );
		return loader;
	}
	return nullptr;
}

Int32 CLayerStorage::EntityData::GetNumComponents() const
{
	Int32 size = 0;
	ComponentData* compData = m_componentsData;
	while ( compData )
	{
		compData = compData->m_next;
		size++;
	}
	return size;
}

Uint32 CLayerStorage::EntityData::GetDataSize() const
{
	Uint32 size = sizeof( this );

	if ( m_data )
	{
		size += m_data->GetSize();
	}

	ComponentData* compData = m_componentsData;
	while ( compData )
	{
		size += compData->GetDataSize();
		compData = compData->m_next;
	}

	if ( m_spawnData )
	{
		size += m_spawnData->GetDataSize();
	}

	return size;
}

/////////////////////////////////////////////////////////////////////////////////////////////

CLayerStorage::CLayerStorage()
	: m_closed( false )
{
}

CLayerStorage::~CLayerStorage()
{
	m_entities.ClearPtr();
}

void CLayerStorage::RestoreManagedEntities( CLayer* layer )
{
	// Restore entities
	for ( EntityData* data : m_entities )
	{
		if ( data->m_spawnData )
		{
			// Setup entity spawn data
			EntitySpawnInfo spawnInfo;
			data->m_spawnData->PushData( spawnInfo );

			// Setup IdTag
			spawnInfo.m_idTag = data->m_idTag;

			// Create entity
			layer->CreateEntitySync( spawnInfo );
		}
	}
}

void CLayerStorage::RegisterManagedEntity( CPeristentEntity* entity )
{
	ASSERT( entity );

	// Entity should be managed
	ASSERT( entity->IsManaged() );
	if ( !entity->IsManaged() )
	{
		WARN_ENGINE( TXT("SaveGame: Trying to register unmanaged entity '%ls' in the managed list. That is not allowed."), entity->GetFriendlyName().AsChar() );
		return;
	}

	// Only entities with valid IdTags can be processed
	const IdTag& spawnedTag = entity->GetIdTag();
	ASSERT( spawnedTag.IsValid() );
	if ( !spawnedTag.IsValid() )
	{
		WARN_ENGINE( TXT("SaveGame: Trying to register entity '%ls' that has NULL spawn tag. That is not allowed."), entity->GetFriendlyName().AsChar() );
		return;
	}

	// Try to connect to existing data
	if ( FindEntityData( spawnedTag ) )
	{
		// Already registered
		WARN_ENGINE( TXT("SaveGame: Trying to reregister entity '%ls' that was already registered. That is not allowed."), entity->GetFriendlyName().AsChar() );
		return;
	}

	// Create entity data
	EntityData* data = new EntityData();
	data->m_idTag = spawnedTag;
	data->m_spawnData = new SpawnData;

	// Pull spawn data from entity
	data->m_spawnData->PullData( entity );

	// Add to entity list
	m_entities.Insert( data );
}

void CLayerStorage::UnregisterManagedEntity( CPeristentEntity* entity )
{
	ASSERT( entity );

	// Entity should be managed
	ASSERT( entity->IsManaged() );
	if ( !entity->IsManaged() )
	{
		WARN_ENGINE( TXT("SaveGame: Trying to unregister unmanaged entity '%ls' from the managed list. That is not allowed."), entity->GetFriendlyName().AsChar() );
		return;
	}

	// Only entities with valid IdTags can be processed
	const IdTag& spawnedTag = entity->GetIdTag();
	ASSERT( spawnedTag.IsValid() );
	if ( !spawnedTag.IsValid() )
	{
		WARN_ENGINE( TXT("SaveGame: Trying to unregister entity '%ls' that has NULL spawn tag. That is not allowed."), entity->GetFriendlyName().AsChar() );
		return;
	}

	// Remove entity state
	RemoveEntityState( entity );
}

void CLayerStorage::RestoreEntityState( CPeristentEntity* entity, CLayer* layer )
{
	ASSERT( entity );

	// Only entities with valid IdTags can be processed

	#ifndef NO_SAVE_VERBOSITY
		static Uint32 cnt = 0;
		RED_LOG( Save, TXT("Restoring entity %ld: %s, class: %s"), cnt++, entity->GetFriendlyName().AsChar(), entity->GetClass()->GetName().AsChar() ); 
	#endif	

	const IdTag& idTag = entity->GetIdTag();
	if ( !idTag.IsValid() )
	{
		#ifndef NO_SAVE_VERBOSITY
			RED_LOG( Save, TXT("(invalid idtag)") ); 
		#endif	
		return;
	}

	// Attempt to restore from this storage

	if ( EntityData* data = FindEntityData( idTag ) )
	{
		data->ApplyStateTo( entity );
		#ifndef NO_SAVE_VERBOSITY
			RED_LOG( Save, TXT("(state applied)") ); 
		#endif	
		return;
	}

	// Attempt to restore from other worlds (and transfer to this world) - only done for dynamic layers and dynamic objects

	if ( layer->IsA<CDynamicLayer>() && !entity->IsManaged() )
	{
		if ( EntityData* data = GGame->GetUniverseStorage()->TransferEntityData( this, idTag ) )
		{
			data->ApplyStateTo( entity );
			#ifndef NO_SAVE_VERBOSITY
				RED_LOG( Save, TXT("(state transferred and applied)") ); 
			#endif	
			return;
		}
	}
}

CLayerStorage::EntityData* CLayerStorage::TransferEntityData( CLayerStorage* targetStorage, const IdTag& idTag )
{
	if ( EntityData* data = FindEntityData( idTag ) )
	{
		m_entities.Erase( idTag );
		targetStorage->m_entities.Insert( data );
		return data;
	}
	return nullptr;
}

CLayerStorage::EntityData* CLayerStorage::GetEntityData( const IdTag& idTag, Bool createIfNull )
{
	if ( !idTag.IsValid() )
	{
		return nullptr;
	}

	EntityData* entityData = FindEntityData( idTag );
	if ( !entityData && createIfNull )
	{
		entityData = new EntityData();
		entityData->m_idTag = idTag;
		m_entities.Insert( entityData );
	}

	return entityData;
}

CLayerStorage::EntityData* CLayerStorage::FindEntityData( const IdTag& idTag )
{
	auto it = m_entities.Find( idTag );
	return it != m_entities.End() ? *it : nullptr;
}

THashMap<CGUID,SLayerStorageDumpInfo> GLayerStorageDumpInfoMap;

void CLayerStorage::SaveEntityState( CPeristentEntity* entity )
{
	#ifndef NO_SAVE_VERBOSITY
		static Uint32 cnt = 0;
		RED_LOG( Save, TXT("Saving entity %ld: %s, class: %s"), cnt++, entity->GetFriendlyName().AsChar(), entity->GetClass()->GetName().AsChar() ); 
	#endif	
	
	ASSERT( entity );

	// Only entities with valid IdTags can be processed
	const IdTag& spawnedTag = entity->GetIdTag();
	if ( spawnedTag.IsValid() )
	{
		// Try to capture state of already registered entity
		if ( EntityData* data = GetEntityData( spawnedTag, true ) )
		{
			data->CaptureStateFrom( entity );
			return;
		}
	}
	else
	{
		Char buf[ RED_GUID_STRING_BUFFER_SIZE ];
		spawnedTag.GetGuid().ToString( buf, RED_GUID_STRING_BUFFER_SIZE ); 
		RED_LOG( Save, TXT("CLayerStorage::SaveEntityState(): invalid IdTag %s of entity %s"), buf, entity->GetFriendlyName().AsChar() );
	}
}

Bool CLayerStorage::RemoveEntityState( const IdTag& idTag )
{
	if ( idTag.IsValid() )
	{
		if ( EntityData* data = FindEntityData( idTag ) )
		{
			m_entities.Erase( idTag );
			delete data;
			return true;
		}
	}
	return false;
}

Bool CLayerStorage::RemoveEntityState( CPeristentEntity* entity )
{
	#ifndef NO_SAVE_VERBOSITY
		static Uint32 cnt = 0;
		RED_LOG( Save, TXT("Removing entity %ld: %s"), cnt++, entity->GetFriendlyName().AsChar() ); 
	#endif

	ASSERT( entity );
	return RemoveEntityState( entity->GetIdTag() );
}

Bool CLayerStorage::ShouldSave( CLayer* parentLayer ) const
{
	if ( m_closed )
	{
		return false;
	}

	if ( m_entities.Size() > 0 )
	{
		return true;
	}

	if ( parentLayer && parentLayer->IsAttached() )
	{
		const LayerEntitiesArray& entities = parentLayer->GetEntities();
		for ( Uint32 i=0; i<entities.Size(); ++i )
		{
			if ( CPeristentEntity* pEntity = Cast< CPeristentEntity >( entities[i] ) )
			{
				if ( pEntity->ShouldSave() )
				{
					return true;
				}
			}
		}
	}

	return false;
}

void CLayerStorage::UpdateStorage( CLayer* parentLayer )
{
	if ( m_closed )
	{
		return;
	}

	const LayerEntitiesArray& entities = parentLayer->GetEntities();
	for ( Uint32 i=0; i<entities.Size(); ++i )
	{
		CPeristentEntity* pEntity = Cast< CPeristentEntity >( entities[i] );
		if ( pEntity )
		{
			if ( pEntity->ShouldSave() )
			{
				SaveEntityState( pEntity );
			}
			else
			{
				RemoveEntityState( pEntity );
			}
		}
	}
}

void CLayerStorage::StoreData( IGameSaver* saver )
{
	if ( m_closed )
	{
		return;
	}

	CGameSaverBlock block( saver, CNAME(layerStorage) );

	// Count entities
	const Uint32 numEntities = m_entities.Size();
	saver->WriteValue( CNAME(numEntities), numEntities );

	// Save entities
	for ( const EntityData* data : m_entities )
	{
		data->StoreData( saver );
	}
}

void CLayerStorage::UpdateAndStoreData( IGameSaver* saver, CLayer* parentLayer )
{
	if ( m_closed )
	{
		return;
	}

	StoreData( saver );
}

void CLayerStorage::RestoreData( IGameLoader* loader )
{
	CGameSaverBlock block( loader, CNAME(layerStorage) );

	// Cleanup
	m_entities.ClearPtr();

	// Load entity count
	const Uint32 numEntities = loader->ReadValue< Uint32 >( CNAME(numEntities) );
	for ( Uint32 i=0; i<numEntities; ++i )
	{
		EntityData* data = new EntityData;
		data->RestoreData( loader );
		m_entities.Insert( data );
	}
}

Bool CLayerStorage::RestorePlayerEntityDataOnly( IGameLoader* loader, const IdTag& playerIdTag, const TDynArray< IdTag >& attachments )
{
	CGameSaverBlock block( loader, CNAME(layerStorage) );

	const Uint32 numEntities = loader->ReadValue< Uint32 >( CNAME(numEntities) );

	Bool success = false;
	for ( Uint32 i=0; i<numEntities; ++i )
	{
		EntityData* data = new EntityData;
		data->RestoreData( loader );
		if ( data->m_idTag == playerIdTag )
		{
			m_entities.Insert( data );
			success = true;
		}
		else if ( attachments.Exist( data->m_idTag ) )
		{
			m_entities.Insert( data );
		}
		else
		{
			delete data;
		}
	}

	return success;
}

void CLayerStorage::ResetStorage()
{
	// Remove all data entries
	m_entities.ClearPtr();
}

Uint32 CLayerStorage::GetDataSize() const
{
	Uint32 size = 0;

	// Hash map overhead
	size += m_entities.DataSize();

	// Size of all entries

	for ( const EntityData* data : m_entities )
	{
		size += data->GetDataSize();
	}

	return size;
}

Uint32 CLayerStorage::GetNumComponents() const
{
	Uint32  ret = 0;

	for ( const EntityData* data : m_entities )
	{
		ret += data->GetNumComponents();
	}

	return ret;
}

void CLayerStorage::PurgeAndCloseStorage()
{
	ResetStorage();
	m_closed = true;
}
