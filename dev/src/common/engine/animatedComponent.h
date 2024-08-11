/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "animationEvent.h"
#include "../physics/physicalCollision.h"
#include "component.h"
#include "skeletonProvider.h"
#include "slotProvider.h"
#include "animatedObjectInterface.h"
#include "scaleable.h"
#include "../core/countedBool.h"
#include "animSyncInfo.h"
#include "animMath.h"
#include "behaviorGraphOutput.h"
#include "skeletonUtils.h"
#include "wetnessComponent.h"
#include "teleportDetector.h"

class IAnimationConstraint;
class CBehaviorGraphStack;
class CSlotComponent;
class CPhysicsRagdollWrapper;
class CRagdoll;
class CBehaviorGraphStackSnapshot;
class CSkeletalAnimationContainer;
class CWetnessSupplier;
struct SBehaviorSampleContext;
struct SBehaviorUpdateContext;
class CBehaviorAnimatedSkeleton;
class CJobAsyncUpdateAnimation;
class CSpeedConfig;
struct SMeshSkinningUpdateContext;

enum ECharacterPhysicsState : CEnum::TValueType;
enum EBehaviorLod : CEnum::TValueType;

typedef THashMap< CName, CEventNotifier< CAnimationEventFired > >	TEventHandlersMap;

//#define USE_OPT_SKINNING

#ifdef DEBUG_ANIMS
#define DEBUG_AC
#endif

enum ECharacterCollisionFlags
{
	CCF_Controller	= FLAG(1),
	CCF_Camera		= FLAG(2),
	CCF_Areas		= FLAG(3),
	CCF_Proxy		= FLAG(4),
	CCF_Exploration = FLAG(5),
	CCF_All			= CCF_Controller | CCF_Camera | CCF_Areas | CCF_Proxy | CCF_Exploration
};

enum EAnimatedComponentDebugDisp
{
	ACDD_SkeletonBone = FLAG(1),
	ACDD_SkeletonAxis = FLAG(2),
	ACDD_SkeletonName = FLAG(3)
};

RED_DECLARE_NAME(StopAllAnimationsOnSkeleton)

//////////////////////////////////////////////////////////////////////////
// Prototype

class CAnimatedComponentPhysicsRepresentation : public CObject
{
	DECLARE_ENGINE_CLASS( CAnimatedComponentPhysicsRepresentation, CObject, 0 );

private:
	THandle< CRagdoll >			m_ragdoll;							// Ragdoll resource

	Bool						m_ragdollAlwaysEnabled;				// Ragdoll should be always enabled
	Bool						m_allowRagdollInCutscene;			// Ragdoll can be enabled during cutscenes

	Bool						m_notifyParentAboutRagdollChange;	// Indicates whether script event should be called
	Bool						m_hasRagdollAttachments;			// Indicates that ragdoll attachment is connected
	Bool						m_ragdollStateToSet;				// Ragdoll state to be set
	Bool						m_ragdollStateChanged;				// Ragdoll state have changed
	Bool						m_ragdollDeactivated;

	CPhysicsRagdollWrapper*		m_ragdollPhysicsWrapper;			// Ragdoll wrapper
	CPhysicalCollision			m_ragdollCollisionType;

public:
	CAnimatedComponentPhysicsRepresentation();

	
};

BEGIN_CLASS_RTTI( CAnimatedComponentPhysicsRepresentation )
	PARENT_CLASS( CObject );
	PROPERTY_EDIT( m_ragdoll, TXT("Ragdoll") );
	PROPERTY_CUSTOM_EDIT( m_ragdollCollisionType, TXT( "Defines what of types it is from physical collision point of view" ), TXT("PhysicalCollisionTypeSelector") );
	PROPERTY_EDIT( m_ragdollAlwaysEnabled, TXT("Ragdoll should always be enabled") );
	PROPERTY_EDIT( m_allowRagdollInCutscene, TXT( "Ragdoll can be enabled during cutscenes" ) );
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

/// Animation driven skeleton, does not support drawing by itself
class CAnimatedComponent	: public CComponent
							, protected ISkeletonDataProvider
							, protected ISlotProvider
							, public IAnimatedObjectInterface
							, public ILODable
{
	friend class CJobImmediateUpdateAnimation;
	friend class CJobAsyncUpdateAnimation;

	DECLARE_ENGINE_CLASS( CAnimatedComponent, CComponent, 0 );

	class CBoxVisibilityFlag
	{
		static const Int32 COUNTER_VAL = 3;

		Int32 m_counter;

	public:
		CBoxVisibilityFlag()		: m_counter( COUNTER_VAL ) {}
		void Set()					{ m_counter = Max( m_counter-1, 0 ); }
		void Reset()				{ m_counter = COUNTER_VAL; }
		operator Bool() const		{ return m_counter == 0; }
	};

public:
	enum AnimationSuppressReason
	{
		AnimationSuppressionReason_Visibility	= FLAG( 0 ),
		AnimationSuppressionReason_Distance		= FLAG( 1 )
	};

protected:
	typedef TDynArray< THandle< CSkeletalAnimationSet > > TAnimationSets;
	SBehaviorUsedAnimations					m_recentlyUsedAnims;						//!< to store recently used anims that can be used to predict character's movement

	TStaticArray< CAnimationEventFired, 32 >	m_trackedEvents;
	TStaticArray< CAnimationEventFired, 32 >	m_lastFrameEvents;
	TDynArray< THandle< CAnimatedAttachment > >	m_cachedAnimatedAttachments;
	TDynArray< CAnimatedComponent* >			m_cachedAnimatedChildComponents;			//!< All children that are animated components
	TDynArray< SBehaviorGraphInstanceSlot >		m_runtimeBehaviorInstanceSlots;
	TDynArray< SBehaviorGraphInstanceSlot >		m_behaviorInstanceSlots;
	TDynArray< AnimQsTransform >				m_skeletonPreRagdollLocalSpace;				//!< Skeleton matrices in local space used to blend pre-ragdoll pose with ragdoll
	TDynArray< Matrix >							m_skeletonModelSpace;						//!< Skeleton matrices in model space (as displayed, used for skinning and ragdoll sync)
	TAnimationSets								m_animationSets;
	TDynArray< CAnimatedComponent* >			m_updateAnimatedComponents;					//!< Animated components to be updated after this component is updated

#ifdef USE_OPT_SKINNING
	TDynArray< Matrix >							m_skeletonInvBindSpace;
#endif

	// TO REFACTOR - skinning is in modes space now ---
	TDynArray< Matrix >						m_skeletonWorldSpace;						//!< Skeleton matrices in world space
	// ---

	TEventHandlersMap						m_animationEventHandlers;					//!< Notifiers for animation events	

	// TO REMOVE ---
	CJobAsyncUpdateAnimation*				m_updateAnimationSyncJob;
	// ---
	
	THandle< CSkeleton >					m_skeleton;
	THandle< CRagdoll >						m_ragdoll;
	
	CAnimatedComponentPhysicsRepresentation* m_physicsRepresentation;

	CBehaviorGraphStack*					m_behaviorGraphStack;						//!< Behavior graph instance stack
	SBehaviorSampleContext*					m_behaviorGraphSampleContext;				//!< Sample context for behavior graphs
	SBehaviorUpdateContext*					m_behaviorGraphUpdateContext;				//!< Update context for behavior graphs
	CBehaviorGraphStackSnapshot*			m_behaviorGraphStackSnapshot;				//!< Behavior graph stack snapshot for loading
	CSkeletalAnimationContainer*			m_animations;								//!< Build set of animations for faster name lookup
	CBehaviorAnimatedSkeleton*				m_animatedSkeleton;							//!< Runtime skeleton
	CPhysicsRagdollWrapper*					m_ragdollPhysicsWrapper;					//!< Ragdoll wrapper
	Bool									m_hasBehaviorRagdollNode;
	Bool									m_createRagdollAsKinematic;					//!< Force ragdoll's physics wrapper switch to kinematic right after creation
	CPhysicalCollision						m_ragdollCollisionType;
	CAnimatedComponent*						m_updatedByAnimatedComponent;				//!< If explicitely updated with other animated component

#ifndef RED_FINAL_BUILD
	static Red::Threads::CAtomic< Int32 >	s_fullyProcessedCounter;
	static Red::Threads::CAtomic< Int32 >	s_skippedCounter;
#endif

	Matrix									m_thisFrameTempLocalToWorld;				//!< This frame temporary local to world
	Matrix									m_characterTransformDelta;					//!< Transformation of character
	Box										m_componentBBoxMS;							//!< Components bbox in MS
	CBoxVisibilityFlag						m_componentBBoxValid;
	Bool									m_isNotInCameraView;

	// TO REMOVE ---
	Float									m_relativeMoveSpeed;						//!< Relative move speed inputed by steering systems
	Vector									m_moveTarget;								//!< Move destination
	Float									m_moveHeading;								//!< Move final heading
	Float									m_moveDirection;							//!< Move direction (move direction in actor space, for animation selection)
	Float									m_moveRotation;								//!< Move rotation to set in current frame
	Float									m_moveRawDesiredRotation;					//!< Move rotation but not blended
	Float									m_moveRotationSpeed;
	// ---

	Float									m_storedTimeDelta;							//!< Time delta stored to process attached objects after getting ragdoll info
	Float									m_skeletonPreRagdollWeight;					//!< used to blend local space from pre-ragdoll (or last update without ragdoll) with ragdoll
	Uint32									m_debugDisp;								//!< Debug display flags
	EBehaviorLod							m_behaviorLOD;
	Int32									m_lodBoneNum;
	Int32									m_trajectoryBone;		
	Float									m_timeMultiplier;							//!< timeDelta multiplier for this component

	Uint32									m_rareTickSkip;								//!< How many frames do we skip
	Uint32									m_rareTickSkipMultiplier;					//!< Multiplier for frame skip
	Uint32									m_rareTickSkipAdd;							//!< Additional value for frame skip
	Uint32									m_rareTickSkipLimit;						//!< How many frames can we skip
	Uint32									m_rareTickSkipLimitDueToAI;					//!< How many frames can we skip (AI may force different limit)
	Float									m_rareTickAccumulatedTimeDelta;				//!< Time accumulated to do next advancement
	Float									m_rareTickPrevToLastTimeDelta;				//!< So we know how much motion did we accumulate, etc
	Uint32									m_rareTickFramesSkipped;					//!< How many frames have we skipped so far
	EBehaviorLod							m_rareTickDoneForLOD;						//!< For what LOD did we do it recently - if that changes, we do full pass again

	Float									m_overrideBudgetedTickDistance;				//!< Override for default (min) budgeted tick distance; 0 indicates to use default from LOD manager
	Float									m_overrideDisableTickDistance;				//!< Override for default (min) disabled tick (i.e. no tick) distance; 0 indicates to use default from LOD manager

	// TO REMOVE ---
	CSyncInfo								m_asyncPlayedAnimSyncInfo;
	CName									m_asyncPlayedAnimName;
	CName									m_defaultBehaviorAnimationSlotNode;			//!< animation slot used in entity browser
	Float									m_animationMultiplier;
	// ---

	Bool									m_savable;
	Bool									m_isFrozenOnStart;
	
	// TO REMOVE ---
	Bool									m_includedInAllAppearances;					//!< this animated component is implicitly included in all appearances
	// ---

	// TO REMOVE ---
	Bool									m_isInCutscene;
	Bool									m_isInScene;
	// ---

	Bool									m_useExtractedMotion;						//!< Extract motion from movement and apply it
	Bool									m_movementFinalized;						//!< has movement been finalized during this frame (to do it just once)
	Bool									m_ragdollNodeProvidesValidPose;				//!< if switched to ragdoll, ragdoll node may still allow to update nodes and will provide their output, so it can be blended
	Bool									m_stickRagdollToCapsule;					//!< pulls ragdoll to capsule if too far away
	Bool									m_allowConstraintsUpdate;					//!< allows ignoring constraints behavior graph (i.e., for job tree editor previews)

protected:
	Bool									m_forceUpdateSkinning				: 1;	//!< Update skinning flag - use after teleport etc. ( Temp )
	Bool									m_extractTrajectory					: 1;	//!< Animated component should extract trajectory
	Bool									m_postInstanceInitializationDone	: 1;	//!< We have used post instancing initialization
	Bool									m_attachedToParentWithAnimatedAttachment: 1;//!< Attached to parent through animated attachment
	Bool									m_attachedToAnimatedComponentParent	: 1;	//!< Attached to parent that is animated component

protected:

	Bool									m_rareTickInvalidatePrev			: 1;	//!< When we will be caching update and sample, make prev same as new cache
	Bool									m_rareTickForceFullProcessing		: 1;	//!< Force full processing (for any reason)
	Bool									m_disableConstraintsIfPossible		: 1;	//!< Constraints should be disabled if possible (they should make decision on their own whether to disable or not)
	Bool									m_poseSetManually					: 1;	//!< Pose was set "manually" (and probably shouldn't be "automatically" modified)

protected:
	Bool									m_allowPoseActions					: 1;	//!< Disable executing post actions on components that should not have any actions but trying to execute them may break stuff
	Bool									m_allowScriptNotifications			: 1;	//!< Disable executing script notifications on components that should not have any actions but trying to execute them may break stuff

	Bool									m_isRegisteredForComponentLODding	: 1 ;	// Is registered in component LOD system?	
	Bool									m_processPostponedEvents			: 1 ;	// Is registered in component LOD system?	
	Uint8									m_animationSuppressionMask;					// Animation suppression reason

private:
	CWetnessSupplier*						m_wetnessSupplier;							// Wetness support
	CTeleportDetector*						m_teleportDetector;							// TeleportDetector to determine teleport based on l2w change or pose change

protected:
	// Speed Config
	/// The key to the speed config in speedConfig.xml
	CName									m_defaultSpeedConfigKey;
	/// The actual speed config data
	const CSpeedConfig *					m_speedConfig;
public:
	// Set resource
	virtual void SetResource( CResource* resource ) override;
	virtual void GetResource( TDynArray< const CResource* >& resources ) const override;

	//! Get source skeleton
	virtual CSkeleton* GetSkeleton() const { return m_skeleton.Get(); }

	//! Get group of all used animations
	RED_INLINE CSkeletalAnimationContainer* GetAnimationContainer() const { return m_animations; }

	//! Get animated skeleton
	RED_INLINE CBehaviorAnimatedSkeleton* GetAnimatedSkeleton() const { return m_animatedSkeleton; }

	//! Get animation sets used by this animated component
	RED_INLINE const TDynArray< THandle< CSkeletalAnimationSet > > &GetAnimationSets() const { return m_animationSets; }

	//! Get behavior instances used by this animated component
	RED_INLINE const TDynArray< SBehaviorGraphInstanceSlot > &GetBehaviorSlots() const { return m_behaviorInstanceSlots; }

	//! Get default animation slot used in entity browser
	RED_INLINE const CName& GetDefaultBehaviorAnimationSlotNode() const {return m_defaultBehaviorAnimationSlotNode; }

	//! Set default animation slot used in entity browser
	RED_INLINE void SetDefaultBehaviorAnimationSlotNode(const CName defaultBehaviorAnimationSlotNode) {m_defaultBehaviorAnimationSlotNode=defaultBehaviorAnimationSlotNode; }

	//! Get spawned ragdoll instance
	RED_INLINE CPhysicsRagdollWrapper* GetRagdollPhysicsWrapper() { return m_ragdollPhysicsWrapper; }

	//! Check if character is ragdolled (really ragdolled or in mockup ragdoll)
	virtual Bool IsRagdolled( Bool realRagdoll = true ) const;
	virtual Bool IsStatic( ) const;

	//! Check what's skeleton pre ragdoll weight
	RED_INLINE Float GetSkeletonPreRagdollWeight() const { return m_skeletonPreRagdollWeight; }

	//! Mark that ragdoll node provides valid pose
	RED_INLINE void SetRagdollNodeProvidesValidPose( bool ragdollNodeProvidesValidPose = true ) { m_ragdollNodeProvidesValidPose = ragdollNodeProvidesValidPose; }

	//! Get spawned ragdoll instance (const)
	RED_INLINE CPhysicsRagdollWrapper const * GetRagdollPhysicsWrapper() const { return m_ragdollPhysicsWrapper; }

	//! Mark that ragdoll if it has behavior ragdoll node
	RED_INLINE void SetHasBehaviorRagdollNode( bool ragIsActor = false ) { m_hasBehaviorRagdollNode = ragIsActor; }

	void OverrideRagdollCollisionGroup( CPhysicalCollision collisionGroup ) { m_ragdollCollisionType = collisionGroup; }

	//! Get information if it has behavior ragdoll node
	RED_INLINE Bool GetHasBehaviorRagdollNode() { return m_hasBehaviorRagdollNode; }

	//! Checks if ragdoll can be pulled to capsule if too far away
	RED_INLINE Bool IsRagdollStickedToCapsule() const { return m_stickRagdollToCapsule; }

	// Should this component be included in all appearances ?
	RED_INLINE Bool IsIncludedInAllAppearances() const { return m_includedInAllAppearances; }

	// Get attachment group for this component - it determines the order
	RED_INLINE virtual EAttachmentGroup GetAttachGroup() const { return ATTACH_GROUP_A1; }

	RED_INLINE Bool IsInScene() const { return m_isInScene; }
	RED_INLINE Bool IsInCutscene() const { return m_isInCutscene; }
	RED_INLINE Bool IsInCinematic() const { return IsInScene() || IsInCutscene(); }

	//! Get anim multiplier
	RED_INLINE Float GetAnimationMultiplier() const { return m_animationMultiplier; }

	//! Set anim multiplier
	RED_INLINE void SetAnimMultiplier( Float mult ) { m_animationMultiplier = mult; }

	const SBehaviorUsedAnimations& GetRecentlyUsedAnims() const { return m_recentlyUsedAnims; }

	const TDynArray< Matrix > & GetSkeletonModelSpace() const { return m_skeletonModelSpace; }

	//! Skip frame for update and sample to allow better framerate
	void SetSkipUpdateAndSampleFrames( Uint32 skipUpdateAndSampleFrames );
	RED_FORCE_INLINE Uint32 GetSkipUpdateAndSampleFrames() const { return m_rareTickSkip; }

	//! Set bias for skip frame for update and sample to allow better framerate
	void SetSkipUpdateAndSampleFramesBias( Uint32 multiplier = 1, Uint32 addition = 0 );

	//! Set limit for skip frame for update and sample to allow better framerate
	void SetSkipUpdateAndSampleFramesLimit( Uint32 limit = 1 );

	//! Set limit for skip frame for update and sample to allow better framerate (requested by AI, if no parameter given, will reset)
	void SetSkipUpdateAndSampleFramesLimitDueToAI( Uint32 limit = 0xffff );

	//! Force restart frame skipping
	void ForceRestartFrameSkipping() { m_rareTickInvalidatePrev = true; m_rareTickForceFullProcessing = true; }

	//! Ask constraints to disable to get better framerate
	void AskConstraintsToDisableIfPossible( Bool disableIfPossible ) { m_disableConstraintsIfPossible = disableIfPossible; }
	Bool ShouldConstraintsBeDisabledIfPossible() const { return m_disableConstraintsIfPossible; }

	const Matrix& GetThisFrameTempLocalToWorld() const { return m_thisFrameTempLocalToWorld; }
	const Vector& GetThisFrameTempPositionWSRef() const { return m_thisFrameTempLocalToWorld.GetTranslationRef(); }
	void SetThisFrameTempLocalToWorld( const Matrix& m ) { m_thisFrameTempLocalToWorld = m; }
	Bool GetThisFrameTempBoneTranslationWorldSpace( const Int32 boneIdx, Vector& outPositionWS ) const;
	Bool GetThisFrameTempBoneMatrixWorldSpace( const Int32 boneIdx, Matrix& outMatrixWS ) const;
	void SetThisFrameTempBoneModelSpace( const Int32 boneIdx, const Matrix& m );

	RED_INLINE const CName& GetAsyncPlayedAnimName() const { return m_asyncPlayedAnimName; }

private:
	void CopyRateTickSettingsFrom( const CAnimatedComponent * source );

public:
	CAnimatedComponent();
	virtual ~CAnimatedComponent();

	//! Property was changed in editor
	virtual void OnPropertyPostChange( IProperty* property );

	//! Override this to disallow some overrides
	virtual Bool CanOverridePropertyViaInclusion( const CName& propertyName ) const override;

	//! Property missing
	virtual Bool OnPropertyMissing( CName propertyName, const CVariant& readValue );

	//! Serialization
	virtual void OnSerialize( IFile &file );

	//! Object is going to be collected by GC
	virtual void OnFinalize();

	//! Component was destroyed
	virtual void OnDestroyed();

	//! Component was initialized
	virtual void OnInitialized();

	//! Component was uninitialized
	virtual void OnUninitialized();

	//! Component was attached to world
	virtual void OnAttached( CWorld* world );

	//! Component was detached from world
	virtual void OnDetached( CWorld *world );

	virtual void OnPostComponentInitializedAsync() override;

protected:
    void PostInitialization();

	virtual void OnPostInitializationDone();

public:
	// Component update, called async to physics
	virtual void OnTick( Float timeDelta );

	// Component update, called before physics simulation
	virtual void OnTickPrePhysics( Float timeDelta );

	virtual void OnTickPrePhysicsPost( Float timeDelta );

	// Component update, called after physics simulation		
	virtual void OnTickPostPhysics( Float timeDelta );

	// Component update, called after physics simulation, second
	virtual void OnTickPostPhysicsPost( Float timeDelta );

	// Component update, called after update transform
	virtual void OnTickPostUpdateTransform( Float timeDelta );

	// Update transform
	virtual void OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld ) override;
	virtual void OnUpdateTransformNode( SUpdateTransformContext& context, const Matrix& prevLocalToWorld ) override;

	// Turns on/off MovingAgentComponent motion. As function is rarely used as 'virtual' I made two functions on MAC, and marked this one as virtual.
	virtual void SetACMotionEnabled( Bool b );

	// Tests above shit NOTICE: it will always return 'true' for pure animated component
	virtual Bool IsACMotionEnabled() const;

public:
	// Gets whether this component is subject to LODding
	Bool IsGameplayLODable() const;

protected:
	// ILODable
	void UpdateLOD( ILODable::LOD newLOD, CLODableManager* manager ) override;
	ILODable::LOD ComputeLOD( CLODableManager* manager ) const override;

public:
	//! Called when we need to store gameplay state
	virtual void OnSaveGameplayState( IGameSaver* saver );
	virtual Bool CheckShouldSave() const;

	//! Called when we need to restore gameplay state
	virtual void OnLoadGameplayState( IGameLoader* loader );

public:
	RED_INLINE void  SetTimeMultiplier( Float multiplier ) { m_timeMultiplier = Max(0.f, multiplier); }
	RED_INLINE Float GetTimeMultiplier() const             { return m_timeMultiplier; }

public:
	virtual Int32 GetLodBoneNum() const;

	void SetLod( EBehaviorLod lod );
	EBehaviorLod GetLod() const;

protected:
	virtual void OnLodSet( EBehaviorLod prevLod, EBehaviorLod currLod ) {}

public:
	//! Freeze
	RED_FORCE_INLINE void Freeze() { SuppressTick( true, SR_Freeze ); }
	//! Unfreeze
	RED_FORCE_INLINE void Unfreeze() { SuppressTick( false, SR_Freeze ); }
	//! Is frozen
	RED_FORCE_INLINE Bool IsFrozen() const { return IsTickSuppressed( SR_Freeze ); }

	void FreezePoseFadeIn( Float duration );		// Smooth frozen pose transition based on deltaTime
	void UnfreezePoseFadeOut( Float duration );		// Smooth frozen pose transition based on deltaTime

	void FreezePoseBlendIn( Float duration );		// Smooth frozen pose transition based on pose blending
	void UnfreezePoseBlendOut( Float duration );	// Smooth frozen pose transition based on pose blending

	Bool HasFrozenPose() const;

	RED_INLINE const CTeleportDetector* GetTeleportDetector() const { return m_teleportDetector; }
	void SetTeleportDetectorForceUpdateOneFrame();

public:
	//! Get skeleton data provider interface
	virtual const ISkeletonDataProvider* QuerySkeletonDataProvider() const;

	//! Get slot provider interface
	virtual ISlotProvider* QuerySlotProvider();

	// Get animated object interface
	virtual IAnimatedObjectInterface* QueryAnimatedObjectInterface();

public:
	// Get mimic skeleton
	virtual CSkeleton* GetMimicSkeleton() const { return NULL; }

	// Get mimic pose name
	virtual const CName &GetMimicPoseName() const { return CName::NONE; }

public:
	//! Play animation without behavior system
	Bool PlayAnimationOnSkeleton( const CName& animation, Bool repleace = true, Bool looped = true, Float weight = 1.f );

	//! Play animation without behavior system paused at given time
	Bool PlayAnimationAtTimeOnSkeleton( const CName& animation, Float time, Bool repleace = true, Bool looped = true, Float weight = 1.f );

	//! Play animation without behavior system, with given sync info
	Bool PlayAnimationOnSkeletonWithSync( const CName& animation, const CSyncInfo& syncInfo );

	//! Stop all animations
	void StopAllAnimationsOnSkeleton();

	//! Pause all animations
	void PauseAllAnimationsOnSkeleton( Bool flag );
	void TogglePauseAllAnimationsOnSkeleton();

public:
	//! Get behavior stack
	RED_INLINE CBehaviorGraphStack* GetBehaviorStack() const { return m_behaviorGraphStack; }

	//! Get behavior graph update context
	RED_INLINE SBehaviorUpdateContext* GetBehaviorGraphUpdateContext() const { return m_behaviorGraphUpdateContext; }

	//! Get behavior graph sampling context
	RED_INLINE SBehaviorSampleContext* GetBehaviorGraphSampleContext() const { return m_behaviorGraphSampleContext; }

protected:
	//! Load behavior graphs
	void LoadBehaviorGraphs();

	//! Clear behavior graphs
	void ClearBehaviorGraphs();

	//! Create behavior context
	void CreateBehaviorContext();

	//! Destroy behavior context
	void DestroyBehaviorContext();

	//! Rebuilds animation container
	void BuildAnimContainer();

	//! Clear animation container
	void ClearAnimContainer();

public:
	//! Add animation set
	void AddAnimationSet( CSkeletalAnimationSet *animSet );

	//! Remove animation set
	void RemoveAnimationSet( CSkeletalAnimationSet *animSet );

	//! Force TPose on skeleton
	void ForceTPose( Bool scheduleUpdateTransformAfter = true );
	void ForceTPoseAndRefresh();

	//! Force initial behavior pose
	void ForceBehaviorPose();

	//! Force custom behavior pose
	void ForceBehaviorPose( const SBehaviorGraphOutput& pose );

	//! Force update skinning
	void ForceUpdateSkinning() { m_forceUpdateSkinning = true; }

	//! Sync pose to reference component's pose in world space.
	Bool SyncToPoseWS( const CAnimatedComponent* refComponent );

	//! Get current motion extraction delta
	const Matrix& GetAnimationMotionDelta() const { return m_characterTransformDelta; }

	//! Reset cached animation pointers
	void ResetAnimationCache();

#ifndef NO_EDITOR_FRAGMENTS
	// Generate editor rendering fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags );
#endif

	// This component should be show/hidden due to change in the entity appearance
	virtual void OnAppearanceChanged( Bool added ) override;

	// This component was created by the streaming system
	virtual void OnStreamIn() override;

public: // for editor
	//! Get behavior instance slot name ( editor )
	void GetBehaviorInstanceSlots( TDynArray< CName >& slotNames ) const;

	//! Get behavior instance slot graph ( editor )
	CBehaviorGraph* GetBehaviorInstanceGraph( const CName& slotName ) const;

public: // ISkeletonDataProvider
	// Find bone by name
	virtual Int32 FindBoneByName( const Char* name ) const;

	// Find bone by name
	virtual Int32 FindBoneByName( const AnsiChar* name ) const;

	// Get bones in the skeleton, returns number of bones in skeleton
	virtual Uint32 GetBones( TDynArray< BoneInfo >& bones ) const;

	// Get bones in the skeleton, returns number of bones in skeleton
	virtual Uint32 GetBones( TAllocArray< BoneInfo >& bones ) const;

	// Calc bone matrix in model space
	virtual Matrix CalcBoneMatrixModelSpace( Uint32 boneIndex ) const;

	// Get bone matrix in model space
	virtual Matrix GetBoneMatrixModelSpace( Uint32 boneIndex ) const;

	// Get bone matrix in world space
	virtual Matrix GetBoneMatrixWorldSpace( Uint32 boneIndex ) const;

	// Get bone matrices in the world space
	virtual void GetBoneMatricesAndBoundingBoxModelSpace( const ISkeletonDataProvider::SBonesData& bonesData ) const override;

	// Skeleton mapping stuff
	virtual Uint32 GetRuntimeCacheIndex() const  override;
	virtual const struct SSkeletonSkeletonCacheEntryData* GetSkeletonMaping( const ISkeletonDataProvider* parentSkeleton ) const override;

public: // IAnimatedObjectInterface
	virtual CEntity* GetAnimatedObjectParent() const;

	virtual Bool HasSkeleton() const;

	virtual Bool HasTrajectoryBone() const;
	virtual Int32 GetTrajectoryBone() const;

	virtual Bool UseExtractedMotion() const;
	virtual Bool UseExtractedTrajectory() const;

	virtual Int32 GetBonesNum() const;
	virtual Int32 GetTracksNum() const;

	virtual Int32 GetParentBoneIndex( Int32 bone ) const;
	virtual const Int16* GetParentIndices() const;

	virtual CEventNotifier< CAnimationEventFired >*	GetAnimationEventNotifier( const CName &eventName );

	virtual void PlayEffectForAnimation( const CName& animation, Float time );

public:
	// Find bone by name
	virtual Int32 FindBoneByName( const CName& name ) const;

	// Get ragdoll resource
	const CRagdoll* GetRagdoll() const;

	// Check if components has a ragdoll resource
	Bool HasRagdoll() const;

	//! Get float tracks num
	virtual Uint32 GetFloatTrackNum() const;

	//! Get float track
	virtual Float GetFloatTrack( Uint32 trackIndex ) const;

	//! Get bone matrix in local space
	virtual Matrix GetBoneMatrixLocalSpace( Uint32 boneIndex ) const;

	//! Get bone transform in local space
	AnimQsTransform GetBoneTransformLocalSpace( Uint32 boneIndex ) const;

	//! Is current skeleton pose i TPose
	Bool IsCurrentPoseTPose() const;

	//! Choose whether we should use motion extracted from animation to move this component
	void SetUseExtractedMotion( Bool flag );

	//! Choose whether we should use trajectory extracted from animation to move this component
	void SetUseExtractedTrajectory( Bool flag );

	//! Has animation event occurred
	Bool HasAnimationEventOccurred( const CName& animEventName ) const;

	//! Show skeleton
	Bool IsDispSkeleton( EAnimatedComponentDebugDisp flag ) const;
	void SetDispSkeleton( EAnimatedComponentDebugDisp flag, Bool enabled );

	// Get bone matrix in model space in animation at time
	void GetBoneMatrixMovementModelSpaceInAnimation( Uint32 boneIndex, CName const & animation, Float time, Float deltaTime, Matrix & refAtTime, Matrix & refWithDeltaTime ) const;

public:
	// Create slot
	virtual ISlot* CreateSlot( const String& slotName );

	// Enum all available slot names
	virtual void EnumSlotNames( TDynArray< String >& slotNames ) const;

	// Setup everything like in other animated component
	void SetAsCloneOf( const CAnimatedComponent* otherAnimComponent );

	// Setup based on slot component
	void BaseOnSlotComponent( const CSlotComponent* slotComponent );

public:

	//! Set move direction
	virtual void SetMoveDirection( Float moveDirection );

	//! Set move target
	void SetMoveTarget( const Vector& target );

	

	//! Set move heading
	void SetMoveHeading( Float heading );

	//! Move rotation to set in current frame
	void SetMoveRotation( Float rotation ) { SetMoveRotation( rotation, rotation ); }
	void SetMoveRotation( Float currentRotation, Float rawDesiredRotation );

	void SetMoveRotationSpeed( Float speed );

	// World space movement params
	const Vector& GetMoveTargetWorldSpace() const;
	Float GetMoveHeadingWorldSpace() const;
	Float GetMoveRotationWorldSpace() const;
	Float GetMoveDirectionWorldSpace() const;
	Float GetMoveRawDesiredRotationWorldSpace() const;
	virtual Float GetDesiredMoveDirectionWorldSpace() const;

	//! Converts an Absolute speed (m.s-1) into relative speed
	Float ConvertSpeedAbsToRel( Float speedAbs )const;

	//! Converts an Absolute speed (m.s-1) into relative speed
	Float ConvertSpeedRelToAbs( Float speedRel )const;

	//! Get relative speed that is fed to the behaviour graph
	Float GetRelativeMoveSpeed() const	{ return m_relativeMoveSpeed; }
	Float GetAbsoluteMoveSpeed() const;
	Float GetSpeedRelSpan() const;
	Float GetRotationSpeed() const;

	virtual void ForceSetRelativeMoveSpeed( Float relativeMoveSpeed ){ m_relativeMoveSpeed = relativeMoveSpeed; }

	// Model space movement coordinates
	Float GetMoveRotationModelSpace() const;
	virtual Vector GetMoveTargetModelSpace() const;
	virtual Float GetMoveHeadingModelSpace() const;
	Float GetMoveDirectionModelSpace() const;

protected:

	// Ragdoll setter
	void SetRagdoll( CRagdoll* ragdoll );

	// Toggle component internal state, implemented in child classes
	virtual void SetEnabled( Bool enabled );

	// Should add to tick groups
	virtual Bool ShouldAddToTickGroups() const;

	// Update whether it should be added or not to tick groups (requires to be in world)
	void UpdateTickGroups( CWorld* world = nullptr );

	// Add to tick groups
	void AddToTickGroups( CWorld* world );

	// Remove from all tick groups
	void RemoveFromTickGroups( CWorld* world );

	//! Set skeleton used by this animated component
	void SetSkeleton( CSkeleton *skeleton );

	//! Skeleton was changed
	virtual void OnSkeletonChanged();

	//! Can use behavior
	Bool CanUseBehavior() const;

	//! Can use animated skeleton
	Bool CanUseAnimatedSkeleton() const;

	// Can use behavior lod
	Bool CanUseBehaviorLod() const;

	//! Get behavior lod
	EBehaviorLod GetBehaviorLodLevel() const;

	//! Setup behavior contexts
	void SetupBehaviorContexts( SBehaviorSampleContext* sampleContext, SBehaviorUpdateContext* updateContext = NULL );

	//! Process pose actions from context
	void ProcessPoseActionsFromBehaviorContexts( const SBehaviorGraphOutput& output, SBehaviorSampleContext* sampleContext, SBehaviorUpdateContext* updateContext = NULL );

	//! Process scripts notifications from stack ( all instances )
	void ProcessScriptNotificationsFromStack();

	//! Update skeleton from pose
	void ProcessBehaviorOutputAll( Float timeDelta, SBehaviorGraphOutput& output );

	//! Update from pose
	void ProcessBehaviorOutputMotion( SBehaviorGraphOutput& output );

	//! Update from pose
	void ProcessBehaviorOutputEvents( SBehaviorGraphOutput& output );

	//! Update skeleton from pose
	void ProcessBehaviorOutputPose( SBehaviorGraphOutput& output );

	//! Update skeleton from pose
	virtual void ProcessPoseConstraints( Float timeDelta, SBehaviorGraphOutput& output );

	//! post process constraints
	virtual void PostProcessPoseConstraints( SBehaviorGraphOutput& /*output*/ ) {}

	//! Reset dynamic data
	void Reset();

	//! Cache trajectory bone index
	void CacheTrajectoryBone();

	// Display skeleton
	void DisplaySkeleton( CRenderFrame *frame, Color color = Color::WHITE ) const;

public:
	//! Get the value of effect parameter
	virtual Bool GetEffectParameterValue( CName paramName, CVariant &value /* out */ ) const;

	//! Set the value of effect parameter
	virtual Bool SetEffectParameterValue( CName paramName, const CVariant &value );

	using CComponent::GetEffectParameterValue;
	using CComponent::SetEffectParameterValue;

	//! Enumerate effect parameters. Copy parameters from behavior to effects
	virtual void EnumEffectParameters( CFXParameters &effectParams /* out */ );	

	// Returns behavior events that can be used in effect component
	void EnumEffectBehaviorEvents( TDynArray< CName > &behaviorEvents /* out */ );

public:
	// New parent attachment was added
	virtual void OnParentAttachmentAdded( IAttachment* attachment );

	// Parent attachment was broken
	virtual void OnParentAttachmentBroken( IAttachment* attachment );

	// New child attachment was added
	virtual void OnChildAttachmentAdded( IAttachment* attachment );

	// Child attachment was broken
	virtual void OnChildAttachmentBroken( IAttachment* attachment );

	virtual class CPhysicsWrapperInterface* GetPhysicsRigidBodyWrapper() const;

	virtual ECharacterPhysicsState GetCurrentPhysicsState() const;

public:
	// Called when this animated component should no longer be updated with other animated component
	void DontUpdateByOtherAnimatedComponent();

	// To force updating this animated component when other animated component is updated (it will be updated after that other animated component)
	void UpdateByOtherAnimatedComponent( CAnimatedComponent* ac );

private:
	// update when attachment has changed
	void UpdateAttachedToAnimParent();

	void AddChildAnimatedAttachment( CAnimatedAttachment* aa );
	void RemoveChildAnimatedAttachment( CAnimatedAttachment* aa );
	void SortAnimatedAttachments();

	// check whether it should be animated by parent that is animated component
	Bool ShouldBeUpdatedByAnimatedComponentParent() const;

protected:
	//! Internal update of graph
	void InternalUpdateAndSample( Float timeDelta );

	//! Internal update of graph
	virtual void InternalUpdateAndSampleMultiAsyncPart( Float timeDelta );
	virtual void InternalUpdateAndSampleMultiSyncPart( Float timeDelta );

	//! Do processing of pose constraints
	void UpdatePoseConstraints( Float timeDelta );

	//! Do post processing of pose constraints
	void PostUpdatePoseConstraints( Float timeDelta );

	//! Updates attached components transforms 
	void UpdateAttachedComponentsTransforms( SUpdateTransformContext& context );
	void UpdateAttachedComponentsTransformsWithoutSkinning( SUpdateTransformContext& context );

	//! Can use async update mode
	virtual Bool CanUseAsyncUpdateMode() const;

	//! Start async update
	void StartAsyncUpdateAndSample( Float timeDelta );

	//! Finish async update
	void FinishAsyncUpdateAndSample();

	//! Calc transforms
	void CalcTransforms();
	void CalcTransformsInvMSAndWS();
	void CalcTransformsWS( const Int32 lodBoneNum );
	void CalcTransformsInvMS( const Int32 lodBoneNum );

private:
	void CacheAnimatedAttachments();

	void UpdateAttachedAnimatedObjectsLS( Float dt, SBehaviorGraphOutput* poseLS );
	void UpdateAttachedAnimatedObjectsLSAsync( Float dt, SBehaviorGraphOutput* poseLS );
	void UpdateAttachedAnimatedObjectsLSSync( Float dt, SBehaviorGraphOutput* poseLS );
	void UpdateAttachedAnimatedObjectsLSConstrainted( Float dt, SBehaviorGraphOutput* poseLS );

	void UpdateAttachedAnimatedObjectsWS();

public:
	virtual void OnParentUpdatedAttachedAnimatedObjectsLS( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones );
	virtual void OnParentUpdatedAttachedAnimatedObjectsLSAsync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones );
	virtual void OnParentUpdatedAttachedAnimatedObjectsLSSync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones );
	virtual void OnParentUpdatedAttachedAnimatedObjectsLSConstrainted( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones );
	virtual void OnParentUpdatedAttachedAnimatedObjectsWS( const CAnimatedComponent* parent, SBehaviorGraphOutput* poseLS, TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones );

	bool IsAttachedToParentWithAnimatedAttachment() const { return m_attachedToParentWithAnimatedAttachment; }
	bool IsAttachedToAnimatedComponentParent() const { return m_attachedToAnimatedComponentParent; }

public:
	//! Internal finalize movement ( apply delta ), pass controller index in physx list to properly solve CC-CC separation
    virtual Bool FinalizeMovement1_PreSeparation( Float timeDelta ); // returns true if moved
    virtual void FinalizeMovement2_PostSeparation( Float timeDelta );

	virtual void SnapToNavigableSpace( Bool b ) {}
	virtual Bool IsSnapedToNavigableSpace() const { return false; }

protected:
	Bool ExtractMotion( Float timeDelta, Vector& outDeltaPosition, EulerAngles& outDeltaRotation );

	//! Compute delta and apply it
	virtual Bool ProcessMovement( Float timeDelta );

	//! Triggers the movement processing
    virtual void UpdateMovement1_PreSeperation( Float /*timeDelta*/ ){};
    virtual Bool UpdateMovement2_PostSeperation( Float /*timeDelta*/ );

	//! Called when an output pose is received
	virtual void OnProcessBehaviorPose( const SBehaviorGraphOutput& /*output*/ ) {}

public:
	// Collect immediate jobs
	virtual void CollectImmediateJobs( STickManagerContext& context, CTaskBatch& taskBatch );

public:
	// Get the radius for physical environment tests (method to be overloaded)
	virtual Float GetRadius() const;

	// Return true if this component should be informed about time switches via OnTimeSwitch
	virtual Bool IsHandlingTimeSwitch();

	// Called on cutscene started
	virtual void OnCutsceneStarted();

	// Called on cutscene ended
	virtual void OnCutsceneEnded();

	virtual void OnCutsceneDebugCheck();

	virtual void OnCinematicStorySceneStarted() override;
	virtual void OnCinematicStorySceneEnded() override;

	// Blend pelvis in world space - it can be useful after teleports etc.
	virtual void BlendPelvisWS( const Matrix& pointWS, Float blendTime ) {}

	RED_FORCE_INLINE Float GetOverrideBudgetedTickDistance() const { return m_overrideBudgetedTickDistance; }
	RED_FORCE_INLINE Float GetOverrideDisableTickDistance() const { return m_overrideDisableTickDistance; }

	// Hack
	void ForcePoseAndStuffDuringCook();

	void SuppressAnimation( Bool suppress, AnimationSuppressReason reason );
	RED_FORCE_INLINE Bool IsAnimationSuppressed( AnimationSuppressReason reason ) const { return ( m_animationSuppressionMask & reason ) != 0; }

	void MarkProcessPostponedEvents() { m_processPostponedEvents = true; }
	//////////////////////////////////////////////////////////////////////////
	// TO REFACTOR
public:
	//! Determine length of an animation
	Float GetAnimationDuration( const CName& animationName ) const;
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Editor
#ifndef NO_EDITOR
public:
	struct SetupData
	{
		CSkeleton*					m_skeleton;
		SBehaviorGraphInstanceSlot*	m_slot;
		CSkeletalAnimationSet*		m_set;
	};
	void Setup( SetupData& data );

	void DestroyRagdoll();

	void EditorOnTransformChangeStart();
	void EditorOnTransformChanged();
	void EditorOnTransformChangeStop();

	//! Choose whether we should apply constraints behavior graph
	void SetAllowConstraintsUpdate( Bool flag );
#endif

public:
	static const CAnimatedComponent* GetRootAnimatedComponentFromAttachment( const CComponent* component );

protected:
	void funcActivateBehaviors( CScriptStackFrame& stack, void* result );
	void funcAttachBehavior( CScriptStackFrame& stack, void* result );
	void funcDetachBehavior( CScriptStackFrame& stack, void* result );
	void funcSetBehaviorVariable( CScriptStackFrame& stack, void* result );
	void funcGetBehaviorVariable( CScriptStackFrame& stack, void* result );
	void funcSetBehaviorVectorVariable( CScriptStackFrame& stack, void* result );
	void funcGetBehaviorVectorVariable( CScriptStackFrame& stack, void* result );
	void funcDisplaySkeleton( CScriptStackFrame& stack, void* result );
	void funcGetAnimationTimeMultiplier( CScriptStackFrame& stack, void* result );
	void funcSetAnimationTimeMultiplier( CScriptStackFrame& stack, void* result );
	void funcGetMoveSpeedAbs( CScriptStackFrame& stack, void* result );
	void funcGetMoveDirWorldSpace( CScriptStackFrame& stack, void* result );
	void funcRaiseBehaviorEvent( CScriptStackFrame& stack, void* result );
	void funcRaiseBehaviorForceEvent( CScriptStackFrame& stack, void* result );
	void funcFindNearestBoneWS( CScriptStackFrame& stack, void* result );
	void funcFindNearestBoneToEdgeWS( CScriptStackFrame& stack, void* result );
    void funcGetCurrentBehaviorState( CScriptStackFrame& stack, void* result );
	void funcFreezePose( CScriptStackFrame& stack, void* result );
	void funcUnfreezePose( CScriptStackFrame& stack, void* result );
	void funcFreezePoseFaded( CScriptStackFrame& stack, void* result );
	void funcUnfreezePoseFaded( CScriptStackFrame& stack, void* result );
	void funcHasFrozenPose( CScriptStackFrame& stack, void* result );
	void funcSyncTo( CScriptStackFrame& stack, void* result );
	void funcUseExtractedMotion( CScriptStackFrame& stack, void* result );
	void funcSetUseExtractedMotion( CScriptStackFrame& stack, void* result );
	void funcStickRagdollToCapsule( CScriptStackFrame& stack, void* result );
	void funcHasRagdoll( CScriptStackFrame& stack, void* result );
	void funcGetRagdollBoneName( CScriptStackFrame& stack, void* result );
	void funcPlaySlotAnimationAsync( CScriptStackFrame& stack, void* result );
	void funcPlaySkeletalAnimationAsync( CScriptStackFrame& stack, void* result );
	void funcGetMoveSpeedRel( CScriptStackFrame& stack, void* result );
	void funcGetBoneMatrixMovementModelSpaceInAnimation( CScriptStackFrame& stack, void* result );
	void funcDontUpdateByOtherAnimatedComponent( CScriptStackFrame& stack, void* result );
	void funcUpdateByOtherAnimatedComponent( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CAnimatedComponent )
	PARENT_CLASS( CComponent );
	PROPERTY_EDIT( m_ragdoll, TXT("Ragdoll") );
	PROPERTY_CUSTOM_EDIT( m_ragdollCollisionType, TXT( "Defines what of types it is from physical collision point of view" ), TXT("PhysicalCollisionTypeSelector") );
	PROPERTY_EDIT( m_skeleton, TXT("Skeleton") );
	PROPERTY_INLINED( m_physicsRepresentation, TXT("Physics representation") );
	PROPERTY_EDIT( m_animationSets, TXT("Animations sets") );
	PROPERTY_EDIT( m_behaviorInstanceSlots, TXT("Behavior instance slots ( first slot is on the top )") );
	PROPERTY_EDIT( m_useExtractedMotion, TXT("Use motion extracted from animation to move this component") );
	PROPERTY_EDIT( m_stickRagdollToCapsule, TXT("Pull ragdoll back to capsule if too far away") );
	PROPERTY_EDIT( m_includedInAllAppearances, TXT("Should this animated component be implicitly included in all appearances?") );
	PROPERTY_EDIT( m_savable, TXT("Save component with behavior states") );
	PROPERTY_EDIT( m_defaultBehaviorAnimationSlotNode, TXT("Name of default behavior") );
	PROPERTY_EDIT( m_isFrozenOnStart, TXT("If component should be frozen on attaching") );
	PROPERTY_EDIT( m_defaultSpeedConfigKey,				TXT("Key to the speed config to link to ( see speedConfig.xml )") );
	PROPERTY_EDIT( m_overrideBudgetedTickDistance, TXT("Override for default (min) budgeted tick distance; 0 indicates to use default from LOD manager") );
	PROPERTY_EDIT( m_overrideDisableTickDistance, TXT("Override for default (min) disabled tick (i.e. no tick) distance; 0 indicates to use default from LOD manager") );
	PROPERTY( m_runtimeBehaviorInstanceSlots );
	
	NATIVE_FUNCTION( "UseExtractedMotion", funcUseExtractedMotion );
	NATIVE_FUNCTION( "SetUseExtractedMotion", funcSetUseExtractedMotion );
	NATIVE_FUNCTION( "StickRagdollToCapsule", funcStickRagdollToCapsule );	
	NATIVE_FUNCTION( "ActivateBehaviors", funcActivateBehaviors );
	NATIVE_FUNCTION( "AttachBehavior", funcAttachBehavior );
	NATIVE_FUNCTION( "DetachBehavior", funcDetachBehavior );
	NATIVE_FUNCTION( "SetBehaviorVariable", funcSetBehaviorVariable );
	NATIVE_FUNCTION( "GetBehaviorVariable", funcGetBehaviorVariable );
	NATIVE_FUNCTION( "SetBehaviorVectorVariable", funcSetBehaviorVectorVariable );
	NATIVE_FUNCTION( "GetBehaviorVectorVariable", funcGetBehaviorVectorVariable );
	NATIVE_FUNCTION( "DisplaySkeleton", funcDisplaySkeleton );
	NATIVE_FUNCTION( "GetAnimationTimeMultiplier", funcGetAnimationTimeMultiplier);
	NATIVE_FUNCTION( "SetAnimationTimeMultiplier", funcSetAnimationTimeMultiplier);
	NATIVE_FUNCTION( "GetMoveSpeedAbs", funcGetMoveSpeedAbs);
	NATIVE_FUNCTION( "GetMoveDirWorldSpace", funcGetMoveDirWorldSpace);
	NATIVE_FUNCTION( "RaiseBehaviorEvent", funcRaiseBehaviorEvent);
	NATIVE_FUNCTION( "RaiseBehaviorForceEvent", funcRaiseBehaviorForceEvent);
	NATIVE_FUNCTION( "FindNearestBoneWS", funcFindNearestBoneWS );
	NATIVE_FUNCTION( "FindNearestBoneToEdgeWS", funcFindNearestBoneToEdgeWS );
    NATIVE_FUNCTION( "GetCurrentBehaviorState", funcGetCurrentBehaviorState );
	NATIVE_FUNCTION( "FreezePose", funcFreezePose );
	NATIVE_FUNCTION( "UnfreezePose", funcUnfreezePose );
	NATIVE_FUNCTION( "FreezePoseFadeIn", funcFreezePoseFaded );
	NATIVE_FUNCTION( "UnfreezePoseFadeOut", funcUnfreezePoseFaded );
	NATIVE_FUNCTION( "HasFrozenPose", funcHasFrozenPose );
	NATIVE_FUNCTION( "SyncTo", funcSyncTo );
	NATIVE_FUNCTION( "HasRagdoll", funcHasRagdoll );
	NATIVE_FUNCTION( "GetRagdollBoneName", funcGetRagdollBoneName );
	NATIVE_FUNCTION( "PlaySlotAnimationAsync", funcPlaySlotAnimationAsync );
	NATIVE_FUNCTION( "PlaySkeletalAnimationAsync", funcPlaySkeletalAnimationAsync );
	NATIVE_FUNCTION( "GetMoveSpeedRel", funcGetMoveSpeedRel );
	NATIVE_FUNCTION( "GetBoneMatrixMovementModelSpaceInAnimation", funcGetBoneMatrixMovementModelSpaceInAnimation );
	NATIVE_FUNCTION( "DontUpdateByOtherAnimatedComponent", funcDontUpdateByOtherAnimatedComponent );
	NATIVE_FUNCTION( "UpdateByOtherAnimatedComponent", funcUpdateByOtherAnimatedComponent );
END_CLASS_RTTI();
