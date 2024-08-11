/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "storySceneVoicetagMapping.h"

class CStoryScene;
class CStoryScenePlayer;

// This class can play a single instance of a story scene.
// The scene is played asynchronously - calling 'Play'
// only makes a request to the quest scenes manager that 
// a particular scene is played - it's up to the manager
// to decide when to grant that request.

//#define WAIT_FOR_CONTEXT_DIALOGS_LOADING

class CQuestScenePlayer : public CStorySceneController::IListener
{
private:
	enum EPlayingPhase
	{
		PF_NONE,
		PF_INITIALIZATION_REQUEST,
		PF_INITIALIZE,
		PF_PLAYING,
		PF_OVER,
	};

private:
	const TSoftHandle< CStoryScene >*					m_scene;
	EStorySceneForcingMode								m_forcingMode;
	String												m_inputName;
	Bool												m_interrupt;
	Bool												m_shouldFadeOnLoading;
	Bool												m_stopPlayerMovement;
	
	CStorySceneController*								m_sceneController;
	EPlayingPhase										m_playingPhase;
	CName												m_outputName;
	CStoryScene*										m_loadedScene;

	Bool												m_scenePlayingFailure;
	Bool												m_isFadeSet;

	// statistics
	Float												m_startTime;			//!< time the play method was called
	Float												m_endTime;				//!< time the scene's finished being played
	String												m_lastSectionName;		//!< name of the last known section the scene was playing
#ifdef WAIT_FOR_CONTEXT_DIALOGS_LOADING
	Float												m_waitForContextDialogTimer;
	static const Float									WAIT_FOR_CONTEXT_DIALOG_TIMEOUT;
#endif

#ifdef USE_STORY_SCENE_LOADING_STATS
	Bool												m_startedLoading;
	CTimeCounter										m_loadingTimer;
#endif

public:
	CQuestScenePlayer( const TSoftHandle< CStoryScene >& scene, 
		EStorySceneForcingMode forcingMode,  const String& inputName, 
		Bool interrupt, Bool shouldFadeOnLoading );
	~CQuestScenePlayer();

	// returns the name of the played scene
	String GetSceneName() const;

	// returns the name of the scene input the player will use
	RED_INLINE const String& GetInputName() const { return m_inputName; }

	// Tells whether the scene should interrupt other scenes blocking it
	RED_INLINE Bool IsInterrupting() const { return m_interrupt; }

	// Schedules the scene to be played
	void Play( String inputName = String::EMPTY );

	// Executes the scene playing order
	void Execute();

	// Stops the scene
	void Stop();

	// Tells if the scene is loaded
	Bool IsLoaded() const;

	void Finish();

	void Reinitialize();

	void Pause( Bool pause );
	Bool HasLoadedSection() const;

	RED_INLINE Bool IsReadyForStart() const { return m_playingPhase == PF_NONE; }

	// Tells if the scene is currently playing
	RED_INLINE Bool IsPlaying() const { return m_playingPhase == PF_PLAYING; }

	// Tells if the scene has finished
	RED_INLINE Bool IsOver() const { return m_playingPhase == PF_OVER; }

	// Returns the output the scene exited with
	RED_INLINE const CName& GetOutput() const { return m_outputName; }

	// Returns the input the scene started with
	const CStorySceneInput* GetInput() const;

	// Allows the scene to be started
	void EnableSceneStart();

	//! CStorySceneController::IListener impl
	virtual void OnStorySceneMappingDestroyed( CStorySceneController * mapping );
	virtual void OnStorySceneMappingStopped( CStorySceneController * mapping );

	// Get the scene controller
	RED_INLINE CStorySceneController* DEBUG_GetSceneController() const { return m_sceneController; }

	// -------------------------------------------------------------------------
	// Statistics
	// -------------------------------------------------------------------------
	Float GetTotalTime() const;
	RED_INLINE const String& GetLastSectionName() const { return m_lastSectionName; }

private:
	void Reset( SceneCompletionReason reason );
	Bool StartPlayingScene();
};


///////////////////////////////////////////////////////////////////////////////

class IQuestScenesManagerListener;

// The manager schedules the sequence in which the requested scenes
// will be played. The order relies upon the voicetags each scene
// uses as well as on whether the particular scene is a gameplay scene
// (those are played immediately).
class CQuestScenesManager
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Gameplay );

private:
	TDynArray< CQuestScenePlayer* >				m_scheduledScenes;
	TDynArray< CQuestScenePlayer* >				m_loadedScenes;
	TDynArray< CQuestScenePlayer* >				m_activeScenes;

#ifndef NO_EDITOR_DEBUG_QUEST_SCENES
	TagList												m_activeScenesVoicetags;
#endif

	TDynArray< IQuestScenesManagerListener* >	m_listeners;

public:
	CQuestScenesManager();

	// schedules the scene for loading
	void Schedule( CQuestScenePlayer& player );

	// removes a scene
	void Remove( CQuestScenePlayer& player, SceneCompletionReason reason );

	// removes all scenes
	void Reset();

	// ticks the manager
	void Tick();

	RED_INLINE const TDynArray< CQuestScenePlayer* >& GetScheduledScenes() const { return m_scheduledScenes; }
	RED_INLINE const TDynArray< CQuestScenePlayer* >& GetActiveScenes() const { return m_activeScenes; }

#ifndef NO_EDITOR_DEBUG_QUEST_SCENES
	RED_INLINE const TagList& GetActiveTags() const { return m_activeScenesVoicetags; }
#endif

	void AttachListener( IQuestScenesManagerListener& listener );
	void DetachListener( IQuestScenesManagerListener& listener );

private:
	void Start( CQuestScenePlayer* player );
	Bool CanSceneBeStarted( CQuestScenePlayer& player ) const;
	void InterruptBlockingScenes( CQuestScenePlayer& player );

	// -------------------------------------------------------------------------
	// Processing
	// -------------------------------------------------------------------------
	
	// Loads the scenes scheduled for processing
	void ProcessScheduled();

	// Runs through the loaded scenes, checking which can be started
	void ProcessLoaded();

	// Waits for the active scenes to finish
	void ProcessPlayed();

	// -------------------------------------------------------------------------
	// Notifications
	// -------------------------------------------------------------------------
	void Notify();
	void NotifySceneCompleted( CQuestScenePlayer& player, SceneCompletionReason reason );
};


class IQuestScenesManagerListener
{
public:
	virtual ~IQuestScenesManagerListener() {}

	virtual void Notify( CQuestScenesManager& mgr ) = 0;

	virtual void NotifySceneCompleted( CQuestScenePlayer& player, SceneCompletionReason reason ) = 0;
};
