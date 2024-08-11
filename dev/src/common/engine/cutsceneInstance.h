/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "cutsceneActor.h"
#include "cutscene.h"
#include "entity.h"

struct CSyncInfo;
class CCutsceneStreamer;
class ICutsceneModifierInstance;
class ICutsceneProvider;

class ICutsceneSceneInterface
{
public:
	virtual void OnCameraCut() = 0;
};

class ICutsceneWorldInterface
{
public:
	virtual CWorld* GetWorld() const = 0;

	virtual void ActivateGameCamera( Float blendTime ) = 0;

	virtual void SetBlackscreen( Bool enable, const String& reason, const Color & color = Color::BLACK ) = 0;
	virtual void Fade( Bool fadeIn, const String& reason, Float fadeDuration = 1.f, const Color & color = Color::BLACK ) = 0;

	virtual void OnCutsceneEvent( const String& csName, const CName& csEvent ) = 0;

	virtual void RegisterCutscene( CCutsceneInstance* cs ) = 0;
	virtual void UnregisterCutscene( CCutsceneInstance* cs ) = 0;
};

class CCutsceneInGameWorldInterface : public ICutsceneWorldInterface
{
public:
	virtual CWorld* GetWorld() const;

	virtual void ActivateGameCamera( Float blendTime );

	virtual void SetBlackscreen( Bool enable, const String& reason, const Color & color = Color::BLACK );

	virtual void Fade( Bool fadeIn, const String& reason, Float fadeDuration = 1.f, const Color & color = Color::BLACK );

	virtual void OnCutsceneEvent( const String& csName, const CName& csEvent );

	virtual void RegisterCutscene( CCutsceneInstance* cs );

	virtual void UnregisterCutscene( CCutsceneInstance* cs );
};

class CCutsceneEmptyWorldInterface : public ICutsceneWorldInterface
{
	CWorld*			m_world;

public:
	CCutsceneEmptyWorldInterface( CWorld* world )
		: m_world( world )
	{

	}

public:
	virtual CWorld* GetWorld() const
	{
		return m_world;
	}

	virtual void ActivateGameCamera( Float blendTime ) 
	{ 
		
	}

	virtual void SetBlackscreen( Bool enable, const String& reason, const Color & color = Color::BLACK ) 
	{ 
		
	}

	virtual void Fade( Bool fadeIn, const String& reason, Float fadeDuration = 1.f, const Color & color = Color::BLACK ) 
	{ 
		 
	}

	virtual void OnCutsceneEvent( const String& csName, const CName& csEvent ) 
	{ 
		 
	}

	virtual void RegisterCutscene( CCutsceneInstance* cs )
	{
		
	}

	virtual void UnregisterCutscene( CCutsceneInstance* cs )
	{
		
	}
};

class CCutsceneInstance : public CEntity
{
	DECLARE_ENGINE_CLASS( CCutsceneInstance, CEntity, 0 )

	friend class CCutsceneTemplate;

	typedef TDynArray< CutsceneActor >			TCsActorList;
	typedef TDynArray< TPair< TagList, Int32 > >	TCacheTags;

protected:
	ICutsceneWorldInterface*			m_worldInterface;
	ICutsceneSceneInterface*			m_sceneInterface;

	TDynArray< ICutsceneModifierInstance* > m_modifierInstances;

	TCsActorList						m_actorList;			//! Actors

	TDynArray< CCamera*	>				m_csCameras;			//!	Cameras
	CCamera*							m_csCurrCamera;			//! Current camera

	TDynArray<THandle<CEntity>>			m_hiddenEntities;		//!Entities hidden for the duration of cutscene

	Bool								m_finished;
	Bool								m_ready;
	Bool								m_running;
	Bool								m_isSkipped;
	Int32								m_pauseCount;

	Float								m_timeFactor;
	Float								m_slowMoPrevFactor;
	Float								m_slowMoCurrFactor;
	Float								m_slowMoDestFactor;
	Float								m_csTime;
	Float								m_csTimeRemaining;
	Float								m_csDuration;
	
	Float								m_timeoutTimer;
	static const Float					CS_TIMEOUT;

	Matrix								m_offset;

	Double								m_lastTime;

	Bool								m_selfUpdated;
	Bool								m_autoDestroy;
	Bool								m_looped;
	Bool								m_isGameplay;
	Bool								m_useStreaming;
	CCutsceneStreamer*					m_streaming;

	Bool								m_fadeOutTimer;
	Bool								m_fadeInTimer;
	Float								m_currFade;

	static const CName					CAM_BLEND_IN_SLOT;
	static const CName					CAM_BLEND_OUT_SLOT;

	CCutsceneTemplate*					m_csTemplate;

	TDynArray< String >					m_playingMusic;

	Bool								m_fadeInSended;
	Bool								m_fadeOutSended;

	TDynArray< const CExtAnimCutsceneDurationEvent* > m_runningEvts;

	static const CName					NPC_CLASS_NAME;
	static const CName					ACTOR_CLASS_NAME;
	static const CName					CAMERA_CLASS_NAME;
	static const CName					PLAYER_CLASS_NAME;

	THashMap< const CExtAnimCutsceneEffectEvent*, THandle< CEntity > >	m_csEffects;			//! Effect map

public:
	CCutsceneInstance();

	void Update( Float dt, Bool force = false );
	void UpdateAsync();
	void SkipToEnd();
	Bool IsFinished() const;

	void Play( Bool selfUpdated, Bool autoDestroy, Bool looped, Int32 camera = 0, Float timeout = 5.f );
	void DestroyCs( Bool restoreGameCamera = true );

	CCutsceneTemplate* GetCsTemplate() const;

	void Pause();
	void Unpause();
	Bool IsPause() const;
	void SetTime( Float time );
	void SetTimeMul( Float factor );
	
	Float GetTime() const;
	Float GetDuration() const;
	Float GetTimeRemaining() const;
	Float GetTimeMul() const;

	void ThrowError( const String& errors ) const;

	void ProcessRemainingEvents();

public:
	void DestroyEffects();
	void StopAllEffects();
	void PauseAllEffect( Bool isPaused );
	void StopAllEnvironments();
	void StopAllEvents();

	void StartSlowMoBlending( Float factor );
	void FinishSlowMoBlending( Float factor );
	void ProcessSlowMoBlending( Float factor, Float weight );
	Float GetSlowMo() const;
	void CancelSlowMo();

	void SetIsGameplay( Bool flag );

	Bool HasBoundingBoxes() const;
	void GetBoundingBoxes( TDynArray< Box >& boxes, Matrix& offset ) const;

	void GetActors( TDynArray< CEntity* >& entities ) const;
	void GetNonSceneActors( TDynArray< TPair< CEntity*, Bool > >& entities ) const;
	void GetAllPlayableElements( TDynArray< const CAnimatedComponent* >& elements ) const;
	void CollectAllPlayableActorsWithAnims( TDynArray< const CAnimatedComponent* >& actors, TDynArray< CName >& anims ) const;
	const CAnimatedComponent* GetActorsPlayableElementByAnimation( const CName& animation ) const;
	const CEntity* GetActorsEntityByAnimation( const CName& animation ) const;
	
	const CCamera*		GetCsCamera() const;
	CCameraComponent*	GetCamera() const;
	void				GetAvailableCameras( TDynArray< CCameraComponent* >& cameras ) const;
	Bool				HasCamera() const;
	Bool				HasActiveCamera() const;

	THashMap< const CExtAnimCutsceneEffectEvent*, THandle< CEntity > >&	GetEffectMap() { return m_csEffects; }

	const Matrix& GetCsPosition() const;

	Bool CheckActors( const TDynArray< CEntity*>& actors );
	
	Matrix GetActorFinalPosition( const CEntity* entity ) const;
	Matrix CalcActorInitialPelvisPosition( const CEntity* entity ) const;
	Matrix CalcActorCurrentPelvisPosition( const CEntity* entity ) const;
	Matrix CalcActorFinalPelvisPosition( const CEntity* entity ) const;
	
	CEntity* FindActorByName( const String& actorName );

	void CsToGameplayRequest( Float blendTime = 1.f, Bool flag = true );
	void CsToGameplayRequestForActor( CEntity* entity, Float blendTime, Bool flag = true );

public:
	virtual void OnTimer( const CName name, Uint32 id, Float timeDelta );
	virtual void OnTick( Float timeDelta );
	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld* world );

protected:
	Bool CreateActors( ICutsceneProvider* provider, Bool sync, String& errors );
	
	void DestroyActors();

	void UpdateActors( const CSyncInfo& info );

	Bool LockActors();
	void UnlockActors();

	CutsceneActor* FindActor( const String& name );
	CutsceneActor* FindActor( const CEntity* entity );
	const CutsceneActor* FindActor( const CEntity* entity ) const;

	CName	FindActorAppearance( CEntity* entity, const CName& voicetag ) const;

	void UnloadAllTemplates();

	void RefreshSlowMo();

protected:
	Bool Init( const Matrix& point, ICutsceneProvider* provider, ICutsceneWorldInterface* worldInterface, String& errorMsg, ICutsceneSceneInterface* sceneInterface );

	void Finish();
	void ProcessCsEvents( Float prevTime, Float currTime, Bool skippingFades = false, Bool skippingsounEvt = false );

	void CollectCameras();
	void CreateEffects( String& errors );
	void ResetVariables();

	void AddModifier( ICutsceneModifierInstance* m );
	void DestroyModifiers();
	void ApplyModifiersUpdate();
	void ApplyModifiersPlayCutscene();
	void ApplyBurnedAudioTrackName();
	
	String GetActorName( const CName& animation ) const		{ return m_csTemplate->GetActorName( animation ); }
	String GetComponentName( const CName& animation ) const	{ return m_csTemplate->GetComponentName( animation ); }
	Bool IsRootComponent( const CName& animation ) const		{ return m_csTemplate->IsRootComponent( animation ); }
	Bool IsMimicComponent( const CName& animation ) const		{ return m_csTemplate->IsMimicComponent( animation ); }
	CAnimatedComponent* GetComponent( const CName& animation, CEntity* entity ) const;
	CAnimatedComponent* GetMimicComponent( const CName& animation, CEntity* entity ) const;

	CEntity* SpawnActorEntity( CEntityTemplate* templ, const Matrix& spawnPos, const CName& appearance, const CName& voicetag );
	CEntity* SpawnEffectEntity( const CExtAnimCutsceneEffectEvent& event );
	CEntity* FindGameplayEntity( const TagList& tags, const CName& voiceTag, TCacheTags& cacheTags );

	Matrix GetActorEndPos( const CutsceneActor& actor ) const;

	void SetCamera( CCamera* camera );

	void FadeOn();
	void FadeOff();
	void UpdateFade();

	void ProcessRemainingActorEvents( Float prevTime, Float currTime, Bool skipping );
	void StopAllSounds();

	void StartStreaming();
	Bool UpdateStreaming( Float time );
	void CancelStreaming();

public:
	const String& GetActorName( const CEntity* ent ) const;
	Bool InitStageOne( const Matrix& point, ICutsceneProvider* provider, ICutsceneWorldInterface* worldInterface, Bool sync, String& errorMsg );
	Bool AreActorsReady( String& errors ) const;
	void InitStageTwo( String& errorMsg );
	void Cleanup();

	void SetLooped( Bool looped ) { m_looped = looped; }
	Bool IsLooped() const { return m_looped; }
	void SoundSeekTo( Float newTime );

	void HideEntityForCsDuration( CEntity* ent );
	
	struct CameraState
	{
		Vector		m_position;
		EulerAngles m_rotation;
		Float		m_fov;
		SDofParams	m_dof;

		CameraState()
			: m_position( Vector::ZERO_3D_POINT )
			, m_rotation( EulerAngles::ZEROS )
			, m_fov( 45.f )
		{}
	};
	
	CameraState GetCameraState();
};

BEGIN_CLASS_RTTI( CCutsceneInstance );
	PARENT_CLASS( CEntity );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

#ifdef USE_HAVOK_ANIMATION

class CCutsceneAnimationStreamingJob : public ILoadJob
{
private:
	Uint32					m_firstAnimId;
	Uint32					m_animNums;
	Uint32					m_animParts;
	TDynArray< AnimBuffer* > m_buffers;
	const TDynArray< Uint32 >	m_animBuffPos;
	TDynArray< Uint32 >		m_animOffsets;
	TDynArray< Float >		m_accTimes;
	volatile Uint32			m_lastLoadedPart;

public:
	CCutsceneAnimationStreamingJob( Uint32 firstAnimId, const TDynArray< Uint32 >& animBuffPos, Uint32 animNums );
	virtual ~CCutsceneAnimationStreamingJob();

	Uint32 GetLastLoadedPartNum() const;
	Uint32 GetAnimPartNum() const;
	Float GetAccTime( Uint32 part ) const;
#ifdef USE_HAVOK_ANIMATION
	void MoveLoadedBuffer( tHavokAnimationBuffer& dest, Uint32 anim, Uint32 part );
#endif
public:
	//! Process the job, is called from job thread
	EJobResult Process();

	RED_INLINE Int32 GetSubPriority() const { return m_firstAnimId; }

	virtual const Char* GetDebugName() const override { return TXT("IO Cutscene"); }
};

class CCutsceneStreamer
{
	TDynArray< CSkeletalAnimation* >	m_animationsToStream;
	CCutsceneAnimationStreamingJob*		m_streamingJob;
	Float								m_time;
	Uint32								m_prevPart;
	Bool								m_fadeOwner;

public:
	CCutsceneStreamer( TDynArray< CSkeletalAnimationSetEntry* >& anims );
	~CCutsceneStreamer();

	Bool Update( Float time );

	void KeepFading();

private:
	void PrepareAnimBuffers();
	void MoveDataBufferPart( AnimBuffer* buff, Uint32 anim, Uint32 part );

	void FadeOff();

	void DebugCheck( Float time ) const;
	void DebugFinalCheck() const;
};

#endif
