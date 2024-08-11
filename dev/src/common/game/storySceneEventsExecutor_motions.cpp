
#include "build.h"
#include "storySceneEventsExecutor.h"
#include "storySceneEventsCollector.h"
#include "storySceneEventsCollector_events.h"
#include "storySceneDebugger.h"
#include "storySceneEventMimicFilter.h"
#include "storySceneEventMimicPose.h"
#include "storyScenePlayer.h"
#include "storySceneLine.h"
#include "storySceneVoicetagMapping.h"
#include "actorSpeech.h"
#include "lookAtTypes.h"
#include "storySceneLookAtController.h"
#include "storySceneUtils.h"
#include "storySceneSystem.h"
#include "storySceneMotionExtractionSampler.h"
#include "storySceneCutscene.h"
#include "gameWorld.h"
#include "../engine/cutsceneInstance.h"
#include "../engine/behaviorGraphUtils.inl"
#include "../engine/morphedMeshComponent.h"
#include "../engine/clothComponent.h"
#include "../engine/behaviorGraphStack.h"
#include "../engine/behaviorGraphAnimationSlotNode.h"
#include "../engine/behaviorGraphAnimationManualSlot.h"
#include "movingPhysicalAgentComponent.h"
#include "../engine/visualDebug.h"
#include "../engine/mimicComponent.h"
#include "../engine/camera.h"
#include "movableRepresentationPathAgent.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

RED_DEFINE_STATIC_NAME( BehGraphIntToName )
RED_DEFINE_STATIC_NAME( DoStorySceneGameplayAction )
RED_DEFINE_STATIC_NAME( DoesStorySceneGameplayActionChangeItems )
RED_DEFINE_STATIC_NAME( StoryScene2 )

Bool CStorySceneEventsExecutor::FillActorsPlacements( const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger )
{
	const Uint32 size = collector.m_actorPlacement.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const StorySceneEventsCollector::ActorPlacement& evt = collector.m_actorPlacement[ i ];

		SCENE_ASSERT( evt.m_actorId != CName::NONE );

		THandle< CEntity > entityH;
		if ( CEntity* entity = FindActorByType( evt.m_actorId, AT_ACTOR ).Get() )
		{			
			Matrix finalPositionWS = StorySceneUtils::CalcWSFromSS( evt.m_placementSS, evt.m_sceneTransformWS );

			TActorPlacement* placement = FindOrCreateActorCachedData( entity, m_actorsPlacements );
			SCENE_ASSERT( placement );

			TActorTransform* trans = FindOrCreateActorCachedData( entity, m_actorsTransforms );
			SCENE_ASSERT( trans );

			placement->m_data.m_placementWS = finalPositionWS;
			placement->m_data.m_timeStampAbsEvt = evt.m_eventTimeAbs;
			placement->m_data.m_timeStampAbsAccVal = evt.m_eventTimeAbs;
			placement->m_data.m_timeStampAbsAcc = 0;
			trans->m_data = finalPositionWS;
		}	
	}

	return size > 0;
}

namespace
{
	Int32 FloatToInt( const Float val )
	{
		return (Int32)( val );
	}
}

Bool CStorySceneEventsExecutor::FillActorsMotions( const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger )
{
	struct InternalData
	{
		CEntity*											m_entity;
		TDynArray< CStorySceneMotionExtractionSampler >		m_samplers;
		Float												m_startTime;
		Float												m_endTime;

		InternalData() : m_startTime( FLT_MAX ), m_endTime( -FLT_MAX ) {}
	};

	TDynArray< InternalData > toRefresh;

	const Uint32 size = collector.m_actorMotion.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const StorySceneEventsCollector::ActorMotion& evt = collector.m_actorMotion[ i ];

		SCENE_ASSERT( evt.m_actorId != CName::NONE );
		SCENE_ASSERT( evt.m_animation );

		if ( CEntity* entity = FindActorByType( evt.m_actorId, AT_ACTOR ).Get() )
		{
			TActorPlacement* placement = FindOrCreateActorCachedData( entity, m_actorsPlacements );
			SCENE_ASSERT( placement );

			if ( evt.m_eventTimeAbs <= placement->m_data.m_timeStampAbsEvt )
			{
				continue;
			}

			TActorTransform* trans = FindOrCreateActorCachedData( entity, m_actorsTransforms );
			SCENE_ASSERT( trans );

			const Float prevTimeAbs = Max( (Float)evt.m_eventTimeAbsStart, placement->m_data.m_timeStampAbsEvt );
			const Float currTimeAbs = (Float)evt.m_eventTimeAbs;

			InternalData* data = nullptr;
			{
				const Uint32 toRefreshSize = toRefresh.Size();
				for ( Uint32 j=0; j<toRefreshSize; ++j )
				{
					if ( toRefresh[ j ].m_entity == entity )
					{
						data = &(toRefresh[ j ]);
					}
				}
				if ( !data )
				{
					toRefresh.Grow( 1 );
					data = &(toRefresh.Back());
					data->m_entity = entity;
				}
			}

			SCENE_ASSERT( data );

			const Float animTimeStartAbs = evt.m_eventTimeAbsStart;
			const Float animTimeEndAbs = evt.m_eventTimeAbsEnd;

			CStorySceneMotionExtractionSampler sampler( evt.m_animation );
			sampler.SetTimes( animTimeStartAbs, animTimeEndAbs );
			sampler.SetWeight( evt.m_weight );
			sampler.SetBlends( evt.m_blendIn, evt.m_blendOut );
			sampler.SetStretch( evt.m_stretch );

			if ( evt.m_supportsClipFront )
			{
				sampler.SetClipFront( evt.m_clipFront );
			}

			data->m_samplers.PushBack( sampler );

			if ( prevTimeAbs < data->m_startTime )
			{
				data->m_startTime = prevTimeAbs;
			}
			if ( currTimeAbs > data->m_endTime )
			{
				data->m_endTime = currTimeAbs;
			}
		}
	}

	static const Float SAMPLE_TIME_STEP = 1.f / 60.f;

	static const Float EPS = 1e-3f;

	const Uint32 num = toRefresh.Size();
	for ( Uint32 it=0; it<num; ++it )
	{
		InternalData& data = toRefresh[ it ];

		const Uint32 numSamples = data.m_samplers.Size();
		if ( numSamples == 0 )
		{
			continue;
		}

		TActorPlacement* placement = FindActorCachedData( data.m_entity, m_actorsPlacements );
		SCENE_ASSERT( placement );
		
		const Float timeStampEvt = placement->m_data.m_timeStampAbsEvt;
		const Float timeStampAccVal = placement->m_data.m_timeStampAbsAccVal;
		if ( timeStampAccVal > 0.f )
		{
			placement->m_data.m_timeStampAbsAcc = FloatToInt( timeStampAccVal / SAMPLE_TIME_STEP );
			placement->m_data.m_timeStampAbsAccVal = 0.f;
		}

		SCENE_ASSERT( data.m_startTime >= timeStampEvt );

		const Float timeStampAcc = placement->m_data.m_timeStampAbsAcc * SAMPLE_TIME_STEP;

		const Float dataStartTimeClamped = Max( data.m_startTime, timeStampAcc );
		SCENE_ASSERT( dataStartTimeClamped <= data.m_endTime );

		const Bool skipFirstPart = timeStampAcc > data.m_startTime;

		const Int32 stepStart = skipFirstPart ? placement->m_data.m_timeStampAbsAcc : FloatToInt( dataStartTimeClamped / SAMPLE_TIME_STEP );
		const Float stepStartTime = stepStart * SAMPLE_TIME_STEP;
		Float timeToStepStart = skipFirstPart ? 0.f : stepStartTime+SAMPLE_TIME_STEP - dataStartTimeClamped;

		const Int32 stepEnd = FloatToInt( data.m_endTime / SAMPLE_TIME_STEP );
		const Float stepEndTime = stepEnd * SAMPLE_TIME_STEP;
		Float timeFromStepEnd = data.m_endTime-stepEndTime;

		SCENE_ASSERT( stepStart <= stepEnd );
		SCENE_ASSERT( timeToStepStart >= 0.f );
		SCENE_ASSERT( timeToStepStart - SAMPLE_TIME_STEP < EPS );
		SCENE_ASSERT( timeFromStepEnd >= 0.f );
		SCENE_ASSERT( timeFromStepEnd - SAMPLE_TIME_STEP < EPS );

		timeToStepStart = Clamp( timeToStepStart, 0.f, SAMPLE_TIME_STEP );
		timeFromStepEnd = Clamp( timeFromStepEnd, 0.f, SAMPLE_TIME_STEP );

		const AnimQsTransform identity( AnimQsTransform::IDENTITY );
		AnimQsTransform accWindowTransformFirst( AnimQsTransform::IDENTITY );
		AnimQsTransform accWindowTransformFirstClamped( AnimQsTransform::IDENTITY );
		AnimQsTransform accWindowTransformMid( AnimQsTransform::IDENTITY );
		AnimQsTransform accWindowTransformLast( AnimQsTransform::IDENTITY );
		AnimQsTransform accWindowTransformLastClamped( AnimQsTransform::IDENTITY );

		Float accTime = 0.f;

		if ( stepStart == stepEnd )
		{
			const Float timeToSampleStartAbs = (Float)( stepStart * SAMPLE_TIME_STEP );
			const Float timeToSampleEndAbs = (Float)( (stepStart+1) * SAMPLE_TIME_STEP );

			accTime += (data.m_endTime-dataStartTimeClamped);

			const Float weight = (data.m_endTime-dataStartTimeClamped) / (Float)SAMPLE_TIME_STEP;
			SCENE_ASSERT( weight >= 0.f && weight <= 1.f );

			SCENE_ASSERT( timeToSampleStartAbs <= timeToSampleEndAbs );

			AnimQsTransform accTemp;
			Bool accSomething = false;
			{
				accTemp.SetZero();

				Float accWeight = 0.f;

				for ( Uint32 s=0; s<numSamples; ++s )
				{
					CStorySceneMotionExtractionSampler& sampler = data.m_samplers[ s ];

					AnimQsTransform motion;
					if ( sampler.Sample( timeToSampleStartAbs, timeToSampleEndAbs, motion ) )
					{
						const Float localWeight = sampler.GetWeight();
						accWeight += localWeight;

						accTemp.BlendAddMul( motion, localWeight );
					}
				}

				if ( accWeight > 0.f )
				{
					accTemp.BlendNormalize( accWeight );
					accTemp.Translation.W = 1.f;
					accSomething = true;
				}
			}

			if ( accSomething )
			{
				accWindowTransformLast = accTemp;
				accWindowTransformLastClamped.Lerp( identity, accWindowTransformLast, weight );
			}
		}
		else
		{
			if ( !skipFirstPart )
			{
				const Float timeToSampleStartAbs = (Float)( stepStart * SAMPLE_TIME_STEP );
				const Float timeToSampleEndAbs = (Float)( (stepStart+1) * SAMPLE_TIME_STEP );

				accTime += timeToStepStart;

				const Float weight = timeToStepStart / (Float)SAMPLE_TIME_STEP;
				SCENE_ASSERT( weight >= 0.f && weight <= 1.f );

				SCENE_ASSERT( timeToSampleStartAbs <= timeToSampleEndAbs );

				AnimQsTransform accTemp;
				Bool accSomething = false;
				{
					accTemp.SetZero();

					Float accWeight = 0.f;

					for ( Uint32 s=0; s<numSamples; ++s )
					{
						CStorySceneMotionExtractionSampler& sampler = data.m_samplers[ s ];

						AnimQsTransform motion;
						if ( sampler.Sample( timeToSampleStartAbs, timeToSampleEndAbs, motion ) )
						{
							const Float localWeight = sampler.GetWeight();
							accWeight += localWeight;

							accTemp.BlendAddMul( motion, localWeight );
						}
					}

					if ( accWeight > 0.f )
					{
						accTemp.BlendNormalize( accWeight );
						accTemp.Translation.W = 1.f;
						accSomething = true;
					}
				}

				if ( accSomething )
				{
					accWindowTransformFirst = accTemp;
					accWindowTransformFirstClamped.Lerp( identity, accWindowTransformFirst, weight );
				}
			}

			{
				accWindowTransformMid.SetIdentity();

				Int32 i = skipFirstPart ? stepStart : stepStart+1;
				for ( ; i<stepEnd; ++i )
				{
					const Float timeToSampleStartAbs = (Float)( i * SAMPLE_TIME_STEP );
					const Float timeToSampleEndAbs = (Float)( (i+1) * SAMPLE_TIME_STEP );

					accTime += SAMPLE_TIME_STEP;

					AnimQsTransform accTemp;
					Bool accSomething = false;
					{
						accTemp.SetZero();

						Float accWeight = 0.f;

						for ( Uint32 s=0; s<numSamples; ++s )
						{
							CStorySceneMotionExtractionSampler& sampler = data.m_samplers[ s ];

							AnimQsTransform motion;
							if ( sampler.Sample( timeToSampleStartAbs, timeToSampleEndAbs, motion ) )
							{
								const Float localWeight = sampler.GetWeight();
								accWeight += localWeight;

								accTemp.BlendAddMul( motion, localWeight );
							}
						}

						if ( accWeight > 0.f )
						{
							accTemp.BlendNormalize( accWeight );
							accTemp.Translation.W = 1.f;
							accSomething = true;
						}
					}

					if ( accSomething )
					{
						accWindowTransformMid.SetMul( accWindowTransformMid, accTemp );
					}
				}
			}

			{
				const Float timeToSampleStartAbs = (Float)( stepEnd * SAMPLE_TIME_STEP );
				const Float timeToSampleEndAbs = (Float)( (stepEnd+1) * SAMPLE_TIME_STEP );

				accTime += timeFromStepEnd;

				const Float weight = timeFromStepEnd / (Float)SAMPLE_TIME_STEP;
				SCENE_ASSERT( weight >= 0.f && weight <= 1.f );

				SCENE_ASSERT( timeToSampleStartAbs <= timeToSampleEndAbs );

				AnimQsTransform accTemp;
				Bool accSomething = false;
				{
					accTemp.SetZero();

					Float accWeight = 0.f;

					for ( Uint32 s=0; s<numSamples; ++s )
					{
						CStorySceneMotionExtractionSampler& sampler = data.m_samplers[ s ];

						AnimQsTransform motion;
						if ( sampler.Sample( timeToSampleStartAbs, timeToSampleEndAbs, motion ) )
						{
							const Float localWeight = sampler.GetWeight();
							accWeight += localWeight;

							accTemp.BlendAddMul( motion, localWeight );
						}
					}

					if ( accWeight > 0.f )
					{
						accTemp.BlendNormalize( accWeight );
						accTemp.Translation.W = 1.f;
						accSomething = true;
					}
				}

				if ( accSomething )
				{
					accWindowTransformLast = accTemp;
					accWindowTransformLastClamped.Lerp( identity, accWindowTransformLast, weight );
				}
			}
		}

		const Float motionDuration = data.m_endTime - dataStartTimeClamped;
		ASSERT( MAbs( motionDuration - accTime ) < EPS );

		TActorTransform* trans = FindActorCachedData( data.m_entity, m_actorsTransforms );
		SCENE_ASSERT( trans );

		{
			AnimQsTransform accTransform( AnimQsTransform::IDENTITY );
			accTransform.SetMul( accTransform, accWindowTransformFirstClamped );
			accTransform.SetMul( accTransform, accWindowTransformMid );
			accTransform.SetMul( accTransform, accWindowTransformLastClamped );

			const AnimQsTransform placementTrans = MatrixToAnimQsTransform( placement->m_data.m_placementWS );

			AnimQsTransform finalTransWS;
			finalTransWS.SetMul( placementTrans, accTransform );
			trans->m_data = AnimQsTransformToMatrix( finalTransWS );
		}

		{
			AnimQsTransform accTransform( AnimQsTransform::IDENTITY );
			accTransform.SetMul( accTransform, accWindowTransformFirst );
			accTransform.SetMul( accTransform, accWindowTransformMid );

			const AnimQsTransform placementTrans = MatrixToAnimQsTransform( placement->m_data.m_placementWS );

			AnimQsTransform finalTransWS;
			finalTransWS.SetMul( placementTrans, accTransform );
			placement->m_data.m_placementWS = AnimQsTransformToMatrix( finalTransWS );
		}

		/*if ( CActor* a = Cast< CActor >( placement->m_id ) )
		{
			if ( a->GetVoiceTag() == TXT("GERALT") )
			{
				_startStep = stepStart;
				_endStep = stepEnd;
				_deltaFirst = accWindowTransformFirst.Translation.Y;
				_deltaFirstCl = accWindowTransformFirstClamped.Translation.Y;
				_deltaFirstW = timeToStepStart / (Float)SAMPLE_TIME_STEP;
				_deltaMid = accWindowTransformMid.Translation.Y;
				_deltaEnd = accWindowTransformLast.Translation.Y;
				_deltaEndCl = accWindowTransformLastClamped.Translation.Y;
				_deltaEndW = timeFromStepEnd / (Float)SAMPLE_TIME_STEP;
				_deltaAll1 = _deltaFirstCl + _deltaMid + _deltaEndCl;
				_deltaAll2 = _deltaFirst + _deltaMid;
			}
		}*/

		placement->m_data.m_timeStampAbsAcc = stepEnd;
	}

	return num > 0;
}

Bool CStorySceneEventsExecutor::ProcessChangeGameStates( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector )
{
	Bool ret = false;

	if ( collector.m_actorChangeGameState.Empty() )
	{
		return ret;
	}

	const CStoryScenePlayer* p = player;
	const CStorySceneSectionPlayingPlan* plan = p->GetCurrentPlayingPlan();

	// TODO - special code for cutscenes
	CCutsceneInstance* csInstance = nullptr;
	if ( plan && plan->GetCurrElement() && plan->GetCurrElement()->GetElement()->IsA< CStorySceneCutscenePlayer >() )
	{
		const StorySceneCutscenePlayerInstanceData* csPlayerInstanceData = static_cast< const StorySceneCutscenePlayerInstanceData* >( plan->GetCurrElement() );
		csInstance = csPlayerInstanceData->GetCutsceneInstance();
	}

	if ( plan && plan->GetCurrElement() && plan->GetCurrElement()->IsSkipped() && csInstance )
	{
		return ret;
	}

	const Uint32 size = collector.m_actorChangeGameState.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const StorySceneEventsCollector::ActorChangeGameState& evt = collector.m_actorChangeGameState[ i ];
		if ( THandle< CEntity > entityHandle = FindActorByType( evt.m_actorId, AT_ACTOR ) )
		{
			CEntity* entity = entityHandle.Get();
			if ( CActor* actor = Cast< CActor >( entity ) )
			{
				Bool teleportedToSafeSpot = false;

				TActorPlacement* placement = FindOrCreateActorCachedData( entity, m_actorsPlacements );
				SCENE_ASSERT( placement );
				TActorTransform* trans = FindOrCreateActorCachedData( entity, m_actorsTransforms );
				SCENE_ASSERT( trans );

				// Switch to gameplay pose
				if ( evt.m_switchToGameplayPose )
				{
					if ( csInstance )
					{
						csInstance->CsToGameplayRequestForActor( actor, evt.m_blendPoseDuration );
					}

					if ( player->IsSceneInGame() )
					{
						// Apply transform to actor (to avoid loosing placement events placed at the same time on timeline)
						actor->GetRootAnimatedComponent()->SetThisFrameTempLocalToWorld( trans->m_data );

						// Switch actor to gameplay
						Vector teleportedTransform;
						if ( ( teleportedToSafeSpot = player->SetSceneStateOnActor( entityHandle, false, &teleportedTransform ) ) )
						{
							ret = true;
							trans->m_data.SetTranslation( teleportedTransform );
						}

						if ( actor->GetRootAnimatedComponent() && actor->GetRootAnimatedComponent()->GetBehaviorStack() )
						{
							CBehaviorGraphStack* stack = actor->GetRootAnimatedComponent()->GetBehaviorStack();

							actor->RaiseBehaviorForceEvent( CNAME(ForceIdle));

							if ( evt.m_activateBehaviorGraph >= 0 && !actor->IsPlayer() )
							{
								CName behaviorNameToSet;
								CallFunctionRet< CName, Int32 >( nullptr, CNAME( BehGraphIntToName ), evt.m_activateBehaviorGraph, behaviorNameToSet );

								if ( behaviorNameToSet )
								{
									if ( !stack->ActivateBehaviorInstances( behaviorNameToSet ) )
									{
										SCENE_LOG( TXT("Scene to gameplay transition error - Couldn't active behavior graph. Actor: '%ls', graph: '%ls'"), evt.m_actorId.AsChar(), behaviorNameToSet.AsChar() );
									}
									
									if ( csInstance )
									{
										if ( !stack->AttachBehaviorInstance( CNAME( Cutscene ) ) )
										{
											SCENE_LOG( TXT("Scene to gameplay transition error - Couldn't attach behavior graphs. Actor: '%ls', graph: 'Cutscene'"), evt.m_actorId.AsChar() );
										}
									}
								}
							}

							if ( evt.m_gameplayPoseTypeName )
							{
								SBehaviorSyncTags syncTags;
								syncTags.Add( evt.m_gameplayPoseTypeName );

								if ( !stack->SynchronizeTo( syncTags ) )
								{
									SCENE_LOG( TXT("Scene to gameplay transition error - Couldn't synchronize behavior graphs. Actor: '%ls', sync tag: '%ls'"), evt.m_actorId.AsChar(), evt.m_gameplayPoseTypeName.AsChar() );
								}
							}

							if ( evt.m_raiseGlobalBehaviorEvent )
							{
								if ( !stack->GenerateBehaviorForceEvent( evt.m_raiseGlobalBehaviorEvent ) )
								{
									SCENE_LOG( TXT("Scene to gameplay transition error - Couldn't process behavior graph event. Actor: '%ls', event name: '%ls'"), evt.m_actorId.AsChar(), evt.m_raiseGlobalBehaviorEvent.AsChar() );
								}
							}
						}
					}
				}

				// TEMP
				if ( !player->IsSceneInGame() )
				{
					continue;
				}

				// Snap to terrain
				if ( evt.m_snapToTerrain || evt.m_startGameplayAction > 0 )
				{
					ret = true;

					if ( csInstance )
					{
						trans->m_data = csInstance->GetActorFinalPosition( entity );

						if ( teleportedToSafeSpot )
						{
							Vector safePosition;
							if ( player->FindSafeSpotForActor( entity, trans->m_data.GetTranslation(), safePosition ) )
							{
								trans->m_data.SetTranslation( safePosition );
							}
						}
					}

					SCENE_ASSERT( trans );
					if ( trans )
					{
						Matrix& finalPositionWS = trans->m_data;

						if ( evt.m_snapToTerrain )
						{
							Vector currPosition = finalPositionWS.GetTranslationRef();
							Vector newPosition( currPosition );

							const Bool traceSuccess = StorySceneUtils::DoTraceZTest( actor, currPosition, newPosition );

							if ( evt.m_snapToTerrainDuration > 0.f && traceSuccess )
							{
								const Float traceOffsetZ_MS = newPosition.Z - currPosition.Z;
								const Float blendTime = evt.m_snapToTerrainDuration;

								if ( CMovingAgentComponent* mac = actor->GetMovingAgentComponent() )
								{
									const Float offsetZ_MS = traceOffsetZ_MS;

									mac->SetAdditionalOffsetToConsumeMS( Vector( 0.f, 0.f, -offsetZ_MS ), EulerAngles::ZEROS, blendTime );
									mac->AccessAnimationProxy().ForceUseIK( false );
									mac->AccessAnimationProxy().SetUseIK( true, blendTime );
								}
							}

							finalPositionWS.SetTranslation( newPosition );
						}

						if ( evt.m_startGameplayAction > 0 )
						{
							SStorySceneGameplayActionCallbackInfo callbackInfo;
							callbackInfo.m_inActorPosition = finalPositionWS.GetTranslation();
							callbackInfo.m_inActorHeading = finalPositionWS.GetAxisY();
							callbackInfo.m_inGameplayAction = evt.m_startGameplayAction;
							callbackInfo.m_inActor = THandle< CActor >( actor );

							//++ Force player's final position because scripts and AI will use this transform inside 'DoStorySceneGameplayAction' func
							if ( actor == GGame->GetPlayerEntity() && actor->GetRootAnimatedComponent() )
							{
								actor->GetRootAnimatedComponent()->SetThisFrameTempLocalToWorld( finalPositionWS );
								player->GetSceneDirector()->SnapPlayerEntityToTeleportedTransform();
							}
							//--

							const Bool ret = CallFunctionRef( nullptr, CNAME( DoStorySceneGameplayAction ), callbackInfo );
							if ( !ret )
							{
								SCENE_LOG( TXT("Scene to gameplay transition error - Couldn't execute 'DoStorySceneGameplayAction' script function. Actor: '%ls', action enum: '%d'"), evt.m_actorId.AsChar(), evt.m_startGameplayAction );
							}
							else
							{
								if ( callbackInfo.m_outDontUseSceneTeleport )
								{
									placement->m_data.m_dontTeleportMe = true;
								}
							}
						}
					}
				}

				if ( evt.m_forceResetClothAndDangles )
				{
					ForceResetWithRelaxedClothState( actor, true, true );
				}

				// Remember this decision
				placement->m_data.m_switchedToGameplayMode = true;

				// Check if data is ok
				{
					SCENE_ASSERT( trans->m_data.IsOk() );
					Vector pos = trans->m_data.GetTranslation();
					EulerAngles rot = trans->m_data.ToEulerAngles();
					rot.Pitch = 0.f;
					rot.Roll = 0.f;
					trans->m_data = rot.ToMatrix();
					trans->m_data.SetTranslation( pos );
				}
			}
		}
	}

	return ret;
}

void CStorySceneEventsExecutor::ProcessActorsTransforms( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger )
{
	const Uint32 size = m_actorsTransforms.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		TActorTransform& t = m_actorsTransforms[ i ];

		CEntity* entity = t.m_id;
		SCENE_ASSERT( entity );

		TActorPlacement* placement = FindOrCreateActorCachedData( entity, m_actorsPlacements );
		SCENE_ASSERT( placement );
		if ( placement->m_data.m_dontTeleportMe )
		{
			continue;
		}

		if ( placement->m_data.m_switchedToGameplayModeDone )
		{
			SCENE_ASSERT( placement->m_data.m_switchedToGameplayMode );
			continue;
		}
		else if ( placement->m_data.m_switchedToGameplayMode )
		{
			placement->m_data.m_switchedToGameplayModeDone = true;
		}

		// Get the transform (and get rid of the scale)

		Matrix finalPositionWS = t.m_data;
		Vector scale;
		finalPositionWS.ExtractScale( finalPositionWS, scale );
		finalPositionWS.SetTranslation( t.m_data.GetTranslation() );
		SCENE_ASSERT( finalPositionWS.IsOk() );

		if ( debugger )
		{
			debugger->OnExecutor_ProcessPlacements( entity, finalPositionWS );
		}

		//const Bool posChaged =  !Vector::Near3( finalPositionWS.GetTranslationRef(), entity->GetWorldPositionRef() );
		//const Bool rotChanged = !finalPositionWS.ToEulerAngles().AlmostEquals( entity->GetWorldRotation() );

		/*if ( CActor* a = Cast< CActor >( entity ) )
		{
			if ( a->GetVoiceTag() == TXT("GERALT") )
			{
				static Float prevY = 0.f;

				SCENE_LOG( TXT("DWS: %1.5f, PDWS: %1.5f, startStep: %d, endStep: %d, dAll1: %1.5f, dAll2: %1.5f, dF: %1.5f, dFCl: %1.5f, dFW: %1.5f, dM: %1.5f, dE: %1.5f, dECl: %1.5f, dEW: %1.5f")
					, finalPositionWS.GetTranslationRef().Y - a->GetWorldPositionRef().Y
					, a->GetWorldPositionRef().Y - prevY
					, _startStep, _endStep
					, _deltaAll1, _deltaAll2
					, _deltaFirst, _deltaFirstCl, _deltaFirstW
					, _deltaMid
					, _deltaEnd, _deltaEndCl, _deltaEndW );

				prevY = a->GetWorldPositionRef().Y;
			}
		}*/

		//if ( posChaged || rotChanged )
		{
			SCENE_VERIFY( entity->Teleport( finalPositionWS.GetTranslation(), finalPositionWS.ToEulerAngles() ) );

			if ( CAnimatedComponent* root = entity->GetRootAnimatedComponent() )
			{
				root->SetThisFrameTempLocalToWorld( finalPositionWS );

				if ( CActor* a = Cast< CActor >( entity ) )
				{
					if ( CMimicComponent* mimics = a->GetMimicComponent() )
					{
						const Matrix mimicsWSMat = CStorySceneActorsEyesTracker::GetActorMimicWorldMatrix( finalPositionWS, mimics );
						mimics->SetThisFrameTempLocalToWorld( mimicsWSMat );
					}

					if ( a->GetVisualDebug() )
					{
						a->GetVisualDebug()->AddAxis( CNAME( StoryScene ), 1.f, finalPositionWS.GetTranslation(), finalPositionWS.ToEulerAngles(), true, 10.f );
						a->GetVisualDebug()->AddSphere( CNAME( StoryScene2 ), 1.f, finalPositionWS.GetTranslation(), true, Color::RED, 10.f );
					}
				}
			}

			// Make sure the camera uses new player position if we have blend event on the same frame

			if ( entity == GGame->GetPlayerEntity() && collector.m_cameraStartBlendToGameplay.m_isSet )
			{
				player->GetSceneDirector()->SnapPlayerEntityToTeleportedTransform();

				CCamera* sceneCamera = player->GetSceneCamera();
				if ( sceneCamera )
				{
					ICamera::Data data;
					sceneCamera->GetData( data );

					GCommonGame->StartGameplayCameraBlendFrom( data, collector.m_cameraStartBlendToGameplay.m_event.m_blendTime );
				}
			}
		}
	}
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
