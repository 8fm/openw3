/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "sceneActorInterface.h"
#include "../engine/soundEmitter.h"

class CActor;
class LanguagePack;
struct SSoundEventHandle;
class IStorySceneDisplayInterface;

enum EActorSpeechMode
{
	ASM_Text		= FLAG( 0 ),
	ASM_Voice		= FLAG( 1 ),
	ASM_Lipsync		= FLAG( 2 ),
	ASM_Gameplay	= FLAG( 3 ),
	ASM_Subtitle	= FLAG( 4 ),
	ASM_Immediate	= FLAG( 5 ),
	ASM_DialogVoice	= FLAG( 6 ),		// As opposed to ASM_Voice, this one doesn't play vo_2d event, and does not use external source file
};

struct ActorSpeechData : public Red::System::NonCopyable
{
	Uint32							m_speechId;
	StringAnsi						m_soundEventName;
	Bool							m_sync;
	Int32							m_modeFlags;
	Float							m_progress;
	IStorySceneDisplayInterface*	m_sceneDisplay;
	Bool							m_disabledOcclusion;
	Bool							m_alternativeUI;

	ActorSpeechData( Uint32 speechId, const StringAnsi& soundEventName, Bool sync, Int32 modeFlags, Bool disabledOcclusion = false, Bool alternativeUI = false );
	ActorSpeechData( const String& stringKey, const StringAnsi& soundEventName, Bool sync, Int32 modeFlags, Bool disabledOcclusion = false, Bool alternativeUI = false );

	void Init( Uint32 speechId, const StringAnsi& soundEventName, Bool sync, Int32 modeFlags, Bool disabledOcclusion = false, Bool alternativeUI = false );
};

typedef void* TActorSpeechID;
#define TActorSpeechInvalidID NULL

class CActorSpeech
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Dialog );

protected:
	static const Float NO_TEXT_DISTANCE;
	static const Float NO_MIMIC_DISTANCE;
	static const Float NO_VOICE_DISTANCE;
	static const Float NO_GESTURE_DISTANCE;

	static const Float VO_PLAYER_DISTANCE_UPDATE_FREQUENCY;

	static const Float	WAIT_FOR_MIMIC_LOW;
	static const Float	WAIT_FOR_MIMIC_HIGH;

	static const StringAnsi VO_EVENT_DEFAULT;
	static const StringAnsi VO_EVENT_DEFAULT_GAMEPLAY;
	static const StringAnsi VO_EVENT_CINEMATIC;
	static const StringAnsi VO_EVENT_PARAMETER;
	static const StringAnsi VO_PLAYER_DISTANCE_PARAMETER;

protected:
	Uint32									m_speechId;
	ISceneActorInterface*					m_actor;
	StringAnsi								m_soundEventName;
	Bool									m_disabledOcclusion;
	IStorySceneDisplayInterface*			m_sceneDisplay;
	Bool									m_alternativeUI;
	
	Float									m_waitForMimic;		// Number of tries to ensure high mimic during speech
	Int32									m_modeFlags;
	Float									m_duration;
	Float									m_progress;

	Float									m_distanceParameterUpdateTimer;

	LanguagePack*							m_languagePack;
	CSkeletalAnimationSetEntry*				m_lipsyncAnimationEntry;

	SSoundSequenceCallbackData				m_callbackData;
	CSoundEmitterOneShot::TSoundSequenceID	m_dialogVoiceID;

	Bool									m_loaded					: 1;
	Bool									m_playing					: 1;
	Bool									m_finished					: 1;
	Bool									m_isSync					: 1;
	Bool									m_soundWasPlayed			: 1;	// Saves information about voiceover play success/failure
	Bool									m_dialogVoicePlayed			: 1;	// Saves information about voiceover play success/failure (new system)
	Bool									m_dialogVoicePlayingError	: 1;
	Bool									m_updateDistanceParameter	: 1;
	Bool									m_isLipsyncPlayed			: 1;

public:
	CActorSpeech( ISceneActorInterface* actor, const ActorSpeechData& setup );
	~CActorSpeech();

	friend IFile& operator<<( IFile &file, CActorSpeech &actorSpeech );

	Bool	Load();
	void	Play( Float progress );
	void	Cancel();
	void	Update( Float timeDelta );
	Bool	SetTimeState( Float time, Float progress );

	Float	GetDuration() const;

	void	ReplayLipsync();

	RED_INLINE Uint32	GetSpeechId() const		{ return m_speechId; }
	RED_INLINE Bool		IsLoaded() const		{ return m_loaded; }
	RED_INLINE Bool		IsPlaying()	const		{ return m_playing; }
	RED_INLINE Bool		IsFinished() const		{ return m_finished; }
	RED_INLINE Float	GetProgress() const		{ return m_progress; }

	RED_INLINE Bool	HasText() const			{ return ( m_modeFlags & ASM_Text ) ? true : false; }
	RED_INLINE Bool	HasVoice() const		{ return ( m_modeFlags & ASM_Voice ) ? true : false; }
	RED_INLINE Bool	HasLipsync() const		{ return ( m_modeFlags & ASM_Lipsync ) ? true : false; }
	RED_INLINE Bool	IsOneliner() const		{ return ( m_modeFlags & ASM_Gameplay ) && !( m_modeFlags & ASM_Subtitle ); }
	RED_INLINE Bool	IsSubtitle() const		{ return ( m_modeFlags & ASM_Gameplay ) && ( m_modeFlags & ASM_Subtitle ); }
	RED_INLINE Bool	IsGameplay() const		{ return ( m_modeFlags & ASM_Gameplay ) ? true : false; }
	RED_INLINE Bool	HasDialogVoice() const	{ return ( m_modeFlags & ASM_DialogVoice ) ? true : false; }
	
protected:
	void PlayText();
	void PlaySound( Float progress );
	void PlayLipsync( Float progress );
	void StopText();
	void StopSound();
	void StopLipsync();
	void PlayDialogVoice( Float progress );
	void UpdateDialogVoice( Float timeDelta );
	void StopDialogVoice();
	void UpdatePlayerDistance();

	void ValidateModes();

	Vector	GetHeadPosition( ISceneActorInterface* actor );
	Bool	EnsureMimicOn( ISceneActorInterface* actor, Float timeDelta );
};
