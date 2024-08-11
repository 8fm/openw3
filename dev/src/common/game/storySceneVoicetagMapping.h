/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "storyScenePlaybackListener.h"
#include "storySceneForcingMode.h"
#include "storySceneCameraSetting.h"
#include "sceneStopReasons.h"

/************************************************************************/
/* Actor template to match during the search of in the community        */
/************************************************************************/
struct CStorySceneActorTemplate
{
	DECLARE_RTTI_STRUCT( CStorySceneActorTemplate );

	THandle< CEntityTemplate >	m_template;		//!< Template of the entity to map to selected Voice Tag
	TDynArray<CName>			m_appearances;	//!< Appearances of the entity to map to selected Voice Tag
	TagList						m_tags;			//!< Tags of the entity to map to selected Voice Tag

	CStorySceneActorTemplate() : m_template(NULL) {}
};

BEGIN_CLASS_RTTI( CStorySceneActorTemplate );
	PROPERTY_EDIT(	m_template,		TXT( "Template of the entity to map to selected Voice Tag" ) );
	PROPERTY_EDIT(	m_appearances,	TXT( "Appearances of the entity to map to selected Voice Tag" ) );
	PROPERTY_EDIT(	m_tags,			TXT( "Tags of the entity to map to selected Voice Tag" ) );
END_CLASS_RTTI();

/************************************************************************/
/* EStoryScenePerformActionMode                                         */
/************************************************************************/
enum EStoryScenePerformActionMode
{
	SSPAM_PerformAction,
	SSPAM_DoNotPerformAction,
	SSPAM_DoNotCare,
};

BEGIN_ENUM_RTTI(EStoryScenePerformActionMode);
	ENUM_OPTION_DESC( TXT("Perform Action"), SSPAM_PerformAction );
	ENUM_OPTION_DESC( TXT("Don't Perform Action"), SSPAM_DoNotPerformAction );
	ENUM_OPTION_DESC( TXT("Don't care about Action"), SSPAM_DoNotCare );
END_ENUM_RTTI();

/************************************************************************/
/* Definition of the actor position during the scene                    */
/************************************************************************/
struct CStorySceneActorPosition
{
	DECLARE_RTTI_STRUCT( CStorySceneActorPosition );

	TagList							m_position;			//!< Tags of entity that specifies the position of this actor
	Float							m_distance;			//!< Maximal distance from the specified position to perform the scene
	Bool							m_useRotation;		//!< Use positions rotation at the start of the scene
	EStoryScenePerformActionMode	m_performAction;	//!< Should the action in the specified entity be performed?

	CStorySceneActorPosition() : m_distance(1.f), m_useRotation(false), m_performAction(SSPAM_DoNotCare) {}
};

BEGIN_CLASS_RTTI( CStorySceneActorPosition );
	PROPERTY_EDIT(	m_position,			TXT( "Reference to entity that specifies the position of this actor" ) );
	PROPERTY_EDIT(	m_distance,			TXT( "Maximal distance from the specified position to perform the scene" ) );
	PROPERTY_EDIT(	m_useRotation,		TXT( "Use positions rotation at the start of the scene" ) );
	PROPERTY_EDIT(	m_performAction,	TXT( "Should the action in the specified entity be performed?" ) );
END_CLASS_RTTI();

//! 
/************************************************************************/
/* How the Voice Tag can be mapped to entities from community           */
/* and where these entities should be positioned                        */
/************************************************************************/
struct CStorySceneVoicetagMapping
{
	DECLARE_RTTI_STRUCT( CStorySceneVoicetagMapping );

	CName								m_voicetag;				//!< Voice Tag for which this mapping is done

	Bool								m_mustUseContextActor;	//!< Actor must be one of actors suggested by scene registrar
	Bool								m_actorOptional;
	Bool								m_invulnerable;			//!< Should actor be made invulnerable while the scene is playing

	// TODO: change invulnerable default value to true		
	CStorySceneVoicetagMapping()                        : m_voicetag(CName::NONE), m_mustUseContextActor(false), m_invulnerable( false )
	{}
	CStorySceneVoicetagMapping( const CName &voicetag ) : m_voicetag(voicetag),    m_mustUseContextActor(false), m_invulnerable( false )
	{}
};

BEGIN_CLASS_RTTI( CStorySceneVoicetagMapping );
	PROPERTY_RO(	m_voicetag,				TXT( "Voice Tag for which this mapping is done" ) );
	PROPERTY_EDIT(	m_mustUseContextActor,	TXT( "Actor must be one of actors suggested by scene caller" ) );
	PROPERTY_EDIT(	m_invulnerable,			TXT( "Should actor be made invulnerable while the scene is playing" ) );
	PROPERTY_EDIT(	m_actorOptional,		TXT( "Scene can play without this actor dont spawn him if he is not there" ) );
END_CLASS_RTTI();

/************************************************************************/
/* How Voice Tags has been actually mapped to the actors and positions  */
/************************************************************************/
class CAIReactionBase;
class CStorySceneInput;
struct ScenePlayerPlayingContext;
class IStorySceneDisplayInterface;
class IStorySceneDebugger;
class ICutsceneWorldInterface;

struct IControllerSetupActorsProvider
{
	virtual const TDynArray< THandle<CEntity> >* GetActors() = 0;
	virtual const TDynArray< THandle<CEntity> >* GetLights() = 0;
	virtual const TDynArray< THandle<CEntity> >* GetProps() = 0;
	virtual const TDynArray< THandle<CEntity> >* GetEffects() = 0;
};

struct StorySceneControllerSetup
{
	EStorySceneForcingMode						m_forceMode;
	EArbitratorPriorities						m_priority;
	IControllerSetupActorsProvider*				m_contextActorsProvider;
	CNode*										m_suggestedPosition;	// DIALOG_TOMSIN_TODO const
	Bool										m_mustPlay;
	Bool										m_spawnAllActors;
	Bool										m_useApprovedVoDurations;	// True - use durations of approved VO, false - use durations of local VO.
	CWorld*										m_world;
	CPlayer*									m_player;
	CClass*										m_scenePlayerClass;		// DIALOG_TOMSIN_TODO const
	IStorySceneDisplayInterface*				m_sceneDisplay;
	IStorySceneDebugger*						m_sceneDebugger;
	ICutsceneWorldInterface*					m_csWorldInterface;
	Bool										m_asyncLoading;
	void*										m_hackFlowCtrl;

	StorySceneControllerSetup();
};

class CStorySceneController
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Gameplay );

public:
	struct SceneActorInfo
	{
		THandle< CEntity >					m_actor;
		Bool								m_destroyOnFinish;	//!< Actor should be despawned when scene finished
		Bool								m_invulnerable;
		Bool								m_actorOptional;
		CName								m_voicetag;

		SceneActorInfo()                 
			: m_destroyOnFinish(false)
			, m_invulnerable( false )
			, m_actorOptional( false )
		{}
		
		SceneActorInfo( CEntity * actor ) 
			: m_actor(actor)
			, m_destroyOnFinish(false)
			, m_invulnerable( false )
			, m_actorOptional( false )
		{}
	};

	struct ScenePropInfo
	{
		THandle< CEntity >					m_prop;
		CName								m_id;
		Bool								m_destroyAfterScene;

		ScenePropInfo()                 
			: m_destroyAfterScene( false )
		{}

		ScenePropInfo( CEntity* prop ) 
			: m_prop( prop )
			, m_destroyAfterScene( false )
		{}
	};

	class IListener
	{
		DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Gameplay );

	public:
		virtual void OnStorySceneMappingDestroyed( CStorySceneController * mapping ) {}
		virtual void OnStorySceneMappingStopped( CStorySceneController * mapping ) {}
		virtual ~IListener() {}
	};
	
private:
	THandle< CStoryScenePlayer >				m_player;
	Bool										m_started;				//!< This mapping has already given some orders to some actors, use this mapping till it's consumed or stoped
	Bool										m_paused;				//!< Paused?
	THandle< const CStorySceneInput >			m_sceneInput;
	IListener*									m_listener;

	Bool										m_reapplyOrders;		//!< Reapply orders ( after unpausing the mapping )
	
	Bool										m_isQuestScene;
	Float										m_maxStartTime;
	CName										m_activatedOutput;
	SceneCompletionReason						m_completionReason;

	THashMap< CActor*, SActorRequiredItems >	m_actorItems;

	// DIALOG_TIOMSIN_TODO - ten public to hack
public:
	Bool										m_allConditionsMet;		//!< Filled by GetAvailableInstance method
	EStorySceneForcingMode						m_forcingMode;			//!< Scene forcing mode
	EArbitratorPriorities						m_priority;				//!< Priority of the scene
	TDynArray< SceneActorInfo >					m_mappedActors;			//!< Voice tag to actor and position mapping
	TDynArray< ScenePropInfo >					m_mappedProps;
	TDynArray< ScenePropInfo >					m_mappedEffects;
	TDynArray< ScenePropInfo >					m_mappedLights;
	TDynArray< THandle< CActor > >				m_lockedActors;

	// HACK!!!
	TDynArray< THandle< CEntity > >				m_externalContextActors;

public:
	RED_INLINE CName GetActivatedOutput() const { return  m_activatedOutput; }
	RED_INLINE Bool  IsStarted() const { return m_started; }
	RED_INLINE Bool  IsPlaying() const { return m_player.Get() != nullptr; }
	RED_INLINE SceneCompletionReason GetCompletionResult() const { return m_completionReason; }
	
	Bool IsPaused()  const;

	RED_INLINE const CStorySceneInput* GetInput()	const { return m_sceneInput.Get(); }
	RED_INLINE CStoryScenePlayer	* GetPlayer()	const { return m_player; }

	RED_INLINE void SetListener( IListener * listener ) { m_listener = listener; }

	CStorySceneController( const CStorySceneInput* sceneInput );
	CStorySceneController( const CStorySceneInput* sceneInput, EArbitratorPriorities priority, EStorySceneForcingMode forcingMode, Bool isQuestScene = false );
	~CStorySceneController();

	Bool IsPositionReached( SceneActorInfo & mappedActor, Bool force );

	void StoreActorItems( CActor* actor );
	void ForgetActorItems( CActor* actor );
	void RestoreActorItems();

	Bool Start();				//!< Lock the actors, force conditions, return true if we are ready to play the scene
	Bool Play( const ScenePlayerPlayingContext& context );				//!< Play the scene if all conditions has been met
	void Pause( Bool pause );	//!< Pause/resume scene

	// DIALOG_TIOMSIN_TODO - tu sa jakies hardcore hacki - controler usuwa playera a player controlera w srodku!
	void Stop( SceneCompletionReason stopReason );				//!< Stop the scene and release the actors

	void Free();				//!< Free the mapping

	// DIALOG_TIOMSIN_TODO - po co tyle funkcji publicznych, wystarczy play, stop, pause!
	void LockActor( CActor* actor, Bool setInvulnerable = true );
	void UnlockActor( CActor* actor );
	Bool IsActorLocked( CActor* actor ) const;

	void RegisterNewSceneActor( CActor* actor, CName actorName, Bool dontDestroyAfterScene );
	Bool UnregisterSceneActor( CActor* actor, Bool preserveMappingActors );

	Bool IsActorOptional( CName id ) const;

	Bool IsMappingActor( CActor* actor );
	Bool IsMappingActor( const CName& actorVoicetag );

	Bool IsLightDestroyedAfterScene( CName id );
	Bool IsPropDestroyedAfterScene( CName id );
	Bool IsEffectDestroyedAfterScene( CName id );

	void RegisterSpawnedLight( const CName& lightName, CEntity* prop );
	void RegisterSpawnedProp( const CName& lightName, CEntity* prop );
	void RegisterSpawnedEffect( const CName& lightName, CEntity* prop );

	void MarkLightForDestruction( CName id, Bool mark = true );
	void MarkEffectForDestruction( CName id, Bool mark = true );
	void MarkPropForDestruction( CName id, Bool mark = true );

	TDynArray< SceneActorInfo >& GetMappedActors() { return m_mappedActors; }
	TDynArray< ScenePropInfo >& GetMappedLights() { return m_mappedLights; }

	Bool CanStopByExternalSystem() const;
	Bool IsGameplayScene() const;

	const TDynArray< THandle< CActor > >&	GetLockedActors() { return m_lockedActors; }
	CActor* FindPlayerActor();

	friend IFile& operator<<( IFile& file, CStorySceneController* controller );
	/*
private:
	Bool AllocateActors();*/
};
