/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "storySceneIncludes.h"
#include "storySceneDirector.h"
#include "storyScene.h"
#include "storySceneSection.h"
#include "storySceneElement.h"
#include "storySceneEventsCollector.h"
#include "storySceneEventsExecutor.h"
#include "storyScenePlayerStats.h"

#include "storySceneEventDuration.h"
#include "storySceneSectionPreload.h"
#include "storySceneOutput.h"
#include "storySceneSectionPlayingPlan.h"
#include "storySceneInterceptionHelper.h"
#include "storySceneFlowCondition.h"

class IStoryScenePlaybackListener;
class IStorySceneElementInstanceData;
class CStoryScene;
class CStorySceneController;
class CStorySceneEvent;
class CStorySceneInput;
class CStorySceneScript;
class CStorySceneVideoSection;

/// "empty" user signal
#define NO_SCENE_USER_SIGNAL		(-1)

class CStorySceneSectionPreloadJob;
class IStorySceneDisplayInterface;
class IStorySceneDebugger;
class ICutsceneWorldInterface;

class IRenderVideo;
struct SVideoParams;
class IRenderFramePrefetch;

struct ScenePlayerPlayingContext
{
	CWorld*							m_world;
	CPlayer*						m_playerEntity;
	CClass*							m_playerClass;
	IStorySceneDisplayInterface*	m_display;
	IStorySceneDebugger*			m_debugger;
	ICutsceneWorldInterface*		m_csWorldInterface;
	Bool							m_spawnAllActors;
	Bool							m_asyncLoading;
	Bool							m_useApprovedVoDurations;	// True - use durations of approved VO, false - use durations of local VO.
	void*							m_hackFlowCtrl;

	ScenePlayerPlayingContext() 
		: m_world( NULL )
		, m_playerEntity( NULL )
		, m_playerClass( ClassID< CStoryScenePlayer >() )
		, m_display( NULL )
		, m_debugger( NULL )
		, m_csWorldInterface( NULL )
		, m_spawnAllActors( false )
		, m_asyncLoading( true )
		, m_useApprovedVoDurations( false )
		, m_hackFlowCtrl( nullptr )
	{}

	RED_INLINE Bool UsesGameWorld() const { return GGame && GGame->GetActiveWorld() == m_world; }
};

struct ScenePlayerInputState
{
	const CStorySceneSection*					m_section2;

	Bool						m_timeIsSet2;
	Float						m_time2;

	Bool						m_restart2;

	Bool						m_paused2;

	ScenePlayerInputState() : m_section2( nullptr ), m_timeIsSet2( false ), m_time2( 0.f ), m_paused2( false ), m_restart2( false ) {}

	void RequestTime( Float time ) { m_time2 = time; m_timeIsSet2 = true; }
	void RequestRestart() { m_restart2 = true; }
	void RequestPause( Bool flag ) { m_paused2 = flag; }
	void RequestSection( const CStorySceneSection* s ) { m_section2 = s; }
};

struct SStorySceneDialogsetInstanceCalcBoundDesc
{
	const CStorySceneDialogsetInstance* m_dialogSetInstance;
	EngineTransform m_parentTransform;
	Float m_safeZone;
	Bool m_includeCameras;

	SStorySceneDialogsetInstanceCalcBoundDesc()
		: m_dialogSetInstance( nullptr )
		, m_safeZone( 0.5f )
		, m_includeCameras( false )
	{}
};

/// Player of scenes, using actors
class CStoryScenePlayer : public CEntity, public ICutsceneSceneInterface
{
	DECLARE_ENGINE_CLASS( CStoryScenePlayer, CEntity, 0 );

	typedef  THashMap< CName, THandle< CEntity > > SceneActorMap;

	// DIALOG_TOMSIN_TODO
	friend class CStorySceneDirector;
	friend class CStorySceneSectionPlayingPlan;
	friend class CStorySceneInterceptionHelper;

	enum EInternalState
	{
		IS_Init,
		IS_Intro,
		IS_CheckSceneEnd,
		IS_CheckSection,
		IS_CheckElement,
		IS_ChangingSection,
		IS_GoToNextElement,
		IS_PlayElement,
		IS_RequestGoToNextSection,
		IS_GoToNextSection,
		IS_SkipLine,
		IS_SkipSection,
		IS_RestartAndGoTo,
		IS_EmptyInterception,
		IS_SeekTo,
		IS_Error,
	};

	enum EPendingNextSectionStage
	{
		PNSS_Init,
		PNSS_WaitingForPlan,
		PNSS_PlanIsReady,
		PNSS_StartLoadingTextures,
		PNSS_WaitingForTextures,
		PNSS_TexturesAreReady,
		PNSS_StartHud,
		PNSS_WaitingForHud,
		PNSS_StartNPCTeleport,
		PNSS_WaitingForNPCTeleport,
		PNSS_Finish,
		PNSS_Reset,
	};

	struct TickContext
	{
		const CStorySceneSection*	m_forceSection2;

		Bool						m_forceTime;
		Float						m_forcedTime;

		Bool						m_requestRestart;
		Bool						m_requestSceneRestart;

		const CStorySceneSection*	m_destSection2;
		Float						m_timeDelta;		
		Bool						m_requestSceneEnd;

		Bool						m_checkFlow;

		CStorySceneEventsCollector	m_events;
		const CStorySceneEventsCollectorFilter* m_filter;

		TickContext() : m_destSection2( nullptr ), m_forceSection2( nullptr ), m_timeDelta( 0.f ), m_forceTime( false ), m_forcedTime( 0.f ), m_requestSceneEnd( false ), m_requestRestart( false ), m_requestSceneRestart( false ), m_filter( nullptr ), m_checkFlow( false ){}
	};

	struct InternalFlowState
	{
		Bool							m_isInTick;

		const CStorySceneLinkElement*	m_currentFlowElement;
		const CStorySceneInput*			m_input;
		const CStorySceneOutput*		m_activatedOutput;		// Output the scene activated upon ending
		Int32							m_sectionInputNumber;
		const CStorySceneLinkElement*	m_resumeRequest;
		const CStorySceneLinkElement*	m_selectedChoice;
		const CStorySceneChoice*		m_choiceToReturnTo;
		const CStoryScene*				m_injectedScene;

		const CStorySceneSection*		m_prevSection;
		const CStorySceneSection*		m_currentSection;
		const CStorySceneSection*		m_pendingNextSection;
		EPendingNextSectionStage		m_pendingNextSectionStage;
#ifdef USE_STORY_SCENE_LOADING_STATS
		CTimeCounter					m_pendingNextSectionTimer;
#endif

		Int32							m_planId;
		Float							m_timer;
		Float							m_sceneProgress;
		Int32							m_scheduledNextElements;
		Float							m_timeMultiplier;

		Bool							m_canSkipElement;
		Bool							m_skipLine;
		Bool							m_justSkippedElement;
		Bool							m_requestSectionRestart;
		Bool							m_requestSceneRestart;
		Bool							m_sceneStarted;
		Bool							m_isFrozen;
		Bool							m_hasPendingChoice;
		Bool							m_forcingTimeThisFrame;

		Bool							m_sceneEndSkipped;
		Bool							m_sceneEndFadeSet;
		Float							m_sceneEndTimer;

		TDynArray< const CStorySceneLinkElement* >  m_debugFlowList;
		TDynArray< String >					m_factsAdded;
		TDynArray< String >					m_factsRemoved;

		enum IntroState
		{
			IntroState_Initial,
			IntroState_WaitingForNPCTeleport,
			IntroState_Started,
			IntroState_Finished
		};
		struct Intro
		{
			IntroState					m_state;
			Bool						m_playCutscene;
			Bool						m_applyLookAts;

			Float						m_time;
			const CStorySceneSection*	m_destSection;
			CCutsceneInstance*			m_cutsceneInstance;
			THandle< CActor >			m_vehicle;
			Bool						m_fadeOutStarted;

			Intro();
			Bool IsInProgress() const { return m_state == IntroState_WaitingForNPCTeleport || m_state == InternalFlowState::IntroState_Started; }
			void MarkAsSkipped() { m_state = IntroState_Finished; }
		};

		Intro							m_intro;
		StorySceneCameraState			m_cameraState;

#ifdef USE_SCENE_FLOW_MARKERS
		struct FlowMarker
		{
			const CStorySceneLinkElement*	m_element;
			String							m_desc;
			Uint32							m_outputNum;
		};
		TDynArray< FlowMarker >			m_flowMarkers;
#endif

		InternalFlowState();

		void SetCurrentFlowElement( const CStorySceneLinkElement* e );
		void ResetCurrentFlowElement();

		void SetActivatedOutput( const CStorySceneOutput* o );							// DIALOG_TOMSIN_TODO const
		void ResetActivatedOutput();												// DIALOG_TOMSIN_TODO to nie jest uzywane, dlaczego???

		void SetCurrentSection( const CStorySceneSection* s );
		void ResetCurrentSection();

		void SetPendingNextSection( const CStorySceneSection* s );
		void ResetPendingNextSection();

		void SetSectionInputNumber( Int32 index );
		void ResetSectionInputNumber();

#ifdef USE_SCENE_FLOW_MARKERS
		void AddFlowMarker( const FlowMarker& m );
#endif
	};

	InternalFlowState									m_internalState;
	TDynArray< THandle< CActor > >						m_hiddenNonSceneActors;		//!< Actors hidden while the scene is played

private:
	mutable TDynArray< IRenderVideo* >					m_renderVideos;				//!< Any video playing for storyscene blocks

protected:
	TDynArray< IStoryScenePlaybackListener* >			m_playbackListeners;		//!< Playback listeners
	TDynArray< IStoryScenePlaybackListener* >			m_playbackListenersToAdd;	//!< Listeners to be added
	TDynArray< IStoryScenePlaybackListener* >			m_playbackListenersToRemove;//!< Listeners to be removed
	
	// TODO: cleanup and combine these into 1 THashMap<> at some point
	THashMap< CName, THandle< CEntity > >				m_sceneActorEntities;		//!< Entities used as actors;
	THashMap< CName, THandle< CEntity > >				m_scenePropEntities;		//!< Prop entities;
	THashMap< CName, THandle< CEntity > >				m_sceneEffectEntities;		//!< effect entities;
	THashMap< CName, THandle< CEntity > >				m_sceneLightEntities;		//!< light entities;

	Uint16												m_isPaused;					//!< Scene is paused
	Bool												m_isGameplay;				//!< This is a gameplay scene ( non interactive, text above head )	
	Bool												m_isStopped;				//!< Scene is stopped
	Bool												m_isDestoryed;
	Bool												m_allowTickDuringFade;
	Bool												m_frozenFrameSet;
	Bool												m_blackscreenSet;
	Bool												m_sceneInProfiler;

	TDynArray< THandle< CStoryScene > >					m_injectedScenes;
	THandle< CStoryScene >								m_scene;
	String												m_name;						//!< Name of the story scene ( taken from the file name and the name of the first section )

	
	const CStorySceneScript*							m_scriptBlock;				//!< Current script block we are paused on
	CScriptThread*										m_scriptBlockThread;		//!< Current thread being processed
	CPropertyDataBuffer									m_scriptReturnValue;		//!< Return value from script thread

	CStorySceneDirector									m_sceneDirector;			//!< Actors and cameras helper
	CStorySceneSectionLoader							m_sectionLoader;			//!< Resource loading helper
	THandle< CStoryScene >								m_storyScene;
	CStorySceneController*								m_sceneController;
	CStorySceneEventsExecutor							m_eventsExecutor;
	CStorySceneAnimationContainer						m_animContainer;

	mutable THashMap< const CStorySceneDialogsetInstance*, EngineTransform >	m_referencePlacements;		//!< Per dialog set reference placement

	Bool												m_isPrecached;

	CStorySceneInterceptionHelper						m_interceptionHelper;

	CGameSaveLock										m_saveBlockId;

	TDynArray< THandle< CActor > >						m_actorsWithHiResShadows;	//!> Current actors with HiRes shadows enabled
	TDynArray< THandle< CActor > >						m_actorsWithMimicOn;		//!> Current actors with mimic high enabled
	TDynArray< THandle< CActor > >						m_actorsWithLodOverride;	// List of actor LOD overrides.

	Bool												m_switchedToGameplayCamera;	// Switch to gameplay camera when dialog is done? (don't do so if already done e.g. via CStorySceneEventStartBlendToGameplayCamera event)
	Bool												m_resetGameplayCameraOnOutput;		// Reset gameplay camera before switching to gameplay camera? (don't do so if we want smooth switch from scene camera to gameplay camera)

	Bool												m_didStartBlendingOutLights;

	Bool												m_useApprovedVoDurations; 	// True - use durations of approved VO, false - use durations of local VO.

	THandle< CPlayer >									m_player;
	CWorld*												m_world;
	IStorySceneDisplayInterface*						m_display;
	IStorySceneDebugger*								m_debugger;
	ICutsceneWorldInterface*							m_csWorldInterface;

	IRenderTextureStreamRequest*						m_currentSectionTexRequest;	//!< Texture request for the currently playing section. Kept around until the section ends.
	IRenderTextureStreamRequest*						m_currentBackgroundRequest;	//!< Texture request for the currently playing section. Kept around until the section ends.
	IRenderTextureStreamRequest*						m_newBackgroundRequest;		//!< Texture request for the section we are now starting.
	IRenderFramePrefetch* 								m_framePrefetch;			//!< Initial frame prefetch when changing to a new section.

	SStoryScenePlayerDebugStats							m_debugStats;
	Bool												m_IsBlockingMusic;
	Bool												m_sceneIsOverridingListener;
	Bool												m_sectionIsOverridingListener;
public:
	CStoryScenePlayer();
	virtual ~CStoryScenePlayer();

	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld* world );
	
	virtual void OnSerialize( IFile& file );

	//! Initialize scene player to play from given section. Returns true if initialization was successful.
	Bool Init( CStorySceneController& controller, const ScenePlayerPlayingContext& context );

	void Precache();

	//! Advance scene, returns false if scene has stopped
	Bool Tick( Float timeDelta );

	// DIALOG_TIOMSIN_TODO - TEST
	Bool SetState( const ScenePlayerInputState& state );

	Bool HasSceneFinished();

	// DIALOG_TIOMSIN_TODO - w controletrze nie nie po pointerze!
	Bool SignalAcceptChoice( const SSceneChoice& choosenLine );

	Bool SignalSkipLine();

	Bool IsSkippingAllowed();
	Bool CanSkipElement( const CStorySceneElement* elem ) const;

	void UpdateInfoAboutSkipping( Bool force = false );

	enum EPauseChannel
	{
		EPC_Default = FLAG(1),
		EPC_Controller = FLAG(2),
		EPC_ScriptBlock = FLAG(3),
	};

	//! Pause/continue scene. Scene will pause as soon as possible but not until a active latent operation will end
	void Pause( Bool shouldPause, EPauseChannel channel = EPC_Default );
	void TogglePause();
	Bool HasLoadedSection() const;

	// DIALOG_TIOMSIN_TODO - tu sa jakies hardcore hacki - controler usuwa playera a player controlera w srodku!
	//! Stop played scene
	void Stop();

	void SetDestroyFlag();
	Bool HasDestroyFlag() const;
	Bool IsStopped() const;

	Bool IsInWorld( const CWorld* world ) const;

	Bool UseApprovedVoDurations() const;

protected:
	//void PlayNextSceneElement();

	void SchedulePlayNextSceneElement();

	Bool IsPlanInProgress() const;

	Bool FindSafeDialogSetPlacement( const CStorySceneDialogsetInstance* dialogSetInstance, const EngineTransform& initialScenePlacement, TDynArray< CNode* >& taggedNodes, EngineTransform& placementTransform ) const;
	Bool IsDialogSetPlacementSafe( EngineTransform& transform, const TDynArray< Vector >& sceneConvex, const Box& sceneBBox, const TDynArray< Vector >& dialogSetPoints, const TDynArray< CEntity* >& sceneActors, Bool isInterior, Bool checkConvex, Bool& isAcceptableAsFallbackOnly ) const;
public:
	Bool GetReferencePlacement( const CStorySceneDialogsetInstance* dialogSetInstance, EngineTransform& transform ) const;

#ifndef NO_EDITOR
	void Freeze();
#endif

	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags ) override;
	virtual Bool ShouldGenerateEditorFragments( CRenderFrame* frame ) const override;

public:
	RED_INLINE CStorySceneDirector* GetSettingController() { return &m_sceneDirector; }

	RED_INLINE CStorySceneController* GetSceneController() { return m_sceneController; }
	RED_INLINE const CStorySceneController* GetSceneController() const { return m_sceneController; }

	RED_INLINE IStorySceneDisplayInterface* GetSceneDisplay() const { return m_display; }
	RED_INLINE IStorySceneDebugger* GetSceneDebugger() const { return m_debugger; }
	RED_INLINE ICutsceneWorldInterface* GetCsWorldInterface() { return m_csWorldInterface; }
	RED_INLINE ICutsceneSceneInterface* GetCsSceneInterface() { return this; }
	RED_INLINE CStorySceneAnimationContainer& GetAnimationContainer() { return m_animContainer; }

	RED_INLINE Bool IsNextSectionBlocking() const { return m_internalState.m_pendingNextSection && !m_internalState.m_pendingNextSection->IsGameplay(); }

public:
	//! Is this a non interactive scene ( texts above heads )
	RED_INLINE Bool IsGameplay() const { return m_isGameplay; }

	//! Is this scene paused
	RED_INLINE Bool IsPaused() const { return m_isPaused != 0; }

	//! Get name of the story scene player ( debug mostly )
	RED_INLINE const String& GetName() const { return m_name; }

	//! Returns the name of the output the scene activated when it ended
	RED_INLINE const CName& GetActivatedOutputName() const { return  ( m_internalState.m_activatedOutput != NULL ) ? m_internalState.m_activatedOutput->GetOutputName() : CName::NONE; }

	// Parameters of the dialog-to-gameplay camera blend
	RED_INLINE Bool DidJustSkipElement() const { return m_internalState.m_justSkippedElement; }
	RED_INLINE Float GetGameplayCameraBlendTime() const { return m_internalState.m_activatedOutput ? Max( m_internalState.m_activatedOutput->GetGameplayCameraBlendTime(), 0.0f ) : 0.0f; }
	RED_INLINE Bool GetGameplayCameraUseFocusTarget() const { return m_internalState.m_activatedOutput ? m_internalState.m_activatedOutput->GetGameplayCameraUseFocusTarget() : false; }
	RED_INLINE Bool IsGameplayCameraAbandoned() const { return ( m_internalState.m_activatedOutput != NULL ) ? m_internalState.m_activatedOutput->IsGameplayCameraAbandoned() : false; }
	RED_INLINE Bool GetResetGameplayCameraOnOutput() const { return m_resetGameplayCameraOnOutput; }
	RED_INLINE void SetResetGameplayCameraOnOutput( Bool resetGameplayCameraOnOutput ) { m_resetGameplayCameraOnOutput = resetGameplayCameraOnOutput; }
	RED_INLINE Bool GetSwitchedToGameplayCamera() const { return m_switchedToGameplayCamera; }
	RED_INLINE void SetSwitchedToGameplayCamera( Bool switchedToGameplayCamera ) { m_switchedToGameplayCamera = switchedToGameplayCamera; }

	Bool HasPendingChoice();

	//! Returns if all scene data is precached
	RED_INLINE Bool IsScenePrecached() const { return m_isPrecached; }

	//! Returns true if player is now playing a gameplay section
	Bool IsGameplayNow() const;

	// DIALOG_TIOMSIN_TODO - to jest tylko editor only ale jest uzywane poza
	const CGUID& GetActorsSlotID( const CName& actor ) const;

	// DIALOG_TIOMSIN_TODO - to jest tylko editor only ale jest uzywane poza - kamera
	Bool GetActorIdleAnimation( const CName& actor, CName& out ) const;

	Bool IsActorOptional( CName id ) const;
	
	void StartBlendingOutLights( Float blendTime, const Bool disableDof = false );

#ifndef NO_EDITOR
	StorySceneCameraState GetCurrentCameraState() const { return m_internalState.m_cameraState; }

	Bool GetCurrentActorAnimationState( const CName& actor, SStorySceneActorAnimationState& out ) const;
	Bool GetPreviousActorAnimationState( const CName& actor, SStorySceneActorAnimationState& out ) const;
	Bool GetCurrentActorAnimationMimicState( CName actor, CName& mimicEmoState, CName& mimicLayerEyes, CName& mimicLayerPose, CName& mimicLayerAnim, Float& poseWeight )  const;
	Bool GetCurrentLightState( const CName& actor, SStorySceneAttachmentInfo& out, EngineTransform& outPos ) const;

	Matrix GetActorPosition( const CName& actor ) const;
	Matrix GetDialogPosition() const;
	Bool AdjustCameraForActorHeight( const StorySceneCameraDefinition& def, EngineTransform* outAdjusted, Matrix* outCameraWS ) const { return m_sceneDirector.AdjustCameraForActorHeight( def, outAdjusted, outCameraWS ); }
#endif

public:
	//! Get friendly description of this object
	virtual String GetFriendlyName() const;

	Float GetCurrentSectionTime() const;

public:
	// DIALOG_TIOMSIN_TODO - to uzywane w questach przeniesc do controllera
	//! Add playback listener
	void AddPlaybackListener( IStoryScenePlaybackListener* listener );

	// DIALOG_TIOMSIN_TODO - to uzywane w questach przeniesc do controllera
	//! Remove playback listener
	void RemovePlaybackListener( IStoryScenePlaybackListener* listener );

public:
	// DIALOG_TIOMSIN_TODO - to na zew uzywa tylko cs
	//! Register actor in actors map
	Bool RegisterActor( const CName& voiceTag, CEntity* actor );
	Bool RegisterProp( const CName& id, CEntity* prop );
	Bool RegisterEffect( const CName& id, CEntity* effect );
	Bool RegisterLight( const CName& id, CEntity* light );

	// DIALOG_TIOMSIN_TODO - to na zew uzywa tylko cs
	//! Unregister actor in actors map
	void UnregisterActor( const CName& voiceTag );
	void UnregisterProp( const CName& id );
	void UnregisterEffect( const CName& id );
	void UnregisterLight( const CName& id );

public:

	// DIALOG_TIOMSIN_TODO - przeniesc do controllera
	// Add denied area using points list
	Bool AddDeniedArea( const TDynArray< Vector >& localSpaceConvex, const EngineTransform& worldTransform );

	// DIALOG_TIOMSIN_TODO - przeniesc do controllera
	// enable/disable all denied area components
	void SetDeniedAreasEnabled( Bool enabled );

	// DIALOG_TIOMSIN_TODO - przeniesc do controllera
	// Remove all denied area components
	void ClearDeniedAreas();

	void HideNonSceneActor( CActor* actorToHide );
	void UnhideAllNonSceneActors();

	// DIALOG_TIOMSIN_TODO - to wykorzystywane w questach ( quest debug ) przeniesc do sekcji private
	Bool GetActorsUsedInCurrentSection( TDynArray< THandle< CEntity> >& sectionActors ) const;

	//! Get voicetags from setting used during this section
	void GetVoiceTagsForCurrentSetting( TDynArray< CName >& voicetags, Bool append = true ) const;

public:
	//void EnsureActorScenePoses( const CStorySceneSection* section );

	virtual void OnActorSpawned( const EntitySpawnInfo& info, const CEntity* entity ) {}
	virtual void OnPropSpawned( const EntitySpawnInfo& info, const CEntity* entity ) {}
	virtual void OnEffectSpawned( const EntitySpawnInfo& info, const CEntity* entity ) {}
	virtual void OnLightSpawned( const EntitySpawnInfo& info, const CEntity* entity ) {}

	void OnCutscenePlayerStarted( const CCutsceneInstance* cs );
	void OnCutscenePlayerEnded( const CCutsceneInstance* cs );

	void OnActorAppearanceChanged( const CActor* actor );

public: // DIALOG_TIOMSIN_TODO - hack
	const CStorySceneSectionPlayingPlan* GetCurrentPlayingPlan() const;
	void ProcessSceneStateForActors( Bool active );
	Bool FindSafeSpotForActor( const THandle< CEntity >& entity, const Vector inPos, Vector& outPos );
	Bool SetSceneStateOnActor( const THandle< CEntity >& actorHandle, Bool enable, Vector* teleportedTransform = nullptr );
	void SetSceneStateOnProp( CName id, const THandle< CEntity >& actorHandle, Bool enable );

protected:
	void SetSceneBehaviourInst( CEntity* ent, Bool enable, Bool addGraphIfMissing = false );
protected:
	Bool LogicTick( EInternalState state, TickContext& context );
	Bool LogicTick_Intro( EInternalState state, TickContext& context );

	void ResumeSceneFlow( const CStorySceneLinkElement* flowPoint );

	//! Evaluate scene control chain
	const CStorySceneSection* EvaluateControlChain( const CStorySceneLinkElement* part, InternalFlowState& flowState, Bool forced );
	const CStorySceneSection* EvaluateControlChain( const CStorySceneLinkElement* start, const CStorySceneLinkElement* prev, const CStorySceneLinkElement* part, InternalFlowState& flowState, Bool forced );
	virtual Bool EvaluateFlowCondition( const CStorySceneFlowCondition* condition ) const;

	//! Process script block
	const CStorySceneLinkElement* ProcessScriptBlock( const CStorySceneScript* script, Int32& _immediateResult );

	//! Changing active section
	void ChangeSection( const CStorySceneSection* newSection, Bool destroyOldPlan, Bool checkFlow );
	Bool FinishChangingSection();
	Bool IsSectionChanging();

	void ChangeSection_StartBackgroundTextureRequest();
public:
	IRenderFramePrefetch* CreateFramePrefetch( const CStorySceneSectionPlayingPlan* playingPlan, const CStorySceneSectionPlayingPlan::CameraMarker& marker, const Matrix& sceneTransform );

	// Create and start a frame prefetch for the first camera event in a section. Returns the prefetch object.
	IRenderFramePrefetch* StartSectionFirstFramePrefetch( const CStorySceneSectionPlayingPlan* playingPlan );
	// Create and start frame prefetches for all early camera events, except not the first. Fire-and-forget, doesn't return any of the prefetches.
	void StartSectionEarlyFramePrefetches( const CStorySceneSectionPlayingPlan* playingPlan );


protected:

	Bool ShouldFadeOutBetweenSections( const CStorySceneSection* prevSection, const CStorySceneSection* nextSection, Float& duration ) const;
	Bool ShouldFadeInBetweenSections( const CStorySceneSection* prevSection, const CStorySceneSection* nextSection ) const;

	CStorySceneSectionPlayingPlan* GetSectionPlayingPlan( const CStorySceneSection* section );
	const CStorySceneSectionPlayingPlan* GetSectionPlayingPlan( const CStorySceneSection* section ) const;
	void RequestSectionPlayingPlan_StartFlow( const CStorySceneSection* requestedSection, CName& outDialogSet );
	void RequestSectionPlayingPlan_ContinueFlow( const CStorySceneSection* requestedSection );

protected: // TODO
	virtual void OnRequestSectionPlayingPlan_StartFlow( const CStorySceneSection* requestedSection, CName& dialogSet ) {}
	virtual Bool HasValidDialogsetFor( const CStorySceneSection* s, const CStorySceneDialogsetInstance* d ) const { return true; }
public:

	void PlayStoryElement( const CStorySceneElement* element, const CStorySceneSection* section );
	void PlayStoryElement( IStorySceneElementInstanceData* elementInstance, const CStorySceneSection* section );
	
	CStorySceneSectionPlayingPlan* GetCurrentPlayingPlan();

	virtual void EndPlayingSectionPlan( Int32 id );

	void EndPlayingScene( Bool isStopped );

	void CycleStreamingCamera( const CStorySceneSection* prevSection, const CStorySceneSection* nextSection );

	virtual void OnScriptThreadKilled( CScriptThread * thread, Bool finished );

	Bool ShouldTickDuringFading() const;

	Bool TickElementAndEvents( IStorySceneElementInstanceData* elem, Float dt, CStorySceneEventsCollector& collector, const CStorySceneEventsCollectorFilter* filter );
	
	void ResetEventsPlayState( const CStorySceneSectionPlayingPlan& plan );
	void ResetEventsPlayState( const CStorySceneSectionPlayingPlan& plan, const CStorySceneElement* forElement );
	void FireEvents( CStorySceneEventsCollector& collector );
	void CollectSceneEvents( CStorySceneSectionPlayingPlan* plan, Float prevPositionMS, Float currPositionMS, CStorySceneEventsCollector& collector, const CStorySceneEventsCollectorFilter* filterA, const CStorySceneEventsCollectorFilter* filterB ) const;
	void CollectAllEvents( const IStorySceneElementInstanceData* elem, CStorySceneEventsCollector& collector );
	void CollectAllEvents( CStorySceneSectionPlayingPlan* plan, const IStorySceneElementInstanceData* elem, CStorySceneEventsCollector& collector, const CStorySceneEventsCollectorFilter* filterA, const CStorySceneEventsCollectorFilter* filterB ) const;
	void CollectAllEvents( CStorySceneSectionPlayingPlan* plan, CStorySceneEventsCollector& collector, const CStorySceneEventsCollectorFilter* filterA, const CStorySceneEventsCollectorFilter* filterB ) const;
	void InternalFireEvent( const CStorySceneEvent* sceneEvt, CStorySceneSectionPlayingPlan::InstanceData& instance, const Float prevPositionMS, const Float currPositionMS, CStorySceneEventsCollector& collector ) const;
	void SendCustomSceneSignals( const CStorySceneSectionPlayingPlan* plan, const Float prevPositionMS, const Float currPositionMS, Bool wasSkipCalled, CStorySceneEventsCollector& collector );

	void RegisterSpawnedItems( TDynArray< TPair< CName, SItemUniqueId > >& spawnedItems );
	void RegisterSpawnedItem( TPair< CName, SItemUniqueId > spawnedItem );

	Bool ShouldActorHaveHiResShadows( const CActor* a ) const;
	void SetActorHiResShadow( CActor* a, Bool flag );
	void ResetAllActorsHiResShadow();

	void SetActorLodOverride( CActor* a, Bool forceHighestLod, Bool disableAutoHide );
	void ResetActorLodOverrides();

	Bool ShouldActorHaveMimicOn( const CActor* a ) const;
	void SetActorMimicOn( CActor* a, Bool flag );
	void ResetAllActorsMimicOn();

private:
	Int32 FindInputIndexFromTo( const CStorySceneLinkElement* from, const CStorySceneSection* to, Bool forced ) const;

protected:
	void CheckPauseConditions();

	void ResetPlayer();

	void ResetAllSceneEntitiesState();
	virtual void OnPreResetAllSceneEntitiesState( Bool forceDialogset, const CStorySceneSection* currSection, const CStorySceneDialogsetInstance* currDialogset ) {}

	//! Signal scene end to playback listeners
	void SignalPlaybackEnd( Bool stopped );

	void SetupPlayerName( const CStorySceneInput* startInput );

	//! Updates the collection of listeners that should be notified by the player
	void UpdatePlaybackListeners();

	//! Unload all scene templates
	void UnloadSceneTemplates();

public:
	Matrix GetSceneSectionTransform( const CStorySceneSection* section ) const;

	virtual CWorld* GetSceneWorld() const;
	virtual CCamera* GetSceneCamera() const;
	virtual const THashMap< CName, THandle< CEntity > >& GetSceneActors() const { return m_sceneActorEntities; }
	virtual CActor* GetMappedActor( CName voiceTag );

	CEntity* SpawnSceneActorEntity( const CName& actorName );
	CEntity* SpawnScenePropEntity( const CName& propName );
	CEntity* SpawnSceneEffectEntity( const CName& fxName );
	CEntity* SpawnSceneLightEntity( const CName& lightName );

	const CEntity* GetSceneActorEntity( const CName& actorName ) const;
	const CEntity* GetScenePropEntity( const CName& propName ) const;
	const CEntity* GetSceneEffectEntity( const CName& fxName ) const;
	const CEntity* GetSceneLightEntity( const CName& lightName ) const;

	CEntity* GetSceneActorEntity( const CName& actorName );
	CEntity* GetScenePropEntity( const CName& propName );

	virtual THandle<CEntity>*	GetSceneActorEntityHandle( const CName& actorName );
	virtual void AddNewActor( CActor* actor, CName voiceTag, Bool dontDestroyAfterScene );
	virtual void AddNewProp( CEntity* prop, CName id );
	virtual void AddNewEffect( CEntity* effect, CName id );
	virtual void AddNewLight( CEntity* light, CName id );

	virtual CStorySceneDirector* GetSceneDirector() { return &m_sceneDirector; }
	const CStorySceneDirector* GetSceneDirector() const { return &m_sceneDirector; }
	virtual Bool IsInGameplay() const;
	virtual CStoryScene* GetStoryScene() const { return m_storyScene.Get(); }
	virtual const CStorySceneSection* GetCurrentSection() const { return m_internalState.m_currentSection; }
	const CStoryScene* GetCurrentStoryScene() const;
	CStoryScene* GetCurrentStoryScene_Unsafe();

	virtual void SceneFadeIn( const String& reason, Float duration = 0.2f, const Color& color = Color::BLACK );
	virtual void SceneFadeOut( const String& reason, Float duration = 0.2f, const Color& color = Color::BLACK );
	virtual void SceneFrozenFrameStart();
	virtual void SceneFrozenFrameEnd();
	virtual void EnableTickDuringFade( Bool enable );
	
	virtual void OnInit( const ScenePlayerPlayingContext& context ) {}
	virtual Bool WantsToGoToNextSection() { return true; }
	virtual Bool CanFinishScene() { return true; }
	virtual Bool IsCutsceneAsyncTick() const { return true; }
	virtual Bool CanUseCutscene() const { return true; }

	virtual Bool CanUseDefaultScenePlacement() const;
	virtual EngineTransform InformAndGetDefaultScenePlacement() const;
	virtual void InformAboutScenePlacement( const TDynArray< CNode* >& taggedNodes ) const;

	virtual Bool ShouldPlaySpeechSound() const { return true; }
	virtual Bool ShouldPlaySounds() const { return true; }
	virtual Bool ShouldPlayEffects() const { return true; }

public: // Functions for scene events
	const CStorySceneEvent* FindEventByGUID( const CGUID& id, Int32* outIndex = nullptr ) const;
	const CStorySceneEvent* FindEventByIndex( Int32 index ) const;
	virtual Bool IsSceneInGame() const { return true;} 
	virtual const CStorySceneDialogsetInstance* GetDialogsetForEvent( const CStorySceneEvent* e ) const;

	Bool GetBlockSceneArea() const;
	Bool GetEnableDestroyDeadActorsAround() const;

	void GetEventAbsTime( const CStorySceneEvent* e, Float& time, Float& duration ) const;
	Float GetEventScalingFactor( const CStorySceneEvent& e ) const;
	Float GetEventDuration( const CStorySceneEvent& e ) const;
	void SetEventDuration( const CStorySceneEvent& e, Float duration ) const;
	Float GetEventStartTime( const CStorySceneEvent& e ) const;
	void SetEventStartTime( const CStorySceneEvent& e, Float startTime ) const;

public:
	const CStorySceneInput* HACK_GetInput() const;

	Bool DetermineSceneBounds( const SStorySceneDialogsetInstanceCalcBoundDesc& desc, Vector& center, Box& box, TDynArray< Vector >& convex, const CStorySceneSection* section ) const;

	void CollectPropEntitiesForStream(IRenderTextureStreamRequest* textureStreamRequest) const;

protected:
	void ChangeSceneSettingAndResetActors();
	void ChangeSceneSettingWithoutResetActors();

protected:
	//! Internal callbacks
	void OnSceneStarted();
	void OnSceneEnded();

	void DisableStreamingLockdown( const CStorySceneSection* section );
	void EnableStreamingLockdown( const CStorySceneSection* section );

	void OnSectionStarted( const CStorySceneSection* section, const CStorySceneSection* previousSection = nullptr );
	void OnSectionEnded( const CStorySceneSection* section );

	void OnInjectedSceneStarted( const CStoryScene* injectedScene );
	void OnInjectedSceneFinished( const CStorySceneOutput* output, const CStoryScene* injectedScene, const CStoryScene* targetScene );

	const CStorySceneLinkElement* GetSectionNextElement( const CStorySceneLinkElement* sectionElement ) const;
	const CStorySceneLinkElement* GetSectionNextElement( const CStorySceneSection* section ) const;

	void OnPaused();

public:
	virtual void OnLineSkipped();
	virtual void OnCameraCut();

protected:
	
	virtual void TogglePlayerMovement( Bool flag );
	virtual void SetSceneCameraMovable( Bool flag );

	virtual void OnNonGameplaySceneStarted();
	virtual void OnNonGameplaySceneEnded(const CStorySceneSection *newSection);

	virtual void ToggleHud( Bool flag );
	virtual Bool IsHudReady() const;

	virtual Bool IsFadeInProgress() const;

	virtual void CreateNoSaveLock( const String& lockReason );
	virtual void ReleaseNoSaveLock();

public:
	Bool IsSceneBlackscreenSet() const;
	virtual void SetSceneBlackscreen( Bool flag, const String& reason );

	void		 HandleOnEndSoundEvents(const CStorySceneSection *newSection);
	void		HandleOnSkipSoundEvents();

	virtual void PlayVideo( const SVideoParams& params );
	virtual void StopVideo();
	virtual Bool IsPlayingVideo() const;
	virtual Bool HasValidVideo() const;
	virtual Bool GetVideoSubtitles( String& outSubtitles );

	virtual void SoundUpdateAmbientPriorities();
	virtual void SoundForceReverb( const String& reverbDefinition );
	void SetCurrentTimeMultiplier( Float m_multiplier );
	void RemoveTimeMultiplier();

	void DbFactAdded( const String& factName );
	void DbFactRemoved( const String& factName );
#ifndef RED_FINAL_BUILD
public:
	virtual void SetDebugScreenText(const String & arg) const;
	virtual void AddDebugScreenText(const String & arg) const;
	virtual void ClearDebugScreenText() const;
#endif

public:
	void funcGetSceneWorldPos( CScriptStackFrame& stack, void* result );
	void funcRestartScene( CScriptStackFrame& stack, void* result );
	void funcRestartSection( CScriptStackFrame& stack, void* result );
	void funcDbFactAdded( CScriptStackFrame& stack, void* result );
	void funcDbFactRemoved( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CStoryScenePlayer );
	PARENT_CLASS( CEntity );
	PROPERTY( m_storyScene );
	PROPERTY( m_injectedScenes )
	PROPERTY_RO( m_isPaused, TXT( "Is player paused" ) );
	PROPERTY_RO( m_isGameplay, TXT( "Does scene include gameplay parts" ) );
	NATIVE_FUNCTION( "GetSceneWorldPos", funcGetSceneWorldPos );
	NATIVE_FUNCTION( "RestartScene", funcRestartScene );
	NATIVE_FUNCTION( "RestartSection", funcRestartSection );
	NATIVE_FUNCTION( "DbFactAdded", funcDbFactAdded );
	NATIVE_FUNCTION( "DbFactRemoved", funcDbFactRemoved );
END_CLASS_RTTI();

RED_DECLARE_NAME( StorySceneDebugRefresh )
RED_DECLARE_NAME( VideoClient_StoryScenePlayer );

RED_INLINE Bool CStoryScenePlayer::UseApprovedVoDurations() const
{
	return m_useApprovedVoDurations;
}
