
#pragma once

#include "bgNpcSpeech.h"
#include "bgNpcChatPlayer.h"
#include "bgNpcJobPlayer.h"
#include "bgNpcRoot.h"
#include "bgCounter.h"
#include "../../common/game/sceneActorInterface.h"
#include "../engine/animTickableInterface.h"

class CBgNpcVoicesetPlayer;
class CBgNpcChatPlayer;
class CJobTreePlayer;
class CBgJobTreeObject;
class CLookAtController;
class CPhysicsWrapperInterface;
struct SExtractedSceneLineData;
class CVoicesetParam;
class CNewNpc;

struct SBgNpcJobTree
{
	DECLARE_RTTI_STRUCT( SBgNpcJobTree )

	SBgNpcJobTree() : m_jobTree( NULL ), m_fireTime( 0 ) { }

	CName					m_category;
	TSoftHandle< CJobTree > m_jobTree;
	GameTime				m_fireTime;
};

BEGIN_CLASS_RTTI( SBgNpcJobTree );
	PROPERTY_EDIT( m_jobTree, TXT("JobTree") );
	PROPERTY_EDIT( m_category, TXT("Job Tree Category") );
	PROPERTY_CUSTOM_EDIT( m_fireTime, TXT("FireTime"), TXT( "DayTimeEditor" ) );
END_CLASS_RTTI();

class CBgNpc :	public CEntity,
				public ISceneActorInterface,
				public IAnimAsyncTickable,
				public IAnimSyncTickable,
				public ILocalizableObject
{
	DECLARE_ENGINE_CLASS( CBgNpc, CEntity, 0 );

	enum EBgNpcState
	{
		NS_None,
		NS_Idle,
		NS_Working,
	};

	SBgCounter					m_bucketId;

protected:
	Int32						m_headBoneIndex;
	CVoicesetParam*				m_voiceset;

	CName						m_category;
	TSoftHandle< CJobTree >		m_jobTree;
	TDynArray< SBgNpcJobTree >	m_jobTrees;

	Bool						m_collisionCapsule;

	LocalizedString				m_displayName;
	String						m_fakeDisplayName;

	THandle< CEntityTemplate >	m_originalTemplete;

protected:
	EBgNpcState					m_state;
	volatile Bool				m_isInAsyncTick;

	CBgNpcVoicesetPlayer*		m_voicesetPlayer;
	CBgNpcChatPlayer*			m_chatPlayer;
	CJobTreePlayer*				m_jobPlayer;
	CBgJobTreeObject*			m_jobObject;
	CLookAtController*			m_lookAtController;
	CPhysicsWrapperInterface*	m_physicsBodyWrapper;

	Bool						m_isInTick;
	Uint32						m_currentJobNumber;
	Bool						m_requestedJobTreeChange;

#ifndef NO_EDITOR
	Bool						m_initInEditor;
#endif

public:
	CBgNpc();
	~CBgNpc();

	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld* world );

	virtual void OnAttachFinished( CWorld* world );

	virtual void OnSerialize( IFile& file );

	virtual void OnPropertyPostChange( IProperty* property );

#ifndef NO_EDITOR_ENTITY_VALIDATION
	virtual Bool OnValidate( TDynArray< String >& log ) const;
#endif

public: // ILocalizableObject
	virtual void GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings ) /*const*/;

public:
	// Logic
	void Activate();
	void Deactivate();

	// Work
	Bool CanWork() const;
	Bool StartWorking();
	Bool StopWorking();
	Bool IsWorking() const;

	// Items
	void MountItem( const CName& category );			// Draw item events
	void HoldItem( const CName& category );
	void UnmountItem( const CName& category );

	void MountItemByName( const CName& itemName );		// Anim events - take, leave
	void HoldItemByName( const CName& itemName );
	void UnmountItemByName( const CName& itemName );

	void FireJobEvent( const CName& evtName );

	// Scene
	Bool IsSpeaking() const;

	Bool CanPlayChat() const;
	Bool PlayChat( const SExtractedSceneLineData& data );
	void StopPlayingChat();
	Bool IsPlayingChat() const;

	Bool IsPlayingVoiceset() const;
	Bool CanPlayVoiceset() const;
	Bool PlayDefaultVoiceset();
	Bool PlayVoiceset( const String& voiceset );
	void StopPlayingVoiceset();

	// Look at
	Bool LookAt( const SLookAtInfo& lookAtInfo );
	void DisableAllLookAts();

	void SetLookAtLevel( ELookAtLevel level );
	void GetLookAtParams( const CLookAtDynamicParam*& dynamicParam, const CLookAtStaticParam*& staticParam, CLookAtContextParam& contextParam ) const;

	virtual void OnProcessBehaviorPose( const CAnimatedComponent* poseOwner, const SBehaviorGraphOutput& pose );

	// ISceneActorInterface
	virtual Bool HasMimicAnimation( const CName& slot ) const override;
	virtual Bool PlayMimicAnimation( const CName& anim, const CName& slot, Float blentTime = 0.0f, Float offset = 0.0f ) override;
	virtual Bool StopMimicAnimation( const CName& slot ) override;

	virtual Bool PlayLipsyncAnimation( CSkeletalAnimationSetEntry* anim, Float offset = 0.0f ) override;
	virtual Bool StopLipsyncAnimation() override;

	virtual Bool HasSceneMimic() const				{ ASSERT( 0 ); return false; }
	virtual Bool SceneMimicOn()						{ ASSERT( 0 ); return false; }
	virtual void SceneMimicOff()					{ ASSERT( 0 ); }

	virtual CEntity* GetSceneParentEntity()			{ return this; }

	virtual Vector GetSceneHeadPosition() const;

	virtual Int32 GetSceneHeadBone() const;

	//! Get npc aim position
	virtual Vector GetAimPosition() const;

	//! Get name bar position
	virtual Vector GetBarPosition() const;

	virtual CName GetSceneActorVoiceTag() const;

	virtual Bool WasSceneActorVisibleLastFrame() const { return true; }

	// IAnimAsyncTickable
	virtual EAsyncAnimPriority GetPriority() const;
	virtual Box GetBox() const;
	virtual void DoAsyncTick( Float dt );

	// IAnimSyncTickable
	virtual void DoSyncTick( Float dt );

	// Entity
	virtual String GetDisplayName() const;

private:
	Bool IsInTick() const;
	void AddToTick();
	void RemoveFromTick();

	Bool FindScene( const String& voiceset, TSoftHandle< CStoryScene >& scene, CName& voiceTag ) const;

	CBehaviorGraphStack* GetBehaviorStack();
	const CBehaviorGraphStack* GetBehaviorStack() const;

	Bool SetState( EBgNpcState state );

	Bool UpdateState( Float dt );

	Bool StartStateWorking();
	Bool StopStateWorking();
	Bool ChangeStateWorking();

	Uint32 SelectAppropriateJobIndex( const GameTime& time );

	CBgRootComponent* GetRoot() const;

	void CreateCollisionCapsule( CWorld* world );
	void DestroyCollisionCapsule( CWorld* world );

#ifndef NO_EDITOR
public:
	void CopyFrom( const CNewNPC* npc );
#endif
};

BEGIN_CLASS_RTTI( CBgNpc );
	PARENT_CLASS( CEntity );
	PROPERTY_CUSTOM_EDIT( m_displayName, TXT( "A localized name of this npc" ), TXT( "LocalizedStringEditorIdReset" ) );
	PROPERTY_INLINED( m_voiceset, TXT("Custom (per object) voiceset") );
	PROPERTY_EDIT( m_headBoneIndex, TXT("Head bone index") );
	PROPERTY_EDIT( m_jobTree, TXT("Job tree") );
	PROPERTY_EDIT( m_jobTrees, TXT("Multiple job trees") );
	PROPERTY_EDIT( m_category, TXT("Category for job tree") );
	PROPERTY_EDIT( m_collisionCapsule, TXT("Generate collision box") );
	PROPERTY_RO_NOT_COOKED( m_originalTemplete, TXT("") );
END_CLASS_RTTI();
