/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "reactionScene.h"
#include "storyScene.h"

class CReactionSceneActorComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CReactionSceneActorComponent, CComponent, 0 );

	enum EAvailableSceneInputsFetchingState
	{
		ASIFS_NotFetched,
		ASIFS_Fetching,
		ASIFS_Fetched
	};

private:
	Float						m_cooldownInterval;
	THandle< CReactionScene >	m_scene;
	CName						m_sceneRole;
	Bool						m_sceneStartedSuccesfully;

	Float						m_cooldown;

	EAvailableSceneInputsFetchingState	m_availableSceneInputsFetchingState;
	TDynArray< String >					m_availableSceneInputs;
	TSoftHandle< CStoryScene >			m_storyScene;
	TDynArray< String >					m_requiredSceneInputs;
	String								m_voicesetName;
	CName								m_voicesetGroup;
	Bool								m_voicesetGroupCached;
	
public:

	CReactionSceneActorComponent()
		: m_cooldownInterval( 10 )
		, m_sceneRole( CName::NONE )
		, m_cooldown( 0 )
		, m_sceneStartedSuccesfully( false )
		, m_availableSceneInputsFetchingState( ASIFS_NotFetched )
		, m_voicesetName( String::EMPTY )
		, m_voicesetGroup( CName::NONE )
		, m_voicesetGroupCached( false )
	{
		m_cooldown = 0;
	}
	
	RED_INLINE void SetRequiredSceneInputs( TDynArray< String >& requiredSceneInputs ){ m_requiredSceneInputs = requiredSceneInputs; }
	RED_INLINE void SetRequiredSceneInput( const String& requiredSceneInput ){ m_requiredSceneInputs.ClearFast(); m_requiredSceneInputs.PushBack( requiredSceneInput ); }

	RED_INLINE TDynArray< String >& GetRequiredSceneInputs( ){ return m_requiredSceneInputs; }

	RED_INLINE CReactionScene* GetReactionScene(){ return m_scene.Get(); }
	RED_INLINE void SetReactionScene( CReactionScene* scene ){ m_scene = scene; }
	RED_INLINE void SetSceneStartedSuccesfully( Bool sceneStartedSuccesfully ){ m_sceneStartedSuccesfully = sceneStartedSuccesfully; }
	RED_INLINE Bool GetSceneStartedSuccesfully(){ return m_sceneStartedSuccesfully; }

	RED_INLINE CName GetRole(){ return m_sceneRole; }
	RED_INLINE void SetRole( CName role ){ m_sceneRole = role; }
	
	RED_INLINE Bool CanPlayInScene(){ return IsFree() && m_cooldown < ( Float ) GCommonGame->GetEngineTime(); }

	RED_INLINE Bool IsFree(){ return !m_sceneRole; }

	void FinishRole( Bool interupted );
	Bool IsInvoker();
	void LeaveScene();
	Bool IfSceneInterupted();

	void OnAttached( CWorld* world ) override;
	void OnDetached( CWorld* world ) override;

	bool IfActorHasSceneInput( TDynArray< String >& requestedInput );

	bool IfActorHasSceneInput( String& requestedInput );

	const String& GetVoicesetName();
	CName GetVoicesetGroup();
	
	virtual bool UsesAutoUpdateTransform() override { return false; }

	void LockChats();
	void UnlockChats();

private:
	void LazyInitAvailableSceneInputs();
	void FetchAvailableSceneInputs();
};

BEGIN_CLASS_RTTI( CReactionSceneActorComponent );
	PARENT_CLASS( CComponent );
	PROPERTY_EDIT( m_cooldownInterval, TXT("") );
	PROPERTY( m_sceneStartedSuccesfully );
END_CLASS_RTTI();
