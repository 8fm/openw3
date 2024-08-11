/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "actorActionMoveTo.h"
#include "moveLocomotion.h"
#include "movableRepresentationsStack.h"
#include "movableRepresentationEntity.h"
#include "../engine/animatedComponent.h"
#include "../engine/traceTool.h"

class CMoveSteeringBehavior;
enum ENavigationStatus : CEnum::TValueType;
class ISteeringBehaviorListener;
class IMovableRepresentation;
class CStaticMovementTargeter;
class CPathPointLock;
class CPathAgent;
class CMovementAdjustor;

///////////////////////////////////////////////////////////////////////////////

#define MAX_AGENT_RADIUS				3.f

///////////////////////////////////////////////////////////////////////////////

enum EMovementFlags
{
	MM_CombatMode				= FLAG( 0 ),
	MM_MoveSliding				= FLAG( 1 ),
};

BEGIN_BITFIELD_RTTI( EMovementFlags, 1 );
	BITFIELD_OPTION( MM_CombatMode );
	BITFIELD_OPTION( MM_MoveSliding );
END_BITFIELD_RTTI();

struct SPlacementTrace
{
	TraceResultPlacement	m_traceResult;
	Vector					m_tracePoint;

	SPlacementTrace()
		: m_tracePoint( NumericLimits< Float >::Max(), NumericLimits< Float >::Max(), NumericLimits< Float >::Max() )
	{

	}

	RED_INLINE Bool DoTest( const Vector& point )
	{
		if ( !Vector::Near3( m_tracePoint, point ) || !GGame->GetGameplayConfig().m_movementTraceOpt )
		{
			m_tracePoint = point;
			return CTraceTool::StaticAgentPlacementTraceTest( GGame->GetActiveWorld(), point, MAC_TRACE_TEST_RADIUS, m_traceResult );			
		}
		return true;
	}

	RED_INLINE Bool WasLastTestDifferent() const { return abs( m_tracePoint.Z - m_traceResult.m_height ) > 0.05f; }
};

///////////////////////////////////////////////////////////////////////////////

class CMovingAgentComponent;

///////////////////////////////////////////////////////////////////////////////

struct SAnimationProxyStoredTransformData
{
	CName m_storeName;
	AnimQsTransform m_transform;
};

///////////////////////////////////////////////////////////////////////////////

struct SAnimationProxyStoredSynchronizationData
{
	CName m_storeName;
	CSyncInfo m_syncInfo;
	Int32 m_framesNotActive;
};

///////////////////////////////////////////////////////////////////////////////

struct SAnimationProxyData
{
	DECLARE_RTTI_STRUCT( SAnimationProxyData );

private:
	CMovingAgentComponent* m_owner;

private:
	Float	m_useIKWeight;
	Float	m_useIKBlendTime;

private:
	Vector	m_appliedIKOffsetWS;

private:
	Bool	m_useHandsIK;
	Float	m_handsIKOffsetL;
	Float	m_handsIKOffsetR;

private:	// pelvis offset - if animations are done to match, there might be problem when constrains kick in
			// that's why transforms in model space are stored for entity used for pelvis offset - this is only stored if it is required
	THandle<CEntity> m_useEntityForPelvisOffset;
	Int32 m_pelvisBoneIdx; // has to be set to allow for bone (pelvis) to be stored
	Bool m_pelvisBoneDataValid;
	Int32 m_pelvisBoneRequestedCount;
	AnimQsTransform m_preConstraintsPelvisBoneMS;
	AnimQsTransform m_postConstraintsPelvisBoneMS;

private:	// pelvis offset WS - pelvis's transform needs to be corrected after some actions like teleport or smooth transition from cutscene to gameplay
	Bool	m_requestToApplyPelvisCorrection;
	Matrix	m_requestToApplyPelvisCorrectionTransformWS;
	Float	m_requestToApplyPelvisCorrectionDuration;

private:	// additional offset for whole character (applied on root) used to blend when attaching
	Vector m_additionalOffsetLocationMS;
	EulerAngles m_additionalOffsetRotationMS;
	Float m_additionalOffsetTimeLeft;

private:
	static const Uint32 NUM_STORED_TRANSFORMS = 8;
	SAnimationProxyStoredTransformData m_storedTransforms[NUM_STORED_TRANSFORMS];
	Uint32 m_numStoredTransforms;

private:
	static const Uint32 NUM_STORED_SYNCHRONIZATIONS = 4;
	SAnimationProxyStoredSynchronizationData m_storedSynchronizations[NUM_STORED_SYNCHRONIZATIONS];
	Uint32 m_numStoredSynchronizations;

private:
	static const Uint32 NUM_STORED_ANIM_EVENTS = 1;
	CAnimationEventFired m_storedAnimEvents[NUM_STORED_ANIM_EVENTS];
	Uint32 m_numStoredAnimEvents;


	Bool	m_useIK:1; // target for using weight
	Bool	m_slidingOnSlopeIK:1;
	Bool	m_inAnimatedRagdoll:1;
	Bool	m_enabledIKDueToLOD:1;
	Bool	m_enabledIKDueToWork:1;
	Bool	m_HACK_justTeleported:1;  // used for clear some stuff on IK due to teleport, this flag is cleared by behavior graph when processed [Notes]: we need a one frame state concept here!
	Bool	m_extendedIKOffset:1;


public:
	SAnimationProxyData()
	: m_owner( nullptr )
	, m_useIK( true )
	, m_useHandsIK( false )
	, m_slidingOnSlopeIK( false )
	, m_inAnimatedRagdoll( false )
	, m_enabledIKDueToLOD( true )
	, m_enabledIKDueToWork( true )
	, m_HACK_justTeleported( false )
	, m_useIKWeight( 1.0f )
	, m_useIKBlendTime( 0.0f )
	, m_appliedIKOffsetWS( Vector::ZEROS )
	, m_useEntityForPelvisOffset( nullptr )
	, m_pelvisBoneIdx( -1 )
	, m_pelvisBoneDataValid( false )
	, m_pelvisBoneRequestedCount( 0 )
	, m_additionalOffsetTimeLeft( 0.0f )
	, m_requestToApplyPelvisCorrection( false )
	, m_requestToApplyPelvisCorrectionTransformWS( Matrix::IDENTITY )
	, m_requestToApplyPelvisCorrectionDuration( 0.f )
	, m_numStoredTransforms( 0 )
	, m_numStoredSynchronizations( 0 )
	, m_numStoredAnimEvents( 0 )
	{		
	}

	void Update( Float timeDelta );

	RED_INLINE void SetOwner( CMovingAgentComponent* owner ) { m_owner = owner; }

	RED_INLINE Bool GetUseIK() const { return m_useIK; }
	RED_INLINE void SetUseIK( Bool useIK, Float blendTime ) { m_useIK = useIK; m_useIKBlendTime = blendTime; }
	RED_INLINE void ForceUseIK( Bool useIK ) { m_useIK = useIK; m_useIKWeight = m_useIK ? 1.f : 0.f; }
	RED_INLINE Float GetUseIKWeight() const { return m_useIKWeight; }

	RED_INLINE void HACK_SetJustTeleportedFlag( Bool setFlag = true ) { m_HACK_justTeleported = setFlag; }
	RED_INLINE Bool HACK_GetJustTeleportedFlag() const { return m_HACK_justTeleported; }

	RED_INLINE void SetUseHandsIK( Bool use ) { m_useHandsIK = use; }
	RED_INLINE void SetHandsIKOffsets( Float leftOffset, Float rightOffset ) { m_handsIKOffsetL = leftOffset;	m_handsIKOffsetR = rightOffset; }

	RED_INLINE Bool GetUseHandsIK() const { return m_useHandsIK; }
	RED_INLINE void GetHandsIKOffsets( Float & leftOffset, Float & rightOffset ) const { leftOffset = m_handsIKOffsetL; rightOffset = m_handsIKOffsetR; }

	RED_INLINE void EnableIKDueToLOD( Bool enable ) { m_enabledIKDueToLOD = enable; }
	RED_INLINE Bool IsIKEnabledDueToLOD() const { return m_enabledIKDueToLOD; }
	RED_INLINE void EnableIKDueToWork( Bool enable ) { m_enabledIKDueToWork = enable; }

	RED_INLINE Bool UseExtendedIKOffset() const { return m_extendedIKOffset; }
	RED_INLINE void SetUseExtendedIKOffset( Bool enable ) { m_extendedIKOffset = enable; }
	
	RED_INLINE Bool GetSlidingOnSlopeIK() const { return m_slidingOnSlopeIK; }
	RED_INLINE void SetSlidingOnSlopeIK( Bool slidingOnSlopeIK ) { m_slidingOnSlopeIK = slidingOnSlopeIK; }

	RED_INLINE Bool GetInAnimatedRagdoll() const { return m_inAnimatedRagdoll; }
	RED_INLINE void SetInAnimatedRagdoll( Bool inAnimatedRagdoll ) { m_inAnimatedRagdoll = inAnimatedRagdoll; }

	RED_INLINE CEntity* GetUseEntityForPelvisOffset() const { return m_useEntityForPelvisOffset.Get(); }
	void SetUseEntityForPelvisOffset( CEntity * entity = nullptr );

	RED_INLINE void SetPelvisBoneIndex( Int32 _idx ) { m_pelvisBoneIdx = _idx; }

	RED_INLINE void ProvidePelvisBoneData() { ++ m_pelvisBoneRequestedCount; }
	RED_INLINE void DontProvidePelvisBoneData() { -- m_pelvisBoneRequestedCount; }
	RED_INLINE Bool IsPelvisBoneDataValid() const { return m_pelvisBoneDataValid; }
	RED_INLINE AnimQsTransform const & GetPreConstraintsPelvisBoneMS() const { return m_preConstraintsPelvisBoneMS; }
	RED_INLINE AnimQsTransform const & GetPostConstraintsPelvisBoneMS() const { return m_postConstraintsPelvisBoneMS; }

	RED_INLINE void SetAdditionalOffsetMS( Vector const & offsetLocMS, EulerAngles const & offsetRotMS, Float time ) { m_additionalOffsetLocationMS = offsetLocMS; m_additionalOffsetRotationMS = offsetRotMS; m_additionalOffsetTimeLeft = time; }
	void SetAdditionalOffsetMS( CEntity* attachingTo, Float time );
	void AddAdditionalOffsetMS( Vector const & offsetLocMS, EulerAngles const & offsetRotMS, Float time );
	void SetAdditionalOffsetMSPointWS( const Matrix& pointWS, Float time );

	RED_INLINE Vector const & GetAdditionalOffsetLocationMS() const { return m_additionalOffsetLocationMS; }
	RED_INLINE EulerAngles const & GetAdditionalOffsetRotationMS() const { return m_additionalOffsetRotationMS; }
	RED_INLINE Float GetAdditionalOffsetTimeLeft() const { return m_additionalOffsetTimeLeft; }

	RED_INLINE void SetRequestToApplyPelvisCorrectionWS( const Matrix& pelvisWS, Float duration ) { m_requestToApplyPelvisCorrection = true; m_requestToApplyPelvisCorrectionTransformWS = pelvisWS; m_requestToApplyPelvisCorrectionDuration = duration; }
	RED_INLINE void ResetRequestToApplyPelvisCorrectionWS() { m_requestToApplyPelvisCorrection = false; m_requestToApplyPelvisCorrectionTransformWS = Matrix::IDENTITY; m_requestToApplyPelvisCorrectionDuration = 0.f; }
	RED_INLINE Bool HasRequestToApplyPelvisCorrectionWS( Matrix& pelvisWS, Float& duration ) const { pelvisWS = m_requestToApplyPelvisCorrectionTransformWS; duration = m_requestToApplyPelvisCorrectionDuration; return m_requestToApplyPelvisCorrection; }

	RED_INLINE Vector const & GetAppliedIKOffsetWS() const { return m_appliedIKOffsetWS; }
	RED_INLINE void SetAppliedIKOffsetWS( Vector const & offset ) { m_appliedIKOffsetWS = offset; }

	void OnPreProcessPoseConstraints( CAnimatedComponent * ac, SBehaviorGraphOutput& output );
	void OnPostProcessPoseConstraints( CAnimatedComponent * ac, SBehaviorGraphOutput& output );

	void StoreTransform( CName const & storeName, AnimQsTransform const & transform );
	Bool GetTransform( CName const & storeName, AnimQsTransform & outTransform ) const;

	void StoreSynchronization( CName const & storeName, CSyncInfo const & syncInfo );
	Bool GetSynchronization( CName const & storeName, CSyncInfo & outSyncInfo ) const;

	void StoreAnimEvent( CAnimationEventFired const & animEvt );
	Bool GetAnimEvent( CName const & storeName, CAnimationEventFired & animEvt ) const;
};

BEGIN_CLASS_RTTI( SAnimationProxyData )
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

/// Component that supports navigated movement thought the world
class CMovingAgentComponent : public CAnimatedComponent
{
	DECLARE_ENGINE_CLASS( CMovingAgentComponent, CAnimatedComponent, 0 );

private:
	enum EMoveRotationType
	{
		MRT_None,
		MRT_Snap,
		MRT_Clamped
	};

protected:
	CPathAgent*								m_pathAgent;
	THandle< CMoveSteeringBehavior >		m_steeringBehavior;					//!< Steering behavior resource
	CMoveLocomotion*						m_locomotion;						//!< locomotion used to move the agent around
	CMovementAdjustor*						m_movementAdjustor;					//!< adjustor for movement
	
	CStaticMovementTargeter*				m_staticTarget;						//!< Static movement targeter
	ISteeringBehaviorListener*				m_steeringListener;					//!< Steering behavior listener

	CMREntity*								m_meshRepresentation;
	IMovableRepresentation*					m_ragdollRepresentation;
	CMRStack*								m_representationStack;

	EMoveType								m_moveType;
	Uint8									m_movementFlags;

	// Trigger system
	Bool									m_triggerAutoActivator;				//!< Automatically create trigger activator (mimics old system)
	Bool									m_triggerEnableCCD;					//!< Use contignous collision detection (much slower)
	Float									m_triggerActivatorRadius;			//!< Radius of trigger activator
	Float									m_triggerActivatorHeight;			//!< Radius of trigger activator
	ITriggerActivator*						m_triggerActivator;					//!< Activator for trigger system
	Uint32									m_triggerChannels;					//!< Channels on which we do the activation
	Float									m_triggerMaxSingleFrameDistance;	//!< Maximum distance traveled in one frame that can be considered "contignous"

	// Dynamics characteristics
	Float									m_maxSpeed;
	Float									m_allowExceedingMaxSpeedCoef;		//!< Allow exceeding max speed when requesting speed
	Float									m_maxRotation;
	Float									m_directionChangeRate;
	Vector									m_velocity;							//!< Actual velocity, based on how character has moved
	Vector									m_velocityBasedOnRequestedMovement;	//!< Velocity based on requested movement (extracted motion modified by slider, movement adjustor, etc.)
	Vector									m_deltaPositionFromBehaviorGraph; //!< Base for velocity above
	SPlacementTrace							m_placementTrace;

	// MOVEMENT DATA SINK
	Vector									m_teleportToPos;
	EulerAngles								m_teleportToRot;
	Bool									m_teleportCorrectZ;

	Vector									m_slideDir;							//!< The desired slide direction vector
	EulerAngles								m_slideRotation;					//!< The desired slide rotation
	Float									m_slideSpeed;						//!< Desired slide speed

	Float									m_desiredDirection;

	EMoveRotationType						m_rotationSet;
	Float									m_desiredRotation;

	Float									m_desiredAbsoluteSpeed;
	Float									m_gameplayRelativeMoveSpeed;
	Float									m_gameplayMoveDirection;
	Float									m_acceleration;
	Float									m_deceleration;
	Float									m_currentSpeedVal;

	Float									m_lastRelMovementSpeed;

	Float									m_rotationTargetCooldown;			//!< Cooldown for the rotation target

	Vector									m_prevPosition;

	// MOVEMENT DATA OUTPUT
	Vector									m_deltaPosition;
	EulerAngles								m_deltaRotation;

	// RL for movement
	InstanceBuffer*							m_steeringRuntimeData;

	CMoveSteeringBehavior*					m_currentSteeringBehavior;
	InstanceBuffer*							m_currentSteeringRuntimeData;

	const CMovingAgentComponent*			m_wasSeparatedBy;

	EngineTime								m_pullToNavSpaceTimeLimit;

	// Collision data
	Vector									m_collisionLine;
	Float									m_ragdollRadius;					//!< Path engine agent radius for ragdoll
	Float									m_baseRadius;
	Float									m_avoidanceRadius;

	Vector									m_lastNavigableAreaPosition;

	TDynArray<const SPhysicalMaterial*>		m_forcedStandPhysicalMaterials;

	Uint16									m_isDisabled;
	Uint16									m_isEntityRepresentationForced;
	Uint16									m_physicalRepresentationEnableRequest;
	Uint16									m_physicalRepresentationDisableRequest;

	Bool									m_pullToNavigableSpace;
	Bool									m_snapToNavigableSpace;
	Bool									m_steeringControlledMovement;
	Bool									m_physicalRepresentation;

	// State flags
	Bool									m_characterCollisionsEnabled	: 1;
	Bool									m_isCollidable					: 1;//!< Collides with other agents?
	Bool									m_teleported					: 1;
	Bool									m_moved							: 1;
	Bool									m_enableAfterTeleport			: 1;
	Bool									m_isColliding					: 1;
	Bool									m_slide							: 1; //!< Tells if we should ignore the motion extraction data and just slide the agent
	Bool									m_slideSpeedDefined				: 1; //!< Is the slide speed defined
	Bool									m_directionSet					: 1;
	Bool									m_gameplayDirectionSet			: 1;
	Bool									m_ragdollObstacle				: 1; //!< True if generate obstacle for ragdoll 
	Bool									m_representationResetRequest	: 1;
	Uint32																	: 0;	//!< Padding

	SAnimationProxyData						m_animationProxyData;

public:
	CMovingAgentComponent();
	virtual ~CMovingAgentComponent();

	enum ELockSource
	{
		LS_Initial		= FLAG(0),
		LS_Default		= FLAG(1),
		LS_JobTree		= FLAG(2),
		LS_ScenePlayer	= FLAG(3),
		LS_CActor		= FLAG(4),
		LS_Scripts		= FLAG(5),
		LS_AI			= FLAG(6),
		LS_GUI			= FLAG(7),
		LS_LOD			= FLAG(8),
		LS_Steering		= FLAG(9),
		// ....
		LS_Force		= FLAG(15)
	};

	enum ERequest
	{
		Req_Off = 0,
		Req_On,
		Req_Toggle,
		Req_Ignore
	};

	RED_INLINE EMoveType GetMovementType()	{ return m_moveType; }
	RED_INLINE Float GetMovementSpeed()		{ return m_maxSpeed; }

	// -------------------------------------------------------------------------
	// Component management
	// -------------------------------------------------------------------------
	//! Attached to world
	virtual void OnAttached( CWorld* world );

	//! Detached from world
	virtual void OnDetached( CWorld* world );

	//! Serialization support
	virtual void OnSerialize( IFile& file );

	//! Called when the value of a property changes
	virtual void OnPropertyPostChange( IProperty* property );

	// Generate editor rendering fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags );

	// Toggle enabledz
	virtual void SetACMotionEnabled( Bool enabled );
	virtual Bool IsACMotionEnabled() const;

	void SetMotionEnabled( Bool enabled , ELockSource lockSource = LS_Default );
	RED_FORCE_INLINE Bool IsMotionEnabled() const { return m_isDisabled == 0; }
	RED_FORCE_INLINE Bool IsMotionEnabled( ELockSource lockSource ) const { return !( m_isDisabled & lockSource ); }
	
	void ForceEntityRepresentation( Bool force, ELockSource lockSource = LS_Default );
	RED_INLINE Bool IsEntityRepresentationForced() const { return m_isEntityRepresentationForced != 0; }
	RED_INLINE Bool IsEntityRepresentationForced( Uint16 reasons ) const { return (m_isEntityRepresentationForced & reasons) != 0; }

	// allows to set both enable and (optionally) disable requests - the one with greater ELockSource value "wins"
	void SetPhysicalRepresentationRequest( ERequest enableRequest, ELockSource enableSource, ERequest disableRequest = Req_Ignore, ELockSource disableSource = LS_Default );
	Bool IsPhysicalRepresentationRequested() const;
	Bool IsPhysicalRepresentationEnabled() const;

	// Low level interface meant for debug usage
	Uint16 GetMotionDisableFlags() const { return m_isDisabled; }
	Uint16 GetEntityRepresentationForceFlags() const { return m_isEntityRepresentationForced; }
	Uint16 GetPhysicalRepresentationEnableFlags() const { return m_physicalRepresentationEnableRequest; }
	Uint16 GetPhysicalRepresentationDisableFlags() const { return m_physicalRepresentationDisableRequest; }

	//! Component was destroyed
	virtual void OnDestroyed();

	//! Should agent snap to navigable space
	void SnapToNavigableSpace( Bool b );

	virtual Bool IsSnapedToNavigableSpace() const { return m_snapToNavigableSpace; }

	//! Should agent pull to navigable space if outside
	void PullToNavigableSpace();
	void StopPullingToNavigableSpace();

	Bool IsPulledToNavigableSpace();
	
	//! Returns the steering behavior resource used by this agent
	RED_INLINE CMoveSteeringBehavior* GetSteeringBehavior() { return m_steeringBehavior.Get(); }

	//! Sets external steering behavior
	void SetCustomSteeringBehavior( CMoveSteeringBehavior* steeringGraph, InstanceBuffer* runtimeData );

	//! Clears external steering behavior
	void ClearCustomSteeringBehavior();

	//! Access currently used steering graph
	RED_INLINE CMoveSteeringBehavior* GetCurrentSteeringBehavior() const { return m_currentSteeringBehavior; }

	//! Access current starring graph instance buffer
	RED_INLINE InstanceBuffer* GetCurrentSteeringRuntimeData() const { return m_currentSteeringRuntimeData; }

	//! Access movement adjustor
	RED_INLINE CMovementAdjustor* GetMovementAdjustor() const { ASSERT( m_movementAdjustor ); return m_movementAdjustor; }

	// Is ragdoll obstacle enabled ?
	RED_INLINE Bool IsRagdollObstacle() const { return m_ragdollObstacle; }

	// Find ragdoll position in path engine world
	Bool GetRagdollPosition(Vector& position) const;

	// check if ragdolled (might be animated ragdoll too)
	virtual Bool IsRagdolled( Bool realRagdoll ) const { return TBaseClass::IsRagdolled( realRagdoll ) || ( ! realRagdoll && m_animationProxyData.GetInAnimatedRagdoll() ); }

	// Tells if the agent can collide with other agents
	RED_INLINE Bool IsCollidable() const { return m_isCollidable; }

	// Sets a flag that tells whether the agent can collide with other agents
	RED_INLINE void SetCollidable( Bool enable ) { m_isCollidable = enable; }

	// Tells if the two agents can slide along each other
	Bool CanSlideAlong( CMovingAgentComponent& agent ) const;

	// Predict position of agent in the world in given time
	Vector PredictWorldPosition( Float inTime ) const;

	// -------------------------------------------------------------------------
	// Methods that tell when to perform movement calculations
	// -------------------------------------------------------------------------
	//! Split animation updates, drive movement behavior in a multi-threaded movement mode
	virtual void InternalUpdateAndSampleMultiAsyncPart( Float timeDelta );
	virtual void InternalUpdateAndSampleMultiSyncPart( Float timeDelta );

	virtual Bool CanUseAsyncUpdateMode() const;

	//! Pre physics tick, drives movement behavior in a single-threaded movement mode
	virtual void OnTickPrePhysics( Float timeDelta );

	virtual void OnTickPostUpdateTransform( Float timeDelta );

	// -------------------------------------------------------------------------
	// Locomotion controller management
	// -------------------------------------------------------------------------

	// Attaches a new locomotion controller
	void AttachLocomotionController( CMoveLocomotion::IController& controller );

	// Detaches a locomotion controller
	void DetachLocomotionController( CMoveLocomotion::IController& controller );

	// Checks if the current movement can be canceled.
	Bool CanCancelMovement() const;

	// Cancels current movement
	void CancelMove();

	// -------------------------------------------------------------------------
	// Movement params management
	// -------------------------------------------------------------------------
	//! Get last trace result
	RED_INLINE TraceResultPlacement GetLastPlacementTrace() const { return m_placementTrace.m_traceResult; }

	//! Checks if last trace resulted in difference on Z axis
	RED_INLINE Bool WasLastTraceDifferentThanPosition() const { return m_placementTrace.WasLastTestDifferent(); }

	//! Get PathLib agent related to this moving component
	RED_INLINE CPathAgent* GetPathAgent() const { return m_pathAgent; }

	//! Get Locomotion instance
	RED_INLINE CMoveLocomotion *const GetLocomotion()const{ return m_locomotion; }

	//! Teleport to position ( with optional rotation ). Will cancel movement.
	virtual void Teleport( const Vector& position, const EulerAngles* rotation = NULL );

	//! Tells if a rotation target is set
	Bool IsRotationTargetEnabled() const;

	//! Tells if the agent is in the combat movement mode
	RED_INLINE Bool IsInCombatMode() const { return 0 != ( m_movementFlags & MM_CombatMode ); }

	// -------------------------------------------------------------------------
	// Static targeters
	// -------------------------------------------------------------------------
	
	//! Clear rotation target
	void ClearRotationTarget();

	//! Sets rotation target to position
	void SetRotationTarget( const Vector& position, Bool clamping );

	//! Sets rotation target to entity
	void SetRotationTarget( const THandle< CNode >& node, Bool clamping );

	//! Halt rotation target for a while
	void RelaxRotationTarget( Float cooldown );

	// -------------------------------------------------------------------------
	// CMoveLocomotion interface
	// -------------------------------------------------------------------------
	
	//! Makes the agent use the realtime data that can be used to store debug info
	void EnableRuntimeData( Bool enable );

	//! Deposits a movement delta that will be applied this frame ( motion extraction data sink )
	void AddDeltaMovement( const Vector& deltaPosition, const EulerAngles& deltaRotation );

	//! Returns the currently used RL data buffer
	RED_INLINE InstanceBuffer& AccessRuntimeSteeringData() const { return *m_steeringRuntimeData; }

	//! Changes the movement direction speed
	RED_INLINE void SetDirectionChangeRate( Float rate ) { m_directionChangeRate = ::Max( 0.0f, rate ); }

	//! Requests a new movement direction
	RED_INLINE void RequestMoveDirection( Float moveDirection ) { m_desiredDirection = moveDirection; m_directionSet = true; }

	//! Requests a new agent rotation
	RED_INLINE void RequestMoveRotation( Float rotation, Bool clamped ) { m_desiredRotation = rotation; m_rotationSet = clamped ? MRT_Clamped : MRT_Snap; }

	//! Resets the movement requests
	RED_INLINE void ResetMoveRequests() { m_directionSet = false; m_rotationSet = MRT_None; m_desiredAbsoluteSpeed = 0.0f; }

	//! Requests a new absolute move speed
	void RequestAbsoluteMoveSpeed( Float val );

	//! Requests a new relative move speed
	void RequestRelativeMoveSpeed( Float val );

	//! This is not a request set move speed immediately !
	void ForceSetRelativeMoveSpeed( Float relativeMoveSpeed ) override;

	//! Slides the agent with the specified direction and rotation within the specified time
	void Slide( const Vector& direction, const EulerAngles& rotation, Float* speed = NULL );

	//! Teleports the agent to the specified position ( no movement is canceled )
	Bool TeleportTo( const Vector& newPosition, const EulerAngles& newRotation, const Bool correctZ = true );

	//! Returns absolute speed for the specified movement type
	Float GetSpeedForMoveType( EMoveType type, Float absSpeed ) const;

	//! Sets type of maximum move speed
	void SetMoveType( EMoveType speed, Float absSpeed = 2.0f );

	//! Gets the set movement type
	RED_INLINE void GetMoveType( EMoveType& outMoveType, Float& outAbsSpeed ) { outMoveType = m_moveType; outAbsSpeed = m_maxSpeed; }

	//! Changes the values of acceleration and deceleration of the agent
	RED_INLINE void SetMaxAcceleration( Float acceleration, Float deceleration ) { m_acceleration = acceleration; m_deceleration = deceleration; }

	//! Sets acceleration and deceleration the infinite vals
	RED_INLINE void ResetMaxAcceleration(){ m_acceleration = FLT_MAX; m_deceleration = FLT_MAX; }

	//! Returns the maximum speed of the agent
	RED_INLINE Float GetMaxSpeed() const { return m_maxSpeed; }

	//! Sets maximum move rotation speed [deg/s]
	RED_INLINE void SetMaxRotation( Float val ) { m_maxRotation = val; }

	//! Returns the definition of how fast an agent can rotate
	RED_INLINE Float GetMaxRotation() const { return m_maxRotation; }

	//! Returns the desired velocity of the agent
	Vector2 GetSteeringVelocity() const;

	// Returns the global position 
	Vector GetAgentPosition() const;

	// Returns agent previous position 
	const Vector& GetAgentPrevPosition() const { return m_prevPosition; }

	// Returns the current velocity (vector) of the agent
	RED_INLINE const Vector& GetVelocity() const { return m_velocity; }

	// Returns the velocity (vector) of the agent as it was a result from delta movement
	RED_INLINE const Vector& GetVelocityBasedOnRequestedMovement() const { return m_velocityBasedOnRequestedMovement; }

	// Returns if the component has moved since beginning of the frame
	Bool HasMoved() const { return m_moved; }

	// Returns if the component has separated since beginning of the frame
	const CMovingAgentComponent* WasSeparatedBy() const { return m_wasSeparatedBy; }

	// Enables/disables character-character separation
	void EnableCharacterCollisions( Bool enable ) { m_characterCollisionsEnabled = enable; }

	// Is separation on
	Bool IsCharacterCollisionsEnabled() const { return m_characterCollisionsEnabled; }
private:
	// Adjusts requested movement direction (basing on navmesh)
	Bool AdjustRequestedMovementDirectionNavMesh( Vector& directionWS, Float speed, Float maxAngle, Int32 maxIteration, Int32 maxIterationStartSide, Vector& preferedDirection, Bool acceptExploration );
	// Adjusts requested movement direction (basing on surrounding environment) also suggest whether character should stop, angleToDeflect = 0 is parallel to wall
	bool AdjustRequestedMovementDirectionPhysics( Vector& directionWS, Bool & shouldStop, Float speed, Float angleToDeflect, Float freeSideDistanceRequired, Bool & cornerDetected, Bool & isPortal );
	bool CheckIfAvoidDirectionIsFree( Vector& originalDirection, Vector& direction, Float freeSideDistanceRequired, Bool& isPortal, Float height  );
	bool CheckIfAvoidInDirectionHasPortal( Vector& originalDirection, Vector& direction, Vector& collisionPoint, Vector& collisionNormal  );
	Bool CheckLineCollisionNavmeshInFan( Vector& pointStart, Vector& direction, Float angleStart, Float angleStep, int steps, Float distance, Float collisionRadius, Vector& resultingDirection, Bool simetric, Bool acceptExploration );
	Bool CheckLineCollisionNavmeshWithYaw( Vector& pointStart, Vector& direction, Float angle, Float distance, Float collisionRadius, Vector& resultingDirection, Bool acceptExploration );
	Bool CheckLineCollisionNavmesh( Vector& pointStart, Vector& pointEnd, Float collisionRadius, Bool acceptExploration );
	Bool CheckLineCollisionsConvenient( const Vector& pointStart, const Vector& pointEnd, Float radius, Vector& collisionPoint, Vector& collisionNormal );
	
	//! Attempts to select an appropriate road and stick to it
	Bool StartRoadFollowing( Float speed, Float maxAngle, Float maxDistance, Vector& outDirection );
	void ResetRoadFollowing();

public:
	// Returns the delta movement from behavior graph
	RED_INLINE const Vector& GetDeltaPositionFromBehaviorGraph() const { return m_deltaPositionFromBehaviorGraph; }

	// Returns agent's heading
	RED_INLINE Vector GetHeading() const { return !m_velocity.AsVector3().IsAlmostZero() ? m_velocity.Normalized3() : GetWorldForward(); }

	//! Modifies the final goal for a steering segment
	void UpdateGoal( SMoveLocomotionGoal& goal, Float timeDelta ) const;

	//! Checks if we can move in a straight line to the destination without bumping into anything
	Bool CanGoStraightToDestination( const Vector& pos ) const;

	//! Test navmesh in straight line
	Bool IsEndOfLinePositionValid( const Vector& destination ) const;

	//! Calculate path and get its length
	Bool CalculatePathLength( const Vector& startPos, const Vector& stopPos, Float maxDistance, Float& outLength ) const;

	//! Get first collision along the line to the specified destination
	Bool GetFirstCollision( const Vector& destination, Vector& lineDir ) const;

	//! Returns the movement mode flags defined for this agent
	RED_INLINE Uint8 GetMovementFlags() const { return m_movementFlags; }

	//! Replaces the movement flags setting with a new one
	RED_INLINE void SetMovementFlags( Uint8 flags ) { m_movementFlags = flags; }

	//! Toggles a movement flag on/off
	void ToggleMovementFlag( Uint8 flag, Bool enable );

	//! Checks if the specified movement flag is set
	Bool IsMovementFlagSet( Uint8 flag ) const;

	//! Attaches/detaches a steering behavior listener
	RED_INLINE void SetSteeringBehaviorListener( ISteeringBehaviorListener* listener ) { m_steeringListener = listener; }

	//! Returns the currently attached instance of steering behavior listener
	RED_INLINE ISteeringBehaviorListener* GetSteeringBehaviorListener() { return m_steeringListener; }

	//! Blackboard method for retrieving the las selected agent movement speed
	RED_INLINE Float GetLastMovementSpeed() const { return m_lastRelMovementSpeed; }

	//! Performs a trace to the nearest surface the position can be snapped to with respect to height.
	void SnapToMesh( Vector& position );

	//! Returns last valid navigable position
	const Vector& GetLastNavigableSpot() const { return m_lastNavigableAreaPosition; }

	//! Calc slope angle
	Float CalcSlopeAngle() const;

	virtual Float GetRadius() const override;

	void ForceStandPhysicalMaterial( const SPhysicalMaterial* forcedStandPhysicalMaterial );
	void ReleaseStandPhysicalMaterial( const SPhysicalMaterial* forcedStandPhysicalMaterial );
	virtual const SPhysicalMaterial* GetCurrentStandPhysicalMaterial() const;

	Float GetDesiredMoveDirectionWorldSpace() const override;

	// If there is an unprocessed request for teleport, returns rotation that was requested in teleport, otherwise return rotation from entity.
	const EulerAngles& GetTeleportedRotationOrWSRotation() const;

	Bool IsTeleported() const { return m_teleported; }

	virtual void OnCutsceneDebugCheck() override;
	virtual void OnCutsceneStarted() override;
	virtual void OnCutsceneEnded() override;

	// ------------------------------------------------------------------------
	void OnDynamicCollision( CMovingAgentComponent* pushedAgent );

	// ------------------------------------------------------------------------
	// DEBUG stuff
	// ------------------------------------------------------------------------
	RED_INLINE const Vector& GetTeleportedPosition() const { return m_teleportToPos; }
	RED_INLINE const EulerAngles& GetTeleportedRotation() const { return m_teleportToRot; }
	RED_INLINE CName GetActiveRepresentationName() const { return m_representationStack->GetActiveRepresentation()->GetName(); }
	RED_INLINE void GetLocoDebugInfo( TDynArray< String >& debugLines ) const { if( m_locomotion != nullptr ) m_locomotion->GenerateDebugPage( debugLines ); }
	RED_INLINE const CStaticMovementTargeter* GetStaticTarget() const { return m_staticTarget; }
	RED_INLINE CMRStack* GetRepresentationStack() const { return m_representationStack; }

protected:
	//! Triggers the movement processing
    virtual void UpdateMovement1_PreSeperation( Float timeDelta ) override;
    virtual Bool UpdateMovement2_PostSeperation( Float timeDelta ) override;

	virtual void ProcessPoseConstraints( Float timeDelta, SBehaviorGraphOutput& output );
	virtual void PostProcessPoseConstraints( SBehaviorGraphOutput& output );

protected:
	// custom on-attached code meant to create movable representations
	virtual void CreateCustomMoveRepresentations( CWorld* world );

	//! Adds a new movable representation
	void AddRepresentation( IMovableRepresentation* representation, Bool activeInBackground );

	//! Removes a movable representation
	void RemoveRepresentation( IMovableRepresentation* representation );

	// -------------------------------------------------------------------------
	// Helper methods
	// -------------------------------------------------------------------------
public:

	//! Avoidance radius returns the largest proper radius used for NPC's avoiding obstacles
	Float GetAvoidanceRadius() const { return m_avoidanceRadius; }

	//! Movement update
	virtual void FinalizeMovement2_PostSeparation( Float timeDelta ) override;

	//! (Re)sets active movable representation
	void ResetMoveRepresentation();

protected:
	//! Enable or disable combat mode
	virtual void EnableCombatMode( Bool combat );

	// virtual controllers
	virtual void EnableVirtualController( const CName& virtualControllerName, const Bool enabled );

	// virtual radius
	virtual void SetVirtualRadius( const CName& radiusName, Bool immediately, const CName& virtualControllerName );
	virtual void ResetVirtualRadius( const CName& virtualControllerName );

	// height
	virtual void SetHeight( const Float Height );
	virtual void ResetHeight();

	//! Compute and apply delta movement
	virtual Bool ProcessMovement( Float timeDelta ) override;

	// ------------------------------------------------------------------------
	// Movement actuation commands
	// ------------------------------------------------------------------------

	//! Updates the parallelized part of movement computations
	void UpdateParallelMovementBehavior( Float timeDelta );

	//! Updates the synchronized ( single-threaded ) part of movement computations
	void UpdateSynchronizedMovementBehavior( Float timeDelta );

 	void CalculateMovement( Float timeDelta );

	void UpdateMovedFlag();

	void CalculateMovementStats( Float timeDelta, const Vector& actualDeltaPosition, const Vector& deltaPositionFromBehaviorGraph );

	virtual IMovableRepresentation* DetermineMoveRepresentation() const;

	// ------------------------------------------------------------------------
	// Trigger system commands
	// ------------------------------------------------------------------------

	//! Change the radius of the trigger activator
	void SetTriggerActivatorRadius( const Float radius );

	//! Change the height of the trigger activator
	void SetTriggerActivatorHeight( const Float height );

public:

	//! Add trigger channel to the trigger activator
	void AddTriggerActivatorChannel( const ETriggerChannel channel );

	//! Remove trigger channel from the trigger activator
	void RemoveTriggerActivatorChannel( const ETriggerChannel channel );

	//! Check if activator is inside trigger
	Bool IsInsideTrigger( const class ITriggerObject* trigger ) const;

private:
	void CreateMovingAgent();

	//! Updates the movement direction based on the requests made
	void UpdateMoveDirection( Float timeDelta );

	//! Updates the movement speed
	void UpdateMoveSpeed( Float timeDelta );

	//! Updates the movement rotation based on the requests made
	void UpdateMoveRotation( Float timeDelta );

	//! Updates the slide data sink
	void UpdateSlide( Float timeDelta );

	//! Updates movement adjustor
	void UpdateAdjustor( Float timeDelta );
	
	//! Clamp rotation to set in current frame
	Float ClampRotation( Float timeDelta, Float rotation ) const;

	// Blend pelvis in world space - it can be useful after teleports etc.
	virtual void BlendPelvisWS( const Matrix& pointWS, Float blendTime );

public: // animation proxy accessors
	//! Set the use of feet IK
	RED_INLINE void SetEnabledFeetIK( Bool enabled, Float blendTime ) { m_animationProxyData.SetUseIK( enabled, blendTime ); }
	RED_INLINE bool GetEnabledFeetIK() const { return m_animationProxyData.GetUseIK(); }

	RED_INLINE void SetEnabledHandsIK( Bool use) { m_animationProxyData.SetUseHandsIK( use ); }
	RED_INLINE void SetHandsIKOffsets( Float leftOffset, Float rightOffset ) { m_animationProxyData.SetHandsIKOffsets( leftOffset, rightOffset ); }

	//! Set the use of sliding on slope (feet, hands without pelvis offset) IK
	RED_INLINE void SetEnabledSlidingOnSlopeIK( Bool enabled ) { m_animationProxyData.SetSlidingOnSlopeIK( enabled ); }
	RED_INLINE bool GetEnabledSlidingOnSlopeIK() const { return m_animationProxyData.GetSlidingOnSlopeIK(); }

	//! Entity for pelvis offset - to match other's entity pelvis offset or accommodate it in some other manner
	RED_INLINE void SetUseEntityForPelvisOffset( CEntity* entity ) { m_animationProxyData.SetUseEntityForPelvisOffset( entity ); }
	RED_INLINE CEntity* GetUseEntityForPelvisOffset() const { return m_animationProxyData.GetUseEntityForPelvisOffset(); }

	//! Offset when attaching to entity
	RED_INLINE void SetAdditionalOffsetWhenAttachingToEntity( CEntity* entity, Float time ) { m_animationProxyData.SetAdditionalOffsetMS( entity, time ); }

	//! Set offset to consumed, for example smooth transition after snapping to terrain
	RED_INLINE void SetAdditionalOffsetToConsumeMS( const Vector& posOffsetMS, const EulerAngles& rotOffsetMS, Float time ) { m_animationProxyData.SetAdditionalOffsetMS( posOffsetMS, rotOffsetMS, time ); }
	RED_INLINE void SetAdditionalOffsetToConsumePointWS( const Matrix& pointWS, Float time ) { m_animationProxyData.SetAdditionalOffsetMSPointWS( pointWS, time ); }

	RED_INLINE SAnimationProxyData & AccessAnimationProxy() { return m_animationProxyData; }
	RED_INLINE const SAnimationProxyData & GetAnimationProxy() const { return m_animationProxyData; }

private:
	void funcSetMaxMoveRotationPerSec( CScriptStackFrame& stack, void* result );	
	void funcGetCurrentMoveSpeedAbs( CScriptStackFrame& stack, void* result );
	void funcAddDeltaMovement( CScriptStackFrame& stack, void* result );
	void funcTeleportBehindCamera( CScriptStackFrame& stack, void* result );
	void funcSetMoveType( CScriptStackFrame& stack, void* result );
	void funcEnableCombatMode( CScriptStackFrame& stack, void* result );
	void funcEnableVirtualController( CScriptStackFrame& stack, void* result );
	void funcSetVirtualRadius( CScriptStackFrame& stack, void* result );
	void funcSetVirtualRadiusImmediately( CScriptStackFrame& stack, void* result );
	void funcResetVirtualRadius( CScriptStackFrame& stack, void* result );
	void funcSetHeight( CScriptStackFrame& stack, void* result );
	void funcResetHeight( CScriptStackFrame& stack, void* result );
	void funcGetSpeed( CScriptStackFrame& stack, void* result );
	void funcGetRelativeMoveSpeed( CScriptStackFrame& stack, void* result );
	void funcGetMoveTypeRelativeMoveSpeed( CScriptStackFrame& stack, void* result );
	void funcForceSetRelativeMoveSpeed( CScriptStackFrame& stack, void* result );
	void funcSetGameplayRelativeMoveSpeed( CScriptStackFrame& stack, void* result );
	void funcSetGameplayMoveDirection( CScriptStackFrame& stack, void* result );
	void funcSetDirectionChangeRate( CScriptStackFrame& stack, void* result );
	void funcGetMaxSpeed( CScriptStackFrame& stack, void* result );
	void funcGetVelocity( CScriptStackFrame& stack, void* result );
	void funcGetVelocityBasedOnRequestedMovement( CScriptStackFrame& stack, void* result );
	void funcAdjustRequestedMovementDirectionPhysics( CScriptStackFrame& stack, void* result );
	void funcAdjustRequestedMovementDirectionNavMesh( CScriptStackFrame& stack, void* result );
	void funcStartRoadFollowing( CScriptStackFrame& stack, void* result );
	void funcResetRoadFollowing( CScriptStackFrame& stack, void* result );
	void funcGetAgentPosition( CScriptStackFrame& stack, void* result );
	void funcGetPathPointInDistance( CScriptStackFrame& stack, void* result );
	void funcSnapToNavigableSpace( CScriptStackFrame& stack, void* result );
	void funcIsOnNavigableSpace( CScriptStackFrame& stack, void* result );
	void funcIsEntityRepresentationForced( CScriptStackFrame& stack, void* result );
	void funcGetLastNavigablePosition( CScriptStackFrame& stack, void* result );

	void funcCanGoStraightToDestination( CScriptStackFrame& stack, void* result );
	void funcIsPositionValid( CScriptStackFrame& stack, void* result );
	void funcGetEndOfLineNavMeshPosition( CScriptStackFrame& stack, void* result );
	void funcIsEndOfLinePositionValid( CScriptStackFrame& stack, void* result );
	void funcIsInSameRoom( CScriptStackFrame& stack, void* result );
	void funcGetMovementAdjustor( CScriptStackFrame& stack, void* result );
	void funcPredictWorldPosition( CScriptStackFrame& stack, void* result );

	void funcSetTriggerActivatorRadius( CScriptStackFrame& stack, void* result );
	void funcSetTriggerActivatorHeight( CScriptStackFrame& stack, void* result );
	void funcAddTriggerActivatorChannel( CScriptStackFrame& stack, void* result );
	void funcRemoveTriggerActivatorChannel( CScriptStackFrame& stack, void* result );

	void funcSetEnabledFeetIK( CScriptStackFrame& stack, void* result );
	void funcGetEnabledFeetIK( CScriptStackFrame& stack, void* result );

	void funcSetEnabledHandsIK( CScriptStackFrame& stack, void* result );
	void funcSetHandsIKOffsets( CScriptStackFrame& stack, void* result );

	void funcSetEnabledSlidingOnSlopeIK( CScriptStackFrame& stack, void* result );
	void funcGetEnabledSlidingOnSlopeIK( CScriptStackFrame& stack, void* result );
	void funcSetUseEntityForPelvisOffset( CScriptStackFrame& stack, void* result );
	void funcGetUseEntityForPelvisOffset( CScriptStackFrame& stack, void* result );
	void funcSetAdditionalOffsetWhenAttachingToEntity( CScriptStackFrame& stack, void* result );
	void funcSetAdditionalOffsetToConsumePointWS( CScriptStackFrame& stack, void* result );
	void funcSetAdditionalOffsetToConsumeMS( CScriptStackFrame& stack, void* result );

	friend class CGameWorld; // to allow ForceFinalizeMovement
};

BEGIN_CLASS_RTTI( CMovingAgentComponent );
	PARENT_CLASS( CAnimatedComponent );
	PROPERTY_EDIT( m_ragdollRadius,						TXT("Ragdoll radius") );
	PROPERTY_EDIT( m_steeringBehavior,					TXT("Steering behavior resource") );
	PROPERTY_EDIT( m_steeringControlledMovement,		TXT("Steering fully control movement (no motion extraction used).") );
	PROPERTY_EDIT( m_snapToNavigableSpace,				TXT("Moving snaps to navigable space.") );
	PROPERTY_EDIT( m_physicalRepresentation,			TXT("Enables physical movement instead of navmesh-based") );
	PROPERTY_NOSERIALIZE( m_movementAdjustor );
	PROPERTY_EDIT( m_triggerAutoActivator,				TXT("Automatically create trigger activator (mimics old system)") );
	PROPERTY_EDIT_RANGE( m_triggerActivatorRadius,		TXT("Radius of trigger activator"), 0.0f, 1000.0f );
	PROPERTY_EDIT_RANGE( m_triggerActivatorHeight,		TXT("Height of trigger activator"), 0.0f, 1000.0f );
	PROPERTY_BITFIELD_EDIT( m_triggerChannels, ETriggerChannel, TXT("Channels on which we do the activation") );
	PROPERTY_EDIT( m_triggerEnableCCD,					TXT("Use contignous collision detection (much slower)") );
	PROPERTY_EDIT( m_triggerMaxSingleFrameDistance,		TXT("Maximum distance traveled in one frame that can be considered contignous") );

	NATIVE_FUNCTION( "SetMaxMoveRotationPerSec", funcSetMaxMoveRotationPerSec );
	NATIVE_FUNCTION( "GetCurrentMoveSpeedAbs", funcGetCurrentMoveSpeedAbs );
	NATIVE_FUNCTION( "AddDeltaMovement", funcAddDeltaMovement );
	NATIVE_FUNCTION( "TeleportBehindCamera", funcTeleportBehindCamera );
	NATIVE_FUNCTION( "SetMoveType", funcSetMoveType );
	NATIVE_FUNCTION( "EnableCombatMode", funcEnableCombatMode );
	NATIVE_FUNCTION( "EnableVirtualController", funcEnableVirtualController );
	NATIVE_FUNCTION( "SetVirtualRadius", funcSetVirtualRadius );
	NATIVE_FUNCTION( "SetVirtualRadiusImmediately", funcSetVirtualRadiusImmediately );
	NATIVE_FUNCTION( "ResetVirtualRadius", funcResetVirtualRadius );
	NATIVE_FUNCTION( "SetHeight", funcSetHeight );
	NATIVE_FUNCTION( "ResetHeight", funcResetHeight );
	NATIVE_FUNCTION( "GetSpeed", funcGetSpeed );
	NATIVE_FUNCTION( "GetRelativeMoveSpeed", funcGetRelativeMoveSpeed );
	NATIVE_FUNCTION( "GetMoveTypeRelativeMoveSpeed", funcGetMoveTypeRelativeMoveSpeed  );
	NATIVE_FUNCTION( "ForceSetRelativeMoveSpeed", funcForceSetRelativeMoveSpeed );
	NATIVE_FUNCTION( "SetGameplayRelativeMoveSpeed", funcSetGameplayRelativeMoveSpeed );
	NATIVE_FUNCTION( "SetGameplayMoveDirection", funcSetGameplayMoveDirection );
	NATIVE_FUNCTION( "SetDirectionChangeRate", funcSetDirectionChangeRate );
	NATIVE_FUNCTION( "GetMaxSpeed", funcGetMaxSpeed );
	NATIVE_FUNCTION( "GetVelocity", funcGetVelocity );
	NATIVE_FUNCTION( "GetVelocityBasedOnRequestedMovement", funcGetVelocityBasedOnRequestedMovement );
	NATIVE_FUNCTION( "AdjustRequestedMovementDirectionPhysics", funcAdjustRequestedMovementDirectionPhysics );
	NATIVE_FUNCTION( "AdjustRequestedMovementDirectionNavMesh", funcAdjustRequestedMovementDirectionNavMesh );
	NATIVE_FUNCTION( "StartRoadFollowing", funcStartRoadFollowing );
	NATIVE_FUNCTION( "ResetRoadFollowing", funcResetRoadFollowing );
	NATIVE_FUNCTION( "GetAgentPosition", funcGetAgentPosition );
	NATIVE_FUNCTION( "GetPathPointInDistance", funcGetPathPointInDistance );
	NATIVE_FUNCTION( "SnapToNavigableSpace", funcSnapToNavigableSpace );
	NATIVE_FUNCTION( "IsOnNavigableSpace", funcIsOnNavigableSpace );
	NATIVE_FUNCTION( "IsEntityRepresentationForced", funcIsEntityRepresentationForced );
	NATIVE_FUNCTION( "GetLastNavigablePosition", funcGetLastNavigablePosition );

	NATIVE_FUNCTION( "CanGoStraightToDestination", funcCanGoStraightToDestination );
	NATIVE_FUNCTION( "IsPositionValid", funcIsPositionValid );
	NATIVE_FUNCTION( "GetEndOfLineNavMeshPosition", funcGetEndOfLineNavMeshPosition );
	NATIVE_FUNCTION( "IsEndOfLinePositionValid", funcIsEndOfLinePositionValid );
	NATIVE_FUNCTION( "IsInSameRoom", funcIsInSameRoom );
	NATIVE_FUNCTION( "GetMovementAdjustor", funcGetMovementAdjustor );
	NATIVE_FUNCTION( "PredictWorldPosition", funcPredictWorldPosition );

	NATIVE_FUNCTION( "SetTriggerActivatorRadius", funcSetTriggerActivatorRadius );
	NATIVE_FUNCTION( "SetTriggerActivatorHeight", funcSetTriggerActivatorHeight );
	NATIVE_FUNCTION( "AddTriggerActivatorChannel", funcAddTriggerActivatorChannel );
	NATIVE_FUNCTION( "RemoveTriggerActivatorChannel", funcRemoveTriggerActivatorChannel );

	NATIVE_FUNCTION( "SetEnabledFeetIK", funcSetEnabledFeetIK );
	NATIVE_FUNCTION( "GetEnabledFeetIK", funcGetEnabledFeetIK );
	NATIVE_FUNCTION( "SetEnabledHandsIK", funcSetEnabledHandsIK );
	NATIVE_FUNCTION( "SetHandsIKOffsets", funcSetHandsIKOffsets );
	NATIVE_FUNCTION( "SetEnabledSlidingOnSlopeIK", funcSetEnabledSlidingOnSlopeIK );
	NATIVE_FUNCTION( "GetEnabledSlidingOnSlopeIK", funcGetEnabledSlidingOnSlopeIK );
	NATIVE_FUNCTION( "SetUseEntityForPelvisOffset", funcSetUseEntityForPelvisOffset );
	NATIVE_FUNCTION( "GetUseEntityForPelvisOffset", funcGetUseEntityForPelvisOffset );
	NATIVE_FUNCTION( "SetAdditionalOffsetWhenAttachingToEntity", funcSetAdditionalOffsetWhenAttachingToEntity );
	NATIVE_FUNCTION( "SetAdditionalOffsetToConsumePointWS", funcSetAdditionalOffsetToConsumePointWS );
	NATIVE_FUNCTION( "SetAdditionalOffsetToConsumeMS", funcSetAdditionalOffsetToConsumeMS );
	
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
