#include "build.h"
#include "storySceneEventsExecutor.h"
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
#include "../engine/behaviorGraphUtils.inl"
#include "../engine/morphedMeshComponent.h"
#include "../engine/clothComponent.h"
#include "../engine/animDangleComponent.h"
#include "../engine/behaviorGraphStack.h"
#include "../engine/behaviorGraphAnimationSlotNode.h"
#include "../engine/behaviorGraphAnimationManualSlot.h"
#include "itemIterator.h"
#include "../engine/camera.h"
#include "../engine/mimicComponent.h"
#include "gameWorld.h"


#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

void CStorySceneEventsExecutor::ProcessActorsLookAts_ResampleHeadBone( CEntity* target, Int32 headIdx, CStorySceneEventsExecutor::TActorMixer* mixerTarget, const TActorSignalsMixerPrevState& signalsMixerPrevState )
{
	SCENE_ASSERT( mixerTarget );
	SCENE_ASSERT( target );

	SAnimationMixerVisualState mixerVState_Prev;
	if ( signalsMixerPrevState.Get( target, mixerVState_Prev ) && mixerTarget && target )
	{
		SAnimationMixerVisualState mixerVState_Curr;
		mixerTarget->m_data.GetState( mixerVState_Curr );

		if ( SAnimationMixerVisualState::IsNotEqual( mixerVState_Prev, mixerVState_Curr ) )
		{
			if ( CAnimatedComponent* ac = target->GetRootAnimatedComponent() )
			{
				if ( const CSkeleton* sk = ac->GetSkeleton() )
				{
					const Uint32 numBones = sk->GetBonesNum();
					const Uint32 numTracks = sk->GetTracksNum();

					SCENE_ASSERT( numBones > 0 );
					SCENE_ASSERT( headIdx != -1 );

					if ( numBones > 0 && numTracks == 0 && numBones < 256 && headIdx != -1 && headIdx < (Int32)numBones )
					{
						TAllocArrayCreate( AnimQsTransform, bonesBuf, numBones );
						SBehaviorGraphOutput pose;
						SBehaviorGraphOutputParameter param =
						{
							numBones,
							0,
							bonesBuf.TypedData(),
							nullptr, 
							nullptr,
							true
						};

						pose.Init( param );

						Matrix boneMS;
						if ( mixerTarget->m_data.ResamplePose( pose, headIdx, boneMS ) )
						{
							ac->SetThisFrameTempBoneModelSpace( headIdx, boneMS );
						}
					}
				}
			}
		}
	}
}

void CStorySceneEventsExecutor::ProcessActorsLookAts_Pre( const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger, TActorSignalsMixerPrevState& signalsMixerPrevState )
{
	// New look at system
	{
		// Update look at's data
		const Uint32 size = collector.m_actorLookAts.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const StorySceneEventsCollector::ActorLookAt& evt = collector.m_actorLookAts[ i ];
			if ( evt.m_bodyInstant )
			{
				CEntity* targetBody = nullptr;

				if ( evt.m_type != DLT_StaticPoint && evt.m_bodyTarget )
				{
					targetBody = FindActorByType( evt.m_bodyTarget, AT_ACTOR ).Get();
				}

				if ( CActor* targetBodyActor = Cast< CActor >( targetBody ) )
				{
					SAnimationMixerVisualState mixerVState;
					if ( !signalsMixerPrevState.Get( targetBodyActor, mixerVState ) )
					{
						TActorMixer* mixerTarget = FindActorBodyMixer( targetBodyActor->GetVoiceTag(), targetBodyActor );
						if ( mixerTarget )
						{
							mixerTarget->m_data.GetState( mixerVState );
							signalsMixerPrevState.Set( targetBodyActor, mixerVState );
						}
					}
				}
			}
			
			if ( evt.m_eyesInstant )
			{
				CEntity* targetEyes = nullptr;

				if ( evt.m_type != DLT_StaticPoint && evt.m_eyesTarget )
				{
					targetEyes = FindActorByType( evt.m_eyesTarget, AT_ACTOR ).Get();
				}

				if ( CActor* targetEyesActor = Cast< CActor >( targetEyes ) )
				{
					SAnimationMixerVisualState mixerVState;
					if ( !signalsMixerPrevState.Get( targetEyesActor, mixerVState ) )
					{
						TActorMixer* mixerTarget = FindActorBodyMixer( targetEyesActor->GetVoiceTag(), targetEyesActor );
						if ( mixerTarget )
						{
							mixerTarget->m_data.GetState( mixerVState );
							signalsMixerPrevState.Set( targetEyesActor, mixerVState );
						}
					}
				}
			}
		}
	}

	// Old look at system
	{
		const Uint32 size = collector.m_actorLookAtTicks.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const StorySceneEventsCollector::ActorLookAtTick& evt = collector.m_actorLookAtTicks[ i ];
			SCENE_ASSERT( evt.m_actorId != CName::NONE );

			if ( evt.m_enable && evt.m_infoA.IsInstant() )
			{
				if ( evt.m_type == DLT_Dynamic )
				{
					SCENE_ASSERT( evt.m_infoA.m_targetOwner );
					CEntity* target = evt.m_infoA.m_targetOwner->GetEntity();
					if ( CActor* targetActor = Cast< CActor >( target ) )
					{
						SAnimationMixerVisualState mixerVState;
						if ( !signalsMixerPrevState.Get( target, mixerVState ) )
						{
							TActorMixer* mixerTarget = FindActorBodyMixer( targetActor->GetVoiceTag(), target );
							if ( mixerTarget )
							{
								mixerTarget->m_data.GetState( mixerVState );
								signalsMixerPrevState.Set( target, mixerVState );
							}
						}
					}
				}
				else if ( evt.m_type == DLT_Static )
				{
					// TODO
					//...
				}
			}
		}
	}
}

void CStorySceneEventsExecutor::ProcessActorsLookAts_Post( const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger, const TActorSignalsDisableLookAtBLP& signalsDisableLookAtBLP, const TActorSignalsMixerPrevState& signalsMixerPrevState )
{
	struct BlinkData
	{
		TActorLookAt*									m_lookAt;
		const StorySceneEventsCollector::ActorLookAt*	m_evt;
		Bool											m_isRequested;
	};
	TDynArray< BlinkData > blinksToDo;
	blinksToDo.Reserve( collector.m_actorLookAts.Size() );

	// New look at system - first pass
	{
		// Update look at's data
		const Uint32 size = collector.m_actorLookAts.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const StorySceneEventsCollector::ActorLookAt& evt = collector.m_actorLookAts[ i ];

			SCENE_ASSERT( evt.m_actorId != CName::NONE );

			if ( CEntity* entity = FindActorByType( evt.m_actorId, AT_ACTOR ).Get() )
			{
				TActorLookAt* lookAtData = FindActorCachedData( entity, m_actorsLookAts );
				if ( !lookAtData )
				{
					lookAtData = CreateActorCachedDataAndInit( entity, m_actorsLookAts );
				}

				SCENE_ASSERT( lookAtData );

				if ( lookAtData )
				{
					CEntity* targetBody = nullptr;
					CEntity* targetEyes = nullptr;

					if ( evt.m_type != DLT_StaticPoint && evt.m_bodyTarget )
					{
						targetBody = FindActorByType( evt.m_bodyTarget, AT_ACTOR ).Get();
					}
					if ( evt.m_type != DLT_StaticPoint && evt.m_eyesTarget )
					{
						targetEyes = FindActorByType( evt.m_eyesTarget, AT_ACTOR ).Get();
					}

					if ( evt.m_bodyInstant && targetBody )
					{
						if ( CActor* targetBodyActor = Cast< CActor >( targetBody ) ) // TODO
						{
							TActorMixer* mixerTarget = FindActorBodyMixer( targetBodyActor->GetVoiceTag(), targetBodyActor );
							const Int32 headBoneIdx = targetBodyActor->GetHeadBone();
							if ( mixerTarget && headBoneIdx != -1 )
							{
								ProcessActorsLookAts_ResampleHeadBone( targetBodyActor, headBoneIdx, mixerTarget, signalsMixerPrevState );
							}
						}
					}
					if ( evt.m_eyesInstant && targetEyes )
					{
						if ( CActor* targetEyesActor = Cast< CActor >( targetEyes ) ) // TODO
						{
							TActorMixer* mixerTarget = FindActorBodyMixer( targetEyesActor->GetVoiceTag(), targetEyesActor );
							const Int32 headBoneIdx = targetEyesActor->GetHeadBone();
							if ( mixerTarget && headBoneIdx != -1 )
							{
								ProcessActorsLookAts_ResampleHeadBone( targetEyesActor, headBoneIdx, mixerTarget, signalsMixerPrevState );
							}
						}
					}

					const Vector bodyStaticPointWS = StorySceneUtils::CalcWSFromSS( evt.m_bodyStaticPointWS, evt.m_sceneTransformWS );

					const Bool targetChanged = lookAtData->m_data.SetPointCloud( evt, targetBody, targetEyes, bodyStaticPointWS );
					if ( targetChanged )
					{
						lookAtData->m_data.UnblockBlink();
						lookAtData->m_data.NotifyTargetWasChanged();
					}

					BlinkData blinkData;
					blinkData.m_evt = &evt;
					blinkData.m_lookAt = lookAtData;
					blinkData.m_isRequested = targetChanged;
					blinksToDo.PushBack( blinkData );

					Float disableLowerBodyPart( 0.f );
					if ( signalsDisableLookAtBLP.Get( entity, disableLowerBodyPart ) && disableLowerBodyPart > 0.f )
					{
						lookAtData->m_data.SetLowerBodyPartsWeight( 1.f-disableLowerBodyPart );
					}
					else
					{
						lookAtData->m_data.SetLowerBodyPartsWeight( 1.f );
					}
				}
			}
		}
	}

	// Old look at system
	{
		const Uint32 size = collector.m_actorLookAtTicks.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const StorySceneEventsCollector::ActorLookAtTick& evt = collector.m_actorLookAtTicks[ i ];		
			SCENE_ASSERT( evt.m_actorId != CName::NONE );	

			CEntity* entity = FindActorByType( evt.m_actorId, AT_ACTOR ).Get();	
			SCENE_ASSERT( entity );
			if ( CActor* actor = Cast< CActor >( entity ) )
			{
				TActorLookAt* lookAtData = FindActorCachedData( entity, m_actorsLookAts );
				if ( !lookAtData )
				{
					lookAtData = CreateActorCachedDataAndInit( entity, m_actorsLookAts );
				}

				SCENE_ASSERT( lookAtData );

				if ( lookAtData )
				{
					if ( evt.m_enable )
					{
						const Matrix& actorL2W = actor->GetRootAnimatedComponent() ? actor->GetRootAnimatedComponent()->GetThisFrameTempLocalToWorld() : actor->GetLocalToWorld();

						if ( evt.m_type == DLT_Dynamic )
						{
							actor->EnableLookAt( evt.m_infoA );

							if ( evt.m_infoA.GetLevel() < LL_Eyes )
							{
								lookAtData->m_data.NotifyTargetWasChanged();
							}

							if ( evt.m_infoA.IsInstant() )
							{
								SCENE_ASSERT( evt.m_infoA.m_targetOwner );
								CEntity* target = evt.m_infoA.m_targetOwner->GetEntity();
								if ( CActor* targetActor = Cast< CActor >( target ) ) // TODO
								{
									TActorMixer* mixerTarget = FindActorBodyMixer( targetActor->GetVoiceTag(), target );

									if ( mixerTarget && target )
									{
										ProcessActorsLookAts_ResampleHeadBone( target, evt.m_infoA.m_boneIndex, mixerTarget, signalsMixerPrevState );
									}
								}
							}
						}
						else if ( evt.m_type == DLT_Static )
						{	
							SCENE_ASSERT( evt.m_targetId );
							THandle< CEntity > targetH = FindActorByType( evt.m_actorId, AT_ACTOR );
							if ( CActor* target =  Cast< CActor >( targetH.Get() ) )
							{
								SLookAtDialogStaticInfo info( evt.m_infoB );
								info.m_target = CStorySceneActorsEyesTracker::GetActorEyePosWS( target );
								actor->EnableLookAt( info );
								if ( evt.m_infoA.GetLevel() < LL_Eyes )
								{
									lookAtData->m_data.NotifyTargetWasChanged();
								}
							}
						}
						else if( evt.m_type == DLT_StaticPoint )
						{
							if ( evt.m_isSetByNewLookat )
							{
								const Vector staticPointWS = StorySceneUtils::CalcWSFromSS( evt.m_staticPoint, evt.m_sceneTransformWS );

								const Matrix actorW2L = actorL2W.FullInverted();
								const Vector targetMS = actorW2L.TransformPoint( staticPointWS );

								SLookAtDialogStaticInfo info( evt.m_infoB );
								info.m_target = staticPointWS;
								actor->EnableLookAt( info );
								if ( evt.m_infoA.GetLevel() < LL_Eyes )
								{
									lookAtData->m_data.NotifyTargetWasChanged();
								}
							}
							else
							{
								SLookAtDialogStaticInfo info( evt.m_infoB );
								info.m_target = actorL2W.TransformPoint( evt.m_staticPoint );
								actor->EnableLookAt( info );
								if ( evt.m_infoA.GetLevel() < LL_Eyes )
								{
									lookAtData->m_data.NotifyTargetWasChanged();
								}
							}
						}
					}
					else
					{
						actor->DisableDialogsLookAts( evt.m_disableSpeed );
					}
				}
			}
		}
	}

	// Gameplay look at system
	{
		const Uint32 size = collector.m_actorLookAtGameplay.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const StorySceneEventsCollector::ActorGameplayLookAt& evt = collector.m_actorLookAtGameplay[ i ];		
			if ( evt.m_actorId )
			{
				THandle< CEntity > entityH = FindActorByType( evt.m_actorId, AT_ACTOR | AT_PROP );
				if ( CEntity* entity = entityH.Get() )
				{
					TActorLookAt* lookAtData = FindActorCachedData( entity, m_actorsLookAts );
					if ( !lookAtData )
					{
						lookAtData = CreateActorCachedDataAndInit( entity, m_actorsLookAts );
					}

					SCENE_ASSERT( lookAtData );

					if ( lookAtData )
					{
						THandle< CEntity > targetH;
						if ( evt.m_type != DLT_StaticPoint && evt.m_target )
						{
							targetH = FindActorByType( evt.m_target, AT_ACTOR | AT_PROP );
						}

						const Vector bodyStaticPointWS = StorySceneUtils::CalcWSFromSS( evt.m_staticPointSS, evt.m_sceneTransformWS );

						lookAtData->m_data.SetGameplay( evt, targetH.Get(), bodyStaticPointWS );
					}
				}
			}
		}
	}

	// Update look ats
	const Uint32 numLookAts = m_actorsLookAts.Size();
	for ( Uint32 i=0; i<numLookAts; ++i )
	{
		TActorLookAt& l = m_actorsLookAts[ i ];
		l.m_data.Update();
	}

	// New look at system - second pass
	{
		const Uint32 size = blinksToDo.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			BlinkData& b = blinksToDo[ i ];

			if ( b.m_isRequested )
			{
				const Float horDeg = b.m_lookAt->m_data.GetApproximateHorAngleDegToTargetLS();
				const Float horDegAbs = MAbs( horDeg );

				if ( b.m_evt->m_blinkHorAngleDeg > 0.f && horDegAbs < b.m_evt->m_blinkHorAngleDeg )
				{
					b.m_lookAt->m_data.BlockBlink( b.m_evt->m_id );
				}
			}

			if (  b.m_lookAt->m_data.IsBlinkBlocked( b.m_evt->m_id ) )
			{
				TActorMixer* mixer = FindActorCachedData( b.m_evt->m_actorId, m_actorsMimicsMixers[ DML_Animation ] );
				SCENE_ASSERT( mixer );
				if ( mixer )
				{
					const Bool ret = mixer->m_data.RemoveAnimation( b.m_evt->m_id );
					SCENE_ASSERT( ret );
				}
			}
		}
	}
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
