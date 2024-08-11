/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/sortedmap.h"

#include "../engine/renderVisibilityQuery.h"
#include "../physics/physicalCallbacks.h"
#include "../engine/entityHandle.h"
#include "../engine/characterControllerParam.h"

#include "gameplayEntity.h"

#include "actorActionSlideTo.h"
#include "actorActionMoveOnCurveTo.h"
#include "actorActionMoveTo.h"
#include "actorActionRotateTo.h"
#include "actorActionExploration.h"
#include "actorActionMoveToDynamicNode.h"
#include "actorActionCustomSteer.h"
#include "actorActionMatchTo.h"
#include "actorActionMoveOutsideNavdata.h"

#include "actorAction.h"
#include "actorActionAnimation.h"
#include "actorActionRiseEvent.h"
#include "actorActionWork.h"

#include "behTreeEventParams.h"
#include "behTreeArbitratorPriorities.h"
#include "behTreeDynamicNodeEvent.h"
#include "aiStorageExternalPtr.h"

#include "attitude.h"
#include "attitudeManager.h"
#include "lookAtController.h"
#include "sceneActorInterface.h"
#include "actorSpeech.h"
#include "actorSpeechQueue.h"
#include "asyncCheckResult.h"

///////////////////////////////////////////////////////////////////////////////

class CAppearanceComponent;
class CMovingAgentComponent;
class CMorphedMeshManagerComponent;
class CNPCSpawnSetComponent;
class CMovable;
class CStorySceneController;
class CActorSpeech;
class CAnimationEventFilter;
class CMoveTRGScript;
class CBehTreeMachine;
class CAIAttackRange;
class CAIDefaults;
class CBehTree;
class IBehTreeNodeDefinition;
class CLookAtStaticParam;
class CVoicesetPlayer;
class CBaseDamage;
class CAIBaseTree;
struct ActorSpeechData;

struct SItemDefinition;
struct SItemSets;
struct SInventoryItem;
struct SActorLODInstanceInfo;

enum SceneCompletionReason : CEnum::TValueType;

typedef THashMap< CActor*, EAIAttitude > TActorAttitudeMap;

///////////////////////////////////////////////////////////////////////////////

enum EPushingDirection : CEnum::TValueType
{
	EPD_Front,
	EPD_Back
};
BEGIN_ENUM_RTTI( EPushingDirection );
	ENUM_OPTION( EPD_Front );
	ENUM_OPTION( EPD_Back );
END_ENUM_RTTI();

///////////////////////////////////////////////////////////////////////////////

enum EExplorationState
{
	EX_Normal,
	EX_WithRangedWeapon,
	EX_WithWeapon,
	EX_Drunk,
	EX_Injured,
	EX_Stealth,
	EX_CarryTorch,
	EX_WithPickaxe
};

BEGIN_ENUM_RTTI( EExplorationState );
	ENUM_OPTION( EX_Normal );
	ENUM_OPTION( EX_WithRangedWeapon );
	ENUM_OPTION( EX_WithWeapon );
	ENUM_OPTION( EX_Drunk );
	ENUM_OPTION( EX_Injured );
	ENUM_OPTION( EX_Stealth );
	ENUM_OPTION( EX_CarryTorch );
	ENUM_OPTION( EX_WithPickaxe );
END_ENUM_RTTI();

///////////////////////////////////////////////////////////////////////////////

struct SActorRequiredItems
{
	TDynArray< TPair<CName,CName> > m_slotAndItem;

	SActorRequiredItems()
	{}

	//most common case for human
	SActorRequiredItems( CName leftHandItem, CName rightHandItem )
		 : m_slotAndItem( 2 )
	{
		m_slotAndItem[0] = TPair<CName,CName>( CNAME( l_weapon ), leftHandItem);
		m_slotAndItem[1] = TPair<CName,CName>( CNAME( r_weapon ), rightHandItem);
	}
};

///////////////////////////////////////////////////////////////////////////////
/// Agent type
enum EPathEngineAgentType
{
	PEAT_Player,							//!< Player
	PEAT_TallNPCs,							//!< tall NPCs ( humans, elves )
	PEAT_ShortNPCs,							//!< short NPCs( children, dwarves )
	PEAT_Monsters,							//!< Monsters
	PEAT_Ghost,								//!< Ghosts ( formations etc. )
};
BEGIN_ENUM_RTTI( EPathEngineAgentType );
	ENUM_OPTION( PEAT_Player );
	ENUM_OPTION( PEAT_TallNPCs );
	ENUM_OPTION( PEAT_ShortNPCs );
	ENUM_OPTION( PEAT_Monsters );
	ENUM_OPTION( PEAT_Ghost );
END_ENUM_RTTI();

///////////////////////////////////////////////////////////////////////////////
// Attitude group priority

// it would be nice to have generic Greater<T> comparer struct
struct EAttitiudeGroupPriorityComparer
{
	static RED_INLINE Bool Less( Int32 a, Int32 b ) { return a > b; }
};

typedef TSortedMap< Int32, CName, EAttitiudeGroupPriorityComparer > TTemporaryAttitudeGroups;

///////////////////////////////////////////////////////////////////////////////

enum EAIEventType
{
	EAIE_Unknown,
	EAIE_Goal,
	EAIE_Action,
	EAIE_Sense,
	EAIE_State,
	EAIE_Movement,
	EAIE_Misc,
};
BEGIN_ENUM_RTTI( EAIEventType );
	ENUM_OPTION( EAIE_Unknown );
	ENUM_OPTION( EAIE_Goal );
	ENUM_OPTION( EAIE_Action );
	ENUM_OPTION( EAIE_Sense );
	ENUM_OPTION( EAIE_State );
	ENUM_OPTION( EAIE_Movement );
	ENUM_OPTION( EAIE_Misc );
END_ENUM_RTTI();

enum EAIEventResult
{
	EAIR_InProgress,
	EAIR_Success,
	EAIR_Failure,
	EAIR_Exception,
	EAIR_Interrupted
};

class IAIDebugListener
{
public:
	virtual ~IAIDebugListener() {}

	virtual void OnAIEvent( EAIEventType type, EAIEventResult result, const String& name, const String& description, Float time ) = 0;

	virtual void OnAIEventStart( EAIEventType type, const String& name, const String& description, Float startTime ) = 0;

	virtual void OnAIEventEnd( Float endTime, EAIEventType type, EAIEventResult result ) = 0;
};

class CEncounter;

class IActorTerminationListener
{
public:
	virtual ~IActorTerminationListener() {}

	virtual void OnDeath( CActor* actor ) = 0;
	virtual void OnDetach( CActor* actor ) = 0;
	// Called before the actor is moved into the general pooling system
	virtual void OnPrePooling( CActor* actor ){}
	virtual CEncounter*  AsEncounter() { return nullptr; }

	// Stray actor interface ( See CStrayActorManager )
	virtual Bool CanBeConvertedToStrayActor()const { return true; }
	virtual void OnConvertToStrayActor( CActor *const actor ) { }
};

struct SActorLODState
{
	Float					m_timeInvisible;
	const SActorLODConfig*	m_desiredLOD;

	SActorLODState()
		: m_timeInvisible( 0.0f )
		, m_desiredLOD( nullptr )
	{}
};

///////////////////////////////////////////////////////////////////////////////

class CNewNpc;

/// The scene Actor
class CActor :	public CGameplayEntity,
				public IActorInterface,
				public ISceneActorInterface,
				public IPhysicalCollisionTriggerCallback
{
	DECLARE_ENGINE_CLASS( CActor, CGameplayEntity, 0 );

	friend class CActorsManager;
private:
	mutable CMovingAgentComponent*				m_agentComponent;		//!< Agent component, the root of all evil
	EntityHandle								m_cachedEntityHandle;	//!< Cached entity handle of actor

	CLookAtController							m_lookAtController;

	mutable CMorphedMeshManagerComponent*		m_morphedMeshManagerComponent;

protected:
	EActorAnimState								m_actorAnimState;

	EPathEngineAgentType						m_actorGroups;			//!< Group to which this actor belongs

	CName										m_voiceTag;				//!< Actor's voice tag
	TDynArray<CStorySceneController*>			m_storyScenes;			//!< Scenes we are playing in
	CVoicesetPlayer*							m_voicesetPlayer;

	TDynArray< StringAnsi >						m_voiceToRandomize;

	CAnimationEventFilter*						m_animationEventFilter;

	Float										m_aimOffset; //!< Vertical target aim offset
	Float										m_barOffset; //!< GUI health bar offset

	mutable String								m_cachedDisplayName; // cache

	CName										m_frontPushAnim;			//!< Front push animation
	CName										m_backPushAnim;				//!< Back push animation

	CMimicComponent*							m_mimicComponent;

protected:
	SActorRequiredItems							m_requiredItemsState;

	Bool										m_hasInteractionVoicesetScene;
	Bool										m_isAIControlled;
	Bool										m_freezeOnFailedPhysicalRequest;

	Bool										m_alive;
	Bool										m_externalControl;		//!< Actor is controlled by code
	Bool										m_lockedByScene;
	Bool										m_isInNonGameplayScene;
	Bool										m_isInGameplayScene;

	Bool										m_isUsingExploration;
	Bool										m_usesRobe;
	Bool										m_appearanceChanged;
	Bool										m_forceAppearanceSoundUpdate;

	Bool										m_isAttackableByPlayer;				//!< If true than player cannot attack this actor
	Bool										m_isCollidable;						//!< Collides with other agents?
	Bool										m_isVisibileFromFar;				//!> Override LOD system 'hide in game' logic
	//Bool										m_spawnHidden;						//!< Çode pushed in disabled form, as we are evaluating to use it with creatures spawning with animation. Spawns 'hidden'
	Bool										m_useHiResShadows;					//!< Use hi resolution shadow for actor ( cutscenes and dialog scenes )
	Bool										m_useGroupShadows;					//!< Use group shadows
	Bool										m_isInFFMiniGame;					//!< Directly moved from scripts (so we can consider it a hack). NOTICE: its being changed by the scripts.
	Bool										m_isLookAtLevelForced;	
	Bool										m_useAnimationEventFilter;
	ELookAtLevel								m_forcedLookAtLevel;
	Uint16										m_hideReason;									

	CName										m_pelvisBoneName;
	CName										m_torsoBoneName;
	CName										m_headBoneName;

	Int32										m_losTestBoneIndex;
	Int32										m_torsoBoneIndex;
	Int32										m_headBoneIndex;

	EExplorationState							m_movementType;

	EngineTime									m_attackableByPlayerDisabledTime;	//!< If higher than current EngineTime actor cannot be attacked

	CBehTreeMachine*							m_behTreeMachine;

	CVisualDebug*								m_visualDebug;			//!< Visual debug
	TDynArray< CActor* >						m_attackers;			//!< List of attackers, attacking this actor

	
	CSoundEntityParam*							m_cachedSoundParams;

	SActorLODState								m_LOD;

#ifndef NO_ERROR_STATE
	mutable Bool				m_dbgIsErrorState;	//!< True if NPC is in error state
	mutable String				m_dbgErrorState;	//!< Used only for debugging purposes. When NPC is in bad state, than this member
#endif

protected:

	IRenderEntityGroup*			m_entityGroup;			//!< Rendering group for this actor


protected:
	ActorAction*					m_action;					//!< Current action
	ActorActionMoveTo				m_actionMoveTo;				//!< MoveTo action
	ActorActionRotateTo				m_actionRotateTo;			//!< RotateTo action
	ActorActionPlayAnimation		m_actionAnimation;			//!< Animation action
	ActorActionRaiseEvent			m_actionRaiseEvent;			//!< Raise behavior event action
	ActorActionSlideTo				m_actionSlideTo;			//!< Slide action
	ActorActionMoveOnCurveTo		m_actionMoveOnCurveTo;		//!< Move on curve action
	ActorActionWork					m_actionWork;				//!< Working action
	ActorActionExploration			m_actionExploration;		//!< Traverse an exploration
	ActorActionMoveToDynamicNode	m_actionMoveToDynamicNode;	//!< Follow dynamic node action
	ActorActionAnimatedSlideTo		m_actionAnimatedSlideTo;	//!< Slide action with animation
	ActorActionCustomSteer			m_actionCustomSteer;		//!< Strafe action
	ActorActionMatchTo				m_actionMatchTo;			//!< Action match to
	ActorActionMoveOutsideNavdata	m_actionMoveOutsideNavdata;	//!< Action to be able to move not being on navdata

	CActorSpeechQueue			m_speechQueue;
	CActorSpeech*				m_speech;

	THandle<CActor>				m_attackTarget;			//!< Last attack target
	EngineTime					m_attackTargetSetTime;	//!< Last attack target time

	CName						m_currentAttitudeGroup;	//!< Current attitude group
	CName						m_baseAttitudeGroup;	//!< Base attitude group (loaded from profile or changed in quests)
	TTemporaryAttitudeGroups	m_temporaryAttitudeGroups;	//!< Temporary attitude groups with priorities higher than base group

protected:
	EActorActionType			m_latentActionType;		//!< Latent action being performed ( heavy internal )
	EActorActionResult			m_latentActionResult;	//!< Result of latent action
	Uint32						m_latentActionIndex;	//!< Counter for latent actions performed ( heavy internal )

	TDynArray< IAIDebugListener* >	m_aiDebugListeners;
	TDynArray< IActorTerminationListener* > m_terminationListeners;

	Int32						m_encounterGroupUsedToSpawn;

private:
	class PushReactionController : public ISlotAnimationListener
	{
	private:
		CBehaviorGraphStack*	m_behaviorStack;
		Bool					m_animationBeingPlayed;

	public:
		PushReactionController( CBehaviorGraphStack* behaviorStack );
		virtual ~PushReactionController() { }

		//! Plays an animation
		void PlayAnimation( CName animName );

		//! stops playing the push animation
		void StopAnimation();

		//! Animation in slot has ended
		virtual void OnSlotAnimationEnd( const CBehaviorGraphAnimationBaseSlotNode * sender, CBehaviorGraphInstance& instance, ISlotAnimationListener::EStatus status );
		virtual String GetListenerName() const { return TXT("PushReactionController"); }
	};

	PushReactionController*					m_pushController;

	String									m_soundListenerOverride;

public:		
	RED_INLINE void		SetForcedLookatLevel( Bool forced, ELookAtLevel level = LL_Null ){ m_isLookAtLevelForced = forced; m_forcedLookAtLevel = level; }

	RED_INLINE CActorSpeechQueue*		 GetSpeechQueue(){ return &m_speechQueue; }
	RED_INLINE ActorActionExploration* GetActionExploration(){ return &m_actionExploration; }

	RED_INLINE Bool UsesAnimationEventFilter() const { return m_useAnimationEventFilter; }
	RED_INLINE CAnimationEventFilter* GetAnimationEventFilter() { return m_animationEventFilter; }

	RED_INLINE Bool IsInFistFightMiniGame() const { return m_isInFFMiniGame; }

	TDynArray< IActorTerminationListener* >& GetTerminationListeners() { return m_terminationListeners; }

	//! Updates LOD
	void UpdateLODLevel( const SActorLODInstanceInfo* dynamicInfo );
	//! Gets current LOD state
	const SActorLODState& GetLODState() const { return m_LOD; }

	Bool IsGameplayLODable() override;
	Bool IsDependentComponentGameplayLODable( CComponent* component ) override { return false; } // Excluding CActor owned components from component-LOD system because these are handled by CActorsManager's LOD system

	//! Plays a proper push animation
	void PlayPushAnimation( EPushingDirection animDirection );

	//! Pushes the actor in the specified direction
	void PushInDirection( const Vector& pusherPos, const Vector& pushDir, Float speed, Bool playAnimation = true, Bool applyRotation = true );

	//! Get actor voice tag
	CName GetVoiceTag() const;

	//! Called when we need to store gameplay state of this entity
	virtual void OnSaveGameplayState( IGameSaver* saver ) override;

	//! Called when we need to restore gameplay state of this entity
	virtual void OnLoadGameplayState( IGameLoader* loader ) override;

	//! Get cached entity handle
	RED_INLINE const EntityHandle& GetEntityHandle()
	{
		if( m_cachedEntityHandle.Empty() )
		{
			m_cachedEntityHandle.Set( this );
		}

		return m_cachedEntityHandle;
	}

	Bool IsVisibleForMonsters() const;

	//! Check if actor is doing something more important
	virtual Bool IsDoingSomethingMoreImportant( Int32 newPriority, String* errorMessage = NULL ) const;

	virtual Bool CanPlayQuestScene( Bool noLog = false ) const;

	virtual Bool HasInteractionScene() const;

	Bool CanTalk( Bool ignoreCurrentSpeech = false ) const;

	virtual Bool OnExplorationStarted()		{ m_isUsingExploration = true; return true; }
	virtual void OnExplorationEnded()		{ m_isUsingExploration = false; }
	Bool IsUsingExploration() const			{ return m_isUsingExploration; }

	//! Check if it is player
	virtual Bool IsPlayer() const { return false; }

	//! Check if actor is alive
	RED_INLINE Bool IsAlive() const { return m_alive; }

	//! Set alive value
	void SetAlive( Bool flag );

	void SetIsAIControlled( Bool b )		{ m_isAIControlled = b; }
	Bool IsAIControlled() const				{ return m_isAIControlled; }

	//! Is attackable by player
	Bool IsAttackableByPlayer() const { return m_isAttackableByPlayer && GGame->GetEngineTime() > m_attackableByPlayerDisabledTime; }

	//! Returns whether the current appearance of the NPC uses a robe
	RED_INLINE Bool GetUsesRobe()		const { return m_usesRobe; }

	// Set attackable by player persistent
	void SetAttackableByPlayerPersistent( Bool flag ) { m_isAttackableByPlayer = flag; }

	// Set attackable by player runtime
	void SetAttackableByPlayerRuntime( Bool flag, Float timeout );

	//! Is actor controlled externally
	RED_INLINE Bool IsExternalyControlled() const { return m_externalControl; }

	//! Set external control
	void SetExternalControl( Bool flag );

	//! Is this actor a main character?
	RED_INLINE EPathEngineAgentType GetActorGroups() const { return m_actorGroups; }

	//! Get visual debug (can be NULL)
	virtual CVisualDebug* GetVisualDebug() const { return m_visualDebug; }

	//! Get Behavior Tree machine
	CBehTreeMachine* GetBehTreeMachine() const;

	//! Get current action
	RED_INLINE const ActorAction* GetAction() const { return m_action; }

	RED_INLINE ActorActionMoveTo* GetActionMoveTo() { return &m_actionMoveTo; }
	RED_INLINE ActorActionCustomSteer* GetActionCustomSteer() { return &m_actionCustomSteer; }

	//! Get type of current action
	RED_INLINE const EActorActionType GetActionType() const { return m_action ? m_action->GetType() : ActorAction_None; }

	//! Get moving agent radius
	Float GetRadius() const;

	//! Is actor moving
	Bool IsMoving() const;

	//! Get actor's move destination
	const Vector& GetMoveDestination() const;

	//! Get actor current position or move destination if moving
	Vector GetPositionOrMoveDestination() const;

	//! Get move type modification
	RED_INLINE Float GetAimOffset() const { return m_aimOffset; }

	//! Set move type modification
	RED_INLINE void SetAimOffset( Float value ) { m_aimOffset = value; }

	//! Get aim aim position
	virtual Vector GetAimPosition() const { return GetTorsoPosition() + Vector3( 0, 0, m_aimOffset ); }

	//! Get bar position with hardcoded offset
	virtual Vector GetBarPosition() const;

	// Gets actor bounds (used for example in GameplayStorage)
	virtual void GetStorageBounds( Box& box ) const override;

	//! Is actor in combat (for player checks combat mode, for npc checks current goal) // NOTICE: R4 specyfic solution!
	virtual Bool IsInCombat() const;

	// Called when actor combat state got changed // NOTICE: R4 specyfic code!
	void OnCombatModeSet( Bool b );

	//! Get position for LOS test
	Vector GetLOSTestPosition() const;

	//! Get position of torso
	Vector GetTorsoPosition() const;

	virtual Int32 GetSceneHeadBone() const;

	//! Get attack range
	const CAIAttackRange* GetAttackRange( const CName& attackRangeName = CName::NONE ) const;

	CActor* GetLastAttackTarget() const { return m_attackTarget.Get(); }
	EngineTime GetLastTimeAttackTarget() const { return m_attackTargetSetTime; }

	//! Is the given actor a danger to us
	const Bool IsDangerous( CActor* actor );

	//! Get attitude towards actor
	EAIAttitude	GetAttitude( CActor* actor );

	//! Set attitude towards actor
	void SetAttitude( CActor* actor, EAIAttitude attitude );

	//! Reset attitude towards actor (to global/group attitude)
	void ResetAttitude( CActor* actor );

	//! Clear attitudes
	void ClearAttitudes( Bool hostile, Bool neutral, Bool friendly );

	//! Get attitude map for current actor, returns true if map is nonempty
	//! This is quite expensive so use it only for debug purposes
	Bool GetAttitudeMap( TActorAttitudeMap& attitudeMap );

	//! Get current attitude group
	CName GetAttitudeGroup() const { return m_currentAttitudeGroup; }

	//! Get base attitude group
	CName GetBaseAttitudeGroup() const { return m_baseAttitudeGroup; }

	//! Set base attitude group
	void SetBaseAttitudeGroup( CName groupName );

	//! Reset attitude group to the original, profile based group
	void ResetBaseAttitudeGroup( Bool force );

	//! Set temporary attitude group
	void SetTemporaryAttitudeGroup( CName groupName, Int32 priority );

	//! Reset temporary attitude group
	void ResetTemporaryAttitudeGroup( Int32 priority );

	//! Test if we can safely pool actor
	Bool OnPoolRequest() override;

	//! Is Master entity
	virtual Bool IsMasterEntity() const override { return true; }

protected:
	
	//! Update current attitude group with respect to base group and priorities
	void UpdateCurrentAttitudeGroup();

	//! Gets attitude group from AI profile and remembers it for optimization
	void LoadBaseAttitudeGroup();

public:
	Bool IsConsciousAtWork();
	Int32 GetCurrentJTType();
	Bool IsSittingAtWork();
	Bool IsAtWork( Bool orWasJustWorking = false );
	Bool CanUseChatInCurrentAP();
	Bool IsPlayingChatScene();

	//! Set actor to ignore temporary denied areas (scene zone, fistfight, etc.)
	virtual void SetCrossSafeZoneEnabled( Bool flag );

	//! Check if actor is in ragdoll obstacle state
	Bool IsRagdollObstacle() const;

	// ------------------------------------------------------------------------
	// IAIDebugListener support
	// ------------------------------------------------------------------------
	//! Attaches a new AI debug listener
	void AttachAIDebugListener( IAIDebugListener& listener );

	//! Attaches a new AI debug listener
	void DetachAIDebugListener( IAIDebugListener& listener );

	//! Sends a notification about a one-time ai event
	void OnAIEvent( EAIEventType type, EAIEventResult result, const String& name, const String& description );

	//! Sends a notification about AI event start - returns a handle to the event which we need
	//! to specify when sending the 'event end' notification
	void OnAIEventStart( EAIEventType type, const String& name, const String& description );

	//! Sends a notification about AI event end
	void OnAIEventEnd( EAIEventType type, EAIEventResult result );

	//! Get AI combat target
	CActor* GetTarget() const;

	//! Get current combat target
	virtual CActor* GetScriptTarget() const;

	//! Force target update in next tick
	void ForceTargetUpdate();

	//! Signals event that is directly passed to AI system with a CName parameter
	void SignalGameplayEvent( CName name, CName param ) const;

	//! Signals event that is directly passed to AI system with an int parameter
	void SignalGameplayEvent( CName name, Int32 param ) const;

	//! Signals event that is directly passed to AI system with a float parameter
	void SignalGameplayEvent( CName name, Float param ) const;

	//! Signals event that is directly passed to AI system with a object pointer as parameter
	void SignalGameplayEvent( CName name, CObject* param ) const;

	//! Signals event that is directly passed to AI system with a CName parameter
	CName SignalGameplayEventReturnCName( CName name, CName defaultVal ) const;

	//! Signals event that is directly passed to AI system with an int parameter
	Int32 SignalGameplayEventReturnInt( CName name, Int32 defaultVal ) const;

	//! Signals event that is directly passed to AI system with a float parameter
	Float SignalGameplayEventReturnFloat( CName name, Float defaultVal ) const;

	//! Signals event that is directly passed to AI system
	void SignalGameplayEvent( CName name, void* additionalData = NULL, IRTTIType* additionalDataType = NULL ) const;

	//! Signals damage event that is directly passed to AI system
	void SignalGameplayDamageEvent( CName name, CBaseDamage* additionalData );

	//! Signals anim event that is directly passed to AI system
	void SignalGameplayAnimEvent( CName name, CName additionalData );

	//! Forced behaviors
	Bool ForceAIBehavior( IAITree* aiTree, Int8 priority, Int16* forcedActionId = NULL, CName forceEventName = SForcedBehaviorEventData::GetEventName(), IGameDataStorage* aiState = nullptr, EAIForcedBehaviorInterruptionLevel interruptionLevel = EAIForcedBehaviorInterruptionLevel::Low );
	Bool CancelAIBehavior( Int16 forcedActionId, CName cancelEventName = SForcedBehaviorEventData::GetCancelEventName(), Bool fireEventAndForget = true );

	//! Dynamic behaviors
	Bool ForceDynamicBehavior( IAITree* tree, CName forceEventName, Bool interrupt );
	Bool CancelDynamicBehavior( CName forceEventName, Bool interrupt );

	// Enable/Disable actor's hi resolution shadows
	void SetHiResShadows( Bool flag );
#ifndef NO_EDITOR
	Bool SetAndCheckHiResShadows( Bool flag );
#endif
	Bool UseHiResShadows() const;

	// Enable/Disable shadows
	void SetGroupShadows( Bool flag );

	Bool IsVisibleFromFar() const;
	void SetIsVisibleFromFar( Bool state );

public:
	CActor();
	virtual ~CActor();

	// Actor was just instanced from template (might be on loading thread)
	virtual void OnCreatedAsync( const EntitySpawnInfo& info ) override;

	// Actor is being restored from entity pool manager
	virtual void OnRestoredFromPoolAsync( const EntitySpawnInfo& info ) override;

	// Actor was reattached from pool
	virtual void OnRestoredFromPool( CLayer* layer, const EntitySpawnInfo& info );

	virtual void OnInitialized() override;

	// Entity was uninitialized
	virtual void OnUninitialized();

	// Entity was attached to world
	virtual void OnAttached( CWorld* world );

	// All components of entity has been attached
	virtual void OnAttachFinished( CWorld* world );

	// Entity was detached from world
	virtual void OnDetached( CWorld* world );

	//! Serialize(GC)
	virtual void OnSerialize( IFile& file );

	// Tick occurred
	virtual void OnTick( Float timeDelta );

	// Property is changing
	virtual void OnPropertyPreChange( IProperty* property );

	// Property has changed
	virtual void OnPropertyPostChange( IProperty* property );

	// Cache LOS test bone index
	void CacheBoneIndices();

	// Attaches an entity to a specified bone
	Bool AttachEntityToBone( CEntity* entity, CName boneName );

	// Detaches entity from the actor's skeleton
	void DetachEntityFromSkeleton( CEntity* entity );

	// Collect animation synchronization tokens
	virtual void OnCollectAnimationSyncTokens( CName animationName, TDynArray< CAnimationSyncToken* >& tokens ) const;

	static EActorAnimState MapItemDefToAnimState( const SItemDefinition* itemDef );

	// Get moving agent component or first root animated component if not found
	virtual CAnimatedComponent* GetRootAnimatedComponent() const;

	// Get actor's moving agent component
	CMovingAgentComponent* GetMovingAgentComponent() const;

	// Find actor's moving agent component, slow, no error reported
	virtual CMovingAgentComponent* FindMovingAgentComponent() const;

	// Get actor's morphed mesh manager component
	CMorphedMeshManagerComponent* GetMorphedMeshManagerComponent() const;

	// Find actor's morphed mesh manager component, slow, no error reported
	CMorphedMeshManagerComponent* FindMorphedMeshManagerComponent() const;

	// null the cached reference to the agent component when it's destroyed
	RED_INLINE void OnMovingAgentComponentChanged() { m_agentComponent = NULL; }

	//! Get the agent
	CMovingAgentComponent* GetAgent() const;

	// Process behavior output
	virtual void OnProcessBehaviorPose( const CAnimatedComponent* poseOwner, const SBehaviorGraphOutput& pose );

	//! Check if actor is rotated towards given entity
	Bool IsRotatedTowards( const CNode* node, Float maxAngle = 10.0f ) const;

	//! Check if actor is rotated towards given point
	Bool IsRotatedTowards( const Vector& point , Float maxAngle = 10.0f ) const;

	//! Kill actor
	void Kill( Bool forced = false );

	//! Stun actor
	void Stun( Bool forced = false );

	//! Handle event from anim track
	void HandleAnimTrackEvent( const CName& name, const CAnimationEventFired &animEvent );

	void  SetAnimationTimeMultiplier( Float mult );
	Float GetAnimationTimeMultiplier() const;

	//! Get animation combat slots positions
	Bool GetAnimCombatSlots( const CName& animSlotName, TDynArray< Matrix >& slots, Uint32 slotsNum,
		const Matrix& mainEnemyMatrix ) const;

	//! Clear rotation target
	void ClearRotationTarget() const;

	//! Set rotation target
	void SetRotationTarget( const Vector& position, Bool clamping ) const;

	//! Set rotation target
	void SetRotationTarget( const THandle< CNode >& node, Bool clamping ) const;

	//! Is in attack range
	Bool InAttackRange( const CGameplayEntity* entity, const CName& rangeName = CName::NONE ) const;

	//! Generate debug fragments
	virtual void GenerateDebugFragments( CRenderFrame* frame );
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags ) override;

	void GenerateAIDebugInfo( CRenderFrame* frame, Uint32& lineCounter );
	void GenerateAITicketInfo( CRenderFrame* frame, Uint32& lineCounter );

	//! Get actor display name
	virtual String GetDisplayName() const;

	//! Get entity group used by this actor
	virtual IRenderEntityGroup* GetRenderEntityGroup() const { return m_entityGroup; }

	//! Changes current exploration state of an actor
	Bool SetMovementType( EExplorationState state );

	//! Returns actor's exploration state
	RED_INLINE EExplorationState GetMovementType() const { return m_movementType; }

	//! Test if a given actor is an attacker
	Bool IsAttackedBy( CActor* attacker ) const;

	//! Register or unregister an attacker
	Bool RegisterAttacker( CActor* attacker, Bool registration );

	//! Retrieve the number of attackers
	Uint32 GetAttackerCount() const { return m_attackers.Size(); };

	//! Retrieve an attacker with the index provided
	CActor* GetAttacker(Uint32 index) const;

	Bool RegisterTerminationListener( Bool registerListener, IActorTerminationListener* listener );
	void ReportTerminationDeath();
	void ReportTerminationDetach();
	void ReportTerminationPrePooling();

	template < class T > T * GetAiStorageData( CName aiStorageName );
	template < class T > T * GetScriptAiStorageData( CName aiStorageName );

	//! Disable motion if physical representation request cannot be fulfilled due to unstreamed physics
	RED_INLINE void FreezeOnFailedPhysicalRequest( Bool val ) { m_freezeOnFailedPhysicalRequest = val; }
	RED_INLINE Bool IsFreezedOnFailedPhysicalRequest() const { return m_freezeOnFailedPhysicalRequest; }

public:

	//Items
	void EmptyHands( Bool drop = false );
	void EmptyHand( CName itemSlot, Bool drop = false );
	Bool DrawItem( SItemUniqueId itemId, Bool instant );
	Bool HolsterItem( SItemUniqueId itemId, Bool instant );
	Bool DrawWeaponAndAttack( SItemUniqueId itemId );
	Bool UseItem( SItemUniqueId itemId, Bool destroyItem = false );
	Bool HasLatentItemAction() const;

	virtual void OnGrabItem( SItemUniqueId itemId, CName slot ) override;
	virtual void OnPutItem( SItemUniqueId itemId, Bool emptyHand ) override;
	virtual void OnMountItem( SItemUniqueId itemId, Bool wasHeld ) override;
	virtual void OnUnmountItem( SItemUniqueId itemId ) override;
	virtual void OnEnhanceItem( SItemUniqueId enhancedItemId, Int32 slotIndex ) override;
	virtual void OnRemoveEnhancementItem( SItemUniqueId enhancedItemId, Int32 slotIndex ) override;

	//! Item was added to inventory
	virtual void OnAddedItem( SItemUniqueId itemId ) override;
	//! Item was removed from inventory
	virtual void OnRemovedItem( SItemUniqueId itemId ) override;

	//! Item ability was added
	virtual void OnItemAbilityAdded( SItemUniqueId itemId, CName ability ) override;
	//! Item ability was removed
	virtual void OnItemAbilityRemoved( SItemUniqueId itemId, CName ability ) override;



	struct IssueRequiredItemsInfo
	{
		IssueRequiredItemsInfo() : m_success( true ), m_usePriorityForSceneItems( false )
		{}

		//Items ( mostly weapons ) without tag SecondaryWeapon will take priority when looking for weapons by category
		Bool						m_usePriorityForSceneItems;

		TDynArray< SItemUniqueId >	m_spawnedItems;
		Bool						m_success;
	};

	//! Issue required item info
	void IssueRequiredItems( const SActorRequiredItems& info, Bool instant = false, IssueRequiredItemsInfo* res = nullptr );
	void SetRequiredItems( const SActorRequiredItems& info );
	void StoreRequiredItems( SActorRequiredItems& itemIds );


	//! draws/holsters items according to required items state
	void ProcessRequiredItemsState( Bool instant = false, IssueRequiredItemsInfo* res = nullptr );
	void ProcessRequiredItem( CName required, CName slot, Bool instant = false, IssueRequiredItemsInfo* res = nullptr );

public:
	// Teleport entity to new location
	virtual Bool Teleport( const Vector& position, const EulerAngles& rotation );

	// Teleport to node
	virtual Bool Teleport( CNode* node, Bool applyRotation = true, const Vector& offset = Vector::ZEROS );

	// Set local component position
	virtual void SetPosition( const Vector& position );

	// Update transform data (called from multiple threads!!!)
	virtual void OnUpdateTransformEntity() override;

	// For new event system
	virtual void ProcessAnimationEvent( const CAnimationEventFired* event );

public:
	//! Play effect on this entity
	virtual Bool PlayEffect( const CName& effectName, const CName& boneName = CName::NONE, const CNode* targetNode = NULL );

	//! Play animation effect on this entity
	virtual Bool PlayEffectForAnimation( CName animationName, Float startTime=0.0f );
	using CEntity::PlayEffectForAnimation;

	//! Apply appearance with given name on entity
	virtual void ApplyAppearance( const CName &appearanceName );

public:
	virtual IPhysicalCollisionTriggerCallback* QueryPhysicalCollisionTriggerCallback() { return this; }

	//! For debug - Is actor working
	Bool IsWorking() const;

	//! For debug - Is in quest scene
	Bool IsInQuestScene() const;

	void SetHideInGame( Bool hideInGame, Bool immediate = false, EHideReason hideReason = HR_Default ) override;
	Uint32 GetHideReason() const override { return ( Uint32 ) m_hideReason; }
	RED_FORCE_INLINE Bool IsHiddentInGame( EHideReason hideReason ) const { return ( m_hideReason & hideReason ) != 0; }

// ------------------------------------------------------------------------
// SCENES support
// ------------------------------------------------------------------------
//++ All functionality for scenes etc. This code needs to be heavily refactored

public: // Play mimic animation
	virtual Bool PlayMimicAnimation( const CName& animation, const CName& slot, Float blendTime = 0.0f, Float offset = 0.0f ) override;
	virtual Bool StopMimicAnimation( const CName& slot ) override;
	virtual Bool HasMimicAnimation( const CName& slot ) const override;

	virtual Bool PlayLipsyncAnimation( CSkeletalAnimationSetEntry* anim, Float offset = 0.0f ) override;
	virtual Bool StopLipsyncAnimation() override;

public: // Actor speech
	TActorSpeechID ProceedSpeechFromQueue( );
	TActorSpeechID SpeakLine( const ActorSpeechData& speechData );

	void CancelSpeech( Bool cleanupQueue = true );

	Bool IsSpeaking( Uint32 stringId = 0 );

	Bool UpdateSpeaking( const TActorSpeechID& speechId, Float time, Float progress );
	void ReplayCurrentSpeechLipsync();

	const CActorSpeech* GetSpeech() const;

public: // ISceneActorInterface
	virtual Bool HasSceneMimic() const;
	virtual Bool SceneMimicOn();
	virtual void SceneMimicOff();

	virtual CEntity* GetSceneParentEntity();

	virtual Vector GetSceneHeadPosition() const;

	virtual Bool WasSceneActorVisibleLastFrame() const;

	virtual CName GetSceneActorVoiceTag() const;

public: // IActorInterface
	//! Get the actor interface
	virtual IActorInterface* QueryActorInterface();

	//! Get the actor interface
	virtual const IActorInterface* QueryActorInterface() const;

	//! Turn on/off mimic high
	virtual Bool MimicOn();
	virtual void MimicOff();

	//! Has actor high mimic
	virtual Bool HasMimic() const;

	//! Set mimic variable
	virtual Bool SetMimicVariable( const CName varName, Float value );

	//! Is actor in non gameplay scene
	virtual Bool IsInNonGameplayScene() const;

	Bool IsInGameplayScene() const { return m_isInGameplayScene; }

	//! Actor's animation state
	virtual Int32 GetActorAnimState() const;

	//! Get position of head
	virtual Vector GetHeadPosition() const;

	//! Get head bone
	virtual Int32 GetHeadBone() const;

	//! Get mimic component
	virtual CMimicComponent* GetMimicComponent() const;

	//! Is LookAt enabled
	virtual Bool IsLookAtEnabled() const;

	//! Get look at level
	virtual ELookAtLevel GetLookAtLevel() const;

	//! Get look at target in world space
	virtual Vector GetLookAtTarget() const;

	//! Get look at body parts weights
	virtual Vector GetLookAtBodyPartsWeights() const;

	//! Get look at data: speed damp, speed follow, autoLimit
	virtual Vector GetLookAtCompressedData() const;

	//! Get eyes look at data: convergence weight, is additive
	virtual Vector GetEyesLookAtCompressedData() const;

public: // Look at
	//! Enable look at
	Bool EnableLookAt( const SLookAtInfo& lookAtInfo );

	//! Simplified version of lookat code
	Bool EnableDynamicLookAt( CNode* node, Float duration );

	//! Disables all LookAts
	void DisableLookAts();
	
	//! Disable dialogs lookat
	void DisableDialogsLookAts( Float speed );
	void RemoveAllNonDialogsLookAts();

	virtual void ActivateLookatFilter( CName key, Bool value );
	void SetLookatFilterData( ELookAtLevel level, CName key );
	void RemoveLookatFilterData( CName key );

	//! Set look at level
	void SetLookAtLevel( ELookAtLevel level );

	//! Get actor look ats description
	void GetLookAtDesc( CActorLookAtDesc& desc ) const;

	//! Get look at params
	void GetLookAtParams( const CLookAtDynamicParam*& dynamicParam, const CLookAtStaticParam*& staticParam, CLookAtContextParam& contextParam ) const;

	//! Set look at mode. You have to call ResetLookAtMode
	void SetLookAtMode( ELookAtMode mode );

	//! Reset look at mode
	void ResetLookAtMode( ELookAtMode mode );

	//! Script callback when required items processing is finished
	void OnProcessRequiredItemsFinish();

private:
	//! Updates the look ats, disabling them if necessary
	void UpdateLookAt( Float timeElapsed );

public:
	//! Set mimic component
	virtual void SetMimicComponent( CMimicComponent* comp );

	CStorySceneComponent* GetCurrentStorySceneComponent();

	// Play voiceset scene
	virtual Bool PlayVoiceset( EArbitratorPriorities priority, const String& voiceset, Bool breakCurrentSpeach = false );
	void StopAllVoicesets( Bool cleanupQueue );
	EAsyncCheckResult HasVoiceset( const String& voiceset );

public: // Scene controls (?)
	Bool IsLockedByScene() const;

	void SetSceneLock( Bool enable, Bool isGameplayScene , Int8 priority = 100);

	void AddStoryScene( CStorySceneController * scene );
	void RemoveStoryScene( CStorySceneController * scene );

	void StopAllScenes( const SceneCompletionReason& reason );

	const TDynArray< CStorySceneController* >& GetStoryScenes() const;

	virtual void OnCutsceneStarted() override;
	virtual void OnCutsceneEnded() override;

	void NotifyScenesAboutChangingApperance();

// ------------------------------------------------------------------------
// SCENES support end
// -----------------------------------------------------------------------

public:
	virtual Bool WasInventoryVisibleLastFrame() const override;

public:
	//! Is actor ready for new action
	RED_INLINE Bool IsReadyForNewAction() const { return m_action == NULL; }

	RED_INLINE EActorActionType GetCurrentActionType() const { return m_latentActionType; }
	RED_INLINE EActorActionResult GetCurrentActionStatus() const { return m_latentActionResult; }

	virtual Int32 GetCurrentActionPriority() const { return 0; }

	//! Cancel any actions in progress
	void ActionCancelAll();

	//! Start low-level actor action of moving to target
	Bool ActionMoveTo( const CNode * target, Bool withHeading, EMoveType moveType = MT_Walk, Float absSpeed = 1.0f, Float radius = 0.0f, EMoveFailureAction failureAction = MFA_REPLAN, Uint16 actionFlags = 0, Float tolerance = 0.f );

	//! Start low-level actor action of moving to target
	Bool ActionMoveTo( const Vector& target, EMoveType moveType = MT_Walk, Float absSpeed = 1.0f, Float radius=0.0f, EMoveFailureAction failureAction = MFA_REPLAN, Uint16 actionFlags = 0, Float tolerance = 0.f );
	Bool ActionMoveToChangeTarget( const Vector& target, EMoveType moveType = MT_Walk, Float absSpeed = 1.0f, Float radius=0.0f );

	//! Start low-level actor action of moving to target, ensures valid rotation at the end
	Bool ActionMoveTo( const Vector& target, Float heading, EMoveType moveType = MT_Walk, Float absSpeed = 1.0f, Float radius=0.0f, EMoveFailureAction failureAction = MFA_REPLAN, Uint16 actionFlags = 0, Float tolerance = 0.f );

	//! Makes an actor use the specified action area
	Bool ActionSlideThrough( CActionAreaComponent* actionArea );

	//! Traverse through exploration
	Bool ActionTraverseExploration( const SExplorationQueryToken & token, const THandle< IScriptable >& listener = THandle< IScriptable >::Null(), CNode *const steeringGraphTargetNode = nullptr );

	//! Start low-level actor action of sliding to target in duration
	Bool ActionSlideTo( const Vector& target, Float duration );

	//! Start low-level actor action of sliding to target in duration
	Bool ActionSlideTo( const Vector& target, Float heading, Float duration, ESlideRotation rotation );

	//! Start low-level actor action of moving on curve to target in duration
	Bool ActionMoveOnCurveTo( const Vector& target, Float duration, Bool rightShift  );

	//! Start low-level actor action of escaping from given position
	Bool ActionMoveAwayFrom( const CNode * position, Float distance, EMoveType moveType = MT_Walk, Float absSpeed = 1.0f, Float radius=2.0f, EMoveFailureAction failureAction = MFA_REPLAN );

	//! Start low-level actor action of escaping from given line segment
	Bool ActionMoveAwayFromLine( const Vector& positionA, const Vector& positionB, Float distance, Bool makeMinimalMovement,
								 EMoveType moveType = MT_Walk, Float absSpeed = 1.0f, Float radius=2.0f, EMoveFailureAction failureAction = MFA_REPLAN );

	//! Start low-level actor action of moving in a custom way
	Bool ActionMoveCustom( IMovementTargeter* targeter );

	//! Start low-level actor action of moving to another node that is capable of movement on its own
	Bool ActionMoveToDynamicNode( const CNode* target, EMoveType moveType  = MT_Walk, Float absSpeed  = 1.0f, Float radius = 0.0f, Bool keepDistance = false, EMoveFailureAction failureAction = MFA_REPLAN, Float tolerance = 0.f );

	//! Start low-level actor action of moving to another node while not even being on navmesh
	Bool ActionMoveOutsideNavdata( const CNode* target, EMoveType moveType  = MT_Walk, Float absSpeed  = 1.f, Float radius = 0.f );

	//! Start low-level actor action of strafing around a target
	Bool ActionCustomSteer( IMovementTargeter* targeter, EMoveType moveType  = MT_Walk, Float absSpeed  = 1.0f, EMoveFailureAction failureAction = MFA_REPLAN);

	//! Start low-level actor action of rotating towards given target
	Bool ActionRotateTo( const Vector& target, Bool endOnRotationFinished = true, Float angleTolerance = 3.f );

	//! Update actor action of rotating towards given target
	Bool ActionRotateTo_Update( const Vector& target, Bool endOnRotationFinished );

	//! Start low-level actor action that will make the actor match the specified orientation
	Bool ActionSetOrientation( Float orientation, Bool endOnRotationFinished = true, Float angleTolerance = 3.f );

	//! Update actor action of matching the specified orientation
	Bool ActionSetOrientation_Update( Float orientation, Bool endOnRotationFinished );

	//! Start low-level actor action of playing animation on slot
	Bool ActionPlaySlotAnimation( const CName& slotName, const CName& animationName, Float blendIn = 0.2f, Float blendOut = 0.2f, Bool continuePlaying = false );

	Bool ActionStartWorking( CJobTree* jobTree, CName category, Bool skipEntryAnimations );

	//! Start low-level actor action of stopping working action :)
	Bool ActionExitWorking( Bool fast = false );

	//! Start low-level actor action of raising behavior event
	Bool ActionRaiseEvent( const CName& eventName, const CName& notificationName, EBehaviorGraphNotificationType notificationType, Float timeout );

	//! Start low-level actor action of raising force behavior event
	Bool ActionRaiseForceEvent( const CName& eventName, const CName& notificationName, EBehaviorGraphNotificationType notificationType, Float timeout );

	// Start low-level slide action with animation
	Bool ActionAnimatedSlideTo( const SAnimatedSlideSettings& settings, const SAnimSliderTarget& target );
	Bool ActionAnimatedSlideTo( const SAnimatedSlideSettings& settings, const SAnimSliderTarget& target, THandle< CActionMoveAnimationProxy >& proxy );

	// Start low-level action match to
	Bool ActionMatchTo( const SActionMatchToSettings& settings, const SActionMatchToTarget& target );
	Bool ActionMatchTo( const SActionMatchToSettings& settings, const SActionMatchToTarget& target, THandle< CActionMoveAnimationProxy >& proxy );

	//! Action has finished
	void ActionEnded( EActorActionType actionType, EActorActionResult result );

	//! Called when action started
	virtual void OnActionStarted( EActorActionType actionType );

	//! Called when action ended
	virtual void OnActionEnded( EActorActionType actionType, Bool result );

	//! Can start action from script
	virtual Bool CanPerformActionFromScript( CScriptStackFrame& stack ) const;

	//! Access to the action work FOR DEBUG PURPOSES ONLY
	const ActorActionWork& GetActionWork() const { return m_actionWork; }

	//! Retrieves and stores sound parameters from template
	void CacheSoundParams( Bool force = false );

	void MuteHeadAudio( Bool mute );

	void PredictWorldPosition( Float time, Vector& outPosition ) const;

	CAppearanceComponent* GetAppearanceComponent() const;

	CName GetAppearance() const;

	virtual void OnAppearanceChanged( const CEntityAppearance& appearance );

	Vector GetNearestPointInPersonalSpaceAt(const Vector& myPosition, const Vector& otherPosition) const;

	RED_INLINE void SetEncounterGroupUsedToSpawn( Int32 group ){ m_encounterGroupUsedToSpawn = group; }
	RED_INLINE Int32  GetEncounterGroupUsedToSpawn( ){ return m_encounterGroupUsedToSpawn; }

protected:
	//! Update actions
	void UpdateActions( Float timeDelta );

	//! On draw weapon, used in player
	virtual void OnDrawWeapon() {}

	//! Set actor anim state ( not public! )
	void SetActorAnimState( EActorAnimState state );

	//! Check if actor can be 'stolen' by other actor
	virtual Bool CanStealOtherActor( const CActor* const other ) const;

private:
	Bool ActivateMovementType( EExplorationState state );
	void DeactivateMovementType( EExplorationState state );

	Bool HasInteractionVoicesetScene() const;

	virtual void onCollision( const SPhysicalCollisionInfo& info );

	static void SetupAIParametersList( const EntitySpawnInfo& info, SAIParametersSpawnList& outList );
	virtual void ResetClothAndDangleSimulation() override;

protected:
	void OnAttachmentCreated() override;
	void OnAttachmentBroken() override;

	void NotifyEncounterAboutAttach();
	void NotifyEncounterAboutDetach();

public:

#ifndef NO_ERROR_STATE
	virtual void SetErrorState( const String &description ) const;
	void SetErrorState( const Char* description ) const;
	Bool IsInErrorState() const;
	virtual const String &GetErrorState() const;
#endif


//Delete when no longer necessary
#ifndef NO_EDITOR
	const CAIAttackRange *	m_debugAttackRange;
	Bool					m_debugEnableTraceDraw;
#endif
	void funcSetDebugAttackRange( CScriptStackFrame& stack, void* result );
	void funcEnableDebugARTraceDraw( CScriptStackFrame& stack, void* result );

protected:
	// ----------------------------------------------------------------------------
	// Scripting support
	// ----------------------------------------------------------------------------
	void funcSignalGameplayEvent( CScriptStackFrame& stack, void* result );
	void funcSignalGameplayEventParamCName( CScriptStackFrame& stack, void* result );
	void funcSignalGameplayEventParamInt( CScriptStackFrame& stack, void* result );
	void funcSignalGameplayEventParamFloat( CScriptStackFrame& stack, void* result );
	void funcSignalGameplayEventParamObject( CScriptStackFrame& stack, void* result );
	void funcSignalGameplayEventReturnCName( CScriptStackFrame& stack, void* result );
	void funcSignalGameplayEventReturnInt( CScriptStackFrame& stack, void* result );
	void funcSignalGameplayEventReturnFloat( CScriptStackFrame& stack, void* result );
	void funcSignalGameplayDamageEvent( CScriptStackFrame& stack, void* result );
	void funcSignalGameplayAnimEvent( CScriptStackFrame& stack, void* result );
	void funcForceAIUpdate( CScriptStackFrame& stack, void* result );
	void funcGetRadius( CScriptStackFrame& stack, void* result );
	void funcPlayVoiceset( CScriptStackFrame& frame, void* result );
	void funcStopAllVoicesets( CScriptStackFrame& frame, void* result );
	void funcHasVoiceset( CScriptStackFrame& frame, void* result );
	void funcIsRotatedTowards( CScriptStackFrame& stack, void* result );
	void funcIsRotatedTowardsPoint( CScriptStackFrame& stack, void* result );
	void funcGetAliveFlag( CScriptStackFrame& stack, void* result );
	void funcSetAlive( CScriptStackFrame& stack, void* result );
	void funcIsExternalyControlled( CScriptStackFrame& stack, void* result );
	void funcIsInCombat( CScriptStackFrame& stack, void* result );
	void funcIsMoving( CScriptStackFrame& stack, void* result );
	void funcGetMoveDestination( CScriptStackFrame& stack, void* result );
	void funcGetPositionOrMoveDestination( CScriptStackFrame& stack, void* result );
	void funcGetVisualDebug( CScriptStackFrame& stack, void* result );
	void funcGetVoicetag( CScriptStackFrame& stack, void* result );
	void funcGetSkeletonType( CScriptStackFrame& stack, void* result );

	void funcHasLatentItemAction( CScriptStackFrame& stack, void* result );
	void funcWaitForFinishedAllLatentItemActions( CScriptStackFrame& stack, void* result );
	void funcUseItem( CScriptStackFrame& stack, void* result );

	void funcGetVisibility( CScriptStackFrame& stack, void* result );
	void funcSetVisibility( CScriptStackFrame& stack, void* result ); 
	void funcSetAppearance( CScriptStackFrame& stack, void* result );
	void funcGetAppearance( CScriptStackFrame& stack, void* result );
	void funcGetHeadAngleHorizontal( CScriptStackFrame& stack, void* result );
	void funcGetHeadAngleVertical( CScriptStackFrame& stack, void* result );
	//void funcCheckInteraction( CScriptStackFrame& stack, void* result );
	//void funcCheckInteractionPlayerOnly( CScriptStackFrame& stack, void* result );
	void funcGetMovingAgentComponent( CScriptStackFrame& stack, void* result );
	void funcGetMorphedMeshManagerComponent( CScriptStackFrame& stack, void* result );
	void funcEnablePathEngineAgent( CScriptStackFrame& stack, void* result );
	void funcIsRagdollObstacle( CScriptStackFrame& stack, void* result );
	void funcClearRotationTarget( CScriptStackFrame& stack, void* result );
	void funcSetRotationTarget( CScriptStackFrame& stack, void* result );
	void funcSetRotationTargetPos( CScriptStackFrame& stack, void* result );
	void funcEnableCollisionInfoReportingForItem( CScriptStackFrame& stack, void* result );
	void funcEnablePhysicalMovement( CScriptStackFrame& stack, void* result );
	void funcEnableCharacterCollisions( CScriptStackFrame& stack, void* result );
	void funcEnableStaticCollisions( CScriptStackFrame& stack, void* result );
	void funcEnableDynamicCollisions( CScriptStackFrame& stack, void* result );
	void funcIsCharacterCollisionsEnabled( CScriptStackFrame& stack, void* result );
	void funcIsStaticCollisionsEnabled( CScriptStackFrame& stack, void* result );
	void funcIsDynamicCollisionsEnabled( CScriptStackFrame& stack, void* result );
	void funcPlayLine( CScriptStackFrame& stack, void* result );
	void funcPlayLineByStringKey( CScriptStackFrame& stack, void* result );
	void funcEndLine( CScriptStackFrame& stack, void* result );
	void funcIsSpeaking( CScriptStackFrame& stack, void* result );
	void funcEnableDynamicLookAt( CScriptStackFrame& stack, void* result );
	void funcEnableStaticLookAt( CScriptStackFrame& stack, void* result );
	void funcDisableLookAt( CScriptStackFrame& stack, void* result );
	void funcCutBodyPart( CScriptStackFrame& stack, void* result );
	void funcIsAttackableByPlayer( CScriptStackFrame& stack, void* result );
	void funcSetAttackableByPlayerPersistent( CScriptStackFrame& stack, void* result );
	void funcSetAttackableByPlayerRuntime( CScriptStackFrame& stack, void* result );
	void funcInAttackRange( CScriptStackFrame& stack, void* result );
	void funcGetNearestPointInPersonalSpace( CScriptStackFrame& stack, void* result );
	void funcGetNearestPointInPersonalSpaceAt( CScriptStackFrame& stack, void* result );
	void funcGatherEntitiesInAttackRange( CScriptStackFrame& stack, void* result );
	void funcCalculateHeight( CScriptStackFrame& stack, void* result );
	void funcGetAnimationTimeMultiplier( CScriptStackFrame& stack, void* result );
	void funcSetAnimationTimeMultiplier( CScriptStackFrame& stack, void* result );
	void funcIsInNonGameplayScene( CScriptStackFrame& stack, void* result );
	void funcIsInGameplayScene( CScriptStackFrame& stack, void* result );
	void funcPlayScene( CScriptStackFrame& stack, void* result );
	void funcStopAllScenes( CScriptStackFrame& stack, void* result );
	void funcEmptyHands( CScriptStackFrame& stack, void* result );
	void funcGetCurrentActionType( CScriptStackFrame& stack, void* result );
	void funcSetErrorState( CScriptStackFrame& stack, void* result );
	void funcWasVisibleLastFrame( CScriptStackFrame& stack, void* result );
	void funcPlayMimicAnimationAsync( CScriptStackFrame& stack, void* result );
	void funcCanPlayQuestScene( CScriptStackFrame& stack, void* result );
	void funcHasInteractionScene( CScriptStackFrame& stack, void* result );
	void funcCanTalk( CScriptStackFrame& stack, void* result );
	void funcGetActorAnimState( CScriptStackFrame& stack, void* result );
	void funcIsInView( CScriptStackFrame& stack, void* result );
	void funcGetFallTauntEvent( CScriptStackFrame& stack, void* result );
	void funcSetBehaviorMimicVariable( CScriptStackFrame& stack, void* result );
	void funcRaiseBehaviorMimicEvent( CScriptStackFrame& stack, void* result );
	void funcSetLookAtMode( CScriptStackFrame& stack, void* result );
	void funcResetLookAtMode( CScriptStackFrame& stack, void* result );
	void funcIsUsingExploration( CScriptStackFrame& stack, void* result );
	void funcGetAnimCombatSlots( CScriptStackFrame& stack, void* result );
	void funcSetInteractionPriority( CScriptStackFrame& stack, void* result );
	void funcSetOriginalInteractionPriority( CScriptStackFrame& stack, void* result );
	void funcRestoreOriginalInteractionPriority( CScriptStackFrame& stack, void* result );
	void funcGetOriginalInteractionPriority( CScriptStackFrame& stack, void* result );
    void funcGetInteractionPriority( CScriptStackFrame& stack, void* result );
	void funcSetUnpushableTarget( CScriptStackFrame& stack, void* result );
	void funcGetHeadBoneIndex( CScriptStackFrame& stack, void* result );
	void funcGetTorsoBoneIndex( CScriptStackFrame& stack, void* result );
	void funcGetTarget( CScriptStackFrame& stack, void* result );
	void funcIsDangerous( CScriptStackFrame& stack, void* result );
	void funcGetAttitude( CScriptStackFrame& stack, void* result );
	void funcSetAttitude( CScriptStackFrame& stack, void* result );
	void funcResetAttitude( CScriptStackFrame& stack, void* result );
	void funcHasAttitudeTowards( CScriptStackFrame& stack, void* result );
	void funcClearAttitudes( CScriptStackFrame& stack, void* result );
	void funcGetAttitudeGroup( CScriptStackFrame& stack, void* result );
	void funcGetBaseAttitudeGroup( CScriptStackFrame& stack, void* result );
	void funcSetBaseAttitudeGroup( CScriptStackFrame& stack, void* result );
	void funcResetBaseAttitudeGroup( CScriptStackFrame& stack, void* result );
	void funcSetTemporaryAttitudeGroup( CScriptStackFrame& stack, void* result );
	void funcResetTemporaryAttitudeGroup( CScriptStackFrame& stack, void* result );
	void funcSetAttitudeGroup( CScriptStackFrame& stack, void* result );
	void funcResetAttitudeGroup( CScriptStackFrame& stack, void* result );
	void funcCanStealOtherActor( CScriptStackFrame& stack, void* result );
	void funcResetClothAndDangleSimulation( CScriptStackFrame& stack, void* result );
	// ----------------------------------------------------------------------------
	// Latent scripting support
	// ----------------------------------------------------------------------------
	void funcIsReadyForNewAction( CScriptStackFrame& stack, void* result );
	void funcActionCancelAll( CScriptStackFrame& stack, void* result );
	void funcGetCurrentActionPriority( CScriptStackFrame& stack, void* result );
	void funcIsDoingSomethingMoreImportant( CScriptStackFrame& stack, void* result );
	void funcIsCurrentActionInProgress( CScriptStackFrame& stack, void* result );
	void funcIsCurrentActionSucceded( CScriptStackFrame& stack, void* result );
	void funcIsCurrentActionFailed( CScriptStackFrame& stack, void* result );
	void funcActionMoveToNode( CScriptStackFrame& stack, void* result );
	void funcActionMoveToNodeAsync( CScriptStackFrame& stack, void* result );
	void funcActionMoveToNodeWithHeading( CScriptStackFrame& stack, void* result );
	void funcActionMoveToNodeWithHeadingAsync( CScriptStackFrame& stack, void* result );
	void funcActionMoveTo( CScriptStackFrame& stack, void* result );
	void funcActionMoveToAsync( CScriptStackFrame& stack, void* result );
	void funcActionMoveToChangeTargetAsync( CScriptStackFrame& stack, void* result );	
	void funcActionMoveAwayFromNode( CScriptStackFrame& stack, void* result );
	void funcActionMoveAwayFromNodeAsync( CScriptStackFrame& stack, void* result );
	void funcActionMoveAwayFromLine( CScriptStackFrame& stack, void* result );
	void funcActionMoveAwayFromLineAsync( CScriptStackFrame& stack, void* result );
	void funcActionMoveToWithHeading( CScriptStackFrame& stack, void* result );
	void funcActionMoveToWithHeadingAsync( CScriptStackFrame& stack, void* result );
	void funcActionMoveCustom( CScriptStackFrame& stack, void* result );
	void funcActionMoveCustomAsync( CScriptStackFrame& stack, void* result );
	void funcActionMoveToDynamicNode( CScriptStackFrame& stack, void* result );
	void funcActionMoveToDynamicNodeAsync( CScriptStackFrame& stack, void* result );
	void funcActionSlideThrough( CScriptStackFrame& stack, void* result );
	void funcActionSlideThroughAsync( CScriptStackFrame& stack, void* result );
	void funcActionSlideTo( CScriptStackFrame& stack, void* result );
	void funcActionSlideToAsync( CScriptStackFrame& stack, void* result );
	void funcActionSlideToWithHeadingAsync( CScriptStackFrame& stack, void* result );
	void funcActionSlideToWithHeading( CScriptStackFrame& stack, void* result );
	void funcActionMoveOnCurveTo( CScriptStackFrame& stack, void* result );
	void funcActionMoveOnCurveToAsync( CScriptStackFrame& stack, void* result );
	void funcActionRotateTo( CScriptStackFrame& stack, void* result );
	void funcActionRotateToAsync( CScriptStackFrame& stack, void* result );
	void funcActionSetOrientation( CScriptStackFrame& stack, void* result );
	void funcActionPlaySlotAnimation( CScriptStackFrame& stack, void* result );
	void funcActionPlaySlotAnimationAsync( CScriptStackFrame& stack, void* result );
	void funcActionRaiseEvent( CScriptStackFrame& stack, void* result );
	void funcActionRaiseForceEvent( CScriptStackFrame& stack, void* result );
	void funcActionRaiseEventAsync( CScriptStackFrame& stack, void* result );
	void funcActionRaiseForceEventAsync( CScriptStackFrame& stack, void* result );
	void funcActionAnimatedSlideToStatic( CScriptStackFrame& stack, void* result );
	void funcActionAnimatedSlideToStaticAsync( CScriptStackFrame& stack, void* result );
	void funcActionAnimatedSlideToStaticAsync_P( CScriptStackFrame& stack, void* result );
	void funcActionAnimatedSlideTo( CScriptStackFrame& stack, void* result );
	void funcActionAnimatedSlideToAsync( CScriptStackFrame& stack, void* result );
	void funcActionAnimatedSlideToAsync_P( CScriptStackFrame& stack, void* result );
	void funcActionMatchTo( CScriptStackFrame& stack, void* result );
	void funcActionMatchToAsync( CScriptStackFrame& stack, void* result );
	void funcActionMatchToAsync_P( CScriptStackFrame& stack, void* result );

	void funcActionExitWork( CScriptStackFrame& stack, void* result );
	void funcActionExitWorkAsync( CScriptStackFrame& stack, void* result );
	void funcActorActionExploration( CScriptStackFrame& stack, void* result );
	void funcPushAway( CScriptStackFrame& stack, void* result );
	void funcPushInDirection( CScriptStackFrame& stack, void* result );
	void funcSetMovementType( CScriptStackFrame& stack, void* result );
	void funcGetMovementType( CScriptStackFrame& stack, void* result );

	void funcDrawItems( CScriptStackFrame& stack, void* result );
	void funcHolsterItems( CScriptStackFrame& stack, void* result );
	void funcDrawItemsLatent( CScriptStackFrame& stack, void* result );
	void funcHolsterItemsLatent( CScriptStackFrame& stack, void* result );
	void funcDrawWeaponAndAttackLatent( CScriptStackFrame& stack, void* result );
	void funcIssueRequiredItems( CScriptStackFrame& stack, void* result );
	void funcSetRequiredItems( CScriptStackFrame& stack, void* result );
	void funcIssueRequiredItemsGeneric( CScriptStackFrame& stack, void* result );
	void funcSetRequiredItemsGeneric( CScriptStackFrame& stack, void* result );
	void funcProcessRequiredItems( CScriptStackFrame& stack, void* result );

	void funcForceAIBehavior( CScriptStackFrame& stack, void* result );
	void funcCancelAIBehavior( CScriptStackFrame& stack, void* result );
	void funcSetDynamicBehavior( CScriptStackFrame& stack, void* result );
	void funcIsAttackedBy( CScriptStackFrame& stack, void* result );
	void funcRegisterAttacker( CScriptStackFrame& stack, void* result );
	void funcGetScriptStorageObject( CScriptStackFrame& stack, void* result );
	void funcGetAutoEffects( CScriptStackFrame& stack, void* result );
	void funcGetCharacterStatsParam( CScriptStackFrame& stack, void* result );
	void funcPlayPushAnimation( CScriptStackFrame& stack, void* result );
	void funcActivateAndSyncBehaviorWithItemsParallel( CScriptStackFrame& stack, void* result );
	void funcActivateAndSyncBehaviorWithItemsSequence( CScriptStackFrame& stack, void* result );
	void funcEnableCollisions( CScriptStackFrame& stack, void* result );	
	void funcPredictWorldPosition( CScriptStackFrame& stack, void* result );
	void funcApplyItemAbilities( CScriptStackFrame& stack, void* result );
	void funcRemoveItemAbilities( CScriptStackFrame& stack, void* result );
	void funcReportDeathToSpawnSystems( CScriptStackFrame& stack, void* result );
	void funcCanPush( CScriptStackFrame& stack, void* result );
	void funcMuteHeadAudio( CScriptStackFrame& stack, void* result );

	void funcSetGroupShadows( CScriptStackFrame& stack, void* result );

	void funcForceSoundAppearanceUpdate(CScriptStackFrame& stack, void* result );

public :
	/// Fetch all ai tree and ai related params
	/// Static because it is more of a utility function than an actor function
	static void GetAITemplateParams( CEntityTemplate*const entityTemplate, THandle< CAIBaseTree >& outAIBaseTree, TDynArray< CAIPresetParam* > * outAIPresetsList = nullptr );
};

BEGIN_CLASS_RTTI( CActor )
	PARENT_CLASS( CGameplayEntity )
	PROPERTY_EDIT( m_actorGroups,						TXT("Actor group to which this actor belongs") );
	PROPERTY_EDIT( m_aimOffset,							TXT("Vertical target aim offset.") );
	PROPERTY_EDIT( m_barOffset,							TXT("Vertical bar aim offset.") );
	PROPERTY_EDIT_SAVED_NOSERIALIZE( m_isAttackableByPlayer, TXT("Setting this to false will make the npc untargetable AND invulnerable") );
	PROPERTY_NOSERIALIZE( m_losTestBoneIndex );
	PROPERTY_NOSERIALIZE( m_attackTarget );
	PROPERTY_NOSERIALIZE( m_attackTargetSetTime );
	PROPERTY_EDIT( m_frontPushAnim,						TXT( "Name of the front push animation" ) );
	PROPERTY_EDIT( m_backPushAnim,						TXT( "Name of the back push animation" ) );
	PROPERTY_EDIT( m_isCollidable,						TXT( "Collides with other agents" ) );
	PROPERTY_EDIT( m_isVisibileFromFar,					TXT( "Flag overrides LOD system actor hide-by-distance mechanic" ) );
	//PROPERTY_EDIT( m_spawnHidden,						TXT( "Actor spawns in 'hidden' state" ) );
	PROPERTY_RO( m_voiceTag,							TXT( "Default voice of actor" ) );
	PROPERTY_CUSTOM_EDIT_ARRAY( m_voiceToRandomize, TXT( "" ), TXT( "AudioEventBrowser" ) )
	PROPERTY_NOSERIALIZE( m_behTreeMachine );
	PROPERTY_EDIT_NOSERIALIZE( m_useHiResShadows,		TXT("(DEBUG) Render using hi resolution shadows") );
	PROPERTY_SETTER( m_useHiResShadows, SetHiResShadows );
	PROPERTY_NOSERIALIZE( m_isInFFMiniGame );
	PROPERTY_EDIT( m_pelvisBoneName,					TXT("") );
	PROPERTY_EDIT( m_torsoBoneName,						TXT("") );
	PROPERTY_EDIT( m_headBoneName,						TXT("") );
	PROPERTY_EDIT( m_useAnimationEventFilter,			TXT( "Filters animation events" ) );
	PROPERTY_EDIT( m_soundListenerOverride, 			TXT("Add an override name to have this available as a sound listener override"));
	PROPERTY_SAVED( m_encounterGroupUsedToSpawn );

//Delete when no longer necessary
	NATIVE_FUNCTION( "SetDebugAttackRange", funcSetDebugAttackRange );
	NATIVE_FUNCTION( "EnableDebugARTraceDraw", funcEnableDebugARTraceDraw );
	NATIVE_FUNCTION( "SignalGameplayDamageEvent", funcSignalGameplayDamageEvent );
	NATIVE_FUNCTION( "SignalGameplayEvent", funcSignalGameplayEvent );
	NATIVE_FUNCTION( "SignalGameplayEventParamCName", funcSignalGameplayEventParamCName );
	NATIVE_FUNCTION( "SignalGameplayEventParamInt", funcSignalGameplayEventParamInt );
	NATIVE_FUNCTION( "SignalGameplayEventParamFloat", funcSignalGameplayEventParamFloat );
	NATIVE_FUNCTION( "SignalGameplayEventParamObject", funcSignalGameplayEventParamObject );
	NATIVE_FUNCTION( "SignalGameplayEventReturnCName", funcSignalGameplayEventReturnCName );
	NATIVE_FUNCTION( "SignalGameplayEventReturnInt", funcSignalGameplayEventReturnInt );
	NATIVE_FUNCTION( "SignalGameplayEventReturnFloat", funcSignalGameplayEventReturnFloat );
	NATIVE_FUNCTION( "ForceAIUpdate", funcForceAIUpdate );
	NATIVE_FUNCTION( "GetRadius", funcGetRadius );
	NATIVE_FUNCTION( "PlayVoiceset", funcPlayVoiceset );
	NATIVE_FUNCTION( "StopAllVoicesets", funcStopAllVoicesets );
	NATIVE_FUNCTION( "HasVoiceset", funcHasVoiceset );	
	NATIVE_FUNCTION( "PlayScene", funcPlayScene );
	NATIVE_FUNCTION( "StopAllScenes", funcStopAllScenes );
	NATIVE_FUNCTION( "EmptyHands", funcEmptyHands );
	NATIVE_FUNCTION( "IsRotatedTowards", funcIsRotatedTowards );
	NATIVE_FUNCTION( "IsRotatedTowardsPoint", funcIsRotatedTowardsPoint );
	NATIVE_FUNCTION( "GetAliveFlag", funcGetAliveFlag );
	NATIVE_FUNCTION( "SetAlive", funcSetAlive );
	NATIVE_FUNCTION( "IsExternalyControlled", funcIsExternalyControlled );
	NATIVE_FUNCTION( "IsInCombat", funcIsInCombat );
	NATIVE_FUNCTION( "IsMoving", funcIsMoving );
	NATIVE_FUNCTION( "GetMoveDestination", funcGetMoveDestination );
	NATIVE_FUNCTION( "GetPositionOrMoveDestination", funcGetPositionOrMoveDestination );
	NATIVE_FUNCTION( "GetVisualDebug", funcGetVisualDebug );
	NATIVE_FUNCTION( "GetVoicetag", funcGetVoicetag );
	NATIVE_FUNCTION( "GetSkeletonType", funcGetSkeletonType );
	NATIVE_FUNCTION( "UseItem", funcUseItem );
	NATIVE_FUNCTION( "SetVisibility", funcSetVisibility );
	NATIVE_FUNCTION( "GetVisibility", funcGetVisibility );
	NATIVE_FUNCTION( "SetAppearance", funcSetAppearance );
	NATIVE_FUNCTION( "GetAppearance", funcGetAppearance );
	NATIVE_FUNCTION( "GetHeadAngleHorizontal", funcGetHeadAngleHorizontal );
	NATIVE_FUNCTION( "GetHeadAngleVertical", funcGetHeadAngleVertical );
	NATIVE_FUNCTION( "GetAnimationTimeMultiplier", funcGetAnimationTimeMultiplier);
	NATIVE_FUNCTION( "SetAnimationTimeMultiplier", funcSetAnimationTimeMultiplier);
	NATIVE_FUNCTION( "GetMovingAgentComponent", funcGetMovingAgentComponent );
	NATIVE_FUNCTION( "GetMorphedMeshManagerComponent", funcGetMorphedMeshManagerComponent );
	NATIVE_FUNCTION( "EnablePathEngineAgent", funcEnablePathEngineAgent );
	NATIVE_FUNCTION( "IsRagdollObstacle", funcIsRagdollObstacle );
	NATIVE_FUNCTION( "ClearRotationTarget", funcClearRotationTarget );
	NATIVE_FUNCTION( "SetRotationTarget", funcSetRotationTarget );
	NATIVE_FUNCTION( "SetRotationTargetPos", funcSetRotationTargetPos );
	NATIVE_FUNCTION( "EnableCollisionInfoReportingForItem", funcEnableCollisionInfoReportingForItem );
	NATIVE_FUNCTION( "EnablePhysicalMovement", funcEnablePhysicalMovement );
	NATIVE_FUNCTION( "EnableCharacterCollisions", funcEnableCharacterCollisions );
	NATIVE_FUNCTION( "EnableDynamicCollisions", funcEnableDynamicCollisions );
	NATIVE_FUNCTION( "EnableStaticCollisions", funcEnableStaticCollisions );
	NATIVE_FUNCTION( "IsInNonGameplayCutscene", funcIsInNonGameplayScene );
	NATIVE_FUNCTION( "IsInGameplayScene", funcIsInGameplayScene );
	NATIVE_FUNCTION( "PlayLine", funcPlayLine );
	NATIVE_FUNCTION( "PlayLineByStringKey", funcPlayLineByStringKey );
	NATIVE_FUNCTION( "EndLine", funcEndLine );
	NATIVE_FUNCTION( "IsSpeaking", funcIsSpeaking );
	NATIVE_FUNCTION( "EnableDynamicLookAt", funcEnableDynamicLookAt );
	NATIVE_FUNCTION( "EnableStaticLookAt", funcEnableStaticLookAt );
	NATIVE_FUNCTION( "DisableLookAt", funcDisableLookAt );
	NATIVE_FUNCTION( "CutBodyPart", funcCutBodyPart );
	NATIVE_FUNCTION( "IsAttackableByPlayer", funcIsAttackableByPlayer );
	NATIVE_FUNCTION( "SetAttackableByPlayerPersistent", funcSetAttackableByPlayerPersistent );
	NATIVE_FUNCTION( "SetAttackableByPlayerRuntime", funcSetAttackableByPlayerRuntime );
	NATIVE_FUNCTION( "InAttackRange", funcInAttackRange );
	NATIVE_FUNCTION( "GetNearestPointInPersonalSpace", funcGetNearestPointInPersonalSpace );
	NATIVE_FUNCTION( "GetNearestPointInPersonalSpaceAt", funcGetNearestPointInPersonalSpaceAt );
	NATIVE_FUNCTION( "GatherEntitiesInAttackRange", funcGatherEntitiesInAttackRange );
	NATIVE_FUNCTION( "CalculateHeight", funcCalculateHeight );
	NATIVE_FUNCTION( "PlayMimicAnimationAsync", funcPlayMimicAnimationAsync );
	NATIVE_FUNCTION( "CanPlayQuestScene", funcCanPlayQuestScene );
	NATIVE_FUNCTION( "HasInteractionScene", funcHasInteractionScene );
	NATIVE_FUNCTION( "CanTalk", funcCanTalk );
	NATIVE_FUNCTION( "GetActorAnimState", funcGetActorAnimState );
	NATIVE_FUNCTION( "IsInView", funcIsInView );
	NATIVE_FUNCTION( "GetHeadBoneIndex", funcGetHeadBoneIndex );
	NATIVE_FUNCTION( "GetTorsoBoneIndex", funcGetTorsoBoneIndex );
	NATIVE_FUNCTION( "GetTarget", funcGetTarget );
	NATIVE_FUNCTION( "IsDangerous", funcIsDangerous );
	NATIVE_FUNCTION( "GetAttitude", funcGetAttitude );
	NATIVE_FUNCTION( "SetAttitude", funcSetAttitude );
	NATIVE_FUNCTION( "ResetAttitude", funcResetAttitude );
	NATIVE_FUNCTION( "HasAttitudeTowards", funcHasAttitudeTowards );
	NATIVE_FUNCTION( "ClearAttitudes", funcClearAttitudes );
	NATIVE_FUNCTION( "GetAttitudeGroup", funcGetAttitudeGroup );
	NATIVE_FUNCTION( "GetBaseAttitudeGroup", funcGetBaseAttitudeGroup );
	NATIVE_FUNCTION( "SetBaseAttitudeGroup", funcSetBaseAttitudeGroup );
	NATIVE_FUNCTION( "ResetBaseAttitudeGroup", funcResetBaseAttitudeGroup );
	NATIVE_FUNCTION( "SetTemporaryAttitudeGroup", funcSetTemporaryAttitudeGroup );
	NATIVE_FUNCTION( "ResetTemporaryAttitudeGroup", funcResetTemporaryAttitudeGroup );
	NATIVE_FUNCTION( "SetAttitudeGroup", funcSetAttitudeGroup );
	NATIVE_FUNCTION( "ResetAttitudeGroup", funcResetAttitudeGroup );
	NATIVE_FUNCTION( "CanStealOtherActor", funcCanStealOtherActor );
	NATIVE_FUNCTION( "ResetClothAndDangleSimulation", funcResetClothAndDangleSimulation );

	NATIVE_FUNCTION( "DrawItems", funcDrawItems );
	NATIVE_FUNCTION( "HolsterItems", funcHolsterItems );
	NATIVE_FUNCTION( "DrawItemsLatent", funcDrawItemsLatent );
	NATIVE_FUNCTION( "HolsterItemsLatent", funcHolsterItemsLatent );

	NATIVE_FUNCTION( "HasLatentItemAction", funcHasLatentItemAction );
	NATIVE_FUNCTION( "WaitForFinishedAllLatentItemActions", funcWaitForFinishedAllLatentItemActions );
	NATIVE_FUNCTION( "DrawWeaponAndAttackLatent", funcDrawWeaponAndAttackLatent );
	NATIVE_FUNCTION( "SetBehaviorMimicVariable", funcSetBehaviorMimicVariable );
	NATIVE_FUNCTION( "RaiseBehaviorMimicEvent", funcRaiseBehaviorMimicEvent );
	NATIVE_FUNCTION( "SetLookAtMode", funcSetLookAtMode );
	NATIVE_FUNCTION( "ResetLookAtMode", funcResetLookAtMode );
	NATIVE_FUNCTION( "IsUsingExploration", funcIsUsingExploration );
	NATIVE_FUNCTION( "GetAnimCombatSlots", funcGetAnimCombatSlots );
	NATIVE_FUNCTION( "ForceAIBehavior", funcForceAIBehavior );
	NATIVE_FUNCTION( "CancelAIBehavior", funcCancelAIBehavior );
	NATIVE_FUNCTION( "GetScriptStorageObject", funcGetScriptStorageObject );
	NATIVE_FUNCTION( "GetAutoEffects", funcGetAutoEffects );

	NATIVE_FUNCTION( "SetInteractionPriority", funcSetInteractionPriority );
	NATIVE_FUNCTION( "SetOriginalInteractionPriority", funcSetOriginalInteractionPriority );
	NATIVE_FUNCTION( "RestoreOriginalInteractionPriority", funcRestoreOriginalInteractionPriority );
	NATIVE_FUNCTION( "GetOriginalInteractionPriority", funcGetOriginalInteractionPriority );
    NATIVE_FUNCTION( "GetInteractionPriority", funcGetInteractionPriority );
	NATIVE_FUNCTION( "SetUnpushableTarget", funcSetUnpushableTarget );
	NATIVE_FUNCTION( "EnableCollisions", funcEnableCollisions );
	NATIVE_FUNCTION( "PredictWorldPosition", funcPredictWorldPosition );


	// Actions
	NATIVE_FUNCTION( "IsReadyForNewAction", funcIsReadyForNewAction );
	NATIVE_FUNCTION( "ActionCancelAll", funcActionCancelAll );
	NATIVE_FUNCTION( "GetCurrentActionPriority", funcGetCurrentActionPriority );
	NATIVE_FUNCTION( "IsDoingSomethingMoreImportant", funcIsDoingSomethingMoreImportant );
	NATIVE_FUNCTION( "IsCurrentActionInProgress", funcIsCurrentActionInProgress );
	NATIVE_FUNCTION( "IsCurrentActionSucceded", funcIsCurrentActionSucceded );
	NATIVE_FUNCTION( "IsCurrentActionFailed", funcIsCurrentActionFailed );
	NATIVE_FUNCTION( "ActionMoveToNode", funcActionMoveToNode );
	NATIVE_FUNCTION( "ActionMoveToNodeAsync", funcActionMoveToNodeAsync );
	NATIVE_FUNCTION( "ActionMoveToNodeWithHeading", funcActionMoveToNodeWithHeading );
	NATIVE_FUNCTION( "ActionMoveToNodeWithHeadingAsync", funcActionMoveToNodeWithHeadingAsync );
	NATIVE_FUNCTION( "ActionMoveTo", funcActionMoveTo );
	NATIVE_FUNCTION( "ActionMoveToChangeTargetAsync", funcActionMoveToChangeTargetAsync );	
	NATIVE_FUNCTION( "ActionMoveToAsync", funcActionMoveToAsync );
	NATIVE_FUNCTION( "ActionMoveToWithHeading", funcActionMoveToWithHeading );
	NATIVE_FUNCTION( "ActionMoveToWithHeadingAsync", funcActionMoveToWithHeadingAsync );
	NATIVE_FUNCTION( "ActionMoveAwayFromNode", funcActionMoveAwayFromNode );
	NATIVE_FUNCTION( "ActionMoveAwayFromNodeAsync", funcActionMoveAwayFromNodeAsync );
	NATIVE_FUNCTION( "ActionMoveAwayFromLine", funcActionMoveAwayFromLine );
	NATIVE_FUNCTION( "ActionMoveAwayFromLineAsync", funcActionMoveAwayFromLineAsync );
	NATIVE_FUNCTION( "ActionMoveCustom", funcActionMoveCustom );
	NATIVE_FUNCTION( "ActionMoveCustomAsync", funcActionMoveCustomAsync );
	NATIVE_FUNCTION( "ActionMoveToDynamicNode", funcActionMoveToDynamicNode );
	NATIVE_FUNCTION( "ActionMoveToDynamicNodeAsync", funcActionMoveToDynamicNodeAsync );
	NATIVE_FUNCTION( "ActionSlideThrough", funcActionSlideThrough );
	NATIVE_FUNCTION( "ActionSlideThroughAsync", funcActionSlideThroughAsync );
	NATIVE_FUNCTION( "ActionSlideTo", funcActionSlideTo );
	NATIVE_FUNCTION( "ActionSlideToAsync", funcActionSlideToAsync );
	NATIVE_FUNCTION( "ActionSlideToWithHeadingAsync", funcActionSlideToWithHeadingAsync );
	NATIVE_FUNCTION( "ActionSlideToWithHeading", funcActionSlideToWithHeading );
	NATIVE_FUNCTION( "ActionMoveOnCurveTo", funcActionMoveOnCurveTo );
	NATIVE_FUNCTION( "ActionMoveOnCurveToAsync", funcActionMoveOnCurveToAsync );
	NATIVE_FUNCTION( "ActionRotateTo", funcActionRotateTo );
	NATIVE_FUNCTION( "ActionSetOrientation", funcActionSetOrientation );
	NATIVE_FUNCTION( "ActionRotateToAsync", funcActionRotateToAsync );
	NATIVE_FUNCTION( "ActionPlaySlotAnimation", funcActionPlaySlotAnimation );
	NATIVE_FUNCTION( "ActionPlaySlotAnimationAsync", funcActionPlaySlotAnimationAsync );
	NATIVE_FUNCTION( "ActionRaiseEvent", funcActionRaiseEvent );
	NATIVE_FUNCTION( "ActionRaiseForceEvent", funcActionRaiseForceEvent );
	NATIVE_FUNCTION( "ActionRaiseEventAsync", funcActionRaiseEventAsync );
	NATIVE_FUNCTION( "ActionRaiseForceEventAsync", funcActionRaiseForceEventAsync );
	NATIVE_FUNCTION( "ActionExitWork", funcActionExitWork );
	NATIVE_FUNCTION( "ActionExitWorkAsync", funcActionExitWorkAsync );
	NATIVE_FUNCTION( "ActionExploration", funcActorActionExploration );
	NATIVE_FUNCTION( "ActionAnimatedSlideToStatic", funcActionAnimatedSlideToStatic );
	NATIVE_FUNCTION( "ActionAnimatedSlideToStaticAsync", funcActionAnimatedSlideToStaticAsync );
	NATIVE_FUNCTION( "ActionAnimatedSlideToStaticAsync_P", funcActionAnimatedSlideToStaticAsync_P );
	NATIVE_FUNCTION( "ActionAnimatedSlideTo", funcActionAnimatedSlideTo );
	NATIVE_FUNCTION( "ActionAnimatedSlideToAsync", funcActionAnimatedSlideToAsync );
	NATIVE_FUNCTION( "ActionAnimatedSlideToAsync_P", funcActionAnimatedSlideToAsync_P );
	NATIVE_FUNCTION( "ActionMatchTo", funcActionMatchTo );
	NATIVE_FUNCTION( "ActionMatchToAsync", funcActionMatchToAsync );
	NATIVE_FUNCTION( "ActionMatchToAsync_P", funcActionMatchToAsync_P );

	NATIVE_FUNCTION( "GetCurrentActionType", funcGetCurrentActionType );
	NATIVE_FUNCTION( "SetErrorState", funcSetErrorState );
	NATIVE_FUNCTION( "WasVisibleLastFrame", funcWasVisibleLastFrame );
	NATIVE_FUNCTION( "PushAway", funcPushAway );
	NATIVE_FUNCTION( "PushInDirection", funcPushInDirection );
	NATIVE_FUNCTION( "SetMovementType", funcSetMovementType );
	NATIVE_FUNCTION( "GetMovementType", funcGetMovementType );
	NATIVE_FUNCTION( "IssueRequiredItems", funcIssueRequiredItems );
	NATIVE_FUNCTION( "SetRequiredItems", funcSetRequiredItems );

	NATIVE_FUNCTION( "IssueRequiredItemsGeneric", funcIssueRequiredItemsGeneric )
	NATIVE_FUNCTION( "SetRequiredItemsGeneric", funcSetRequiredItemsGeneric )

	NATIVE_FUNCTION( "ActivateAndSyncBehaviorWithItemsParallel", funcActivateAndSyncBehaviorWithItemsParallel );
	NATIVE_FUNCTION( "ActivateAndSyncBehaviorWithItemsSequence", funcActivateAndSyncBehaviorWithItemsSequence );
	NATIVE_FUNCTION( "ProcessRequiredItems", funcProcessRequiredItems );
	NATIVE_FUNCTION( "PlayPushAnimation", funcPlayPushAnimation );
	NATIVE_FUNCTION( "GetFallTauntEvent", funcGetFallTauntEvent );
	NATIVE_FUNCTION( "GetCharacterStatsParam", funcGetCharacterStatsParam );
	NATIVE_FUNCTION( "ApplyItemAbilities", funcApplyItemAbilities );
	NATIVE_FUNCTION( "RemoveItemAbilities", funcRemoveItemAbilities );
	NATIVE_FUNCTION( "ReportDeathToSpawnSystems", funcReportDeathToSpawnSystems );
	NATIVE_FUNCTION( "CanPush", funcCanPush );
	NATIVE_FUNCTION( "MuteHeadAudio", funcMuteHeadAudio );

	NATIVE_FUNCTION( "SetGroupShadows", funcSetGroupShadows );

	NATIVE_FUNCTION( "ForceSoundAppearanceUpdate", funcForceSoundAppearanceUpdate);

END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CPlaySoundOnActorRequest : public IEntityStateChangeRequest
{
	DECLARE_ENGINE_CLASS( CPlaySoundOnActorRequest, IEntityStateChangeRequest, 0 );

private:
	CName					m_boneName;
	StringAnsi				m_soundName;
	Float					m_fadeTime;

	Bool					m_executed;			// A runtime value telling if the request's already
												// been processed - 'cause this request can only
												// be processed once, but needs to stay registered
												// for future saves

public:
	CPlaySoundOnActorRequest();

	virtual void Execute( CGameplayEntity* entity );
	virtual String OnStateChangeRequestsDebugPage() const;

private:
	void funcInitialize( CScriptStackFrame& stack, void* result );
};
BEGIN_CLASS_RTTI( CPlaySoundOnActorRequest );
	PARENT_CLASS( IEntityStateChangeRequest );
	PROPERTY_EDIT_SAVED( m_boneName, TXT("Name of the bone to which the sound emitter should be attached") );
	PROPERTY_EDIT_SAVED( m_soundName, TXT("Sound event name") );
	PROPERTY_EDIT_SAVED( m_fadeTime, TXT("Fade time") );
	NATIVE_FUNCTION( "Initialize", funcInitialize );
END_CLASS_RTTI();



template< class T >
inline T * CActor::GetAiStorageData( CName aiStorageName )
{
	if ( m_behTreeMachine == nullptr )
	{
		return nullptr;
	}
	CBehTreeInstance *const behTreeInstance = m_behTreeMachine->GetBehTreeInstance();
	if ( behTreeInstance == nullptr )
	{
		return nullptr;
	}
	return behTreeInstance->GetTypedItem< T >( aiStorageName );
}

template< class T >
inline T * CActor::GetScriptAiStorageData( CName aiStorageName )
{
	if ( m_behTreeMachine == nullptr )
	{
		return nullptr;
	}
	CBehTreeInstance *const behTreeInstance = m_behTreeMachine->GetBehTreeInstance();
	if ( behTreeInstance == nullptr )
	{
		return nullptr;
	}
	return behTreeInstance->TScriptableStoragetFindItem< T >( aiStorageName );
}



///////////////////////////////////////////////////////////////////////////////
