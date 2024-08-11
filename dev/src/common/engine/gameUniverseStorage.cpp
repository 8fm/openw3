#include "build.h"
#include "gameUniverseStorage.h"
#include "layerGroup.h"
#include "game.h"
#include "../core/gameSave.h"
#include "gameSaveManager.h"
#include "idTagManager.h"
#include "persistentEntity.h"
#include "world.h"
#include "layerStorage.h"
#include "dynamicLayer.h"
#include "layerInfo.h"
#include "../core/diskFile.h"
#include "gameDataStorage.h"

CLayerVisibilityStorage::EStatus CLayerVisibilityStorage::GetStatus( const String& layer ) const
{
	Hash hash( layer );

	if ( m_hidden.Exist( hash ) )
	{
		return LAYER_Hidden;
	}

	if ( m_shown.Exist( hash ) )
	{
		return LAYER_Shown;
	}

	return LAYER_NotFound;
}

void CLayerVisibilityStorage::UpdateStatus( const String& layer, Bool flag )
{
	Hash hash( layer );

	ptrdiff_t hidden = m_hidden.GetIndex( hash );
	ptrdiff_t shown = m_shown.GetIndex( hash );

	if ( flag )	// we want to show it...
	{
		if ( hidden >= 0 ) // ...and it was hidden => remove from hidden list
		{
			m_hidden.RemoveAtFast( size_t( hidden ) );	
		}

		if ( shown < 0 ) // ...and it wasn't show => add to shown list
		{
			m_shown.PushBack( hash );
		}
	}
	else  // we want to hide it...
	{
		if ( shown >= 0 ) // ...and it was shown => remove from shown list
		{
			m_shown.RemoveAtFast( size_t( shown ) );	
		}

		if ( hidden < 0 ) // ...and it wasn't hidden => add to hidden list
		{
			m_hidden.PushBack( hash );
		}
	}


}

void CLayerVisibilityStorage::Load( IGameLoader* loader )
{
	Clear();

	CGameSaverBlock block( loader, CNAME( visibility ) );

	Uint64 readValue = 0;
	Uint32 num = loader->ReadValue< Uint32 >( CNAME( num ), 0 );
	for ( Uint32 i = 0; i < num; ++i )
	{
		readValue = loader->ReadValue< Uint64 >( CNAME( hash ), (Uint64)0 );
		if ( readValue )
		{
			m_hidden.PushBack( Hash( readValue ) );
		}
	}

	num = loader->ReadValue< Uint32 >( CNAME( num ), 0 );
	for ( Uint32 i = 0; i < num; ++i )
	{
		readValue = loader->ReadValue< Uint64 >( CNAME( hash ), (Uint64)0 );
		if ( readValue )
		{
			m_shown.PushBack( Hash( readValue ) );
		}
	}
}

void CLayerVisibilityStorage::Save( IGameSaver* saver ) const
{
	CGameSaverBlock block( saver, CNAME( visibility ) );

	Uint64 writeValue;
	Uint32 num = m_hidden.Size();
	saver->WriteValue( CNAME( num ), num );

	for ( Uint32 i = 0; i < m_hidden.Size(); ++i )
	{
		writeValue = m_hidden[ i ].m_hash;
		saver->WriteValue( CNAME( hash ), writeValue );
	}

	num = m_shown.Size();
	saver->WriteValue( CNAME( num ), num );

	for ( Uint32 i = 0; i < m_shown.Size(); ++i )
	{
		writeValue = m_shown[ i ].m_hash;
		saver->WriteValue( CNAME( hash ), writeValue );
	}
}

void CLayerVisibilityStorage::Clear()
{
	m_hidden.Clear();
	m_shown.Clear();
}

// ------------------------------------------------------------


CExtractLayersTask::CExtractLayersTask( CWorld* world )
	: CTask( 0 )
	, m_storage( GGame->GetUniverseStorage()->GetWorldStorage( world ) )
	, m_world( world )
{
}

void CExtractLayersTask::Run()
{
	PC_SCOPE_PIX( CExtractLayersTask );

	CLayerGroup** layerGroups;				
	CLayerInfo** layers;			
	Uint32 numLayerGroups;
	Uint32 numLayers;		

	const Uint32 MAX_LAYER_GROUPS( 4096 );
	const Uint32 MAX_LAYERS( 8192 );
	layerGroups = m_storage->m_layerGroups = new CLayerGroup* [ MAX_LAYER_GROUPS ];
	numLayerGroups = m_storage->m_numLayerGroups = m_world->GetWorldLayers()->GetLayerGroupsForSave( layerGroups, MAX_LAYER_GROUPS );
	if ( numLayerGroups >= MAX_LAYER_GROUPS )
	{
		RED_FATAL("Increase MAX_LAYER_GROUPS in CExtractLayersTask::Run(). Do it NOW. The game will crash now, to make sure you do it.");
		int* forceCrashNow = nullptr;
		*forceCrashNow = 0xF0000000;
	}

	layers = m_storage->m_layers = new CLayerInfo* [ MAX_LAYERS ];
	
	// dynamic layer have no layer info
	layers[ 0 ] = nullptr;
	numLayers = 1;

	// root group layers
	{
		const CLayerGroup::TLayerList& l = m_world->GetWorldLayers()->GetLayers();
		for ( auto it = l.Begin(); it != l.End(); ++it )
		{
			CLayer* layer = ( *it )->GetLayer();
			if ( layer && layer->IsAttached() )
			{
				layers[ numLayers++ ] = *it;
			}
		}
	}

	// layers from all child layer groups
	for ( Uint32 i = 0; i < numLayerGroups; ++i )
	{
		const CLayerGroup::TLayerList& l = layerGroups[ i ]->GetLayers();
		for ( auto it = l.Begin(); it != l.End(); ++it )
		{
			CLayer* layer = ( *it )->GetLayer();
			if ( layer && layer->IsAttached() )
			{
				layers[ numLayers++ ] = *it;
				ASSERT( numLayers < MAX_LAYERS );
				if ( numLayers == MAX_LAYERS )
				{
					RED_LOG( Save, TXT("CExtractLayersTask: NOT collected all layers, due to the limit. Please increase the limit or see why the hell there are so many saveable layers.") );
					m_storage->m_numLayers = numLayers;
					return;
				}
			}
		}
	}

	m_storage->m_numLayers = numLayers;
}

void CExtractLayersTask::WaitForFinish()
{
	while ( false == IsFinished() )
	{
		Red::Threads::YieldCurrentThread();
	};
}

CWorldStorage::CWorldStorage()
	: m_storage( nullptr )			
	, m_layerGroups( nullptr )			
	, m_numLayerGroups( 0 )		
	, m_layers( nullptr )				
	, m_numLayers( 0 )		
	, m_extractLayersTask( nullptr )
	, m_worldBeingCaptured( nullptr )		
	, m_currentCaptureStage( 0 )
	, m_captureTaskParams( nullptr )
{
}

CWorldStorage::~CWorldStorage()
{
	Reset();
}

void CWorldStorage::Reset()
{
	m_depotPath.Clear();

	if ( m_storage )
	{
		delete m_storage;
		m_storage = nullptr;
	}

	m_dynamicLayerStorage.ResetStorage();
}

void CWorldStorage::Load( IGameLoader* loader )
{
	CGameSaverBlock block( loader, CNAME( world ) );

	if ( loader->GetSaveVersion() >= SAVE_VERSION_LAYERS_VISIBILITY_STORAGE )
	{
		m_layerVisibilityStorage.Load( loader );
	}

	loader->ReadValue( CNAME( name ), m_depotPath );
	m_storage = loader->ExtractDataStorage();
	if ( loader->GetSaveVersion() >= SAVE_VERSION_TRANSFERABLE_ENTITIES )
	{
		m_dynamicLayerStorage.RestoreData( loader );
	}
}

Bool CWorldStorage::LoadPlayerOnly( CWorld* activeWorld, IGameLoader* loader, const IdTag& playerIdTag, const TDynArray< IdTag >& attachments )
{
	CGameSaverBlock block( loader, CNAME( world ) );

	if ( loader->GetSaveVersion() >= SAVE_VERSION_LAYERS_VISIBILITY_STORAGE )
	{
		CGameSaverBlock block( loader, CNAME( visibility ) );
	}

	String depotPath;
	loader->ReadValue( CNAME( name ), depotPath );

	// don't load world entities data, we don't need them
	loader->SkipDataStorage();

	if ( loader->GetSaveVersion() >= SAVE_VERSION_TRANSFERABLE_ENTITIES )
	{
		if ( m_dynamicLayerStorage.RestorePlayerEntityDataOnly( loader, playerIdTag, attachments ) )
		{
			// ignore the loaded path, set desired
			m_depotPath = activeWorld->GetDepotPath();
			activeWorld->GetDynamicLayer()->SetStorage( &m_dynamicLayerStorage );

			return true;
		}
	}

	return false;
}

void CWorldStorage::Save( IGameSaver* saver )
{
	CGameSaverBlock block( saver, CNAME( world ) );

	m_layerVisibilityStorage.Save( saver );

	saver->WriteValue( CNAME( name ), m_depotPath );
	saver->AddStorageStream( m_storage );

	m_dynamicLayerStorage.StoreData( saver );
}

void CWorldStorage::CaptureStateStage1( CWorld* world )
{
	ASSERT( nullptr == m_worldBeingCaptured && nullptr == m_extractLayersTask && 0 == m_currentCaptureStage );
	
	m_currentCaptureStage = 1;
	m_worldBeingCaptured = world;
	m_extractLayersTask = new ( CTask::Root ) CExtractLayersTask( world );

	GTaskManager->Issue( *m_extractLayersTask, TSP_Critical );
}

void CWorldStorage::CaptureStateStage2()
{
	ASSERT( nullptr != m_worldBeingCaptured && nullptr != m_extractLayersTask && 1 == m_currentCaptureStage );

	TIMER_BLOCK( stage2 )

	// wait for the current async task to finish
	m_extractLayersTask->WaitForFinish();
	m_extractLayersTask->Release();
	m_extractLayersTask = nullptr;

	m_currentCaptureStage = 2;

	// Create async task to handle capturing world state
	m_captureTaskParams = CParallelForTaskSingleArray< CLayerInfo*, CWorldStorage >::SParams::Create();
	{
		m_captureTaskParams->m_array					= m_layers;
		m_captureTaskParams->m_numElements				= Int32( m_numLayers );
		m_captureTaskParams->m_processFunc				= &CWorldStorage::UpdateLayerStorage;
		m_captureTaskParams->m_processFuncOwner			= this;
		m_captureTaskParams->m_priority					= TSP_Critical;
		m_captureTaskParams->SetDebugName				( TXT("CaptureWorldState") );

		m_captureTaskParams->StartProcessing();	
	}

	END_TIMER_BLOCK( stage2 )
}

void CWorldStorage::CaptureStateStage3()
{
	ASSERT( nullptr != m_worldBeingCaptured && nullptr == m_extractLayersTask && 2 == m_currentCaptureStage && nullptr != m_layerGroups && nullptr != m_layers );

	TIMER_BLOCK( stage3 )

	m_currentCaptureStage = 3;

	if ( !m_storage )
	{
		// MemoryPool_Default here intentional, it's a root level storage
		m_storage = CGameDataStorage< MC_Gameplay, MemoryPool_Default >::Create( 5 * 1024 * 1024, 0 );
	}

	IGameSaver* saver = SGameSaveManager::GetInstance().CreateSaver( m_storage, nullptr );
	if ( !saver )
	{
		WARN_ENGINE( TXT("Failed to capture/save world %s state, reason: failed to create saver"), m_depotPath.AsChar() );
		return;
	}

	TIMER_BLOCK( finishingCaptureTask )
	// Finish the async task ( help finighing it, if it's not finished yet )
	m_captureTaskParams->FinalizeProcessing();
	m_captureTaskParams->Release();
	m_captureTaskParams = nullptr;
	END_TIMER_BLOCK( finishingCaptureTask )

	// save!
	{
		CGameSaverBlock block( saver, CNAME( worldLayerGroups ) );

		// write number of layer groups
		saver->WriteValue( CNAME( count ), m_numLayerGroups );

		// save root layer group always, regardless of ShouldSave()
		CLayerGroup* rootGroup = m_worldBeingCaptured->GetWorldLayers();
		rootGroup->SaveState( saver );

		const Uint32 BUF_SIZE( 1024 );
		AnsiChar buf[ BUF_SIZE ];

		// write all the layer gropus
		for ( Uint32 i = 0; i < m_numLayerGroups; ++i )
		{
			if ( m_layerGroups[ i ]->ShouldSave() )
			{
				Uint32 size = m_layerGroups[ i ]->GetGroupPathName( rootGroup, buf, BUF_SIZE );
				saver->WriteRawAnsiValue( CNAME( path ), buf, size );

				m_layerGroups[ i ]->SaveState( saver );
			}
		}
	}

	delete saver;
	
	CancelCaptureState();

	END_TIMER_BLOCK( stage3 )
}

void CWorldStorage::CancelCaptureState()
{
	if ( m_extractLayersTask )
	{
		m_extractLayersTask->WaitForFinish();
		m_extractLayersTask->Release();
		m_extractLayersTask = nullptr;
	}

	if ( m_captureTaskParams )
	{
		m_captureTaskParams->FinalizeProcessing();
		m_captureTaskParams->Release();
		m_captureTaskParams = nullptr;
	}

	delete m_layerGroups;
	delete m_layers;
	m_layerGroups = nullptr;
	m_layers = nullptr;

	m_numLayerGroups = 0;
	m_numLayers = 0;
	m_currentCaptureStage = 0;
	m_worldBeingCaptured = nullptr;
}

void CWorldStorage::UpdateVisibilityInfo( const String& path, Bool flag )
{
	m_layerVisibilityStorage.UpdateStatus( path, flag );
	#ifndef NO_SAVE_VERBOSITY
		RED_LOG( Save, TXT("Visibility change: %s, layer group: %s is now %s."), m_depotPath.AsChar(), path.AsChar(), flag ? TXT("shown") : TXT("hidden") );
	#endif
}

void CWorldStorage::UpdateLayerStorage( CLayerInfo*& info )
{
	// nullptr means: dynamic layer (it has no layer info)
	if ( nullptr == info )
	{
		m_dynamicLayerStorage.UpdateStorage( m_worldBeingCaptured->GetDynamicLayer() );
	}
	else
	{
		info->GetLayerStorage()->UpdateStorage( info->GetLayer() );
	}
}

void CWorldStorage::ApplyState( CWorld* world )
{
	// Set up dynammic layer storage

	CDynamicLayer* dynamicLayer = world->GetDynamicLayer();
	dynamicLayer->SetStorage( &m_dynamicLayerStorage );

	// Static layers

	if ( m_storage )
	{
		IGameLoader* loader = SGameSaveManager::GetInstance().CreateLoader( m_storage, nullptr, nullptr );
		if ( !loader )
		{
			WARN_ENGINE( TXT("Failed to apply/load world %s state, reason: failed to create loader"), m_depotPath.AsChar() );
			return;
		}

		if ( loader->GetSaveVersion() < SAVE_VERSION_LAYERGROUPS )
		{
			CGameSaverBlock block( loader, CNAME( worldLayerGroups ) );
			world->GetWorldLayers()->LoadState( loader ); // recurrent call
		}
		else
		{
			CGameSaverBlock block( loader, CNAME( worldLayerGroups ) );
			
			// read number of groups
			Uint32 numGroups( 0 );
			loader->ReadValue( CNAME( count ), numGroups );

			// restore root group
			world->GetWorldLayers()->LoadState( loader ); 

			// restore state of the groups iteratively
			for ( Uint32 i = 0; i < numGroups; ++i )
			{
				String path;
				loader->ReadValue( CNAME( path ), path );
				if ( path.Empty() )
				{
					RED_LOG( Save, TXT("Error: path empty for layer group %ld. Please debug."), i );
				}

				CLayerGroup *lg = path.Empty() ? nullptr : world->GetWorldLayers()->FindGroupByPath( path );
				if ( nullptr == lg )
				{
					RED_LOG( Save, TXT("Error: can not find layer group %ls."), path.AsChar() );

					// skip the layer group block
					CGameSaverBlock block( loader, CNAME( layerGroup ) );
					continue;
				}

				lg->LoadState( loader );
			}

		}

		if ( loader->GetSaveVersion() < SAVE_VERSION_TRANSFERABLE_ENTITIES )
		{
			CGameSaverBlock block( loader, CNAME( dynamicEntities ) );
			{
				CGameSaverBlock block( loader, CNAME( dynamicLayer ) );
				m_dynamicLayerStorage.RestoreData( loader );
			}
		}

		delete loader;
	}

	// Restore managed objects

	m_dynamicLayerStorage.RestoreManagedEntities( dynamicLayer );
}

Bool CWorldStorage::GetLayerGroups( CLayerGroup**& outArray, Uint32& outNumber )
{
	if ( m_layerGroups )
	{
		outArray = m_layerGroups;
		outNumber = m_numLayerGroups;
		return true;
	}

	return false;
}

void CWorldStorage::OnLayersVisibilityChanged( const TDynArray< String > &groupsToHide, const TDynArray< String > &groupsToShow )
{
	for ( Uint32 i = 0; i < groupsToHide.Size(); ++i )
	{
		UpdateVisibilityInfo( groupsToHide[ i ], false );
	}

	for ( Uint32 i = 0; i < groupsToShow.Size(); ++i )
	{
		UpdateVisibilityInfo( groupsToShow[ i ], true );
	}
}

Bool CWorldStorage::ShouldLayerGroupBeVisible( const String& path, Bool isVisibleOnStart ) const
{
	#ifndef NO_SAVE_VERBOSITY
		RED_LOG( Save, TXT("Visibility status request: %s, layer group: %s"), m_depotPath.AsChar(), path.AsChar() );
	#endif

	const CLayerVisibilityStorage::EStatus status = m_layerVisibilityStorage.GetStatus( path );
	switch ( status )
	{
	case CLayerVisibilityStorage::LAYER_Hidden:
		#ifndef NO_SAVE_VERBOSITY
			RED_LOG( Save, TXT("result: hidden.") );
		#endif
		return false;
	case  CLayerVisibilityStorage::LAYER_Shown:
		#ifndef NO_SAVE_VERBOSITY
			RED_LOG( Save, TXT("result: shown.") );
		#endif
		return true;
	case CLayerVisibilityStorage::LAYER_NotFound:
	default:
		#ifndef NO_SAVE_VERBOSITY
			RED_LOG( Save, TXT("result: not found (was %s on start)."), isVisibleOnStart ? TXT("visible") : TXT("not visible") );
		#endif
		return isVisibleOnStart;
	};
}

// ------------------------------------------------------------

void CActivePlayerStorage::Reset()
{
	if ( GGame )
	{
		m_idTag = GGame->GetIdTagManager()->GetReservedId( 0 );
	}
	m_position = Vector::ZERO_3D_POINT;
	m_rotation = EulerAngles::ZEROS;
	m_managedAttachmentsIdTags.Clear();
	m_attachmentsEnabled = true;
}

#define FAILSAFE_MANAGED_ATTACHMENTS_LIMIT 10

void CActivePlayerStorage::Load( IGameLoader* loader )
{
	CGameSaverBlock block( loader, CNAME( Player ) );
	
	loader->ReadValue( CNAME( id ), m_idTag );
	loader->ReadValue( CNAME( position ), m_position );
	loader->ReadValue( CNAME( rotation ), m_rotation );
	loader->ReadValue( CNAME( template ), m_templatePath );
	Uint32 attachmentsCount = 0;
	loader->ReadValue( CNAME( count ), attachmentsCount );
	for ( Uint32 i = 0; i < attachmentsCount; ++i )
	{
		IdTag idTag;
		loader->ReadValue( CNAME( idTag ), idTag );
		m_managedAttachmentsIdTags.PushBackUnique( idTag );
		if ( i >= FAILSAFE_MANAGED_ATTACHMENTS_LIMIT )
		{
			break;
		}
	}

	// sanity check
	if ( attachmentsCount > FAILSAFE_MANAGED_ATTACHMENTS_LIMIT )
	{
		RED_LOG( Save, TXT("Player attachmentsCount > FAILSAFE_MANAGED_ATTACHMENTS_LIMIT, broken save?") ); 
	}
}

void CActivePlayerStorage::Save( IGameSaver* saver )
{
	CGameSaverBlock block( saver, CNAME( Player ) );

	saver->WriteValue( CNAME( id ), m_idTag );
	saver->WriteValue( CNAME( position ), m_position );
	saver->WriteValue( CNAME( rotation ), m_rotation );
	saver->WriteValue( CNAME( template ), m_templatePath );

	saver->WriteValue( CNAME( count ), m_managedAttachmentsIdTags.Size() );
	for ( const auto& idTag : m_managedAttachmentsIdTags )
	{
		saver->WriteValue( CNAME( idTag ), idTag );
	}
}

void CActivePlayerStorage::CaptureState( CPeristentEntity* player )
{
	m_idTag = player->GetIdTag();
	m_position = player->GetWorldPosition();
	m_rotation = player->GetRotation();
	m_templatePath = player->GetEntityTemplate()->GetFile()->GetDepotPath();
}

void CActivePlayerStorage::CaptureManagedAttachments( CPeristentEntity* player )
{
	m_managedAttachmentsIdTags.Clear();

	const auto& childAttachments = player->GetChildAttachments();
	for ( const auto& attachment : childAttachments )
	{
		if ( attachment->GetChild()->IsA< CPeristentEntity >() )
		{
			const CPeristentEntity* entity = static_cast< CPeristentEntity* >( attachment->GetChild() );
			if ( entity->IsManaged() )
			{
				m_managedAttachmentsIdTags.PushBackUnique( entity->GetIdTag() );
			}
		}
	}
}

void CActivePlayerStorage::RestoreManagedAttachments( CUniverseStorage* universeStorage, CPeristentEntity* player )
{
	auto activeWorld = GGame->GetActiveWorld();
	auto currentDynamicLayer = activeWorld->GetDynamicLayer();
	auto currentLayerStorage = currentDynamicLayer->GetLayerStorage();
	for ( const auto& idTag : m_managedAttachmentsIdTags )
	{
		auto entityData = universeStorage->TransferEntityData( currentLayerStorage, idTag );
		CEntity* transferedEntity = CPeristentEntity::FindByIdTag( idTag );

		if ( transferedEntity == nullptr )
		{
			if ( entityData )
			{
				// spawn entity and restore it's state
				EntitySpawnInfo spawnInfo;
				entityData->m_spawnData->PushData( spawnInfo );
				spawnInfo.m_idTag = entityData->m_idTag;

				// check if there is any already existing entity (like horse manager) on target dynamic layer
				// this may occur after using RestoreHorseManager() from scripts
				auto alreadyExistingEntityToRemove = currentDynamicLayer->FindEntity( spawnInfo.m_name );
				if ( alreadyExistingEntityToRemove != nullptr )
				{
					alreadyExistingEntityToRemove->Destroy();
				}

				transferedEntity = currentDynamicLayer->CreateEntitySync( spawnInfo );
				transferedEntity->DetachFromWorld( activeWorld );
				entityData->ApplyStateTo( Cast< CPeristentEntity >( transferedEntity ) );
				transferedEntity->AttachToWorld( activeWorld );
			}
		}
		if ( transferedEntity == nullptr )
		{
			continue;
		}
		const auto& parentAttachments = transferedEntity->GetParentAttachments();
		Bool alreadyAttached = false;
		for ( auto attachment : parentAttachments )
		{
			if ( attachment->GetParent() == player )
			{
				alreadyAttached = true;
				break;
			}
		}
		if ( !alreadyAttached )
		{
			transferedEntity->CreateAttachmentImpl( player, CName::NONE );
		}
	}
}

// ------------------------------------------------------------

CUniverseStorage::CUniverseStorage()
{
}

CUniverseStorage::~CUniverseStorage()
{
	Reset();
}

void CUniverseStorage::Reset()
{
	for ( auto it = m_worldStorages.Begin(); it != m_worldStorages.End(); ++it )
	{
		delete *it;
	}
	m_worldStorages.Clear();

	m_playerStorage.Reset();
}

void CUniverseStorage::Load( IGameLoader* loader )
{
	ASSERT( loader->GetSaveVersion() >= SAVE_VERSION_UNIVERSE );

	Reset();	// Get rid of old data

	CTimeCounter timer;

	CGameSaverBlock block( loader, CNAME( universe ) );

	// Load player state

	if ( loader->GetSaveVersion() >= SAVE_VERSION_INCLUDE_PLAYER_STATE )
	{
		m_playerStorage.Load( loader );
	}
	else if ( loader->GetSaveVersion() >= SAVE_VERSION_TRANSFERABLE_ENTITIES )
	{
		loader->ReadValue( CNAME( Player ), m_playerStorage.m_idTag );
	}

	// Load state of all the worlds

	Uint32 count = 0;
	loader->ReadValue( CNAME( count ), count );

	for ( Uint32 i = 0; i < count; i++ )
	{
		CWorldStorage* storage = new CWorldStorage();
		storage->Load( loader );
		m_worldStorages.PushBack( storage );
	}

	LOG_ENGINE( TXT("Universe loaded in %f secs"), timer.GetTimePeriod() );
}

void CUniverseStorage::LoadPlayerOnly( CWorld* activeWorld, IGameLoader* loader )
{
	Reset();

	CGameSaverBlock block( loader, CNAME( universe ) );

	// Load player state
	if ( loader->GetSaveVersion() >= SAVE_VERSION_INCLUDE_PLAYER_STATE )
	{
		m_playerStorage.Load( loader );
	}
	else if ( loader->GetSaveVersion() >= SAVE_VERSION_TRANSFERABLE_ENTITIES )
	{
		loader->ReadValue( CNAME( Player ), m_playerStorage.m_idTag );
	}

	Uint32 count = 0;
	loader->ReadValue( CNAME( count ), count );

	Bool haveIt = false;
	CWorldStorage* storage = new CWorldStorage();
	for ( Uint32 i = 0; i < count; i++ )
	{		
		if ( storage->LoadPlayerOnly( activeWorld, loader, m_playerStorage.m_idTag, m_playerStorage.m_managedAttachmentsIdTags ) )
		{
			haveIt = true;
			break;
		}
	}

	if ( !haveIt )
	{
		HALT( "A savegame without the player?? WTF?" );
		delete storage;
		return;
	}

	m_worldStorages.PushBack( storage );
}

void CUniverseStorage::Save( IGameSaver* saver )
{
	TIMER_BLOCK( time )

	CGameSaverBlock block( saver, CNAME( universe ) );

	// Save player state

	m_playerStorage.Save( saver );

	// Save state of all worlds

	saver->WriteValue( CNAME( count ), m_worldStorages.Size() );
	for ( auto it = m_worldStorages.Begin(); it != m_worldStorages.End(); ++it )
	{
		(*it)->Save( saver );
	}

	END_TIMER_BLOCK( time )
}

CWorldStorage* CUniverseStorage::GetWorldStorage( const CWorld* world )
{
	return GetWorldStorage( world->GetDepotPath() );
}

CWorldStorage* CUniverseStorage::GetWorldStorage( const String& world )
{
	// Find or create matching world storage

	CWorldStorage* worldStorage = nullptr;
	for ( auto it = m_worldStorages.Begin(); it != m_worldStorages.End(); ++it )
	{
		if ( (*it)->m_depotPath.EqualsNC( world ) )
		{
			worldStorage = *it;
			break;
		}
	}

	if ( nullptr == worldStorage )
	{
		worldStorage = new CWorldStorage();
		worldStorage->m_depotPath = world;
		m_worldStorages.PushBack( worldStorage );
	}

	return worldStorage;
}

const CWorldStorage* CUniverseStorage::GetWorldStorage( const CWorld* world ) const
{
	return GetWorldStorage( world->GetDepotPath() );
}

const CWorldStorage* CUniverseStorage::GetWorldStorage( const String& world ) const
{
	for ( auto it = m_worldStorages.Begin(); it != m_worldStorages.End(); ++it )
	{
		if ( (*it)->m_depotPath.EqualsNC( world ) )
		{
			return *it;
		}
	}

	return nullptr;
}

void CUniverseStorage::ApplyWorldState( CWorld* world )
{
	CTimeCounter timer;

	GetWorldStorage( world )->ApplyState( world );

	LOG_ENGINE( TXT("State applied to world %s in %f secs"), world->GetDepotPath().AsChar(), timer.GetTimePeriod() );
}

void CUniverseStorage::CapturePlayerState( CPeristentEntity* player )
{
	m_playerStorage.CaptureState( player );
}


const CActivePlayerStorage& CUniverseStorage::GetActivePlayerStorage() const
{
	return m_playerStorage;
}

void CUniverseStorage::CapturePlayerManagedAttachments( CEntity* playerEntity )
{	
	if ( m_playerStorage.AttachmentsEnabled() )
	{
		m_playerStorage.CaptureManagedAttachments( Cast< CPeristentEntity >( playerEntity ) );
	}
}

void CUniverseStorage::RestorePlayerManagedAttachments( CEntity* playerEntity )
{
	if ( m_playerStorage.AttachmentsEnabled() )
	{
		m_playerStorage.RestoreManagedAttachments( this, Cast< CPeristentEntity >( playerEntity ) );
	}
}

void CUniverseStorage::EnablePlayerAttachments()
{
	m_playerStorage.EnableAttachments();
}

void CUniverseStorage::DisablePlayerAttachments()
{
	m_playerStorage.DisableAttachments();
}

CLayerStorage::EntityData* CUniverseStorage::TransferEntityData( CLayerStorage* targetStorage, const IdTag& idTag )
{
	// Search all other worlds

	for ( auto it = m_worldStorages.Begin(); it != m_worldStorages.End(); ++it )
	{
		if ( &(*it)->m_dynamicLayerStorage == targetStorage ) // Skip given world
		{
			continue;
		}

		// Transfer from current entity world into given world

		if ( CLayerStorage::EntityData* entityData = (*it)->m_dynamicLayerStorage.TransferEntityData( targetStorage, idTag ) )
		{
			return entityData;
		}
	}
	return nullptr;
}

void CUniverseStorage::RemoveEntity( CWorld* world, const IdTag& idTag )
{
	GetWorldStorage( world )->m_dynamicLayerStorage.RemoveEntityState( idTag );
}

void CUniverseStorage::OnLayersVisibilityChanged( const String& world, const TDynArray< String > &groupsToHide, const TDynArray< String > &groupsToShow )
{
	if ( groupsToHide.Empty() && groupsToShow.Empty() )
	{
		return;
	}

	RED_LOG_SPAM( Save, TXT("world: %s, groupsToHide: %ld, groupsToShow: %ld"), world.AsChar(), groupsToHide.Size(), groupsToShow.Size() );

	CWorldStorage* storage = GetWorldStorage( world );
	storage->OnLayersVisibilityChanged( groupsToHide, groupsToShow );
}

Bool CUniverseStorage::ShouldLayerGroupBeVisible( const CLayerGroup* group ) const
{
	const CWorld* world = group->GetWorld();
	if ( nullptr == world )
	{
		return false;
	}

	const CWorldStorage* storage = GetWorldStorage( world );
	if ( nullptr == storage )
	{
		return group->IsVisibleOnStart();
	}

	return storage->ShouldLayerGroupBeVisible( group->GetGroupPathName( world->GetWorldLayers() ), group->IsVisibleOnStart() ); 
}