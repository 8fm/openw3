
#include "build.h"
#include "storySceneEventsExecutor.h"
#include "storySceneEventsCollector.h"
#include "storySceneEventsCollector_events.h"
#include "storySceneDebugger.h"
#include "storySceneEventMimicFilter.h"
#include "storySceneEventMimicPose.h"
#include "storyScenePlayer.h"
#include "storySceneLine.h"
#include "actorSpeech.h"
#include "lookAtTypes.h"
#include "storySceneLookAtController.h"
#include "storySceneUtils.h"
#include "storySceneSystem.h"
#include "storySceneMotionExtractionSampler.h"
#include "../engine/behaviorGraphUtils.inl"
#include "../engine/morphedMeshComponent.h"
#include "../engine/clothComponent.h"
#include "../engine/behaviorGraphStack.h"
#include "../engine/behaviorGraphAnimationSlotNode.h"
#include "../engine/behaviorGraphAnimationManualSlot.h"
#include "../engine/mimicComponent.h"
#include "storySceneIncludes.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

RED_DEFINE_STATIC_NAME( PrepareToTalk )
RED_DEFINE_STATIC_NAME( FinishTalk )

namespace
{
	Bool IsAnimationLoaded( const CName& animationName, const CAnimatedComponent* ac, const CStorySceneAnimationContainer& c )
	{
		if ( ac && animationName )
		{
			if ( const CSkeletalAnimationContainer* cont = ac->GetAnimationContainer() )
			{
				if ( const CSkeletalAnimationSetEntry* anim = cont->FindAnimationRestricted( animationName ) )
				{
					if ( anim->GetAnimation() )
					{
						SCENE_ASSERT( c.HasAnimation( anim ) );
						return anim->GetAnimation()->IsFullyLoaded();
					}
				}
			}
		}

		return true;
	}

	Bool IsAnimationLoaded( const SAnimationFullState& state, const CAnimatedComponent* ac, const CStorySceneAnimationContainer& c )
	{
		return IsAnimationLoaded( state.m_state.m_animation, ac, c );
	}

	Bool IsAnimationLoaded_Mimics( const SAnimationFullState& state, const CEntity* e, const CStorySceneAnimationContainer& c )
	{
		const IActorInterface* a = e ? e->QueryActorInterface() : nullptr;
		return a ? IsAnimationLoaded( state, a->GetMimicComponent(), c ) : true;
	}

	Bool IsAnimationLoaded_Mimics( const CName& animationName, const CEntity* e, const CStorySceneAnimationContainer& c )
	{
		const IActorInterface* a = e ? e->QueryActorInterface() : nullptr;
		return a ? IsAnimationLoaded( animationName, a->GetMimicComponent(), c ) : true;
	}

	Bool IsAnimationLoaded_Body( const SAnimationFullState& state, const CEntity* e, const CStorySceneAnimationContainer& c )
	{
		return e ? IsAnimationLoaded( state, e->GetRootAnimatedComponent(), c ) : true;
	}

	Bool IsAnimationLoaded_Body( const CName& animationName, const CEntity* e, const CStorySceneAnimationContainer& c )
	{
		return e ? IsAnimationLoaded( animationName, e->GetRootAnimatedComponent(), c ) : true;
	}
}

THandle< CEntity > CStorySceneEventsExecutor::FindActorByType( CName actorID, Int32 types ) const
{
	THandle< CEntity > entityH;	
	if ( types & AT_ACTOR )
	{
		m_actorsMap->Find( actorID, entityH );
	}
	if ( ( types & AT_PROP ) && !entityH )
	{
		m_propsMap->Find( actorID, entityH );
	}
	if ( ( types & AT_EFFECT ) && !entityH )
	{
		//SCENE_ASSERT( 0 );
		//m_effectsMap->Find( evt.m_actorId, entityH );
	}
	if ( ( types & AT_LIGHT ) && !entityH )
	{
		m_lightsMap->Find( actorID, entityH );
	}
	return entityH;
}

void CStorySceneEventsExecutor::ProcessChangeStates( const CStorySceneEventsCollector& collector, const CStorySceneAnimationContainer& c, IStorySceneDebugger* debugger )
{
	typedef TPair< CName, CEntity* > InternalData;

	TDynArray< InternalData > toRefreshBody;
	TDynArray< InternalData > toRefreshMimics;

	static Bool USE_BODY_IDLE = true;
	static Bool USE_MIMICS_IDLE = true;

	// 1. Detect changes
	const Uint32 size = collector.m_actorChangeState.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const StorySceneEventsCollector::ActorChangeState& evt = collector.m_actorChangeState[ i ];

		SCENE_ASSERT( evt.m_actorId != CName::NONE );

		if ( THandle< CEntity > entityH = FindActorByType( evt.m_actorId, AT_ACTOR | AT_PROP ) )
		{
			CEntity* entity = entityH.Get();
			SCENE_ASSERT( entity );

			if ( entity )
			{
				TActorAnimState* cachedState = FindOrCreateActorCachedData( entity, m_actorsAnimStates );
				SCENE_ASSERT( cachedState );
				SCENE_ASSERT( cachedState->m_id );

				Bool addToRefreshBody = false;
				Bool addToRefreshMimics = false;

				if ( evt.m_reset )
				{
					cachedState->m_data.m_statePrev = evt.m_state;
					cachedState->m_data.m_stateCurr = evt.m_state;
					cachedState->m_data.m_animationBodyNamePrev = CName::NONE;
					cachedState->m_data.m_animationBodyNameCurr = CName::NONE;
					cachedState->m_data.m_bodyWeight = 1.f;
					cachedState->m_data.m_animationMimicsEyesNamePrev = CName::NONE;
					cachedState->m_data.m_animationMimicsEyesNameCurr = CName::NONE;
					cachedState->m_data.m_animationMimicsPoseNamePrev = CName::NONE;
					cachedState->m_data.m_animationMimicsPoseNameCurr = CName::NONE;
					cachedState->m_data.m_animationMimicsAnimNamePrev = CName::NONE;
					cachedState->m_data.m_animationMimicsAnimNameCurr = CName::NONE;
					cachedState->m_data.m_mimicsWeight = 1.f;
					cachedState->m_data.m_mimicsPoseWeightPrev = 1.f;
					cachedState->m_data.m_mimicsPoseWeightCurr = evt.m_mimicsPoseWeight;
					cachedState->m_data.m_prevID = evt.m_ID;
					cachedState->m_data.m_currID = evt.m_ID;

					cachedState->m_data.m_prevMimicID = evt.m_ID;
					cachedState->m_data.m_currMimicID = evt.m_ID;

					addToRefreshBody = true;
					addToRefreshMimics = true;
				}
				else
				{
					#define CHECK_STATE_DIFF( x )			( evt.m_state.x && cachedState->m_data.m_stateCurr.x != evt.m_state.x )
					#define ADD_STATE_DIFF_TO_CURR( x )	if	( evt.m_state.x && cachedState->m_data.m_stateCurr.x != evt.m_state.x ) { cachedState->m_data.m_stateCurr.x = evt.m_state.x; }

					// Body
					if ( CHECK_STATE_DIFF( m_status ) || CHECK_STATE_DIFF( m_emotionalState ) || CHECK_STATE_DIFF( m_poseType ) )
					{
						cachedState->m_data.m_statePrev.CopyBodyData( cachedState->m_data.m_stateCurr );
						cachedState->m_data.m_animationBodyNamePrev = cachedState->m_data.m_animationBodyNameCurr;
						cachedState->m_data.m_prevID = cachedState->m_data.m_currID;
						cachedState->m_data.m_animationBodyNameCurr = CName::NONE;

						ADD_STATE_DIFF_TO_CURR( m_status );
						ADD_STATE_DIFF_TO_CURR( m_emotionalState );
						ADD_STATE_DIFF_TO_CURR( m_poseType );

						cachedState->m_data.m_currID = evt.m_ID;

						addToRefreshBody = true;
					}

					if ( evt.m_bodyBlendSet && cachedState->m_data.m_bodyWeight != evt.m_bodyBlendWeight )
					{
						cachedState->m_data.m_bodyWeight = evt.m_bodyBlendWeight;
						addToRefreshBody = true;
					}

					if ( evt.m_forceBodyIdleAnimation && cachedState->m_data.m_animationBodyNameCurr != evt.m_forceBodyIdleAnimation )
					{
						cachedState->m_data.m_statePrev.CopyBodyData( cachedState->m_data.m_stateCurr );
						cachedState->m_data.m_animationBodyNamePrev = cachedState->m_data.m_animationBodyNameCurr;
						cachedState->m_data.m_prevID = cachedState->m_data.m_currID;
						cachedState->m_data.m_animationBodyNameCurr = evt.m_forceBodyIdleAnimation;
						cachedState->m_data.m_currID = evt.m_ID;
						addToRefreshBody = true;
					}

					// Mimics
					if ( CHECK_STATE_DIFF( m_mimicsLayerEyes ) || CHECK_STATE_DIFF( m_mimicsLayerPose ) || CHECK_STATE_DIFF( m_mimicsLayerAnimation ) )
					{
						cachedState->m_data.m_statePrev.CopyMimicsData( cachedState->m_data.m_stateCurr );
						cachedState->m_data.m_animationMimicsEyesNamePrev = cachedState->m_data.m_animationMimicsEyesNameCurr;
						cachedState->m_data.m_animationMimicsEyesNameCurr = CName::NONE;
						cachedState->m_data.m_animationMimicsPoseNamePrev = cachedState->m_data.m_animationMimicsPoseNameCurr;
						cachedState->m_data.m_animationMimicsPoseNameCurr = CName::NONE;
						cachedState->m_data.m_animationMimicsAnimNamePrev = cachedState->m_data.m_animationMimicsAnimNameCurr;
						cachedState->m_data.m_animationMimicsAnimNameCurr = CName::NONE;
						cachedState->m_data.m_prevMimicID = cachedState->m_data.m_currMimicID;

						ADD_STATE_DIFF_TO_CURR( m_mimicsLayerEyes );
						ADD_STATE_DIFF_TO_CURR( m_mimicsLayerPose );
						ADD_STATE_DIFF_TO_CURR( m_mimicsLayerAnimation );

						cachedState->m_data.m_currMimicID = evt.m_ID;
						cachedState->m_data.m_mimicsPoseWeightPrev = cachedState->m_data.m_mimicsPoseWeightCurr;
						cachedState->m_data.m_mimicsPoseWeightCurr = evt.m_mimicsPoseWeight;

						addToRefreshMimics = true;
					}

					if ( evt.m_mimicsBlendSet && cachedState->m_data.m_mimicsPoseWeightCurr != evt.m_mimicsPoseWeight )
					{
						if ( !addToRefreshMimics )
						{
							cachedState->m_data.m_statePrev.CopyMimicsData( cachedState->m_data.m_stateCurr );

							cachedState->m_data.m_animationMimicsEyesNamePrev = cachedState->m_data.m_animationMimicsEyesNameCurr;
							cachedState->m_data.m_animationMimicsPoseNamePrev = cachedState->m_data.m_animationMimicsPoseNameCurr;
							cachedState->m_data.m_animationMimicsAnimNamePrev = cachedState->m_data.m_animationMimicsAnimNameCurr;

							cachedState->m_data.m_mimicsPoseWeightPrev = cachedState->m_data.m_mimicsPoseWeightCurr;
							cachedState->m_data.m_mimicsPoseWeightCurr = evt.m_mimicsPoseWeight;

							addToRefreshMimics = true;
						}
					}

					if ( evt.m_mimicsBlendSet && cachedState->m_data.m_mimicsWeight != evt.m_mimicsBlendWeight )
					{
						cachedState->m_data.m_mimicsWeight = evt.m_mimicsBlendWeight;
						addToRefreshMimics = true;
					}

					if ( evt.m_forceMimicsIdleEyesAnimation && cachedState->m_data.m_animationMimicsEyesNameCurr != evt.m_forceMimicsIdleEyesAnimation )
					{
						cachedState->m_data.m_animationMimicsEyesNameCurr = evt.m_forceMimicsIdleEyesAnimation;
						addToRefreshMimics = true;
					}
					if ( evt.m_forceMimicsIdlePoseAnimation && cachedState->m_data.m_animationMimicsPoseNameCurr != evt.m_forceMimicsIdlePoseAnimation )
					{
						cachedState->m_data.m_animationMimicsPoseNameCurr = evt.m_forceMimicsIdlePoseAnimation;
						addToRefreshMimics = true;
					}
					if ( evt.m_forceMimicsIdleAnimAnimation && cachedState->m_data.m_animationMimicsAnimNameCurr != evt.m_forceMimicsIdleAnimAnimation )
					{
						cachedState->m_data.m_animationMimicsAnimNameCurr = evt.m_forceMimicsIdleAnimAnimation;
						addToRefreshMimics = true;
					}

					#undef CHECK_STATE_DIFF
					#undef ADD_STATE_DIFF_TO_CURR
				}

				if ( addToRefreshBody && USE_BODY_IDLE )
				{
					toRefreshBody.PushBackUnique( InternalData( evt.m_actorId, entity ) );
				}

				if ( addToRefreshMimics && USE_MIMICS_IDLE )
				{
					toRefreshMimics.PushBackUnique( InternalData( evt.m_actorId, entity ) );
				}
			}
		}
	}

	// 2. Apply body pose changes
	const Uint32 numBody = toRefreshBody.Size();
	for ( Uint32 i=0; i<numBody; ++i )
	{
		const CName& id = toRefreshBody[ i ].m_first;
		CEntity* e = toRefreshBody[ i ].m_second;

		TActorAnimState* s = FindActorCachedData( e, m_actorsAnimStates );

		SCENE_ASSERT( s );
		SCENE_ASSERT( s->m_id && s->m_id == e );

		const CAnimatedComponent* ac = s->m_id->GetRootAnimatedComponent();

		if ( !s->m_data.m_animationBodyNamePrev )
		{
			CStorySceneAnimationList::IdleAndLookAtAnimationData queryData;
			GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList().RandBodyIdleAnimation( s->m_data.m_statePrev, queryData, ac );
			s->m_data.m_animationBodyNamePrev = queryData.m_idle;
			s->m_data.m_lookAtBodyAnimationPrev = queryData.m_lookAtBody;
			s->m_data.m_lookAtHeadAnimationPrev = queryData.m_lookAtHead;
		}
		if ( !s->m_data.m_animationBodyNameCurr )
		{
			CStorySceneAnimationList::IdleAndLookAtAnimationData queryData;
			GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList().RandBodyIdleAnimation( s->m_data.m_stateCurr, queryData, ac );
			s->m_data.m_animationBodyNameCurr = queryData.m_idle;
			s->m_data.m_lookAtBodyAnimationCurr = queryData.m_lookAtBody;
			s->m_data.m_lookAtHeadAnimationCurr = queryData.m_lookAtHead;
		}

		Bool randAnim = false;

		TActorMixer* mixer = FindActorBodyMixer( id, e );
		if ( !mixer )
		{
			mixer = CreateActorBodyMixer( id, s->m_id );
			randAnim = true;
		}

		TActorLookAt* lookAt = FindActorCachedData( e, m_actorsLookAts );
		if ( !lookAt )
		{
			lookAt = CreateActorCachedDataAndInit( e, m_actorsLookAts );
		}

		SCENE_ASSERT( mixer );
		SCENE_ASSERT( lookAt );

		if ( mixer->m_data.IsValid() )
		{
			SAnimationFullState animationStatePrev;
			animationStatePrev.m_motion = false;
			animationStatePrev.m_state.m_animation = s->m_data.m_animationBodyNamePrev;
			animationStatePrev.m_ID = s->m_data.m_prevID;

			SAnimationFullState animationStateCurr;
			animationStateCurr.m_motion = false;
			animationStateCurr.m_state.m_animation = s->m_data.m_animationBodyNameCurr;
			animationStateCurr.m_ID = s->m_data.m_currID;
			
			SCENE_ASSERT( IsAnimationLoaded_Body( s->m_data.m_lookAtBodyAnimationPrev, e, c ) );
			SCENE_ASSERT( IsAnimationLoaded_Body( s->m_data.m_lookAtHeadAnimationPrev, e, c ) );
			SCENE_ASSERT( IsAnimationLoaded_Body( s->m_data.m_lookAtBodyAnimationCurr, e, c ) );
			SCENE_ASSERT( IsAnimationLoaded_Body( s->m_data.m_lookAtHeadAnimationCurr, e, c ) );
			SCENE_ASSERT( IsAnimationLoaded_Body( animationStatePrev, e, c ) );
			SCENE_ASSERT( IsAnimationLoaded_Body( animationStateCurr, e, c ) );

			CAnimationMixerAnimSynchronizer sync;
			sync.SetSetterMode();
			mixer->m_data.SetIdleAnimationToSample( animationStatePrev, animationStateCurr, s->m_data.m_bodyWeight, true, &sync );
			sync.SetGetterMode();
			SetSyncItemIdleAnimToSample( id, animationStatePrev, animationStateCurr, s->m_data.m_bodyWeight, true, &sync );	

			lookAt->m_data.SyncWithIdle( s->m_data.m_lookAtBodyAnimationPrev, s->m_data.m_lookAtHeadAnimationPrev, s->m_data.m_lookAtBodyAnimationCurr, s->m_data.m_lookAtHeadAnimationCurr );

			//SCENE_LOG( TXT("Idle [%s], %1.3f, %s->%s"), id.AsChar(), s->m_data.m_bodyWeight, animationStatePrev.m_state.m_animation.AsChar(), animationStateCurr.m_state.m_animation.AsChar() );
		}

		if ( debugger )
		{
			debugger->OnExecutor_ChangeIdle( s->m_id, s->m_data.m_stateCurr );
		}
	}

	// 3. Apply mimics pose changes
	const Uint32 numMimics = toRefreshMimics.Size();
	for ( Uint32 i=0; i<numMimics; ++i )
	{
		const CName& id = toRefreshMimics[ i ].m_first;
		CEntity* e = toRefreshMimics[ i ].m_second;

		TActorAnimState* s = FindActorCachedData( e, m_actorsAnimStates );

		SCENE_ASSERT( s );
		SCENE_ASSERT( s->m_id && s->m_id == e );

		const CAnimatedComponent* ac = s->m_id->QueryActorInterface() ? s->m_id->QueryActorInterface()->GetMimicComponent() : nullptr;

		if ( !s->m_data.m_animationMimicsEyesNamePrev )
		{
			GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList().RandMimicsIdleAnimation( s->m_data.m_statePrev, s->m_data.m_animationMimicsEyesNamePrev, ac, CStorySceneAnimationList::LAYER_EYES );
		}
		if ( !s->m_data.m_animationMimicsEyesNameCurr )
		{
			GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList().RandMimicsIdleAnimation( s->m_data.m_stateCurr, s->m_data.m_animationMimicsEyesNameCurr, ac, CStorySceneAnimationList::LAYER_EYES );
		}
		if ( !s->m_data.m_animationMimicsPoseNamePrev )
		{
			GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList().RandMimicsIdleAnimation( s->m_data.m_statePrev, s->m_data.m_animationMimicsPoseNamePrev, ac, CStorySceneAnimationList::LAYER_POSE );
		}
		if ( !s->m_data.m_animationMimicsPoseNameCurr )
		{
			GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList().RandMimicsIdleAnimation( s->m_data.m_stateCurr, s->m_data.m_animationMimicsPoseNameCurr, ac, CStorySceneAnimationList::LAYER_POSE );
		}
		if ( !s->m_data.m_animationMimicsAnimNamePrev )
		{
			GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList().RandMimicsIdleAnimation( s->m_data.m_statePrev, s->m_data.m_animationMimicsAnimNamePrev, ac, CStorySceneAnimationList::LAYER_ANIMATION );
		}
		if ( !s->m_data.m_animationMimicsAnimNameCurr )
		{
			GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList().RandMimicsIdleAnimation( s->m_data.m_stateCurr, s->m_data.m_animationMimicsAnimNameCurr, ac, CStorySceneAnimationList::LAYER_ANIMATION );
		}

		TActorMixer* mixerEyes = FindActorCachedData( id, m_actorsMimicsMixers[ DML_Eyes ] );
		if ( !mixerEyes )
		{
			mixerEyes = CreateActorMimicsMixer( id, s->m_id, DML_Eyes );
		}
		TActorMixer* mixerPose = FindActorCachedData( id, m_actorsMimicsMixers[ DML_Pose ] );
		if ( !mixerPose )
		{
			mixerPose = CreateActorMimicsMixer( id, s->m_id, DML_Pose );
		}
		TActorMixer* mixerAnim = FindActorCachedData( id, m_actorsMimicsMixers[ DML_Animation ] );
		if ( !mixerAnim )
		{
			mixerAnim = CreateActorMimicsMixer( id, s->m_id, DML_Animation );
		}

		SCENE_ASSERT( mixerEyes );
		SCENE_ASSERT( mixerPose );
		SCENE_ASSERT( mixerAnim );

		CAnimationMixerAnimSynchronizer mimicsAnimSynchronizer;

		if ( mixerPose->m_data.IsValid() )
		{
			SAnimationFullState animationStatePrev;
			animationStatePrev.m_motion = false;
			animationStatePrev.m_state.m_animation = s->m_data.m_animationMimicsPoseNamePrev;
			animationStatePrev.m_weight = s->m_data.m_mimicsPoseWeightPrev;
			animationStatePrev.m_ID = s->m_data.m_prevMimicID;

			SAnimationFullState animationStateCurr;
			animationStateCurr.m_motion = false;
			animationStateCurr.m_state.m_animation = s->m_data.m_animationMimicsPoseNameCurr;
			animationStateCurr.m_weight = s->m_data.m_mimicsPoseWeightCurr;
			animationStateCurr.m_ID = s->m_data.m_currMimicID;

			SCENE_ASSERT( IsAnimationLoaded_Mimics( animationStatePrev, e, c ) );
			SCENE_ASSERT( IsAnimationLoaded_Mimics( animationStateCurr, e, c ) );

			mixerPose->m_data.SetIdleAnimationToSample( animationStatePrev, animationStateCurr, s->m_data.m_mimicsWeight, true );
		}
		if ( mixerAnim->m_data.IsValid() )
		{
			SAnimationFullState animationStatePrev;
			animationStatePrev.m_motion = false;
			animationStatePrev.m_state.m_animation = s->m_data.m_animationMimicsAnimNamePrev;
			animationStatePrev.m_ID = s->m_data.m_prevMimicID;

			SAnimationFullState animationStateCurr;
			animationStateCurr.m_motion = false;
			animationStateCurr.m_state.m_animation = s->m_data.m_animationMimicsAnimNameCurr;
			animationStateCurr.m_ID = s->m_data.m_currMimicID;

			mimicsAnimSynchronizer.SetSetterMode();

			SCENE_ASSERT( IsAnimationLoaded_Mimics( animationStatePrev, e, c ) );
			SCENE_ASSERT( IsAnimationLoaded_Mimics( animationStateCurr, e, c ) );

			mixerAnim->m_data.SetIdleAnimationToSample( animationStatePrev, animationStateCurr, s->m_data.m_mimicsWeight, true, &mimicsAnimSynchronizer );
		}
		if ( mixerEyes->m_data.IsValid() )
		{
			SAnimationFullState animationStatePrev;
			animationStatePrev.m_motion = false;
			animationStatePrev.m_state.m_animation = s->m_data.m_animationMimicsEyesNamePrev;
			animationStatePrev.m_ID = s->m_data.m_prevMimicID;

			SAnimationFullState animationStateCurr;
			animationStateCurr.m_motion = false;
			animationStateCurr.m_state.m_animation = s->m_data.m_animationMimicsEyesNameCurr;
			animationStateCurr.m_ID = s->m_data.m_currMimicID;

			mimicsAnimSynchronizer.SetGetterMode();

			SCENE_ASSERT( IsAnimationLoaded_Mimics( animationStatePrev, e, c ) );
			SCENE_ASSERT( IsAnimationLoaded_Mimics( animationStateCurr, e, c ) );

			mixerEyes->m_data.SetIdleAnimationToSample( animationStatePrev, animationStateCurr, s->m_data.m_mimicsWeight, true, &mimicsAnimSynchronizer );
		}

		if ( debugger )
		{
			debugger->OnExecutor_ChangeIdle( s->m_id, s->m_data.m_stateCurr );
		}
	}
}

void CStorySceneEventsExecutor::OpenMixers( TActorsMixers& mixers )
{
	const Uint32 size = mixers.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		TActorMixer& mixer = mixers[ i ];
		if ( mixer.m_data.IsValid() )
		{
			mixer.m_data.OpenMixer();
		}
	}
}

void CStorySceneEventsExecutor::CloseMixers( TActorsMixers& mixers )
{
	const Uint32 size = mixers.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		TActorMixer& mixer = mixers[ i ];
		if ( mixer.m_data.IsValid() )
		{
			mixer.m_data.CloseMixer();
		}
	}
}

void CStorySceneEventsExecutor::ProcessBodyAnimations( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, const CStorySceneAnimationContainer& c, IStorySceneDebugger* debugger, TActorSignalsDisableLookAtBLP& signalsDisableLookAtBLP )
{
	OpenMixers( m_actorsBodyMixers );
	OpenMixers( m_actorsGmplBodyMixers );
	OpenSyncItemMixer( m_syncItem.m_parentActor );

	// Add animations
	{
		const Uint32 size = collector.m_bodyAnimations.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const StorySceneEventsCollector::BodyAnimation& evt = collector.m_bodyAnimations[ i ];

			SCENE_ASSERT( evt.m_actorId != CName::NONE );
			SCENE_ASSERT( evt.m_event );
			SCENE_ASSERT( evt.m_weight >= 0.f || evt.m_weight <= 1.f );

			if ( THandle< CEntity > entityH = FindActorByType( evt.m_actorId, AT_ACTOR | AT_PROP ) )
			{
				CEntity* entity = entityH.Get();
				SCENE_ASSERT( entity );

				if ( entity )
				{
					TActorMixer* mixer = FindActorBodyMixer( evt.m_actorId, entity );
					if ( !mixer )
					{
						// Add state and open new mixer
						mixer = CreateActorBodyMixer( evt.m_actorId, entity );

						mixer->m_data.OpenMixer();
						OpenSyncItemMixer( evt.m_actorId );
					}
					SAnimationFullState anim;
					anim.m_motion = false;
					anim.m_fakeMotion = evt.m_useFakeMotion;
					anim.m_gatherSyncTokens = evt.m_gatherSyncTokens;
					anim.m_muteSoundEvents = evt.m_muteSoundEvents;
					anim.m_weight = evt.m_weight;
					anim.m_state = evt.m_animationState;
					anim.m_additiveType = evt.m_additiveType;
					anim.m_convertToAdditive = evt.m_convertToAdditive;
					anim.m_bonesIdx = evt.m_bonesIdx;
					anim.m_bonesWeight = evt.m_bonesWeight;
					anim.m_ID = evt.m_ID;
					anim.m_allowPoseCorrection = evt.m_allowPoseCorrection;

#ifndef NO_EDITOR
					anim.m_muteSoundEvents = anim.m_muteSoundEvents || !player->ShouldPlaySounds();
#endif

					if ( !evt.m_useLowerBodyPartsForLookAt )
					{
						signalsDisableLookAtBLP.Add( entity, anim.m_weight );
					}

					SCENE_ASSERT( IsAnimationLoaded_Body( anim, entity, c ) );

					if ( evt.m_type == EAT_Normal )
					{
						mixer->m_data.AddAnimationToSample( anim );
					}
					else if ( evt.m_type == EAT_Additive )
					{
						mixer->m_data.AddAdditiveAnimationToSample( anim );
					}
					else if ( evt.m_type == EAT_Override )
					{
						mixer->m_data.AddOverrideAnimationToSample( anim );
					}
					else
					{
						SCENE_ASSERT( 0 );
					}
					AddSyncItemAnimToSample( evt.m_actorId, anim, evt.m_type );
				}
			}
			else
			{
				SCENE_ASSERT( 0 );
			}
		}
	}

	// Add poses
	{
		const Uint32 size = collector.m_bodyPoses.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const StorySceneEventsCollector::BodyPose& evt = collector.m_bodyPoses[ i ];

			SCENE_ASSERT( evt.m_actorId != CName::NONE );
			if ( evt.m_poseId == 0 ) // Reset - see flow ctrl, only for dialogset correction pose
			{
				SCENE_ASSERT( !evt.m_event );
				SCENE_ASSERT( evt.m_reset );
			}
			else
			{
				SCENE_ASSERT( evt.m_event );
			}
			SCENE_ASSERT( evt.m_weight >= 0.f || evt.m_weight <= 1.f );

			if ( THandle< CEntity > entityH = FindActorByType( evt.m_actorId, AT_ACTOR | AT_PROP ) )
			{
				CEntity* entity = entityH.Get();
				SCENE_ASSERT( entity );

				if ( entity )
				{
					ESAnimationMappedPoseMode poseMode = AMPM_Additive;
					//...

					TActorMixer* mixer = FindActorBodyMixer( evt.m_actorId, entity );
					if ( !mixer )
					{
						// Add state and open new mixer
						mixer = CreateActorBodyMixer( evt.m_actorId, entity );
						SCENE_ASSERT( mixer->m_data.IsValid() );

						mixer->m_data.OpenMixer();
						OpenSyncItemMixer( evt.m_actorId );
					}

					SCENE_ASSERT( mixer->m_data.IsValid() );

					if ( evt.m_reset )
					{
						mixer->m_data.RemoveAllPoses();
						RemoveAllSyncItemPoses( evt.m_actorId );
					}
					else
					{
						if ( evt.m_enable )
						{
							SAnimationMappedPose pose;
							pose.m_weight = evt.m_weight;
							pose.m_mode = poseMode;
							pose.m_bonesMapping = evt.m_boneIndices;
							pose.m_bones = evt.m_boneTransforms;

							if ( evt.m_linkToDialogset )
							{
								SCENE_ASSERT( !evt.m_linkToChangePose );
								SCENE_ASSERT( evt.m_correctionID.IsZero() );

								const CStorySceneDialogsetInstance* dialogset = evt.m_linkToDialogsetPtr;
								SCENE_ASSERT( dialogset );
								if ( dialogset )
								{
									const CStorySceneDialogsetSlot* slot = dialogset->GetSlotByActorName( evt.m_actorId );
									SCENE_ASSERT( slot );
									if ( slot )
									{
										const CName& slotAnim = slot->GetForceBodyIdleAnimation();
										if ( slotAnim )
										{
											pose.m_correctionIdleID = slotAnim;
										}
										else
										{
											const CAnimatedComponent* ac = entity->GetRootAnimatedComponent();

											SStorySceneActorAnimationState state;
											state.m_status = slot->GetBodyFilterStatus();
											state.m_emotionalState = slot->GetBodyFilterEmotionalState();
											state.m_poseType = slot->GetBodyFilterPoseName();

											CStorySceneAnimationList::IdleAndLookAtAnimationData queryData;

											GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList().RandBodyIdleAnimation( state, queryData, ac );

											pose.m_correctionIdleID = queryData.m_idle;
										}
									}
								}
							}
							else if ( evt.m_linkToChangePose )
							{
								SCENE_ASSERT( evt.m_correctionID.IsZero() );
								SCENE_ASSERT( ARRAY_COUNT( evt.m_linkToChangePoseState ) == 4 );

								const CName& forceAnim = evt.m_linkToChangePoseState[ 3 ];
								if ( forceAnim )
								{
									pose.m_correctionIdleID = forceAnim;
								}
								else
								{
									const CAnimatedComponent* ac = entity->GetRootAnimatedComponent();

									SStorySceneActorAnimationState state;
									state.m_status = evt.m_linkToChangePoseState[ 0 ];
									state.m_emotionalState = evt.m_linkToChangePoseState[ 1 ];
									state.m_poseType = evt.m_linkToChangePoseState[ 2 ];

									CStorySceneAnimationList::IdleAndLookAtAnimationData queryData;

									GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList().RandBodyIdleAnimation( state, queryData, ac );

									pose.m_correctionIdleID = queryData.m_idle;
								}
							}
							else
							{
								pose.m_correctionID = evt.m_correctionID;
							}

							mixer->m_data.AddPoseToSample( evt.m_poseId, pose );
							AddSyncItemPoseToSample( evt.m_actorId, evt.m_poseId, pose );

						}
						else
						{
							mixer->m_data.RemovePose( evt.m_poseId );
							RemoveSyncItemPose( evt.m_actorId, evt.m_poseId );
						}
					}
				}
			}
			else
			{
				SCENE_ASSERT( 0 );
			}
		}
	}

	CloseMixers( m_actorsBodyMixers );
	CloseMixers( m_actorsGmplBodyMixers );
	CloseSyncItemMixers();
}

void CStorySceneEventsExecutor::ProcessMimicsAnimations( const CStorySceneEventsCollector& collector, const CStorySceneAnimationContainer& c, IStorySceneDebugger* debugger )
{
	OpenMixers( m_actorsMimicsMixers[DML_Animation] );
	OpenMixers( m_actorsMimicsMixers[DML_Pose] );

	// Add animations
	{
		const Uint32 size = collector.m_mimicsAnimations.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const StorySceneEventsCollector::MimicsAnimation& evt = collector.m_mimicsAnimations[ i ];

			SCENE_ASSERT( evt.m_actorId != CName::NONE );
			SCENE_ASSERT( evt.m_event );
			SCENE_ASSERT( evt.m_weight >= 0.f || evt.m_weight <= 1.f );

			if ( THandle< CEntity > entityH = FindActorByType( evt.m_actorId, AT_ACTOR | AT_PROP ) )
			{
				CEntity* entity = entityH.Get();
				SCENE_ASSERT( entity );

				if ( entity )
				{
					TActorMixer* mixer = FindActorCachedData( evt.m_actorId, m_actorsMimicsMixers[ DML_Animation ] );
					if ( !mixer )
					{
						// Add state and open new mixer
						mixer = CreateActorMimicsMixer( evt.m_actorId, entity, DML_Animation );

						mixer->m_data.OpenMixer();
					}

					SAnimationFullState anim;
					anim.m_motion = false;
					anim.m_weight = evt.m_weight;
					anim.m_state = evt.m_animationState;
					anim.m_ID = evt.m_ID;
					anim.m_fullEyesWeight = evt.m_fullEyesWeight;
					anim.m_allowPoseCorrection = evt.m_allowPoseCorrection;

					SCENE_ASSERT( IsAnimationLoaded_Mimics( anim, entity, c ) );

					mixer->m_data.AddAnimationToSample( anim );
				}
			}
			else
			{
				SCENE_ASSERT( 0 );
			}
		}
	}

	// Add poses
	{
		const Uint32 size = collector.m_mimicsPoses.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const StorySceneEventsCollector::MimicPose& evt = collector.m_mimicsPoses[ i ];

			if ( THandle< CEntity > entityH = FindActorByType( evt.m_actorId, AT_ACTOR | AT_PROP ) )
			{
				CEntity* entity = entityH.Get();
				SCENE_ASSERT( entity );

				if ( entity )
				{
					const CAnimatedComponent* ac = entity->QueryActorInterface() ? entity->QueryActorInterface()->GetMimicComponent() : nullptr;

					const EDialogMimicsLayer layer = evt.m_linkToDialogset || evt.m_linkToChangePose ? DML_Pose : DML_Animation;
					const CName layerName = layer == DML_Pose ? CStorySceneAnimationList::LAYER_POSE : CStorySceneAnimationList::LAYER_ANIMATION;

					ESAnimationMappedPoseMode poseMode = AMPM_Additive;

					TActorMixer* mixer = FindActorCachedData( evt.m_actorId, m_actorsMimicsMixers[ layer ] );
					if ( !mixer )
					{
						// Add state and open new mixer
						mixer = CreateActorMimicsMixer( evt.m_actorId, entity, layer );

						mixer->m_data.OpenMixer();
					}

					SCENE_ASSERT( mixer->m_data.IsValid() );

					if ( evt.m_reset )
					{
						mixer->m_data.RemoveAllPoses();
					}
					else
					{
						if ( evt.m_enable )
						{
							SAnimationMappedPose pose;
							pose.m_weight = evt.m_weight;
							pose.m_mode = poseMode;
							pose.m_tracksMapping = evt.m_trackIndices;
							pose.m_tracks = evt.m_trackValues;

							if ( evt.m_linkToDialogset )
							{
								SCENE_ASSERT( !evt.m_linkToChangePose );
								SCENE_ASSERT( evt.m_correctionID.IsZero() );

								const CStorySceneDialogsetInstance* dialogset = evt.m_linkToDialogsetPtr;
								SCENE_ASSERT( dialogset );
								if ( dialogset )
								{
									const CStorySceneDialogsetSlot* slot = dialogset->GetSlotByActorName( evt.m_actorId );
									SCENE_ASSERT( slot );
									if ( slot )
									{
										//const CName& slotAnim = slot->GetForceBodyIdleAnimation();
										//if ( slotAnim )
										//{
										//	pose.m_correctionIdleID = slotAnim;
										//}
										//else
										{
											SStorySceneActorAnimationState state;
											slot->GetMimicsLayers( state.m_mimicsLayerEyes, state.m_mimicsLayerPose, state.m_mimicsLayerAnimation );

											CName animName;
											GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList().RandMimicsIdleAnimation( state, animName, ac, layerName );

											pose.m_correctionIdleID = animName;
										}
									}
								}
							}
							else if ( evt.m_linkToChangePose )
							{
								SCENE_ASSERT( evt.m_correctionID.IsZero() );
								SCENE_ASSERT( ARRAY_COUNT( evt.m_linkToChangePoseState ) == 4 );

								//const CName& forceAnim = evt.m_linkToChangePoseState[ 3 ];
								//if ( forceAnim )
								//{
								//	pose.m_correctionIdleID = forceAnim;
								//}
								//else
								{
									SStorySceneActorAnimationState state;
									state.m_mimicsLayerEyes = evt.m_linkToChangePoseState[ 0 ];
									state.m_mimicsLayerPose = evt.m_linkToChangePoseState[ 1 ];
									state.m_mimicsLayerAnimation = evt.m_linkToChangePoseState[ 2 ];
									SCENE_ASSERT( evt.m_linkToChangePoseState[ 3 ] == CName::NONE );

									CName animName;
									GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList().RandMimicsIdleAnimation( state, animName, ac, layerName );

									pose.m_correctionIdleID = animName;
								}
							}
							else
							{
								pose.m_correctionID = evt.m_correctionID;
							}

							mixer->m_data.AddPoseToSample( evt.m_poseId, pose );
						}
						else
						{
							mixer->m_data.RemovePose( evt.m_poseId );
						}
					}
				}
			}
			else
			{
				SCENE_ASSERT( 0 );
			}
		}
	}

	CloseMixers( m_actorsMimicsMixers[DML_Animation] );
	CloseMixers( m_actorsMimicsMixers[DML_Pose] );
}

void CStorySceneEventsExecutor::ProcessActorsPrepareToAndFinishTalk( const CStorySceneEventsCollector& collector )
{
	{
		const Uint32 size = collector.m_actorPrepareToTalk.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const StorySceneEventsCollector::ActorPrepareToTalk& evt = collector.m_actorPrepareToTalk[ i ];
			if ( evt.m_actorId )
			{
				if ( THandle< CEntity > entityH = FindActorByType( evt.m_actorId, AT_ACTOR ) )
				{
					CEntity* entity = entityH.Get();
					SCENE_ASSERT( entity );

					if ( CActor* a = Cast< CActor >( entity ) )
					{
						if ( CMimicComponent* m = a->GetMimicComponent() )
						{
							if ( CBehaviorGraphStack* stack =  m->GetBehaviorStack() )
							{
								stack->GenerateBehaviorEvent( CNAME( PrepareToTalk ) );
							}
						}
					}
				}
			}
		}
	}

	{
		const Uint32 size = collector.m_actorFinishTalk.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const StorySceneEventsCollector::ActorFinishTalk& evt = collector.m_actorFinishTalk[ i ];
			if ( evt.m_actorId )
			{
				if ( THandle< CEntity > entityH = FindActorByType( evt.m_actorId, AT_ACTOR ) )
				{
					CEntity* entity = entityH.Get();
					SCENE_ASSERT( entity );

					if ( CActor* a = Cast< CActor >( entity ) )
					{
						if ( CMimicComponent* m = a->GetMimicComponent() )
						{
							if ( CBehaviorGraphStack* stack =  m->GetBehaviorStack() )
							{
								stack->GenerateBehaviorEvent( CNAME( FinishTalk ) );
							}
						}
					}
				}
			}
		}
	}
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
