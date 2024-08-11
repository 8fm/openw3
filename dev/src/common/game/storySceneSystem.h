/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "gameSystem.h"
#include "storySceneForcingMode.h"
#include "storyScene.h"
#include "storySceneChoiceLineAction.h"
#include "storySceneAnimationCache.h"
#include "storySceneExtractedLineData.h"
#include "behTreeArbitratorPriorities.h"
#include "storySceneAnimationList.h"
#include "storySceneDisplayInterface.h"
#include "storySceneIncludes.h"
#include "storyScenePlayer.h"
#include "../engine/inputListener.h"
#include "storySceneVoicetagMapping.h"


class CStorySceneController;
class IStoryScenePlaybackListener;
class ISceneActorInterface;
class CHudInstance;
class CStorySceneActorMap;
class CActorSpeechQueue;
struct ScenePlayerPlayingContext;
struct StorySceneControllerSetup;

struct SSceneVideo
{
	enum EWhiteScreen : Int8
	{
		eWhiteScreen_None,
		eWhiteScreen_Start,
		eWhiteScreen_End,
	};

	EWhiteScreen	m_whiteScreenCtrl;
};

/// Top level system to play game scenes
class CStorySceneSystem : public IGameSystem, public IInputListener
{
	DECLARE_ENGINE_CLASS( CStorySceneSystem, IGameSystem, 0 );

	struct SQueuedSceneData
	{
		CStorySceneController*	scene;
		Bool					isQuestScene;

		SQueuedSceneData( CStorySceneController* _scene = NULL, Bool _isQuestScene = false )
			: scene( _scene ), isQuestScene( _isQuestScene )
		{}

		Bool operator==( const SQueuedSceneData& rhs ) const
		{
			return scene == rhs.scene;
		}
	};

	struct SFinishedSceneBlendData
	{
		SFinishedSceneBlendData()
			: m_progress( 0.f )
			, m_blendTime( -1.f )
			, m_disableDof( false )
		{}

		Float	m_progress;
		Float	m_blendTime;
		Bool	m_disableDof;
		SCameraLightsModifiersSetup m_envSetupToBlendFrom;
		TDynArray< CStorySceneController::ScenePropInfo >	 m_sceneLightEntities;

		void Clear()
		{
			m_progress = 0.f;
			m_blendTime = -1.f;
			m_disableDof = false;
			m_envSetupToBlendFrom.SetModifiersIdentity( false );
			m_sceneLightEntities.Clear();
		}
	};

	struct SCameraState
	{
		SCameraState() { Reset(); }

		Float	m_cameraLeftRight;
		Float	m_cameraUpDown;
		Float	m_cameraLeftRightAcc;
		Float	m_cameraUpDownAcc;
		Bool	m_enabled;

		void Reset()
		{
			m_cameraLeftRight = 0.f;
			m_cameraUpDown = 0.f;
			m_cameraLeftRightAcc = 0.f;
			m_cameraUpDownAcc = 0.f;
		}
	};

	// Scene system remembers last values given by most recently accessed scene randomizers
	static const Uint32	RANDOMIZER_STATES_SIZE = 4;
	static const Uint8	RANDOMIZER_COOLDOWN;
	struct SSceneRandomizerState
	{
		Uint32	m_randomizerId;
		Uint8	m_lastValue;
		Uint8	m_cooldown;

		SSceneRandomizerState() : m_randomizerId( 0 ), m_lastValue( 255 ), m_cooldown( 0 ) {}
	};

protected:
	const static Float								SCENE_UPDATE_INTERVAL;	//!< Time that must pass before potential scenes update (seconds)
	const static Uint32								NUM_SCENE_CAMERAS;		//!< Maximum number of scene cameras

	Float											m_sceneUpdateTimeout;	//!< Time till potential scenes will be updated (seconds)
	TDynArray< ScenePlayerPlayingContext >			m_contextsToStart;
	TDynArray< CStorySceneController* >				m_scenesToStart;		//!< List of mappings that are locked and being prepared for being played
	TDynArray< CStorySceneController* >				m_scenesToFree;			//!< List of mappings that are waiting to be deleted
	
	TDynArray< THandle< CStoryScenePlayer > >		m_activeScenes;			//!< Active scene players
	TagList											m_activeScenesVoicetags;//!< Voicetags used by all active scenes

	TDynArray< const CStorySceneLine* >				m_lines;				//!< Visible text lines
	TDynArray< SSceneChoice >						m_choices;				//!< Visible choice lines
	Int32											m_highlightedChoice;	//!< Currently selected choice line
	String											m_currentOneLiner;		//!< Last visible one liner
	CHudInstance*									m_dialogHud;			//!< HUD for the dialog system
	Bool											m_dialogHudShown;		//!< Status if dialog HUD visibility
	String											m_lastLine;
	ISceneActorInterface*							m_lastLineActor;		//!< For GC - we remember actor who should be speaking, so we do not hide lines which were overlapped
	Float											m_lastSkipLineTime;
	CStorySceneActorMap*							m_actorMap;				//!< Caches mappings of voicetags->CActors
	IStorySceneDisplayInterface*					m_sceneDisplay;
	CStorySceneAnimationList						m_animationList;
	
	SCameraState									m_sceneCameraState;
	CCamera*										m_sceneCamera;

	SSceneRandomizerState							m_randomizerStates[ RANDOMIZER_STATES_SIZE ];
	SFinishedSceneBlendData							m_finScenesBlendData;

	THashMap< String, SSceneVideo >					m_sceneVideoMap;

protected:

#ifndef RED_FINAL_BUILD
	mutable Float debugTextFadeoutTimer;
	mutable bool  fadingOutDebugText;
	static const Float DEFAULT_DEBUG_TEXT_FADEOUT_TIME;
	
	mutable String					m_dbgErrorState;	//!< Used only for debugging purposes.
	mutable String					m_cutsceneDebugText;
#endif

public:
	CStorySceneSystem();
	~CStorySceneSystem();

	virtual void Initialize() override;
	virtual void Shutdown() override;

	virtual void OnSerialize( IFile& file );
	
	void ReloadAnimationsList();

public:
	// DIALOG_TOMSIN_TODO input ma byc const
	CStorySceneController* PlayInput( const CStorySceneInput* input, const StorySceneControllerSetup& setup );

	Bool CanStopScene( CStorySceneController* c ) const;
	void StopScene( CStorySceneController* c, const SceneCompletionReason& reason );

public:
	Bool GetSceneVideo( const String& videoPath, SSceneVideo& outSceneVideo ) { return m_sceneVideoMap.Find( videoPath.ToLower(), outSceneVideo ); }

public:
	static Bool ExtractVoicesetDataFromSceneInput( const CStorySceneInput* input, const CName& voicetag, CActorSpeechQueue* speechQueue );
	static Bool ExtractChatDataFromSceneInput( const CStorySceneInput* input, TDynArray< SExtractedSceneLineData >& lines );
	
	// DIALOG_TOMSIN_TODO: scany a gameplay do rozkminy
	Bool PlayAsVoiceset( const CStorySceneInput* input, CActor* actor );

	// DIALOG_TOMSIN_TODO: to do wywalenia wszysto ma byc przez PlayInput
	CStorySceneController* PlayInputExt( const CStorySceneInput* input, EStorySceneForcingMode forceMode = SSFM_ForceNothing, 
		EArbitratorPriorities priority = BTAP_Idle , const TDynArray< THandle< CEntity > > * contextActors = NULL, CNode * suggestedPosition = NULL, Bool isQuestScene = false );

	void OnSceneMappingStopped( CStorySceneController * scene, Bool hasBeenPlaying );

	Bool CanPlaySceneInput( const TSoftHandle< CStoryScene >& sceneHandle, const String& inputName ) const; // Fairly ligthweigth method of checking if the scene can play. It is less cumbersome than trying to play scene and checking result
	Bool CanPlaySceneInput( const CStorySceneInput* input ) const;												// Fairly ligthweigth method of checking if the scene can play. It is less cumbersome than trying to play scene and checking result

public:
	//! Start scene
	CStoryScenePlayer* StartScene( CStorySceneController & controller, const ScenePlayerPlayingContext& context );

	//Returns true if there is an active scene that wants to block music trigger events
	bool ActiveScenesBlockMusicEvents();

	//! Set hud instance
	RED_INLINE void SetHUD( CHudInstance* hud ) { m_dialogHud = hud; } 

	RED_INLINE Bool IsDialogHudShown() const { return m_dialogHudShown; }

	RED_INLINE IStorySceneDisplayInterface* GetSceneDisplay() const { ASSERT( m_sceneDisplay ); return m_sceneDisplay; }

	//! Get players array
	RED_INLINE const TDynArray< THandle< CStoryScenePlayer > >& GetScenePlayers() { return m_activeScenes; }

	RED_INLINE Bool IsCurrentlyPlayingAnyScene() const { return !m_activeScenes.Empty(); }
	Bool IsCurrentlyPlayingAnyCinematicScene() const;

	void UnregisterPlaybackListenerFromActiveScenes( IStoryScenePlaybackListener* listener );

	Bool HasActorInAnyPlayingScene( const CActor* actor ) const;

public:
	void Stop( CWorld* world );
	void StopAll();

	virtual void OnGameStart( const CGameInfo& gameInfo );

	virtual void OnGameEnd( const CGameInfo& gameInfo );

	virtual void OnWorldStart( const CGameInfo& gameInfo );

	virtual void OnWorldEnd( const CGameInfo& gameInfo );

	//! Update active scene
	virtual void Tick( Float timeDelta );

	// Generate editor rendering fragments
	virtual void OnGenerateDebugFragments( CRenderFrame* frame );

	//! User input signal ( choices, skipping )
	virtual void Signal( EStorySceneSignalType inputSignalType, Int32 inputSignalValue );
	
	//! Process game input
	virtual Bool OnGameInputEvent( const SInputAction & action );

	//! Attach to game world
	void AttachToWorld();

	//! Detach from game world
	void DetachFromWorld();

	RED_INLINE CStorySceneActorMap* GetActorMap() const { return m_actorMap; }

	RED_INLINE const CStorySceneAnimationList& GetAnimationList() const { return m_animationList; }

private:
	//! Clear data used by dialogs with player
	void StopForPlayer();
	void DestroyScene( CStoryScenePlayer* p );

protected:
	//! Process camera input
	Bool ProcessCameraInput( const CName& event, Float value );

	// XXX Hud related methods should be moved to hud instance or some other class;
public:
	//! Show/Hide dialog HUD
	void ToggleDialogHUD( Bool hud );

	Bool IsDialogHUDAvailable() const;

	void OnCanSkipChanged( Bool canSkip );

	//! Display set of choices
	void SetChoices( const TDynArray< SSceneChoice >& choices, Bool alternativeUI );

	void ShowChoiceTimer( Float time );

	void HideChoiceTimer();

	void Fade( const String& sceneName, const String& reason, Bool fadeIn, Float duration = 0.2f, const Color& color = Color::BLACK );

	void Blackscreen( const String& sceneName, Bool enable, const String& reason );

	//! Add dialog line to display
	void AddDialogLine( const CStorySceneLine* line );

	//! Remove dialog line to display
	void RemoveDialogLine( const CStorySceneLine* line );

	void ShowDialogText( const String& text, ISceneActorInterface* actor, EStorySceneLineType lineType, Bool alternativeUI );

	void HideDialogText( ISceneActorInterface* actor, EStorySceneLineType lineType );

	void ShowPreviousDialogText( const String& text );

	void HidePreviousDialogText();

	void HideAllDialogTexts();

	const String& GetLastDialogText();

	void SetSceneCameraMovable( Bool flag );

	Bool IsSceneCameraMovable() const;

	CCamera* GetSceneCamera() const;

	CCamera* GetActiveSceneCamera() const;

	void FlushLoadingVisibleData();

public:
	Uint8	GetLastRandomizerValue( Uint32 randomizerId );
	void	SetLastRandomizerValue( Uint32 randomizerId, Uint8 randomValue );

	void SetFinishedSceneBlendData( SFinishedSceneBlendData& data );
	Bool IsBlendingFinishedScene();
protected:
	void CleanupFinishedSceneBlend();
	void ProcessFinishedSceneBlend( Float timeDelta );

// debug methods

#ifndef RED_FINAL_BUILD
public:	
	virtual void SetErrorState( const String &description ) const { m_dbgErrorState = description; }
	virtual const String &GetErrorState() const { return m_dbgErrorState; }

	CFont * m_debugScreenTextFont;
	CFont * m_debugVideoTextFont;

	void SetDebugScreenText(const String & arg) const;
	void AddDebugScreenText(const String & arg) const;
	void ClearDebugScreenText(bool withoutDelay = false) const;
	const String &GetDebugScreenText() const; 

protected:
	void EraseDebugScreenText() const;
#endif

#ifndef NO_EDITOR
public:
	const String& GetLangRefName() const;
#endif

public:	
	//! Called when an interaction has executed
	void OnInteractionExecuted( const String& name, CEntity* owner );
	void FreeFinished();

protected:
	//! Update scenes to start
	void UpdateScenesToStart( Float timeDelta );

	//! Update playing scenes
	void UpdatePlayingScenes( Float timeDelta );

	//! Update scene camera
	void UpdateSceneCamera( Float timeDelta );

protected:
	CCamera*	CreateSceneCamera() const;

public:
	static CCamera*	CreateSceneCamera( CWorld* world );

private:
	void InitSceneVideoMap();

private:
	void funcIsSkippingLineAllowed( CScriptStackFrame& stack, void* result );
	void funcSendSignal( CScriptStackFrame& stack, void* result );
	void funcGetChoices( CScriptStackFrame& stack, void* result );
	void funcGetHighlightedChoice( CScriptStackFrame& stack, void* result );
	void funcPlayScene( CScriptStackFrame& stack, void* result );
	void funcIsCurrentlyPlayingAnyScene( CScriptStackFrame& stack, void* result );

	ASSIGN_GAME_SYSTEM_ID( GS_StoryScene );
};

BEGIN_CLASS_RTTI( CStorySceneSystem );
	PARENT_CLASS( IGameSystem );
	PROPERTY( m_activeScenes );
	PROPERTY( m_actorMap );
	NATIVE_FUNCTION( "SendSignal", funcSendSignal );
	NATIVE_FUNCTION( "GetChoices", funcGetChoices );
	NATIVE_FUNCTION( "GetHighlightedChoice", funcGetHighlightedChoice );
	NATIVE_FUNCTION( "PlayScene", funcPlayScene );
	NATIVE_FUNCTION( "IsCurrentlyPlayingAnyScene", funcIsCurrentlyPlayingAnyScene );
	NATIVE_FUNCTION( "IsSkippingLineAllowed", funcIsSkippingLineAllowed );
END_CLASS_RTTI();
