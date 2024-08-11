#include "build.h"
#include "questScenePlayer.h"

#include "questsSystem.h"

#include "../core/configVar.h"
#include "storySceneSystem.h"
#include "storySceneInput.h"
#include "commonGame.h"

#ifdef WAIT_FOR_CONTEXT_DIALOGS_LOADING
const Float CQuestScenePlayer::WAIT_FOR_CONTEXT_DIALOG_TIMEOUT = 30.f;
#endif

CQuestScenePlayer::CQuestScenePlayer( const TSoftHandle< CStoryScene >& scene, 
									EStorySceneForcingMode forcingMode,
									const String& inputName,
									Bool interrupt,
									Bool shouldFadeOnLoading )
	: m_scene( &scene )
	, m_forcingMode( forcingMode )
	, m_inputName( inputName )
	, m_interrupt( interrupt )
	, m_sceneController( NULL )
	, m_playingPhase( PF_NONE )
	, m_loadedScene( NULL )
	, m_scenePlayingFailure( false )
	, m_startTime( 0.f )
	, m_endTime( 0.f )
	, m_shouldFadeOnLoading( shouldFadeOnLoading )
	, m_isFadeSet( false )
#ifdef WAIT_FOR_CONTEXT_DIALOGS_LOADING
	, m_waitForContextDialogTimer( 0.f )
#endif
#ifdef USE_STORY_SCENE_LOADING_STATS
	, m_startedLoading(false)
#endif
{

}

CQuestScenePlayer::~CQuestScenePlayer() 
{
	Reset( SCR_PLAYER_DESTROYED );

	m_outputName = CName::NONE;
	m_playingPhase = PF_NONE;

	m_scene->Release();
}

Bool CQuestScenePlayer::IsLoaded() const
{
	return m_loadedScene != NULL;
}

String CQuestScenePlayer::GetSceneName() const
{
	if ( m_loadedScene )
	{
		return m_loadedScene->GetSceneName();
	}
	else
	{
		return TXT( "<<Scene not loaded yet>>" );
	}
}

void CQuestScenePlayer::Play( String inputName /*= String::EMPTY */ )
{
	// initialize the statistics
	m_lastSectionName = TXT( "" );
	m_startTime = (Float)GGame->GetEngineTime();
	m_endTime = 0;

	// initialize the player's state
	m_outputName = CName::NONE;
	m_playingPhase = PF_INITIALIZATION_REQUEST;

	if ( inputName != String::EMPTY )
	{
		m_inputName = inputName;
	}

	if ( GCommonGame->GetSystem< CQuestsSystem >() )
	{
		GCommonGame->GetSystem< CQuestsSystem >()->GetStoriesManager()->Schedule( *this );
	}
}

void CQuestScenePlayer::EnableSceneStart()
{
	if ( m_playingPhase == PF_INITIALIZATION_REQUEST )
	{
		m_playingPhase = PF_INITIALIZE;
		m_scenePlayingFailure = false;
	}
	else
	{
		WARN_GAME( TXT( "Invalid quest scene state transition request" ) );
	}
}

RED_DEFINE_STATIC_NAME( OnPreSceneInvulnerability );

void CQuestScenePlayer::Execute()
{	
	if ( m_playingPhase == PF_NONE || m_playingPhase == PF_OVER )
	{
		return;
	}

	// load the scene
	if ( !m_loadedScene )
	{
		BaseSoftHandle::EAsyncLoadingResult loadingResult = m_scene->GetAsync( true );

		if ( loadingResult == BaseSoftHandle::ALR_Loaded )
		{
#ifdef WAIT_FOR_CONTEXT_DIALOGS_LOADING
			CStoryScene* scene = m_scene->Get();
			if ( scene ) // Waiting for context dialogs
			{
				const CQuestsSystem* qs = GCommonGame->GetSystem< CQuestsSystem >();
				if ( qs->ShouldWaitForContextDialogs( scene ) )
				{
					m_waitForContextDialogTimer += GEngine->GetLastTimeDelta();
					if ( m_waitForContextDialogTimer > WAIT_FOR_CONTEXT_DIALOG_TIMEOUT )
					{
						SCENE_ASSERT( !(m_waitForContextDialogTimer > WAIT_FOR_CONTEXT_DIALOG_TIMEOUT) );
						SCENE_LOG( TXT("CQuestScenePlayer::Execute - Waiting for context dialog for story scene timeout occured! - Scene '%ls'"), scene->GetDepotPath().AsChar() );
					}
					else
					{
						SCENE_LOG( TXT("CQuestScenePlayer::Execute - Waiting for context dialog for story scene '%ls'"), scene->GetDepotPath().AsChar() );
						// Wait
						return;
					}
				}
			}

			m_loadedScene = scene;
			m_waitForContextDialogTimer = 0.f;
#else
			m_loadedScene = m_scene->Get();
#endif
		}

#ifdef USE_STORY_SCENE_LOADING_STATS 

		if ( !m_startedLoading )
		{
			m_startedLoading = true;
			m_loadingTimer.ResetTimer();
		}
		if ( m_loadedScene )
		{
			RED_LOG( SceneLoading, TXT("****************************************************") );
			RED_LOG( SceneLoading, TXT("Story Scene Loading Stats") );
			RED_LOG( SceneLoading, TXT("Loading scene file") );
			RED_LOG( SceneLoading, TXT("Resource loading time : [%.5f] ms"), (Float)m_loadingTimer.GetTimePeriodMS() );
			RED_LOG( SceneLoading, TXT("****************************************************") );
		}
#endif
		if ( !m_loadedScene )
		{
			if ( m_shouldFadeOnLoading && !m_isFadeSet )
			{
				GGame->StartFade( false, String::Printf( TXT( "Quest_ShouldFadeOnLoading - %ls" ), m_scene->GetPath().AsChar() ) );
				GGame->SetFadeLock( TXT( "Quest_ShouldFadeOnLoading" ) );

				if( CPlayer* player = GCommonGame->GetPlayer() )
				{
					player->CallEvent( CNAME( OnPreSceneInvulnerability ), true );
				}

				m_isFadeSet = true;
			}

			return;
		}
	}
	

	// wait for a signal from the quest scenes manager that tells the player
	// to start playing the scene
	if ( m_playingPhase != PF_INITIALIZE )
	{
		return;
	}

	if ( m_scenePlayingFailure == false  )
	{
		if ( m_sceneController == NULL )
		{
			if ( StartPlayingScene() == false )
			{
				m_scenePlayingFailure = true;
				return;
			}
			m_playingPhase = PF_PLAYING;
		}
	}
	else
	{
		Reset( SCR_TIMEDOUT );
		Finish();
	}
}

Bool CQuestScenePlayer::StartPlayingScene()
{
	// before we can start playing a scene, we need to have a scene controller - try creating one
	if ( !m_sceneController )
	{
		if ( m_inputName.Empty() == true )
		{
			WARN_GAME( TXT( "Trying to play scene %s with no input name" ), m_loadedScene->GetSceneName().AsChar() );
			m_scenePlayingFailure = true;
			return false;
		}

		const CStorySceneInput* sceneInput = m_loadedScene->FindInput( m_inputName );
		if ( !sceneInput )
		{
			WARN_GAME( TXT( "Input %s is not one of the scene's %s inputs" ), m_inputName.AsChar(), m_loadedScene->GetSceneName().AsChar() );
			m_scenePlayingFailure = true;
			return false;
		}

		// try playing the scene
		TDynArray< THandle< CEntity > > contextActors;
		contextActors.PushBack( GCommonGame->GetPlayer() );

		EArbitratorPriorities scenePriority = sceneInput->IsGameplay()
			? BTAP_AboveCombat
			: BTAP_FullCutscene;
		
		m_sceneController = GCommonGame->GetSystem< CStorySceneSystem >()->PlayInputExt( sceneInput, 
			m_forcingMode, scenePriority, &contextActors, NULL, true );

		if ( m_sceneController )
		{
			m_sceneController->SetListener( this );
		}
	}

	if ( !m_sceneController )
	{
		return false;
	}

	return true;
}

void CQuestScenePlayer::Stop()
{
	Reset( SCR_STOPPED );

	Finish();
	m_outputName = CName::NONE;

}

void CQuestScenePlayer::OnStorySceneMappingDestroyed( CStorySceneController * mapping )
{
	if ( IsOver() == true )
	{
		return;
	}

	if( CPlayer* player = GCommonGame->GetPlayer() )
	{
		player->CallEvent( CNAME( OnPreSceneInvulnerability ), true );
	}

	ASSERT ( mapping == m_sceneController );

	m_outputName = mapping->GetActivatedOutput();
	Reset( mapping->GetCompletionResult() );
	Finish();
}

void CQuestScenePlayer::OnStorySceneMappingStopped( CStorySceneController * mapping )
{
	ASSERT ( mapping == m_sceneController );

	m_outputName = mapping->GetActivatedOutput();
	Reset( mapping->GetCompletionResult() );
	
	// That was a deprecated feature allowing for restarting quest gameplay scene after actors
	// were despawned and spawned again. It appears that it causes a major performance issues
	// and feature itself has been abandoned for now. Still, we might need it in the future -
	// it may be required due to open world nature
	/*if ( mapping->GetCompletionResult() != SCR_ACTOR_DESPAWN || mapping->IsGameplayScene() == false )
	{
		Finish();
	}
	else
	{
		Play( m_inputName );
	}*/

	Finish();
	
}

const CStorySceneInput* CQuestScenePlayer::GetInput() const
{
	const CStoryScene* scene = m_scene->Get();
	ASSERT( scene );
	if ( scene == NULL )
	{
		return NULL;
	}

	const CStorySceneInput* sceneInput = scene->FindInput( m_inputName );
	return sceneInput;
}

void CQuestScenePlayer::Reset( SceneCompletionReason reason )
{
	// set the end time
	m_endTime = (Float)GGame->GetEngineTime();

	// Temp fix for rare crash - TODO: Find the root cause
	/* 
	if ( m_sceneController )
	{
		CStoryScenePlayer * player = m_sceneController->GetPlayer();
		if ( player && player->GetCurrentSection() )
		{
			m_lastSectionName = player->GetCurrentSection()->GetName();
		}
	}
	*/

	// unregister the name
	if ( GCommonGame->GetSystem< CQuestsSystem >() )
	{
		GCommonGame->GetSystem< CQuestsSystem >()->GetStoriesManager()->Remove( *this, reason );
	}

	if ( m_sceneController )
	{
		m_sceneController->SetListener( NULL );

		if( CStorySceneSystem* sss = GCommonGame->GetSystem< CStorySceneSystem >() )
		{
			if ( sss->CanStopScene( m_sceneController ) )
			{
				sss->StopScene( m_sceneController, reason );
			}
		}

		m_sceneController = NULL;
	}
}

Float CQuestScenePlayer::GetTotalTime() const
{
	if ( m_endTime > 0 )
	{
		return m_endTime - m_startTime;
	}
	else
	{
		return (Float)GGame->GetEngineTime() - m_startTime;
	}
}

void CQuestScenePlayer::Finish()
{
	m_playingPhase = PF_OVER;
	m_loadedScene = NULL;
	m_scene->Release();
}

void CQuestScenePlayer::Reinitialize()
{
	m_playingPhase = PF_NONE;
}

void CQuestScenePlayer::Pause( Bool pause )
{
	if ( m_sceneController )
	{
		m_sceneController->Pause( pause );
	}
}

Bool CQuestScenePlayer::HasLoadedSection() const
{
	if ( m_sceneController && m_sceneController->GetPlayer() )
	{
		return m_sceneController->GetPlayer()->HasLoadedSection();
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////

CQuestScenesManager::CQuestScenesManager()
{
}

void CQuestScenesManager::Schedule( CQuestScenePlayer& player )
{
	// check if the scene hasn't already been scheduled
	for ( TDynArray< CQuestScenePlayer* >::iterator it = m_scheduledScenes.Begin();
		it != m_scheduledScenes.End(); ++it )
	{
		if ( *it == &player )
		{
			// yup - it's already scheduled
			return;
		}
	}

	// check if the scene hasn't already been loaded
	for ( TDynArray< CQuestScenePlayer* >::iterator it = m_loadedScenes.Begin();
		it != m_loadedScenes.End(); ++it )
	{
		if ( *it == &player )
		{
			// yup - it's already loaded
			return;
		}
	}

	// check if the scene isn't already active
	for ( TDynArray< CQuestScenePlayer* >::iterator it = m_activeScenes.Begin();
		it != m_activeScenes.End(); ++it )
	{
		if ( *it == &player )
		{
			// yup - it's already active
			return;
		}
	}

	// an interrupting scene should be scheduled before any other scene - it has
	// a priority of being played
	if ( player.IsInterrupting() )
	{
		m_scheduledScenes.Insert( 0, &player );
	}
	else
	{
		m_scheduledScenes.PushBack( &player );
	}

	Notify();
}

void CQuestScenesManager::Remove( CQuestScenePlayer& player, SceneCompletionReason reason )
{
	Bool removed = m_scheduledScenes.Remove( &player );
	removed |= m_loadedScenes.Remove( &player );
	
	if ( m_activeScenes.Remove( &player ) )
	{
		removed = true;

#ifndef NO_EDITOR_DEBUG_QUEST_SCENES
		// recreate currently used resources list
		TDynArray< CName > allVoicetags;
		Uint32 count = m_activeScenes.Size();
		for ( Uint32 i = 0; i < count; ++i )
		{
			ASSERT( m_activeScenes[ i ]->IsLoaded() );
			const CStorySceneInput* sceneInput = m_activeScenes[ i ]->GetInput();
			ASSERT( sceneInput != NULL );
			TDynArray< CName > sceneVoicetags;
			const_cast< CStorySceneInput* >( sceneInput )->CollectVoiceTags(sceneVoicetags);

			allVoicetags.PushBackUnique( sceneVoicetags );
		}

		m_activeScenesVoicetags.SetTags( allVoicetags );
		if ( GCommonGame->GetPlayer() )
		{
			m_activeScenesVoicetags.SubtractTag( GCommonGame->GetPlayer()->GetVoiceTag() );
		}
#endif
	}

	// add information that the scene was completed, and how did it finish
	if ( removed )
	{
		NotifySceneCompleted( player, reason );
		Notify();
	}
}

void CQuestScenesManager::Tick()
{
	ProcessScheduled();
	ProcessLoaded();
	Notify();
}

void CQuestScenesManager::ProcessScheduled()
{
	TDynArray< CQuestScenePlayer* > cache = m_scheduledScenes;
	for ( TDynArray< CQuestScenePlayer* >::iterator it = cache.Begin();
		it != cache.End(); ++it )
	{
		// verify the scene is loaded
		if ( (*it)->IsLoaded() )
		{
			m_scheduledScenes.Remove( *it );

			// the scene is loaded - put it to the scheduled scenes, respecting 
			// the priority ( interrupting scenes go in first )
			if ( (*it)->IsInterrupting() )
			{
				m_loadedScenes.Insert( 0, *it );
			}
			else
			{
				m_loadedScenes.PushBack( *it );
			}
		}
	}
}

void CQuestScenesManager::ProcessLoaded()
{
	while( !m_loadedScenes.Empty() )
	{
		CQuestScenePlayer* player = m_loadedScenes[ 0 ];
		ASSERT( player, TXT( "Invalid quest scene player instance" ) );

		// verify that a valid input with which the scene should start was specified
		const CStorySceneInput* input = player->GetInput();
		if ( input == NULL )
		{
			WARN_GAME( TXT( "Scene with no input was scheduled to be played in a quest" ) );
			Remove( *player, SCR_INVALID_SCENE );
			continue;
		}

		// check if all necessary conditions for the scene to be started were met
		if ( !CanSceneBeStarted( *player ) )
		{
			break;
		}
		
		Start( player );
		m_loadedScenes.Remove( player );
	}
}

void CQuestScenesManager::Start( CQuestScenePlayer* player )
{
	player->EnableSceneStart();
	m_activeScenes.PushBack( player );

#ifndef NO_EDITOR_DEBUG_QUEST_SCENES
	// update the list of occupied voicetags
	const CStorySceneInput* input = player->GetInput();
	if ( input != NULL )
	{
		TDynArray< CName > sceneVoicetags;
		const_cast< CStorySceneInput* >( input )->CollectVoiceTags(sceneVoicetags);

		for ( TDynArray< CName >::const_iterator voicetagIter = sceneVoicetags.Begin();
			voicetagIter != sceneVoicetags.End(); ++voicetagIter )
		{
			m_activeScenesVoicetags.AddTag( *voicetagIter );
		}

		if ( GCommonGame->GetPlayer() )
		{
			m_activeScenesVoicetags.SubtractTag( GCommonGame->GetPlayer()->GetVoiceTag() );
		}	
	}

#endif
}

Bool CQuestScenesManager::CanSceneBeStarted( CQuestScenePlayer& player ) const
{
	return true;
}

void CQuestScenesManager::Reset()
{
	m_scheduledScenes.Clear();
	m_loadedScenes.Clear();

	// Buffer the scenes
	TDynArray< CQuestScenePlayer* > cachedPlayers = m_activeScenes;
	Uint32 count = cachedPlayers.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		cachedPlayers[ i ]->Stop();
		NotifySceneCompleted( *cachedPlayers[ i ], SCR_SYSTEM_RESET );
	}
	m_activeScenes.Clear();

#ifndef NO_EDITOR_DEBUG_QUEST_SCENES
	m_activeScenesVoicetags.Clear();
#endif

	Notify();
}

void CQuestScenesManager::AttachListener( IQuestScenesManagerListener& listener )
{
	m_listeners.PushBackUnique( &listener );
	listener.Notify( *this );
}

void CQuestScenesManager::DetachListener( IQuestScenesManagerListener& listener )
{
	m_listeners.Remove( &listener );
}

void CQuestScenesManager::Notify()
{
	Uint32 count = m_listeners.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		m_listeners[ i ]->Notify( *this );
	}
}

void CQuestScenesManager::NotifySceneCompleted( CQuestScenePlayer& player, SceneCompletionReason reason )
{
    Uint32 count = m_listeners.Size();
    for ( Uint32 i = 0; i < count; ++i )
    {
        m_listeners[ i ]->NotifySceneCompleted( player, reason );
    }
}

