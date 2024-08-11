
#pragma once

#include "animSyncInfo.h"
#include "entityTemplate.h"

class CBehaviorGraphCutsceneControllerNode;

enum ECutsceneActorType
{
	CAT_None,
	CAT_Actor,
	CAT_Prop,
	CAT_Camera
};

BEGIN_ENUM_RTTI( ECutsceneActorType );
	ENUM_OPTION( CAT_None );
	ENUM_OPTION( CAT_Actor );
	ENUM_OPTION( CAT_Prop );
	ENUM_OPTION( CAT_Camera );
END_ENUM_RTTI();

struct SCutsceneActorDef
{
	SCutsceneActorDef() : m_type( CAT_None ), m_killMe( false ), m_useMimic( false ) {}

	String							m_name;
	TSoftHandle< CEntityTemplate >  m_template;
	TagList							m_tag;
	CName							m_voiceTag;
	CName							m_appearance;
	ECutsceneActorType				m_type;
	TagList							m_finalPosition;
	CName							m_animationAtFinalPosition;		
	Bool							m_killMe;
	Bool							m_useMimic;

	RED_INLINE CEntityTemplate* GetEntityTemplate()	{ return m_template.Get(); }
	RED_INLINE Bool UseFinalPosition() const			{ return !m_finalPosition.Empty(); }

	DECLARE_RTTI_STRUCT( SCutsceneActorDef );
};

BEGIN_CLASS_RTTI( SCutsceneActorDef );
	PROPERTY_RO( m_name, TXT("Actor name") );
	PROPERTY_EDIT( m_tag, TXT("Actor tag - for searching gameplay actors") );
	PROPERTY_CUSTOM_EDIT( m_voiceTag, TXT("Actor voice tag - for filtering gameplay actors"), TXT("EntityVoiceTagSelect") );
	PROPERTY_EDIT( m_template, TXT("Entity tamplate - for spawning actor") );
	PROPERTY_CUSTOM_EDIT( m_appearance, TXT("Entity appearance - for spawning actors and gameplay actors"), TXT("EntityAppearanceSelect") );
	PROPERTY_EDIT( m_type, TXT("Actor type") );
	PROPERTY_EDIT( m_finalPosition, TXT("Actor final position") );
	PROPERTY_EDIT( m_killMe, TXT("Actor will be in death state after with cutscene death pose") );
	PROPERTY_EDIT( m_useMimic, TXT("Use mimic") );
	PROPERTY_EDIT( m_animationAtFinalPosition, TXT("Final idle animation actor should go to after cutscene ( Fill if no trajectory for actor )") )
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct CutsceneActor;

struct CutsceneActorPlayableElement
{
public:
	TDynArray< CName >						m_animations;
	CBehaviorGraphCutsceneControllerNode*	m_controller;

private:
	String									m_componentName;
	THandle< CAnimatedComponent >			m_component;

public:
	Bool IsFor( const CAnimatedComponent* ac ) const
	{
		return m_component.Get() == ac;
	}

	CAnimatedComponent* Get( const CutsceneActor& a );
	const CAnimatedComponent* Get( const CutsceneActor& a ) const;

	CAnimatedComponent* GetRestrict( const CutsceneActor& a );
	const CAnimatedComponent* GetRestrict( const CutsceneActor& a ) const;

	void Set( CAnimatedComponent* ac );
};

enum EActorSource
{
	AS_Spawned,
	AS_Gameplay,
	AS_ExtContext,
	AS_Scene
};

struct CutsceneActor
{
	Bool				m_isLocked;

	THandle< CEntity >	m_entity;
	String				m_name;

	ECutsceneActorType	m_type;

	CName				m_prevApp;
	CName				m_newApp;

	Matrix				m_endPos;

	EActorSource		m_source;

	Bool				m_setDeathPose;
	Bool				m_useMimic;
	Bool				m_wasHiddenByScene;
	Bool				m_wasSnapedToNavigableSpace;
	Bool				m_wasSwitchedToGameplay;
	Float				m_switchedToGameplayBlendTime;

	const CCutsceneInstance* m_cutscene;

	TDynArray< CutsceneActorPlayableElement > m_playableElements;

public:
	CutsceneActor( const CCutsceneInstance* csInstance );

	void AddPlayableElement( CAnimatedComponent* ac, const CName& animation );

	void Init( CWorld* world, Bool sync );
	void PrepareForCutscene();
	void Destroy( Bool isCutsceneFinished );

	void OnSwitchedToGameplay( Bool flag, Float blendTime );

	Bool Lock();
	Bool Lock( CutsceneActorPlayableElement& elem );
	void Relock( CutsceneActorPlayableElement& elem );
	void Unlock( Bool isSkipped );
	Bool IsLocked() const;

	void Update( const CSyncInfo& info, THashMap< const CExtAnimCutsceneSoundEvent*, CAnimatedComponent* >* soundEvents );
	void SetBlendFactor( Float factor );

	Bool IsReady( String& msg ) const;
	RedMatrix4x4 HACK_GetPelvisIdleOffset() const;
private:
	Bool AttachGraphAndFindController();
	Bool AttachGraphAndFindController( CutsceneActorPlayableElement& elem );
	void DetachGraph();

	void PreparePlayableElements( Bool flag );
	void PreparePlayableElements( CutsceneActorPlayableElement& elem, Bool flag );
	void LockStacks( Bool flag );
	void LockStacks( CutsceneActorPlayableElement& elem, Bool flag );
	void SetExternalControl( Bool flag );
	void SetMimics( Bool flag );

	void SetStartingPos();
	void SetEndingPos();
	void ResampleFinalPose();
	void BlendPoseToGameplay( Float blendTime );

	Bool ApplyCsAppearance();
	void RevertCsAppearance();

	void FinishAnimation( Bool isSkipped );
	Bool IsCutsceneFinished() const;

	void SendKillEvent();
	void CacheDeathPose();

	void ParanoidCheck();

	CBehaviorGraphCutsceneControllerNode* FindController( CBehaviorGraphInstance* instance, String& msg ) const;
	
};

