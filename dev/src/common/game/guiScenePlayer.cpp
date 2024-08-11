/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "../core/rttiSystem.h"
#include "../core/softHandle.h"
#include "../core/loadingJobManager.h"
#include "../core/depot.h"
#include "../engine/appearanceComponent.h"
#include "../engine/entity.h"
#include "../engine/entityTemplate.h"
#include "../engine/renderFrame.h"
#include "../engine/mesh.h"
#include "../engine/world.h"
#include "../engine/bitmapTexture.h"
#include "../engine/jobSpawnEntity.h"

#include "../engine/guiGlobals.h"
#include "../engine/drawableComponent.h"
#include "../engine/dynamicLayer.h"
#include "../engine/environmentDefinition.h"
#include "../engine/environmentManager.h"
#include "../engine/meshComponent.h"
#include "../physics/physicsWorld.h"
#include "../engine/worldTick.h"

#include "../engine/renderCommands.h"
#include "../engine/renderGameplayRenderTargetInterface.h"

#include "movingAgentComponent.h"
#include "movingPhysicalAgentComponent.h"

#include "guiScenePlayerListener.h"

#include "guiScenePlayer.h"

// Masks to specify what need to be rendered in this fake world
static EShowFlags DEFAULT_SHOW_FLAGS[] = 
{
	SHOW_Meshes					,
	SHOW_Flares					,
	SHOW_Brushes				,
	SHOW_Shadows				,
	SHOW_Dimmers				,
	SHOW_Lights					,
	SHOW_CameraLights			,
	SHOW_UseRegularDeferred		,
	SHOW_Particles				,
	SHOW_PostProcess			,
	SHOW_Lighting				,
	SHOW_TSLighting				,
	SHOW_Decals					,
	SHOW_DynamicDecals			,
	SHOW_Emissive				,
	SHOW_PreferTemporalAA		,
	SHOW_OnScreenMessages		,
	SHOW_PostProcess			,
	SHOW_Apex					,
	SHOW_AllowApexShadows		,
	SHOW_HiResEntityShadows		,

	SHOW_ReversedProjection		,

	SHOW_HairAndFur				,
	SHOW_ForwardPass			,
	SHOW_GeometrySkinned		,
	SHOW_GeometryStatic			,

	SHOW_RenderTargetBackground	,

	SHOW_MAX_INDEX				// the way we mark an end
};

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CGuiScenePlayer );

IMPLEMENT_ENGINE_CLASS( SGuiEnhancementInfo );

//////////////////////////////////////////////////////////////////////////
// SGuiScenePlayerInfo
//////////////////////////////////////////////////////////////////////////

SGuiScenePlayerInfo::SGuiScenePlayerInfo()
	: m_cameraFarPlane( 128.0f )
	, m_updatePhysics( true )
	, m_uniquePhysicsWorld( false )
	, m_backgroundTexture( nullptr )
{
	Red::MemoryZero( m_renderingMask, sizeof(m_renderingMask) );
}

SGuiScenePlayerInfo::~SGuiScenePlayerInfo()
{
	m_backgroundTexture = nullptr;
}

//////////////////////////////////////////////////////////////////////////
// CGuiScenePlayer
//////////////////////////////////////////////////////////////////////////
CGuiScenePlayer::CGuiScenePlayer()
	: m_world( nullptr )
	, m_entity( nullptr )
{
	// Copy rendering masks into class member
	Red::MemoryCopy( m_sceneInfo.m_renderingMask, DEFAULT_SHOW_FLAGS, sizeof(DEFAULT_SHOW_FLAGS) );
}

Bool CGuiScenePlayer::Init( CClass* worldClass /*=ClassID< CWorld >()*/ )
{
	if ( !worldClass || ! worldClass->IsA< CWorld >() )
	{
		GUI_ERROR(TXT("Invalid world class!"));
		return false;
	}

	m_world = CreateWorld( worldClass, this );
	if ( ! m_world )
	{
		return false;
	}

	SetShadowCascadeSettings( Vector(3.0f,6.0f,8.0f,16.0f) , Vector(0.001f,0.002f,0.004f,0.008f) , 2 );

	return true;
}

CGuiScenePlayer::~CGuiScenePlayer()
{
}

void CGuiScenePlayer::SetEntityTemplate( const THandle< CEntityTemplate >& entityTemplate , const CName& animationName )
{
	// SetTemplate( entityTemplate );
	m_nextSpawn.Clear();
	m_nextSpawn.m_entityTemplate	= entityTemplate;
	m_nextSpawn.m_animation			= animationName;
}

void CGuiScenePlayer::ApplyAppearanceToSceneEntity( const CName& appearanceName )
{
	if ( m_entity )
	{
		for ( ComponentIterator< CAppearanceComponent > it( m_entity ); it; ++it )
		{
			(*it)->ApplyAppearance( appearanceName );
			return;
		}
	}
}

void CGuiScenePlayer::ApplyAnimationOnEntity( CEntity * entity, const CName& animationName )
{
	if( !entity )
	{
		return;
	}
	if ( animationName == CName::NONE )
	{
		return;
	}

	CAnimatedComponent* ac = entity->GetRootAnimatedComponent();
	if ( ac )
	{
		ac->PlayAnimationOnSkeleton( animationName );
	}

}

Bool CGuiScenePlayer::ApplyItemsToSceneEntity( CEntity* entity, const TDynArray< SItemUniqueId >& itemIds, const TDynArray< SGuiEnhancementInfo >& enhancementsItems, TDynArray< SItemUniqueId >* delayedEnhancementsItems )
{
	// Sanity check
	if( !entity || !m_world )
	{
		return false;
	}

	CEntityTemplate* entityTemplate = entity->GetEntityTemplate();
	if ( !entityTemplate )
	{
		return false; 
	}
	CInventoryComponent* inventoryComponent = entity->FindComponent<CInventoryComponent>();
	if ( !inventoryComponent )
	{
		return false;
	}

	Bool success = true;
	CName heldCategory;
	SItemUniqueId itemUniqueId;

	// Copy into temporary container; elements can get removed during iteration
	const auto inventoryItems = inventoryComponent->GetItems();

	for ( Uint32 i = 0; i < inventoryItems.Size(); ++i )
	{
//		if ( items.Exist( inventoryItems[i].GetName() ) )
//		{
//			continue;
//		}

		itemUniqueId = inventoryItems[i].GetUniqueId();

		// remember held item category
		if ( inventoryComponent->IsItemHeld( inventoryItems[i].GetName() ) )
		{
			heldCategory = inventoryItems[ i ].GetCategory();
		}

		inventoryComponent->UnMountItem( itemUniqueId );

		if ( inventoryComponent->GetCategoryDefaultItem( inventoryItems[i].GetCategory() ) != inventoryItems[i].GetName() )
		{
			inventoryComponent->RemoveItem( itemUniqueId );
		}
	}

//	for ( Uint32 i = 0; i < itemIds.Size(); ++i )
//	{
// 		if ( inventoryComponent->IsItemMounted( itemIds[i] ) || inventoryComponent->IsItemHeld( itemIds[i] ) )
// 		{
// 			SItemUniqueId id = inventoryComponent->GetItemId( itemIds[i] );
// 			SInventoryItem* item = inventoryComponent->GetItem( id );
// 			
// 			// reapply enhancements
// 			item->RemoveAllSlots();
// 			if ( !EnhanceItem( *inventoryComponent, itemIds[i], enhancementsItems, true ) )
// 			{
// 				success = false;
// 				delayedEnhancementsItems->PushBack( itemIds[i] );
// 			}
// 			continue;
// 		}

	for ( Uint32 i = 0; i < itemIds.Size(); ++i )
	{
		CEntity* playerEntity = GGame->GetPlayerEntity();
		CInventoryComponent* playerInvComp = static_cast< CGameplayEntity* >( playerEntity )->GetInventoryComponent();
		SInventoryItem* playerItem = playerInvComp->GetItem( itemIds[i] );
			
//		if ( !inventoryComponent->HasItem( playerInvComp->GetItem( itemIds [i ] ) )
//		{
		CInventoryComponent::SAddItemInfo newItemInfo;
		newItemInfo.m_informGui = false;

		TDynArray< SItemUniqueId> addItemId = inventoryComponent->AddItem( *playerItem, newItemInfo );
		if ( addItemId.Empty() )
		{
			RED_ASSERT( !addItemId.Empty(), TXT( "No Unique ID") );
		}
//		}

		// if item with given category was held mount item to hand
		SInventoryItem* item = inventoryComponent->GetItem( addItemId[ 0 ] );

		CInventoryComponent::SMountItemInfo mountItemInfo;
		if ( item->GetCategory() == heldCategory )
		{
			mountItemInfo.m_toHand = true;
		}

		mountItemInfo.m_proxy = item->GetItemEntityProxy();

		inventoryComponent->MountItem( addItemId[ 0 ], mountItemInfo );

		if ( !EnhanceItem( *inventoryComponent, item->GetName(), enhancementsItems ) )
		{
			success = false;
			delayedEnhancementsItems->PushBack( itemIds[ i ] );
		}
	}

	return success;
}

Int32 CGuiScenePlayer::GetEnhancementNum( CName item, const TDynArray< SGuiEnhancementInfo >& enhancementsItems )
{
	Int32 num = 0;
	for ( Uint32 k = 0; k < enhancementsItems.Size(); k++ )
	{
		if ( enhancementsItems[k].m_enhancedItem == item )
		{
			num++;
		}
	}
	return num;
}

Bool CGuiScenePlayer::EnhanceItem( CInventoryComponent& inventoryComponent, CName item, const TDynArray< SGuiEnhancementInfo >& enhancementsItems, Bool updateEnhancement /* = false */ )
{
	SItemUniqueId id = inventoryComponent.GetItemId( item );
	SInventoryItem* invItem = inventoryComponent.GetItem( id );

	if ( !invItem )
	{
		return false;
	}

	for ( Uint32 k = 0; k < enhancementsItems.Size(); k++ )
	{
		if ( enhancementsItems[k].m_enhancedItem == item )
		{
			invItem->AddSlotItem( enhancementsItems[k].m_enhancement, true, 0 );
			updateEnhancement = true;
		}

		if ( enhancementsItems[k].m_oilItem == item )
		{
			inventoryComponent.PlayItemEffect( id, GetOilFxName( enhancementsItems[k].m_oil ) );
		}

		if ( enhancementsItems[k].m_dyeItem == item )
		{
			if ( !invItem->GetItemEntity() )
			{
				return false;
			}
			
			CAppearanceComponent* appearanceComponent = invItem->GetItemEntity()->FindComponent< CAppearanceComponent >();
			if ( !appearanceComponent )
			{
				return false;
			}

			//invItem->SetDyeColor( enhancementsItems[k].m_dye, enhancementsItems[k].m_dyeColor );
			invItem->SetDyePreviewColor( enhancementsItems[k].m_dyeColor );
		}
	}

	if ( updateEnhancement )
	{
		CItemEntity* itemEntity = inventoryComponent.GetItemEntityUnsafe( id );
		if ( itemEntity )
		{
			itemEntity->CallEvent( CNAME( OnItemEnhanced ) );
		}
		else
		{
			m_spawningItems.PushBack( id );
		}
	}

	return true;
}

CName CGuiScenePlayer::GetOilFxName( CName oilName )
{
	if ( oilName == CNAME( BeastOil_1 ) || oilName == CNAME( BeastOil_2 ) || oilName == CNAME( BeastOil_3 ) )
	{
		return CNAME( oil_beast );
	}
	else if ( oilName == CNAME( CursedOil_1 ) || oilName == CNAME( CursedOil_2 ) || oilName == CNAME( CursedOil_3 ) )
	{
		return CNAME( oil_cursed );
	}
	else if ( oilName == CNAME( HangedManVenom_1 ) || oilName == CNAME( HangedManVenom_2 ) || oilName == CNAME( HangedManVenom_3 ) )
	{
		return CNAME( oil_venom );
	}
	else if ( oilName == CNAME( HybridOil_1 ) || oilName == CNAME( HybridOil_2 ) || oilName == CNAME( HybridOil_3 ) )
	{
		return CNAME( oil_hybrid );
	}
	else if ( oilName == CNAME( InsectoidOil_1 ) || oilName == CNAME( InsectoidOil_2 ) || oilName == CNAME( InsectoidOil_3 ) )
	{
		return CNAME( oil_insectoid );
	}
	else if ( oilName == CNAME( MagicalsOil_1 ) || oilName == CNAME( MagicalsOil_2 ) || oilName == CNAME( MagicalsOil_3 ) )
	{
		return CNAME( oil_magical );
	}
	else if ( oilName == CNAME( NecrophageOil_1 ) || oilName == CNAME( NecrophageOil_2 ) || oilName == CNAME( NecrophageOil_3 ) )
	{
		return CNAME( oil_necrophage );
	}
	else if ( oilName == CNAME( SpecterOil_1 ) || oilName == CNAME( SpecterOil_2 ) || oilName == CNAME( SpecterOil_3 ) )
	{
		return CNAME( oil_specter );
	}
	else if ( oilName == CNAME( VampireOil_1 ) || oilName == CNAME( VampireOil_2 ) || oilName == CNAME( VampireOil_3 ) )
	{
		return CNAME( oil_vampire );
	}
	else if ( oilName == CNAME( DraconideOil_1 ) || oilName == CNAME( DraconideOil_2 ) || oilName == CNAME( DraconideOil_3 ) )
	{
		return CNAME( oil_draconide );
	}
	else if ( oilName == CNAME( OgreOil_1 ) || oilName == CNAME( OgreOil_2 ) || oilName == CNAME( OgreOil_3 ) )
	{
		return CNAME( oil_ogre );
	}
	else if ( oilName == CNAME( RelicOil_1 ) || oilName == CNAME( RelicOil_2 ) || oilName == CNAME( RelicOil_3 ) )
	{
		return CNAME( oil_relic );
	}
	return CName::NONE;
}

void CGuiScenePlayer::UpdateEntityItems( const TDynArray< SItemUniqueId >& items, const TDynArray< SGuiEnhancementInfo >& enhancements )
{
	m_delayedItemsSpawn = items;
	m_delayedEnhancementsItemsSpawn = enhancements;
	m_delayedEnhancementsItems.ClearFast();
}

void CGuiScenePlayer::SetEnvironment( const THandle< CEnvironmentDefinition >& envDef, const EulerAngles& sunRotation )
{
	UpdateWorldEnvironment( m_world.Get(), envDef.Get(), sunRotation );
}

void CGuiScenePlayer::UpdateCamera( const Vector& position, const EulerAngles& rotation, Float fov  )
{
	m_camera.m_position = m_initialEntityPosition + position;
	m_camera.m_rotation = rotation;
	m_camera.m_fov = fov;
}

void CGuiScenePlayer::UpdateEntityTransform( const Vector* position, const EulerAngles* rotation,  const Vector* scale )
{
	if (m_entity)
	{
		Vector finalPos = m_initialEntityPosition + *position;

		m_entity->SetRawPlacement( &finalPos, rotation, scale);
	}
}

Vector CGuiScenePlayer::GetSpawnPosition()
{
	if( GGame && GGame->GetPlayerEntity() ) 
	{
		CEntity* player = GGame->GetPlayerEntity();
		return player->GetLocalToWorld().GetTranslation();
	}
	return Vector::ZERO_3D_POINT; 
}

void CGuiScenePlayer::EnablePhysics( Bool value )
{
	m_sceneInfo.m_updatePhysics = value;
}

void CGuiScenePlayer::OnSerialize( IFile& file )
{
	if ( file.IsGarbageCollector() )
	{
		file << m_world;
		for ( SSpawnJobContext& context : m_cancelledSpawnJobContexts )
		{
			file << context.m_spawnInfo.m_entityTemplate;
		}
		file << m_currentSpawnJobContext.m_spawnInfo.m_entityTemplate;
		
		file << m_nextSpawn.m_entityTemplate;
	}
}

void CGuiScenePlayer::Uninit()
{
	for ( SSpawnJobContext& context : m_cancelledSpawnJobContexts )
	{
		context.m_spawnJob->Release();
		context = SSpawnJobContext();
	}

	if ( m_currentSpawnJobContext.IsValid() )
	{
		m_currentSpawnJobContext.m_spawnJob->Cancel();
		m_currentSpawnJobContext.m_spawnJob->Release();
		m_currentSpawnJobContext = SSpawnJobContext();
	}

	// Avoid mystery crashes on shutting down. Not great.
	SJobManager::GetInstance().FlushPendingJobs();

	ClearScene();

	if ( m_world )
	{
		m_world->DelayedActions();
		m_world->Shutdown();
		m_world->Discard();
		m_world = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////

CWorld* CGuiScenePlayer::CreateWorld( CClass* worldClass, CObject* parent )
{
	CWorld* world = ::CreateObject< CWorld >( worldClass, parent, OF_Transient );
	if ( ! world )
	{
		GUI_ERROR(TXT("CGuiScenePlayer: Could not create world!"));
		return nullptr;
	}

	WorldInitInfo initInfo;
	initInfo.m_previewWorld = true;
	initInfo.m_initializePhysics	= m_sceneInfo.m_uniquePhysicsWorld;	
	initInfo.m_sharePhysicalWorld	= m_sceneInfo.m_uniquePhysicsWorld ? nullptr : GGame->GetActiveWorld();

	world->Init( initInfo );

	return world;
}

void CGuiScenePlayer::ClearScene( )
{
	// Destroy all previously spawned items
	// It needs to be here, bcoz m_world can actually be long time nullptr, and these items cannot leak - they'd cause crash
	
	for ( THashMap< CName, CItemEntity* >::iterator it = m_itemsArray.Begin(); it != m_itemsArray.End(); ++it )
	{
		if( (*it).m_second )
		{
			(*it).m_second->Destroy();
		}
	}
	m_itemsArray.Clear();

	if( !m_world )
	{
		return;
	}

	m_world->DelayedActions();

	if ( m_entity )
	{
		CEntity* entity = m_entity;
		CWorld* world = m_world;

		m_entity = nullptr;

		world->DelayedActions();

		entity->RemoveFromRootSet();

		entity->DetachFromWorld( world );
		world->DelayedActions();

		entity->Destroy();
		world->DelayedActions();
		
		ForEach( m_listeners, [](IGuiScenePlayerListener* listener ){ listener->OnGuiSceneEntityDestroyed(); });
	}
}


CJobSpawnEntity* CGuiScenePlayer::CreateSpawnJob( const CGuiScenePlayer::SNextSpawnInfo& spawnInfo, CWorld* world )
{
	EntitySpawnInfo einfo;
	einfo.m_template = spawnInfo.m_entityTemplate;
	einfo.m_spawnPosition = m_initialEntityPosition = GetSpawnPosition();

	if ( Cast< CActor >( spawnInfo.m_entityTemplate->GetEntityObject() ) )
	{
		einfo.m_entityClass = ClassID< CActor >();
	}
	einfo.m_name = TXT("PreviewEntity");
	einfo.m_detachTemplate = false;
	einfo.m_previewOnly = true; // TBD: Uncached

	// Create new entity in this layer, done in the background
	CJobSpawnEntity* spawnJob = world->GetDynamicLayer()->CreateEntityAsync( Move( einfo ) );
	if ( ! spawnJob )
	{
		GUI_ERROR(TXT("CGuiScenePlayer: Failed to spawn gui entity! '%ls'"), spawnInfo.m_entityTemplate->GetFriendlyName().AsChar() );
	}

	return spawnJob;
}

Bool CGuiScenePlayer::InitEntity( CEntity* entity , const SNextSpawnInfo& spawnInfo )
{
	RED_FATAL_ASSERT( entity, "No entity" );
	
	// Stop the entity from falling
	CMovingPhysicalAgentComponent* mac = entity->FindComponent<CMovingPhysicalAgentComponent>();
	if ( mac )
	{
		mac->SetMotionEnabled( false, CMovingAgentComponent::LS_GUI );
	}

	// No dissolves for all drawable components
	for ( ComponentIterator< CDrawableComponent > it( entity ); it; ++it )
	{
		(*it)->SetNoDisolves( true );
	}
	
	CWorld* world = entity->GetLayer()->GetWorld();
	world->DelayedActions();
	entity->CreateStreamedComponents( SWN_NotifyWorld );
	entity->AddToRootSet();
	// entity->SetStreamingLock( true );

	CInventoryComponent* inventoryComponent = entity->FindComponent<CInventoryComponent>();
	if ( inventoryComponent )
	{
		inventoryComponent->SpawnMountedItems();
	}
	if( !m_delayedItemsSpawn.Empty() )
	{
		if ( ApplyItemsToSceneEntity( entity , m_delayedItemsSpawn, m_delayedEnhancementsItemsSpawn, &m_delayedEnhancementsItems ) )
		{
			m_delayedItemsSpawn.ClearFast();
			m_delayedEnhancementsItemsSpawn.ClearFast();
		}
	}

	ApplyAnimationOnEntity( entity , spawnInfo.m_animation );

	return true;
}

void CGuiScenePlayer::Tick( Float timeDelta )
{
	TickSpawnJobs();
	TickWorld( timeDelta );
}

void CGuiScenePlayer::UpdateBackgroundTexture( CBitmapTexture* resource )
{
	m_sceneInfo.m_backgroundTexture = resource;
}

void CGuiScenePlayer::SetShadowCascadeSettings( const Vector& cascadeRanges, const Vector& cascadeKernels, Uint32 numCascades )
{
	if( m_world )
	{
		RED_ASSERT( numCascades > 0 && numCascades <= 2 , TXT("Invalid number of inventory shadow cascades") );

		CWorldShadowConfig cfg = m_world->GetShadowConfig();

		cfg.m_numCascades = numCascades;

		cfg.m_cascadeRange1 = cascadeRanges.X;
		cfg.m_cascadeRange2 = cascadeRanges.Y;
		cfg.m_cascadeRange3 = cascadeRanges.Z;
		cfg.m_cascadeRange4 = cascadeRanges.W;

		cfg.m_cascadeFilterSize1 = cascadeKernels.X;
		cfg.m_cascadeFilterSize2 = cascadeKernels.Y;
		cfg.m_cascadeFilterSize3 = cascadeKernels.Z;
		cfg.m_cascadeFilterSize4 = cascadeKernels.W;

		m_world->SetShadowConfig( cfg );
	}
}

void CGuiScenePlayer::RenderWorld( IRenderGameplayRenderTarget* renderTarget )
{
	if( !m_world || !renderTarget )
	{
		return;
	}

	const Float aspectRatio = renderTarget->GetAspectRatio();

	const Vector& cameraPosition = m_camera.m_position;
	const EulerAngles& cameraRotation = m_camera.m_rotation;
	const Float cameraFov = m_camera.m_fov;
	const Float cameraZoom = m_camera.m_zoom;

	CRenderCamera renderCamera( cameraPosition, cameraRotation, cameraFov, aspectRatio, 0.1f, m_sceneInfo.m_cameraFarPlane, cameraZoom );
	CRenderFrameInfo info( renderTarget->GetImageWidth(), renderTarget->GetImageHeight(), RM_Shaded, m_sceneInfo.m_renderingMask, renderCamera );

	info.m_camera.SetNonDefaultNearRenderingPlane();
	info.m_camera.SetNonDefaultFarRenderingPlane();

	info.m_clearColor				= Color::CLEAR;
	info.m_customRenderResolution	= true;
	info.m_tonemapFixedLumiance		= Config::cvInventoryFixedLuminance.Get();
	info.m_backgroundTextureColorScale = Vector( Config::cvInventoryBgColorScaleR.Get(), Config::cvInventoryBgColorScaleG.Get(), Config::cvInventoryBgColorScaleB.Get(), 1.0f );

	if( m_sceneInfo.m_backgroundTexture )
	{
		renderTarget->SetTextureMask( m_sceneInfo.m_backgroundTexture->GetRenderResource() );
		m_sceneInfo.m_backgroundTexture = nullptr;
	}
	
	info.SetGameplayRenderTarget( renderTarget );
	info.SetShadowConfig( m_world->GetShadowConfig() );

	CRenderFrame* frame = m_world->GenerateFrame( nullptr, info );
	if ( !frame )
	{
		return;
	}

	m_cachedFrameInfo = info;

	IRenderScene* renderScene = m_world->GetRenderSceneEx();
	if ( !renderScene )
	{
		frame->Release();
		return;
	}

	( new CRenderCommand_RenderScene( frame, renderScene ) )->Commit();
	frame->Release();
	
}

// FIXME: Make less messy!
void CGuiScenePlayer::TickSpawnJobs()
{
	if ( m_nextSpawn.IsValid() )
	{
		ClearScene();

		// If we want to spawn a new entity, cancel any in-flight spawn jobs
		if ( m_currentSpawnJobContext.IsValid() )
		{
			m_currentSpawnJobContext.m_spawnJob->Cancel();
			m_cancelledSpawnJobContexts.PushBack( m_currentSpawnJobContext );
			m_currentSpawnJobContext = SSpawnJobContext();
		}
	}

	// Clear out cancelled spawn jobs if finished
	for ( Int32 j = m_cancelledSpawnJobContexts.SizeInt()-1; j >= 0; --j )
	{
		SSpawnJobContext& context = m_cancelledSpawnJobContexts[ j ];
		CJobSpawnEntity* spawnJob = context.m_spawnJob;
		if ( spawnJob->HasEnded() )
		{
			spawnJob->Release();
			m_cancelledSpawnJobContexts.RemoveAt( j );
		}
	}

	// Create a new spawn job if needed
	if ( m_nextSpawn.IsValid() && m_cancelledSpawnJobContexts.Size() <= MaxCancelledSpawnJobs )
	{
		RED_FATAL_ASSERT( ! m_currentSpawnJobContext.IsValid(), "Current spawn job should have been cancelled!" );
		CJobSpawnEntity* spawnJob = CreateSpawnJob( m_nextSpawn , m_world.Get() );
		if ( spawnJob )
		{
			m_currentSpawnJobContext.m_spawnJob = spawnJob;
			m_currentSpawnJobContext.m_spawnInfo = m_nextSpawn;
		}
		m_nextSpawn.Clear();
	}

	// Check current spawn job for completion and clean up
	// Update current scene entity and notifies listeners when entity is fully spawned with its components all attached
	if ( m_currentSpawnJobContext.m_spawnJob && m_currentSpawnJobContext.m_spawnJob->HasEnded() )
	{
		CJobSpawnEntity* spawnJob = m_currentSpawnJobContext.m_spawnJob;
		if ( !spawnJob->HasFinishedWithoutErrors() )
		{
			spawnJob->Release();
			m_currentSpawnJobContext = SSpawnJobContext();
		}
		else
		{
			if ( ! m_currentSpawnJobContext.m_spawnedEntity )
			{
				THandle< CEntity > ent = spawnJob->GetSpawnedEntity();
				if ( ent && InitEntity( ent.Get() , m_currentSpawnJobContext.m_spawnInfo ) )
				{
					m_currentSpawnJobContext.m_spawnedEntity = ent;
				}
				else
				{
					GUI_ERROR(TXT("CGuiScenePlayer: Failed to spawn entity"));
					spawnJob->Release();
					m_currentSpawnJobContext = SSpawnJobContext();
				}
			}
			else
			{
				// Components finished attaching
				if ( m_currentSpawnJobContext.m_spawnedEntity->IsSpawned() )
				{
					m_entity = m_currentSpawnJobContext.m_spawnedEntity;

					m_world->GetEffectManager().SetReferencePosition( m_initialEntityPosition );

					ForEach( m_listeners, [&](IGuiScenePlayerListener* listener) {
						listener->OnGuiSceneEntitySpawned( m_entity );
					} );
					m_currentSpawnJobContext.m_spawnJob->Release();
					m_currentSpawnJobContext = SSpawnJobContext();
				}
			}
		}	
	}

	if( m_entity )
	{
		CInventoryComponent* inventoryComponent = m_entity->FindComponent<CInventoryComponent>();
		CEntity* playerEntity = GGame->GetPlayerEntity();
		CInventoryComponent* playerInvComp = static_cast< CGameplayEntity* >( playerEntity )->GetInventoryComponent();
		SItemUniqueId id;
		
		if ( !m_delayedEnhancementsItems.Empty() && inventoryComponent )
		{
			TDynArray< SItemUniqueId > itemsRemoved;

			for ( Uint32 i = 0; i < m_delayedEnhancementsItems.Size(); ++i )
			{
				SInventoryItem* playerItem = playerInvComp->GetItem( m_delayedEnhancementsItems[i] );
				if ( EnhanceItem( *inventoryComponent, playerItem->GetName(), m_delayedEnhancementsItemsSpawn, true ) )
				{
					itemsRemoved.PushBack( m_delayedEnhancementsItems[i] );
				}
			}

			for ( Uint32 j = 0; j < itemsRemoved.Size(); ++j )
			{
				m_delayedEnhancementsItems.Remove( itemsRemoved[ j ] );
			}
		}

		if ( !m_delayedItemsSpawn.Empty() )
		{
			if ( ApplyItemsToSceneEntity( m_entity , m_delayedItemsSpawn, m_delayedEnhancementsItemsSpawn, &m_delayedEnhancementsItems ) )
			{
				m_delayedEnhancementsItemsSpawn.ClearFast();
			}
			m_delayedItemsSpawn.ClearFast();
		}
		
	}

	if ( !m_spawningItems.Empty() )
	{
		ApplyFxToSpawnedItems();
	}
}

void CGuiScenePlayer::ApplyFxToSpawnedItems()
{
	// Sanity check
	if( !m_entity || !m_world )
	{
		return;
	}

	CEntityTemplate* entityTemplate = m_entity->GetEntityTemplate();
	if ( !entityTemplate )
	{
		return; 
	}
	CInventoryComponent* inventoryComponent = m_entity->FindComponent<CInventoryComponent>();
	if ( !inventoryComponent )
	{
		return;
	}

	for ( Uint32 i = 0; i < m_spawningItems.Size(); ++i )
	{
		CItemEntity* itemEntity = inventoryComponent->GetItemEntityUnsafe( m_spawningItems[i] );

		if ( itemEntity )
		{
			itemEntity->CallEvent( CNAME( OnItemEnhanced ) );

			m_spawningItems.RemoveAt( i );
			--i;
		}
	}
}

void CGuiScenePlayer::TickWorld( Float timeDelta )
{
	if ( m_world )
	{
		CWorldTickInfo tickInfo( m_world.Get(), timeDelta );
		tickInfo.m_updatePhysics	= m_sceneInfo.m_updatePhysics ; // && !m_sceneInfo.m_uniquePhysicsWorld;
		
		tickInfo.m_updateWater		= false;
		tickInfo.m_updateFoliage	= false;
		tickInfo.m_updateTerrain	= false;
		tickInfo.m_updateGame		= false;
		tickInfo.m_updateTriggers	= false;
		tickInfo.m_updateCameraDirector	= false;
		tickInfo.m_updateEffects	= true;
		tickInfo.m_updatePathLib	= false;
		tickInfo.m_updateScripts	= false;

		tickInfo.m_fromTickThread	= false;

		/*
		SPosponedWorldTickInfo* info = new SPosponedWorldTickInfo();
		info->m_world = m_world;
		info->m_tickInfo = tickInfo;
		info->m_info = &m_cachedFrameInfo;

		GGame->PushBackWorldTick( info );
		*/

		m_world->Tick( tickInfo , &m_cachedFrameInfo );
	}
}

Bool CGuiScenePlayer::RegisterListener( IGuiScenePlayerListener* listener )
{
	RED_FATAL_ASSERT( listener, "No listener" );
	if ( ! listener )
	{
		return false;
	}
	return m_listeners.PushBackUnique( listener );
}

Bool CGuiScenePlayer::UnregisterListener( IGuiScenePlayerListener* listener )
{
	RED_FATAL_ASSERT( listener, "No listener" );
	if ( ! listener )
	{
		return false;
	}
	return m_listeners.Remove( listener );
}

void CGuiScenePlayer::UpdateWorldEnvironment( CWorld* world, CEnvironmentDefinition* envDef, const EulerAngles& sunRotation )
{
	if ( envDef )
	{
		CAreaEnvironmentParams aep = envDef->GetAreaEnvironmentParams();
		if ( aep.m_finalColorBalance.m_activatedBalanceMap )
		{
			if ( aep.m_finalColorBalance.m_balanceMap0 != nullptr ) aep.m_finalColorBalance.m_balanceMap0.Get();
			if ( aep.m_finalColorBalance.m_balanceMap1 != nullptr ) aep.m_finalColorBalance.m_balanceMap1.Get();
		}

		SWorldEnvironmentParameters wep = world->GetEnvironmentParameters();
		wep.m_environmentDefinition = envDef;
		wep.m_environmentDefinition->SetAreaEnvironmentParams( aep );
		world->SetEnvironmentParameters( wep );
	}

	if ( CEnvironmentManager *envManager = world->GetEnvironmentManager() )
	{		
		CGameEnvironmentParams params = envManager->GetGameEnvironmentParams();
		params.m_dayCycleOverride.m_enableCustomSunRotation = true;
		params.m_dayCycleOverride.m_customSunRotation = sunRotation;

		params.m_displaySettings.m_allowGlobalFog = false;
		params.m_displaySettings.m_allowBloom = true;
		params.m_displaySettings.m_disableTonemapping = false;

		params.m_displaySettings.m_gamma = 1.8f;
		params.m_displaySettings.m_enableInstantAdaptation = true;
		params.m_displaySettings.m_allowDOF = false;
		params.m_displaySettings.m_allowCloudsShadow = false;
		params.m_displaySettings.m_allowVignette = false;
		params.m_displaySettings.m_forceCutsceneDofMode = false;
		params.m_displaySettings.m_displayMode = EMM_None;

		envManager->SetGameEnvironmentParams( params );
	}
}
