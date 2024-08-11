#pragma once

#include "../../common/engine/springDampers.h"

class ICustomCameraBaseController : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( ICustomCameraBaseController, CObject )

protected:
	CName	m_controllerName;

public:
	RED_INLINE Bool operator==( const ICustomCameraBaseController& a ) const	{ return m_controllerName == a.m_controllerName; }
	RED_INLINE Bool operator!=( const ICustomCameraBaseController& a ) const	{ return m_controllerName != a.m_controllerName; }
	RED_INLINE Bool operator< ( const ICustomCameraBaseController& a ) const	{ return m_controllerName < a.m_controllerName; }
	RED_INLINE Bool operator> ( const ICustomCameraBaseController& a ) const	{ return m_controllerName > a.m_controllerName; }
	RED_INLINE Bool operator<=( const ICustomCameraBaseController& a ) const	{ return m_controllerName <= a.m_controllerName; }
	RED_INLINE Bool operator>=( const ICustomCameraBaseController& a ) const	{ return m_controllerName >= a.m_controllerName; }

	RED_INLINE Bool operator==( const CName& name ) const	{ return m_controllerName == name; }
	RED_INLINE Bool operator!=( const CName& name ) const	{ return m_controllerName != name; }

	RED_INLINE CName GetControllerName() { return m_controllerName; }
};

BEGIN_ABSTRACT_CLASS_RTTI( ICustomCameraBaseController )
	PARENT_CLASS( CObject )
	PROPERTY_EDIT( m_controllerName, TXT("Controller name") )
END_CLASS_RTTI()


//////////////////////////////////////////////////////////////////////////
// PIVOT POSITION CONTROLLERS
//////////////////////////////////////////////////////////////////////////


class ICustomCameraPivotPositionController : public ICustomCameraBaseController
{
	DECLARE_ENGINE_ABSTRACT_CLASS( ICustomCameraPivotPositionController, ICustomCameraBaseController )

protected:
	Float					m_offsetZ;
	Float					m_pivotZSmoothTime;
	TFloatCriticalDampRef	m_dampPivotZ;

public:
	ICustomCameraPivotPositionController();

	// Called when controller is activated
	virtual void Activate( const Vector& currentPosition, Float currentOffsetZ ) { m_dampPivotZ.Force( currentPosition.Z ); }

	// Called when controller is deactivated
	virtual void Deactivate() {};

	// Updates the controller, it moves the position to the desired one within its internal constraints
	virtual void Update( Vector& currentPosition, Vector& currentVelocity, Float timeDelta ) = 0;

	// Set the desired position that will be approached within each update
	virtual void SetDesiredPosition( const Vector& position, Float mult = 1.f ) = 0;

	virtual Float GetCurrentZOffset() const = 0;

	virtual void GenerateDebugFragments( CRenderFrame* frame ) {};

	RED_INLINE Float GetZOffset() { return m_offsetZ; }

	RED_INLINE void SetZOffset( Float offset ) { m_offsetZ = offset; }
private:
	void funcSetDesiredPosition( CScriptStackFrame& stack, void* result );
	void funcUpdate( CScriptStackFrame& stack, void* result );
};

BEGIN_ABSTRACT_CLASS_RTTI( ICustomCameraPivotPositionController )
	PARENT_CLASS( ICustomCameraBaseController )
	PROPERTY_EDIT( m_offsetZ, String::EMPTY )
	PROPERTY_EDIT( m_pivotZSmoothTime, String::EMPTY )
	NATIVE_FUNCTION( "SetDesiredPosition", funcSetDesiredPosition )
	NATIVE_FUNCTION( "Update", funcUpdate )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CCustomCameraRopePPC : public ICustomCameraPivotPositionController, public Red::System::NonCopyable
{
	DECLARE_ENGINE_CLASS( CCustomCameraRopePPC, ICustomCameraPivotPositionController, 0 )

private:
	Float	m_mult;
	Float	m_dampFactor;
	Float	m_ropeLength;
	Float	m_smoothZ;

protected:

	TVectorCriticalDampRef	m_damp;
	TFloatCriticalDampRef	m_dampZ;

	Vector	m_desiredPosition;

public:
	CCustomCameraRopePPC();

	virtual void Update( Vector& currentPosition, Vector& currentVelocity, Float timeDelta );

	virtual void Activate( const Vector& currentPosition, Float currentOffsetZ ) { TBaseClass::Activate( currentPosition, currentOffsetZ ); m_dampZ.Force( currentOffsetZ ); }

	RED_INLINE virtual void SetDesiredPosition( const Vector& position, Float mult = 1.f )
	{
		m_desiredPosition = position;
		if( mult > 0.f ) m_mult = m_dampFactor / mult;
		else m_mult = FLT_MAX;
	}

	virtual Float GetCurrentZOffset() const { return m_dampZ.GetValue(); }

protected:

	void UpdateStep( Vector& currentPosition, Vector& currentVelocity, Float timeDelta );
};

BEGIN_CLASS_RTTI( CCustomCameraRopePPC )
	PARENT_CLASS( ICustomCameraPivotPositionController )
	PROPERTY_EDIT( m_dampFactor, TXT("How fast the controller will reach its desired position") )
	PROPERTY_EDIT( m_smoothZ, TXT("Smooth time for the Z offset change") )
	PROPERTY_EDIT( m_ropeLength, String::EMPTY )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CCustomCameraPlayerPPC : public ICustomCameraPivotPositionController, public Red::System::NonCopyable
{
	DECLARE_ENGINE_CLASS( CCustomCameraPlayerPPC, ICustomCameraPivotPositionController, 0 )

private:
	Float	m_mult;
	Float	m_dampFactor;
	Float	m_smoothZ;

	TVectorCriticalDampRef	m_damp;
	TFloatCriticalDampRef	m_dampZ;
	
public:
	CCustomCameraPlayerPPC();

	virtual void Update( Vector& currentPosition, Vector& currentVelocity, Float timeDelta );

	virtual void Activate( const Vector& currentPosition, Float currentOffsetZ ) { TBaseClass::Activate( currentPosition, currentOffsetZ ); m_dampZ.Force( currentOffsetZ ); }

	RED_INLINE virtual void SetDesiredPosition( const Vector& position, Float mult = 1.f )
	{
		if( mult > 0.f ) m_mult = m_dampFactor / mult;
		else m_mult = FLT_MAX;
	}

	virtual Float GetCurrentZOffset() const { return m_dampZ.GetValue(); }
};

BEGIN_CLASS_RTTI( CCustomCameraPlayerPPC )
	PARENT_CLASS( ICustomCameraPivotPositionController )
	PROPERTY_EDIT( m_dampFactor, TXT("How fast the controller will reach its desired position") )
	PROPERTY_EDIT( m_smoothZ, TXT("Smooth time for the Z offset change") )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CCustomCameraBlendPPC : public ICustomCameraPivotPositionController
{
	DECLARE_ENGINE_CLASS( CCustomCameraBlendPPC, ICustomCameraPivotPositionController, 0 )

private:
	ICustomCameraPivotPositionController*	m_from;
	ICustomCameraPivotPositionController*	m_to;
	Float									m_weight;
	Float									m_blendTime;

	ICustomCameraPivotPositionController**	m_ownerRef;

public:
	CCustomCameraBlendPPC();

	virtual void Activate( const Vector& currentPosition, Float currentOffsetZ );

	virtual void Update( Vector& currentPosition, Vector& currentVelocity, Float timeDelta );

	virtual void SetDesiredPosition( const Vector& position, Float mult = 1.f );

	virtual Float GetCurrentZOffset() const;

	virtual void GenerateDebugFragments( CRenderFrame* frame );

	void Setup( ICustomCameraPivotPositionController* from, ICustomCameraPivotPositionController* to, Float blendTime );

	RED_INLINE ICustomCameraPivotPositionController* GetDestinationController() { return m_to; }

	RED_INLINE Bool IsFinishedBlending() { return m_weight >= 0.999f; }

	// This is magic
	RED_INLINE void SetOwner( CCustomCameraBlendPPC& other ) { m_ownerRef = &other.m_from; }
};

BEGIN_CLASS_RTTI( CCustomCameraBlendPPC )
	PARENT_CLASS( ICustomCameraPivotPositionController )
	PROPERTY( m_from )
	PROPERTY( m_to )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CCustomCameraBoatPPC : public CCustomCameraRopePPC
{
	DECLARE_ENGINE_CLASS( CCustomCameraBoatPPC, CCustomCameraRopePPC, 0 )

	Vector m_offset;
public:
	CCustomCameraBoatPPC();

	virtual void Update( Vector& currentPosition, Vector& currentVelocity, Float timeDelta ) override;

	void funcSetPivotOffset( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CCustomCameraBoatPPC )
	PARENT_CLASS( CCustomCameraRopePPC )
	NATIVE_FUNCTION( "SetPivotOffset", funcSetPivotOffset )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////
// PIVOT ROTATION CONTROLLERS
//////////////////////////////////////////////////////////////////////////


class ICustomCameraPivotRotationController : public ICustomCameraBaseController
{
	DECLARE_ENGINE_ABSTRACT_CLASS( ICustomCameraPivotRotationController, ICustomCameraBaseController )

public:
	enum ECCPIVOTROTATION_FLAG
	{
		ECCPRF_StandStill			= 0,
		ECCPRF_RotateRight			= FLAG( 0 ),
		ECCPRF_RotateLeft			= FLAG( 1 ),
		ECCPRF_RotateUp				= FLAG( 2 ),
		ECCPRF_RotateDown			= FLAG( 3 ),
		ECCPRF_DesiredYaw			= FLAG( 4 ),
		ECCPRF_DesiredPitch			= FLAG( 5 ),
		ECCPRF_ManualHorizontal		= FLAG( 6 ),
		ECCPRF_ManualVertical		= FLAG( 7 ),
		ECCPRF_AbsoluteHorizontal	= FLAG( 8 ),
		ECCPRF_AbsoluteVertical		= FLAG( 9 ),

		ECCPRF_HorizontalRotation	= ECCPRF_RotateLeft | ECCPRF_RotateRight | ECCPRF_DesiredYaw,
		ECCPRF_VerticalRotation		= ECCPRF_RotateDown | ECCPRF_RotateUp | ECCPRF_DesiredPitch,
	};

protected:
	Uint32	m_flags;

	Float	m_minPitch;
	Float	m_maxPitch;

	EInputSensitivityPreset	m_sensitivityPreset;

public:
	RED_INLINE void	SetRotationFlags( Uint32 flags )	 { m_flags = flags; }
	RED_INLINE void	SetRotationFlag( Uint32 flag )		 { m_flags |= flag; }
	RED_INLINE void	ClearRotationFlag( Uint32 flag )	 { m_flags &= ~flag; }
														 
	RED_INLINE Uint32	GetRotationFlags() const		 { return m_flags; }
	RED_INLINE Bool	HasRotationFlag( Uint32 flag ) const { return (m_flags & flag) != 0; }

	// Called when controller is activated
	virtual void Activate( const EulerAngles& currentRotation, Uint32 flags );

	// Called when controller is deactivated
	virtual void Deactivate() {};

	// Updates the controller, it rotates to the desired values within constraints
	virtual void Update( EulerAngles& currentRotation, EulerAngles& currentVelocity, Float timeDelta ) = 0;

	// Set the desired yaw rotation
	virtual void SetDesiredYaw( Float yaw, Float mult = 1.f ) = 0;

	// Set the desired pitch rotation
	virtual void SetDesiredPitch( Float pitch, Float mult = 1.f ) = 0;

	virtual void RotateHorizontal( Bool right, Float mult = 1.f ) = 0;
	virtual void RotateVertical( Bool up, Float mult = 1.f ) = 0;
	virtual void StopRotating() = 0;

	// Get calculated rotation delta (yaw & pitch only)
	virtual EulerAngles GetRotationDelta() = 0;

	virtual void UpdateInput( Bool& movedHorizontal, Bool& movedVertical ) = 0;

	virtual void GenerateDebugFragments( CRenderFrame* frame ) {};

private:
	void funcSetDesiredHeading( CScriptStackFrame& stack, void* result );
	void funcSetDesiredPitch( CScriptStackFrame& stack, void* result );
	void funcRotateHorizontal( CScriptStackFrame& stack, void* result );
	void funcRotateVertical( CScriptStackFrame& stack, void* result );
	void funcStopRotating( CScriptStackFrame& stack, void* result );
	void funcUpdate( CScriptStackFrame& stack, void* result );
};

BEGIN_ABSTRACT_CLASS_RTTI( ICustomCameraPivotRotationController )
	PARENT_CLASS( ICustomCameraBaseController )
	PROPERTY_EDIT( m_minPitch, String::EMPTY )
	PROPERTY_EDIT( m_maxPitch, String::EMPTY )
	PROPERTY_EDIT( m_sensitivityPreset, String::EMPTY )
	NATIVE_FUNCTION( "SetDesiredHeading", funcSetDesiredHeading )
	NATIVE_FUNCTION( "SetDesiredPitch", funcSetDesiredPitch )
	NATIVE_FUNCTION( "RotateHorizontal", funcRotateHorizontal )
	NATIVE_FUNCTION( "RotateVertical", funcRotateVertical )
	NATIVE_FUNCTION( "StopRotating", funcStopRotating )
	NATIVE_FUNCTION( "Update", funcUpdate )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CCustomCameraDefaultPRC : public ICustomCameraPivotRotationController, public Red::System::NonCopyable
{
	DECLARE_ENGINE_CLASS( CCustomCameraDefaultPRC, ICustomCameraPivotRotationController, 0 )

protected:
	Float			m_dampYawFactor;
	Float			m_dampPitchFactor;
	Float			m_desiredYaw;
	Float			m_desiredPitch;

	Float			m_yawSmooth;
	Float			m_pitchSmooth;
	Float			m_horMult;
	Float			m_verMult;

	TFloatAngleCriticalDampRef	m_yawDamp;
	TFloatAngleCriticalDampRef	m_pitchDamp;

	Float			m_absoluteYawDelta;
	Float			m_absolutePitchDelta;

	Float			m_yawAcceleration;
	Float			m_yawMaxVelocity;
	Float			m_pitchAcceleration;
	Float			m_pitchMaxVelocity;

	EulerAngles		m_rotationDelta;

public:
	CCustomCameraDefaultPRC();

	virtual void Update( EulerAngles& currentRotation, EulerAngles& currentVelocity, Float timeDelta );

	virtual void SetDesiredYaw( Float yaw, Float mult = 1.f );
	virtual void SetDesiredPitch( Float pitch, Float mult = 1.f );

	virtual void RotateHorizontal( Bool right, Float mult = 1.f );
	virtual void RotateVertical( Bool up, Float mult = 1.f );
	virtual void StopRotating();

	RED_INLINE virtual EulerAngles GetRotationDelta() { return m_rotationDelta; }

	virtual void UpdateInput( Bool& movedHorizontal, Bool& movedVertical );

private:
	RED_INLINE void ApplyDeceleration( Float& velocity, Float deceleration, Float target = 0.0f )
	{
		velocity = velocity > 0.f ? Max( velocity - deceleration, target ) : Min( velocity + deceleration, target );
	}

	RED_INLINE void RotateAbsolute( Float yaw, Float pitch );
};

BEGIN_CLASS_RTTI( CCustomCameraDefaultPRC )
	PARENT_CLASS( ICustomCameraPivotRotationController )
	PROPERTY_EDIT( m_dampYawFactor, TXT("How fast the controller will reach its desired horizontal rotation") )
	PROPERTY_EDIT( m_dampPitchFactor, TXT("How fast the controller will reach its desired vertical rotation") )
	PROPERTY_EDIT( m_yawMaxVelocity, String::EMPTY )
	PROPERTY_EDIT( m_yawAcceleration, String::EMPTY )
	PROPERTY_EDIT( m_pitchAcceleration, String::EMPTY )
	PROPERTY_EDIT( m_pitchMaxVelocity, String::EMPTY )
END_CLASS_RTTI()


//////////////////////////////////////////////////////////////////////////
// PIVOT DISTANCE CONTROLLERS
//////////////////////////////////////////////////////////////////////////


class ICustomCameraPivotDistanceController : public ICustomCameraBaseController
{
	DECLARE_ENGINE_ABSTRACT_CLASS( ICustomCameraPivotDistanceController, ICustomCameraBaseController )

protected:
	Float	m_minDist;
	Float	m_maxDist;

public:
	// Called when controller is activated
	virtual void Activate( Float currentDistance ) {};

	// Called when controller is deactivated
	virtual void Deactivate() {};

	// Updates the controller, it zooms to the desired values within constraints
	virtual void Update( Float& currentDistance, Float& currentVelocity, Float timeDelta ) = 0;

	// Set the desired distance of the camera from pivot
	virtual void SetDesiredDistance( Float distance, Float mult = 1.f ) = 0;

	virtual void GenerateDebugFragments( CRenderFrame* frame ) {};

private:
	void funcSetDesiredDistance( CScriptStackFrame& stack, void* result );
	void funcUpdate( CScriptStackFrame& stack, void* result );
};

BEGIN_ABSTRACT_CLASS_RTTI( ICustomCameraPivotDistanceController )
	PARENT_CLASS( ICustomCameraBaseController )
	PROPERTY_EDIT( m_minDist, String::EMPTY )
	PROPERTY_EDIT( m_maxDist, String::EMPTY )
	NATIVE_FUNCTION( "SetDesiredDistance", funcSetDesiredDistance )
	NATIVE_FUNCTION( "Update", funcUpdate )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CCustomCameraDefaultPDC : public ICustomCameraPivotDistanceController, public Red::System::NonCopyable
{
	DECLARE_ENGINE_CLASS( CCustomCameraDefaultPDC, ICustomCameraPivotDistanceController, 0 )

private:
	Float	m_mult;
	Float	m_dampFactor;
	Float	m_desiredDistance;

	TFloatCriticalDampRef	m_damp;

public:
	CCustomCameraDefaultPDC();

	virtual void Activate( Float currentDistance );

	virtual void Update( Float& currentDistance, Float& currentVelocity, Float timeDelta );

	RED_INLINE virtual void SetDesiredDistance( Float distance, Float mult = 1.f )
	{
		m_desiredDistance = Clamp( distance, m_minDist, m_maxDist );
		if( mult > 0.f ) m_mult = m_dampFactor / mult;
		else m_mult = FLT_MAX;
	}
};

BEGIN_CLASS_RTTI( CCustomCameraDefaultPDC )
	PARENT_CLASS( ICustomCameraPivotDistanceController )
	PROPERTY_EDIT( m_dampFactor, TXT("How fast the controller will reach its desired distance") )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CCustomCameraAdditivePDC : public CCustomCameraDefaultPDC
{
	DECLARE_ENGINE_CLASS( CCustomCameraAdditivePDC, CCustomCameraDefaultPDC, 0 )

private:
	Float	m_addedValue;

public:
	CCustomCameraAdditivePDC();

	RED_INLINE virtual void SetDesiredDistance( Float distance, Float mult = 1.f )
	{
		TBaseClass::SetDesiredDistance( distance + m_addedValue, mult );
	}
};

BEGIN_CLASS_RTTI( CCustomCameraAdditivePDC )
	PARENT_CLASS( CCustomCameraDefaultPDC )
	PROPERTY_EDIT( m_addedValue, TXT("Value that is added to SetDesiredDistance call") )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////
// CAMERA POSITION CONTROLLERS
//////////////////////////////////////////////////////////////////////////


struct SCameraMovementData
{
	DECLARE_RTTI_STRUCT( SCameraMovementData );

	THandle<CCustomCamera>							m_camera;
	THandle<ICustomCameraPivotPositionController>	m_pivotPositionController;
	THandle<ICustomCameraPivotRotationController>	m_pivotRotationController;
	THandle<ICustomCameraPivotDistanceController>	m_pivotDistanceController;
	Vector											m_pivotPositionValue;
	Vector											m_pivotPositionVelocity;
	EulerAngles										m_pivotRotationValue;
	EulerAngles										m_pivotRotationVelocity;
	Float											m_pivotDistanceValue;
	Float											m_pivotDistanceVelocity;
	Vector											m_cameraLocalSpaceOffset;
	Vector											m_cameraLocalSpaceOffsetVel;
	Vector											m_cameraOffset;

	SCameraMovementData()
		: m_camera( NULL )
		, m_pivotPositionController( NULL )
		, m_pivotRotationController( NULL )
		, m_pivotDistanceController( NULL )
		, m_pivotPositionValue( Vector::ZERO_3D_POINT )
		, m_pivotPositionVelocity( Vector::ZEROS )
		, m_pivotRotationValue( EulerAngles::ZEROS )
		, m_pivotRotationVelocity( EulerAngles::ZEROS )
		, m_pivotDistanceValue( 3.f )
		, m_pivotDistanceVelocity( 0.f )
		, m_cameraLocalSpaceOffset( Vector::ZEROS )
		, m_cameraLocalSpaceOffsetVel( Vector::ZEROS )
		, m_cameraOffset( Vector::ZEROS )
		{}
};

BEGIN_CLASS_RTTI( SCameraMovementData )
	PROPERTY( m_camera )
	PROPERTY( m_pivotPositionController )
	PROPERTY( m_pivotRotationController )
	PROPERTY( m_pivotDistanceController )
	PROPERTY( m_pivotPositionValue )
	PROPERTY( m_pivotPositionVelocity )
	PROPERTY( m_pivotRotationValue )
	PROPERTY( m_pivotRotationVelocity )
	PROPERTY( m_pivotDistanceValue )
	PROPERTY( m_pivotDistanceVelocity )
	PROPERTY( m_cameraLocalSpaceOffset )
	PROPERTY( m_cameraLocalSpaceOffsetVel )
	PROPERTY( m_cameraOffset )
END_CLASS_RTTI()

struct SCustomCameraPreset
{
	DECLARE_RTTI_STRUCT( SCustomCameraPreset );

	CName	m_name;
	Float	m_explorationDistance;
	Vector	m_explorationOffset;
};

BEGIN_CLASS_RTTI( SCustomCameraPreset )
	PROPERTY_EDIT_NAME( m_name, TXT("pressetName" ), TXT("Preset Name") )
	PROPERTY_EDIT_NAME( m_explorationDistance, TXT("distance" ), TXT("Camera distance in exploration") )
	PROPERTY_EDIT_NAME( m_explorationOffset, TXT("offset" ), TXT("Localspace offset of the camera while not in combat") )
END_CLASS_RTTI()

class ICustomCameraPositionController : public ICustomCameraBaseController
{
	DECLARE_ENGINE_ABSTRACT_CLASS( ICustomCameraPositionController, ICustomCameraBaseController )
protected:
	Bool	m_enableAutoCollisionAvoidance;
	Bool	m_enableScreenSpaceCorrections;
	SCustomCameraPreset	m_currentPreset;

public:
	virtual void Activate( SCameraMovementData& currData ) {};
	virtual void Activate( SCameraMovementData& currData, const SCameraMovementData& prevData ) {};

	virtual void PreUpdate( CCustomCamera& camera, Float timeDelta ) {};
	virtual void Update( SCameraMovementData& moveData, Float timeDelta ) = 0;

	virtual Vector		GetPosition() = 0;
	virtual EulerAngles	GetRotation() = 0;

	RED_INLINE void SetPreset( const SCustomCameraPreset& preset ) { m_currentPreset = preset; }
	RED_INLINE const SCustomCameraPreset& GetPreset() const { return m_currentPreset; }

	RED_INLINE void EnableScreenSpaceCorrections( Bool enable ) { m_enableScreenSpaceCorrections = enable; }

	virtual void SetColisionOriginOffset( const Vector& offset ) {};

	virtual void SetDesiredOffset( const Vector& offset ) {};

	virtual void Reset() {};
	virtual void ResetColliisons() {};

	virtual void OnBeforeStartBlendFrom( SCameraMovementData& out, const ICamera::Data& cameraData );

	virtual void GenerateDebugFragments( CRenderFrame* frame ) {};

protected:
	RED_INLINE Bool CallGameCameraEvent( CObject* context, const CName& event, SCameraMovementData& moveData, Float timeDelta )
	{
		Bool returnValue = false;

		if( CallFunctionRef1Val1Ret( context, event, moveData, timeDelta, returnValue ) )
		{
			return returnValue;
		}

		return false;
	}
};

BEGIN_ABSTRACT_CLASS_RTTI( ICustomCameraPositionController )
	PARENT_CLASS( ICustomCameraBaseController )
	PROPERTY( m_enableAutoCollisionAvoidance )
	PROPERTY( m_enableScreenSpaceCorrections )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class ICustomCameraCollisionController;

class CCustomCameraSimplePositionController : public ICustomCameraPositionController
{
	DECLARE_ENGINE_CLASS( CCustomCameraSimplePositionController, ICustomCameraPositionController, 0 )

private:
	Vector		m_finalPosition;
	EulerAngles	m_finalRotation;

	// Collisions
	ICustomCameraCollisionController*	m_collisionController;
	ICustomCameraCollisionController*	m_collisionController2;

	//////////////////////////////////////////////////////////////////////////

public:
	CCustomCameraSimplePositionController();

	virtual void OnPostLoad();

	virtual void Update( SCameraMovementData& moveData, Float timeDelta );

	RED_INLINE virtual Vector			GetPosition()	{ return m_finalPosition; }
	RED_INLINE virtual EulerAngles	GetRotation()	{ return m_finalRotation; }

	virtual void GenerateDebugFragments( CRenderFrame* frame );

	virtual void SetColisionOriginOffset( const Vector& offset );
};

BEGIN_CLASS_RTTI( CCustomCameraSimplePositionController )
	PARENT_CLASS( ICustomCameraPositionController )
	PROPERTY_NOSERIALIZE( m_collisionController )
	PROPERTY_NOSERIALIZE( m_collisionController2 )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CTrailerCameraPositionController : public ICustomCameraPositionController
{
	DECLARE_ENGINE_CLASS( CTrailerCameraPositionController, ICustomCameraPositionController, 0 )

private:
	Bool		m_pullCamera;
	Float		m_defaultPitch;
	Float		m_defaultZOffset;

	Float					m_offsetSmoothTime;
	TVectorCriticalDampRef	m_offsetDamp;

	ICustomCameraCollisionController*	m_collisionController;
	Vector								m_defaultCollisionOriginOffset;

	Vector		m_prevPos;

	Vector		m_finalPosition;
	EulerAngles	m_finalRotation;

	//////////////////////////////////////////////////////////////////////////

public:
	CTrailerCameraPositionController();

	virtual void Activate( SCameraMovementData& currData );
	virtual void Activate( SCameraMovementData& currData, const SCameraMovementData& prevData );

	virtual void OnPostLoad();

	virtual void PreUpdate( CCustomCamera& camera, Float timeDelta );
	virtual void Update( SCameraMovementData& moveData, Float timeDelta );

	RED_INLINE virtual Vector			GetPosition()	{ return m_finalPosition; }
	RED_INLINE virtual EulerAngles	GetRotation()	{ return m_finalRotation; }
};

BEGIN_CLASS_RTTI( CTrailerCameraPositionController )
	PARENT_CLASS( ICustomCameraPositionController )
	PROPERTY_EDIT( m_defaultPitch, String::EMPTY )
	PROPERTY_EDIT( m_defaultZOffset, String::EMPTY )
	PROPERTY_EDIT( m_offsetSmoothTime, TXT("Smooth time for damping camera localspace offset") )
	PROPERTY_NOSERIALIZE( m_collisionController )
	PROPERTY_EDIT( m_defaultCollisionOriginOffset, TXT("Origin of the collision checks") )
END_CLASS_RTTI()


//////////////////////////////////////////////////////////////////////////
// CAMERA SCRIPTED CONTROLLERS
//////////////////////////////////////////////////////////////////////////


RED_DECLARE_NAME( ControllerActivate );
RED_DECLARE_NAME( ControllerDeactivate );
RED_DECLARE_NAME( ControllerUpdate );
RED_DECLARE_NAME( ControllerSetDesiredPosition );

class ICustomCameraScriptedPivotPositionController : public ICustomCameraPivotPositionController
{
	DECLARE_ENGINE_ABSTRACT_CLASS( ICustomCameraScriptedPivotPositionController, ICustomCameraPivotPositionController )

protected:
	ICustomCameraScriptedPivotPositionController() {};

public:
	virtual void Activate( const Vector& currentPosition, Float currentOffsetZ );				// scripted function ControllerActivate
	virtual void Deactivate();																	// scripted function ControllerDeactivate
	
	virtual void Update( Vector& currentPosition, Vector& currentVelocity, Float timeDelta );	// scripted function ControllerUpdate

	virtual void SetDesiredPosition( const Vector& position, Float mult = 1.f );				// scripted function ControllerSetDesiredPosition

	// TODO: implement when needed
	virtual Float GetCurrentZOffset() const { return m_offsetZ; }
};

BEGIN_CLASS_RTTI( ICustomCameraScriptedPivotPositionController )
	PARENT_CLASS( ICustomCameraPivotPositionController )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

RED_DECLARE_NAME( ControllerSetDesiredYaw );
RED_DECLARE_NAME( ControllerSetDesiredPitch );
RED_DECLARE_NAME( ControllerRotateHorizontal );
RED_DECLARE_NAME( ControllerRotateVertical );
RED_DECLARE_NAME( ControllerStopRotating );
RED_DECLARE_NAME( ControllerGetRotationDelta );
RED_DECLARE_NAME( ControllerUpdateInput );

class ICustomCameraScriptedPivotRotationController : public ICustomCameraPivotRotationController
{
	DECLARE_ENGINE_ABSTRACT_CLASS( ICustomCameraScriptedPivotRotationController, ICustomCameraPivotRotationController )

protected:
	ICustomCameraScriptedPivotRotationController() {};

public:
	virtual void Activate( const EulerAngles& currentRotation, Uint32 flags );							// scripted function ControllerActivate
	virtual void Deactivate();																			// scripted function ControllerDeactivate

	virtual void Update( EulerAngles& currentRotation, EulerAngles& currentVelocity, Float timeDelta );	// scripted function ControllerUpdate

	virtual void SetDesiredYaw( Float yaw, Float mult = 1.f );											// scripted function ControllerSetDesiredYaw
	virtual void SetDesiredPitch( Float pitch, Float mult = 1.f );										// scripted function ControllerSetDesiredPitch

	virtual void RotateHorizontal( Bool right, Float mult = 1.f );										// scripted function ControllerRotateHorizontal
	virtual void RotateVertical( Bool up, Float mult = 1.f );											// scripted function ControllerRotateVertical
	virtual void StopRotating();																		// scripted function ControllerStopRotating

	virtual EulerAngles GetRotationDelta();																// scripted function ControllerGetRotationDelta

	virtual void UpdateInput( Bool& movedHorizontal, Bool& movedVertical );								// scripted function ControllerUpdateInput
};

BEGIN_CLASS_RTTI( ICustomCameraScriptedPivotRotationController )
	PARENT_CLASS( ICustomCameraPivotRotationController )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

RED_DECLARE_NAME( ControllerSetDesiredDistance );

class ICustomCameraScriptedPivotDistanceController : public ICustomCameraPivotDistanceController
{
	DECLARE_ENGINE_ABSTRACT_CLASS( ICustomCameraScriptedPivotDistanceController, ICustomCameraPivotDistanceController )

protected:
	ICustomCameraScriptedPivotDistanceController() {};
	
public:
	virtual void Activate( Float currentDistance );											// scripted function ControllerActivate
	virtual void Deactivate();																// scripted function ControllerDeactivate

	virtual void Update( Float& currentDistance, Float& currentVelocity, Float timeDelta );	// scripted function ControllerUpdate

	virtual void SetDesiredDistance( Float distance, Float mult = 1.f );					// scripted function ControllerSetDesiredDistance
};

BEGIN_CLASS_RTTI( ICustomCameraScriptedPivotDistanceController )
	PARENT_CLASS( ICustomCameraPivotDistanceController )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

RED_DECLARE_NAME( ControllerGetPosition );
RED_DECLARE_NAME( ControllerGetRotation );

class ICustomCameraScriptedPositionController : public ICustomCameraPositionController
{
	DECLARE_ENGINE_ABSTRACT_CLASS( ICustomCameraScriptedPositionController, ICustomCameraPositionController )

protected:
	ICustomCameraScriptedPositionController() {};

public:
	virtual void Update( SCameraMovementData& moveData, Float timeDelta );					// scripted function ControllerUpdate

	virtual Vector		GetPosition();														// scripted function ControllerGetPosition
	virtual EulerAngles	GetRotation();														// scripted function ControllerGetRotation
};

BEGIN_CLASS_RTTI( ICustomCameraScriptedPositionController )
	PARENT_CLASS( ICustomCameraPositionController )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class ICustomCameraScriptedCurveSetPivotPositionController : public ICustomCameraScriptedPivotPositionController
{
	DECLARE_ENGINE_CLASS( ICustomCameraScriptedCurveSetPivotPositionController, ICustomCameraScriptedPivotPositionController, 0 )

protected:
	TDynArray< CCurve* >	m_curveSet;
	TDynArray< CName >		m_curveNames;

public:
	virtual void OnPropertyPostChange( IProperty* property );

protected:
	void funcFindCurve( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( ICustomCameraScriptedCurveSetPivotPositionController )
	PARENT_CLASS( ICustomCameraScriptedPivotPositionController )
	PROPERTY_CUSTOM_EDIT_ARRAY( m_curveSet, TXT("Curve set"), TXT("CurveSelection") )
	PROPERTY_EDIT( m_curveNames, TXT("Curve names") )
	NATIVE_FUNCTION( "FindCurve", funcFindCurve )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class ICustomCameraScriptedCurveSetPivotRotationController : public ICustomCameraScriptedPivotRotationController
{
	DECLARE_ENGINE_CLASS( ICustomCameraScriptedCurveSetPivotRotationController, ICustomCameraScriptedPivotRotationController, 0 )

protected:
	TDynArray< CCurve* >	m_curveSet;
	TDynArray< CName >		m_curveNames;

public:
	virtual void OnPropertyPostChange( IProperty* property );

protected:
	void funcFindCurve( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( ICustomCameraScriptedCurveSetPivotRotationController )
	PARENT_CLASS( ICustomCameraScriptedPivotRotationController )
	PROPERTY_CUSTOM_EDIT_ARRAY( m_curveSet, TXT("Curve set"), TXT("CurveSelection") )
	PROPERTY_EDIT( m_curveNames, TXT("Curve names") )
	NATIVE_FUNCTION( "FindCurve", funcFindCurve )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class ICustomCameraScriptedCurveSetPivotDistanceController : public ICustomCameraScriptedPivotDistanceController
{
	DECLARE_ENGINE_CLASS( ICustomCameraScriptedCurveSetPivotDistanceController, ICustomCameraScriptedPivotDistanceController, 0 )

protected:
	TDynArray< CCurve* >	m_curveSet;
	TDynArray< CName >		m_curveNames;

public:
	virtual void OnPropertyPostChange( IProperty* property );

protected:
	void funcFindCurve( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( ICustomCameraScriptedCurveSetPivotDistanceController )
	PARENT_CLASS( ICustomCameraScriptedPivotDistanceController )
	PROPERTY_CUSTOM_EDIT_ARRAY( m_curveSet, TXT("Curve set"), TXT("CurveSelection") )
	PROPERTY_EDIT( m_curveNames, TXT("Curve names") )
	NATIVE_FUNCTION( "FindCurve", funcFindCurve )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class ICustomCameraScriptedCurveSetPositionController : public ICustomCameraScriptedPositionController
{
	DECLARE_ENGINE_CLASS( ICustomCameraScriptedCurveSetPositionController, ICustomCameraScriptedPositionController, 0 )

protected:
	TDynArray< CCurve* >	m_curveSet;
	TDynArray< CName >		m_curveNames;

public:
	virtual void OnPropertyPostChange( IProperty* property );

protected:
	void funcFindCurve( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( ICustomCameraScriptedCurveSetPositionController )
	PARENT_CLASS( ICustomCameraScriptedPositionController )
	PROPERTY_CUSTOM_EDIT_ARRAY( m_curveSet, TXT("Curve set"), TXT("CurveSelection") )
	PROPERTY_EDIT( m_curveNames, TXT("Curve names") )
	NATIVE_FUNCTION( "FindCurve", funcFindCurve )
END_CLASS_RTTI()


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


struct SCameraDistanceInfo
{
	DECLARE_RTTI_STRUCT( SCameraDistanceInfo );

	Float	m_minDistance;
	Float	m_distanceRange;
	Float	m_enemiesMaxDistanceToCamera;
	Float	m_enemiesMaxDistanceToPlayer;
	Float	m_standardDeviationRelevance;
	Float	m_cameraZOffset;
	Float	m_cameraZOffsetRange;
};

BEGIN_CLASS_RTTI( SCameraDistanceInfo )
	PROPERTY_EDIT( m_minDistance, TXT("Minimum distance of camera") )
	PROPERTY_EDIT( m_distanceRange, TXT("How much distance to add to minDistance") )
	PROPERTY_EDIT( m_enemiesMaxDistanceToCamera, TXT("When enemy is between Player and camera, higher value means camera will reach max distance earlier (formula =  1- currentDistance/m_enemiesMaxDistanceToCamera )") )
	PROPERTY_EDIT( m_enemiesMaxDistanceToPlayer, TXT("When enemy is on side edge of screen, the lower the value the faster camera reaches max range") )
	PROPERTY_EDIT( m_standardDeviationRelevance, TXT("if enemies spread X meters from average position, it adds X * m_standardDeviationRelevance meter to camera distance") )
	PROPERTY_EDIT( m_cameraZOffset, TXT("default offset of camera") )
	PROPERTY_EDIT( m_cameraZOffsetRange, TXT("the further the camera the more offset") )
END_CLASS_RTTI()


class CCombatCameraPositionController : public ICustomCameraPositionController
{
	DECLARE_ENGINE_CLASS( CCombatCameraPositionController, ICustomCameraPositionController, 0 )

private:
	Vector		m_finalPosition;
	EulerAngles	m_finalRotation;

	Bool		m_isResetScheduled;

	Float					m_baseSmoothTime;
	Float					m_smoothTime;
	Float					m_offsetSmoothTime;
	TVectorCriticalDampRef	m_offsetDamp;

	Float		m_defaultCameraAngle;
	Float		m_defaultCameraZOffset;
	Float		m_flipCameraAngle;
	
	CCurve*		m_followRotation;
	CCurve*		m_followRotationSprint;
	CCurve*		m_followRotationFlip;
	CCurve*		m_slopeCameraAngleChange;
	CCurve*		m_slopeAngleCameraSpaceMultiplier;
	CCurve*		m_slopeResetTimeout;
	CCurve*		m_cameraPivotDampMult;

	TFloatCriticalDampRef	m_pivotMultDamp;

	Float		m_slopeTimer;

#define SlopeHistorySize 30
	Float		m_slopeHistory[ SlopeHistorySize ];
	Uint32		m_currHistory;

	Float							m_combatPivotDampMult;
	Float							m_bigMonsterHeightThreshold;
	Float							m_180FlipThreshold;
	Bool							m_flipTriggered;

	CName							m_explorationRotationCtrlName;
	CName							m_combatRotationCtrlName;

	Float							m_combatPitch;
	TDynArray<SCameraDistanceInfo>	m_combatEnemiesToDistanceMap;
	Uint32							m_bigMonsterCountMultiplier;
	CCurve*							m_monsterSizeAdditiveOffset;
	CCurve*							m_monsterSizeAdditivePitch;

	CCurve*							m_1v1Pitch;
	CCurve*							m_1v1AdditivePitch;
	CCurve*							m_1v1BigMonsterPitch;
	CCurve*							m_1v1BMAdditivePitch;
	CCurve*							m_1v1Distance;
	CCurve*							m_1v1SignificanceAddDistance;
	CCurve*							m_1v1ZOffset;
	CCurve*							m_1v1BigMonsterZOffset;
	Float							m_1v1PivotMultiplier;
	Float							m_1v1KeepAngle;
	Float							m_1v1OffScreenMult;

	CName							m_oneOnOneCtrlName;

	Float							m_screenSpaceXRatio;
	Float							m_screenSpaceYRatio;

	//Screenspace corrections
	Float							m_ssCorrectionXTreshold;
	Float							m_ssCorrectionYTreshold;
	Float							m_ssPivotCorrSmooth;
	Float							m_ssPivotCorrActualSmooth;
	Vector							m_ssPivotCorrVelocity;
	TVectorCriticalDampRef			m_ssPivotCorrection;
	Float							m_ssDistCorrSmooth;
	Float							m_ssDistCorrActualSmooth;
	Float							m_ssDistCorrVelocity;
	TFloatCriticalDampRef			m_ssDistCorrection;

	// Temp
	Bool							m_useExplorationCamInSprint;

	// Collisions
	ICustomCameraCollisionController*	m_collisionController;
	ICustomCameraCollisionController*	m_collisionController2;

	Vector								m_defaultCollisionOriginOffset;

	// Debug
#ifndef RED_FINAL_BUILD
	Float	m_dbgPlayerCamSpaceHeading;
	Float	m_dbgCurrentSlope;
	SCameraMovementData m_dbgMoveData;
#endif

	//////////////////////////////////////////////////////////////////////////

public:
	CCombatCameraPositionController();

	virtual void Activate( SCameraMovementData& currData );
	virtual void Activate( SCameraMovementData& currData, const SCameraMovementData& prevData );

	virtual void OnPostLoad();

	virtual void PreUpdate( CCustomCamera& camera, Float timeDelta );
	virtual void Update( SCameraMovementData& moveData, Float timeDelta );

	RED_INLINE virtual Vector			GetPosition()	{ return m_finalPosition; }
	RED_INLINE virtual EulerAngles	GetRotation()	{ return m_finalRotation; }

	RED_INLINE virtual void SetDesiredOffset( const Vector& offset ) { m_offsetDamp.SetDestValue( offset ); }

	virtual void Reset() { m_isResetScheduled = true; }

	virtual void ResetColliisons();

	void OnBeforeStartBlendFrom( SCameraMovementData& out, const ICamera::Data& cameraData ) override;

	virtual void GenerateDebugFragments( CRenderFrame* frame );

	virtual void SetColisionOriginOffset( const Vector& offset );

private:
	void GetHostileEnemies( CR4Player* player, TDynArray< THandle<CActor> >& hostileEnemies );
	void PreUpdateLockedToTarget( CR4Player* player, Float timeDelta, SCameraMovementData& moveData, CCustomCamera& camera, const CActor* moveTarget );
	void PreUpdateNonCombat( CR4Player* player, Float timeDelta, SCameraMovementData& moveData, CCustomCamera& camera, const Vector& playerVelocity, Float playerSpeed, Bool isSprinting );
	void PreUpdateCombat( CR4Player* player, Float timeDelta, SCameraMovementData& moveData, CCustomCamera& camera, const CActor* moveTarget );
	void PreUpdateNoEnemies( CR4Player* player, Float timeDelta, SCameraMovementData& moveData );
	void PreUpdateEnemies( CR4Player* player, Float timeDelta, SCameraMovementData& moveData, CCustomCamera& camera, TDynArray< THandle<CActor> >& enemies );
	void ResetHistory();
	Float CalcNewSlopeValue( Float currentSlope );
};

BEGIN_CLASS_RTTI( CCombatCameraPositionController )
	PARENT_CLASS( ICustomCameraPositionController )
	PROPERTY_EDIT( m_defaultCameraAngle, TXT("Default camera angle (no slopes, no enemies etc.)") )
	PROPERTY_EDIT( m_defaultCameraZOffset, TXT("Default camera Z offset (no combat)") )
	PROPERTY_EDIT( m_flipCameraAngle, TXT("Camera angle when running 'towards' camera (when flip is triggered)") )
	PROPERTY_CUSTOM_EDIT( m_followRotation, TXT("Curve for camera following player based on player heading in camera space"), TXT("CurveSelection") )
	PROPERTY_CUSTOM_EDIT( m_followRotationSprint, TXT("Curve for camera following player based on player heading in camera space"), TXT("CurveSelection") )
	PROPERTY_CUSTOM_EDIT( m_followRotationFlip, TXT("Curve for camera following player after reaching flipThreshold angle"), TXT("CurveSelection") )
	PROPERTY_CUSTOM_EDIT( m_slopeCameraAngleChange, TXT("Desired camera angle based on slope angle"), TXT("CurveSelection") )
	PROPERTY_CUSTOM_EDIT( m_slopeAngleCameraSpaceMultiplier, TXT("Multiplier for the slope camera angle based on player-camera heading diff"), TXT("CurveSelection") )
	PROPERTY_CUSTOM_EDIT( m_slopeResetTimeout, TXT("Timeout when stand still based on current slope camera angle"), TXT("CurveSelection") )
	PROPERTY_CUSTOM_EDIT( m_cameraPivotDampMult, TXT("Camera pivot position damp multiplier based on player-camera heading diff"), TXT("CurveSelection") )
	PROPERTY_EDIT( m_combatPivotDampMult, TXT("Multiplier for pivot position smooth time when in combat") )
	PROPERTY_EDIT( m_bigMonsterHeightThreshold, TXT("Height threshold to consider monster as 'Big'") )
	PROPERTY_EDIT( m_180FlipThreshold, TXT("I don't know how to explain it... sorry") )
	PROPERTY_EDIT( m_explorationRotationCtrlName, TXT("Pivot rotation controller name when in exploration") )
	PROPERTY_EDIT( m_combatRotationCtrlName, TXT("Pivot rotation controller name when in combat") )
	PROPERTY_EDIT( m_offsetSmoothTime, TXT("Smooth time for damping camera localspace offset") )
	PROPERTY_EDIT( m_combatPitch, TXT("Camera pitch when in combat") )
	PROPERTY_EDIT( m_combatEnemiesToDistanceMap, TXT("Distance based on number of enemies - array index indicates number of enemies + 1") )
	PROPERTY_EDIT( m_bigMonsterCountMultiplier, TXT("For how many normal enemies does a big monster count") )
	PROPERTY_CUSTOM_EDIT( m_monsterSizeAdditiveOffset, TXT("Camera additive Z offset based on the most significant enemy"), TXT("CurveSelection") )
	PROPERTY_CUSTOM_EDIT( m_monsterSizeAdditivePitch, TXT("Camera additive pitch based on the most significant enemy"), TXT("CurveSelection") )
	PROPERTY_CUSTOM_EDIT( m_1v1Pitch, TXT("Camera pitch when 1v1 based on pitch angle to enemy"), TXT("CurveSelection") )
	PROPERTY_CUSTOM_EDIT( m_1v1AdditivePitch, TXT("Camera additive pitch when 1v1 based on distance to enemy"), TXT("CurveSelection") )
	PROPERTY_CUSTOM_EDIT( m_1v1BigMonsterPitch, TXT("Camera pitch when 1v1 based on pitch angle to enemy"), TXT("CurveSelection") )
	PROPERTY_CUSTOM_EDIT( m_1v1BMAdditivePitch, TXT("Camera additive pitch when 1v1 based on distance to enemy"), TXT("CurveSelection") )
	PROPERTY_CUSTOM_EDIT( m_1v1Distance, TXT("Camera distance when 1v1 based on enemy capsule height"), TXT("CurveSelection") )
	PROPERTY_CUSTOM_EDIT( m_1v1SignificanceAddDistance, TXT("Camera additive distance when 1v1 based on enemy significance"), TXT("CurveSelection") )
	PROPERTY_CUSTOM_EDIT( m_1v1ZOffset, TXT("Camera Z-Offset when 1v1 based on pitch angle to enemy"), TXT("CurveSelection") )
	PROPERTY_CUSTOM_EDIT( m_1v1BigMonsterZOffset, TXT("Camera Z-Offset when 1v1 based on pitch angle to enemy"), TXT("CurveSelection") )
	PROPERTY_EDIT( m_1v1PivotMultiplier, TXT("Pivot damp multiplier when 1v1") )
	PROPERTY_EDIT( m_1v1KeepAngle, TXT("Camera angle span relative to direction towards enemy when 1v1") )
	PROPERTY_EDIT( m_1v1OffScreenMult, TXT("Camera speed multiplier when enemy is out of screen-space bounds") )
	PROPERTY_EDIT( m_oneOnOneCtrlName, TXT("Pivot rotation controller name only one enemy") )
	PROPERTY_EDIT( m_screenSpaceXRatio, TXT("Screen space X ratio affecting camera to target rotation") )
	PROPERTY_EDIT( m_screenSpaceYRatio, TXT("Screen space Y ratio affecting camera to target rotation") )
	PROPERTY_EDIT( m_ssCorrectionXTreshold, TXT("X axis threshold for player screen space correction") )
	PROPERTY_EDIT( m_ssCorrectionYTreshold, TXT("Y axis threshold for player screen space correction") )
	PROPERTY_EDIT( m_ssPivotCorrSmooth, TXT("Smooth for screen space correction pivot movement") )
	PROPERTY_EDIT( m_ssDistCorrSmooth, TXT("Smooth for screen space correction distance movement") )
	PROPERTY_EDIT( m_useExplorationCamInSprint, TXT("Use exploration camera setup when sprinting in combat") )
	PROPERTY_NOSERIALIZE( m_collisionController )
	PROPERTY_NOSERIALIZE( m_collisionController2 )
	PROPERTY_EDIT( m_defaultCollisionOriginOffset, TXT("Origin of the collision checks") )
END_CLASS_RTTI()
