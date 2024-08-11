#include "build.h"
#include "storyScenePlayer.h"

#include "../core/feedback.h"
#include "../core/contentManager.h"
#include "../engine/behaviorGraphStack.h"
#include "../engine/soundStartData.h"
#include "../engine/deniedAreaComponent.h"
#include "../engine/cutsceneInstance.h"

#include "gameWorld.h"
#include "movableRepresentationPathAgent.h"
#include "movingPhysicalAgentComponent.h"

#include "storySceneIncludes.h"
#include "storySceneComponent.h"
#include "storySceneCutsceneSection.h"
#include "storySceneInput.h"
#include "storyScenePlaybackListener.h"
#include "storySceneRandomizer.h"
#include "storySceneScriptingBlock.h"
#include "storySceneSection.h"
#include "storySceneEvent.h"
#include "storySceneEventDuration.h"
#include "storySceneSectionPlayingPlan.h"
#include "storySceneFlowCondition.h"
#include "storySceneFlowSwitch.h"
#include "storySceneSectionOverrideCondition.h"
#include "storySceneSystem.h"
#include "sceneLog.h"
#include "storySceneChoiceLine.h"
#include "storySceneChoice.h"
#include "storySceneVideo.h"
#include "storySceneDisplayInterface.h"
#include "storySceneDebugger.h"
#include "storySceneEventsCollector.h"
#include "storySceneEventDialogLine.h"
#include "storySceneControlPartsUtil.h"

#ifndef RED_FINAL_BUILD
#include "../engine/rawInputManager.h"
#include "../engine/inputKeys.h"
#endif

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

namespace
{
	class CSeekEventFilter : public CStorySceneEventsCollectorFilter
	{
	public:
		virtual Bool DoFilterOut( const CStorySceneEvent* e ) const override
		{
			if ( e->GetClass()->IsA< CStorySceneEventDialogLine >() )
			{
				return true;
			}

			return false;
		}
	};

	class CSkipEventFilter : public CStorySceneEventsCollectorFilter
	{
	public:
		virtual Bool DoFilterOut( const CStorySceneEvent* e ) const override
		{
			if ( e->GetClass()->IsA< CStorySceneEventDialogLine >() )
			{
				return true;
			}

			return false;
		}
	};
}

Bool CStoryScenePlayer::SetState( const ScenePlayerInputState& inputState )
{
	m_internalState.m_isInTick = true;

	if ( m_internalState.m_isFrozen )
	{
		if ( inputState.m_section2 )
		{
			m_internalState.m_isFrozen = false;
		}
		else
		{
			return true;
		}
	}

	if( CStorySceneSectionPlayingPlan* plan = m_sectionLoader.FindPlanById( m_internalState.m_planId ) )
	{
		ResetEventsPlayState( *plan );
	}

	{
		// To nie ok
		if ( m_isStopped == false )
		{
			m_sectionLoader.OnTick( 0.f );
		}

		// To ok - czemu to tu jest w ogole potrzebne?
		UpdatePlaybackListeners();
	}

	EInternalState state = IS_Init;
	TickContext context;
	context.m_timeDelta = 0.f;
	context.m_forceTime = inputState.m_timeIsSet2;
	context.m_forcedTime = inputState.m_time2;
	context.m_forceSection2 = inputState.m_section2;
	context.m_requestRestart = inputState.m_restart2;

	m_internalState.m_forcingTimeThisFrame = context.m_forceTime;

	{
		if ( m_debugger )
		{
			m_debugger->PlayerLogicTickBegin( this );
		}

		LogicTick( state, context );

		if ( m_debugger )
		{
			m_debugger->PlayerLogicTickEnd( this );
		}
	}

	m_internalState.m_isInTick = false;

	if ( inputState.m_paused2 && !IsPaused() )
	{
		Pause( true );
	}
	else if ( !inputState.m_paused2 && IsPaused() )
	{
		Pause( false ); 
	}

	return inputState.m_section2 ? inputState.m_section2 == GetCurrentSection() : true;
}

Bool CStoryScenePlayer::Tick( Float timeDelta )
{
	SCENE_ASSERT__FIXME_LATER( !m_isStopped );
	SCENE_ASSERT__FIXME_LATER( !m_isDestoryed );

	m_debugStats.SetCurrDeltaTime( timeDelta );

	if ( m_internalState.m_requestSceneRestart || m_internalState.m_requestSectionRestart )
	{
		m_debugStats.Reset();
	}

	m_internalState.m_isInTick = true;

	if ( m_internalState.m_isFrozen )
	{
		return true;
	}

	{
		// To nie ok
		if ( !m_isStopped )
		{
			m_sectionLoader.OnTick( timeDelta );
		}

		// To ok - czemu to tu jest w ogole potrzebne?
		UpdatePlaybackListeners();
	}

	Bool ret = !m_isStopped;

	if ( !m_isStopped && !IsPaused()  )
	{
		EInternalState state = IS_Init;
		TickContext context;
		m_internalState.m_sceneProgress  += timeDelta;
		context.m_timeDelta = timeDelta / m_internalState.m_timeMultiplier;

		if ( m_internalState.m_requestSectionRestart )
		{
			m_internalState.m_requestSectionRestart = false;
			context.m_forcedTime = 0.f;
			context.m_forceTime = true;
			context.m_requestRestart = true;
		}
		if ( m_internalState.m_requestSceneRestart )
		{
			m_internalState.m_requestSceneRestart = false;	
			context.m_requestSceneRestart = true;
		}
		m_internalState.m_forcingTimeThisFrame = context.m_forceTime;

		if ( m_debugger )
		{
			m_debugger->PlayerLogicTickBegin( this );
		}

		m_interceptionHelper.OnPlayerTick( timeDelta );


#ifndef RED_FINAL_BUILD
		if ( RIM_KEY_JUST_PRESSED( IK_NumPad0 ) )
			state = IS_RequestGoToNextSection;
#endif

		ret = LogicTick( state, context );
		if ( !ret )
		{
			return false;
		}

		if ( m_debugger )
		{
			m_debugger->PlayerLogicTickEnd( this );
		}
	}

	m_internalState.m_isInTick = false;

	return ret;
}

namespace
{
	void SetError( CStoryScenePlayer::EInternalState& state )
	{
		// Place for breakpoint
		state = CStoryScenePlayer::IS_Error;
	}

	String ConvertToString( CStoryScenePlayer::EInternalState state )
	{
		if		( state ==  CStoryScenePlayer::IS_Init )				return TXT("IS_Init");
		else if ( state ==  CStoryScenePlayer::IS_CheckSceneEnd )		return TXT("IS_CheckSceneEnd");
		else if ( state ==  CStoryScenePlayer::IS_CheckSection )		return TXT("IS_CheckSection");
		else if ( state ==  CStoryScenePlayer::IS_CheckElement )		return TXT("IS_CheckElement");
		else if ( state ==  CStoryScenePlayer::IS_ChangingSection )		return TXT("IS_ChangingSection");
		else if ( state ==  CStoryScenePlayer::IS_GoToNextElement )		return TXT("IS_GoToNextElement");
		else if ( state ==  CStoryScenePlayer::IS_PlayElement )			return TXT("IS_PlayElement");
		else if ( state ==  CStoryScenePlayer::IS_RequestGoToNextSection )	return TXT("IS_RequestGoToNextSection");
		else if ( state ==  CStoryScenePlayer::IS_GoToNextSection )		return TXT("IS_GoToNextSection");
		else if ( state ==  CStoryScenePlayer::IS_Error )				return TXT("IS_Error");
		else if ( state ==  CStoryScenePlayer::IS_SkipLine )			return TXT("IS_SkipLine");
		else if ( state ==  CStoryScenePlayer::IS_SkipSection )			return TXT("IS_SkipSection");
		else if ( state ==  CStoryScenePlayer::IS_RestartAndGoTo )		return TXT("IS_RestartAndGoTo");
		else															{ SCENE_ASSERT( 0 ); return TXT("<Unknown>"); }
	}
}

#define LOG_SCENE_SEBUG_INFO( x );

Bool CStoryScenePlayer::LogicTick_Intro( CStoryScenePlayer::EInternalState state, CStoryScenePlayer::TickContext& context )
{
	ASSERT( m_internalState.m_intro.m_state != InternalFlowState::IntroState_Finished );

	// Enter intro stage

	if ( m_internalState.m_intro.m_state == InternalFlowState::IntroState_Initial )
	{
		// Shall we play vehicle dismount cutscene?

		m_internalState.m_intro.m_playCutscene =
			m_internalState.m_input->GetEnableIntroVehicleDismount() &&
			m_player && m_player->ShouldPlayPreDialogCutscene( m_internalState.m_intro.m_vehicle );

		// Shall we apply look-ats

		m_internalState.m_intro.m_applyLookAts = false;
		if ( !m_internalState.m_intro.m_playCutscene && m_internalState.m_input->GetEnableIntroLookAts() )
		{
			for ( auto it = m_sceneActorEntities.Begin(), end = m_sceneActorEntities.End(); it != end; ++it )
			{
				CActor* actor = Cast< CActor >( it->m_second.Get() );
				if ( !actor || actor == m_player )
				{
					continue;
				}

				m_internalState.m_intro.m_applyLookAts = true;
				break;
			}
		}

		// Can we skip the whole intro?

		const Bool skipIntro = !m_internalState.m_intro.m_playCutscene && !m_internalState.m_intro.m_applyLookAts;
		if ( skipIntro )
		{
			m_internalState.m_intro.m_state = InternalFlowState::IntroState_Finished;
			state = IS_CheckSection;
			return LogicTick( state, context );
		}

		// Store section to go to after intro is done

		m_internalState.m_intro.m_destSection = context.m_destSection2;

		// Clear dialog area

		m_sceneDirector.ClearAreaForSection( m_internalState.m_intro.m_destSection );

		m_internalState.m_intro.m_state = InternalFlowState::IntroState_WaitingForNPCTeleport;
	}

	// Wait until NPCs teleport job is completed

	if ( m_internalState.m_intro.m_state == InternalFlowState::IntroState_WaitingForNPCTeleport &&
		 !m_sceneDirector.IsNPCTeleportJobInProgress() )
	{
		m_sceneDirector.FinalizeTeleportNPCsJob();

		m_internalState.m_intro.m_state = InternalFlowState::IntroState_Started;
		m_internalState.m_intro.m_time = 0.0f;

		// Enable scene HUD

		ToggleHud( true );

		// Switch actors into scene mode

		// SetSceneState( true );

		// Start cutscene

		if ( m_internalState.m_intro.m_playCutscene )
		{
			m_internalState.m_intro.m_cutsceneInstance = m_player->StartPreDialogCutscene( m_internalState.m_intro.m_vehicle );
			ASSERT( m_internalState.m_intro.m_cutsceneInstance );

			SceneFadeIn( TXT("Intro"), 0.0f );

			// Switch player into "scene" mode

			if ( m_player )
			{
				m_player->CallEvent< THandle< CStoryScene > >( CNAME( OnBlockingSceneStarted_OnIntroCutscene ), GetStoryScene() );
			}
		}

		else // If no cutscene is to be played, apply lookats
		{
			m_sceneDirector.ActivateCameras();

			CActor* nonPlayerActor = nullptr;

			// Make all actors look at the player

			for ( auto it = m_sceneActorEntities.Begin(), end = m_sceneActorEntities.End(); it != end; ++it )
			{
				CActor* actor = Cast< CActor >( it->m_second.Get() );
				if ( !actor || actor == m_player )
				{
					continue;
				}

				if ( !nonPlayerActor )
				{
					nonPlayerActor = actor;
				}

				SLookAtDialogBoneInfo lookAtInfo;
				lookAtInfo.m_targetOwner = m_player->GetRootAnimatedComponent( );
				lookAtInfo.m_boneIndex = m_player->GetHeadBone();
				actor->EnableLookAt( lookAtInfo );
			}

			// Make the player look at some NPC

			if ( nonPlayerActor )
			{
				SLookAtDialogBoneInfo lookAtInfo;
				lookAtInfo.m_targetOwner = nonPlayerActor->GetRootAnimatedComponent();
				lookAtInfo.m_boneIndex = nonPlayerActor->GetHeadBone();
				m_player->EnableLookAt( lookAtInfo );
			}
		}
	}

	// Update

	m_internalState.m_intro.m_time += context.m_timeDelta;

	// After certain time, switch to check section state

	if ( ( !m_internalState.m_intro.m_cutsceneInstance && m_internalState.m_intro.m_time >= m_internalState.m_input->GetIntroTotalTime() ) ||
		 ( m_internalState.m_intro.m_cutsceneInstance && m_internalState.m_intro.m_cutsceneInstance->IsFinished() ) )
	{
		// Destroy the cutscene

		if ( m_internalState.m_intro.m_cutsceneInstance )
		{
			m_internalState.m_intro.m_cutsceneInstance->ProcessRemainingEvents();
			m_internalState.m_intro.m_cutsceneInstance->DestroyCs( false );
			m_internalState.m_intro.m_cutsceneInstance = nullptr;

			m_player->OnPostPreDialogCutscene( m_internalState.m_intro.m_vehicle, this );
			m_internalState.m_intro.m_vehicle = nullptr;

			m_sceneDirector.ActivateCameras();
		}

		// Start the actual scene

		m_internalState.m_intro.m_state = InternalFlowState::IntroState_Finished;
		context.m_destSection2 = m_internalState.m_intro.m_destSection;

		state = IS_CheckSection;
		return LogicTick( state, context );
	}

	// Trigger fadeout

	if ( !m_internalState.m_intro.m_fadeOutStarted && m_internalState.m_input->GetEnableIntroFadeOut() )
	{
		const Float cutsceneFadeoutTime = 0.3f;

		Bool doFadeOut = false;
		Float fadeOutDuration = 0.0f;

		// Non-cutscene case

		if ( !m_internalState.m_intro.m_cutsceneInstance && m_internalState.m_intro.m_time >= m_internalState.m_input->GetIntroFadeOutStartTime() )
		{
			doFadeOut = true;
			fadeOutDuration = m_internalState.m_input->GetIntroTotalTime() - m_internalState.m_intro.m_time;
		}

		// Cutscene case

		else if ( m_internalState.m_intro.m_cutsceneInstance && m_internalState.m_intro.m_cutsceneInstance->GetTimeRemaining() <= cutsceneFadeoutTime )
		{
			doFadeOut = true;
			fadeOutDuration = m_internalState.m_intro.m_cutsceneInstance->GetTimeRemaining();
		}

		// Apply fade out

		if ( doFadeOut )
		{
			m_internalState.m_intro.m_fadeOutStarted = true;
			SceneFadeOut( TXT("Intro"), fadeOutDuration );
		}
	}

	return true;
}

namespace
{
	void OnCanSkipChanged( Bool canSkip )
	{
		GCommonGame->GetSystem< CStorySceneSystem >()->OnCanSkipChanged( canSkip );
	}
}

void CStoryScenePlayer::UpdateInfoAboutSkipping( Bool force )
{
	const Bool canSkip = IsSkippingAllowed();
	if( m_internalState.m_canSkipElement != canSkip || force )
	{
		m_internalState.m_canSkipElement = canSkip;
		OnCanSkipChanged( canSkip );
	}	
}

Bool CStoryScenePlayer::CanSkipElement( const CStorySceneElement* elem ) const
{
	return
		(elem == NULL || elem->CanBeSkipped()) &&
		(GContentManager->GetStallForMoreContent() == eContentStall_None) &&
		!GGame->HasBlackscreenRequested() &&
		!m_internalState.m_intro.IsInProgress() &&																													// Intro stage in progress
		!( m_internalState.m_intro.m_state == InternalFlowState::IntroState_Initial && m_internalState.m_input && m_internalState.m_input->GetEnableIntroStage() );	// Intro state is about to start
}

Bool CStoryScenePlayer::IsSkippingAllowed()
{
	if( const CStorySceneSectionPlayingPlan* plan = m_sectionLoader.FindPlanById( m_internalState.m_planId ) )
	{
		if( const IStorySceneElementInstanceData* instData = plan->GetCurrElement() )
		{
			if( const CStorySceneElement* elem = instData->GetElement() )
			{
				return CanSkipElement( elem );
			}
		}
	}

	return CanSkipElement(NULL);
}

Bool CStoryScenePlayer::LogicTick( CStoryScenePlayer::EInternalState state, CStoryScenePlayer::TickContext& context )
{
	//if ( m_debugger )
	//{
	//	m_debugger->PlayerLogicTickBegin( ConvertToString( state ) );
	//}

	Bool running = true;
	//while ( running )
	{
		switch ( state )
		{
		case IS_Init:
			{
				LOG_SCENE_SEBUG_INFO( TXT("IS_Init") );

				SCENE_ASSERT( context.m_events.Empty() );

				if ( m_internalState.m_intro.IsInProgress() )
				{
					state = IS_Intro;
					return LogicTick( state, context );
				}
				else if ( m_interceptionHelper.IsEmptyInterceptionInProgress() )
				{
					state = IS_EmptyInterception;
					return LogicTick( state, context );
				}

				if ( context.m_forceSection2 )
				{
					m_internalState.m_sceneStarted = true;

					const CStorySceneSection* nextSection = EvaluateControlChain( context.m_forceSection2, m_internalState, true );
					SCENE_ASSERT( nextSection == context.m_forceSection2 ); // ? to po co ten EvaluateControlChain?

					if ( nextSection )
					{
						context.m_destSection2 = nextSection;
						context.m_checkFlow = true;

						state = IS_CheckSection;

						return LogicTick( state, context );
						//break;
					}
				}
				else if ( ( m_internalState.m_input && !m_internalState.m_sceneStarted ) || context.m_requestSceneRestart )
				{
					m_internalState.m_sceneStarted = true;

					if ( context.m_requestSceneRestart )
					{
						m_sceneDirector.Reinitialize( m_internalState.m_input ); // we need to have if because prev initialization is in Init function

						context.m_requestSceneRestart = false;
					}

					if ( m_debugger )
					{
						m_debugger->OnStartFromInput( m_internalState.m_input );
					}

					const CStorySceneSection* sectionToStartPlaying = EvaluateControlChain( m_internalState.m_input, m_internalState, true );
					if ( sectionToStartPlaying )
					{
						context.m_destSection2 = sectionToStartPlaying;
						// context.m_checkFlow = true; - do not do this

						state = IS_CheckSection;

						return LogicTick( state, context );
					}

					SetError( state );

					return LogicTick( state, context );
					//break;
				}
				else if ( m_internalState.m_resumeRequest )
				{
					// ???
					SCENE_ASSERT( !IsPaused() );
					Pause( false );
					SCENE_ASSERT( !IsPaused() );

					const CStorySceneSection* nextSection = EvaluateControlChain( m_internalState.m_resumeRequest, m_internalState, true );

					m_internalState.m_resumeRequest = NULL;

					if ( nextSection )
					{
						context.m_destSection2 = nextSection;
						// context.m_checkFlow = true; - do not do this

						state = IS_CheckSection;

						return LogicTick( state, context );
						//break;
					}
				}
				else if ( m_internalState.m_selectedChoice )
				{
					if ( m_internalState.m_selectedChoice->IsA< CStorySceneChoiceLine >() )
					{
						// DIALOG_TOMSIN_TODO - to jakis krap
						// if this is a choice line, persist the choice that was made
						const CStorySceneChoiceLine* choiceLine = static_cast< const CStorySceneChoiceLine* >( m_internalState.m_selectedChoice );
						choiceLine->OnChoiceSelected( this );
					}

					const CStorySceneSection* nextSection = EvaluateControlChain( m_internalState.m_selectedChoice, m_internalState, false );
					m_internalState.m_selectedChoice = NULL;

					// nextSection can be null
					context.m_destSection2 = nextSection;
					// context.m_checkFlow = true; - do not do this

					if ( nextSection )
					{
						state = IS_CheckSection;

						return LogicTick( state, context );
					}
					else
					{
						if ( !m_scriptBlockThread )
						{
							context.m_requestSceneEnd = true;
						}						

						state = IS_CheckSceneEnd;

						return LogicTick( state, context );
					}
				}
				else if ( m_internalState.m_skipLine )
				{
					m_internalState.m_skipLine = false;

					state = IS_SkipLine;

					return LogicTick( state, context );
					// break;
				}
				else if ( m_internalState.m_scheduledNextElements > 0 )
				{
					m_internalState.m_scheduledNextElements = 0;
					CStorySceneSectionPlayingPlan* plan = m_sectionLoader.FindPlanById( m_internalState.m_planId );
					if ( plan->GetCurrElement() )
					{
						plan->GetCurrElement()->Stop();
					}

					state = IS_GoToNextElement;

					return LogicTick( state, context );
				}

				state = IS_CheckSceneEnd;

				return LogicTick( state, context );
			}

		case IS_Intro:
			{
				return LogicTick_Intro( state, context );
			}

		case IS_SkipLine:
			{
				LOG_SCENE_SEBUG_INFO( TXT("IS_SkipLine") );

				SCENE_ASSERT( context.m_events.Empty() );

				CStorySceneSectionPlayingPlan* plan = m_sectionLoader.FindPlanById( m_internalState.m_planId );

				OnLineSkipped();

				if ( plan )
				{
					IStorySceneElementInstanceData* elem = plan->GetCurrElement();
					if ( elem && CanSkipElement( elem->GetElement() ) )
					{
						elem->MarkSkipped();
					}
				}

				state = IS_Init;

				return LogicTick( state, context );
			}


		case IS_CheckSceneEnd:
			{
				LOG_SCENE_SEBUG_INFO( TXT("IS_CheckSceneEnd") );

				SCENE_ASSERT( context.m_events.Empty() );

				const Bool shouldEndScene = !m_internalState.m_pendingNextSection && !IsPlanInProgress() && !IsPaused(); //// czemu !pausa
				if ( ( context.m_requestSceneEnd || shouldEndScene ) && CanFinishScene() )
				{
					//++ DIALOG_TOMSIN_TODO - co to ma byc?
					//ChangeSection( NULL );
					//SCENE_ASSERT( !m_internalState.m_currentSection );
					SCENE_ASSERT( !m_internalState.m_pendingNextSection );
					//FinishChangingSection();
					//--

					Bool canEndScene( true );

					if ( m_internalState.m_justSkippedElement && !m_internalState.m_sceneEndSkipped )
					{
						m_internalState.m_sceneEndSkipped = true;
						m_internalState.m_sceneEndTimer = 10.f;
					}

					if ( m_internalState.m_sceneEndSkipped && IsInGame() && GCommonGame->GetPlayer() )
					{
						const CPlayer* player = GCommonGame->GetPlayer();
						const CName playerVoicetag = player->GetVoiceTag();
						if ( playerVoicetag )
						{
							if ( CEntity* e = GetSceneActorEntity( playerVoicetag ) )
							{
								SCENE_ASSERT( e == player );

								if ( CInventoryComponent* inventoryComponent = player->GetInventoryComponent() )
								{
									if ( !inventoryComponent->AreAllMountedItemsSpawned() )
									{
										canEndScene = false;
									}
								}
							}
						}
					}
					
					m_internalState.m_sceneEndTimer -= context.m_timeDelta;
					if ( m_internalState.m_sceneEndSkipped && m_internalState.m_sceneEndTimer < 0.f )
					{
						canEndScene = true;
					}
					
					if ( canEndScene )
					{
						if ( m_internalState.m_sceneEndSkipped )
						{
							const String reason = String::Printf( TXT("StoryScene '%ls' - Waiting for player's items"), GetStoryScene()->GetDepotPath().AsChar() );
							SceneFadeIn( reason );

							m_internalState.m_sceneEndSkipped = false;
						}

						EndPlayingScene( false );

						running = false;
						return false;
					}
					else if ( !m_internalState.m_sceneEndFadeSet )
					{
						const String reason = String::Printf( TXT("StoryScene '%ls' - Waiting for player's items"), GetStoryScene()->GetDepotPath().AsChar() );
						SetSceneBlackscreen( true, reason );
						
						m_internalState.m_sceneEndFadeSet = true;
					}

					return true;
				}
				else if ( m_internalState.m_planId == -1 && !m_internalState.m_pendingNextSection )
				{
					// Wait
					return true;
				}
				else if ( context.m_requestSceneEnd )
				{
					// Wait
					context.m_requestSceneEnd = false;

					return true;
				}
				else
				{
					state = IS_CheckSection;

					return LogicTick( state, context );
				}

				break;
			}

		case IS_CheckSection:
			{
				LOG_SCENE_SEBUG_INFO( TXT("IS_CheckSection") );

				SCENE_ASSERT( context.m_events.Empty() );

				if ( IsInGame() &&
					 context.m_destSection2 &&
					 m_internalState.m_input && m_internalState.m_input->GetEnableIntroStage() &&
					 m_internalState.m_intro.m_state != InternalFlowState::IntroState_Finished )
				{
					if ( !context.m_destSection2->IsGameplay() )
					{
						state = IS_Intro;
						return LogicTick( state, context );
					}
					else
					{
						m_internalState.m_intro.MarkAsSkipped();
					}
				}

				if ( context.m_destSection2 && m_internalState.m_pendingNextSection == context.m_destSection2 )
				{
					state = IS_ChangingSection;

					return LogicTick( state, context );
				}
				else if ( !context.m_destSection2 && m_internalState.m_pendingNextSection )
				{
					state = IS_ChangingSection;

					return LogicTick( state, context );
				}
				else if ( ( context.m_destSection2 && m_internalState.m_currentSection != context.m_destSection2 ) || m_internalState.m_planId == -1 )
				{
					ChangeSection( context.m_destSection2, true, context.m_checkFlow );

					state = IS_ChangingSection;

					return LogicTick( state, context );
				}
				else if ( m_sectionLoader.FindPlanById( m_internalState.m_planId )->HasElements() )
				{
					state = IS_CheckElement;

					return LogicTick( state, context );
				}
				else
				{
					// Wait
					return true;
				}

				break;
			}


		case IS_ChangingSection:
			{
				LOG_SCENE_SEBUG_INFO( TXT("IS_ChangingSection") );

				SCENE_ASSERT( m_internalState.m_pendingNextSection );
				SCENE_ASSERT( context.m_events.Empty() );

				// DIALOG_TOMSIN_TODO - na razie ten krap, musze przeniesc kod z FinishChangingSection i OnTick 
				// bo teraz sa cross dependencje i jedno nie dziala bez drugiego na zmiane...
				m_sectionLoader.OnTick( context.m_timeDelta );

				Bool finished = false;

				if ( FinishChangingSection() )
				{
					finished = true;
				}

				if ( !finished )
				{
					m_sectionLoader.OnTick( context.m_timeDelta );

					if ( FinishChangingSection() )
					{
						finished = true;
					}
				}

				if ( finished )
				{
					SCENE_ASSERT( m_internalState.m_currentSection );
					SCENE_ASSERT( !m_internalState.m_pendingNextSection );
					//SCENE_ASSERT( !m_internalState.m_currentFlowElement );

					m_internalState.m_planId = m_sectionLoader.GetPlayingPlanId( m_internalState.m_currentSection );
					m_internalState.m_timer = 0.f;

					state = IS_GoToNextElement;

					return LogicTick( state, context );
				}

				SCENE_ASSERT( m_sectionLoader.m_asyncLoading );

				// Wait
				running = false;
				return true;
			}


		case IS_RequestGoToNextSection:
			{
				SCENE_ASSERT( context.m_events.Empty() );
				SCENE_ASSERT( !context.m_checkFlow );

#ifndef NO_EDITOR_EVENT_SYSTEM
				SEvents::GetInstance().QueueEvent( CNAME( SectionFinished ), CreateEventData( m_internalState.m_currentSection ) );
#endif

				if ( WantsToGoToNextSection() )
				{
					state = IS_GoToNextSection;

					return LogicTick( state, context );
				}
				else
				{
					// Wait
					return true;
				}
			}


		case IS_GoToNextSection:
			{
				LOG_SCENE_SEBUG_INFO( TXT("IS_GoToNextSection") );

				SCENE_ASSERT( context.m_events.Empty() );
				SCENE_ASSERT( !context.m_checkFlow );

				const CStorySceneLinkElement* link = NULL;
				CStorySceneSectionPlayingPlan* plan = m_sectionLoader.FindPlanById( m_internalState.m_planId );
				if ( plan )
				{
					if ( m_internalState.m_currentFlowElement )
					{
						if ( m_internalState.m_currentFlowElement->IsA< CStorySceneSection >() )
						{
							link = GetSectionNextElement( m_internalState.m_currentFlowElement );
						}
						else
						{
							link = m_internalState.m_currentFlowElement->GetNextElement();
						}
					}
					else
					{
						link = GetSectionNextElement( plan->GetSection() );
					}

					if ( link )
					{
						context.m_destSection2 = ( link != NULL ) ? EvaluateControlChain( link, m_internalState, false ) : NULL;
						if ( context.m_destSection2 )
						{
							state = IS_CheckSection;

							return LogicTick( state, context );
						}
						else
						{
							ChangeSection( NULL, true, false );

							state = IS_Init;

							return LogicTick( state, context );
						}
					}
				}

				//break;
				SCENE_ASSERT( 0 );
				return false;
			}

		case IS_CheckElement:
			{
				LOG_SCENE_SEBUG_INFO( TXT("IS_CheckElement") );

				SCENE_ASSERT( context.m_events.Empty() );

				// DIALOG_TOMSIN_TODO
				// Is this element proper for this time???
				CStorySceneSectionPlayingPlan* plan = m_sectionLoader.FindPlanById( m_internalState.m_planId );
				IStorySceneElementInstanceData* currElem = plan->GetCurrElement();

				if ( context.m_requestRestart )
				{
					context.m_requestRestart = false;
					if ( currElem && currElem->AllowSeek() )
					{
						state = IS_SeekTo;
						return LogicTick( state, context );
					}
					else if ( currElem )
					{
						currElem->Stop();
					}

					state = IS_RestartAndGoTo;
					return LogicTick( state, context );
				}
				else if ( context.m_forceTime )
				{
					Float offset = 0.f;
					if ( plan->CalcCurrElemTimeOffset( context.m_forcedTime, offset ) )
					{
						SCENE_ASSERT( currElem );
						SCENE_ASSERT( currElem->IsRunning() );

						if ( !currElem->IsRunning() )
						{
							// WTF???
							currElem->Play();
						}

						const Float newTimeDelta = offset - currElem->GetCurrentTime();

						if ( newTimeDelta > 0.f )
						{
							context.m_forceTime = false;
							context.m_forcedTime = 0.f;
							context.m_timeDelta = newTimeDelta;

							CSeekEventFilter filter;
							context.m_filter = &filter;

							state = IS_PlayElement;

							//plan->GetCurrElement()->Seek( offset );  we already have timedelta = offset

							return LogicTick( state, context );
						}
						else if ( context.m_forceTime && newTimeDelta < 0.f && currElem && currElem->AllowSeek() )
						{
							state = IS_SeekTo;
							return LogicTick( state, context );
						}
						else if ( newTimeDelta != 0.f )
						{
							if ( currElem )
							{
								currElem->Stop();
							}

							state = IS_RestartAndGoTo;

							return LogicTick( state, context );
						}
					}
					else
					{
						if ( currElem )
						{
							currElem->Stop();
						}

						state = IS_RestartAndGoTo;

						return LogicTick( state, context );
					}
				}
				else
				{
					if ( m_interceptionHelper.OnCheckElement( currElem ) )
					{
						state = IS_GoToNextElement;

						return LogicTick( state, context );
					}
					else if ( currElem )
					{
						if ( currElem->IsRunning() )
						{
							state = IS_PlayElement;

							return LogicTick( state, context );
						}
						else
						{
							// Wait
							return true;
						}
					}
					else
					{
						state = IS_GoToNextElement;

						return LogicTick( state, context );
					}
				}

				break;
			}


		case IS_RestartAndGoTo:
			{
				SCENE_ASSERT( context.m_forceTime );
				SCENE_ASSERT( context.m_events.Empty() );

				CStorySceneSectionPlayingPlan* plan = m_sectionLoader.FindPlanById( m_internalState.m_planId );
				if ( plan->GetCurrElement() )
				{
					SCENE_ASSERT( !plan->GetCurrElement()->IsRunning() );
				}

				GetSceneDisplay()->HideAllDebugComments();
				ResetAllSceneEntitiesState();

				plan->GoToFirstElement();

				CStorySceneEventsCollector collector;

				while ( plan->GetCurrElement() )
				{
					Float offset = 0.f;

					const Bool isInRange = plan->CalcCurrElemTimeOffset( context.m_forcedTime, offset );
					if ( isInRange || ( !isInRange && !plan->HasNextElement() ) )
					{
						SCENE_ASSERT( offset >= 0.f );

						const Float newTimeDelta = offset;

						context.m_forceTime = false;
						context.m_forcedTime = 0.f;
						context.m_timeDelta = newTimeDelta;
						
						context.m_events = collector;

						m_sectionLoader.EnsureNextElementsHaveSpeeches( plan );
						
						plan->GetCurrElement()->Play();
						//plan->GetCurrElement()->Seek( offset ); we already have timedelta = offset

						state = IS_PlayElement;

						return LogicTick( state, context );
					}
					else
					{
						CollectAllEvents( plan->GetCurrElement(), collector );

						plan->GoToNextElement();
					}
				}

				SCENE_ASSERT( 0 );

				// Keep going
				return LogicTick( state, context );
			}


		case IS_SeekTo:
			{
				CStorySceneSectionPlayingPlan* plan = m_sectionLoader.FindPlanById( m_internalState.m_planId );
				IStorySceneElementInstanceData* currElem = plan->GetCurrElement();

				GetSceneDisplay()->HideAllDebugComments();
				ResetAllSceneEntitiesState();

				Float offset = 0.f;
				if ( plan->CalcCurrElemTimeOffset( context.m_forcedTime, offset ) )
				{
					CStorySceneEventsCollector collector;
					CSeekEventFilter filter;
					CollectSceneEvents( plan, 0.f, offset, collector, &filter, nullptr );
					context.m_events = collector;
					context.m_filter = &filter;
					
					context.m_forceTime = false;
					context.m_forcedTime = 0.f;
					context.m_timeDelta = 0.f;				

					state = IS_PlayElement;
					plan->GetCurrElement()->Seek( offset );
					return LogicTick( state, context );
				}
		
				SCENE_ASSERT( 0 );
				break;
			}

		case IS_EmptyInterception:
			{
				if ( m_interceptionHelper.TickEmptyInterception() )
				{
					return true;
				}

				state = IS_GoToNextElement;
				return LogicTick( state, context );
			}

		case IS_GoToNextElement:
			{
				LOG_SCENE_SEBUG_INFO( TXT("IS_GoToNextElement") );

				SCENE_ASSERT( context.m_events.Empty() );

				//SCENE_ASSERT( !m_internalState.m_plan->GetCurrElement() );

				CStorySceneSectionPlayingPlan* plan = m_sectionLoader.FindPlanById( m_internalState.m_planId );

				Bool changedState = false;
				if ( m_interceptionHelper.OnGoToNextElement( changedState ) )
				{
					if ( changedState )
					{
						state = IS_ChangingSection;
						return LogicTick( state, context );
					}
					return true;
				}

				if ( plan->HasNextElement() )
				{
					IStorySceneElementInstanceData* elem = plan->GoToNextElement();

					m_sectionLoader.EnsureNextElementsHaveSpeeches( plan );

					elem->Play();

					state = IS_PlayElement;

					return LogicTick( state, context );
				}
				else
				{
					state = IS_RequestGoToNextSection;

					return LogicTick( state, context );
				}
				break;
			}


		case IS_PlayElement:
			{
				LOG_SCENE_SEBUG_INFO( TXT("IS_PlayElement") );

				CStorySceneSectionPlayingPlan* plan = m_sectionLoader.FindPlanById( m_internalState.m_planId );
				IStorySceneElementInstanceData* element = plan->GetCurrElement();

				SCENE_ASSERT( element );
				SCENE_ASSERT( element->IsRunning() );

				if ( !IsGameplay() )
				{
					UpdateInfoAboutSkipping();	
				}

				/*if ( m_stats.OnTickElement() )
				{
					m_stats.PrintStats( GetName() );

					const Float time = m_stats.GetTimeToFirstEventTickMS();
					if ( time > 1000.f && GScreenLog ) // 1 sec
					{
						GScreenLog->PerfWarning( time, TXT("STORY SCENE"), TXT("Slow story scene loading '%ls'"), GetName().AsChar() );
					}
				}*/

				// Tick events
				Float cachedElementTime = element->GetCurrentTime();
				const Bool keepRunning = TickElementAndEvents( element, context.m_timeDelta, context.m_events, context.m_filter );
#ifndef NO_EDITOR
				if ( element->IsLooped() && cachedElementTime > element->GetCurrentTime() )
				{
					SEvents::GetInstance().QueueEvent( RED_NAME( SectionFinished ), CreateEventData( m_internalState.m_currentSection ) );
				}
#endif
				if ( !keepRunning )
				{
					FireEvents( context.m_events );

					if ( plan->HasNextElement() || plan->HasNextSection() )
					{
						element->Stop();

						// TODO - zmniejsz timeDelta o to ile skonsumowal poprzedni element

						state = IS_GoToNextElement;

						return LogicTick( state, context );
					}
					else if ( m_interceptionHelper.OnPlayElement() )
					{
						state = IS_ChangingSection;
						return LogicTick( state, context );
					}
					else
					{
						context.m_requestSceneEnd = true;

						state = IS_CheckSceneEnd;

						return LogicTick( state, context );
					}
				}
				else
				{
					FireEvents( context.m_events );

					running = false;

					return true;
				}

				break;
			}
		}
	}

	return true;
}

void CStoryScenePlayer::CollectAllEvents( CStorySceneSectionPlayingPlan* plan, CStorySceneEventsCollector& collector, const CStorySceneEventsCollectorFilter* filterA, const CStorySceneEventsCollectorFilter* filterB ) const
{
	const Uint32 numElements = plan->m_sectionInstanceData.m_elemData.Size();
	for ( Uint32 i=0; i<numElements; ++i )
	{
		const IStorySceneElementInstanceData* elem = plan->m_sectionInstanceData.m_elemData[ i ];

		CollectAllEvents( plan, elem, collector, filterA, filterB );
	}
}

void CStoryScenePlayer::CollectAllEvents( CStorySceneSectionPlayingPlan* playingPlan, const IStorySceneElementInstanceData* elem, CStorySceneEventsCollector& collector, const CStorySceneEventsCollectorFilter* filterA, const CStorySceneEventsCollectorFilter* filterB ) const
{
	SCENE_ASSERT( elem );

	if ( m_debugger )
	{
		m_debugger->FireAllStartEvents( elem );
	}

	const Float elemDurationMS = elem->GetDuration();

	//SCENE_ASSERT( elem->GetCurrentTime() == 0.f );

	Float prevPositionMS = elem->GetStartTime();
	Float currPositionMS = prevPositionMS + elemDurationMS;

	CollectSceneEvents( playingPlan, prevPositionMS, currPositionMS, collector, filterA, filterB );
}

void CStoryScenePlayer::CollectAllEvents( const IStorySceneElementInstanceData* elem, CStorySceneEventsCollector& collector )
{
	CStorySceneSectionPlayingPlan* playingPlan = m_sectionLoader.FindPlanById( m_internalState.m_planId );
	CollectAllEvents( playingPlan, elem, collector, nullptr, nullptr );
}

Bool CStoryScenePlayer::TickElementAndEvents( IStorySceneElementInstanceData* elem, Float timeDelta, CStorySceneEventsCollector& collector, const CStorySceneEventsCollectorFilter* filter )
{
	SCENE_ASSERT( elem );
	SCENE_ASSERT( m_internalState.m_currentSection );
	
	CStorySceneSectionPlayingPlan* plan = m_sectionLoader.FindPlanById( m_internalState.m_planId );
	SCENE_ASSERT( plan );

	const Float elemStartTimeMS = elem->GetStartTime();
	const Float elemDurationMS = elem->GetDuration();
	const Float elemEndTimeMS = elemStartTimeMS + elemDurationMS;

	// 1. Skip functionality
	m_internalState.m_justSkippedElement = elem->IsSkipped();
	if ( m_internalState.m_justSkippedElement )
	{
		const Float currTime = elem->GetCurrentTime();
		timeDelta = elemDurationMS - currTime;
	}

	// 2. Mark progress
	Float prevPositionMS = elem->GetCurrentTime() + elem->GetStartTime();

	Bool ret = elem->Tick( timeDelta );

	if ( m_debugger )
	{ 
		m_debugger->BeginTickElement( elem, timeDelta );
	}

	Float currPositionMS = elem->GetStartTime() + elem->GetCurrentTime();

	const Bool timeForward = timeDelta >= 0.f;
	if ( !timeForward )
	{
		SCENE_ASSERT( prevPositionMS >= currPositionMS );
		Swap( prevPositionMS, currPositionMS );
	}

	SCENE_ASSERT( timeForward );

	CSkipEventFilter skipFilter;
	const CStorySceneEventsCollectorFilter* skipEvtFilter = m_internalState.m_justSkippedElement ? &skipFilter : nullptr;

	// 3. Process events per scene element
	if ( prevPositionMS <= currPositionMS )
	{
		// this is the usual case
		CollectSceneEvents( plan, prevPositionMS, currPositionMS, collector, filter, skipEvtFilter );

		SendCustomSceneSignals( plan, prevPositionMS, currPositionMS, m_internalState.m_justSkippedElement, collector );
	}
	else
	{
		// prevPositionMS may be greater than currPositionMS only for looped elements
		SCENE_ASSERT( elem->IsLooped() );

		// For looped elements, prevPositionMS > currPositionMS when we finish one loop and start
		// another one. To properly collect events in such case, we first collect events at the end
		// of one loop and then those at the beginning of next loop.
		CollectSceneEvents( plan, prevPositionMS, elemEndTimeMS, collector, filter, skipEvtFilter );
		ResetEventsPlayState( *plan, elem->GetElement() );
		CollectSceneEvents( plan, elemStartTimeMS, currPositionMS, collector, filter, skipEvtFilter );

		SendCustomSceneSignals( plan, prevPositionMS, elemEndTimeMS, m_internalState.m_justSkippedElement, collector );
		SendCustomSceneSignals( plan, elemStartTimeMS, currPositionMS, m_internalState.m_justSkippedElement, collector );
	}

	// Choice - todo find proper place
	m_internalState.m_hasPendingChoice = elem->GetElement()->IsA< CStorySceneChoice >();

	// 4. Tick element
	//Bool ret = elem->Tick( timeDelta );
	if ( !ret || m_internalState.m_justSkippedElement )
	{
		SCENE_ASSERT__FIXME_LATER( !ret );

		ret = false;
	}

	if ( m_debugger )
	{
		m_debugger->EndTickElement( elem, timeDelta );
	}

	return ret;
}

/*
Resets play state of all events from playing plan.

This function resets play state of all events from playing plan so they report that they have "not yet started" and "not yet finished".
*/
void CStoryScenePlayer::ResetEventsPlayState( const CStorySceneSectionPlayingPlan& plan )
{
	for( const CStorySceneEvent* evt : plan.m_sectionInstanceData.m_evts )
	{
		evt->SetInstanceStarted( *plan.m_sectionInstanceData.m_data, false );
		evt->SetInstanceFinished( *plan.m_sectionInstanceData.m_data, false );
	}
}

void CStoryScenePlayer::ResetEventsPlayState( const CStorySceneSectionPlayingPlan& plan, const CStorySceneElement* forElement )
{
	for( const CStorySceneEvent* evt : plan.m_sectionInstanceData.m_evts )
	{
		if ( evt->GetSceneElement() == forElement )
		{
			evt->SetInstanceStarted( *plan.m_sectionInstanceData.m_data, false );
			evt->SetInstanceFinished( *plan.m_sectionInstanceData.m_data, false );
		}		
	}
}

void CStoryScenePlayer::FireEvents( CStorySceneEventsCollector& collector )
{
	m_eventsExecutor.Execute( this, collector, m_debugger );

	collector.Clear();
}

void CStoryScenePlayer::CollectSceneEvents( CStorySceneSectionPlayingPlan* plan, Float prevPositionMS, Float currPositionMS, CStorySceneEventsCollector& collector, const CStorySceneEventsCollectorFilter* filterA, const CStorySceneEventsCollectorFilter* filterB ) const
{
	SCENE_ASSERT( plan );
	if ( !plan )
	{
		return;
	}

	CStorySceneSectionPlayingPlan::InstanceData& instance = plan->m_sectionInstanceData;
	SCENE_ASSERT( instance.m_data );

	for ( Uint32 j=0; j<instance.m_evts.Size(); ++j )
	{
		const CStorySceneEvent* sceneEvt = instance.m_evts[ j ];
		if ( !sceneEvt )
		{
			SCENE_ASSERT( sceneEvt );
			continue;
		}

		if ( sceneEvt->IsMuted() )
		{
			continue;
		}

		if ( ( filterA && filterA->DoFilterOut( sceneEvt ) ) || ( filterB && filterB->DoFilterOut( sceneEvt ) ) )
		{
			continue;
		}

		InternalFireEvent( sceneEvt, instance, prevPositionMS, currPositionMS, collector );
	}
}

void CStoryScenePlayer::InternalFireEvent( const CStorySceneEvent* sceneEvt, CStorySceneSectionPlayingPlan::InstanceData& instance, const Float prevPositionMS, const Float currPositionMS, CStorySceneEventsCollector& collector ) const
{
	SCENE_ASSERT( instance.m_data );

	Float startTimeMS = 0.f;
	Float endTimeMS = 0.f;
	Float durationMS = 0.f;
	sceneEvt->GetEventInstanceData( *instance.m_data, startTimeMS, endTimeMS, durationMS );

	SCENE_ASSERT( startTimeMS >= 0.f );
	SCENE_ASSERT( endTimeMS >= 0.f );
	SCENE_ASSERT( durationMS >= 0.f );

	// process event if it hasn't already finished
	if( currPositionMS >= startTimeMS && !sceneEvt->HasInstanceFinished( *instance.m_data ) )
	{
		SStorySceneEventTimeInfo timeInfo;
		timeInfo.m_timeLocal = Min( currPositionMS - startTimeMS, durationMS );
		timeInfo.m_timeAbs = Min( currPositionMS, endTimeMS );
		timeInfo.m_timeDelta = currPositionMS - prevPositionMS;
		timeInfo.m_progress = ( durationMS > 0.0f? Clamp( timeInfo.m_timeLocal / durationMS, 0.0f, 1.0f ) : 1.0f );
		SCENE_ASSERT( timeInfo.m_timeAbs >= timeInfo.m_timeLocal );

		if( !sceneEvt->HasInstanceStarted( *instance.m_data ) )
		{
			sceneEvt->EventStart( *instance.m_data, this, collector, timeInfo );
		}

		sceneEvt->EventProcess( *instance.m_data, this, collector, timeInfo );

		if( currPositionMS >= endTimeMS )
		{
			sceneEvt->EventEnd( *instance.m_data, this, collector, timeInfo );
		}
	}
}

void CStoryScenePlayer::SendCustomSceneSignals( const CStorySceneSectionPlayingPlan* plan, const Float prevPositionMS, const Float currPositionMS, Bool wasSkipCalled, CStorySceneEventsCollector& collector )
{
	if ( !wasSkipCalled )
	{
		const Uint32 numTimeMarkers = plan->m_sectionInstanceData.m_cachedLineMarkers.Size();
		for ( Uint32 i=0; i<numTimeMarkers; ++i )
		{
			const CStorySceneSectionPlayingPlan::LineMarker& marker = plan->m_sectionInstanceData.m_cachedLineMarkers[ i ];
			const Float preTime = marker.m_startTimeMS - ActorPrepareToTalk_PRE_TIMEOFFSET;
			const Float postTime = marker.m_endTimeMS;

			// 1. Actors prepare to talk
			if ( preTime > 0.f && preTime > prevPositionMS && preTime <= currPositionMS )
			{
				StorySceneEventsCollector::ActorPrepareToTalk evt( marker.m_actorId );
				collector.AddEvent( evt );
			}

			// 2. Actors finish talk
			if ( postTime > prevPositionMS && postTime <= currPositionMS )
			{
				StorySceneEventsCollector::ActorFinishTalk evt( marker.m_actorId );
				collector.AddEvent( evt );
			}
		}
	}
	else
	{
		// 3. Add cloth reset after user skipped element
		for ( auto it = GetSceneActors().Begin(), end = GetSceneActors().End(); it != end; ++it ) // TODO: remove this for-loop for this type of data
		{
			const CName& actorName = (*it).m_first;
			if ( actorName )
			{
				StorySceneEventsCollector::ActorResetClothAndDangles evt( nullptr, actorName );
				evt.m_eventTimeAbs = currPositionMS;
				evt.m_eventTimeLocal = 0.f;
				evt.m_forceRelaxedState = true;
				
				collector.AddEvent( evt );
			}
		}
	}

	// 4. Check for any upcoming camera markers, and prefetch the new frame.
	if ( !IsGameplayNow() )
	{
		for ( const auto& marker : plan->m_sectionInstanceData.m_cachedCameraMarkers )
		{
			const Float preTime = marker.m_eventTime - CameraPrefetch_PRE_TIMEOFFSET;

			if ( preTime > prevPositionMS && preTime <= currPositionMS )
			{
				StorySceneEventsCollector::CameraPrefetch evt;
				evt.m_camMatrixSceneLocal = marker.m_cameraMatrixSceneLocal;
				evt.m_camFov = marker.m_cameraFov;
				collector.AddEvent( evt );
			}
		}
	}

	// 5.
	//...
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
