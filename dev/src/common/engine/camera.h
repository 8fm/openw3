/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behaviorGraphPoseSlotListener.h"
#include "cameraComponent.h"
#include "animMath.h"
#include "entity.h"
#include "../core/countedBool.h"

class CBehaviorGraphStack;
class CBehaviorGraphPoseSlotNode;

RED_DECLARE_NAME( LOOK_AT_ACTIVATION );
RED_DECLARE_NAME( LOOK_AT_TARGET );
RED_DECLARE_NAME( LOOK_AT_ACT_DURATION );

RED_DECLARE_NAME( FOLLOW_ACTIVATION );
RED_DECLARE_NAME( FOLLOW_TARGET );

RED_DECLARE_NAME( ANGLE_UP_DOWN );
RED_DECLARE_NAME( ANGLE_LEFT_RIGHT );
RED_DECLARE_NAME( RESET_ANGLE_UP_DOWN );
RED_DECLARE_NAME( RESET_ANGLE_LEFT_RIGHT );

RED_DECLARE_NAME( FOCUS_ACTIVATION );
RED_DECLARE_NAME( FOCUS_TARGET );
RED_DECLARE_NAME( FOCUS_ACT_DURATION );

RED_DECLARE_NAME( ZOOM );

RED_DECLARE_NAME( DOF_OVERRIDE );
RED_DECLARE_NAME( DOF_FOCUS_DIST_FAR );
RED_DECLARE_NAME( DOF_BLUR_DIST_FAR );
RED_DECLARE_NAME( DOF_INTENSITY );
RED_DECLARE_NAME( DOF_FOCUS_DIST_NEAR );
RED_DECLARE_NAME( DOF_BLUR_DIST_NEAR );

RED_DECLARE_NAME( EVT_RESET_ROT_HOR );
RED_DECLARE_NAME( EVT_RESET_ROT_VER );
RED_DECLARE_NAME( EVT_HARD_RESET_ROT_HOR );
RED_DECLARE_NAME( EVT_HARD_RESET_ROT_VER );
RED_DECLARE_NAME( VAR_RESET_ROT_DURATION );
RED_DECLARE_NAME( HOR_FOLLOW_VAR );

class CCamera : public CEntity, public ICamera
{
	DECLARE_ENGINE_CLASS( CCamera, CEntity, 0 )

public:
	CCamera();

	//! Camera was attached to world
	virtual void OnAttached( CWorld* world );

	//! Camera was detached from world
	virtual void OnDetached( CWorld* world );

	//! All components of entity has been attached
	virtual void OnAttachFinished( CWorld* world );

	//! Camera was loaded
	virtual void OnPostLoad();

	//! Process behavior output
	virtual void OnProcessBehaviorPose( const CAnimatedComponent* poseOwner, const SBehaviorGraphOutput& pose );
	
	//////////////////////////////////////////////////////////////////////////
	// ICamera implementation
	 
	virtual Bool Update( Float timeDelta );

	virtual Bool GetData( Data& outData ) const override;
	virtual Bool SetData( const Data& data ) override;

	//////////////////////////////////////////////////////////////////////////

public:
	//! Activate camera's selected camera component
	void SetActive( Float blendTime = 0.f );

	//! Is selected camera component active
	Bool IsActive() const;

	//! Is selected camera on stack
	Bool IsOnStack() const;

	//! Freeze
	void Freeze();

	//! Unfreeze
	void Unfreeze();

	//! Is frozen
	Bool IsFrozen() const;

	//! Reset
	void Reset( Bool resetFollowing = false );

	//! Set fov
	void SetFov( Float fov );

	//! Get fov
	Float GetFov() const;

	//! Set DOF params
	void SetDofParams( const SDofParams& param );

	//! Set BokehDOF params
	void SetBokehDofParams( const SBokehDofParams& param );

	//! Reset DOF params
	void ResetDofParams();

	//! Get DOF params
	SDofParams GetDofParams() const;

	//! Get BokehDOF params
	SBokehDofParams GetBokehDofParams() const;

	//! Zoom
	virtual void SetZoom( Float factor );

	//! Get zoom factor
	Float GetZoom() const;

	//! Get rotation angles
	Bool GetRotationAngles( Float& updown, Float& leftRight ) const;

	//! Get direction
	Bool GetCameraDirection( Vector& dir ) const;

	//! Get eye position
	Bool GetCameraPosition( Vector& pos ) const;

	//! Get wished eye position
	Bool GetCameraWishedPosition( Vector& pos ) const;

	//! Get look at target position
	Bool GetCameraLookAtPosition( Vector& pos ) const;

	//! Get camera position in world space
	Bool GetCameraMatrixWorldSpace( Matrix& transform ) const;

	//! Get camera orbit position in world space
	Bool GetCameraOrbitWorldSpace( Matrix& transform ) const;

	//! Get camera component position
	const Vector& GetCameraComponentPosition() const;

	//! Get camera component yaw
	Float GetCameraComponentYaw() const;

	//! Get distance to camera ray
	Float GetDistanceToCameraRay( const Vector& testPos, Vector* linePoint = NULL ) const;

	//! Get selected camera component from camera
	CCameraComponent* GetSelectedCameraComponent();

	//! Get selected camera component from camera ( const )
	const CCameraComponent* GetSelectedCameraComponent() const;

	//! Get distance to target
	Float GetDistanceToTargetRatio() const;

	//! Get distance to target
	Float GetDistanceToTargetRatio01() const;

public:
	//! Rotate - use behavior for rotating
	virtual void Rotate( Float leftRightSpeed, Float upDownSpeed );

	//! Reset rotation
	virtual void ResetRotation( Bool smoothly = true, Bool horizontal = true, Bool vertical = true, Float duration = 0.5f );
	virtual void ResetRotation( Bool smoothly, Float horAngle, Float verAngle, Float duration = 0.5f );

	//! Rotation lock/unlock, counted
	void LockRotation();
	void UnlockRotation();

	//! Follow - use behavior
	virtual void Follow( CEntity* dest );
	virtual void FollowWithRotation( CEntity* dest, Int32 boneIndex );

	virtual Bool FollowDeactivation();
	virtual Bool IsFollowing() const;
	virtual Vector GetFollowTargetPosition() const;
	virtual Float GetFollowTargetYaw() const;
	virtual Bool HasFollowTarget( const CEntity* entity ) const;

	//! Look at target
	void LookAt( const Vector& staticTarget, Float duration = 0.f, Float activationTime = 0.f );
	void LookAt( const CNode* target, Float duration = 0.f, Float activationTime = 0.f );
	void LookAt( const CAnimatedComponent* target, const String& bone, Float duration = 0.f, Float activationTime = 0.f );

	Bool	LookAtDeactivation( Float deactivationTime = 0.0f );
	Bool	HasLookAt() const;
	Vector	GetLookAtTargetPosition();

	//! Focus on target
	void FocusOn( const Vector& staticTarget, Float duration = 0.f, Float activationTime = 0.f );
	void FocusOn( const CNode* target, Float duration = 0.f, Float activationTime = 0.f );
	void FocusOn( const CAnimatedComponent* target, const String& bone, Float duration = 0.f, Float activationTime = 0.f );

	Bool	FocusDeactivation( Float deactivationTime = 0.0f );
	Bool	IsFocused() const;
	Vector	GetFocusTargetPosition();

	//! Use with care!
	void ForceBehaviorCameraPose();

	//! Reload input config
	virtual void ReloadInputConfig();

protected:
	virtual void InitializeCamera();
	virtual void ResetMovementData();

protected:
	void InternalReset();

	Bool CanWork() const;
	Bool IsConstraintEnabled( const CName varAct, const CName varDur  = CName::NONE ) const;
	Bool DeactivateConstraint( const CName varAct, const CName varDur = CName::NONE, Float duration = 0.f );
	Bool HasConstraint( const CName varAct ) const;
	Vector GetConstraintTarget( const CName varAct ) const;

	CBehaviorGraphStack* GetCameraBehStack() const;

	void DeactivateAllConstraints();
	void SelectDefaultCameraComponent();

	void CacheDistToTargetRef();

	Bool IsRotationAllowed() const;
	void ResetRotationAlpha();

	void ProcessFollowTarget();
	Float CalcDistToTarget() const;

	void SetFollowTarget( CEntity* target );
	void FollowRotation( Bool flag );

	void SyncRotationsFromConstraintedPose();

protected:
	CCameraComponent*	m_selectCamera;			//!< Select camera component
	Float				m_prevFOV;

	CEntity*			m_followTarget;

	Bool				m_allowRotation;		//!< Is rotation allowed
	CountedBool			m_rotationLocked;		//!< Rotation is locked from outside world
	Float				m_rotationAlpha;		//!< Sensitivity filter

	Float				m_moveForwardBackward;	//!< Move forward/backward
	Float				m_moveLeftRight;		//!< Move left/right
	Float				m_moveUpDown;			//!< Move up/down
	Float				m_rotateUpDown;			//!< Rotate up/down
	Float				m_rotateLeftRight;		//!< Rotate left/right

	Float				m_distToTargetRef;		//! Distance to target
	Float				m_distToTargetRatio;	//! Distance to target ratio ( no target = 1 )

	Int32					m_boneEye;				//!< Cached index of camera eye position bone
	Int32					m_boneEyeWished;		//!< Cached index of camera wished eye position bone
	Int32					m_boneLookAt;			//!< Cached index of camera center position bone
	Int32					m_boneOrbit;			//!< Cached index of camera orbit bone ( camera skeleton direction )

	static const Float	COLLISION_WEIGHT_TRESHOLD_UP;
	static const Float	COLLISION_WEIGHT_TRESHOLD_DOWN;

private:
	void funcRotate( CScriptStackFrame& stack, void* result );
	void funcFollow( CScriptStackFrame& stack, void* result );
	void funcFollowWithRotation( CScriptStackFrame& stack, void* result );
	void funcLookAt( CScriptStackFrame& stack, void* result );
	void funcLookAtStatic( CScriptStackFrame& stack, void* result );
	void funcLookAtBone( CScriptStackFrame& stack, void* result );
	void funcLookAtDeactivation( CScriptStackFrame& stack, void* result );
	void funcHasLookAt( CScriptStackFrame& stack, void* result );
	void funcGetLookAtTargetPosition( CScriptStackFrame& stack, void* result );
	void funcFocusOn( CScriptStackFrame& stack, void* result );
	void funcFocusOnStatic( CScriptStackFrame& stack, void* result );
	void funcFocusOnBone( CScriptStackFrame& stack, void* result );
	void funcFocusDeactivation( CScriptStackFrame& stack, void* result );
	void funcIsFocused( CScriptStackFrame& stack, void* result );
	void funcGetFocusTargetPosition( CScriptStackFrame& stack, void* result );
	void funcSetActive( CScriptStackFrame& stack, void* result );
	void funcIsActive( CScriptStackFrame& stack, void* result );
	void funcIsOnStack( CScriptStackFrame& stack, void* result );
	void funcGetCameraDirection( CScriptStackFrame& stack, void* result );
	void funcGetCameraPosition( CScriptStackFrame& stack, void* result );
	void funcGetCameraMatrixWorldSpace( CScriptStackFrame& stack, void* result );	
	void funcSetFov( CScriptStackFrame& stack, void* result );
	void funcGetFov( CScriptStackFrame& stack, void* result );
	void funcSetZoom( CScriptStackFrame& stack, void* result );
	void funcGetZoom( CScriptStackFrame& stack, void* result );
	void funcReset( CScriptStackFrame& stack, void* result );
	void funcResetRotation( CScriptStackFrame& stack, void* result );
	void funcResetRotationTo( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CCamera );
	PARENT_CLASS( CEntity );
	NATIVE_FUNCTION( "Rotate", funcRotate );
	NATIVE_FUNCTION( "Follow", funcFollow );
	NATIVE_FUNCTION( "FollowWithRotation", funcFollowWithRotation );
	NATIVE_FUNCTION( "LookAt", funcLookAt );
	NATIVE_FUNCTION( "LookAtStatic", funcLookAtStatic );
	NATIVE_FUNCTION( "LookAtBone", funcLookAtBone );
	NATIVE_FUNCTION( "LookAtDeactivation", funcLookAtDeactivation );
	NATIVE_FUNCTION( "HasLookAt", funcHasLookAt );
	NATIVE_FUNCTION( "GetLookAtTargetPosition", funcGetLookAtTargetPosition );
	NATIVE_FUNCTION( "FocusOn", funcFocusOn );
	NATIVE_FUNCTION( "FocusOnStatic", funcFocusOnStatic );
	NATIVE_FUNCTION( "FocusOnBone", funcFocusOnBone );
	NATIVE_FUNCTION( "FocusDeactivation", funcFocusDeactivation );
	NATIVE_FUNCTION( "IsFocused", funcIsFocused );
	NATIVE_FUNCTION( "GetFocusTargetPosition", funcGetFocusTargetPosition );
	NATIVE_FUNCTION( "SetActive", funcSetActive );
	NATIVE_FUNCTION( "IsActive", funcIsActive );
	NATIVE_FUNCTION( "IsOnStack", funcIsOnStack );
	NATIVE_FUNCTION( "GetCameraDirection", funcGetCameraDirection );
	NATIVE_FUNCTION( "GetCameraPosition", funcGetCameraPosition );
	NATIVE_FUNCTION( "GetCameraMatrixWorldSpace", funcGetCameraMatrixWorldSpace );
	NATIVE_FUNCTION( "SetFov", funcSetFov );
	NATIVE_FUNCTION( "GetFov", funcGetFov );
	NATIVE_FUNCTION( "SetZoom", funcSetZoom );
	NATIVE_FUNCTION( "GetZoom", funcGetZoom );
	NATIVE_FUNCTION( "Reset", funcReset );
	NATIVE_FUNCTION( "ResetRotation", funcResetRotation );
	NATIVE_FUNCTION( "ResetRotationTo", funcResetRotationTo );
END_CLASS_RTTI();

#ifndef NO_EDITOR
class ICameraClipboard
{
public:
	virtual void Copy( const Vector& position, const EulerAngles& rotation )=0;
	virtual void Paste( Vector& position, EulerAngles& rotation )=0;

	virtual void CopyToBookmark( Int32 bookmark, const Vector& position, const EulerAngles& rotation )=0;
	virtual Bool PasteFromBookmark( Int32 bookmark, Vector& position, EulerAngles& rotation )=0;
};

extern ICameraClipboard* GCameraClipboard;
#endif

// Parses a camera view string as copied by the editor and game's free camera mode
Bool ParseCameraViewString( const String& viewString, Vector& position, EulerAngles& rotation );
