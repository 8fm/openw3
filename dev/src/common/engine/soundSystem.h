/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "gameSession.h"
#include "soundAmbientManager.h"
#include "soundFileLoader.h"
#include "soundAdditionalListener.h"
#include "soundListener.h"
#include "MusicSystem.h"
#include "soundWallaSystem.h"

#define SPEAKER_VOLUME_MATRIX AK_SPEAKER_SETUP_5_1

#define BIT(x) (1 << x)


////////////////////   LOGGING SHIT     //////////////////////////////////

#ifndef NO_LOG

	RED_DECLARE_NAME( Sound )

	#define SOUND_LOG( format, ... ) RED_LOG( Sound, format, ## __VA_ARGS__ )
	#define SOUND_WARN( format, ... ) RED_LOG( Sound, format, ## __VA_ARGS__ )
	#define SOUND_ERR( format, ... ) RED_LOG( Sound, format, ## __VA_ARGS__ )

#else

	#define SOUND_LOG( format, ... ) 
	#define SOUND_WARN( format, ... )
	#define SOUND_ERR( format, ... ) 

#endif

// DEPRECATED - implemented to fix issues with entities that can't be changed anymore
struct SSoundEventConversionHelper
{
	enum EConversionType 
	{ 
		ECT_Switch, 
		ECT_RTPC 
	}			m_type;
	StringAnsi	m_name;
	StringAnsi	m_switchValue;
	Float		m_rtpcValue;
};


//////////////////////////////////////////////////////////////////////////

////////////////////   DEBUG WINDOWS     ////////////////////////////////
#ifndef NO_DEBUG_WINDOWS

struct SSoundBankQuests
{
	Uint32 m_refCount;
	TDynArray< String > m_blockPhases;
};

#endif
//////////////////////////////////////////////////////////////////////////

class CTickSoundEmittersTask : public CTask
{
public:
	CTickSoundEmittersTask( Float dt );
	virtual ~CTickSoundEmittersTask() override;

	void FinalizeSoundEmittersTick();
	virtual void Run() override final;

private:
	Float m_delta;

#ifndef NO_DEBUG_PAGES
	virtual const Char* GetDebugName() const override;
	virtual Uint32 GetDebugColor() const override;
#endif // NO_DEBUG_PAGES
};

class CSoundSystem : public Red::System::NonCopyable, public CSoundEmitter, public IGameSaveSection
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

#ifdef SOUND_DEBUG
	friend class CDebugPageWindow;
	friend class CDebugPageSound;
#endif

#ifndef NO_DEBUG_PAGES
	friend class CDebugPageGameplayConfig;
#endif
	friend class CSoundAdditionalListener;

public:
	static const AnsiChar* PARAM_SURFACE;
	
	CSoundSystem();
	virtual ~CSoundSystem();

	//////////////////////////////////////////////////////////////////////////

	static void Init( const String& rootPath );
	void PostInit();
	void FinalizeLoading();
	void Shutdown();
	void Reset();
	Bool IsInitialized() { return m_initialized; }

	void Tick( Float dt );

	static String GetCacheFileName();

#ifndef NO_EDITOR
	TDynArray< String > GetDefinedEnvironments();
	TDynArray< String > GetDefinedEvents();
	TDynArray< String > GetDefinedSwitches();
	TDynArray< String > GetDefinedGameParameters();

	void ReloadSoundbanks();
#endif

	//////////////////////////////////////////////////////////////////////////

	void OnBlackscreenStart();
	void OnBlackscreenEnd();

	void SetMasterVolume( Float volume );
	void SetMusicVolume( Float volume );
	void SetVoiceoversVolume( Float volume );
	void SetSoundsVolume( Float volume );

	Float GetMasterVolume();
	Float GetMusicVolume();
	Float GetVoiceoversVolume();
	Float GetSoundsVolume();

	static Float LinearToDb(Float linVol);

	// DEPRECATED - implemented to fix issues with entities that can't be changed anymore
	Bool CheckSoundEventConversion( const StringAnsi& soundEvent, SSoundEventConversionHelper& foundConversion );

	void ForceReverb( const String& reverbDefinition ) {}

	//! Initialize at game start, called directly in StartGame, but should always be implemented
	virtual void OnGameStart(const CGameInfo& gameInfo);

	//! Shutdown at game end, called directly in EndGame, but should always be implemented
	virtual void OnGameEnd();

#ifndef NO_DEBUG_WINDOWS
	void CheckQuestBank( String bnk, String phaseName );
	void AddQuestBank( String bnk, String phaseName );
	void RemoveQuestBank( String bnk, String phaseName );
	THashMap< String, SSoundBankQuests > GetQuestBanks();
#endif
	
	//////////////////////////////////////////////////////////////////////////

	//! Handle user signing in
	void OnUserEvent( const EUserEvent& event );
	//////////////////////////////////////////////////////////////////////////

#ifdef SOUND_EDITOR_STUFF
	// Mute from editor
	virtual void MuteFromEditor( Bool mute );
#endif

	//////////////////////////////////////////////////////////////////////////

	void SetListenerVectorsFromCameraAndCharacter( const Vector& cameraPosition, const Vector& up, const Vector& forward, const Vector* controllerPosition = 0 );
	void SetListenerVectorsManually( const Vector& cameraPosition = Vector::ZEROS, const Vector& up = Vector::ZEROS, const Vector& forward = Vector::ZEROS );
	void GetListenerVectors( Vector& position, Vector& up, Vector& forward );

	Bool isFreeCamera() const { return m_cameraIsFree; }

	//////////////////////////////////////////////////////////////////////////
	RED_INLINE CSoundAmbientManager& GetAmbientManager() { return m_ambientManager; }
	RED_INLINE CSoundFileLoader&	GetFileLoader()	  { return sm_fileLoader; }

	RED_INLINE CSoundListenerComponent* GetSoundListner() const { return m_SoundListener; }
	RED_INLINE CSoundListenerComponent* GetPanningListner() const { return m_PanningListener; }

	const Vector GetListenerDirection() const;

	//Returns the position of the main volume listener for the game
	const Vector& GetListenerPosition() const;
	//Returns the position of the listener used for panning
	const Vector& GetPanningListenerPosition() const;

	void FlushListener();

	const String& GetDepotBankPath() const { return m_depotBanksPath; }

	CMusicSystem& GetMusicSystem( ) { return m_musicSystem; }

#ifndef NO_EDITOR
	class CSoundAdditionalListener* GetAdditionalListener( Int32 bitmask );
#endif

	//Incrementing the music blocked count will prevent music events being triggered 
	//from sound volumes; remember to decrement after you're done or music won't trigger.
	void IncrementMusicBlockedCount();
	void DecrementMusicBlockedCount();
	bool MusicTriggersAreBlocked() { return m_MusicBlockedCount > 0; }

	void IncrementUnderwaterSpeechMutingCount();
	void DecrementUnderwaterSpeechMutingCount();

	const CSoundWallaSystem &GetWallaSystem() const { return m_wallaSystem; }

	//returns true if it successfully finds the specified parameter, sets the parameter value to retVal, optionally supply
	//a game object id to search first for parameters on the game object
	Bool GetRtpcValue(const char * parameterName, Float &retVal, Uint64 gameObject = 0) const;
	//HACK this is only used for stagger going in at the end of ep2 so I'm writing it very custom for that
	//it is NOT curently safe for general use
	void TimedSoundEvent(String startEvent, String stopEvent, Float duration, Bool updateTimeParameter = false);


private:

	TDynArray< STimedSoundEvent > m_timedSoundEvents;


	CSoundAmbientManager			m_ambientManager;
	CSoundWallaSystem				m_wallaSystem;

	static CSoundFileLoader			sm_fileLoader;
	CScriptSoundSystem*				m_scriptWrapper;

	CMusicSystem					m_musicSystem;
	CSoundListenerComponent*		m_SoundListener;
	CSoundListenerComponent*		m_PanningListener;

	// Volumes from user settings
	Float							m_soundCurrentVolume;
	Float							m_musicCurrentVolume;
	Float							m_voiceoversCurrentVolume;
	Float							m_masterCurrentVolume;

	Bool							m_cameraIsFree;

	Float							m_lastGameTimeUpdate;

	Vector							m_cameraPosition;
	Vector							m_controllerPosition;
	Vector							m_up;
	Vector							m_forward;
	Float							m_multipler;
	Bool							m_manuallListner;

	Bool							m_isBlackscreen;
	Bool							m_callOnBlackscreenEnd;

	Float							m_delta;

	Uint32							m_MusicBlockedCount;
	Uint32							m_underwaterSpeechMutingCount;

	static Bool						m_initialized;
#ifdef RED_PLATFORM_WINPC
	static Bool					m_initializedCom;
#endif

	String							m_depotBanksPath;

	TDynArray< StringAnsi >			m_savedSoundEvents;

	// DEPRECATED - implemented to fix issues with entities that can't be changed anymore
	THashMap< StringAnsi, SSoundEventConversionHelper > m_soundEventConversionMap;

	CTickSoundEmittersTask*			m_soundEmittersTask;

#ifndef NO_DEBUG_WINDOWS
	typedef THashMap< String, SSoundBankQuests > TSoundBankQuestsDynArray;
	TSoundBankQuestsDynArray		m_currentSoundBanks;
#endif
#ifndef NO_EDITOR
	typedef TDynArray< class CSoundAdditionalListener* > SoundListenerDynArray;
	SoundListenerDynArray m_listeners;




	TDynArray< String >				m_definedEnvironments;
	TDynArray< String >				m_definedEvents;
	TDynArray< String >				m_definedSwitches;
	TDynArray< String >				m_definedGameParameters;
	
#endif

	struct SListenerOverride
	{
		String overrideName;
		Vector position;
		Vector up;
		Vector forwards;
	};

	THashMap<String, SListenerOverride >	m_listenerOverrides;
	TDynArray<String> m_listenerOverrideRequests;
	Vector m_cachedListenerPosition;
	Bool m_muteSpeechUnderWater;
	String m_currentLocalization;
	
	void ProcessListener();

public:	
	void OnLoadGame( IGameLoader* loader );

	virtual bool OnSaveGame( IGameSaver* saver );
	void ClearSavedSoundEvents();
	void AddSoundEventToSave( StringAnsi eventName );

	void PreTick( Float dt );
	void IssuePreTick( Float dt );

#ifndef RED_FINAL_BUILD
	virtual StringAnsi GetSoundObjectName() const { return "SOUND_SYSTEM"; }
#endif

	static Bool OutOfMemoryCallback();
	void PushListenerOverride(String overrideTarget);
	void PopListenerOverride();
	void RegisterListenerOverride(String overrideName, Vector position, Vector up, Vector forwards);
	void UnregisterListenerOverride(String overrideName);
	void GenerateEditorFragments(CRenderFrame* frame);
	void MuteSpeechUnderWater(Bool enable);
};
//////////////////////////////////////////////////////////////////////////

extern CSoundSystem* GSoundSystem;
