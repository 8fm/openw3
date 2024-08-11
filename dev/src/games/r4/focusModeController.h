/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../../common/game/gameSystem.h"
#include "../../common/game/entitiesDetector.h"
#include "../../common/core/priqueue.h"
#include "dynamicClueStorage.h"

// needed for shared colorblind mode config
#include "../../common/engine/renderGameplayEffects.h"

//////////////////////////////////////////////////////////////////////////

class CFocusActionComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CFocusActionComponent, CComponent, 0 )

private:
	CName	m_actionName;

public:
	const CName& GetActionName() const				{  return m_actionName;        }
	void SetActionName( const CName& actionName )	{  m_actionName = actionName;  }
};

BEGIN_CLASS_RTTI( CFocusActionComponent );
	PARENT_CLASS( CComponent );
	PROPERTY_EDIT( m_actionName, TXT("Name of the action shown on entity in focus mode") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SFocusPosition
{
	Vector	m_position;
	Vector	m_direction2D;
	Vector	m_cameraPosition;

	SFocusPosition()
		: m_position( Vector::ZEROS )
		, m_direction2D( Vector::ZEROS )
		, m_cameraPosition( Vector::ZEROS )
	{}
};

//////////////////////////////////////////////////////////////////////////

class CFocusModeController : public IGameSystem, IGameSaveSection
{
	DECLARE_ENGINE_CLASS( CFocusModeController, IGameSystem, 0 );

	typedef CGameplayEntity*			TCluePtr;
	typedef THandle< CGameplayEntity >	TClue;
	typedef TDynArray< TClue >			TClues;

	struct SSoundClueParams
	{
		SSoundClueParams();

		CName								m_eventStart;
		CName								m_eventStop;
		Float								m_hearingAngleCos;						   
		Uint8								m_effectType;
	};

	struct SSoundClue
	{
	private:

		typedef THandle< CGameplayEntity > TSoundEntity;
		
		TSoundEntity						m_entity;
		THandle< CSoundEmitterComponent	>	m_soundEmitter;		
		Float								m_timeLeft;
		SSoundClueParams					m_params;

	public:

		SSoundClue();
		SSoundClue( CGameplayEntity* entity );

		void Init( CFocusModeController* parent );
		Bool IsValid() const;
		Bool IsAlive() const;
		Bool IsInHearingAngle( const SFocusPosition& focusPosition ) const;
		Bool Update( Float deltaTime );
		void Start();
		void Stop();
		void ResetTimer();
		void UpdateAccuracy(  const SFocusPosition& focusPosition );
		Float CalcAccuracy( const SFocusPosition& focusPosition ) const;
		void DrawDebug( CRenderFrame* frame, const SFocusPosition& focusPosition ) const;
		
		RED_INLINE CName GetId() const
		{
			return m_params.m_eventStart;
		}

		RED_INLINE const Vector& GetWorldPosition() const
		{
			return m_entity->GetWorldPositionRef();
		}

		RED_INLINE Bool operator==( const SSoundClue& other )
		{
			return Equal( *this, other );
		}

		RED_INLINE Bool operator!=( const SSoundClue& other )
		{
			return !Equal( *this, other );
		}

		static Bool Equal( const SSoundClue& a, const SSoundClue& b );
	};

	struct SIsSoundClueEvaluator
	{
		SIsSoundClueEvaluator( CFocusModeController* parent );
		Bool operator()( const CGameplayEntity* entity ) const;

	private:

		CFocusModeController*		m_parent;
		mutable SSoundClueParams	m_params;
	};

	struct SEntitiesDetector : public IEntitiesDetectorListener
	{
		SEntitiesDetector( CFocusModeController* parentController );

	protected:

		CFocusModeController*	m_parent;
	};

	struct SScentClue
	{

	private:

		CName				m_effectName;
		EntityHandle		m_entityHandle;
		Float				m_duration;
		Float				m_activationTime;
		Float				m_intensity;
		Bool				m_isActive;
		Uint32				m_worldHash;

	public:

		SScentClue();
		SScentClue( const CName& effectName, CEntity* entity, Float duration, Uint32 worldHash );
		SScentClue( IGameLoader* loader );
		~SScentClue();

		void Activate( Bool activate );
		Bool Update( Float focusIntensity );
		RED_INLINE Bool IsOnThisWorld( Uint32 currentWorldHash ) { return m_worldHash == currentWorldHash; }

		void Save( IGameSaver* saver ) const;

		RED_INLINE Bool operator==( const CEntity* entity ) const
		{
			return m_entityHandle.Get() == entity;
		}

	private:

		void SetIntensity( Float newIntensity );
	};



	struct SEntitiesWithStateDetector : public SEntitiesDetector
	{
		SEntitiesWithStateDetector( CFocusModeController* parentController, TClues* clues );
		void BeginEntitiesDetection() override;
		void EndEntitiesDetection() override;
		void ProcessDetectedEntity( CGameplayEntity* entity );

	protected:

		TClues*		m_clues;
		TClues		m_oldClues;

		virtual Bool CheckClueCondition( TCluePtr entity ) = 0;
		virtual void OnClueStateChange( TCluePtr entity, Bool enabled ) = 0;
	};

	// Detects all CGameplayEntities and updates its focus visibility
	struct SGameplayEntitiesDetector : public SEntitiesWithStateDetector
	{
		SGameplayEntitiesDetector( CFocusModeController* parent );
		void OnDeactivate();

	protected:

		Bool CheckClueCondition( TCluePtr clue )  override;
		void OnClueStateChange( TCluePtr clue, Bool enabled ) override;

	private:

		TClues		m_gameplayEntities;

		void OnEnabledInFocusMode( TCluePtr entity, Bool enable );
	};

	// This one finds all W3MonsterClue entities in given range
	struct SFocusCluesDetector : public SEntitiesDetector
	{
		SFocusCluesDetector( CFocusModeController* parent );
		void Update();
		void Clear();

		RED_INLINE TClues::iterator begin() { return m_focusClues.Begin(); }
		RED_INLINE TClues::iterator end() { return m_focusClues.End(); }

	protected:

		void BeginEntitiesDetection() override;
		void ProcessDetectedEntity( CGameplayEntity* entity ) override;

	private:

		TClues		m_focusClues;
		CClass*		m_focusClueClass;
	};

	// CGameplayEntity'ies with CFocusSoundParam.
	// All sound clues in given range should be "activate" on focus mode
	// activation and deactivated (only those which have been previously
	// activated) on focus mode deactivation.
	struct SSoundCluesDetector : public SEntitiesWithStateDetector
	{
		SSoundCluesDetector( CFocusModeController* parent );
		void Update( Float timeDelta );
		void OnDeactivate();
		void OnSoundEventsChanged( TCluePtr clue );

		typedef THashMap< CGUID, SSoundClue > TSoundClues;
		typedef THashMap< CName, SSoundClue > TActiveSoundClues;

		RED_INLINE TActiveSoundClues::iterator begin() { return m_activeSounds.Begin(); }
		RED_INLINE TActiveSoundClues::iterator end() { return m_activeSounds.End(); }

	protected:

		void BeginEntitiesDetection() override;
		void EndEntitiesDetection() override;
		Bool CheckClueCondition( TCluePtr clue )  override;
		void OnClueStateChange( TCluePtr clue, Bool enabled ) override;

	private:

		struct SBestSoundClue
		{
			const SSoundClue*	m_clue;
			Float				m_distanceSqr;
		};

		typedef THashMap< CName, SBestSoundClue >	TBestSoundClues;

		CActor*					m_player;
		TClues					m_rawSoundClues;	// needed by SEntitiesWithStateDetector
		TSoundClues				m_soundClues;
		TActiveSoundClues		m_activeSounds;
		TBestSoundClues			m_bestSoundClues;

		void RemoveInvalidClues( Float timeDelta );
		void EnableSoundClue( TCluePtr clue, Bool enable );
		void ForceClear();
	};

	// Enemies should be highlighted in focus mode
	struct SEnemiesDetector : public SEntitiesWithStateDetector
	{
		SEnemiesDetector( CFocusModeController* parent );
		void OnDeactivate();

	protected:

		void BeginEntitiesDetection() override;
		void EndEntitiesDetection() override;
		Bool CheckClueCondition( TCluePtr clue )  override;
		void OnClueStateChange( TCluePtr clue, Bool enabled ) override;

	private:

		CActor*		m_player;
		TClues		m_enemies;

		void ShowInFocusMode( TCluePtr entity, Bool show );
	};

	// Custom sound clues (for example: 'leshy clues') are special kind of clues
	// (not related to 'standard' focus mode clues). Each frame we need to find nearest
	// custom sound clue and update sound system parameters according to its position.
	struct SCustomSoundCluesDetector : public SEntitiesDetector
	{
		SCustomSoundCluesDetector( CFocusModeController* parentController );
		void Update();
		void OnDeactivate();

	protected:

		void BeginEntitiesDetection() override;
		void ProcessDetectedEntity( CGameplayEntity* entity ) override;

	private:

		TClues		m_customSoundClues;
		TClue		m_activeCustomSoundClue;

		void SetActiveCustomSound( TCluePtr entity );
	};

	struct SUpdateFocusVisibilityEntity
	{
		SUpdateFocusVisibilityEntity();
		SUpdateFocusVisibilityEntity( CGameplayEntity* entity, Float dist );

		static Bool Less( const SUpdateFocusVisibilityEntity& key1, const SUpdateFocusVisibilityEntity& key2 );

		THandle< CGameplayEntity >	m_entity;
		Float						m_dist;
	};

private:

	SGameplayEntitiesDetector		m_gameplayEntitiesDetector;
	SFocusCluesDetector				m_focusCluesDetector;
	SSoundCluesDetector				m_soundCluesDetector;
	SEnemiesDetector				m_enemiesDetector;
	SCustomSoundCluesDetector		m_customSoundCluesDetector;

	CDynamicClueStorage				m_dynamicClueStorage;

	Bool							m_isActive;
	Float							m_activationTimer;
	Float							m_deactivationTimer;

	Bool							m_dimming;
	Float							m_dimmingFactor;
	Float							m_dimmingTime;

	Uint32							m_currentWorldHash;

	typedef THashMap< IdTag, SSoundClueParams >	TSoundClueParams;
	TSoundClueParams				m_overridenSoundClueParams;

	mutable TDynArray< const CEntity* >	m_lostTestIgnoreEntities;

	SIsSoundClueEvaluator			m_isSoundClueEvaluator;

	typedef TDynArray < SScentClue > TScentClues;
	TScentClues						m_scentClues;

	typedef TPriQueue< SUpdateFocusVisibilityEntity, SUpdateFocusVisibilityEntity >	TUpdateFocusVisibilityEntities;
	TUpdateFocusVisibilityEntities	m_updateFocusVisibilityEntities;

	typedef THashMap< IdTag, EFocusModeVisibility > TStoredFocusModeVisibility;
	TStoredFocusModeVisibility		m_storedFocusModeVisibility;

protected:

	// System parameters, TODO: Move to game resource
	static const Float ACTIVATION_DURATION;
	static const Float DIMMING_DURATION;

	// time interval and range for clues in range update (loaded from csv)
	static Float CLUES_UPDATE_INTERVAL;
	static Float CLUES_UPDATE_RANGE;
	static Float SOUND_CLUES_UPDATE_INTERVAL;
	static Float SOUND_CLUES_RANGE;
	static Float ENEMIES_UPDATE_INTERVAL;
	static Float ENEMIES_RANGE;
	static Float CUSTOM_SOUND_CLUES_UPDATE_INTERVAL;
	static Float CUSTOM_SOUND_CLUES_RANGE;

	// focus fx params
	static const Float EFFECT_DESATURATION;
	static const Float EFFECT_HIGHLIGHT_BOOST;

	// TODO: I hear the fading is to be controlled by a potion, so how these values work will need to change
	static const Float EFFECT_FADE_NEAR;
	static const Float EFFECT_FADE_FAR;
	static const Float EFFECT_DIMMING_TIME;
	static const Float EFFECT_DIMMING_SPEED;

public:
	CFocusModeController();

	virtual void Initialize() override;
	virtual void Shutdown() override;
	virtual void OnGameStart( const CGameInfo& gameInfo ) override;
	virtual void OnGameEnd( const CGameInfo& gameInfo ) override;
	virtual void OnWorldStart( const CGameInfo& gameInfo ) override;
	virtual void Tick( Float timeDelta ) override;

	virtual void OnGenerateDebugFragments( CRenderFrame* frame ) override;

	void SetActive( Bool isActive );
	RED_INLINE Bool IsActive() const { return m_isActive; }

	Float GetIntensity() const;

	RED_INLINE CDynamicClueStorage* GetDynamicClueStorage() { return &m_dynamicClueStorage; }

protected:

	virtual Bool OnSaveGame( IGameSaver* saver ) override;
	void LoadGame( IGameLoader* lodaer );
	void LoadConfig();
	void SetRange( const String& rangeName, Float range, Float interval );
	void OnActivationFinished();
	void OnDeactivationFinished();
	void EnableVisuals( Bool enable, Float desaturation, Float highlightBoost, Bool forceDisable = false );
	void EnableExtendedVisuals( Bool enable, Float fadeTime );
	void SetDimming( Bool dimming );
	void UpdateScentClues();
	void ProcessUpdateFocusModeVisibilityEntities();

public:

	void RegisterForUpdateFocusModeVisibility( CGameplayEntity* entity );
	void StoreFocusModeVisibility( CGameplayEntity* entity, Bool persistent );
	Bool UpdateFocusModeVisibility( CGameplayEntity* entity );
	Bool GetOverridenSoundClueParams( const CGameplayEntity* entity, SSoundClueParams& params ) const;
	Float CalculateFocusAccuracy(  const SFocusPosition& focusPosition, TCluePtr entity, const Vector& directionToEntity ) const; 
	void GetFocusPosition( SFocusPosition& focusPosition ) const;
	void SetFadeParameters( Float NearFadeDistance, Float FadeDistanceRange, Float dimmingTime, Float dimmingSpeed );
					
	ASSING_R4_GAME_SYSTEM_ID( GSR4_FocusMode );

	void funcSetActive( CScriptStackFrame& stack, void* result );
	void funcIsActive( CScriptStackFrame& stack, void* result );
	void funcGetIntensity( CScriptStackFrame& stack, void* result );
	void funcEnableVisuals(  CScriptStackFrame& stack, void* result );
	void funcEnableExtendedVisuals(  CScriptStackFrame& stack, void* result );
	void funcSetDimming(  CScriptStackFrame& stack, void* result );
	void funcSetFadeParameters( CScriptStackFrame& stack, void* result );
	void funcSetSoundClueEventNames( CScriptStackFrame& stack, void* result );
	void funcActivateScentClue( CScriptStackFrame& stack, void* result );
	void funcDeactivateScentClue( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CFocusModeController )
	PARENT_CLASS( IGameSystem )
	NATIVE_FUNCTION( "SetActive", funcSetActive );
	NATIVE_FUNCTION( "IsActive", funcIsActive );
	NATIVE_FUNCTION( "GetIntensity", funcGetIntensity );
	NATIVE_FUNCTION( "EnableVisuals", funcEnableVisuals );
	NATIVE_FUNCTION( "EnableExtendedVisuals", funcEnableExtendedVisuals );
	NATIVE_FUNCTION( "SetDimming", funcSetDimming );
	NATIVE_FUNCTION( "SetFadeParameters", funcSetFadeParameters );
	NATIVE_FUNCTION( "SetSoundClueEventNames", funcSetSoundClueEventNames );
	NATIVE_FUNCTION( "ActivateScentClue", funcActivateScentClue );
	NATIVE_FUNCTION( "DeactivateScentClue", funcDeactivateScentClue );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CFocusSoundParam : public CGameplayEntityParam
{
public:

	CFocusSoundParam();

	CName GetEventStart() const { return m_eventStart; }
	CName GetEventStop() const { return m_eventStop; }
	Float GetHearingAngle() const { return m_hearingAngle; }
	Float GetVisualEffectBoneName() const { return m_visualEffectBoneName; }

private:

	CName	m_eventStart;
	CName	m_eventStop;
	Float	m_hearingAngle;
	CName	m_visualEffectBoneName;

	DECLARE_ENGINE_CLASS( CFocusSoundParam, CGameplayEntityParam, 0 );

	void funcGetEventStart(  CScriptStackFrame& stack, void* result );
	void funcGetEventStop(  CScriptStackFrame& stack, void* result );
	void funcGetHearingAngle(  CScriptStackFrame& stack, void* result );
	void funcGetVisualEffectBoneName(  CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CFocusSoundParam );
PARENT_CLASS( CGameplayEntityParam );
	PROPERTY_EDIT( m_eventStart, TXT( "Start sound event name" ) );
	PROPERTY_EDIT( m_eventStop, TXT( "Stop sound event name" ) );
	PROPERTY_EDIT( m_hearingAngle, TXT( "Max. angle (between listener forward vector and listener-to-sound vector) within which event can be heared" ) );
	PROPERTY_EDIT( m_visualEffectBoneName, TXT( "Name of the bone that will be used as position to play visual effect. If empty, entity position will be used" ) );
	NATIVE_FUNCTION( "GetEventStart", funcGetEventStart );
	NATIVE_FUNCTION( "GetEventStop", funcGetEventStop );
	NATIVE_FUNCTION( "GetHearingAngle", funcGetHearingAngle );
	NATIVE_FUNCTION( "GetVisualEffectBoneName", funcGetVisualEffectBoneName );
END_CLASS_RTTI();
