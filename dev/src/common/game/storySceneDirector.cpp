/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "storySceneIncludes.h"
#include "../engine/behaviorGraphInstance.h"
#include "../engine/behaviorGraphStack.h"
#include "../engine/cameraDirector.h"
#include "../../common/core/gatheredResource.h"
#include "movableRepresentationPathAgent.h"

#include "storySceneDirector.h"
#include "storyScenePlayer.h"
#include "storySceneInput.h"
#include "storySceneActions.h"
#include "storySceneAnimationParams.h"
#include "sceneLog.h"
#include "storySceneSystem.h"
#include "communitySystem.h"
#include "storySceneUtils.h"
#include "storySceneItems.h"
#include "storySceneCutsceneSection.h"
#include "storySceneNPCTeleport.h"
#include "../engine/camera.h"
#include "../engine/cameraComponent.h"
#include "../engine/mimicComponent.h"
#include "../engine/particleSystem.h"
#include "../engine/dynamicLayer.h"
#include "../engine/particleComponent.h"
#include "../engine/spotLightComponent.h"
#include "../engine/pointLightComponent.h"
#include "gameWorld.h"
#include "actorsManager.h"
#include "../engine/cutsceneInstance.h"
#include "../engine/dimmerComponent.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

RED_DEFINE_STATIC_NAME( customCameraEnable );
RED_DEFINE_STATIC_NAME( cameraNoiseWeight );
RED_DEFINE_STATIC_NAME( idleDialogWeight );

//#define DONT_USE_CAMERA_ADJUST

CStorySceneDirector::CStorySceneDirector()
	: m_parentPlayer( NULL )
	, m_isEnabled( false )
	, m_allowCameraReactivaton( true )
	, m_isCustomCamera( false )
	, m_sceneNearPlane( NP_Further40cm )
	, m_sceneFarPlane( FP_DefaultEnv )
	, m_prevNearPlane( NP_DefaultEnv )
	, m_prevFarPlane( FP_DefaultEnv )
	, m_areActorPositionsValid( true )
	, m_sceneCamerasActivated( false )
	, m_desiredDialogsetInstance( NULL )
	, m_desiredSettingTimeElapsed( 0.0f )
	, m_currentDialogsetInstance( NULL )
{
	
}

CStorySceneDirector::~CStorySceneDirector()
{
	CancelTeleportNPCsJob();
}

void CStorySceneDirector::Initialize( CStoryScenePlayer* parentPlayer, const CStorySceneInput* sceneInput )
{
	m_parentPlayer = parentPlayer;

	m_placementHelper.Init( parentPlayer ); 

	m_placementHelper.SetPivotActors( &(parentPlayer->GetSceneActors()) );
	m_placementHelper.SetInitialPlacementFallbackNode( m_parentPlayer->m_player );

	m_sceneNearPlane = sceneInput->GetNearPlane();
	m_sceneFarPlane = sceneInput->GetFarPlane();

	if( m_parentPlayer )
	{
		CCamera* sceneCamera = m_parentPlayer->GetSceneCamera();
		if( sceneCamera )
		{
			CCameraComponent* cameraComponent = sceneCamera->GetSelectedCameraComponent();
			if( cameraComponent )
			{
				ApplyNearAndFarPlanes( cameraComponent );
			}
		}
	}

	SCENE_ASSERT( !m_desiredDialogsetInstance );
	m_currentDialogsetInstance = sceneInput->GetScene()->GetDialogsetByName( sceneInput->GetDialogsetName() );
}

void CStorySceneDirector::Reinitialize( const CStorySceneInput* sceneInput )
{
	SCENE_ASSERT__FIXME_LATER( !m_parentPlayer->IsSceneInGame() );
	m_currentDialogsetInstance = sceneInput->GetScene()->GetDialogsetByName( sceneInput->GetDialogsetName() );
	m_desiredDialogsetInstance = nullptr;
}

void CStorySceneDirector::KeepCurrentSetting()
{
	SCENE_ASSERT( !m_desiredDialogsetInstance );
	m_desiredDialogsetInstance = m_currentDialogsetInstance;
}

void CStorySceneDirector::ChangeDesiredSetting( const CStorySceneDialogsetInstance* desiredSetting )
{
	SCENE_ASSERT( !m_desiredDialogsetInstance );
	m_desiredDialogsetInstance = desiredSetting;
}

void CStorySceneDirector::FinishChangingSettings()
{
	//SCENE_ASSERT( m_desiredDialogsetInstance ); // TODO - special case fo null - see m_player->Stop()
	m_currentDialogsetInstance = m_desiredDialogsetInstance;
	m_desiredDialogsetInstance = nullptr;
	EnsureExistenceOfSettingActors( m_currentDialogsetInstance, true );
}

const CStorySceneDialogsetInstance* CStorySceneDirector::GetCurrentSettings() const
{
	SCENE_ASSERT( !m_desiredDialogsetInstance );
	SCENE_ASSERT__FIXME_LATER( !m_parentPlayer->IsSceneInGame() ||  m_currentDialogsetInstance );
	return m_currentDialogsetInstance;
}

const CStorySceneDialogsetInstance* CStorySceneDirector::GetCurrentSettingsIfValid() const
{
	SCENE_ASSERT__FIXME_LATER( !m_parentPlayer->IsSceneInGame() );
	return m_desiredDialogsetInstance == nullptr ? m_currentDialogsetInstance : nullptr;
}

const CStorySceneDialogsetInstance* CStorySceneDirector::GetDesiredSettings() const
{
	SCENE_ASSERT( m_desiredDialogsetInstance );
	return m_desiredDialogsetInstance;
}

CCamera* CStorySceneDirector::GetSceneCamera( const CName& cameraName )
{
	return ( m_parentPlayer != NULL ) ? m_parentPlayer->GetSceneCamera() : NULL;
}

CCamera* CStorySceneDirector::GetSceneCamera( Uint32 cameraNumber )
{
	return ( m_parentPlayer != NULL ) ? m_parentPlayer->GetSceneCamera() : NULL;
}

void CStorySceneDirector::EnableDirector()
{
	if ( IsEnabled() == true )
	{
		return;
	}
	m_isEnabled = true;

	ActivateCameras();
	return;
}

void CStorySceneDirector::DisableDirector()
{
	for ( TDynArray< CName >::iterator hiddenIter = m_hiddenActors.Begin();
		hiddenIter != m_hiddenActors.End(); ++hiddenIter )
	{
		CActor* actor = m_parentPlayer->GetMappedActor( *hiddenIter );
		if ( actor != NULL )
		{
			actor->SetHideInGame( false, false, CEntity::HR_Scene );
		}
	}
	m_hiddenActors.Clear();

	DeactivateCameras();	

	// Destroy all safe position entities
	m_placementHelper.CleanupPlacements();

	m_isEnabled = false;
#ifndef RED_FINAL_BUILD
	GCommonGame->GetSystem< CStorySceneSystem >()->SetErrorState( String::EMPTY );
#endif
}

void CStorySceneDirector::SetCameraData( const CCamera* camera )
{
	CCamera* sceneCamera = m_parentPlayer->GetSceneCamera();
	if ( !sceneCamera )
	{
		return;
	}

	// Get data from given camera

	ICamera::Data data;
	camera->GetData( data );

	// Set data to current scene camera

	sceneCamera->SetData( data );

	// Activate the camera

	sceneCamera->ForceUpdateTransformNodeAndCommitChanges();
	sceneCamera->SetActive();
}

void CStorySceneDirector::ActivateCameras()
{
	if ( m_sceneCamerasActivated == true )
	{
		return;
	}

	CCamera* sceneCamera = m_parentPlayer->GetSceneCamera();
	if ( sceneCamera == NULL )
	{
		SCENE_ERROR( TXT( "Scene fatal error - cannot get camera!!!" ) );
		return;
	}


	// Apply Story Scene behavior
	CAnimatedComponent* ac = sceneCamera->GetRootAnimatedComponent();
	if ( !ac || !ac->GetBehaviorStack() )
	{
		return;
	}

	if ( !ac->GetBehaviorStack()->HasInstance( CNAME( StoryScene ) ) )
	{
		Bool ret = ac->GetBehaviorStack()->ActivateBehaviorInstances( CNAME( StoryScene ) );
		if ( !ret )
		{
			SCENE_ERROR( TXT("Couldn't activate behavior 'StoryScene' for scene camera") );
		}
	}


	// E3 Demo Hack: Ensure that when scene camera is activated it will be more or less where the gameplay camera was. Prevents unnecessary streaming on camera change
	if ( GCommonGame->IsActive() )
	{
		const ICamera::Data& prevCameraData = GGame->GetActiveWorld()->GetCameraDirector()->GetCameraData();

		sceneCamera->SetData( prevCameraData );
		sceneCamera->ForceUpdateTransformNodeAndCommitChanges();
	}

	ApplyNearAndFarPlanes( sceneCamera->GetSelectedCameraComponent() );

	sceneCamera->SetActive();
	if ( sceneCamera->IsFrozen() )
	{
		sceneCamera->Unfreeze();
	}

	m_sceneCamerasActivated = true;
}

// One time input listener - switches (from scene's abandoned) to game camera on any input
class CStopAbandonCameraActivator : public IInputListener
{
private:
	Float m_blendTime;
	Bool m_useFocusTarget;

public:
	CStopAbandonCameraActivator( Float blendTime, Bool useFocusTarget )
		: m_blendTime( blendTime )
		, m_useFocusTarget( useFocusTarget )
	{}

	virtual Bool OnGameInputEvent( const SInputAction & action ) override
	{
		GGame->ActivateGameCamera( m_blendTime, m_useFocusTarget, false );
		delete this;
		return true;
	}
};

void CStorySceneDirector::SnapPlayerEntityToTeleportedTransform()
{
	// HACK: If teleport is in progress (e.g. at the end of the cutscene), make sure the player has the final position
	// This is done so that, after the dialog, the camera can immediately figure out its optimal blend-to-gameplay state
	if( GGame == nullptr ) { return; }

	CEntity* p = GGame->GetPlayerEntity();
	if ( p )
	{
		// Sanity cheks. May fail when game is shuting down
		if( p->GetRootAnimatedComponent() == nullptr ){ return; }
		if( p->GetLayer() == nullptr ) { return; }

		const Matrix desiredLocalToWorld = p->GetRootAnimatedComponent()->GetThisFrameTempLocalToWorld();

		const EngineTransform desiredLocalToWorldTransform( desiredLocalToWorld );
		EulerAngles desiredRotation = desiredLocalToWorldTransform.GetRotation();
		desiredRotation.Pitch = 0.f;
		desiredRotation.Roll = 0.f;
		p->SetRawPlacement( &desiredLocalToWorldTransform.GetPosition(), &desiredRotation, nullptr );
		p->HACK_UpdateLocalToWorld();
		
		CWorld* w = p->GetLayer()->GetWorld();
		
		p->ClearFlag( NF_ScheduledUpdateTransform );
		p->ClearFlag( NF_MarkUpdateTransform );

		if( w != nullptr )
		{
			w->GetUpdateTransformManager().UnscheduleEntity( p );
		}
		
	}
}

void CStorySceneDirector::DeactivateCameras()
{
	if ( m_sceneCamerasActivated == false )
	{
		return;
	}

	CCamera* sceneCamera = m_parentPlayer->GetSceneCamera();

	// Camera can be NULL at the end of game
	if ( sceneCamera == NULL )
	{
		return;
	}

	RestoreNearAndFarPlanes( sceneCamera->GetSelectedCameraComponent() );

	// Switch to gameplay camera

	if ( !m_parentPlayer->GetSwitchedToGameplayCamera() && GGame && GGame->IsActive() )
	{
		const Bool useFocusTarget = m_parentPlayer->GetGameplayCameraUseFocusTarget();
		const Bool didSkipScene = m_parentPlayer->DidJustSkipElement();
		const Bool resetCamera = m_parentPlayer->GetResetGameplayCameraOnOutput();
		const Float blendTime = didSkipScene ? 0.0f : m_parentPlayer->GetGameplayCameraBlendTime();

		// Switch to abandoned camera?

		if ( m_parentPlayer->IsGameplayCameraAbandoned() )
		{
			CWorld* world = GGame->GetActiveWorld();
			world->GetCameraDirector()->AbandonTopmostCamera();
			
			GGame->GetInputManager()->RegisterListener( new CStopAbandonCameraActivator( blendTime, useFocusTarget ), CName::NONE );
		}
		else
		{
			SnapPlayerEntityToTeleportedTransform();

			// Reposition camera immediately into final position

			GGame->ResetGameplayCamera();

			// Blend to gameplay camera

			GGame->ActivateGameCamera( blendTime, useFocusTarget, true, resetCamera );

			m_parentPlayer->SetSwitchedToGameplayCamera( true );
			if ( m_parentPlayer->IsSceneInGame() )
			{
				GCommonGame->GetActiveWorld()->GetCameraDirector()->SetCameraResetDisabled( true );
			}
		}
	}

	// Apply Story Scene behavior
	CAnimatedComponent* ac = sceneCamera->GetRootAnimatedComponent();
	if ( ac && ac->GetBehaviorStack() )
	{
		ac->GetBehaviorStack()->StopSlotAnimation( CNAME( CAMERA_SHOT_SLOT ) );
		ac->GetBehaviorStack()->DeactivateConstraint( CNAME( customCameraEnable ) );
	}

	//sceneCamera->Freeze();

	m_sceneCamerasActivated = false;
}

void CStorySceneDirector::ApplyNearAndFarPlanes( CCameraComponent* cameraComponent )
{
	if ( cameraComponent == NULL )
	{
		return;
	}

	m_prevNearPlane = cameraComponent->GetNearPlane();
	m_prevFarPlane = cameraComponent->GetFarPlane();

	cameraComponent->SetNearPlane( m_sceneNearPlane );
	cameraComponent->SetFarPlane( m_sceneFarPlane );
}

void CStorySceneDirector::RestoreNearAndFarPlanes( CCameraComponent* cameraComponent )
{
	if ( cameraComponent == NULL )
	{
		return;
	}

	cameraComponent->SetNearPlane( m_prevNearPlane );
	cameraComponent->SetFarPlane( m_prevFarPlane );
}

Matrix CStorySceneDirector::GetActorTrajectory( const CName& actor ) const
{
	SCENE_ASSERT( m_currentDialogsetInstance );

	Matrix mat( Matrix::IDENTITY );

	if ( m_currentDialogsetInstance )
	{
		const TDynArray< CStorySceneDialogsetSlot* >& dialogsetSlots = m_currentDialogsetInstance->GetSlots();
		for ( TDynArray< CStorySceneDialogsetSlot* >::const_iterator slotIter = dialogsetSlots.Begin(); slotIter != dialogsetSlots.End(); ++slotIter )
		{
			const CStorySceneDialogsetSlot* slot = *slotIter;
			if ( slot && actor == slot->GetActorName() )
			{
				EngineTransform actorTransformInSetting;
				m_placementHelper.GetSlotPlacement( slot, m_currentDialogsetInstance, actorTransformInSetting );

				actorTransformInSetting.CalcLocalToWorld( mat );

				break;
			}
		}
	}

	return mat;
}

void CStorySceneDirector::InformNPCsThatSceneIsFinished()
{
	for ( Uint32 i = 0; i < m_nearbyInterestedNPCs.Size(); ++i )
	{
		CNewNPC *npc = m_nearbyInterestedNPCs[i].Get();
		if ( npc )
		{
			npc->CallEvent( CNAME( OnNearbySceneEnded ) );
		}
	}

	m_nearbyInterestedNPCs.Clear();
}

Bool CStorySceneDirector::IsContinousTransitionPossible( const CName& newSettingName ) const
{
	if ( newSettingName == CName::NONE )
	{
		return true;
	}
	const CStorySceneDialogsetInstance* dialogset = m_parentPlayer->GetStoryScene()->GetDialogsetByName( newSettingName );
	if( !dialogset )
	{
		dialogset = m_parentPlayer->GetCurrentStoryScene()->GetDialogsetByName( newSettingName );
	}

	return IsContinousTransitionPossible( dialogset, m_currentDialogsetInstance );
}

Bool CStorySceneDirector::IsContinousTransitionPossible( const CStorySceneDialogsetInstance* newDialogset, const CStorySceneDialogsetInstance* oldDialogset ) const
{
	if ( newDialogset == NULL || oldDialogset == NULL )
	{
		return true;
	}

	const TDynArray< CStorySceneDialogsetSlot* > newSlots = newDialogset->GetSlots();
	const TDynArray< CStorySceneDialogsetSlot* > oldSlots = oldDialogset->GetSlots();

	if ( newSlots.Size() != oldSlots.Size() )
	{
		return false;
	}

	for ( Uint32 i = 0; i < newSlots.Size(); ++i )
	{
		if ( newSlots[ i ] == NULL || oldSlots[ i ] == NULL )
		{
			continue;
		}
		if ( newSlots[ i ]->GetActorName() != oldSlots[ i ]->GetActorName() 
			|| newSlots[ i ]->GetSlotNumber() != oldSlots[ i ]->GetSlotNumber() )
		{
			return false;
		}
	}
	return true;
}

CEntity* CStorySceneDirector::SpawnPropOnDemand( const CName& id  )
{
	const CStorySceneDialogsetInstance* dialogset = m_currentDialogsetInstance;
	EntitySpawnInfo spawnInfo;

	const CStorySceneProp* prop = m_parentPlayer->GetStoryScene()->GetPropDefinition( id );
	if( !prop )
	{
		prop = m_parentPlayer->GetCurrentStoryScene()->GetPropDefinition( id );
	}
	if( prop )
	{
		spawnInfo.m_template = prop->m_entityTemplate.Get();
	}
	if ( !spawnInfo.m_template )
	{
		ERR_GAME( TXT( "Could not find template and appearance to spawn actor %s" ), id.AsString().AsChar() );
		return NULL;
	}
	spawnInfo.m_spawnPosition = GetCurrentScenePlacement().GetPosition();

	CEntity* spawnedProp = Cast< CEntity >( m_parentPlayer->m_world->GetDynamicLayer()->CreateEntitySync( spawnInfo ) );
	spawnedProp->SetHideInGame( true );
	m_parentPlayer->OnPropSpawned( spawnInfo, spawnedProp );

	SCENE_ASSERT( spawnedProp != NULL );

	return spawnedProp;
}


CEntity* CStorySceneDirector::SpawnEffectOnDemand( const CName& id )
{
	const CStorySceneDialogsetInstance* dialogset = m_currentDialogsetInstance;	
	if ( CStorySceneEffect* fxDef = m_parentPlayer->GetStoryScene()->GetSceneEffectDefinition( id ) )
	{		
		if ( CParticleSystem* ps = fxDef->m_particleSystem.Get() )
		{	
			EntitySpawnInfo spawnInfo;
			spawnInfo.m_name = id.AsString();
			spawnInfo.m_spawnPosition = GetCurrentScenePlacement().GetPosition();
			CEntity* entity = m_parentPlayer->m_world->GetDynamicLayer()->CreateEntitySync( spawnInfo );

			CParticleComponent* pc = Cast< CParticleComponent >( entity->CreateComponent( ClassID< CParticleComponent >(), SComponentSpawnInfo() ) );
			pc->ForceUpdateTransformNodeAndCommitChanges();
			pc->SetParticleSystem( ps );
			pc->RefreshRenderProxies();
			spawnInfo.m_spawnPosition = GetCurrentScenePlacement().GetPosition();
			m_parentPlayer->OnEffectSpawned( spawnInfo, entity );

			SCENE_ASSERT( entity != NULL );
			return entity;
		}
	}

	return nullptr;
}

CEntity* CStorySceneDirector::SpawnLightOnDemand( const CName& id )
{
	const CStorySceneDialogsetInstance* dialogset = m_currentDialogsetInstance;	
	const CStorySceneLight* lightDef = m_parentPlayer->GetStoryScene()->GetSceneLightDefinition( id );
	if ( !lightDef )
	{
		lightDef = m_parentPlayer->GetCurrentStoryScene()->GetSceneLightDefinition( id );
	}

	if( lightDef )
	{
		EntitySpawnInfo spawnInfo;
		spawnInfo.m_name = id.AsString();
		spawnInfo.m_spawnPosition = GetCurrentScenePlacement().GetPosition();
		CEntity* entity = m_parentPlayer->m_world->GetDynamicLayer()->CreateEntitySync( spawnInfo );

		CLightComponent*	light	= nullptr;
		CDimmerComponent*	dimmer	= nullptr;
		switch( lightDef->m_type )
		{
		case LT_SpotLight:
			light = Cast< CLightComponent >( entity->CreateComponent( ClassID< CSpotLightComponent >(), SComponentSpawnInfo() ) );
			break;

		case LT_PointLight:
			light = Cast< CLightComponent >( entity->CreateComponent( ClassID< CPointLightComponent >(), SComponentSpawnInfo() ) );
			break;

		case LT_Dimmer:
			dimmer = Cast< CDimmerComponent >( entity->CreateComponent( ClassID< CDimmerComponent >(), SComponentSpawnInfo() ) );
			break;
		}

		if( light )
		{
			light->SetColor(Color(255, 0, 0));
			light->SetBrightness(2.0f);
			light->SetRadius(5.0f);
			light->SetShadowCastingMode( lightDef->m_shadowCastingMode );
			light->SetShadowFadeDistance( lightDef->m_shadowFadeDistance );
			light->SetShadowFadeRange( lightDef->m_shadowFadeRange );
			light->ForceUpdateTransformNodeAndCommitChanges();
			light->RefreshRenderProxies();
		}
		else if( dimmer )
		{
			dimmer->SetEnabled( false );			
			dimmer->SetDimmerType( lightDef->m_dimmerType );
			dimmer->SetAreaMarker( lightDef->m_dimmerAreaMarker );
			dimmer->RefreshRenderProxies();
		}

		m_parentPlayer->OnLightSpawned( spawnInfo, entity );

		SCENE_ASSERT( entity != NULL );
		return entity;
	}

	return nullptr;
}

CActor* CStorySceneDirector::SearchForActorInCommunity( const CName& voicetag )
{
	const CStorySceneDialogsetInstance* dialogset = m_currentDialogsetInstance;
	EngineTransform	actorPlacement;
	if ( dialogset != NULL )
	{
		CStorySceneDialogsetSlot* actorSlot = dialogset->GetSlotByActorName( voicetag );
		m_placementHelper.GetSlotPlacement( actorSlot, dialogset, actorPlacement );
	}
	const CStorySceneActor* actor = m_parentPlayer->GetStoryScene()->GetSceneActorDefinition( voicetag );
	if( !actor )
	{
		actor = m_parentPlayer->GetCurrentStoryScene()->GetSceneActorDefinition( voicetag );
	}	
	if ( GGame && GGame->GetActiveWorld() == m_parentPlayer->m_world && actor && GCommonGame->GetSystem< CCommunitySystem >() != NULL )
	{
		CNewNPC* communityNpc = NULL;
		const TSoftHandle<CEntityTemplate> softHandle = actor->m_entityTemplate;
		CName appearance = actor->m_appearanceFilter.Size() > 0 ? actor->m_appearanceFilter[0] : CName::NONE;
		if ( GCommonGame->GetSystem< CCommunitySystem >()->GetSpawnedNPC( softHandle, appearance, actorPlacement.GetPosition(),true, communityNpc ) && communityNpc != NULL && communityNpc->IsInNonGameplayScene() == false )
		{
			return communityNpc;
		}
	}

	return nullptr;
}

CGatheredResource debugActorCharacter( TXT("gameplay\\resources\\scene_dummy.w2ent"), RGF_Startup );

CActor* CStorySceneDirector::SpawnActorOnDemand( const CName& voicetag )
{
	const CStorySceneDialogsetInstance* dialogset = m_currentDialogsetInstance;
	EngineTransform	actorPlacement;
	if ( dialogset != NULL )
	{
		CStorySceneDialogsetSlot* actorSlot = dialogset->GetSlotByActorName( voicetag );
		m_placementHelper.GetSlotPlacement( actorSlot, dialogset, actorPlacement );
	}
	else if ( m_parentPlayer->m_player )
	{
		actorPlacement = m_parentPlayer->m_player->GetTransform();
	}

	EntitySpawnInfo spawnInfo;
	spawnInfo.m_spawnPosition = actorPlacement.GetPosition();
	spawnInfo.m_spawnRotation = actorPlacement.GetRotation();

	CName actorAppearanceName;
	Bool canSearchInCommunity;

	if ( !m_parentPlayer->GetStoryScene()->GetActorSpawnDefinition( voicetag, spawnInfo.m_template, actorAppearanceName, canSearchInCommunity ) )
	{
		m_parentPlayer->GetCurrentStoryScene()->GetActorSpawnDefinition( voicetag, spawnInfo.m_template, actorAppearanceName, canSearchInCommunity );	
	}

	if( m_parentPlayer->IsGameplay() )
	{
		spawnInfo.m_template = debugActorCharacter.LoadAndGet< CEntityTemplate >();
	}
	if ( !spawnInfo.m_template )
	{
		ERR_GAME( TXT( "Could not find template and appearance to spawn actor %s" ), voicetag.AsString().AsChar() );
		return NULL;
	}
	else if( spawnInfo.m_template->GetEntityObject() && spawnInfo.m_template->GetEntityObject()->IsA< CActor >() )
	{
		spawnInfo.m_entityClass = CActor::GetStaticClass();
	}	
	spawnInfo.m_appearances.PushBack( actorAppearanceName );

	CActor* spawnedSettingActor = NULL;
	spawnedSettingActor = Cast< CActor >( m_parentPlayer->m_world->GetDynamicLayer()->CreateEntitySync( spawnInfo ) );

	if ( spawnedSettingActor && spawnedSettingActor->IsPlayer() && GGame && GGame->GetActiveWorld() && GGame->IsActive() )
	{
		SCENE_ASSERT( 0, TXT("Scene spawned new player. Do you want to have it?") );
		SCENE_WARN( TXT("Scene spawned new player. Do you want to have it?") );
	}

	m_parentPlayer->OnActorSpawned( spawnInfo, spawnedSettingActor );
	return spawnedSettingActor;
}

void CStorySceneDirector::EnsureExistenceOfSettingActors( const CStorySceneDialogsetInstance* dialogsetInstance, Bool makeVisible )
{
	if ( dialogsetInstance == NULL )
	{
		return;
	}

	const TDynArray< CStorySceneDialogsetSlot* > dialogsetSlots = dialogsetInstance->GetSlots();
	for ( Uint32 i = 0; i < dialogsetSlots.Size(); ++i )
	{
		if ( dialogsetSlots[ i ] == NULL )
		{
			continue;
		}

		CName settingActorVoicetag = dialogsetSlots[ i ]->GetActorName();

		CEntity* actor = m_parentPlayer->GetSceneActorEntity( settingActorVoicetag );
		if ( actor != NULL || settingActorVoicetag == CName::NONE )
		{
			// Don;t do anything if actor exists
			continue;
		}

		actor = m_parentPlayer->SpawnSceneActorEntity( settingActorVoicetag );
		if ( actor )
		{
			actor->SetHideInGame( makeVisible == false, false, CEntity::HR_Scene );

			//if ( !makeVisible )
			//{
			//	SCENE_LOG( TXT("Scene hides actor [%s]"), actor->GetName().AsChar() );
			//}
		}
	}
}

void CStorySceneDirector::EnsureExistenceOfProps()
{
	const CStoryScene* scene = m_parentPlayer->GetCurrentStoryScene();
	const TDynArray< CStorySceneProp* >& props = scene->GetScenePropDefinitions();
	for( const CStorySceneProp* prop : props )
	{
		m_parentPlayer->SpawnScenePropEntity( prop->m_id );
	}
}

CCamera* CStorySceneDirector::GetActiveCamera()
{
	return m_parentPlayer->GetSceneCamera();
}

void CStorySceneDirector::SetActorVisibleInScene( const CName& voicetag, Bool show )
{
	CActor* actor = m_parentPlayer->GetMappedActor( voicetag );
	if ( actor == NULL )
	{
		return;
	}

	actor->SetHideInGame( show == false, true, CEntity::HR_Scene );

	if ( show == false )
	{
		m_hiddenActors.PushBackUnique( voicetag );
	}
	else
	{
		m_hiddenActors.Remove( voicetag );
	}
}



Bool CStorySceneDirector::IsCameraMovementAllowed() const
{
	const CStorySceneSection* currentSection = m_parentPlayer->GetCurrentSection();
	if ( currentSection != NULL )
	{
		Bool ret = currentSection->CanMoveCamera();

		if ( !GGame->IsUsingPad() )
		{
			ret &= m_parentPlayer->HasPendingChoice() == false;
		}

		return ret;
	}
	return false;
}

void CStorySceneDirector::SynchronizeCameraWithCS( const CCutsceneInstance::CameraState& state )
{
	m_parentPlayer->m_internalState.m_cameraState.m_position = state.m_position;
	m_parentPlayer->m_internalState.m_cameraState.m_rotation = state.m_rotation;
	m_parentPlayer->m_internalState.m_cameraState.m_fov = state.m_fov;
	m_parentPlayer->m_internalState.m_cameraState.m_dof = state.m_dof;
}

void CStorySceneDirector::ActivateCamera( const StorySceneCameraDefinition& cameraDefinition )
{
	CCamera* camera = m_parentPlayer->GetSceneCamera();
	if ( camera == NULL )
	{
		SCENE_ASSERT( camera );
		return;
	}

	Matrix cameraAdjustedWS;
	m_isCameraAdjusted = AdjustCameraForActorHeight( cameraDefinition, nullptr, &cameraAdjustedWS );

	Matrix scenePlacementWS, cameraTransformWS;
	GetCurrentScenePlacement().CalcLocalToWorld( scenePlacementWS );
	cameraDefinition.m_cameraTransform.CalcLocalToWorld( cameraTransformWS );
	cameraTransformWS = cameraTransformWS * scenePlacementWS;

	camera->SetPosition( cameraAdjustedWS.GetTranslationRef() );
	camera->SetRotation( cameraAdjustedWS.ToEulerAngles() );
	camera->SetFov( cameraDefinition.m_cameraFov );

	SDofParams cameraDof;
	cameraDof.overrideFactor = 1.0f;
	if ( cameraDefinition.m_dof.m_enabled == true )
	{
		cameraDefinition.m_dof.ToEngineDofParams( cameraDof );
	}
	else
	{
		cameraDof.dofIntensity = cameraDefinition.m_dofIntensity;
		cameraDof.dofBlurDistFar = cameraDefinition.m_dofBlurDistFar;
		cameraDof.dofBlurDistNear = cameraDefinition.m_dofBlurDistNear;
		cameraDof.dofFocusDistFar = cameraDefinition.m_dofFocusDistFar;
		cameraDof.dofFocusDistNear = cameraDefinition.m_dofFocusDistNear;
	}
	
	camera->SetDofParams( cameraDof );
 	camera->SetBokehDofParams( cameraDefinition.m_bokehDofParams );
	m_parentPlayer->m_internalState.m_cameraState.m_fov = cameraDefinition.m_cameraFov;
	m_parentPlayer->m_internalState.m_cameraState.m_position = cameraAdjustedWS.GetTranslationRef();
	m_parentPlayer->m_internalState.m_cameraState.m_rotation = cameraAdjustedWS.ToEulerAngles();
	m_parentPlayer->m_internalState.m_cameraState.m_rawPosition = cameraTransformWS.GetTranslationRef();
	m_parentPlayer->m_internalState.m_cameraState.m_rawRotation = cameraTransformWS.ToEulerAngles();
	m_parentPlayer->m_internalState.m_cameraState.m_localToWorld = m_parentPlayer->m_internalState.m_cameraState.m_rotation.ToMatrix();
	m_parentPlayer->m_internalState.m_cameraState.m_localToWorld.SetTranslation( m_parentPlayer->m_internalState.m_cameraState.m_position );
	m_parentPlayer->m_internalState.m_cameraState.m_cameraUniqueID = cameraDefinition.m_cameraUniqueID;
	m_parentPlayer->m_internalState.m_cameraState.m_dof = cameraDof;
	m_parentPlayer->m_internalState.m_cameraState.m_valid = true;
}

Matrix CStorySceneDirector::BuildSpecialMatrix( const Vector& srcVect, const Vector& dstVect )  const
{
	Matrix mat;
	Vector dirVect = dstVect - srcVect;
	// EX from direction, EY pointing up
	mat.V[0] = dirVect.Normalized3();
	mat.V[1] = Vector::EZ;

	// EZ as cross product of EX and EY
	mat.V[2] = Vector::Cross( mat.V[0], mat.V[1] ).Normalized3();

	mat.V[0].W = 0.0f;
	mat.V[1].W = 0.0f;
	mat.V[2].W = 0.0f;
	mat.V[3] = Vector::EW;
	mat.SetTranslation( srcVect );

	return mat;
}

Bool CStorySceneDirector::GetActorHeightData( CName dialogsetSlotName, Uint32 dialogsetSlotNr, Vector & currentPointLS, Matrix & actorL2W ) const
{
	if ( !m_currentDialogsetInstance )
	{
		return false;
	}
	CStorySceneDialogsetSlot* slot = m_currentDialogsetInstance->GetSlotByName( dialogsetSlotName );
	if( !slot )
	{
		slot = m_currentDialogsetInstance->GetSlotBySlotNumber( dialogsetSlotNr );
	}
	if ( !slot )
	{
		return false;
	}
	CActor* actor = Cast< CActor >( m_parentPlayer->GetSceneActorEntity( slot->GetActorName() ) );
	if ( !actor )
	{	
		return false;
	}

	CName animState;
	m_parentPlayer->GetActorIdleAnimation( slot->GetActorName(), animState );
	currentPointLS = StorySceneUtils::CalcEyesPosLS( actor, animState );

	actor->CalcLocalTransformMatrix( actorL2W );
	return true;			
}

namespace
{
#ifdef DONT_USE_CAMERA_ADJUST
	Bool staticAdjust = false;
#else
	Bool staticAdjust = true;
#endif
}

Bool CStorySceneDirector::AdjustCameraForActorHeight( const StorySceneCameraDefinition& def, EngineTransform* outAdjusted, Matrix* outCameraWS ) const
{
	Vector refTargetWS, refSourceWS, refTargetLS, refSourceLS, currTargetWS, currSourceWS, currSourceLS, currTargetLS;
	Matrix sourceL2W, targetL2W, scenePlacementWS, cameraTransformWS, cameraFinalWS;

	refTargetLS = def.m_targetEyesLS;
	refSourceLS = Vector::EZ*def.m_sourceEyesHeigth;

	Bool adjust = ::staticAdjust;
	adjust = adjust && def.m_cameraAdjustVersion > 2;
	adjust = adjust && ( refTargetLS.Sum3() != 0.f ) && ( refSourceLS.Sum3() !=  0.f );
	adjust = adjust && GetActorHeightData( def.m_targetSlotName, def.m_genParam.m_targetSlot, currTargetLS, targetL2W );
	adjust = adjust && GetActorHeightData( def.m_sourceSlotName, def.m_genParam.m_sourceSlot, currSourceLS, sourceL2W );

	currSourceWS = sourceL2W.TransformPoint( currSourceLS );														
	refSourceWS = sourceL2W.TransformPoint( refSourceLS );
	currTargetWS = targetL2W.TransformPoint( currTargetLS );														
	refTargetWS = targetL2W.TransformPoint( refTargetLS );

	adjust = adjust && (  Abs<Float>( currSourceWS.Z - refSourceWS.Z )  > 0.025 || currTargetWS.DistanceSquaredTo( refTargetWS ) > 0.0005 );
	adjust = adjust && !( Vector::Near3( currSourceWS, currTargetWS ) || Vector::Equal3( refSourceWS, refTargetWS ) );
	
	if ( outAdjusted )
	{
		*outAdjusted = def.m_cameraTransform;
	}

	GetCurrentScenePlacement().CalcLocalToWorld( scenePlacementWS );
	def.m_cameraTransform.CalcLocalToWorld( cameraTransformWS );
	cameraTransformWS = cameraTransformWS * scenePlacementWS;
	
	if ( !adjust )
	{
		cameraFinalWS	= cameraTransformWS;
	}
	else
	{
		//horizontal adjustment - only rotation
		Matrix rotMat( Matrix::IDENTITY );
		Vector vec1 =  cameraTransformWS.GetTranslation() - currTargetWS;
		Vector vec2 =  cameraTransformWS.GetTranslation() - refTargetWS;
		Float angleRad = -atan2f( vec1.X,vec1.Y ) + atan2f( vec2.X,vec2.Y );
		rotMat.SetRotZ33( angleRad );

		//vert adjustment rot and translation 
		Matrix matRef = BuildSpecialMatrix( sourceL2W.GetTranslation() + Vector::EZ * refSourceLS.Z, targetL2W.GetTranslation() + Vector::EZ * refTargetLS.Z );
		Matrix matRefInv = matRef.FullInverted();
		Matrix cameraBoneRS = cameraTransformWS * matRefInv;
		Matrix matCurr = BuildSpecialMatrix( sourceL2W.GetTranslation() + Vector::EZ * currSourceLS.Z, targetL2W.GetTranslation() + Vector::EZ * currTargetLS.Z );
		Matrix finalMat = rotMat * cameraBoneRS * matCurr;

		cameraFinalWS.BuildFromDirectionVector( finalMat.V[1] );
		cameraFinalWS.SetTranslation( finalMat.GetTranslation() );

		GetCurrentScenePlacement().CalcWorldToLocal( scenePlacementWS );	
		if ( outAdjusted )
		{
			*outAdjusted = EngineTransform( cameraFinalWS * scenePlacementWS );
		}
	}

	if ( outCameraWS )
	{
		*outCameraWS = cameraFinalWS;
	}
	return adjust;
}

void CStorySceneDirector::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_Scenes ) )
	{
		if ( m_isCameraAdjusted )
		{
			frame->AddDebugFrame(1 ,1 ,frame->GetFrameOverlayInfo().m_width -2 ,frame->GetFrameOverlayInfo().m_height -2 , Color::LIGHT_YELLOW );
		}		
	}	
}


void CStorySceneDirector::ActivateCustomCamera()
{
	if ( m_parentPlayer->GetSwitchedToGameplayCamera() )
	{
		return;
	}

	m_parentPlayer->OnCameraCut();
	m_parentPlayer->GetSceneCamera()->SetActive();

	m_isCustomCamera = true;
	m_activeCameraName = CName::NONE;
	m_activeCameraShot = CName::NONE;
}

const CStorySceneDialogsetInstance* CStorySceneDirector::GetPendingOrCurrentDialogsetInstance() const
{
	return m_desiredDialogsetInstance ? m_desiredDialogsetInstance : m_currentDialogsetInstance;
}

EngineTransform CStorySceneDirector::GetScenePlacement( const CStorySceneDialogsetInstance* dialogsetInstance ) const
{
	EngineTransform dialogsetPlacement;
	m_placementHelper.FindDialogsetPlacement( dialogsetInstance, dialogsetPlacement );
	return dialogsetPlacement;
}

EngineTransform CStorySceneDirector::GetCurrentScenePlacement() const
{
	SCENE_ASSERT__FIXME_LATER( m_currentDialogsetInstance );
	EngineTransform dialogsetPlacement;
	m_placementHelper.FindDialogsetPlacement( m_currentDialogsetInstance, dialogsetPlacement );
	return dialogsetPlacement;
}

EngineTransform CStorySceneDirector::Debug_GetCurrentScenePlacement() const
{
	if ( m_currentDialogsetInstance )
	{
		return GetCurrentScenePlacement();
	}
	else
	{
		return EngineTransform();
	}
}

void CStorySceneDirector::ResetPlacements()
{
	m_placementHelper.CleanupPlacements();
}

const CGUID& CStorySceneDirector::GetActorsSlotID( const CName& actor ) const
{
	SCENE_ASSERT( m_currentDialogsetInstance );
	if ( m_currentDialogsetInstance )
	{
		if ( CStorySceneDialogsetSlot* actorSlot = m_currentDialogsetInstance->GetSlotByActorName( actor ) )
		{
			return actorSlot->GetID();
		}
	}

	return CGUID::ZERO;
}

void CStorySceneDirector::SetCameraNoiseMovement( Bool enable )
{
	if ( m_parentPlayer != NULL && m_parentPlayer->IsInGameplay() == true )
	{
		return;
	}

	CCamera* camera = m_parentPlayer->GetSceneCamera();
	
	if ( camera == NULL || camera->GetRootAnimatedComponent() == NULL || camera->GetRootAnimatedComponent()->GetBehaviorStack() == NULL )
	{
		return;
	}

	CBehaviorGraphStack* stack = camera->GetRootAnimatedComponent()->GetBehaviorStack();
	stack->SetBehaviorVariable( CNAME( cameraNoiseWeight ), ( enable == true ) ? 1.0f : 0.0f );
	//stack->SetBehaviorVariable( TXT("sourceTarget"), Vector::ZERO_3D_POINT );
}

void CStorySceneDirector::GetSlotPlacement( const CStorySceneDialogsetSlot* slot, const CStorySceneDialogsetInstance* dialogsetInstance, EngineTransform& placement )
{
	if ( slot != NULL && dialogsetInstance != NULL )
	{
		m_placementHelper.GetSlotPlacement( slot, dialogsetInstance, placement );
	}
}

const CWorld* CStorySceneDirector::GetSceneWorld() const 
{ 
	return ( m_parentPlayer != NULL ) ? m_parentPlayer->GetSceneWorld() : NULL; 
}

void CStorySceneDirector::SetCameraAdjustedDebugFrame( Bool adjusted )
{
	m_isCameraAdjusted = adjusted;
}

void CStorySceneDirector::DestroyDeadNPCs()
{
	const CStorySceneDialogsetInstance* dialogsetInstance = GetPendingOrCurrentDialogsetInstance();
	if ( !dialogsetInstance )
	{
		return;
	}

	const EngineTransform currentScenePlacement = GetScenePlacement( dialogsetInstance );

	// Find all NPCs within some distance from dialogset

	const Float searchRadius = 10.0f;
	TDynArray< CNewNPC* > deadNPCsToDestroy;

	struct DeadNPCsToDestroyFunctor
	{
		enum { SORT_OUTPUT = 0 };

		DeadNPCsToDestroyFunctor( CStorySceneDirector* director, TDynArray< CNewNPC* >& deadNPCsToDestroy )
			: m_director( director )
			, m_deadNPCsToDestroy( deadNPCsToDestroy )
			, m_communitySystem( GCommonGame->GetSystem< CCommunitySystem >() )
			, m_mappedActors( director->GetDirectorParent()->GetSceneController()->GetMappedActors() )
		{}

		RED_INLINE Bool operator()( const CActorsManagerMemberData& ptr )
		{
			CNewNPC* npc = Cast< CNewNPC >( ptr.Get() );
			if ( npc &&
				!npc->IsAlive() &&
				!IsDialogNPC( npc ) )
			{
				m_deadNPCsToDestroy.PushBack( npc );
			}
			return true;
		}

	private:
		Bool IsDialogNPC( CNewNPC* npc ) const
		{
			for ( const CStorySceneController::SceneActorInfo& sceneActor : m_mappedActors )
			{
				if ( sceneActor.m_actor == npc )
				{
					return true;
				}
			}
			return false;
		}

		CStorySceneDirector* m_director;
		CCommunitySystem* m_communitySystem;
		const TDynArray< CStorySceneController::SceneActorInfo >& m_mappedActors;
		TDynArray< CNewNPC* >& m_deadNPCsToDestroy;

	} deadNPCsToDestroyFunctor( this, deadNPCsToDestroy );

	Box searchBox;
	searchBox.Min = searchBox.Max = currentScenePlacement.GetPosition();
	searchBox.Extrude( searchRadius );

	GCommonGame->GetActorsManager()->TQuery( deadNPCsToDestroyFunctor, searchBox, true, nullptr, 0 );

	// Destroy NPCs

	for ( CNewNPC* npc : deadNPCsToDestroy )
	{
		SCENE_LOG( TXT( "DestroyDeadNPCs(): Destroying dead NPC '%ls' %f meters around current dialogset." ), npc->GetFriendlyName().AsChar(), searchRadius );
		npc->Destroy();
	}
}

void CStorySceneDirector::ClearAreaForSection( const CStorySceneSection* section )
{
	if ( !m_parentPlayer->IsInGame() || m_parentPlayer->IsGameplay() )
	{
		return;
	}

	// Destroy dead NPCs around

	if ( m_parentPlayer->GetEnableDestroyDeadActorsAround() )
	{
		DestroyDeadNPCs();
	}

	// Teleport out or (temporarily) hide NPCs in dialog area

	if ( m_parentPlayer->GetBlockSceneArea() )
	{
		m_teleportNPCsJob = Red::TUniquePtr< CTeleportNPCsJob >( CStorySceneNPCTeleportHelper::BlockAreaForSection( this, section ) );
	}
}

Bool CStorySceneDirector::IsNPCTeleportJobInProgress() const
{
	return m_teleportNPCsJob && !m_teleportNPCsJob->IsAsyncPartDone();
}

void CStorySceneDirector::FinalizeTeleportNPCsJob()
{
	if ( m_teleportNPCsJob )
	{
		m_teleportNPCsJob->FinalizeSyncPart();
		m_teleportNPCsJob.Reset();
	}
}

void CStorySceneDirector::CancelTeleportNPCsJob()
{
	if ( m_teleportNPCsJob )
	{
		m_teleportNPCsJob->Cancel();
		m_teleportNPCsJob.Reset();
	}
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
