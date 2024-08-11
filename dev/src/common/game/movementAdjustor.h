/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

struct SMovementAdjustmentRequestTicket;
struct SMovementAdjustmentNotify;
struct SMovementAdjustmentRequest;
struct SMovementAdjustorContext;

typedef Uint32 MoveAdjustmentID;

///////////////////////////////////////////////////////////////////////////////

enum EMovementAdjustmentNotify
{
	MAN_None, // always have default value!
	MAN_LocationAdjustmentReachedDestination,
	MAN_RotationAdjustmentReachedDestination,
	MAN_AdjustmentEnded,
	MAN_AdjustmentCancelled,
};

BEGIN_ENUM_RTTI( EMovementAdjustmentNotify );
	ENUM_OPTION( MAN_None );
	ENUM_OPTION( MAN_LocationAdjustmentReachedDestination );
	ENUM_OPTION( MAN_RotationAdjustmentReachedDestination );
	ENUM_OPTION( MAN_AdjustmentEnded );
	ENUM_OPTION( MAN_AdjustmentCancelled );
END_ENUM_RTTI();

///////////////////////////////////////////////////////////////////////////////

/// Helper class that packs adjusting of location/heading
class CMovementAdjustor : public CObject
						, public IEventHandler< CAnimationEventFired >
{
	DECLARE_ENGINE_CLASS( CMovementAdjustor, CObject, 0 );
	
public:
	CMovementAdjustor();
	~CMovementAdjustor();

	Bool IsRequestActive( const SMovementAdjustmentRequestTicket& requestTicket ) const;
	Bool HasAnyActiveRequest() const { return ! m_requests.Empty(); }
	Bool HasAnyActiveRotationRequests() const;
	Bool HasAnyActiveTranslationRequests() const;

	SMovementAdjustmentRequest* CreateNewRequest( CName name = CName::NONE );
	SMovementAdjustmentRequest* GetRequest( CName name = CName::NONE );
	SMovementAdjustmentRequest* GetRequest( const SMovementAdjustmentRequestTicket& requestTicket );

	void Tick( SMovementAdjustorContext& context, Float deltaSeconds );

	void Cancel( const SMovementAdjustmentRequestTicket& requestTicket );
	void CancelByName( const CName& requestName );
	void CancelAll();

	void GenerateDebugFragments( CRenderFrame* frame, SMovementAdjustorContext& context );

	// tighten movement using those constraints
	void TightenMovementTo( const Vector2& followPoint, Float tighteningDir, Float tighteningRatio );

	// IEventHandler< CAnimationEventFired >
	virtual	void HandleEvent( const CAnimationEventFired &event );

	void AddOneFrameTranslationVelocity(Vector const & _translation);
	void AddOneFrameRotationVelocity(EulerAngles const & _rotation);

private:
	RED_INLINE class CMovingAgentComponent* GetMAC() const;

protected:
	MoveAdjustmentID m_nextID;
	TDynArray< SMovementAdjustmentRequest > m_requests;

	// tightening
	Vector2 m_tighteningFollowPoint;
	Float m_tighteningDir;
	Float m_tighteningRatio;

	// one frame translation/rotation
	Vector m_oneFrameTranslationVelocity;
	EulerAngles m_oneFrameRotationVelocity;

protected:
	RED_INLINE MoveAdjustmentID GetNextID();
	RED_INLINE Bool IsIDUsed( MoveAdjustmentID id );

private:
	SMovementAdjustmentRequest* m_cachedRequest; // speeds up finding same request few times

private:
	friend struct SMovementAdjustmentRequest;
	void BindRequestToEvent( SMovementAdjustmentRequest* request, const CName & eventName );
	void UnbindRequestFromEvent( SMovementAdjustmentRequest* request, const CName & eventName );

	// scripting support
private:
	void funcIsRequestActive( CScriptStackFrame& stack, void* result );
	void funcHasAnyActiveRequest( CScriptStackFrame& stack, void* result );
	void funcHasAnyActiveRotationRequests( CScriptStackFrame& stack, void* result );
	void funcHasAnyActiveTranslationRequests( CScriptStackFrame& stack, void* result );
	void funcCancel( CScriptStackFrame& stack, void* result );
	void funcCancelByName( CScriptStackFrame& stack, void* result );
	void funcCancelAll( CScriptStackFrame& stack, void* result );

	void funcCreateNewRequest( CScriptStackFrame& stack, void* result );
	void funcGetRequest( CScriptStackFrame& stack, void* result );
	
	void funcBlendIn( CScriptStackFrame& stack, void* result );

	void funcDontEnd( CScriptStackFrame& stack, void* result );
	void funcKeepActiveFor( CScriptStackFrame& stack, void* result );
	void funcAdjustmentDuration( CScriptStackFrame& stack, void* result );
	void funcContinuous( CScriptStackFrame& stack, void* result );

	void funcBaseOnNode( CScriptStackFrame& stack, void* result );

	void funcBindToEvent( CScriptStackFrame& stack, void* result );
	void funcBindToEventAnimInfo( CScriptStackFrame& stack, void* result );

	void funcScaleAnimation( CScriptStackFrame& stack, void* result );
	void funcScaleAnimationLocationVertically( CScriptStackFrame& stack, void* result );
	
	void funcDontUseSourceAnimation( CScriptStackFrame& stack, void* result );
	void funcUpdateSourceAnimation( CScriptStackFrame& stack, void* result );
	void funcCancelIfSourceAnimationUpdateIsNotUpdated( CScriptStackFrame& stack, void* result );
	void funcSyncPointInAnimation( CScriptStackFrame& stack, void* result );
	
	void funcUseBoneForAdjustment( CScriptStackFrame& stack, void* result );
	
	void funcMatchEntitySlot( CScriptStackFrame& stack, void* result );

	void funcKeepLocationAdjustmentActive( CScriptStackFrame& stack, void* result );
	void funcReplaceTranslation( CScriptStackFrame& stack, void* result );
	void funcShouldStartAt( CScriptStackFrame& stack, void* result );
	void funcSlideTo( CScriptStackFrame& stack, void* result );
	void funcSlideBy( CScriptStackFrame& stack, void* result );
	void funcSlideTowards( CScriptStackFrame& stack, void* result );
	void funcSlideToEntity( CScriptStackFrame& stack, void* result );
	void funcMaxLocationAdjustmentSpeed( CScriptStackFrame& stack, void* result );
	void funcMaxLocationAdjustmentDistance( CScriptStackFrame& stack, void* result );
	void funcAdjustLocationVertically( CScriptStackFrame& stack, void* result );
	
	void funcKeepRotationAdjustmentActive( CScriptStackFrame& stack, void* result );
	void funcReplaceRotation( CScriptStackFrame& stack, void* result );
	void funcShouldStartFacing( CScriptStackFrame& stack, void* result );
	void funcRotateTo( CScriptStackFrame& stack, void* result );
	void funcRotateBy( CScriptStackFrame& stack, void* result );
	void funcRotateTowards( CScriptStackFrame& stack, void* result );
	void funcMatchMoveRotation( CScriptStackFrame& stack, void* result );
	void funcMaxRotationAdjustmentSpeed( CScriptStackFrame& stack, void* result );
	void funcSteeringMayOverrideMaxRotationAdjustmentSpeed( CScriptStackFrame& stack, void* result );
	
	void funcLockMovementInDirection( CScriptStackFrame& stack, void* result );
	void funcRotateExistingDeltaLocation( CScriptStackFrame& stack, void* result );

	void funcNotifyScript( CScriptStackFrame& stack, void* result );
	void funcDontNotifyScript( CScriptStackFrame& stack, void* result );

	void funcAddOneFrameTranslationVelocity( CScriptStackFrame& stack, void* result );
	void funcAddOneFrameRotationVelocity( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CMovementAdjustor );
	PARENT_CLASS( CObject );
	NATIVE_FUNCTION( "IsRequestActive", funcIsRequestActive );
	NATIVE_FUNCTION( "HasAnyActiveRequest", funcHasAnyActiveRequest );
	NATIVE_FUNCTION( "HasAnyActiveRotationRequests", funcHasAnyActiveRotationRequests );
	NATIVE_FUNCTION( "HasAnyActiveTranslationRequests", funcHasAnyActiveTranslationRequests );
	NATIVE_FUNCTION( "Cancel", funcCancel );
	NATIVE_FUNCTION( "CancelByName", funcCancelByName );
	NATIVE_FUNCTION( "CancelAll", funcCancelAll );

	NATIVE_FUNCTION( "CreateNewRequest", funcCreateNewRequest );
	NATIVE_FUNCTION( "GetRequest", funcGetRequest );

	NATIVE_FUNCTION( "BlendIn", funcBlendIn );
	
	NATIVE_FUNCTION( "DontEnd", funcDontEnd );
	NATIVE_FUNCTION( "KeepActiveFor", funcKeepActiveFor );
	NATIVE_FUNCTION( "AdjustmentDuration", funcAdjustmentDuration );
	NATIVE_FUNCTION( "Continuous", funcContinuous );

	NATIVE_FUNCTION( "BaseOnNode", funcBaseOnNode );

	NATIVE_FUNCTION( "BindToEvent", funcBindToEvent );
	NATIVE_FUNCTION( "BindToEventAnimInfo", funcBindToEventAnimInfo );

	NATIVE_FUNCTION( "ScaleAnimation", funcScaleAnimation );
	NATIVE_FUNCTION( "ScaleAnimationLocationVertically", funcScaleAnimationLocationVertically );
	
	NATIVE_FUNCTION( "DontUseSourceAnimation", funcDontUseSourceAnimation );
	NATIVE_FUNCTION( "UpdateSourceAnimation", funcUpdateSourceAnimation );
	NATIVE_FUNCTION( "CancelIfSourceAnimationUpdateIsNotUpdated", funcCancelIfSourceAnimationUpdateIsNotUpdated );
	NATIVE_FUNCTION( "SyncPointInAnimation", funcSyncPointInAnimation );
	
	NATIVE_FUNCTION( "UseBoneForAdjustment", funcUseBoneForAdjustment );

	NATIVE_FUNCTION( "MatchEntitySlot", funcMatchEntitySlot );
	
	NATIVE_FUNCTION( "KeepLocationAdjustmentActive", funcKeepLocationAdjustmentActive );
	NATIVE_FUNCTION( "ReplaceTranslation", funcReplaceTranslation );
	NATIVE_FUNCTION( "ShouldStartAt", funcShouldStartAt );
	NATIVE_FUNCTION( "SlideTo", funcSlideTo );
	NATIVE_FUNCTION( "SlideBy", funcSlideBy );
	NATIVE_FUNCTION( "SlideTowards", funcSlideTowards );
	NATIVE_FUNCTION( "SlideToEntity", funcSlideToEntity );
	NATIVE_FUNCTION( "MaxLocationAdjustmentSpeed", funcMaxLocationAdjustmentSpeed );
	NATIVE_FUNCTION( "MaxLocationAdjustmentDistance", funcMaxLocationAdjustmentDistance );
	NATIVE_FUNCTION( "AdjustLocationVertically", funcAdjustLocationVertically );

	NATIVE_FUNCTION( "KeepRotationAdjustmentActive", funcKeepRotationAdjustmentActive );
	NATIVE_FUNCTION( "ReplaceRotation", funcReplaceRotation );
	NATIVE_FUNCTION( "ShouldStartFacing", funcShouldStartFacing );
	NATIVE_FUNCTION( "RotateTo", funcRotateTo );
	NATIVE_FUNCTION( "RotateBy", funcRotateBy );
	NATIVE_FUNCTION( "RotateTowards", funcRotateTowards );
	NATIVE_FUNCTION( "MatchMoveRotation", funcMatchMoveRotation );
	NATIVE_FUNCTION( "MaxRotationAdjustmentSpeed", funcMaxRotationAdjustmentSpeed );
	NATIVE_FUNCTION( "SteeringMayOverrideMaxRotationAdjustmentSpeed", funcSteeringMayOverrideMaxRotationAdjustmentSpeed );

	NATIVE_FUNCTION( "LockMovementInDirection", funcLockMovementInDirection );
	NATIVE_FUNCTION( "RotateExistingDeltaLocation", funcRotateExistingDeltaLocation );

	NATIVE_FUNCTION( "NotifyScript", funcNotifyScript );
	NATIVE_FUNCTION( "DontNotifyScript", funcDontNotifyScript );

	NATIVE_FUNCTION( "AddOneFrameTranslationVelocity", funcAddOneFrameTranslationVelocity );
	NATIVE_FUNCTION( "AddOneFrameRotationVelocity", funcAddOneFrameRotationVelocity );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

/// ticket that defines adjustment requests (to cancel them or learn if they were finished or not)
struct SMovementAdjustmentRequestTicket
{
	DECLARE_RTTI_STRUCT( SMovementAdjustmentRequestTicket );
public:
	~SMovementAdjustmentRequestTicket() {}

protected:
	MoveAdjustmentID m_id; // 0, if invalid

	SMovementAdjustmentRequestTicket( MoveAdjustmentID id )
		: m_id( id )
	{}

public:
	SMovementAdjustmentRequestTicket()
		: m_id( 0 )
	{}

public:
	bool IsValid() const { return m_id != 0; }

	static SMovementAdjustmentRequestTicket Invalid() { return SMovementAdjustmentRequestTicket( 0 ); }

	friend class CMovementAdjustor;
	friend struct SMovementAdjustmentRequest;
};

BEGIN_CLASS_RTTI( SMovementAdjustmentRequestTicket );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

struct SMovementAdjustmentNotify
{
public:
	THandle< IScriptable > m_callee;
	CName m_eventName;
	EMovementAdjustmentNotify m_notify;

	SMovementAdjustmentNotify( const THandle< IScriptable >& callee, const CName& eventName, EMovementAdjustmentNotify notify );

	RED_INLINE void Call( const CName& requestName ) const;
};

///////////////////////////////////////////////////////////////////////////////

/// single element of movement adjustment, atomic action to adjust location and/or heading
struct SMovementAdjustmentRequest
{
public:
	SMovementAdjustmentRequestTicket GetTicket() const { return SMovementAdjustmentRequestTicket( m_id ); }
	SMovementAdjustmentRequestTicket GetTicket() { return SMovementAdjustmentRequestTicket( m_id ); }

	// it is advised to check movementAdjustor.ws (script file) as it has most accurate descriptions
	SMovementAdjustmentRequest* KeepActiveFor( Float duration );
	SMovementAdjustmentRequest* AdjustmentDuration( Float duration );
	SMovementAdjustmentRequest* Continuous();
	SMovementAdjustmentRequest* DontEnd();

	SMovementAdjustmentRequest* BlendIn( Float blendInTime );
	
	SMovementAdjustmentRequest* BaseOnNode( const CNode* node );

	SMovementAdjustmentRequest* BindToEvent( CName eventName, Bool adjustDurationOnNextEvent = false );
	SMovementAdjustmentRequest* BindToEventAnimInfo( const SAnimationEventAnimInfo& animInfo, Bool bindOnly = false );

	SMovementAdjustmentRequest* ScaleAnimation( Bool scaleAnimation = true, Bool scaleLocation = true, Bool scaleRotation = false );
	SMovementAdjustmentRequest* ScaleAnimationLocationVertically( Bool scaleAnimationLocationVertically = true );

	SMovementAdjustmentRequest* DontUseSourceAnimation( Bool dontUseSourceAnimation = true );
	SMovementAdjustmentRequest* UpdateSourceAnimation( const SAnimationEventAnimInfo& animInfo );
	SMovementAdjustmentRequest* CancelIfSourceAnimationUpdateIsNotUpdated( Bool cancelIfSourceAnimationUpdateIsNotUpdated = true );
	SMovementAdjustmentRequest* SyncPointInAnimation( Float syncPointTime = -1.0f );
	
	SMovementAdjustmentRequest* UseBoneForAdjustment( CName boneName = CName::NONE, Bool useContinuously = false, Float useBoneForLocationAdjustmentWeight = 1.0f, Float useBoneForRotationAdjustmentWeight = 0.0f, Float useBoneToMatchTargetHeadingWeight = 1.0f );

	SMovementAdjustmentRequest* MatchEntitySlot( const CEntity* entity, const CName& slotName );

	SMovementAdjustmentRequest* KeepLocationAdjustmentActive();
	SMovementAdjustmentRequest* ReplaceTranslation( Bool replaceTranslation = true );
	SMovementAdjustmentRequest* ShouldStartAt( const Vector& targetLocation );
	SMovementAdjustmentRequest* SlideTo( const Vector& targetLocation );
	SMovementAdjustmentRequest* SlideBy( const Vector& slideByDistance );
	SMovementAdjustmentRequest* SlideTowards( const CNode* node, Float minDistance = 0.0f, Float maxDistance = 0.0f );
	SMovementAdjustmentRequest* SlideToEntity( const CEntity* entity, const CName& entityBoneName = CName::NONE, Float minDistance = 0.0f, Float maxDistance = 0.0f );
	SMovementAdjustmentRequest* MaxLocationAdjustmentSpeed( Float locationAdjustmentMaxSpeed, Float locationAdjustmentMaxSpeedZ = 0.0f );
	SMovementAdjustmentRequest* MaxLocationAdjustmentDistance( Bool throughSpeed = false, Float locationAdjustmentMaxDistanceXY = -1.0f, Float locationAdjustmentMaxDistanceZ = -1.0f );
	SMovementAdjustmentRequest* AdjustLocationVertically( Bool adjustLocationVertically = true );

	SMovementAdjustmentRequest* KeepRotationAdjustmentActive();
	SMovementAdjustmentRequest* ReplaceRotation( Bool replaceRotation = true );
	SMovementAdjustmentRequest* ShouldStartFacing( Float targetHeading );
	SMovementAdjustmentRequest* RotateTo( Float targetHeading );
	SMovementAdjustmentRequest* RotateBy( Float byHeading );
	SMovementAdjustmentRequest* RotateTowards( const CNode* node, Float offsetHeading = 0.0f );
	SMovementAdjustmentRequest* MatchMoveRotation();
	SMovementAdjustmentRequest* MaxRotationAdjustmentSpeed( Float rotationAdjustmentMaxSpeed );
	SMovementAdjustmentRequest* SteeringMayOverrideMaxRotationAdjustmentSpeed( Bool steeringMayOverrideMaxRotationAdjustmentSpeed = true );

	SMovementAdjustmentRequest* LockMovementInDirection( Float heading );
	SMovementAdjustmentRequest* RotateExistingDeltaLocation( Bool rotateExistingDeltaLocation = true );

	SMovementAdjustmentRequest* NotifyScript( const THandle< IScriptable >& notifyObject, const CName& eventName, EMovementAdjustmentNotify MAN );
	SMovementAdjustmentRequest* DontNotifyScript( const THandle< IScriptable >& notifyObject, const CName& eventName, EMovementAdjustmentNotify MAN );
	
	void GenerateDebugFragments( CRenderFrame* frame, SMovementAdjustorContext& context, Uint32& yDispl );

private:
	RED_INLINE Bool CalculateAnimDeltaToSyncPoint( SMovementAdjustorContext& context, Vector& animDeltaLocation, EulerAngles& animDeltaRotation );
	RED_INLINE Bool CalculateAnimDelta( SMovementAdjustorContext& context, Vector& animDeltaLocation, EulerAngles& animDeltaRotation, Float startTime, Float endTime );
	RED_INLINE Float GetSyncPoint() const { return m_syncPointTime >= 0.0f? m_syncPointTime : ( m_autoSyncPointTime >= 0.0f? m_autoSyncPointTime : m_sourceAnimation.m_eventEndsAtTime ); }
	RED_INLINE void UpdateBoneTransformMS( SMovementAdjustorContext& context );

	RED_INLINE void UpdateEntitySlotMatrix();
	RED_INLINE void InvalidateEntitySlotMatrix();

public:
	RED_INLINE Bool HasFinished() const { return m_forceFinished || m_cancelled || ( ! m_dontAutoEnd && ( (m_adjustmentDuration != 0.0f && m_adjustmentTimeLeft <= 0.0f) || (m_keepForTimeLeft <= 0.0f) ) ); }
	RED_INLINE void LogReasonOfFinished() const;

	RED_INLINE Bool HasLocationAdjustment() const { return m_adjustLocation != LAT_NONE; }
	RED_INLINE Bool HasRotationAdjustment() const { return m_adjustRotation != RAT_NONE; }

private:
	SMovementAdjustmentRequest( CMovementAdjustor* movementAdjustor, CName name, const SMovementAdjustmentRequestTicket& ticket );

	void Clear();

	void Tick( SMovementAdjustorContext& context, Float deltaSeconds );

	void FinalizeRequest();
	void UnbindFromEvent();

	Vector GetTargetLocation() const;
	Bool GetNodeEntityLocation( Vector& outLoc );
	Matrix GetNodeMatrix() const;

	RED_INLINE void CallEvent( EMovementAdjustmentNotify MAN ) const;
	RED_INLINE void Cancel() { m_cancelled = true; }

	void HandleEvent( const CAnimationEventFired &event );

private:
	enum LocationAdjustmentType
	{
		LAT_NONE,
		LAT_TO_LOCATION,
		LAT_SHOULD_START_AT,
		LAT_BY_VECTOR,
		LAT_AT_DIST_TO_NODE,
		LAT_TO_ENTITY,
		LAT_MATCH_ENTITY_SLOT,
		LAT_KEEP // but don't do anything
	};

	enum RotationAdjustmentType
	{
		RAT_NONE,
		RAT_TO_HEADING,
		RAT_SHOULD_START_FACING,
		RAT_BY_ANGLE,
		RAT_FACE_NODE,
		RAT_MATCH_MOVE_ROTATION,
		RAT_MATCH_ENTITY_SLOT,
		RAT_KEEP // but don't do anything
	};

	CMovementAdjustor* m_movementAdjustor;

	MoveAdjustmentID m_id;
	CName m_name;
	Bool m_firstUpdate;
	Bool m_forceFinished;
	Bool m_cancelled;
	// when values change, these may need another "first update"
	Bool m_locationNeedsUpdate;
	Bool m_rotationNeedsUpdate;

	Vector m_locationAdjustmentSoFar;

	CName m_boundToEvent;
	Bool m_handledEvent; // in case someone binded this to nonexistent event
	Float m_deltaSecondsFromEvent;
	Float m_lastEventTime; // if <0 it is uninitialized

	Bool m_scaleAnimation;
	Bool m_scaleAnimationLocation;
	Bool m_scaleAnimationRotation;
	Bool m_scaleAnimationLocationVertically;

	Bool m_dontUseSourceAnimation;
	SAnimationEventAnimInfo m_sourceAnimation;
	Float m_prevSourceAnimLocalTime;
	Bool m_sourceAnimationUpdated;
	Bool m_skipSourceAnimationUpdateCheck;
	Bool m_adjustDurationOnNextEvent;
	Bool m_cancelIfSourceAnimationUpdateIsNotUpdated;
	Float m_syncPointTime;
	Bool m_autoFindSyncPointTime;
	Float m_autoSyncPointTime;
	Bool m_storeTotalDelta;
	Vector m_totalDeltaLocationMS;
	EulerAngles m_totalDeltaRotation;
	
	CName m_useBoneName;
	Int32 m_useBoneIdx;
	Bool m_updateBoneTransform;
	Bool m_updateBoneTransformContinuously;
	Float m_useBoneForLocationWeight;
	Float m_useBoneForRotationWeight;
	Float m_useBoneToMatchTargetHeadingWeight;
	Vector m_boneLocMS;
	EulerAngles m_boneRotMS;

	Float m_blendInTime;
	Float m_timeActive;

	Bool m_dontAutoEnd;
	Float m_keepForTimeLeft;
	Float m_adjustmentDuration; // if 0, continuous
	Float m_adjustmentTimeLeft;
	THandle< const CNode > m_basedOnNode;

	LocationAdjustmentType m_adjustLocation;
	Bool m_replaceTranslation;
	Vector m_targetLocation;
	Vector m_locationAdjustmentVector;
	Bool m_adjustLocationVertically;
	THandle< const CNode > m_moveToNode;
	THandle< const CEntity > m_moveToEntity;
	CName m_entityBoneName;
	Int32 m_entityBoneIdx;
	CName m_entitySlotName;
	Float m_locationAdjustmentMinDistanceToTarget;
	Float m_locationAdjustmentMaxDistanceToTarget;
	Float m_locationAdjustmentMaxSpeed;
	Float m_locationAdjustmentMaxSpeedZ;
	Float m_locationAdjustmentMaxDistanceThroughSpeed;
	Float m_locationAdjustmentMaxDistanceXY;
	Float m_locationAdjustmentMaxDistanceZ;
	//
	Matrix m_prevNodeMatrix; // used to calculate additional movement if node moves
	Vector m_prevLocationInNodeSpace;
	//
	Float m_reachedDestinationOff;
	Bool m_reachedDestination;
	//
	EntitySlot const * m_entitySlot;
	Bool m_entitySlotMatrixValid;
	Matrix m_entitySlotMatrixWS;

	RotationAdjustmentType m_adjustRotation;
	Bool m_replaceRotation;
	Float m_targetHeading;
	Float m_rotationAdjustmentHeading;
	THandle< const CNode > m_faceNode;
	Float m_rotationAdjustmentMaxSpeed;
	Bool m_steeringMayOverrideMaxRotationAdjustmentSpeed;
	//
	Float m_prevNodeHeading; // used to calculate additional rotation if node rotates
	//
	Float m_reachedRotationOff;
	Bool m_reachedRotation;

	Bool m_lockMovementInDirection;
	Float m_lockedMovementHeading; // character will move in this direction/heading
	Bool m_rotateExistingDeltaLocation;

	TDynArray< SMovementAdjustmentNotify > m_notifies;

private:
	Bool IsContinuous() const { return m_adjustmentDuration == 0.0f; }
	Bool IsBoundedToEvent( const CName& eventName ) const { return m_boundToEvent == eventName; }
	Bool IsBoundedToAnyEvent() const { return ! m_boundToEvent.Empty(); }

	// friends
	friend class CMovementAdjustor;
};

///////////////////////////////////////////////////////////////////////////////

struct SMovementAdjustorContext
{
public:
	const class CMovingAgentComponent* m_mac;

public:
	Vector m_currentLocation;
	EulerAngles m_currentRotation;
	Vector m_currentDeltaLocation;
	EulerAngles m_currentDeltaRotation;
	Float m_rotationAdjustmentMaxSpeedFromSteering;

public:
	Bool m_replaceDeltaLocation;
	Bool m_replaceDeltaRotation;
	Vector m_outputDeltaLocation;
	EulerAngles m_outputDeltaRotation;
	EulerAngles m_turnExistingDeltaLocation;

public:
	SMovementAdjustorContext( const class CMovingAgentComponent* mac, const Vector& deltaLocation, const EulerAngles& deltaRotation );

protected:
	void ClearOutput();
	void FinalizeAdjustments();

	// friends
	friend class CMovementAdjustor;
};
