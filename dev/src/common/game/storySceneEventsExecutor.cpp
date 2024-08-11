
#include "build.h"
#include "storySceneEventsExecutor.h"

#include "../engine/behaviorGraphUtils.inl"
#include "../engine/morphedMeshComponent.h"
#include "../engine/appearanceComponent.h"
#include "../engine/clothComponent.h"
#include "../engine/animDangleComponent.h"
#include "../engine/behaviorGraphStack.h"
#include "../engine/behaviorGraphAnimationSlotNode.h"
#include "../engine/behaviorGraphAnimationManualSlot.h"
#include "../engine/camera.h"
#include "../engine/mimicComponent.h"
#include "../engine/engineTypeRegistry.h"

#include "storySceneEventsCollector.h"
#include "storySceneEventsCollector_events.h"
#include "storySceneDebugger.h"
#include "storySceneEventMimicFilter.h"
#include "storySceneEventMimicPose.h"
#include "storyScenePlayer.h"
#include "storySceneCutscene.h"
#include "storySceneLine.h"
#include "actorSpeech.h"
#include "lookAtTypes.h"
#include "storySceneLookAtController.h"
#include "storySceneUtils.h"
#include "storySceneVoicetagMapping.h"
#include "storySceneSystem.h"
#include "storySceneMotionExtractionSampler.h"
#include "storySceneCutsceneSection.h"
#include "itemIterator.h"
#include "gameWorld.h"
#include "doorComponent.h"
#include "storySceneVoicetagMapping.h"
#include "../engine/environmentManager.h"

// For frame prefetches
#include "../engine/renderFramePrefetch.h"
#include "../engine/renderer.h"
#include "../engine/renderCommands.h"


#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

namespace Config
{
	TConfigVar<Float> cvBlendToGameplayDuration( "Scenes/Camera", "BlendToGameplayDuration", 1.5f, eConsoleVarFlag_ReadOnly );
}

IMPLEMENT_ENGINE_CLASS( SStorySceneGameplayActionCallbackInfo );

RED_DEFINE_STATIC_NAME( SCENE_GMPL_SLOT );
RED_DEFINE_STATIC_NAME( MIXER_SLOT_EYES );
RED_DEFINE_STATIC_NAME( MIXER_SLOT_POSE );
RED_DEFINE_STATIC_NAME( MIXER_SLOT_ANIM );
RED_DEFINE_STATIC_NAME( MIXER_SLOT_OVERRIDE );
RED_DEFINE_STATIC_NAME( OnCameraBlendToGameplay );

const CName& CStorySceneEventsExecutor::MIXER_SLOT_NAME_CAMERA = CNAME( MIXER_SLOT );
const CName& CStorySceneEventsExecutor::MIXER_SLOT_NAME_BODY = CNAME( MIXER_SLOT );
const CName& CStorySceneEventsExecutor::MIXER_SLOT_NAME_BODY_GAMEPLAY = CNAME( SCENE_GMPL_SLOT );
const CName& CStorySceneEventsExecutor::MIXER_SLOT_NAME_MIMICS_LAYER_EYES = CNAME( MIXER_SLOT_EYES );
const CName& CStorySceneEventsExecutor::MIXER_SLOT_NAME_MIMICS_LAYER_POSE = CNAME( MIXER_SLOT_POSE );
const CName& CStorySceneEventsExecutor::MIXER_SLOT_NAME_MIMICS_LAYER_ANIM = CNAME( MIXER_SLOT_ANIM );
const CName& CStorySceneEventsExecutor::MIXER_SLOT_NAME_MIMICS_LAYER_OVERRIDE = CNAME( MIXER_SLOT_OVERRIDE );

CStorySceneEventsExecutor::CStorySceneEventsExecutor()
	: m_actorsMap( NULL )
	, m_updateID( 1 )
	, m_changedEnvs( -1 )
	, m_blendOutEnv( -1 )
{

}

void CStorySceneEventsExecutor::Init( const THashMap< CName, THandle< CEntity > >* actorMap, const THashMap< CName, THandle< CEntity > >* propMap, const THashMap< CName, THandle< CEntity > >* lightMap, CCamera* camera )
{
	// Cache all actors
	m_actorsMap = actorMap;
	m_propsMap	= propMap;
	m_lightsMap = lightMap;

	// Create camera mixer
	if ( camera && camera->GetRootAnimatedComponent() && camera->GetRootAnimatedComponent()->GetBehaviorStack() )
	{
		camera->GetRootAnimatedComponent()->GetBehaviorStack()->GetSlot( MIXER_SLOT_NAME_CAMERA, m_cameraMixer, false );
	}

	SCENE_ASSERT( m_cameraMixer.IsValid() );
}

void CStorySceneEventsExecutor::Deinit( CStoryScenePlayer* player )
{
	ClearAllSlots();

	if( m_propsMap )
	{
		for ( auto& propIter : (*m_propsMap) )
		{
			if( CEntity* sceneProp = propIter.m_second.Get() )
			{
				if( sceneProp->GetTransformParent() )
				{
					sceneProp->GetTransformParent()->Break();
				}
			}
		}
	}

	for( TActorHiddenItems& hiddenItems : m_hiddenItems ) 
	{	
		if( CGameplayEntity* gEnt = Cast< CGameplayEntity >( hiddenItems.m_id ) )
		{
			if( CInventoryComponent* invComponent = gEnt->GetInventoryComponent() )
			{
				for( SItemUniqueId& id : hiddenItems.m_data )
				{
					invComponent->SetItemVisible( id, true );
				}
			}
		}
	}
	m_hiddenItems.Clear();


	for ( TPair<CName, SItemUniqueId>& pair : m_spawnedItems )
	{
		if( CGameplayEntity* ent = Cast<CGameplayEntity>( FindActorByType( pair.m_first, AT_ACTOR | AT_PROP ).Get() ) )
		{
			if( CInventoryComponent* inv = ent->GetInventoryComponent() )
			{
				inv->UnMountItem( pair.m_second );
				ent->UnequipItem( pair.m_second );
				inv->RemoveItem( pair.m_second );
			}
		}
	}
	m_spawnedItems.Clear();

	//In game enviroments should be already blending out from CStoryScenePlayer::StartBlendingOutLights
	DeactivateCustomEnv( player );

	for ( TActorLookAt& it : m_actorsLookAts )
	{
		it.m_data.Deinit();
	}

	m_actorsMap = nullptr;
	m_propsMap	= nullptr;
	m_lightsMap = nullptr;
}

void CStorySceneEventsExecutor::ClearSlotsForActor( const CName& actorId )
{
	const Uint32 numBody = m_actorsBodyMixers.Size();
	for ( Uint32 i=0; i<numBody; ++i )
	{
		const TActorMixer& m = m_actorsBodyMixers[ i ];
		if ( m.m_id == actorId )
		{
			m_actorsBodyMixers.RemoveAtFast( i );
			break;
		}
	}

	const Uint32 numGmpl = m_actorsGmplBodyMixers.Size();
	for ( Uint32 i=0; i<numGmpl; ++i )
	{
		const TActorMixer& m = m_actorsGmplBodyMixers[ i ];
		if ( m.m_id == actorId )
		{
			m_actorsGmplBodyMixers.RemoveAtFast( i );
			break;
		}
	}

	for ( Uint32 j=0; j<DML_Last; ++j )
	{
		TActorsMixers& mixers = m_actorsMimicsMixers[ j ];

		const Uint32 num = mixers.Size();
		for ( Uint32 i=0; i<num; ++i )
		{
			TActorMixer& m = mixers[ i ];
			if ( m.m_id == actorId )
			{
				mixers.RemoveAtFast( i );
				break;
			}
		}
	}
}

void CStorySceneEventsExecutor::ClearAllSlots()
{
	ClearAllSlots( m_actorsBodyMixers );
	ClearAllSlots( m_actorsGmplBodyMixers );

	for ( Uint32 i=0; i<DML_Last; ++i )
	{
		ClearAllSlots( m_actorsMimicsMixers[i] );
	}
}

void CStorySceneEventsExecutor::ClearAllSlots( CStorySceneEventsExecutor::TActorsMixers& mixers )
{
	const Uint32 num = mixers.Size();
	for ( Uint32 i=0; i<num; ++i )
	{
		TActorMixer& m = mixers[ i ];

		if ( m.m_data.IsValid() )
		{
			m.m_data.RemoveAllAnimations();
		}
	}
}

Bool CStorySceneEventsExecutor::GetCurrentActorIdleAnimation( const CName& actor, CName& out ) const
{
	if ( CEntity* entity = FindActorByType( actor, AT_ACTOR ).Get() )
	{
		const TActorAnimState* s = FindActorCachedData( entity, m_actorsAnimStates );
		if ( s )
		{
			out = s->m_data.m_animationBodyNameCurr;
			return true;
		}	
	}
	return false;
}

#ifndef NO_EDITOR

Bool CStorySceneEventsExecutor::GetCurrentLightState( CName lightId, SStorySceneAttachmentInfo& out, EngineTransform& outPos ) const
{
	if ( CEntity* entity = FindActorByType( lightId, AT_LIGHT ).Get() )
	{
		const TLightPropState* s = FindActorCachedData( entity, m_lightsStates );
		if ( s )
		{
			out = s->m_data.m_attachment;
			outPos = s->m_data.m_transform;
			return true;
		}
	}
	return false;
}

Bool CStorySceneEventsExecutor::GetCurrentActorState( const CName& actor, SStorySceneActorAnimationState& out ) const
{
	if ( CEntity* entity = FindActorByType( actor, AT_ACTOR ).Get() )
	{
		const TActorAnimState* s = FindActorCachedData( entity, m_actorsAnimStates );
		if ( s )
		{
			out = s->m_data.m_stateCurr;

			return true;
		}
	}
	return false;
}

Bool CStorySceneEventsExecutor::GetPreviousActorState( const CName& actor, SStorySceneActorAnimationState& out ) const
{
	if ( CEntity* entity = FindActorByType( actor, AT_ACTOR ).Get() )
	{
		const TActorAnimState* s = FindActorCachedData( entity, m_actorsAnimStates );
		if ( s )
		{
			out = s->m_data.m_statePrev;

			return true;
		}
	}

	return false;
}

Bool CStorySceneEventsExecutor::GetCurrentActorAnimationMimicState( CName actor, CName& mimicEmoState, CName& mimicLayerEyes, CName& mimicLayerPose, CName& mimicLayerAnim, Float& poseWeight ) const
{
	if ( CEntity* entity = FindActorByType( actor, AT_ACTOR ).Get() )
	{
		const TActorAnimState* s = FindActorCachedData( entity, m_actorsAnimStates );
		if ( s )
		{
			mimicEmoState = s->m_data.m_stateCurr.m_mimicsEmotionalState;
			mimicLayerEyes = s->m_data.m_stateCurr.m_mimicsLayerEyes;
			mimicLayerPose = s->m_data.m_stateCurr.m_mimicsLayerPose;
			mimicLayerAnim = s->m_data.m_stateCurr.m_mimicsLayerAnimation;
			poseWeight = s->m_data.m_mimicsPoseWeightPrev;
			return true;
		}	
	}

	return false;
}

void CStorySceneEventsExecutor::GetActorAnimationNames( const CName& actor, TDynArray< CName >& out ) const
{
	if ( CEntity* entity = FindActorByType( actor, AT_ACTOR ).Get() )
	{
		const TActorAnimState* s = FindActorCachedData( entity, m_actorsAnimStates );
		if ( s )
		{
			out.PushBack( s->m_data.m_animationBodyNamePrev );
			out.PushBack( s->m_data.m_animationBodyNameCurr );
			out.PushBack( CName( ToString( s->m_data.m_bodyWeight ) ) );

			out.PushBack( s->m_data.m_animationMimicsEyesNamePrev );
			out.PushBack( s->m_data.m_animationMimicsEyesNameCurr );
			out.PushBack( s->m_data.m_animationMimicsPoseNamePrev );
			out.PushBack( s->m_data.m_animationMimicsPoseNameCurr );
			out.PushBack( s->m_data.m_animationMimicsAnimNamePrev );
			out.PushBack( s->m_data.m_animationMimicsAnimNameCurr );
			out.PushBack( CName( ToString( s->m_data.m_mimicsPoseWeightPrev ) ) );
			out.PushBack( CName( ToString( s->m_data.m_mimicsPoseWeightCurr ) ) );
			out.PushBack( CName( ToString( s->m_data.m_mimicsWeight ) ) );

			out.PushBack( s->m_data.m_lookAtBodyAnimationPrev );
			out.PushBack( s->m_data.m_lookAtHeadAnimationPrev );
			out.PushBack( s->m_data.m_lookAtBodyAnimationCurr );
			out.PushBack( s->m_data.m_lookAtHeadAnimationCurr );
		}
	}
}

Bool CStorySceneEventsExecutor::GetActorCurrIdleAnimationNameAndTime( CName actorId, CName& animName, Float& animTime ) const
{
	if ( CEntity* entity = FindActorByType( actorId, AT_ACTOR ).Get() )
	{
		const TActorMixer* mixer = const_cast< CStorySceneEventsExecutor* >( this )->FindActorBodyMixer( actorId, entity );
		if ( mixer )
		{
			const SAnimationFullState* animS = mixer->m_data.GetIdleB();
			if ( animS )
			{
				animName = animS->m_state.m_animation;
				animTime = animS->m_state.m_currTime;

				return true;
			}
		}
	}

	return false;
}

Bool CStorySceneEventsExecutor::GetActorCurrAnimationTime( CName actorId, CName animName, Float& animTime ) const
{
	if ( CEntity* entity = FindActorByType( actorId, AT_ACTOR ).Get() )
	{
		const TActorMixer* mixer = const_cast< CStorySceneEventsExecutor* >( this )->FindActorBodyMixer( actorId, entity );
		if ( mixer )
		{
			const SAnimationFullState* animS = mixer->m_data.GetAnimationState( animName );
			if ( animS )
			{
				animName = animS->m_state.m_animation;
				animTime = animS->m_state.m_currTime;

				return true;
			}
		}
	}
	return false;
}

#endif

void CStorySceneEventsExecutor::OnAppearanceChanged( const CStoryScenePlayer* player, const CName& actorID )
{
	ClearSlotsForActor( actorID );

	if ( CEntity* entity = FindActorByType( actorID, AT_ACTOR ).Get() )
	{
		if ( CActor* actor = Cast< CActor >( entity ) )
		{
			if ( player->GetSceneController()->IsActorLocked( actor ) )
			{
				actor->MimicOn();

				if ( actor->IsSpeaking() )
				{
					actor->ReplayCurrentSpeechLipsync();
				}
			}
		}
	}
}

Bool CStorySceneEventsExecutor::IsEntityPositionControlledByScene( CEntity* entity )
{
	const TActorPlacement* placement = FindActorCachedData ( entity, m_actorsPlacements );
	if ( placement && ( placement->m_data.m_dontTeleportMe || placement->m_data.m_switchedToGameplayMode ) )
	{
		return false;
	}
	return true;
}

void CStorySceneEventsExecutor::ForceResetForAllEntities( const CStorySceneDialogsetInstance* dialogset, Bool forceDialogset, Bool isGameplay, Bool wasGameplay, Bool isCutscene, Bool wasCutscene, CStoryScenePlayer* player, IStorySceneDebugger* debugger )
{
	CStorySceneEventsCollector collector;

	SCENE_ASSERT( isGameplay == player->IsGameplayNow() );

	// Reset actors
	for ( THashMap< CName, THandle< CEntity > >::const_iterator actorIter = m_actorsMap->Begin(); actorIter != m_actorsMap->End(); ++actorIter )
	{
		const THandle< CEntity >& actorHandle = actorIter->m_second;
		const CName& actorId = actorIter->m_first;

		if ( CEntity* e = actorHandle.Get() )
		{
			ForceResetActorState( actorId, e, dialogset, forceDialogset, isGameplay, wasGameplay, isCutscene, wasCutscene, player, collector );
		}
	}

	//if ( forceDialogset && isGameplay != wasGameplay )
	//{
	//	StorySceneEventsCollector::CameraLightProp evt;
	//	const ECameraLightModType modType = isGameplay ? ECLT_Gameplay : ECLT_Scene;
	//	evt.cameraSetup.SetModifiersAllIdentityOneEnabled( modType );
	//	evt.cameraSetup.m_scenesSystemActiveFactor = 1.f;
	//	collector.AddEvent( evt );
	//}
	
	if ( !isGameplay && !player->GetCurrentSection()->ManualFadeIn() )
	{
		StorySceneEventsCollector::Fade evt( nullptr );
		collector.AddEvent( evt );
	}

	// Force reset camera
	if ( !isGameplay )
	{
		StorySceneEventsCollector::CameraAnimation anim( nullptr );
		anim.m_reset = true;
		anim.m_isIdle = true;
		collector.AddEvent( anim );
	}

	// Force reset props
	for ( THashMap< CName, THandle< CEntity > >::const_iterator actorIter = m_propsMap->Begin(); actorIter != m_propsMap->End(); ++actorIter )
	{
		const THandle< CEntity >& actorHandle = actorIter->m_second;
		const CName& actorId = actorIter->m_first;

		if ( CEntity* e = actorHandle.Get() )
		{
			ForceResetPropState( actorId, e, dialogset, forceDialogset, player, collector );
		}
	}

	collector.AddEvent( StorySceneEventsCollector::VideoOverlay( String() ) );

	Execute( player, collector, debugger );
}


void CStorySceneEventsExecutor::ResetActiveEffectsOnEnt( CName id, CEntity* ent, CStorySceneEventsCollector& collector  )
{
	TActorActiveEffects* effects = FindActorCachedData( ent, m_activeEffects );
	if ( effects )
	{
		Uint32 size = effects->m_data.Size();
		for ( Uint32 i = 0; i < size; ++i )
		{
			const ActorEffectState& state = effects->m_data[i];
			if( !state.m_persistAcrossSections )
			{
				ent->DestroyEffect( state.m_name );
			}
		}
	}	
}

void CStorySceneEventsExecutor::ForceResetPropState( const CName& propId, CEntity* e, const CStorySceneDialogsetInstance* dialogset, Bool forceDialogset, CStoryScenePlayer* player, CStorySceneEventsCollector& collector )
{
	SCENE_ASSERT( dialogset || !forceDialogset );

	if ( forceDialogset )
	{
		// 1. Look ats
		{
			if ( forceDialogset )
			{
				StorySceneEventsCollector::ActorLookAt evt( nullptr, propId );
				evt.m_reset = true;
				collector.AddEvent( evt );
			}
		}

		// 2. Effects
		{
			ResetActiveEffectsOnEnt( propId, e, collector );
		}
	}
}

void CStorySceneEventsExecutor::ForceResetActorState( const CName& actorId, CEntity* entity, const CStorySceneDialogsetInstance* dialogset, Bool forceDialogset, Bool isGameplay, Bool wasGameplay, Bool isCutscene,Bool wasCutscene, CStoryScenePlayer* player, CStorySceneEventsCollector& collector )
{
	//++ Prev mimic stuff
	CBehaviorManualSlotInterface manualMimicSlotInterface;

	SCENE_ASSERT( dialogset || !forceDialogset );

	CActor* actor = Cast< CActor >( entity );
	if ( actor && forceDialogset )
	{
		actor->DisableDialogsLookAts( 0.0f );

		actor->SetMimicVariable( CNAME( MIMIC_FILTER_ENABLE_VARIABLE ), 0.0f );
		actor->SetMimicVariable( CNAME( MIMIC_FILTER_INDEX_VARIABLE ), 0.0f );
		actor->SetMimicVariable( CNAME( MIMIC_POSE_ENABLE_VARIABLE ), 0.0f );
		actor->SetMimicVariable( CNAME( MIMIC_POSE_ENABLE_VARIABLE ), 0.0f );
	}
	//--

	// 0. active effects
	if ( forceDialogset )
	{
		ResetActiveEffectsOnEnt( actorId, entity, collector );
	}
	
	const CStorySceneDialogsetSlot* slot = dialogset ? dialogset->GetSlotByActorName( actorId ) : nullptr;
	if ( slot )
	{
		if ( forceDialogset || isCutscene )
		{
			// 1. Body idle
			{
				SStorySceneActorAnimationState state;
				state.m_status = slot->GetBodyFilterStatus();
				state.m_emotionalState = slot->GetBodyFilterEmotionalState();
				state.m_poseType = slot->GetBodyFilterPoseName();
				state.m_mimicsEmotionalState = slot->GetMimicsFilterEmotionalState();
				slot->GetMimicsLayers( state.m_mimicsLayerEyes, state.m_mimicsLayerPose, state.m_mimicsLayerAnimation );

				#define IF_EMPTY_SET_DEFAULT_STATE( s, val ) if ( !s ) s = val;
				IF_EMPTY_SET_DEFAULT_STATE( state.m_status,					CStorySceneAnimationList::DEFAULT_STATUS );
				IF_EMPTY_SET_DEFAULT_STATE( state.m_emotionalState,			CStorySceneAnimationList::DEFAULT_EMO_STATE );
				IF_EMPTY_SET_DEFAULT_STATE( state.m_poseType,				CStorySceneAnimationList::DEFAULT_POSE );
				IF_EMPTY_SET_DEFAULT_STATE( state.m_mimicsEmotionalState,	CStorySceneAnimationList::DEFAULT_MIMICS_EMO_STATE );
				IF_EMPTY_SET_DEFAULT_STATE( state.m_mimicsLayerEyes,		CStorySceneAnimationList::DEFAULT_MIMICS_LAYER_EYES );
				IF_EMPTY_SET_DEFAULT_STATE( state.m_mimicsLayerPose,		CStorySceneAnimationList::DEFAULT_MIMICS_LAYER_POSE );
				IF_EMPTY_SET_DEFAULT_STATE( state.m_mimicsLayerAnimation,	CStorySceneAnimationList::DEFAULT_MIMICS_LAYER_ANIMATION );
				#undef IF_EMPTY_SET_DEFAULT_STATE

				StorySceneEventsCollector::ActorChangeState evt( nullptr, actorId );
				const CName& idleAnim = slot->GetForceBodyIdleAnimation();
				if ( idleAnim && !isCutscene )
				{
					evt.m_bodyBlendWeight = slot->GetForceBodyIdleAnimationWeight();
					evt.m_bodyBlendSet = true;
					evt.m_forceBodyIdleAnimation = idleAnim;
				}
				else
				{
					evt.m_reset = true;
				}
				evt.m_state = state;
				evt.m_mimicsPoseWeight = slot->GetMimicsPoseWeight();
				evt.m_ID = slot->GetID();

				collector.AddEvent( evt );
			}
		}

		if ( forceDialogset )		
		{
			// 2. Placement
			{
				StorySceneEventsCollector::ActorPlacement evt( nullptr, actorId );
				evt.m_placementSS = slot->GetSlotPlacement();
				evt.m_sceneTransformWS = player->GetSceneDirector()->GetCurrentScenePlacement();

				collector.AddEvent( evt );
			}

			// 3. Look ats
			{
				StorySceneEventsCollector::ActorLookAt evt( nullptr, actorId );
				evt.m_reset = true;

				collector.AddEvent( evt );
			}

			// 4. Visibility
			{
				StorySceneEventsCollector::ActorVisibility evt( nullptr, actorId );
				evt.m_showHide = slot->GetActorVisibility();

				collector.AddEvent( evt );
			}
		
			// 6. Cloth and dangle shake
			{
				StorySceneEventsCollector::ActorDanglesShake evtS( nullptr, actorId );
				collector.AddEvent( evtS );
			}

			// 6a. Cloth and dangle reset
			{
				StorySceneEventsCollector::ActorResetClothAndDangles evtS( nullptr, actorId );
				collector.AddEvent( evtS );
			}
		}

		// 7. Hires shadows
		{
			StorySceneEventsCollector::ActorUseHiresShadows evt( nullptr, actorId );
			evt.m_useHiresShadows = player->ShouldActorHaveHiResShadows( actor );
			collector.AddEvent( evt );
		}

		// 8. Mimic lod
		{
			StorySceneEventsCollector::ActorMimicLod evt( nullptr, actorId );
			evt.m_setMimicOn = player->ShouldActorHaveMimicOn( actor );
			collector.AddEvent( evt );
		}
	}
	else
	{
		// 1. Visibility
		if ( forceDialogset )
		{
			StorySceneEventsCollector::ActorVisibility evt( nullptr, actorId );

			const CStorySceneCutsceneSection* csSection = Cast< CStorySceneCutsceneSection >( player->GetCurrentSection() );
			evt.m_showHide = csSection && csSection->HasActor( actorId );
			collector.AddEvent( evt );
		}

	}

	{
		if ( ( forceDialogset && wasCutscene ) || ( !isGameplay && wasGameplay ) )
		{
			StorySceneEventsCollector::ActorItem evt( nullptr, actorId );
			collector.AddEvent( evt );
		}
	}
}

Bool CStorySceneEventsExecutor::HasActorSceneBehavior( const CEntity* e ) const
{
	return e->GetRootAnimatedComponent() && e->GetRootAnimatedComponent()->GetBehaviorStack() && e->GetRootAnimatedComponent()->GetBehaviorStack()->HasActiveInstance( CNAME( StoryScene ) );
}

Bool CStorySceneEventsExecutor::IsSyncItemValid( CName actorId )
{
	return m_syncItem.m_isActive && m_syncItem.m_parentActor &&  m_syncItem.m_parentActor == actorId && m_syncItem.m_itemMixer.IsValid();
}

void CStorySceneEventsExecutor::OpenSyncItemMixer(CName actorId)
{
	if( IsSyncItemValid( actorId ) )
	{
		m_syncItem.m_itemMixer.OpenMixer();
	}
}

void CStorySceneEventsExecutor::CloseSyncItemMixers()
{
	if( IsSyncItemValid( m_syncItem.m_parentActor ) )
	{
		m_syncItem.m_itemMixer.CloseMixer();
	}
}

void CStorySceneEventsExecutor::SetSyncItemIdleAnimToSample(CName actorId, const SAnimationFullState& animationA, const SAnimationFullState& animationB, Float blendWeight, Bool canRandAnimStartTime /*= false*/, CAnimationMixerAnimSynchronizer* synchronizer /*= nullptr */)
{
	if( IsSyncItemValid( actorId ) )
	{
		m_syncItem.m_itemMixer.SetIdleAnimationToSample( animationA, animationB, blendWeight, canRandAnimStartTime, synchronizer );
	}
}

void CStorySceneEventsExecutor::AddSyncItemAnimToSample( CName actorId, const SAnimationFullState& anim, EAnimationType type )
{
	if( IsSyncItemValid( actorId ) )
	{
		if ( type == EAT_Normal )
		{
			m_syncItem.m_itemMixer.AddAnimationToSample( anim );
		}
		else if ( type == EAT_Additive )
		{
			m_syncItem.m_itemMixer.AddAdditiveAnimationToSample( anim );
		}
		else if ( type == EAT_Override )
		{
			m_syncItem.m_itemMixer.AddOverrideAnimationToSample( anim );
		}
	}
}

void CStorySceneEventsExecutor::RemoveSyncItemPose( CName actorId, Uint32 poseId )
{
	if( IsSyncItemValid( actorId ) )
	{
		m_syncItem.m_itemMixer.RemovePose( poseId );
	}
}

void CStorySceneEventsExecutor::RemoveAllSyncItemPoses( CName actorId )
{
	if( IsSyncItemValid( actorId ) )
	{
		m_syncItem.m_itemMixer.RemoveAllPoses();
	}
}

void CStorySceneEventsExecutor::AddSyncItemPoseToSample( CName actorId, Uint32 poseId, const SAnimationMappedPose& pose )
{
	if( IsSyncItemValid( actorId ) )
	{
		m_syncItem.m_itemMixer.AddPoseToSample( poseId, pose );
	}
}

CStorySceneEventsExecutor::TActorMixer* CStorySceneEventsExecutor::CreateActorBodyMixer( const CName& mixerName, CStorySceneEventsExecutor::TActorsMixers& mixers, const CName& actorId, CEntity* actor )
{
	Int32 index = (Int32)mixers.Grow( 1 );
	TActorMixer& newMixer = mixers[ index ];

	newMixer.m_id = actorId;

	if ( actor->GetRootAnimatedComponent() && actor->GetRootAnimatedComponent()->GetBehaviorStack() )
	{
		actor->GetRootAnimatedComponent()->GetBehaviorStack()->GetSlot( mixerName, newMixer.m_data, false, false );
	}

	SCENE_ASSERT( newMixer.m_data.IsValid() );

	return &newMixer;
}

CStorySceneEventsExecutor::TActorMixer* CStorySceneEventsExecutor::CreateActorBodyMixer( const CName& actorId, CEntity* actor )
{
	if ( HasActorSceneBehavior( actor ) )
	{
		return CreateActorBodyMixer( MIXER_SLOT_NAME_BODY, m_actorsBodyMixers, actorId, actor );
	}
	else
	{
		return CreateActorBodyMixer( MIXER_SLOT_NAME_BODY_GAMEPLAY, m_actorsGmplBodyMixers, actorId, actor );
	}
}

CStorySceneEventsExecutor::TActorMixer* CStorySceneEventsExecutor::FindActorBodyMixer( const CName& actorId, const CEntity* e )
{
	SCENE_ASSERT( e );

	if ( HasActorSceneBehavior( e ) )
	{
		return FindActorCachedData( actorId, m_actorsBodyMixers );
	}
	else
	{
		return FindActorCachedData( actorId, m_actorsGmplBodyMixers );
	}
}

CStorySceneEventsExecutor::TActorMixer* CStorySceneEventsExecutor::CreateActorMimicsMixer( const CName& actorId, CEntity* actor, EDialogMimicsLayer layer )
{
	SCENE_ASSERT( layer >= 0 && layer < DML_Last );

	TActorsMixers& arr = m_actorsMimicsMixers[ layer ];

	Int32 index = (Int32)arr.Grow( 1 );
	TActorMixer& newMixer = arr[ index ];

	newMixer.m_id = actorId;

	CAnimatedComponent* ac = actor->QueryActorInterface() ? actor->QueryActorInterface()->GetMimicComponent() : nullptr;

	CName slotName = MIXER_SLOT_NAME_MIMICS_LAYER_EYES;
	if ( layer == DML_Eyes )
	{
		slotName = MIXER_SLOT_NAME_MIMICS_LAYER_EYES;
	}
	else if ( layer == DML_Pose )
	{
		slotName = MIXER_SLOT_NAME_MIMICS_LAYER_POSE;
	}
	else if ( layer == DML_Animation )
	{
		slotName = MIXER_SLOT_NAME_MIMICS_LAYER_ANIM;
	}
	else if ( layer == DML_Override )
	{
		slotName = MIXER_SLOT_NAME_MIMICS_LAYER_OVERRIDE;
	}
	else
	{
		SCENE_ASSERT( 0 );
	}

	if ( ac && ac->GetBehaviorStack() )
	{
		ac->GetBehaviorStack()->GetSlot( slotName, newMixer.m_data, false );
	}

	return &newMixer;
}

void CStorySceneEventsExecutor::SpawnPropsOnDemand( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger )
{
	for ( const StorySceneEventsCollector::PropVisibility& evt : collector.m_propVisibility )
	{
		if( player->GetStoryScene()->GetPropDefinition( evt.m_actorId ) || player->GetCurrentStoryScene()->GetPropDefinition( evt.m_actorId ) )
		{
			//( Note spawnEntity = true ) this function will spawn prop entity if its not yet spawned
			player->SpawnScenePropEntity( evt.m_actorId );
		}
	}	
}

void CStorySceneEventsExecutor::Execute( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger )
{
	// Update counter
	m_updateID++;

	const CStorySceneAnimationContainer& c = player->GetAnimationContainer();
	
	SpawnPropsOnDemand( player, collector, debugger );

	ProcessTimeModifiers( player, collector, debugger );

	// Appearances
	ProcessAppearances( collector );

	// Camera pass
	ProcessCameras( player, collector, debugger );

	// Fade
	ProcessFade( player, collector, debugger );

	// Placement pass
	Bool placementChanged = FillActorsPlacements( collector, debugger );
	placementChanged |=		FillActorsMotions( collector, debugger );
	placementChanged |=		ProcessChangeGameStates( player, collector ); // Process game state here because of snap to terrain option
	if ( placementChanged )
	{
		ProcessActorsTransforms( player, collector, debugger );
	}

	// Change states
	ProcessChangeStates( collector, c, debugger );

	// Look at Pre pass - gather only informations
	TActorSignalsMixerPrevState signalsMixerPrevState;
	ProcessActorsLookAts_Pre( collector, debugger, signalsMixerPrevState );

	// Animation pass
	TActorSignalsDisableLookAtBLP lowerBodyPartLookAtDisabledRequests;
	ProcessBodyAnimations( player, collector, c, debugger, lowerBodyPartLookAtDisabledRequests );
	ProcessMimicsAnimations( collector, c, debugger );

	// Props attach
	ProcessPropsAttachEvents( player, collector, debugger );

	// Actors properties
	ProcessVisibility( player, collector, debugger );
	ProcessItems( player, collector, debugger );
	ProcessMorph( collector, debugger );
	ProcessCloth( collector, debugger );
	ProcessHiresShadows( player, collector );
	ProcessMimicLod( player, collector );
	ProcessActorLodOverrides( player, collector );
	ProcessActorsPrepareToAndFinishTalk( collector );

	// Look ats
	ProcessActorsLookAts_Post( collector, debugger, lowerBodyPartLookAtDisabledRequests, signalsMixerPrevState );

	// Background lines
	ProcessPlayDialogLineEvents( collector, debugger );

	// Props transforms
	ProcessPropsTransforms( player, collector, debugger );

	// Light properties
	ProcessLightProperties( player, collector );

	// Force unfrozen state for all actors
	ProcessUnfrozenActors();

	// Effects
	ProcessEffects( player, collector, debugger );

	// Video overlay
	ProcessVideoOverlay( player, collector, debugger );

	// Debug comments
	ProcessDebugComments( player, collector, debugger );

	ProcessEnvChanges( player, collector, debugger );

	ProcessDoorsEvents( player, collector, debugger );

	ProcesItemSync( player, collector, debugger );
}

void CStorySceneEventsExecutor::ProcessTimeModifiers( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger )
{
	const Uint32 size = collector.m_timeMultiplier.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const StorySceneEventsCollector::TimeMultiplier& evt = collector.m_timeMultiplier[i];

		if ( evt.m_enable )
		{
			player->SetCurrentTimeMultiplier( Clamp( evt.m_multiplier, 0.f, 100.f ) );
		}
		else
		{
			player->RemoveTimeMultiplier();
		}
	}
}

void CStorySceneEventsExecutor::ProcessVideoOverlay( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger )
{
	const Uint32 size = collector.m_videoOverlays.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const StorySceneEventsCollector::VideoOverlay& evt = collector.m_videoOverlays[i];
		
		if( player->IsPlayingVideo() )
		{
			player->StopVideo();
		}

		if ( !evt.m_params.m_fileName.Empty() )
		{
			player->PlayVideo( evt.m_params );
		}
	}
}

void CStorySceneEventsExecutor::ProcessEffects( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger )
{	
	const Uint32 size = collector.m_playEffectEvents.Size();

#ifndef NO_EDITOR
	if ( !player->ShouldPlayEffects() )
	{
		return;
	}
#endif

	for ( Uint32 i=0; i<size; ++i )
	{		
		const StorySceneEventsCollector::PlayEffect& evt = collector.m_playEffectEvents[ i ];	

		if ( CEntity* entity = FindActorByType( evt.m_actorId, AT_ACTOR | AT_PROP  ).Get() )
		{
			if( evt.m_effectName )
			{
				const ActorEffectState actorEffectState = { evt.m_effectName, evt.m_persistAcrossSections };
				if( evt.m_startStop )
				{
					if( entity->GetActiveEffect( evt.m_effectName ) == NULL )
					{
						FindOrCreateActorCachedData( entity, m_activeEffects )->m_data.PushBackUnique( actorEffectState );
						entity->PlayEffect( evt.m_effectName );
					}
				}
				else
				{
					TActorActiveEffects* data = FindActorCachedData( entity , m_activeEffects );
					if ( data )
					{
						data->m_data.Remove( actorEffectState );
					}
					entity->StopEffect( actorEffectState.m_name );
				}
			}
		}
	}
}

void CStorySceneEventsExecutor::ProcessVisibility( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger )
{
	Uint32 size = collector.m_actorVisibility.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const StorySceneEventsCollector::ActorVisibility& evt = collector.m_actorVisibility[ i ];
		SCENE_ASSERT( evt.m_actorId != CName::NONE );
		THandle< CEntity > entityH;
		if ( CEntity* entity = FindActorByType( evt.m_actorId, AT_ACTOR ).Get() )
		{	
			player->GetSceneDirector()->SetActorVisibleInScene( evt.m_actorId, evt.m_showHide );
		}
	}

	size = collector.m_propVisibility.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const StorySceneEventsCollector::PropVisibility& evt = collector.m_propVisibility[ i ];
		SCENE_ASSERT( evt.m_actorId != CName::NONE );
		THandle< CEntity > entityH;
		if ( CEntity* entity = FindActorByType( evt.m_actorId, AT_PROP ).Get() )
		{
			entity->SetHideInGame( ! evt.m_showHide, true, CEntity::HR_Scene );
		}
	}

	size = collector.m_actorItemVisibility.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const StorySceneEventsCollector::ActorItemVisibility& evt = collector.m_actorItemVisibility[ i ];
		SCENE_ASSERT( evt.m_actorId != CName::NONE );
		THandle< CEntity > entityH;
		if ( CEntity* entity = FindActorByType( evt.m_actorId, AT_ACTOR ).Get() )
		{
			if( CGameplayEntity* gEnt = Cast<CGameplayEntity>( entity ) )
			{
				if( CInventoryComponent* invComponent = gEnt->GetInventoryComponent() )
				{		
					TActorHiddenItems* data = FindActorCachedData( entity , m_hiddenItems );
					if ( evt.m_reset )
					{					
						if ( data )
						{
							for ( SItemUniqueId& id : data->m_data )
							{
								invComponent->SetItemVisible( id, true );
							}		
							data->m_data.Clear();
						}						
						continue;
					}

					if( evt.m_showHide )
					{
						if( data )
						{
							data->m_data.Remove( evt.m_item );
						}
					}
					else
					{
						FindOrCreateActorCachedData( entity , m_hiddenItems )->m_data.PushBackUnique( evt.m_item );
					}

					invComponent->SetItemVisible( evt.m_item, evt.m_showHide );
				}
			}		
		}
	}	
}

void CStorySceneEventsExecutor::ProcessAppearances( const CStorySceneEventsCollector& collector )
{
	const Uint32 size = collector.m_actorAppearances.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const StorySceneEventsCollector::ActorApplyAppearance& evt = collector.m_actorAppearances[ i ];
		SCENE_ASSERT( evt.m_actorId != CName::NONE );
		SCENE_ASSERT( evt.m_appearance != CName::NONE );

		THandle< CEntity > entityH;
		if ( CEntity* entity = FindActorByType( evt.m_actorId, AT_ACTOR | AT_PROP ).Get() )
		{	
			if ( CActor* a = Cast< CActor >( entity ) )
			{
				const CName& currApp = a->GetAppearance();
				if ( currApp != evt.m_appearance )
				{
					a->ApplyAppearance( evt.m_appearance );
				}
			}
		}
	}
}

void CStorySceneEventsExecutor::ProcessHiresShadows( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector )
{
	const Uint32 size = collector.m_actorHiresShadows.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const StorySceneEventsCollector::ActorUseHiresShadows& evt = collector.m_actorHiresShadows[ i ];
		SCENE_ASSERT( evt.m_actorId != CName::NONE );

		THandle< CEntity > entityH;
		if ( CEntity* entity = FindActorByType( evt.m_actorId, AT_ACTOR ).Get() )
		{	
			if ( CActor* a = Cast< CActor >( entity ) )
			{
				player->SetActorHiResShadow( a, evt.m_useHiresShadows );
			}
		}
	}
}

void CStorySceneEventsExecutor::ProcessMimicLod( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector )
{
	const Uint32 size = collector.m_actorMimicLod.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const StorySceneEventsCollector::ActorMimicLod& evt = collector.m_actorMimicLod[ i ];
		SCENE_ASSERT( evt.m_actorId != CName::NONE );

		THandle< CEntity > entityH;
		if ( CEntity* entity = FindActorByType( evt.m_actorId, AT_ACTOR ).Get() )
		{	
			if ( CActor* a = Cast< CActor >( entity ) )
			{
				player->SetActorMimicOn( a, evt.m_setMimicOn );
			}
		}
	}
}

void CStorySceneEventsExecutor::ProcessActorLodOverrides( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector )
{
	struct Functor : public Red::System::NonCopyable
	{
		Functor( CStoryScenePlayer& scenePlayer, const StorySceneEventsCollector::ActorLodOverride& actorLodOverride )
		: m_scenePlayer( scenePlayer ), m_actorLodOverride( actorLodOverride )
		{}

		Bool EarlyTest( CNode* node )
		{
			const Bool isActor = ( Cast< CActor >( node ) != nullptr );
			return isActor;
		}

		void Process( CNode* node, Bool isGuaranteedUnique )
		{
			// Assert that CNode is actually a CActor - we've checked this in EarlyTest().
			RED_FATAL_ASSERT( Cast< CActor >( node ) != nullptr, "" );

			CActor* actor = static_cast< CActor* >( node );
			m_scenePlayer.SetActorLodOverride( actor, m_actorLodOverride.m_forceHighestLod, m_actorLodOverride.m_disableAutoHide );
		}

		CStoryScenePlayer& m_scenePlayer;
		const StorySceneEventsCollector::ActorLodOverride& m_actorLodOverride;
	};

	CTagManager* tagManager = nullptr;
	if( GGame && GGame->GetActiveWorld() && GGame->GetActiveWorld()->GetTagManager() )
	{
		tagManager = GGame->GetActiveWorld()->GetTagManager();
	}

	for ( const StorySceneEventsCollector::ActorLodOverride& evt : collector.m_actorLodOverrides )
	{
		// If evt.m_actorId == CName::NONE then event should not be added to event collector and we shouldn't see that event here.
		SCENE_ASSERT( evt.m_actorId != CName::NONE );

		// process target actor that was specified by voicetag
		THandle< CEntity > entityH;
		if ( CEntity* entity = FindActorByType( evt.m_actorId, AT_ACTOR ).Get() )
		{
			if ( CActor* a = Cast< CActor >( entity ) )
			{
				player->SetActorLodOverride( a, evt.m_forceHighestLod, evt.m_disableAutoHide );
			}
		}

		// process target actors specified by tags
		if( tagManager )
		{
			Functor functor( *player, evt );
			tagManager->IterateTaggedNodes( evt.m_actorsByTag, functor, BCTO_MatchAny );
		}
	}
}

namespace
{ 
	SItemUniqueId MountItemHelper( CInventoryComponent* inv, CName item )
	{
		SItemUniqueId spawnedItem;
		if( item )
		{
			CGameplayEntity* gpEnt = Cast<CGameplayEntity>( inv->GetEntity() );
			SItemUniqueId itemId = inv->GetItemId( item );
			if ( !itemId )
			{
				auto itemsAdded = inv->AddItem( item );
				spawnedItem = itemId = itemsAdded.Size() > 0 ? itemsAdded[0] : SItemUniqueId::INVALID;								
			}			
			if ( !itemId )
			{
				itemId = inv->GetItemByCategoryForScene( item );
			}

			CInventoryComponent::SMountItemInfo mountInfo;
			inv->MountItem( itemId, mountInfo );
			gpEnt->EquipItem( itemId, true );
		}

		return spawnedItem;
	}

	void UnmountItemHelper( CInventoryComponent* inv, CName item )
	{
		if( item )
		{
			CGameplayEntity* gpEnt = Cast<CGameplayEntity>( inv->GetEntity() );
			SItemUniqueId itemId = inv->GetItemId( item );
			if ( !itemId )
			{
				itemId = inv->GetItemByCategoryForScene( item );
			}
			CInventoryComponent::SMountItemInfo mountInfo;
			inv->UnMountItem( itemId );			
			gpEnt->UnequipItem( itemId );
		}
	}
}

void CStorySceneEventsExecutor::ProcessItems( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger )
{
	const Uint32 size = collector.m_actorItems.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const StorySceneEventsCollector::ActorItem& evt = collector.m_actorItems[ i ];

		SCENE_ASSERT( evt.m_actorId != CName::NONE );

		if ( CEntity* entity = FindActorByType( evt.m_actorId, AT_ACTOR ).Get() )
		{
			if ( CActor* actor = Cast< CActor >( entity ) )
			{
				if ( evt.m_useMountInstead )
				{
					CInventoryComponent* inv = actor->GetInventoryComponent();
					if ( SItemUniqueId addedItem = MountItemHelper( inv, evt.m_rightItem ) )
					{
						m_spawnedItems.PushBack( MakePair( evt.m_actorId, addedItem ) );
					}
					if ( SItemUniqueId addedItem = MountItemHelper( inv, evt.m_leftItem ) )
					{
						m_spawnedItems.PushBack( MakePair( evt.m_actorId, addedItem ) );
					}
				}
				else if( evt.m_useUnmountInstead )
				{
					CInventoryComponent* inv = actor->GetInventoryComponent();
					UnmountItemHelper( inv, evt.m_rightItem );
					UnmountItemHelper( inv, evt.m_leftItem );
				}
				else
				{
					CActor::IssueRequiredItemsInfo info;
					info.m_usePriorityForSceneItems = true;

					actor->IssueRequiredItems( SActorRequiredItems( evt.m_leftItem, evt.m_rightItem ), evt.m_instant, &info );

					for ( SItemUniqueId& itemId : info.m_spawnedItems )
					{
						m_spawnedItems.PushBack( MakePair( evt.m_actorId, itemId ) );
					}
				}				
			}
		}
	}
}

void CStorySceneEventsExecutor::ProcessMorph( const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger )
{
	const Uint32 size = collector.m_actorMorph.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const StorySceneEventsCollector::ActorMorph& evt = collector.m_actorMorph[ i ];

		SCENE_ASSERT( evt.m_actorId != CName::NONE );

		if ( CEntity* entity = FindActorByType( evt.m_actorId, AT_ACTOR ).Get() )
		{
			for ( ComponentIterator< CMorphedMeshComponent > it( entity ); it; ++it )
			{
				CMorphedMeshComponent* m = *it;

				if ( m->GetMorphComponentId() == evt.m_morphComponentId )
				{
					m->SetMorphRatio( evt.m_weight );
				}
			}
		}
	}
}

void CStorySceneEventsExecutor::ForceResetWithRelaxedClothState( CActor* a, Bool cloth, Bool dangle )
{
	if ( cloth )
	{
#ifdef USE_APEX
		for ( EntityWithItemsComponentIterator< CClothComponent > it( a ); it; ++it )
		{
			CClothComponent* c = *it;
			c->RequestTeleport();
		}
#endif
	}

	if ( dangle )
	{
		for ( EntityWithItemsComponentIterator< CAnimDangleComponent > it( a ); it; ++it )
		{
			CAnimDangleComponent* c = *it;
			c->ForceResetWithRelaxedState();
		}
	}
}

void CStorySceneEventsExecutor::ProcessCloth( const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger )
{
#ifdef USE_APEX
	{
		const Uint32 size = collector.m_actorDisablePhysicsCloth.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const StorySceneEventsCollector::ActorDisablePhysicsCloth& evt = collector.m_actorDisablePhysicsCloth[ i ];

			SCENE_ASSERT( evt.m_actorId != CName::NONE );

			if ( CEntity* entity = FindActorByType( evt.m_actorId, AT_ACTOR ).Get() )
			{
				if ( CActor* a = Cast< CActor >( entity ) )
				{
					for ( EntityWithItemsComponentIterator< CClothComponent > it( a ); it; ++it )
					{
						CClothComponent* c = *it;

						c->SetMaxDistanceBlendTime( evt.m_blendTime );
						c->SetMaxDistanceScale( 1.f-evt.m_weight );
					}
				}
			}
		}
	}
#endif

	{
		const Uint32 size = collector.m_actorDisableDangle.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const StorySceneEventsCollector::ActorDisableDangle& evt = collector.m_actorDisableDangle[ i ];

			SCENE_ASSERT( evt.m_actorId != CName::NONE );

			if ( CEntity* entity = FindActorByType( evt.m_actorId, AT_ACTOR ).Get() )
			{
				if ( CActor* a = Cast< CActor >( entity ) )
				{
					for ( EntityWithItemsComponentIterator< CAnimDangleComponent > it( a ); it; ++it )
					{
						CAnimDangleComponent* c = *it;
						c->SetBlendToAnimationWeight( evt.m_weight );
					}
				}
			}
		}
	}

	{
		const Uint32 size = collector.m_actorDanglesShakes.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const StorySceneEventsCollector::ActorDanglesShake& evt = collector.m_actorDanglesShakes[ i ];

			SCENE_ASSERT( evt.m_actorId != CName::NONE );

			if ( CEntity* entity = FindActorByType( evt.m_actorId, AT_ACTOR ).Get() )
			{
				if ( CActor* a = Cast< CActor >( entity ) )
				{
					for ( EntityWithItemsComponentIterator< CAnimDangleComponent > it( a ); it; ++it )
					{
						CAnimDangleComponent* c = *it;
						c->SetShakeFactor( evt.m_factor );
					}
				}
			}
		}
	}

	{
		const Uint32 size = collector.m_actorResetClothAndDangles.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const StorySceneEventsCollector::ActorResetClothAndDangles& evt = collector.m_actorResetClothAndDangles[ i ];

			SCENE_ASSERT( evt.m_actorId != CName::NONE );

			if ( CEntity* entity = FindActorByType( evt.m_actorId, AT_ACTOR ).Get() )
			{
				if ( CActor* a = Cast< CActor >( entity ) )
				{
					if ( evt.m_forceRelaxedState )
					{
						ForceResetWithRelaxedClothState( a, evt.m_cloth, evt.m_dangle );
					}
					else
					{
						if ( evt.m_cloth )
						{
#ifdef USE_APEX
							for ( EntityWithItemsComponentIterator< CClothComponent > it( a ); it; ++it )
							{
								CClothComponent* c = *it;
								c->RequestTeleport();
							}
#endif
						}

						if ( evt.m_dangle )
						{
							for ( EntityWithItemsComponentIterator< CAnimDangleComponent > it( a ); it; ++it )
							{
								CAnimDangleComponent* c = *it;
								c->ForceReset();
							}
						}
					}
				}
			}
		}
	}
}

void CStorySceneEventsExecutor::ProcessPlayDialogLineEvents( const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger )
{
	// TEMPORARY
	const Uint32 numEvents = collector.m_playDialogLineEvents.Size();
	for( Uint32 i = 0; i < numEvents; ++i )
	{
		const StorySceneEventsCollector::PlayDialogLine& e = collector.m_playDialogLineEvents[ i ];

		THandle< CEntity > entityH;
		if ( e.m_line )
		{
			CEntity* entity = FindActorByType( e.m_actorId, AT_ACTOR ).Get();
			SCENE_ASSERT( entity );
			if ( CActor* actor = Cast< CActor >( entity ) )
			{
				const Uint32 lineStringId = e.m_line->GetLocalizedContent()->GetIndex();

				Bool subtitle = false;
				Int32 speechFlags = ASM_Text | ASM_Voice | ASM_Lipsync;

				subtitle = e.m_line->GetSection()->HasCinematicOneliners();

				if ( e.m_line->GetSection()->IsGameplay() )
				{
					speechFlags |= ASM_Gameplay;
				}

				if ( e.m_line->GetSection()->HasCinematicOneliners() )
				{
					speechFlags |= ASM_Subtitle;
				}

				ActorSpeechData speechData( lineStringId, e.m_line->GetSoundEventName(), false, speechFlags, e.m_line->GetDisableOclusionFlag(), e.m_line->IsAlternativeUI());
				speechData.m_sceneDisplay = e.m_display;

				/*TActorSpeechID speechID = */
				actor->SpeakLine( speechData );
			}
		}
	}
}

void CStorySceneEventsExecutor::ProcessDebugComments( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger )
{
	for( Uint32 i = 0, numEvents = collector.m_displayDebugCommentEvents.Size(); i < numEvents; ++i )
	{
		const StorySceneEventsCollector::DisplayDebugComment& ev = collector.m_displayDebugCommentEvents[ i ];
		ev.m_display->ShowDebugComment( ev.m_commentId, ev.m_comment );
	}

	for( Uint32 i = 0, numEvents = collector.m_hideDebugCommentEvents.Size(); i < numEvents; ++i )
	{
		const StorySceneEventsCollector::HideDebugComment& ev = collector.m_hideDebugCommentEvents[ i ];
		ev.m_display->HideDebugComment( ev.m_commentId );
	}
}

void CStorySceneEventsExecutor::ProcessCameras( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger )
{
	Bool useShot = collector.m_cameraShot.m_isSet;
	Bool useBlend = collector.m_cameraBlend.m_isSet;
	Bool useBlendToGmpl = collector.m_cameraStartBlendToGameplay.m_isSet;

	if ( player->IsGameplayNow() )
	{
		SCENE_ASSERT( !useShot );
		SCENE_ASSERT( !useBlend );
		SCENE_ASSERT( !useBlendToGmpl );
		SCENE_ASSERT( collector.m_cameraAnimations.Size() == 0 );
		SCENE_ASSERT( collector.m_cameraPrefetches.Size() == 0 );
		return;
	}

	// Prefetches
	if ( !collector.m_cameraPrefetches.Empty() )
	{
		const Matrix sceneL2W = player->GetSceneSectionTransform( player->GetCurrentSection() );
		for ( const auto& evt : collector.m_cameraPrefetches )
		{
			IRenderScene* renderScene = player->GetSceneWorld()->GetRenderSceneEx();

			CRenderFrameInfo frameInfo( GGame->GetViewport() );

			const Matrix cameraWS = evt.m_camMatrixSceneLocal * sceneL2W;

			frameInfo.m_camera.Set( cameraWS.GetTranslation(), cameraWS.ToEulerAngles(), evt.m_camFov, frameInfo.m_camera.GetAspect(), frameInfo.m_camera.GetNearPlane(), frameInfo.m_camera.GetFarPlane() );
			frameInfo.m_occlusionCamera = frameInfo.m_camera;
			frameInfo.UpdateMatrices();

			CRenderFrame* renderFrame = GRender->CreateFrame( nullptr, frameInfo );

			IRenderFramePrefetch* prefetch = GRender->CreateRenderFramePrefetch( renderFrame, renderScene );
			( new CRenderCommand_StartFramePrefetch( prefetch ) )->Commit();
			prefetch->Release();

			renderFrame->Release();
		}
	}


	// 1. Shots
	{	
		if ( useShot && useBlend )
		{
			if ( collector.m_cameraShot.m_event.m_eventTimeAbs >= collector.m_cameraBlend.m_event.m_eventTimeAbs )
			{
				useBlend = false;
			}
			else
			{
				useShot = false;
			}
		}

		if ( useBlend )
		{
			player->GetSceneDirector()->ActivateCamera( collector.m_cameraBlend.m_event.m_currentCameraState );
		}
		else if ( useShot )
		{
			player->GetSceneDirector()->ActivateCamera( collector.m_cameraShot.m_event.m_definition );
			player->GetSceneDirector()->ActivateCustomCamera();
			player->GetSceneDirector()->SetCameraNoiseMovement( collector.m_cameraShot.m_event.m_enableCameraNoise );
		}
	}

	// 2. Blend to gameplay
	if ( useBlendToGmpl )
	{
		player->GetSceneDirector()->SnapPlayerEntityToTeleportedTransform();

		player->StartBlendingOutLights( collector.m_cameraStartBlendToGameplay.m_event.m_lightsBlendTime, true );

		if ( collector.m_cameraStartBlendToGameplay.m_event.m_blendTime == 0.0f )
		{
			GCommonGame->ResetGameplayCamera();
			GCommonGame->ActivateGameCamera( 0.0f );
		}
		else
		{
			CCamera* sceneCamera = player->GetSceneCamera();
			if ( sceneCamera )
			{
				sceneCamera->ForceUpdateTransformNodeAndCommitChanges();

				ICamera::Data data;
				sceneCamera->GetData( data );

				GCommonGame->StartGameplayCameraBlendFrom( data, collector.m_cameraStartBlendToGameplay.m_event.m_blendTime );

				player->CallEvent( CNAME( OnCameraBlendToGameplay ) );
			}

			const Float blendToGameplayDuration = Config::cvBlendToGameplayDuration.Get();
			GCommonGame->ActivateGameCamera( blendToGameplayDuration, false, true, false );
		}

		player->SetSwitchedToGameplayCamera( true );
		if ( player->IsSceneInGame() )
		{
			GCommonGame->GetActiveWorld()->GetCameraDirector()->SetCameraResetDisabled( true );
		}
	}

	SCENE_ASSERT( m_cameraMixer.IsValid() );

	if ( m_cameraMixer.IsValid() )
	{
		m_cameraMixer.OpenMixer();
	}
	else
	{
		return;
	}

	// 3. Idles
	{
		const Uint32 size = collector.m_cameraAnimations.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const StorySceneEventsCollector::CameraAnimation& evt = collector.m_cameraAnimations[ i ];

			SCENE_ASSERT( evt.m_animationWeight >= 0.f || evt.m_animationWeight <= 1.f );
			SCENE_ASSERT( evt.m_blendWeight >= 0.f || evt.m_blendWeight <= 1.f );

			if ( evt.m_isIdle )
			{
				Bool toRefresh = false;

				if ( evt.m_reset )
				{
					m_cameraAnimState.m_blendWeight = 0.f;
					toRefresh = true;
				}
				else
				{
					if ( evt.m_animationState.m_animation != m_cameraAnimState.m_animationNameCurr || evt.m_animationWeight != m_cameraAnimState.m_animationWeightCurr )
					{
						m_cameraAnimState.m_animationNamePrev = m_cameraAnimState.m_animationNameCurr;
						m_cameraAnimState.m_animationWeightPrev = m_cameraAnimState.m_animationWeightCurr;

						m_cameraAnimState.m_animationNameCurr = evt.m_animationState.m_animation;
						m_cameraAnimState.m_animationWeightCurr = evt.m_animationWeight;

						m_cameraAnimState.m_blendWeight = evt.m_blendWeight;

						toRefresh = true;
					}
					else if ( evt.m_blendWeight != m_cameraAnimState.m_blendWeight )
					{
						m_cameraAnimState.m_blendWeight = evt.m_blendWeight;

						toRefresh = true;
					}
				}

				if ( toRefresh )
				{
					SAnimationFullState animationStatePrev;
					animationStatePrev.m_motion = false;
					animationStatePrev.m_state.m_animation = m_cameraAnimState.m_animationNamePrev;
					animationStatePrev.m_weight = m_cameraAnimState.m_animationWeightPrev;

					SAnimationFullState animationStateCurr;
					animationStateCurr.m_motion = false;
					animationStateCurr.m_state.m_animation = m_cameraAnimState.m_animationNameCurr;
					animationStateCurr.m_weight = m_cameraAnimState.m_animationWeightCurr;

					m_cameraMixer.SetIdleAnimationToSample( animationStatePrev, animationStateCurr, m_cameraAnimState.m_blendWeight, true );
				}
			}
		}	
	}

	// 4. Animations
	{
		const Uint32 size = collector.m_cameraAnimations.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const StorySceneEventsCollector::CameraAnimation& evt = collector.m_cameraAnimations[ i ];

			SCENE_ASSERT( evt.m_animationWeight >= 0.f || evt.m_animationWeight <= 1.f );

			if ( !evt.m_isIdle )
			{
				SAnimationFullState anim;
				anim.m_motion = false;
				anim.m_weight = evt.m_animationWeight;
				anim.m_state = evt.m_animationState;

				m_cameraMixer.AddAnimationToSample( anim );
			}
		}
	}

	SCENE_ASSERT( m_cameraMixer.IsValid() );

	m_cameraMixer.CloseMixer();
}

void CStorySceneEventsExecutor::ProcessFade( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger )
{
	const Bool doFade = collector.m_fade.m_isSet;
	if ( doFade )
	{
		const StorySceneEventsCollector::Fade& fade = collector.m_fade.m_event;

		SCENE_ASSERT( !player->IsGameplayNow() );
		if ( !player->IsGameplayNow() )
		{
			const Float fadeProgress = fade.m_duration == 0.0f ? 1.0f : fade.m_time / fade.m_duration;
			const Bool isFadeVisible = ( fade.m_isIn && fadeProgress < 1.0f ) || ( !fade.m_isIn && fadeProgress > 0.0f );
			player->EnableTickDuringFade( isFadeVisible );

			if ( player->IsSceneInGame() )
			{
				const Bool isBcSet = player->IsSceneBlackscreenSet();

				const String reason = String::Printf( TXT("'%ls' Fade event"), player->GetCurrentSection()->GetName().AsChar() );

				if ( fade.m_isIn && isBcSet )
				{
					player->SceneFadeIn( reason, ::Max( fade.m_duration - fade.m_time, 0.0f ), fade.m_color );
				}
				else if ( !fade.m_isIn && !isBcSet )
				{
					player->SceneFadeOut( reason, ::Max( fade.m_duration - fade.m_time, 0.0f ), fade.m_color );
				}
			}
			else
			{
				// Special case for editor - we set progress to have editor timeline working
				GCommonGame->SetFade( fade.m_isIn,  TXT( "Scene event fade" ), fadeProgress, fade.m_color );
			}
		}
	}
}

void CStorySceneEventsExecutor::ProcessUnfrozenActors()
{
	// TODO - do it only for non-gameplay scenes
	for ( THashMap< CName, THandle< CEntity > >::const_iterator actorIter = m_actorsMap->Begin(); actorIter != m_actorsMap->End(); ++actorIter )
	{
		const THandle< CEntity >& actorHandle = actorIter->m_second;
		if ( const CEntity* e = actorHandle.Get() )
		{
			if ( CAnimatedComponent* ac = e->GetRootAnimatedComponent() )
			{
				ac->ForceRestartFrameSkipping();
			}

			if ( const CActor* a = Cast< const CActor >( e ) )
			{
				if ( CMimicComponent* m = a->GetMimicComponent() )
				{
					m->ForceRestartFrameSkipping();
				}
			}
		}
	}
}

void CStorySceneEventsExecutor::ProcesItemSync( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger )
{
	if ( collector.m_syncItemInfo.m_isSet  )
	{		
		if( collector.m_syncItemInfo.m_event.m_activate )
		{
			const StorySceneEventsCollector::SyncItemInfo& evt = collector.m_syncItemInfo.m_event;
			m_syncItem.m_parentActor = evt.m_actorTag;
			m_syncItem.m_itemName = evt.m_itemName;
			m_syncItem.m_isActive = true;					
		}
		else
		{
			m_syncItem.m_isActive = false;
			m_syncItem.m_itemMixer.RemoveAllAnimations();
			m_syncItem.m_itemMixer.RemoveAllPoses();
			m_syncItem.m_itemMixer.Clear();
		}
	}

	if( m_syncItem.m_isActive && !m_syncItem.m_itemMixer.IsValid() )
	{
		if( CGameplayEntity* gpEnt = Cast< CGameplayEntity >( player->GetSceneActorEntity( m_syncItem.m_parentActor ) ) )
		{
			if( CInventoryComponent* inv = gpEnt->GetInventoryComponent() )
			{
				SItemUniqueId id = inv->GetItemId( m_syncItem.m_itemName );
				if ( CEntity* itemEntity = inv->GetItemEntityUnsafe( id ) )
				{
					if( CAnimatedComponent* itemAc = itemEntity->GetRootAnimatedComponent() )
					{
						itemAc->GetBehaviorStack()->GetSlot( MIXER_SLOT_NAME_BODY, m_syncItem.m_itemMixer, false, false );
					}										
				}
			}		
		}	
	}
}

void CStorySceneEventsExecutor::ProcessDoorsEvents( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger )
{
	for( const StorySceneEventsCollector::DoorChangeState& event : collector.m_doorChangeStates )
	{
		if( event.m_resetAll )
		{
			for( SDoorStateChange& state : m_changedDoors )
			{
				if( CDoorComponent* doorComp = state.m_doorComponent.Get() )
				{
					state.m_opened = false;
					doorComp->Close( true );
				}
			}
		}
		else if( CWorld* world = player->GetSceneWorld() )
		{
			if ( CTagManager* mgr = world->GetTagManager() )
			{
				TDynArray< CEntity* > doors;
				mgr->CollectTaggedEntities( event.m_doorTag, doors );
				for( CEntity* it : doors )
				{										
					if( CDoorComponent* doorComp = it->FindComponent< CDoorComponent >() )
					{
						SDoorStateChange state;
						state.m_doorTag = event.m_doorTag;
						state.m_doorComponent = doorComp;
						state.m_opened = event.m_openClose;
						Int32  ind = Int32( m_changedDoors.GetIndex( state ) );
						if ( ind >= 0 )
						{
							m_changedDoors[ind] = state;
						}
						else
						{
							m_changedDoors.PushBack( state );
						}

						if ( event.m_openClose )
						{
							doorComp->SetStateForced();
							doorComp->Open( event.m_instant, true, event.m_flipDirection );
						}
						else
						{
							doorComp->SetStateForced();
							doorComp->Close( event.m_instant );
						}			
					}				
				}
			}
		}
	}
}

void CStorySceneEventsExecutor::DeactivateCustomEnv( CStoryScenePlayer* player, Float blendTime )
{

	CLayer* layer = player->GetLayer();
	CWorld* world = layer ? layer->GetWorld() : nullptr;
	if ( world )
	{
		world->GetEnvironmentManager()->DeactivateEnvironment( m_changedEnvs, blendTime );
	}
	m_changedEnvs = -1;
}


void CStorySceneEventsExecutor::ProcessEnvChanges( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger )
{
	if( player->IsGameplayNow() )
	{
		return;
	}

	CLayer* layer = player->GetLayer();
	CWorld* world = layer ? layer->GetWorld() : nullptr;
	if ( world && collector.m_envChanges.Size() > 0 )
	{
		//for( const StorySceneEventsCollector::EnvChange& evt : collector.m_envChanges )
		const StorySceneEventsCollector::EnvChange& evt = collector.m_envChanges.Back();
		{
			if( evt.m_activate )
			{
				if( evt.m_blendInTime > 0.f)
				{
					if ( m_blendOutEnv )
					{
						world->GetEnvironmentManager()->DeactivateEnvironment( m_blendOutEnv );
					}
					if ( m_changedEnvs )
					{
						m_blendOutEnv = m_changedEnvs;
					}
					DeactivateCustomEnv( player, evt.m_blendInTime );
				}
				if( m_changedEnvs != -1 )
				{	
					m_changedEnvs = world->GetEnvironmentManager()->ActivateAreaEnvironment( evt.m_environmentDefinition , nullptr, evt.m_priority, evt.m_blendFactor, evt.m_blendInTime, m_changedEnvs );										
				}
				else
				{
					m_changedEnvs = world->GetEnvironmentManager()->ActivateAreaEnvironment( evt.m_environmentDefinition , nullptr, evt.m_priority, evt.m_blendFactor, evt.m_blendInTime );
				}					
			}
			else
			{
				DeactivateCustomEnv( player, evt.m_blendInTime );
			}	
		}
	}
}


#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
