/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

class CQuestThread;
class CStorySceneInput;
class IStoryScenePlaybackListener;
struct SSceneChoice;
class CQuestScenePlayer;
class CQuestScenesManager;
class CQuestExternalScenePlayer;
class CQuestCutsceneCondition;
class CGameStateManager;
class CBehaviorGraph;
class CQuestInputCondition;
class CStorySceneChoice;
struct SUsedFastTravelEvent;

#include "storySceneComment.h"

class IQuestSystemListener
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Gameplay );
public:
	virtual ~IQuestSystemListener() {}

	virtual void OnQuestStarted( CQuestThread* thread, CQuest& quest ) = 0;
	virtual void OnQuestStopped( CQuestThread* thread ) = 0;
	virtual void OnSystemPaused( bool paused ) = 0;

	virtual void OnThreadPaused( CQuestThread* thread, bool paused ) = 0;
	virtual void OnAddThread( CQuestThread* parentThread, CQuestThread* thread ) = 0;
	virtual void OnRemoveThread( CQuestThread* parentThread, CQuestThread* thread ) = 0;
	virtual void OnAddBlock( CQuestThread* thread, const CQuestGraphBlock* block ) = 0;
	virtual void OnRemoveBlock( CQuestThread* thread, const CQuestGraphBlock* block ) = 0;
	virtual void OnBlockInputActivated( CQuestThread* thread, const CQuestGraphBlock* block ) = 0;
};

class CQuestsSystem : public IGameSystem, public IGameSaveSection, public IInputListener
{
	DECLARE_ENGINE_CLASS( CQuestsSystem, IGameSystem, 0 );


private:
	struct SDialogInfo
	{
		TSoftHandle< CStoryScene >							m_injectedScene;
		TSoftHandle< CStoryScene >							m_targetScene;
		IStoryScenePlaybackListener*						m_listener;
		THashMap< String, SSceneInjectedChoiceLineInfo >	m_choices; // Do we need hashmap?

		SDialogInfo()
			: m_listener( nullptr )
		{}

		SDialogInfo( const TSoftHandle< CStoryScene > _injectedScene 
			, const TSoftHandle< CStoryScene > _targetScene
			, IStoryScenePlaybackListener* _listener ) 
			: m_injectedScene( _injectedScene )
			, m_targetScene( _targetScene )
			, m_listener( _listener )
		{}

		Bool operator==( const SDialogInfo& rhs) const
		{
			return m_injectedScene == rhs.m_injectedScene && m_targetScene == rhs.m_targetScene && m_listener == rhs.m_listener;
		}
	};

	typedef THashMap< CQuestThread*, THandle< CQuest > >		QuestsMap;
	typedef THashMap< CName, CQuestScenePlayer* >				InteractionDialogs;

private:
	Bool													m_active;
	Bool													m_stable;
	Bool													m_latchThreadCountStable;
	Int32													m_pauseCount;
	QuestsMap												m_questsMap;
	Uint32													m_deadPhaseHackfixCounter;

	TDynArray< SDialogInfo >								m_contextDialogs;

	TDynArray< CQuestInteractionCondition* >				m_interactionListeners;
	TDynArray< CQuestReactionCondition* >					m_reactionListeners;
	TDynArray< CQuestCutsceneCondition* >					m_cutscenesListener;
	TDynArray< CQuestInputCondition* >						m_inputListener;
	TDynArray< IQuestSystemListener*	>					m_listeners;

	TDynArray< CQuestThread* >								m_threads;
	TDynArray< CQuestThread* >								m_threadsToAdd;
	TDynArray< CQuestThread* >								m_threadsToRemove;
	TDynArray< CQuestThread* >								m_threadsToStabilize;

	TDynArray< THandle< CQuest > >							m_runningQuests;

	CQuestScenesManager*									m_scenesManager;

	CQuestExternalScenePlayer*								m_interactionsDialogPlayer;
	CQuestExternalScenePlayer*								m_scriptedDialogPlayer;

public:
	CQuestsSystem();
	virtual ~CQuestsSystem();

	virtual void Initialize() override;
	virtual void Shutdown() override;

	// Activates the system
	virtual void Activate();

	// Deactivates the system
	virtual void Deactivate();

	// Starts execution of a new quest in a dedicated quest thread
	CQuestThread* Run( const THandle< CQuest >& quest, CQuestStartBlock* startBlock );

	// Is quest running on dedicated quest thread
	Bool IsQuestRunning( const THandle< CQuest >& quest );

	// Get dedicated quest thread
	CQuestThread* GetQuestThread( const THandle< CQuest >& quest );

#ifndef NO_EDITOR_GRAPH_SUPPORT
	//! return true if any part of quest is present in currently running quests
	Bool IsDuplicate( const THandle< CQuest >& quest ) const;
#endif //! NO_EDITOR_GRAPH_SUPPORT

	// Stops execution of a quest
	void Stop( CQuestThread* thread );

	// Immediately stops all running quests
	void StopAll();

	// Returns an array of all currently running quest threads
	RED_INLINE const TDynArray< CQuestThread* >& GetRunningThreads() const { return m_threads; }

	// Gathers all running threads
	void GetAllRunningThreads( TDynArray< CQuestThread* >& threads ) const;

	// Updates all active quest threads
	virtual void Tick( Float timeDelta );

	// Pauses the execution of all quests
	void Pause( bool enable );

	// Checks if the system is paused
	RED_INLINE Bool IsPaused() const { return m_pauseCount > 0; }

	// Did the quests system make relevant progress this tick
	RED_INLINE Bool IsStable() const { return m_stable && m_threadsToAdd.Empty() && m_threadsToRemove.Empty(); }
	void ResetStability();

	// CObject implementation
	virtual void OnSerialize( IFile& file );

	// ------------------------------------------------------------------------
	// Listeners
	// ------------------------------------------------------------------------
	// Attaches a new listener to the system
	void AttachListener( IQuestSystemListener& listener );

	// Detaches a listener from the system
	void DetachListener( IQuestSystemListener& listener );

	// ------------------------------------------------------------------------
	// Context dialogs
	// ------------------------------------------------------------------------
	// Registers a new context dialog choice option the scenes can take advantage of
	void RegisterContextDialog( TSoftHandle< CStoryScene > injectedScene, TSoftHandle< CStoryScene > targetScene, IStoryScenePlaybackListener* listener, const THashMap< String, SSceneInjectedChoiceLineInfo >& choices );

	// Unregisters a context dialog choice option. 'listener' can be nullptr, we use it only for debug reason
	void UnregisterContextDialog( TSoftHandle< CStoryScene > injectedScene, TSoftHandle< CStoryScene > targetScene, IStoryScenePlaybackListener* listener );

	// Retrieves a list of quest choices that can be used for particular actors
	void GetContextDialogChoices( TDynArray< SSceneChoice >& choices, const CStoryScene* targetScene, TDynArray< IStoryScenePlaybackListener* >* player, const CStorySceneChoice* targetChoice );

	void GetContextDialogActorData( CStoryScene* targetScene, TDynArray< TPair< const CStorySceneActor*, const CStorySceneVoicetagMapping* > >& out );
	
	void GetContextDialogsForScene( THandle< CStoryScene > targetScene, TDynArray< THandle < CStoryScene > >& outScenes ) const;

	Bool ShouldWaitForContextDialogs( const CStoryScene* targetScene ) const;

	// ------------------------------------------------------------------------
	// Scenes management
	// ------------------------------------------------------------------------
	RED_INLINE CQuestScenesManager* GetStoriesManager() { return m_scenesManager; }

	RED_INLINE CQuestExternalScenePlayer* GetInteractionDialogPlayer() { return m_interactionsDialogPlayer; }
	RED_INLINE CQuestExternalScenePlayer* GetScriptedDialogPlayer() { return m_scriptedDialogPlayer; }



	// ------------------------------------------------------------------------
	// Events
	// ------------------------------------------------------------------------
	void AttachInteractionListener( CQuestInteractionCondition& listener );
	void DetachInteractionListener( CQuestInteractionCondition& listener );
	void OnInteractionExecuted( const String& eventName, CEntity* owner );

	void AttachReactionListener( CQuestReactionCondition& listener );
	void DetachReactionListener( CQuestReactionCondition& listener );
	void OnReactionExecuted( CNewNPC* npc, CInterestPointInstance* interestPoint );

	Bool AreThereAnyDialogsForActor( const CActor& actor );

	void AttachCutsceneListener( CQuestCutsceneCondition& listener );
	void DetachCutsceneListener( CQuestCutsceneCondition& listener );
	void OnCutsceneEvent( const String& csName, const CName& csEvent );

	void AttachGameInputListener( CQuestInputCondition& listener );
	void DetachGameInputListener( CQuestInputCondition& listener );

	virtual Bool OnGameInputEvent( const SInputAction & action ) override;

public:
	//! Initialize at game start, called directly in StartGame, but should always be implemented
	virtual void OnGameStart( const CGameInfo& gameInfo );

	//! Shutdown at game end, called directly in EndGame, but should always be implemented
	virtual void OnGameEnd( const CGameInfo& gameInfo );

	virtual void OnWorldEnd( const CGameInfo& gameInfo );

	//! Save game
	virtual bool OnSaveGame( IGameSaver* saver );

public:
	// -------------------------------------------------------------------------
	// Debug 
	// -------------------------------------------------------------------------
	virtual Bool IsNPCInQuestScene( const CNewNPC* npc ) const;

public:
	RED_INLINE Uint32 GetDeadPhaseHackfixCounter() const { return m_deadPhaseHackfixCounter; }
	RED_INLINE void BumpDeadPhaseHackfixCounter() { ++m_deadPhaseHackfixCounter; }

private:
	// -------------------------------------------------------------------------
	// Notifications
	// -------------------------------------------------------------------------
	void NotifySystemPaused();
	void NotifyQuestStarted( CQuestThread* thread );
	void NotifyQuestStopped( CQuestThread* thread );

private:
	void LoadGame( IGameLoader* loader );
	bool RestoreFromSession();
	void ManageThreads();
	void RemoveThread( CQuestThread* thread );
	void RemoveQuestIfNotUsed( CQuest* quest );
	friend class CQuestThread;

	ASSIGN_GAME_SYSTEM_ID( GS_QuestsSystem )
};

BEGIN_CLASS_RTTI( CQuestsSystem )
	PARENT_CLASS( IGameSystem )
	PROPERTY( m_runningQuests )
END_CLASS_RTTI()
