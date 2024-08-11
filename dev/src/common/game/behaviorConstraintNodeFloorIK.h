/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma  once

#include "../engine/behaviorIkApplyRotation.h"
#include "../engine/behaviorIkTwoBones.h"
#include "../engine/behaviorGraphNode.h"
#include "../engine/behaviorPoseConstraintNode.h"

//////////////////////////////////////////////////////////////////////////

#ifndef NO_EDITOR_GRAPH_SUPPORT
#define DEBUG_FLOOR_IK
//#define DEBUG_FLOOR_IK_TRACES
#define DEBUG_FLOOR_IK_POSES
#endif

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintNodeFloorIKCommonData;
struct SBehaviorConstraintNodeFloorIKVerticalBoneData;
struct SBehaviorConstraintNodeFloorIKLegsData;
struct SBehaviorConstraintNodeFloorIKCommon;
struct SBehaviorConstraintNodeFloorIKVerticalBone;
struct SBehaviorConstraintNodeFloorIKLegs;
struct SBehaviorConstraintNodeFloorIKCachedTrace;
struct SBehaviorConstraintNodeFloorIKLeg;
struct SBehaviorConstraintNodeFloorIKWeightHandler;
struct SBehaviorConstraintNodeFloorIKFrontBackWeightHandler;
struct SBehaviorConstraintNodeFloorIKDebugTrace;
class CBehaviorConstraintNodeFloorIK;

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintNodeFloorIKCommonData
{
public:
	DECLARE_RTTI_STRUCT( SBehaviorConstraintNodeFloorIKCommonData );
	
	CName m_gravityCentreBone;
	Float m_rootRotationBlendTime;
	Float m_speedForFullRootRotationBlend;
	Float m_verticalVelocityOffsetUpBlendTime;
	Float m_verticalVelocityOffsetDownBlendTime;
	Float m_slidingOnSlopeBlendTime;

	SBehaviorConstraintNodeFloorIKCommonData()
	: m_rootRotationBlendTime( 0.2f )
	, m_speedForFullRootRotationBlend( 2.0f )
	, m_verticalVelocityOffsetDownBlendTime( 0.03f )
	, m_verticalVelocityOffsetUpBlendTime( 0.08f )
	, m_slidingOnSlopeBlendTime( 0.2f )
	{
	}
};

BEGIN_CLASS_RTTI( SBehaviorConstraintNodeFloorIKCommonData );
	PROPERTY_CUSTOM_EDIT( m_gravityCentreBone, TXT("Gravity centre bone"), TXT( "BehaviorBoneSelection" ) );
	PROPERTY_EDIT_RANGE( m_rootRotationBlendTime, TXT("Blend time of root rotation"), 0.0f, 1.0f );
	PROPERTY_EDIT_RANGE( m_verticalVelocityOffsetUpBlendTime, TXT("Blend time of vertical velocity when going up"), 0.0f, 1.0f );
	PROPERTY_EDIT_RANGE( m_verticalVelocityOffsetDownBlendTime, TXT("Blend time of vertical velocity when going down"), 0.0f, 1.0f );
	PROPERTY_EDIT( m_slidingOnSlopeBlendTime, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintNodeFloorIKVerticalBoneData
{
	DECLARE_RTTI_STRUCT( SBehaviorConstraintNodeFloorIKVerticalBoneData );
	Vector2 m_offsetRange;
	CName m_bone;
	Float m_offsetToDesiredBlendTime;
	Float m_verticalOffsetBlendTime;
	Float m_stiffness;

	SBehaviorConstraintNodeFloorIKVerticalBoneData()
		: m_offsetRange( -0.5f, 0.5f )
		, m_offsetToDesiredBlendTime( 0.1f )
		, m_verticalOffsetBlendTime( 0.00f )
		, m_stiffness( 1.0f )
	{
	}
};

BEGIN_CLASS_RTTI( SBehaviorConstraintNodeFloorIKVerticalBoneData );
	PROPERTY_CUSTOM_EDIT( m_bone, TXT("Bone"), TXT( "BehaviorBoneSelection" ) );
	PROPERTY_EDIT_NAME( m_offsetRange.X, TXT("Min offset"), TXT("Lowest possible offset for bone") );
	PROPERTY_EDIT_NAME( m_offsetRange.Y, TXT("Max offset"), TXT("Highest possible offset for bone") );
	PROPERTY_EDIT_RANGE( m_offsetToDesiredBlendTime, TXT("Blend time used to blend to desired vertical offset"), 0.0f, 1.0f );
	PROPERTY_EDIT_RANGE( m_verticalOffsetBlendTime, TXT("Blend time used to blend vertical new offset with old adjusted"), 0.0f, 1.0f );
	PROPERTY_EDIT_RANGE( m_stiffness, TXT("Stiffness of bone replacement in relation to legs"), 0.0f, 1.0f );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintNodeFloorIKMaintainLookBoneData
{
	DECLARE_RTTI_STRUCT( SBehaviorConstraintNodeFloorIKMaintainLookBoneData );
	CName m_bone;
	Float m_amountOfRotation;

	SBehaviorConstraintNodeFloorIKMaintainLookBoneData()
		: m_amountOfRotation( 0.5f )
	{
	}
};

BEGIN_CLASS_RTTI( SBehaviorConstraintNodeFloorIKMaintainLookBoneData );
	PROPERTY_CUSTOM_EDIT( m_bone, TXT("Bone"), TXT( "BehaviorBoneSelection" ) );
	PROPERTY_EDIT_RANGE( m_amountOfRotation, TXT("Amount of rotation applied to this bone"), 0.0f, 1.0f );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintNodeFloorIKLegsData
{
	DECLARE_RTTI_STRUCT( SBehaviorConstraintNodeFloorIKLegsData );

	Vector2 m_offsetRange; // relative to ref position
	Vector2 m_traceOffsetRange;
	Float m_verticalOffsetBlendUpTime;
	Float m_verticalOffsetBlendDownTime;
	Float m_maxDistanceForTraceUpdate;
	Float m_maxAngleOffUprightNormal;
	Float m_maxAngleOffUprightNormalSide;
	Float m_maxAngleOffUprightNormalToRevert;
	Bool m_useInputNormalForNormalClamping;

	SBehaviorConstraintNodeFloorIKLegsData()
		: m_offsetRange( 0.0f, 0.5f ) // 0.0f to not overstretch
		, m_traceOffsetRange( -1.0f, 1.0f )
		, m_verticalOffsetBlendUpTime( 0.06f )
		, m_verticalOffsetBlendDownTime( 0.03f )
		, m_maxDistanceForTraceUpdate( 0.02f )
		, m_maxAngleOffUprightNormal( 45.0f )
		, m_maxAngleOffUprightNormalSide( 180.0f )
		, m_maxAngleOffUprightNormalToRevert( 70.0f )
		, m_useInputNormalForNormalClamping( false )
	{
	}
};

BEGIN_CLASS_RTTI( SBehaviorConstraintNodeFloorIKLegsData );
	PROPERTY_EDIT_NAME( m_offsetRange.X, TXT("Min rel offset"), TXT("Lowest possible offset for leg relative to ref pos") );
	PROPERTY_EDIT_NAME( m_offsetRange.Y, TXT("Max rel offset"), TXT("Highest possible offset for leg relative to ref pos") );
	PROPERTY_EDIT_NAME( m_traceOffsetRange.X, TXT("Min trace offset"), TXT("Low point of trace") );
	PROPERTY_EDIT_NAME( m_traceOffsetRange.Y, TXT("Max trace offset"), TXT("Hi point of trace") );
	PROPERTY_EDIT_RANGE( m_verticalOffsetBlendUpTime, TXT("Blend time used to blend vertical offset (when going up)"), 0.0f, 1.0f )
	PROPERTY_EDIT_RANGE( m_verticalOffsetBlendDownTime, TXT("Blend time used to blend vertical offset (when going down)"), 0.0f, 1.0f )
	PROPERTY_EDIT_NAME( m_maxDistanceForTraceUpdate, TXT("Max distance for trace update"), TXT("Max distance for trace update") )
	PROPERTY_EDIT_RANGE( m_maxAngleOffUprightNormal, TXT("Max angle from upright normal"), 0.0f, 180.0f )
	PROPERTY_EDIT_RANGE( m_maxAngleOffUprightNormalSide, TXT("Max angle from upright normal to side"), 0.0f, 180.0f )
	PROPERTY_EDIT_RANGE( m_maxAngleOffUprightNormalToRevert, TXT("Max angle from upright normal to revert orientation"), 0.0f, 180.0f )
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintNodeFloorIKCommon : public IEventHandler< CAnimationEventFired >
{
public:
	DECLARE_RTTI_STRUCT( SBehaviorConstraintNodeFloorIKCommon );

	Float m_additionalBlendCoefOnTeleport; // when character is teleported, this is additional blend coef value used to force all blends to end 

	Float m_slopeFromAnim;
	Float m_slopeFromAnimAccumulated;

	Float m_slidingOnSlopeWeight;
	Float m_prevSlidingOnSlopeWeight;

	Bool m_jumping;
	Bool m_jumpingAccumulator;
	Float m_jumpingOffset;

	Float m_speedXY;

	Vector m_feetDistAlongDirMS; // adjust feet along forward direction - to make feet hit ground closer/further apart
	Float m_feetDistCoef;
	Float m_feetDistOffset;
	Vector m_feetDistAlongDirRequestedMS;
	Float m_feetDistCoefRequested;
	Float m_feetDistOffsetRequested;
	Bool m_useFullInverted;
	Matrix m_localToWorld;
	Matrix m_localToWorldInverted;
	Matrix m_prevLocalToWorld;
	Vector m_actualVelocityWS;
	Float m_adjustOffsetByRaw; // raw value
	Float m_prevAdjustOffsetByRaw;
	Float m_abruptAdjustemntWeight;
	Float m_adjustOffsetByVel; // additional velocity to predict where character will be
	Float m_targetAdjustOffsetByVel; // target to be used with next frame
	Vector m_worldUprightNormalWS;
	Vector m_footUprightNormalRS;
	Vector m_footUprightNormalMS;

	Vector m_currentUpWS;
	Vector m_requestedUpWS;
	Vector m_velocityUpWS;
	AnimQuaternion m_shortestRotationToCurrentWS;
	AnimQuaternion m_shortestRotationToCurrentMS;
	Matrix m_rootSpaceMS; // root space in model space
	Matrix m_rootSpaceMSInverted;
	Matrix m_rootSpaceWS; // root space in world space
	Matrix m_rootSpaceWSInverted;
	Bool m_footUprightNormalMSMoreImportant; // will udpate RS from MS, otherwise will udpate RS from MS

	Float m_cachedStepOffset;
	Float m_centreOfGravityCentreAlt; // so when it is rotated to match current up, it is rotated around such centre of gravity

	Bool m_useFixedVersion;
	Bool m_useExtendedIKOffset;
	Uint32 m_HACK_resetFrameCounter;

#ifdef DEBUG_FLOOR_IK
	CName m_testVarID;
	CName m_testVarSlidingOnSlopeID;
#endif

	virtual	void HandleEvent( const CAnimationEventFired &event );
	
	void ReadyForNextFrame();

	void Reset( CBehaviorGraphInstance& instance );

	void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;

	void OnActivated( CBehaviorGraphInstance& instance );

	void Setup( SBehaviorConstraintNodeFloorIKCommonData const & data, CBehaviorGraphInstance& instance );
	void Update( SBehaviorConstraintNodeFloorIKCommonData const & data, Float timeDelta, Float prevTimeDelta, CBehaviorGraphInstance& instance, Float weight );
	void UpdateAdjustmentOffset( SBehaviorConstraintNodeFloorIKCommonData const & data, Float timeDelta, Float prevTimeDelta, Float weight );
	void SetUprightNormalRS( const Vector& worldUprightNormalWS, const Vector& footUprightNormalRS );
	void SetUprightNormalMS( const Vector& worldUprightNormalWS, const Vector& footUprightNormalMS );

	void SetRequestedUp( const Vector& requestedUpWS ) { m_requestedUpWS = requestedUpWS.Normalized3(); }

	void RequestFeetDistanceAdjustment( const Vector& _dir, Float _coef, Float _offset ) { m_feetDistAlongDirRequestedMS = _dir; m_feetDistCoefRequested = _coef; m_feetDistOffsetRequested = _offset; }

#ifdef DEBUG_FLOOR_IK
	void SetupTestVars( CBehaviorGraphInstance& instance, const CName onOff, const CName slidingOnSlope );
#endif
};

BEGIN_CLASS_RTTI( SBehaviorConstraintNodeFloorIKCommon );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintNodeFloorIKVerticalBone
{
	DECLARE_RTTI_STRUCT( SBehaviorConstraintNodeFloorIKVerticalBone );
	Float m_offset;
	Int32 m_parentBone; // parent bone
	Int32 m_bone;
	Vector m_offsetVerticallyMS;
#ifdef DEBUG_FLOOR_IK
	mutable Vector m_debugVerticalBoneLocWS;
	Float m_debugDesiredOffset;
	Float m_debugLeftAdditionalOffset;
	Float m_debugRightAdditionalOffset;
#endif

	SBehaviorConstraintNodeFloorIKVerticalBone();

	void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;

	void Setup( CBehaviorGraphInstance& instance, const SBehaviorConstraintNodeFloorIKVerticalBoneData& data );

	void Reset();
	void Update( Float timeDelta, SBehaviorConstraintNodeFloorIKLeg& leg, const SBehaviorConstraintNodeFloorIKVerticalBoneData& data, const SBehaviorConstraintNodeFloorIKLegsData& legsData, const SBehaviorConstraintNodeFloorIKCommon& common );
	void Update( Float timeDelta, SBehaviorConstraintNodeFloorIKLeg& leftLeg, SBehaviorConstraintNodeFloorIKLeg& rightLeg,
		const SBehaviorConstraintNodeFloorIKVerticalBoneData& data, const SBehaviorConstraintNodeFloorIKLegsData& legsData, const SBehaviorConstraintNodeFloorIKCommon& common );
	void Update( Float timeDelta, SBehaviorConstraintNodeFloorIKLeg& leftFrontLeg, SBehaviorConstraintNodeFloorIKLeg& rightFrontLeg, 
		SBehaviorConstraintNodeFloorIKLeg& leftBackLeg, SBehaviorConstraintNodeFloorIKLeg& rightBackLeg, 
		const SBehaviorConstraintNodeFloorIKVerticalBoneData& data, const SBehaviorConstraintNodeFloorIKLegsData& legsData, const SBehaviorConstraintNodeFloorIKCommon& common );
	void UpdatePose( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, const SBehaviorConstraintNodeFloorIKVerticalBoneData& data, const SBehaviorConstraintNodeFloorIKCommon& common, Float weight, Bool storeAppliedOffsetInAnimationProxy = false );

	Float GetVerticalAdjustBy( const SBehaviorConstraintNodeFloorIKVerticalBoneData& data, const SBehaviorConstraintNodeFloorIKCommon& common, Float timeDelta, Float rawCoef = 1.0f, Float velocityCoef = 1.0f ) const;

	void StoreTraces( const CBehaviorGraphInstance& instance, const SBehaviorGraphOutput& output, const SBehaviorConstraintNodeFloorIKCommon& common, SBehaviorConstraintNodeFloorIKDebugTrace& characterTrace, SBehaviorConstraintNodeFloorIKDebugTrace& characterOffsetTrace, SBehaviorConstraintNodeFloorIKDebugTrace& pelvisTrace );
};

BEGIN_CLASS_RTTI( SBehaviorConstraintNodeFloorIKVerticalBone );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintNodeFloorIKMaintainLookBone
{
	DECLARE_RTTI_STRUCT( SBehaviorConstraintNodeFloorIKMaintainLookBone );
	Int32 m_bone;
	Int32 m_parent;

	void Setup( CBehaviorGraphInstance& instance, const SBehaviorConstraintNodeFloorIKMaintainLookBoneData& data );

	void UpdatePose( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, const SBehaviorConstraintNodeFloorIKMaintainLookBoneData& data, const SBehaviorConstraintNodeFloorIKCommon& common, Float weight ) const;
};

BEGIN_CLASS_RTTI( SBehaviorConstraintNodeFloorIKMaintainLookBone );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintNodeFloorIKLegs
{
	DECLARE_RTTI_STRUCT( SBehaviorConstraintNodeFloorIKLegs );

	Float m_maxCosAngleOffUprightNormal;
	Float m_maxSinAngleOffUprightNormal;
	Float m_maxCosAngleOffUprightNormalSide;
	Float m_maxSinAngleOffUprightNormalSide;
	Float m_maxCosAngleOffUprightNormalToRevert;

	SBehaviorConstraintNodeFloorIKLegs();

	void Setup( CBehaviorGraphInstance& instance, const SBehaviorConstraintNodeFloorIKLegsData& data );
};

BEGIN_CLASS_RTTI( SBehaviorConstraintNodeFloorIKLegs );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintNodeFloorIKCachedTrace
{
	DECLARE_RTTI_STRUCT( SBehaviorConstraintNodeFloorIKCachedTrace );
private:
	Vector m_locWS;
	Bool m_hit;
	Vector m_hitLocWS;
	Vector m_normalWS;
	Vector m_adjustHitLocZ; // using normal

public:
	SBehaviorConstraintNodeFloorIKCachedTrace();

	/** returns false if requires new trace */
	Bool GetTraceResult( const Vector& locWS, Bool& outHit, Vector& outHitLocWS, Vector& outNormalWS, const SBehaviorConstraintNodeFloorIKLegsData& legs ) const;
	void Update( const Vector& locWS, Bool hit, const Vector& hitLocWS, const Vector& normalWS );
	void Invalidate() { m_hit = false; }

	void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;
};

BEGIN_CLASS_RTTI( SBehaviorConstraintNodeFloorIKCachedTrace );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintNodeFloorIKLeg
{
	DECLARE_RTTI_STRUCT( SBehaviorConstraintNodeFloorIKLeg );

	Float m_minAltitudeAboveGround;

	Matrix m_startMatRS;
	Vector m_startLocRS;
	Vector m_startLocUprightMaintainedRS;
	Vector m_inputStartLocRS; // not maintained - as in animation
	Vector m_inputStartNormalRS; // not maintained - as in animation
	Vector m_inputStartNormalRefForClampingRS; // used for clamping

	Vector m_startSideRS;

	Float m_aboveGround;

	Vector m_finalNotClampedNormalRS;
	Vector m_finalNormalRS;
	Vector m_desiredFootOffsetLeftRS; // desired offset left (pelvis modifies it and shoulders have lesser influence to handle)
	Vector m_footOffsetRS; // this value is blended and is base for "used foot offset" which is modified by pelvis/shoulder adjustments
	Vector m_usedFootOffsetRS;
	Vector m_usedFootOffsetRefRS; // this is reference point and we limit final offset against it

	// both with applied jumping offset
	Vector m_hitLocWS;
	Vector m_hitLocRS;

	SBehaviorConstraintNodeFloorIKCachedTrace m_cachedTrace;

#ifdef DEBUG_FLOOR_IK
	CName m_testVarID;
	CName m_testVarLocID;
	CName m_testVarNormalID;
	CName m_testVarUsePlaneID;
	CName m_testVarPlaneNormalID;
	Bool m_debugHit;
	Vector m_debugHitLocWS;
	Vector m_debugNormalWS;
	Vector m_debugRawNormalWS;
	mutable Matrix m_debugStartMaintainedMatWS;
	mutable Matrix m_debugFinalOffsetRefMatWS;
	mutable Matrix m_debugStartMatWS;
	mutable Matrix m_debugFinalMatWS;
	mutable Matrix m_debugIKMatWS;
	Vector m_debugUsedFootOffsetToStartRS; // used to know how much vertical bones may additionally influence offset
	Vector m_debugStartFinalOffsetRefBaseRS;
	Vector m_debugStartFinalOffsetRefRS; // this is reference point and we limit final offset against it
	Vector m_debugFinalBaseWS;
	Float m_debugAboveGround;
	Float m_debugFinalVerticalOffsetPrePelvis;
	Float m_debugNewBaseVerticalOffset;
	Float m_debugNewFinalVerticalOffset;
	Float m_debugNewFinalVerticalOffsetLeft;
	mutable Float m_debugNewFinalAdjustedVerticalOffset;
	Bool m_debugHitMinVerticalOffset;
	Float m_debugMinVerticalOffset;
	Float m_debugMinDefaultVerticalOffset;
	Vector m_debugUsedFootOffsetRefWS;
	Float m_debugAngleAnim;
	Float m_debugAngleSlope;
	Matrix m_debugLocalToWorldPrev;
	Matrix m_debugLocalToWorldCurr;
	mutable Vector m_debugOffset;
#endif

	SBehaviorConstraintNodeFloorIKLeg();

	void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame, const SBehaviorConstraintNodeFloorIKCommon& common ) const;

	void Setup( CBehaviorGraphInstance& instance, STwoBonesIKSolver& ikSolver );

#ifdef DEBUG_FLOOR_IK
	void SetupTestVars( CBehaviorGraphInstance& instance, const CName onOff, const CName loc, const CName normal, const CName testPlane, const CName planeNormal );
#endif

	void Reset( const SBehaviorConstraintNodeFloorIKCommon& common );
	void Update( Float timeDelta, SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, const STwoBonesIKSolverData& ikSolverData, STwoBonesIKSolver& ikSolver, const SBehaviorConstraintNodeFloorIKLegs& legs, const SBehaviorConstraintNodeFloorIKLegsData& data, const SBehaviorConstraintNodeFloorIKCommon& common );
	void SetupIK( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, STwoBonesIKSolver& ikSolver, const SBehaviorConstraintNodeFloorIKLegsData& data, const SBehaviorConstraintNodeFloorIKCommon& common ) const;
	void SetupRotIK( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, SApplyRotationIKSolver& ikSolver, const SBehaviorConstraintNodeFloorIKCommon& common ) const;

	void StoreTraces( const CBehaviorGraphInstance& instance, const SBehaviorGraphOutput& output, const SBehaviorConstraintNodeFloorIKCommon& common, const STwoBonesIKSolver& ikSolver, SBehaviorConstraintNodeFloorIKDebugTrace& legTrace, SBehaviorConstraintNodeFloorIKDebugTrace& legBaseTrace, SBehaviorConstraintNodeFloorIKDebugTrace& legHitTrace );
};

BEGIN_CLASS_RTTI( SBehaviorConstraintNodeFloorIKLeg );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintNodeFloorIKWeightHandler : public IEventHandler< CAnimationEventFired >
{
	DECLARE_RTTI_STRUCT( SBehaviorConstraintNodeFloorIKWeightHandler );
private:
	CName m_requiredAnimEvent;
	CName m_blockAnimEvent;
	Bool m_requiresSlowBlendDueToEvent;
	Bool m_requiresSlowBlendDueToMovement;
	Bool m_immediateBlendRequired;
	Float m_desiredWeight;
	Float m_eventDesiredWeight;
	Float m_eventForcedWeight;
	Float m_requiredEventDesiredWeight;
	Float m_blockEventDesiredWeight;
	Float m_currentWeight;

public:
	SBehaviorConstraintNodeFloorIKWeightHandler();

	virtual	void HandleEvent( const CAnimationEventFired &event );

	void SetAnimEvents( const CName & requiredAnimEvent, const CName & blockAnimEvent ) { m_requiredAnimEvent = requiredAnimEvent; m_blockAnimEvent = blockAnimEvent; }
	void LimitDesiredWeight( Float limit );
	void UpdateWeight( Float timeDelta, Float blendSpeed, const SBehaviorConstraintNodeFloorIKCommon& common );
	void ReadyForNextFrame();
	Float GetCurrentWeight() const { return m_currentWeight; }

	void UseLongerBlendDueToMovement( Bool usingCustomMovement ) { m_requiresSlowBlendDueToMovement |= usingCustomMovement; }
	void ImmediateBlendRequired() { m_immediateBlendRequired = true; }
};

BEGIN_CLASS_RTTI( SBehaviorConstraintNodeFloorIKWeightHandler );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintNodeFloorIKLegsIKWeightHandler : public IEventHandler< CAnimationEventFired >
{
	DECLARE_RTTI_STRUCT( SBehaviorConstraintNodeFloorIKLegsIKWeightHandler );
private:
	Float m_eventDesiredWeight;
	Float m_currentWeight;
	Float m_blendTime;

public:
	SBehaviorConstraintNodeFloorIKLegsIKWeightHandler();

	virtual	void HandleEvent( const CAnimationEventFired &event );

	void Update( Float timeDelta );

	Float GetWeight() const { return m_currentWeight; }

private:
	void ReadyForNextFrame();
};

BEGIN_CLASS_RTTI( SBehaviorConstraintNodeFloorIKLegsIKWeightHandler );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintNodeFloorIKFrontBackWeightHandler : public IEventHandler< CAnimationEventFired >
{
	DECLARE_RTTI_STRUCT( SBehaviorConstraintNodeFloorIKFrontBackWeightHandler );
private:
	struct PartWeightHandler
	{
		Float m_eventDesiredWeight;
		Float m_currentWeight;
		Float m_blendTime;
		PartWeightHandler()
			: m_eventDesiredWeight(1.0f)
			, m_currentWeight(1.0f)
			, m_blendTime(0.2f)
		{
		}
		void Update( Float timeDelta );
	};
	PartWeightHandler m_front;
	PartWeightHandler m_back;

public:
	SBehaviorConstraintNodeFloorIKFrontBackWeightHandler();

	virtual	void HandleEvent( const CAnimationEventFired &event );

	void Update( Float timeDelta );

	Float GetFrontWeight() const { return m_front.m_currentWeight; }
	Float GetBackWeight() const { return m_back.m_currentWeight; }

private:
	void ReadyForNextFrame();
};

BEGIN_CLASS_RTTI( SBehaviorConstraintNodeFloorIKFrontBackWeightHandler );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintNodeFloorIKDebugTrace
{
	DECLARE_RTTI_STRUCT( SBehaviorConstraintNodeFloorIKDebugTrace );
private:
	static const Int32 c_traceSize = 300;
	Vector m_traceTableWS[c_traceSize];
	Int32 m_traceIdx;

public:
	SBehaviorConstraintNodeFloorIKDebugTrace();

	void Store( Vector const & pointWS );
	void StoreEmpty() { Store( Vector::ZEROS ); }

	void GenerateFragments( CRenderFrame* frame, Color const & color, Float width = 0.0f ) const;
};

BEGIN_CLASS_RTTI( SBehaviorConstraintNodeFloorIKDebugTrace );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorConstraintNodeFloorIKBase : public CBehaviorGraphPoseConstraintNode
										 , public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_ABSTRACT_CLASS( CBehaviorConstraintNodeFloorIKBase, CBehaviorGraphPoseConstraintNode );

protected:
	SBehaviorConstraintNodeFloorIKCommonData m_common;
	Bool m_canBeDisabledDueToFrameRate;
	CName m_requiredAnimEvent;
	CName m_blockAnimEvent;
	Bool m_useFixedVersion;
	Float m_slopeAngleDamp;

protected:
	TInstanceVar< Float > i_timeDelta;
	TInstanceVar< Float > i_prevTimeDelta;
	TInstanceVar< SBehaviorConstraintNodeFloorIKCommon > i_common;
	TInstanceVar< SBehaviorConstraintNodeFloorIKWeightHandler > i_weightHandler;
	TInstanceVar< Bool > i_inCutscene;
	TInstanceVar< Float > i_slopeDampedValue;

#ifdef DEBUG_FLOOR_IK_POSES
	TInstanceVar< TDynArray< Matrix > > i_debugPoseAnim;
	TInstanceVar< TDynArray< Matrix > > i_debugPosePre;
	TInstanceVar< TDynArray< Matrix > > i_debugPosePost;
#endif

#ifndef NO_EDITOR_GRAPH_SUPPORT
protected:
	Bool m_generateEditorFragmentsForIKSolvers;	//!< Generate editor fragments for IK solvers
	Int32 m_generateEditorFragmentsForLegIndex;	//!< -1 all
#endif

public:
	CBehaviorConstraintNodeFloorIKBase();

public:
	virtual void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TXT( "Floor IK" ); }
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

protected:
	virtual void Setup( CBehaviorGraphInstance& instance ) const;

	virtual void UpdateUprightWS( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const {}

	virtual void UpdateAndSampleIK( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, Float weight, Float prevWeight ) const = 0;

	virtual void GenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const = 0;

protected:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	mutable Bool m_forceSetup;
#endif
};

BEGIN_ABSTRACT_CLASS_RTTI( CBehaviorConstraintNodeFloorIKBase );
	PARENT_CLASS( CBehaviorGraphPoseConstraintNode );
	PROPERTY_EDIT( m_requiredAnimEvent, TXT("") );
	PROPERTY_EDIT( m_blockAnimEvent, TXT("") );
	PROPERTY_EDIT( m_canBeDisabledDueToFrameRate, TXT("") );
	PROPERTY_EDIT( m_useFixedVersion, TXT("") );
	PROPERTY_EDIT( m_slopeAngleDamp, TXT("") );
#ifndef NO_EDITOR_GRAPH_SUPPORT
	PROPERTY_EDIT( m_generateEditorFragmentsForIKSolvers, TXT("Generate editor fragments for IK solvers") );
	PROPERTY_EDIT( m_generateEditorFragmentsForLegIndex, TXT("Starting with 0, negative for all") );
#endif
	PROPERTY_INLINED( m_common, String::EMPTY );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorConstraintNodeFloorIK : public CBehaviorConstraintNodeFloorIKBase
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorConstraintNodeFloorIK, CBehaviorConstraintNodeFloorIKBase, "Constraints", "Floor IK (human)" );

protected:
	SBehaviorConstraintNodeFloorIKVerticalBoneData m_pelvis;
	SBehaviorConstraintNodeFloorIKLegsData m_legs;
	STwoBonesIKSolverData m_leftLegIK;
	STwoBonesIKSolverData m_rightLegIK;

protected:
	TInstanceVar< SBehaviorConstraintNodeFloorIKLeg > i_leftLeg;
	TInstanceVar< SBehaviorConstraintNodeFloorIKLeg > i_rightLeg;
	TInstanceVar< SBehaviorConstraintNodeFloorIKLegs > i_legs;
	TInstanceVar< SBehaviorConstraintNodeFloorIKVerticalBone > i_pelvis;
	TInstanceVar< STwoBonesIKSolver > i_leftLegIK;
	TInstanceVar< STwoBonesIKSolver > i_rightLegIK;
#ifdef DEBUG_FLOOR_IK_TRACES
	TInstanceVar< SBehaviorConstraintNodeFloorIKDebugTrace > i_characterTrace;
	TInstanceVar< SBehaviorConstraintNodeFloorIKDebugTrace > i_characterOffsetTrace;
	TInstanceVar< SBehaviorConstraintNodeFloorIKDebugTrace > i_pelvisTrace;
	TInstanceVar< SBehaviorConstraintNodeFloorIKDebugTrace > i_leftLegTrace;
	TInstanceVar< SBehaviorConstraintNodeFloorIKDebugTrace > i_leftLegBaseTrace;
	TInstanceVar< SBehaviorConstraintNodeFloorIKDebugTrace > i_leftLegHitTrace;
	TInstanceVar< SBehaviorConstraintNodeFloorIKDebugTrace > i_rightLegTrace;
	TInstanceVar< SBehaviorConstraintNodeFloorIKDebugTrace > i_rightLegBaseTrace;
	TInstanceVar< SBehaviorConstraintNodeFloorIKDebugTrace > i_rightLegHitTrace;
#endif

public:
	CBehaviorConstraintNodeFloorIK();

public:
	virtual void OnPropertyPostChange( IProperty* property );

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TXT( "Floor IK (human)" ); }
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

protected:
	virtual void Setup( CBehaviorGraphInstance& instance ) const;

	virtual void UpdateAndSampleIK( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, Float weight, Float prevWeight ) const;

	virtual void GenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;
};

BEGIN_CLASS_RTTI( CBehaviorConstraintNodeFloorIK );
	PARENT_CLASS( CBehaviorConstraintNodeFloorIKBase );
	PROPERTY_INLINED( m_pelvis, String::EMPTY );
	PROPERTY_INLINED( m_legs, String::EMPTY );
	PROPERTY_INLINED( m_leftLegIK, String::EMPTY );
	PROPERTY_INLINED( m_rightLegIK, String::EMPTY );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorConstraintNodeFloorIKHandsOnly : public CBehaviorConstraintNodeFloorIKBase
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorConstraintNodeFloorIKHandsOnly, CBehaviorConstraintNodeFloorIKBase, "Constraints", "Floor IK (human - hands only)" );

protected:
	SBehaviorConstraintNodeFloorIKLegsData m_hands;
	STwoBonesIKSolverData m_leftHandIK;
	STwoBonesIKSolverData m_rightHandIK;

protected:
	TInstanceVar< SBehaviorConstraintNodeFloorIKLeg > i_leftHand;
	TInstanceVar< SBehaviorConstraintNodeFloorIKLeg > i_rightHand;
	TInstanceVar< SBehaviorConstraintNodeFloorIKLegs > i_hands;
	TInstanceVar< STwoBonesIKSolver > i_leftHandIK;
	TInstanceVar< STwoBonesIKSolver > i_rightHandIK;

public:
	CBehaviorConstraintNodeFloorIKHandsOnly();

public:
	virtual void OnPropertyPostChange( IProperty* property );

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TXT( "Floor IK (human - hands only)" ); }
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

protected:
	virtual void Setup( CBehaviorGraphInstance& instance ) const;

	virtual void UpdateAndSampleIK( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, Float weight, Float prevWeight ) const;

	virtual void GenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;
};

BEGIN_CLASS_RTTI( CBehaviorConstraintNodeFloorIKHandsOnly );
	PARENT_CLASS( CBehaviorConstraintNodeFloorIKBase );
	PROPERTY_INLINED( m_hands, String::EMPTY );
	PROPERTY_INLINED( m_leftHandIK, String::EMPTY );
	PROPERTY_INLINED( m_rightHandIK, String::EMPTY );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorConstraintNodeFloorIKBipedLong : public CBehaviorConstraintNodeFloorIKBase
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorConstraintNodeFloorIKBipedLong, CBehaviorConstraintNodeFloorIKBase, "Constraints", "Floor IK (biped long)" );

protected:
	SBehaviorConstraintNodeFloorIKVerticalBoneData m_pelvis;
	SBehaviorConstraintNodeFloorIKLegsData m_legs;
	STwoBonesIKSolverData m_leftLegIK;
	STwoBonesIKSolverData m_rightLegIK;
	SBehaviorConstraintNodeFloorIKVerticalBoneData m_leftShoulder;
	SBehaviorConstraintNodeFloorIKVerticalBoneData m_rightShoulder;
	SBehaviorConstraintNodeFloorIKMaintainLookBoneData m_neck1MaintainLook;
	SBehaviorConstraintNodeFloorIKMaintainLookBoneData m_neck2MaintainLook;
	SBehaviorConstraintNodeFloorIKMaintainLookBoneData m_headMaintainLook;
	Float m_speedForFullyPerpendicularLegs;
	Vector m_upDirAdditionalWS;

protected:
	TInstanceVar< SBehaviorConstraintNodeFloorIKLeg > i_leftLeg;
	TInstanceVar< SBehaviorConstraintNodeFloorIKLeg > i_rightLeg;
	TInstanceVar< SBehaviorConstraintNodeFloorIKLegs > i_legs;
	TInstanceVar< SBehaviorConstraintNodeFloorIKVerticalBone > i_pelvis;
	TInstanceVar< SBehaviorConstraintNodeFloorIKVerticalBone > i_leftShoulder;
	TInstanceVar< SBehaviorConstraintNodeFloorIKVerticalBone > i_rightShoulder;
	TInstanceVar< SBehaviorConstraintNodeFloorIKMaintainLookBone > i_neck1MaintainLook;
	TInstanceVar< SBehaviorConstraintNodeFloorIKMaintainLookBone > i_neck2MaintainLook;
	TInstanceVar< SBehaviorConstraintNodeFloorIKMaintainLookBone > i_headMaintainLook;
	TInstanceVar< STwoBonesIKSolver > i_leftLegIK;
	TInstanceVar< STwoBonesIKSolver > i_rightLegIK;
	TInstanceVar< Float > i_usePerpendicularUprightWS;

public:
	CBehaviorConstraintNodeFloorIKBipedLong();

public:
	virtual void OnPropertyPostChange( IProperty* property );

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TXT( "Floor IK (biped long)" ); }
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

protected:
	virtual void Setup( CBehaviorGraphInstance& instance ) const;

	virtual void UpdateUprightWS( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void UpdateAndSampleIK( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, Float weight, Float prevWeight ) const;

	virtual void GenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;
};

BEGIN_CLASS_RTTI( CBehaviorConstraintNodeFloorIKBipedLong );
	PARENT_CLASS( CBehaviorConstraintNodeFloorIKBase );
	PROPERTY_EDIT( m_speedForFullyPerpendicularLegs, TXT("") );
	PROPERTY_EDIT( m_upDirAdditionalWS, TXT("") );
	PROPERTY_INLINED( m_pelvis, String::EMPTY );
	PROPERTY_INLINED( m_legs, String::EMPTY );
	PROPERTY_INLINED( m_leftLegIK, String::EMPTY );
	PROPERTY_INLINED( m_rightLegIK, String::EMPTY );
	PROPERTY_INLINED( m_leftShoulder, String::EMPTY );
	PROPERTY_INLINED( m_rightShoulder, String::EMPTY );
	PROPERTY_INLINED( m_neck1MaintainLook, String::EMPTY );
	PROPERTY_INLINED( m_neck2MaintainLook, String::EMPTY );
	PROPERTY_INLINED( m_headMaintainLook, String::EMPTY );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorConstraintNodeFloorIKQuadruped : public CBehaviorConstraintNodeFloorIKBase
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorConstraintNodeFloorIKQuadruped, CBehaviorConstraintNodeFloorIKBase, "Constraints", "Floor IK (quadruped)" );

protected:
	SBehaviorConstraintNodeFloorIKVerticalBoneData m_pelvis;
	SBehaviorConstraintNodeFloorIKLegsData m_legs;
	STwoBonesIKSolverData m_leftFrontLegIK;
	STwoBonesIKSolverData m_rightFrontLegIK;
	STwoBonesIKSolverData m_leftBackLegIK;
	STwoBonesIKSolverData m_rightBackLegIK;
	SApplyRotationIKSolverData m_leftFrontLegRotIK;
	SApplyRotationIKSolverData m_rightFrontLegRotIK;
	SBehaviorConstraintNodeFloorIKVerticalBoneData m_leftFrontShoulder;
	SBehaviorConstraintNodeFloorIKVerticalBoneData m_rightFrontShoulder;
	SBehaviorConstraintNodeFloorIKVerticalBoneData m_leftBackShoulder;
	SBehaviorConstraintNodeFloorIKVerticalBoneData m_rightBackShoulder;
	SBehaviorConstraintNodeFloorIKMaintainLookBoneData m_neck1MaintainLook;
	SBehaviorConstraintNodeFloorIKMaintainLookBoneData m_neck2MaintainLook;
	SBehaviorConstraintNodeFloorIKMaintainLookBoneData m_headMaintainLook;
	Float m_speedForFullyPerpendicularLegs;
	Float m_upDirFromFrontAndBackLegsDiffCoef;
	Float m_upDirUseFrontAndBackLegsDiff; // 1 for front back legs diff
	Vector m_upDirAdditionalWS;

protected:
	TInstanceVar< SBehaviorConstraintNodeFloorIKLeg > i_leftFrontLeg;
	TInstanceVar< SBehaviorConstraintNodeFloorIKLeg > i_rightFrontLeg;
	TInstanceVar< SBehaviorConstraintNodeFloorIKLeg > i_leftBackLeg;
	TInstanceVar< SBehaviorConstraintNodeFloorIKLeg > i_rightBackLeg;
	TInstanceVar< SBehaviorConstraintNodeFloorIKLegs > i_legs;
	TInstanceVar< SBehaviorConstraintNodeFloorIKVerticalBone > i_pelvis;
	TInstanceVar< SBehaviorConstraintNodeFloorIKVerticalBone > i_leftFrontShoulder;
	TInstanceVar< SBehaviorConstraintNodeFloorIKVerticalBone > i_rightFrontShoulder;
	TInstanceVar< SBehaviorConstraintNodeFloorIKVerticalBone > i_leftBackShoulder;
	TInstanceVar< SBehaviorConstraintNodeFloorIKVerticalBone > i_rightBackShoulder;
	TInstanceVar< SBehaviorConstraintNodeFloorIKMaintainLookBone > i_neck1MaintainLook;
	TInstanceVar< SBehaviorConstraintNodeFloorIKMaintainLookBone > i_neck2MaintainLook;
	TInstanceVar< SBehaviorConstraintNodeFloorIKMaintainLookBone > i_headMaintainLook;
	TInstanceVar< STwoBonesIKSolver > i_leftFrontLegIK;
	TInstanceVar< STwoBonesIKSolver > i_rightFrontLegIK;
	TInstanceVar< STwoBonesIKSolver > i_leftBackLegIK;
	TInstanceVar< STwoBonesIKSolver > i_rightBackLegIK;
	TInstanceVar< SApplyRotationIKSolver > i_leftFrontLegRotIK;
	TInstanceVar< SApplyRotationIKSolver > i_rightFrontLegRotIK;
	TInstanceVar< Float > i_usePerpendicularUprightWS;
	TInstanceVar< SBehaviorConstraintNodeFloorIKFrontBackWeightHandler > i_frontBackWeightHandler;
	TInstanceVar< SBehaviorConstraintNodeFloorIKLegsIKWeightHandler > i_legsIKWeightHandler;

public:
	CBehaviorConstraintNodeFloorIKQuadruped();

public:
	virtual void OnPropertyPostChange( IProperty* property );

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TXT( "Floor IK (quadruped)" ); }
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

protected:
	virtual void Setup( CBehaviorGraphInstance& instance ) const;

	virtual void UpdateUprightWS( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void UpdateAndSampleIK( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, Float weight, Float prevWeight ) const;

	virtual void GenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;
};

BEGIN_CLASS_RTTI( CBehaviorConstraintNodeFloorIKQuadruped );
	PARENT_CLASS( CBehaviorConstraintNodeFloorIKBase );
	PROPERTY_EDIT( m_speedForFullyPerpendicularLegs, TXT("") );
	PROPERTY_EDIT_RANGE( m_upDirFromFrontAndBackLegsDiffCoef, TXT(""), 0.0f, 1.0f );
	PROPERTY_EDIT_RANGE( m_upDirUseFrontAndBackLegsDiff, TXT(""), 0.0f, 1.0f );
	PROPERTY_EDIT( m_upDirAdditionalWS, TXT("") );
	PROPERTY_INLINED( m_pelvis, String::EMPTY );
	PROPERTY_INLINED( m_legs, String::EMPTY );
	PROPERTY_INLINED( m_leftBackLegIK, String::EMPTY );
	PROPERTY_INLINED( m_rightBackLegIK, String::EMPTY );
	PROPERTY_INLINED( m_leftFrontLegIK, String::EMPTY );
	PROPERTY_INLINED( m_rightFrontLegIK, String::EMPTY );
	PROPERTY_INLINED( m_leftFrontLegRotIK, String::EMPTY );
	PROPERTY_INLINED( m_rightFrontLegRotIK, String::EMPTY );
	PROPERTY_INLINED( m_leftBackShoulder, String::EMPTY );
	PROPERTY_INLINED( m_rightBackShoulder, String::EMPTY );
	PROPERTY_INLINED( m_leftFrontShoulder, String::EMPTY );
	PROPERTY_INLINED( m_rightFrontShoulder, String::EMPTY );
	PROPERTY_INLINED( m_neck1MaintainLook, String::EMPTY );
	PROPERTY_INLINED( m_neck2MaintainLook, String::EMPTY );
	PROPERTY_INLINED( m_headMaintainLook, String::EMPTY );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorConstraintNodeFloorIKSixLegs : public CBehaviorConstraintNodeFloorIKBase
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorConstraintNodeFloorIKSixLegs, CBehaviorConstraintNodeFloorIKBase, "Constraints", "Floor IK (six legs)" );

protected:
	SBehaviorConstraintNodeFloorIKVerticalBoneData m_pelvis;
	SBehaviorConstraintNodeFloorIKLegsData m_legs;
	STwoBonesIKSolverData m_leftFrontLegIK;
	STwoBonesIKSolverData m_rightFrontLegIK;
	STwoBonesIKSolverData m_leftMiddleLegIK;
	STwoBonesIKSolverData m_rightMiddleLegIK;
	STwoBonesIKSolverData m_leftBackLegIK;
	STwoBonesIKSolverData m_rightBackLegIK;
	Float m_usePerpendicularUprightWS;
	Float m_upDirUseFromLegsHitLocs;
	Vector m_upDirAdditionalWS;

protected:
	TInstanceVar< SBehaviorConstraintNodeFloorIKLeg > i_leftFrontLeg;
	TInstanceVar< SBehaviorConstraintNodeFloorIKLeg > i_rightFrontLeg;
	TInstanceVar< SBehaviorConstraintNodeFloorIKLeg > i_leftMiddleLeg;
	TInstanceVar< SBehaviorConstraintNodeFloorIKLeg > i_rightMiddleLeg;
	TInstanceVar< SBehaviorConstraintNodeFloorIKLeg > i_leftBackLeg;
	TInstanceVar< SBehaviorConstraintNodeFloorIKLeg > i_rightBackLeg;
	TInstanceVar< SBehaviorConstraintNodeFloorIKLegs > i_legs;
	TInstanceVar< SBehaviorConstraintNodeFloorIKVerticalBone > i_pelvis;
	TInstanceVar< STwoBonesIKSolver > i_leftFrontLegIK;
	TInstanceVar< STwoBonesIKSolver > i_rightFrontLegIK;
	TInstanceVar< STwoBonesIKSolver > i_leftMiddleLegIK;
	TInstanceVar< STwoBonesIKSolver > i_rightMiddleLegIK;
	TInstanceVar< STwoBonesIKSolver > i_leftBackLegIK;
	TInstanceVar< STwoBonesIKSolver > i_rightBackLegIK;
	TInstanceVar< Float > i_usePerpendicularUprightWS;

public:
	CBehaviorConstraintNodeFloorIKSixLegs();

public:
	virtual void OnPropertyPostChange( IProperty* property );

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TXT( "Floor IK (six legs)" ); }
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

protected:
	virtual void Setup( CBehaviorGraphInstance& instance ) const;

	virtual void UpdateUprightWS( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void UpdateAndSampleIK( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, Float weight, Float prevWeight ) const;

	virtual void GenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;
};

BEGIN_CLASS_RTTI( CBehaviorConstraintNodeFloorIKSixLegs );
	PARENT_CLASS( CBehaviorConstraintNodeFloorIKBase );
	PROPERTY_EDIT( m_usePerpendicularUprightWS, TXT("") );
	PROPERTY_EDIT( m_upDirAdditionalWS, TXT("") );
	PROPERTY_EDIT( m_upDirUseFromLegsHitLocs, TXT("") );
	PROPERTY_INLINED( m_pelvis, String::EMPTY );
	PROPERTY_INLINED( m_legs, String::EMPTY );
	PROPERTY_INLINED( m_leftBackLegIK, String::EMPTY );
	PROPERTY_INLINED( m_rightBackLegIK, String::EMPTY );
	PROPERTY_INLINED( m_leftMiddleLegIK, String::EMPTY );
	PROPERTY_INLINED( m_rightMiddleLegIK, String::EMPTY );
	PROPERTY_INLINED( m_leftFrontLegIK, String::EMPTY );
	PROPERTY_INLINED( m_rightFrontLegIK, String::EMPTY );
END_CLASS_RTTI();
