/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "camera.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphPoseSlotNode.h"
#include "behaviorGraphStack.h"
#include "../core/scriptStackFrame.h"
#include "../core/mathUtils.h"
#include "animatedComponent.h"
#include "game.h"
#include "skeleton.h"
#include "componentIterator.h"
#include "world.h"
#include "layer.h"
#include "actorInterface.h"



#ifndef NO_EDITOR
ICameraClipboard* GCameraClipboard = NULL;
#endif

RED_DEFINE_NAMED_NAME( LOOK_AT_ACTIVATION, "lookAtWeight" );
RED_DEFINE_NAMED_NAME( LOOK_AT_TARGET, "lookAtTarget" );
RED_DEFINE_NAMED_NAME( LOOK_AT_ACT_DURATION, "lookAtDuration" );

RED_DEFINE_NAMED_NAME( FOLLOW_ACTIVATION, "followWeight" );
RED_DEFINE_NAMED_NAME( FOLLOW_TARGET, "followTarget" );

RED_DEFINE_NAMED_NAME( ANGLE_UP_DOWN, "cameraUpDownRot" );
RED_DEFINE_NAMED_NAME( ANGLE_LEFT_RIGHT, "cameraLeftRightRot" );
RED_DEFINE_NAMED_NAME( RESET_ANGLE_UP_DOWN, "cameraUpDownRotReset" );
RED_DEFINE_NAMED_NAME( RESET_ANGLE_LEFT_RIGHT, "cameraLeftRightRotReset" );

RED_DEFINE_NAMED_NAME( FOCUS_ACTIVATION, "focusWeight" );
RED_DEFINE_NAMED_NAME( FOCUS_TARGET, "focusTarget" );
RED_DEFINE_NAMED_NAME( FOCUS_ACT_DURATION, "focusDuration" );

RED_DEFINE_NAMED_NAME( ZOOM, "cameraFurther" );

RED_DEFINE_NAMED_NAME( DOF_OVERRIDE, "DOF_Override" );
RED_DEFINE_NAMED_NAME( DOF_FOCUS_DIST_FAR, "DOF_FocusDistFar" );
RED_DEFINE_NAMED_NAME( DOF_BLUR_DIST_FAR, "DOF_BlurDistFar" );
RED_DEFINE_NAMED_NAME( DOF_INTENSITY, "DOF_Intensity" );
RED_DEFINE_NAMED_NAME( DOF_FOCUS_DIST_NEAR, "DOF_FocusDistNear" );
RED_DEFINE_NAMED_NAME( DOF_BLUR_DIST_NEAR, "DOF_BlurDistNear" );

RED_DEFINE_NAMED_NAME( EVT_RESET_ROT_HOR, "cameraResetHor" );
RED_DEFINE_NAMED_NAME( EVT_RESET_ROT_VER, "cameraResetVer" );
RED_DEFINE_NAMED_NAME( EVT_HARD_RESET_ROT_HOR, "cameraResetHorHard" );
RED_DEFINE_NAMED_NAME( EVT_HARD_RESET_ROT_VER, "cameraResetVerHard" );
RED_DEFINE_NAMED_NAME( VAR_RESET_ROT_DURATION, "cameraResetDuration" );
RED_DEFINE_NAMED_NAME( HOR_FOLLOW_VAR, "followHorizontal" );

///////////////////////////////////////////////////////////////////////////
// Behavior variables

const Float		CCamera::COLLISION_WEIGHT_TRESHOLD_UP = 0.9750f;
const Float		CCamera::COLLISION_WEIGHT_TRESHOLD_DOWN = 0.960f;

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CCamera );
IMPLEMENT_ENGINE_CLASS( SBokehDofParams );

CCamera::CCamera()
	: m_selectCamera( NULL )
	, m_prevFOV( 60.0f )
	, m_moveForwardBackward( 0.0f )
	, m_moveLeftRight( 0.0f )
	, m_moveUpDown( 0.0f )
	, m_rotateUpDown( 0.0f )
	, m_rotateLeftRight( 0.0f )
	, m_boneEye( -1 )
	, m_boneEyeWished( -1 )
	, m_boneLookAt( -1 )
	, m_boneOrbit( -1 )
	, m_followTarget( NULL )
	, m_allowRotation( true )
	, m_rotationAlpha( 1.0f )
	, m_distToTargetRef( 1.f )
	, m_distToTargetRatio( 1.f )
{

}

void CCamera::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	// Select default camera
	SelectDefaultCameraComponent();
}

void CCamera::SelectDefaultCameraComponent()
{
	for( ComponentIterator< CCameraComponent > it( this ); it; ++it )
	{
		CCameraComponent* cam = *(it);

		if ( cam->IsDefaultCamera() )
		{
			m_selectCamera = cam;
			return;
		}
	}

	if ( !m_selectCamera )
	{
		for( ComponentIterator< CCameraComponent > it( this ); it; ++it )
		{
			CCameraComponent* cam = *(it);

			if ( cam )
			{
				m_selectCamera = cam;
				return;
			}
		}
	}

	m_selectCamera = NULL;
}

static Int32 GetBoneIndex( CAnimatedComponent * animatedComponent, const Char * boneName )
{
	if ( ! animatedComponent )
	{
		return -1;
	}

	const ISkeletonDataProvider* provider = animatedComponent->QuerySkeletonDataProvider();
	if ( !provider )
	{
		return -1;
	}

	return provider->FindBoneByName( boneName );
}

Bool CCamera::Update( Float timeDelta )
{
		//////////////////////////////////////////////////////////////////////////
	// PTom: Why you need this?
	/*
	/// SLOWMOTION
	Float timeScale = GGame->GetTimeScale();
	CAnimatedComponent* ac = GetRootAnimatedComponent();
	if ( ac )
	{		
		if ( timeScale != 0.f )
		{
			// Allow camera to move fast even if the time is slow
			ac->SetTimeMultiplier( 1.f / timeScale );
		}
	}
	*/

	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// ROTATION SENSITIVITY FILTER

	if ( IsRotationAllowed() )
	{
		if ( m_rotationAlpha < 1.0f )
		{
			const Float rotationRestoreDelta = 2.0f; // restore full sensitivity in 0.5s

			// Fade back to full sensitivity
			m_rotationAlpha += rotationRestoreDelta * timeDelta;
			if ( m_rotationAlpha >= 1.0f )
			{
				m_rotationAlpha = 1.0f;
			}
		}
	}
	else
	{
		// No movement
		m_rotationAlpha = 0.0f;
	}

	//////////////////////////////////////////////////////////////////////////

	return true;
}

Bool CCamera::GetData( Data& outData ) const
{
	if( m_selectCamera )
	{
		return m_selectCamera->GetData( outData );
	}

	return false;
}

void CCamera::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	CAnimatedComponent* ac = GetRootAnimatedComponent();

	m_boneEye       = GetBoneIndex( ac, TXT("Camera_Node") );
	m_boneEyeWished = GetBoneIndex( ac, TXT("Camera_ManipulationNode") );
	m_boneLookAt    = GetBoneIndex( ac, TXT("Camera_LookAtNode") );
	m_boneOrbit		= GetBoneIndex( ac, TXT("Camera_OrbitNode") );
}

void CCamera::OnDetached( CWorld* world )
{
	m_boneEye    = -1;
	m_boneLookAt = -1;
	m_boneLookAt = -1;
	m_boneOrbit	 = -1;

	TBaseClass::OnDetached( world );
}

void CCamera::OnAttachFinished( CWorld* world )
{
	CEntity* temp = m_followTarget;

	InitializeCamera();

	if ( temp )
	{
		Follow( temp );
	}
}

void CCamera::OnProcessBehaviorPose( const CAnimatedComponent* poseOwner, const SBehaviorGraphOutput& pose )
{
	CAnimatedComponent* ac = GetRootAnimatedComponent();

	if ( poseOwner != ac ) return;

	// Process camera component params
	if ( m_selectCamera && m_selectCamera->IsAttached() )
	{
		// FOV
		Float newFov = pose.m_numFloatTracks > SBehaviorGraphOutput::FTT_FOV ? pose.m_floatTracks[ SBehaviorGraphOutput::FTT_FOV ] : 0.0f;

		if ( newFov > 1.f )
		{
			m_selectCamera->SetFov( newFov );
		}
		else
		{
			m_selectCamera->SetFov( m_prevFOV );
		}

		if ( m_selectCamera )
		{
			SDofParams param;

			// DOF
			if ( pose.m_numFloatTracks > SBehaviorGraphOutput::FTT_DOF_BlurDistNear ) 
			{
				param.overrideFactor = pose.m_floatTracks[ SBehaviorGraphOutput::FTT_DOF_Override ];
				param.dofFocusDistFar = pose.m_floatTracks[ SBehaviorGraphOutput::FTT_DOF_FocusDistFar ];
				param.dofBlurDistFar = pose.m_floatTracks[ SBehaviorGraphOutput::FTT_DOF_BlurDistFar ];
				param.dofIntensity = pose.m_floatTracks[ SBehaviorGraphOutput::FTT_DOF_Intensity ];
				param.dofFocusDistNear = pose.m_floatTracks[ SBehaviorGraphOutput::FTT_DOF_FocusDistNear ];
				param.dofBlurDistNear = pose.m_floatTracks[ SBehaviorGraphOutput::FTT_DOF_BlurDistNear ];

				// AC: Fuck! Because of one stupid misunderstanding I have to hack there parameters here
				// bwr: fixed shit ;) 
				param.dofBlurDistNear = param.dofFocusDistNear - Max( 0.f, param.dofBlurDistNear );
				param.dofFocusDistFar  = param.dofFocusDistNear  + Max( 0.f, param.dofFocusDistFar );
				param.dofBlurDistFar  = param.dofFocusDistFar  + Max( 0.f, param.dofBlurDistFar );
			}
			else
			{
				param.Reset();
			}

			m_selectCamera->SetDofParams( param );
		}
	}

	// Process follow target
	ProcessFollowTarget();

	// Reset rotation deltas
	Rotate( 0.f, 0.f );
}

void CCamera::ProcessFollowTarget()
{
	if ( IsFollowing() )
	{
		ASSERT( m_followTarget );

		Float distToTarget = CalcDistToTarget();

		Float distMin = GGame->GetGameplayConfig().m_cameraHidePlayerDistMin;
		Float distMax = GGame->GetGameplayConfig().m_cameraHidePlayerDistMax;

		Bool canBeHidden = true;
		IActorInterface* actor = m_followTarget->QueryActorInterface();
		if ( actor )
		{
			canBeHidden = actor->IsInNonGameplayScene() == false;
		}

		if ( !m_followTarget->IsRenderingSuspended() && distToTarget < distMin && IsActive() && canBeHidden )
		{
			m_followTarget->SuspendRendering( true );
		}
		else if ( m_followTarget->IsRenderingSuspended() && distToTarget > distMax && canBeHidden )
		{
			m_followTarget->SuspendRendering( false );
		}

		m_distToTargetRatio = distToTarget / m_distToTargetRef;

		const CAnimatedComponent* root = GetRootAnimatedComponent();

		if ( m_followTarget == GGame->GetPlayerEntity() && actor && m_boneOrbit != -1 && root )
		{
			if ( !IsActive() )
			{
				canBeHidden = false;
			}
			actor->Hack_SetSwordsHiddenInGame( canBeHidden, distToTarget, root->GetBoneMatrixWorldSpace( m_boneOrbit ).GetYaw() );
		}
	}
	else
	{
		m_distToTargetRatio = 1.f;
	}

	//BEH_LOG( TXT("Dist to target ratio %f"), m_distToTargetRatio );
}

Float CCamera::CalcDistToTarget() const
{
	ASSERT( m_followTarget );

	if ( !m_selectCamera )
	{
		return FLT_MAX;
	}

	IActorInterface* actor = m_followTarget->QueryActorInterface();
	if ( actor )
	{
		return actor->GetHeadPosition().DistanceTo( m_selectCamera->GetWorldPositionRef() );
	}
	else
	{
		return m_followTarget->GetWorldPositionRef().DistanceTo( m_selectCamera->GetWorldPositionRef() );
	}
}

void CCamera::CacheDistToTargetRef()
{
	// BY DEX: is this function doing anything ?

	if ( m_followTarget )
	{
		const CAnimatedComponent* root = GetRootAnimatedComponent();
		if ( root )
		{
			const CSkeleton* skeleton = root->GetSkeleton();
			if ( skeleton )
			{
				const AnimQsTransform posMS = skeleton->GetBoneMS( m_boneEye );

#ifdef USE_HAVOK_ANIMATION
				const Vector camPosMS = reinterpret_cast< const Vector& >( posMS.getTranslation() );
#else
				const Vector camPosMS = reinterpret_cast< const Vector& >( posMS.GetTranslation() );
#endif

				
				Vector targetMS;

				IActorInterface* actor = m_followTarget->QueryActorInterface();
				if ( actor )
				{
					targetMS = actor->GetHeadPosition();
				}
				else
				{
					targetMS = m_followTarget->GetWorldPositionRef();
				}


			}
		}
	}

	m_distToTargetRef = 1.f;
}

void CCamera::SetActive( Float blendTime )
{
	CLayer* layer = GetLayer();
	CWorld* world = layer ? layer->GetWorld() : NULL;
	if( world )
	{
		world->GetCameraDirector()->ActivateCamera( this, this, blendTime );
	}
}

Bool CCamera::IsActive() const
{
	CLayer* layer = GetLayer();
	CWorld* world = layer ? layer->GetWorld() : NULL;

	return world && world->GetCameraDirector()->IsCameraActive( this );
}

Bool CCamera::SetData( const Data& data )
{
	SetPosition( data.m_position );
	SetRotation( data.m_rotation );
	SetDofParams( data.m_dofParams );
	SetFov( data.m_fov );
	return true;
}

Bool CCamera::IsOnStack() const
{
	return false;
}

void CCamera::Freeze()
{
	FreezeAllAnimatedComponents();
}

void CCamera::Unfreeze()
{
	UnfreezeAllAnimatedComponents();
}

Bool CCamera::IsFrozen() const
{
	return GetRootAnimatedComponent() ? GetRootAnimatedComponent()->IsFrozen() : false;
}

CCameraComponent* CCamera::GetSelectedCameraComponent()
{
	return m_selectCamera;
}

const CCameraComponent* CCamera::GetSelectedCameraComponent() const
{
	return m_selectCamera;
}

Float CCamera::GetDistanceToTargetRatio() const
{
	return m_distToTargetRatio;
}

Float CCamera::GetDistanceToTargetRatio01() const
{
	return Clamp( m_distToTargetRatio, 0.f ,1.f );
}

Bool CCamera::GetCameraDirection( Vector& dir ) const
{
	if ( m_boneEye < 0 )
	{
		return false;
	}

	CAnimatedComponent* ac = GetRootAnimatedComponent();

	Matrix cameraWorldMatrix = ac->GetBoneMatrixWorldSpace( m_boneEye );

	dir = cameraWorldMatrix.GetAxisY();
	dir.Normalize3();

	return true;
}

Bool CCamera::GetCameraPosition( Vector& pos ) const
{
	if ( m_boneEye < 0 )
	{
		return false;
	}

	CAnimatedComponent* ac = GetRootAnimatedComponent();

	Matrix cameraWorldMatrix = ac->GetBoneMatrixWorldSpace( m_boneEye );

	pos = cameraWorldMatrix.GetTranslation();

	return true;
}

Bool CCamera::GetCameraWishedPosition( Vector& pos ) const
{
	if ( m_boneEyeWished < 0 )
	{
		return false;
	}

	CAnimatedComponent* ac = GetRootAnimatedComponent();

	Matrix cameraWorldMatrix = ac->GetBoneMatrixWorldSpace( m_boneEyeWished );

	pos = cameraWorldMatrix.GetTranslation();

	return true;
}

Bool CCamera::GetCameraLookAtPosition( Vector& pos ) const
{
	if ( m_boneLookAt < 0 )
	{
		return false;
	}

	CAnimatedComponent* ac = GetRootAnimatedComponent();

	Matrix cameraWorldMatrix = ac->GetBoneMatrixWorldSpace( m_boneLookAt );

	pos = cameraWorldMatrix.GetTranslation();

	return true;
}

Bool CCamera::GetCameraMatrixWorldSpace( Matrix& transform ) const
{
	if ( m_boneEye < 0 )
	{
		return false;
	}
	
	CAnimatedComponent* ac = GetRootAnimatedComponent();

	transform = ac->GetBoneMatrixWorldSpace( m_boneEye );

	return true;
}

Bool CCamera::GetCameraOrbitWorldSpace( Matrix& transform ) const
{
	if ( m_boneOrbit < 0 )
	{
		return false;
	}

	CAnimatedComponent* ac = GetRootAnimatedComponent();

	transform = ac->GetBoneMatrixWorldSpace( m_boneOrbit );

	return true;
}

const Vector& CCamera::GetCameraComponentPosition() const
{
	return m_selectCamera ? m_selectCamera->GetLocalToWorld().GetTranslationRef() : Vector::ZERO_3D_POINT;
}

Float CCamera::GetCameraComponentYaw() const
{
	return m_selectCamera ? m_selectCamera->GetWorldYaw() : 0.f;
}

Float CCamera::GetDistanceToCameraRay( const Vector& testPos, Vector* linePoint ) const
{
	if ( m_selectCamera )
	{
		const Matrix& rayMat = m_selectCamera->GetLocalToWorld();

		const Vector& rayStart = rayMat.GetTranslationRef();
		const Vector rayDir = rayMat.GetAxisY();

		Vector pointInLine;

		Float ret = MathUtils::GeometryUtils::DistancePointToLine( testPos, rayStart, rayStart + rayDir, pointInLine );

		if ( linePoint )
		{
			*linePoint = pointInLine;
		}

		return ret;
	}

	return NumericLimits< Float >::Max();
}

void CCamera::SetFov( Float fov )
{
	CAnimatedComponent* ac = GetRootAnimatedComponent(); 

	if ( ac && ac->GetBehaviorStack() )
	{
		ac->GetBehaviorStack()->SetBehaviorVariable( CNAME( FOV ), fov );
	}
}

Float CCamera::GetFov() const
{
	return m_selectCamera ? m_selectCamera->GetFov() : 0.01f;
}

void CCamera::SetDofParams( const SDofParams& param )
{
	CBehaviorGraphStack* stack = GetRootAnimatedComponent()? GetRootAnimatedComponent()->GetBehaviorStack() : NULL;

	if ( stack )
	{
		Bool ret = true;

		ret &= stack->SetBehaviorVariable( CNAME( DOF_OVERRIDE ), param.overrideFactor );
		ret &= stack->SetBehaviorVariable( CNAME( DOF_FOCUS_DIST_FAR ), param.dofFocusDistFar );
		ret &= stack->SetBehaviorVariable( CNAME( DOF_BLUR_DIST_FAR ), param.dofBlurDistFar );
		ret &= stack->SetBehaviorVariable( CNAME( DOF_INTENSITY ), param.dofIntensity );
		ret &= stack->SetBehaviorVariable( CNAME( DOF_FOCUS_DIST_NEAR ), param.dofFocusDistNear );
		ret &= stack->SetBehaviorVariable( CNAME( DOF_BLUR_DIST_NEAR ), param.dofBlurDistNear );

		ASSERT( ret );
	}
}

void CCamera::SetBokehDofParams( const SBokehDofParams& param )
{
	if ( m_selectCamera )
	{
		m_selectCamera->SetBokehDofParams( param );
	}
}

void CCamera::ResetDofParams()
{
	CBehaviorGraphStack* stack = GetRootAnimatedComponent()? GetRootAnimatedComponent()->GetBehaviorStack() : NULL;

	if ( stack )
	{
		Bool ret = true;

		ret &= stack->SetBehaviorVariable( CNAME( DOF_OVERRIDE ), 0.f );
		ret &= stack->SetBehaviorVariable( CNAME( DOF_FOCUS_DIST_FAR ), 0.f );
		ret &= stack->SetBehaviorVariable( CNAME( DOF_BLUR_DIST_FAR ), 0.f );
		ret &= stack->SetBehaviorVariable( CNAME( DOF_INTENSITY ), 0.f );
		ret &= stack->SetBehaviorVariable( CNAME( DOF_FOCUS_DIST_NEAR ), 0.f );
		ret &= stack->SetBehaviorVariable( CNAME( DOF_BLUR_DIST_NEAR ), 0.f );

		ASSERT( ret );
	}
}

SDofParams CCamera::GetDofParams() const
{
	if ( m_selectCamera )
	{
		return m_selectCamera->GetDofParams();
	}
	else
	{
		return SDofParams();
	}
}

void CCamera::SetZoom( Float factor )
{
	CAnimatedComponent* ac = GetRootAnimatedComponent(); 

	if ( ac && ac->GetBehaviorStack() )
	{
		ac->GetBehaviorStack()->SetBehaviorVariable( CNAME( ZOOM ), factor );
	}
}

Float CCamera::GetZoom() const
{
	CAnimatedComponent* ac = GetRootAnimatedComponent(); 

	if ( ac && ac->GetBehaviorStack() )
	{
		return ac->GetBehaviorStack()->GetBehaviorFloatVariable( CNAME( ZOOM ) );
	}

	return 0.f;
}

void CCamera::InitializeCamera()
{
	// Reset
	InternalReset();

	// Reset camera param
	m_prevFOV = m_selectCamera ? m_selectCamera->GetFov() : 60.0f;
}

void CCamera::InternalReset()
{
	// Reset speed
	ResetMovementData();

	// Delete constraints
	DeactivateAllConstraints();
}

void CCamera::Reset( Bool resetFollowing )
{
	Rotate( 0.f, 0.f );
	ResetMovementData();

	if ( CanWork() )
	{
		CBehaviorGraphStack* stack = GetCameraBehStack();

		if ( resetFollowing == false && IsFollowing() )
		{
			CEntity* target = m_followTarget;

			FollowDeactivation();

			stack->ResetBehaviorInstances();

			Follow( target );
		}
		else if ( IsFollowing() )
		{
			FollowDeactivation();
			stack->ResetBehaviorInstances();
		}
		else
		{
			stack->ResetBehaviorInstances();
		}
	}
}

void CCamera::ResetMovementData()
{
	m_moveForwardBackward = 0.0f;
	m_moveLeftRight = 0.0f;
	m_moveUpDown = 0.0f;
	m_rotateUpDown = 0.0f;
	m_rotateLeftRight = 0.0f;
}

CBehaviorGraphStack* CCamera::GetCameraBehStack() const
{
	return GetRootAnimatedComponent() ? GetRootAnimatedComponent()->GetBehaviorStack() : NULL;
}

Bool CCamera::CanWork() const
{
	return GetCameraBehStack() != NULL;
}

/************************************************************************/
/* Camera constraints                                                   */
/************************************************************************/

Bool CCamera::IsConstraintEnabled( const CName varAct, const CName varDur) const
{
	if ( !CanWork() )
	{
		return false;
	}

	CBehaviorGraphStack* stack = GetRootAnimatedComponent()->GetBehaviorStack();

	const Bool hasActivation = stack->HasBehaviorFloatVariable( varAct );
	const Bool hasDuration = varDur == CName::NONE ? 1 : stack->HasBehaviorFloatVariable( varDur );

	return hasActivation && hasDuration;
}

Bool CCamera::DeactivateConstraint( const CName varAct, const CName varDur, Float duration )
{
	if ( !CanWork() )
	{
		return false;
	}

	CBehaviorGraphStack* stack = GetRootAnimatedComponent()->GetBehaviorStack();

	if ( varDur != CName::NONE )
	{
		stack->SetBehaviorVariable( varDur, duration );
	}

	return stack->DeactivateConstraint( varAct );
}

Bool CCamera::HasConstraint( const CName varAct ) const
{
	if ( !CanWork() )
	{
		return false;
	}

	CBehaviorGraphStack* stack = GetRootAnimatedComponent()->GetBehaviorStack();

	return stack->HasConstraint( varAct );
}

Vector CCamera::GetConstraintTarget( const CName varAct ) const
{
	if ( !CanWork() )
	{
		return Vector::ZERO_3D_POINT;
	}

	CBehaviorGraphStack* stack = GetRootAnimatedComponent()->GetBehaviorStack();

	return stack->GetConstraintTarget( varAct );
}

void CCamera::DeactivateAllConstraints()
{
	CAnimatedComponent* ac = GetRootAnimatedComponent();

	if ( ac && ac->GetBehaviorStack() )
	{
		CBehaviorGraphStack* stack = ac->GetBehaviorStack();

		if ( stack->HasConstraint( CNAME( LOOK_AT_ACTIVATION ) ) ) 
		{
			stack->DeactivateConstraint( CNAME( LOOK_AT_ACTIVATION ) );
		}
		if ( stack->HasConstraint( CNAME( FOCUS_ACTIVATION ) ) ) 
		{
			stack->DeactivateConstraint( CNAME( FOCUS_ACTIVATION ) );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Look at

Bool CCamera::LookAtDeactivation( Float duration /* = 0.0f  */)
{
	m_allowRotation = true;

	return DeactivateConstraint( CNAME( LOOK_AT_ACTIVATION ), CNAME( LOOK_AT_ACT_DURATION ), duration );
}

Bool CCamera::HasLookAt() const
{
	return HasConstraint( CNAME( LOOK_AT_ACTIVATION ) );
}

Vector CCamera::GetLookAtTargetPosition()
{
	return GetConstraintTarget( CNAME( LOOK_AT_ACTIVATION ) );
}

void CCamera::LookAt( const CNode* target, 
					  Float duration /* = 0.f */, 
					  Float activationTime /* = 0.f */ )
{
	if ( IsConstraintEnabled( CNAME( LOOK_AT_ACTIVATION ), CNAME( LOOK_AT_ACT_DURATION ) ) )
	{
		CBehaviorGraphStack* stack = GetCameraBehStack();

		if ( IsFocused() )
		{
			FocusDeactivation();
		}

		if ( HasLookAt() )
		{
			Bool lookAtSuccess = stack->ChangeConstraintTarget( target, CNAME( LOOK_AT_ACTIVATION ), CNAME( LOOK_AT_TARGET ), duration );
			ASSERT( lookAtSuccess );
		}
		else
		{
			Bool lookAtSuccess = stack->ActivateConstraint( target, CNAME( LOOK_AT_ACTIVATION ), CNAME( LOOK_AT_TARGET ), duration );
			ASSERT( lookAtSuccess );
		}

		stack->SetBehaviorVariable( CNAME( LOOK_AT_ACT_DURATION ), activationTime );

		ResetRotationAlpha();
		m_allowRotation = false;
	}
}

void CCamera::LookAt( const Vector& staticTarget, 
					  Float duration /* = 0.f */, 
					  Float activationTime /* = 0.f  */ )
{
	if ( IsConstraintEnabled( CNAME( LOOK_AT_ACTIVATION ), CNAME( LOOK_AT_ACT_DURATION ) ) )
	{
		CBehaviorGraphStack* stack = GetCameraBehStack();

		if ( IsFocused() )
		{
			FocusDeactivation();
		}

		if ( HasLookAt() )
		{
			Bool lookAtSuccess = stack->ChangeConstraintTarget( staticTarget, CNAME( LOOK_AT_ACTIVATION ), CNAME( LOOK_AT_TARGET ), duration );
			ASSERT( lookAtSuccess );
		}
		else
		{
			Bool lookAtSuccess = stack->ActivateConstraint( staticTarget, CNAME( LOOK_AT_ACTIVATION ), CNAME( LOOK_AT_TARGET ), duration );
			ASSERT( lookAtSuccess );
		}

		stack->SetBehaviorVariable( CNAME( LOOK_AT_ACT_DURATION ), activationTime );

		ResetRotationAlpha();
		m_allowRotation = false;
	}
}

void CCamera::LookAt( const CAnimatedComponent* target, 
					  const String& bone, 
					  Float duration /* = 0.f */, 
					  Float activationTime /* = 0.f  */)
{
	if ( IsConstraintEnabled( CNAME( LOOK_AT_ACTIVATION ), CNAME( LOOK_AT_ACT_DURATION ) ) )
	{
		CBehaviorGraphStack* stack = GetCameraBehStack();

		if ( IsFocused() )
		{
			FocusDeactivation();
		}

		if ( HasLookAt() )
		{
			Bool lookAtSuccess = stack->ChangeConstraintTarget( target, bone, CNAME( LOOK_AT_ACTIVATION ), CNAME( LOOK_AT_TARGET ), false, Matrix::IDENTITY, duration );
			ASSERT( lookAtSuccess );
		}
		else
		{
			Bool lookAtSuccess = stack->ActivateConstraint( target, bone, CNAME( LOOK_AT_ACTIVATION ), CNAME( LOOK_AT_TARGET ), false, Matrix::IDENTITY, duration );
			ASSERT( lookAtSuccess );
		}

		stack->SetBehaviorVariable( CNAME( LOOK_AT_ACT_DURATION ), activationTime );

		ResetRotationAlpha();
		m_allowRotation = false;
	}
}

//////////////////////////////////////////////////////////////////////////
// Focus

Bool CCamera::FocusDeactivation( Float duration )
{
	m_allowRotation = true;

	if ( IsFocused() )
	{
		SyncRotationsFromConstraintedPose();
	}

	return DeactivateConstraint( CNAME( FOCUS_ACTIVATION ), CNAME( FOCUS_ACT_DURATION ), duration );
}

Bool CCamera::IsFocused() const
{
	return HasConstraint( CNAME( FOCUS_ACTIVATION ) );
}

Vector CCamera::GetFocusTargetPosition()
{
	return GetConstraintTarget( CNAME( FOCUS_ACTIVATION ) );
}

void CCamera::FocusOn( const CNode* target, 
					   Float duration /* = 0.f */, 
					   Float activationTime /* = 0.f */ )
{
	if ( IsConstraintEnabled( CNAME( FOCUS_ACTIVATION ), CNAME( FOCUS_ACT_DURATION ) ) )
	{
		CBehaviorGraphStack* stack = GetCameraBehStack();

		if ( HasLookAt() )
		{
			LookAtDeactivation();
		}

		if ( IsFocused() )
		{
			Bool focusSuccess = stack->ChangeConstraintTarget( target, CNAME( FOCUS_ACTIVATION ), CNAME( FOCUS_TARGET ), duration );
			ASSERT( focusSuccess );
		}
		else
		{
			Bool focusSuccess = stack->ActivateConstraint( target, CNAME( FOCUS_ACTIVATION ), CNAME( FOCUS_TARGET ), duration );
			ASSERT( focusSuccess );
		}

		stack->SetBehaviorVariable( CNAME( FOCUS_ACT_DURATION ), activationTime );

		ResetRotationAlpha();
		m_allowRotation = false;
	}
}

void CCamera::FocusOn( const Vector& staticTarget, 
					   Float duration /* = 0.f */, 
					   Float activationTime /* = 0.f  */ )
{
	if ( IsConstraintEnabled( CNAME( FOCUS_ACTIVATION ), CNAME( FOCUS_ACT_DURATION ) ) )
	{
		CBehaviorGraphStack* stack = GetCameraBehStack();

		if ( HasLookAt() )
		{
			LookAtDeactivation();
		}

		if ( IsFocused() )
		{
			Bool focusSuccess = stack->ChangeConstraintTarget( staticTarget, CNAME( FOCUS_ACTIVATION ), CNAME( FOCUS_TARGET ), duration );
			ASSERT( focusSuccess );
		}
		else
		{
			Bool focusSuccess = stack->ActivateConstraint( staticTarget, CNAME( FOCUS_ACTIVATION ), CNAME( FOCUS_TARGET ), duration );
			ASSERT( focusSuccess );
		}

		stack->SetBehaviorVariable( CNAME( FOCUS_ACT_DURATION ), activationTime );

		ResetRotationAlpha();
		m_allowRotation = false;
	}
}

void CCamera::FocusOn( const CAnimatedComponent* target, 
					   const String& bone, 
					   Float duration /* = 0.f */, 
					   Float activationTime /* = 0.f  */)
{
	if ( IsConstraintEnabled( CNAME( FOCUS_ACTIVATION ), CNAME( FOCUS_ACT_DURATION ) ) )
	{
		CBehaviorGraphStack* stack = GetCameraBehStack();

		if ( HasLookAt() )
		{
			LookAtDeactivation();
		}

		if ( IsFocused() )
		{
			Bool focusSuccess = stack->ChangeConstraintTarget( target, bone, CNAME( FOCUS_ACTIVATION ), CNAME( FOCUS_TARGET ), false, Matrix::IDENTITY, duration );
			ASSERT( focusSuccess );
		}
		else
		{
			Bool focusSuccess = stack->ActivateConstraint( target, bone, CNAME( FOCUS_ACTIVATION ), CNAME( FOCUS_TARGET ), false, Matrix::IDENTITY, duration );
			ASSERT( focusSuccess );
		}

		stack->SetBehaviorVariable( CNAME( FOCUS_ACT_DURATION ), activationTime );

		ResetRotationAlpha();
		m_allowRotation = false;
	}
}

//////////////////////////////////////////////////////////////////////////
// Follow

void CCamera::FollowRotation( Bool flag )
{
	CBehaviorGraphStack* stack = GetCameraBehStack();
	if ( stack )
	{
		VERIFY( stack->SetBehaviorVariable( CNAME( HOR_FOLLOW_VAR ), flag ? 1.f : 0.f ) );
	}
}

Bool CCamera::FollowDeactivation()
{
	SetFollowTarget( NULL );

	FollowRotation( false );

	return DeactivateConstraint( CNAME( FOLLOW_ACTIVATION ) );
}

Vector CCamera::GetFollowTargetPosition() const
{
	return GetConstraintTarget( CNAME( FOLLOW_ACTIVATION ) );
}

Float CCamera::GetFollowTargetYaw() const
{
	if ( IsFollowing() )
	{
		ASSERT( m_followTarget );
		return m_followTarget->GetWorldYaw();
	}
	else
	{
		return 0.f;
	}
}

Bool CCamera::IsFollowing() const
{
	return HasConstraint( CNAME( FOLLOW_ACTIVATION ) );
}

Bool CCamera::HasFollowTarget( const CEntity* entity ) const
{
	return m_followTarget == entity;
}

void CCamera::SetFollowTarget( CEntity* target )
{
	m_followTarget = target;

	CacheDistToTargetRef();
}

void CCamera::Follow( CEntity* target )
{
	if ( !IsSpawned() )
	{
		SetFollowTarget( target );
	}

	if ( IsConstraintEnabled( CNAME( FOLLOW_ACTIVATION ) ) )
	{
		// Reset
		ResetMovementData();

		FollowRotation( false );

		CBehaviorGraphStack* stack = GetCameraBehStack();

		if ( !IsFollowing() )
		{
			Bool followSuccess = stack->ActivateConstraint( target, CNAME( FOLLOW_ACTIVATION ), CNAME( FOLLOW_TARGET ), 0.f );
			ASSERT( followSuccess );
		}
		else
		{
			Bool followSuccess = stack->ChangeConstraintTarget( target, CNAME( FOLLOW_ACTIVATION ), CNAME( FOLLOW_TARGET ), 0.f );
			ASSERT( followSuccess );
		}

		// Cache target node
		SetFollowTarget( target );
	}
}

void CCamera::FollowWithRotation( CEntity* target, Int32 boneIndex )
{
	if ( !IsSpawned() )
	{
		SetFollowTarget( target );
	}

	if ( IsConstraintEnabled( CNAME( FOLLOW_ACTIVATION ) ) )
	{
		// Reset
		ResetMovementData();

		FollowRotation( true );

		CBehaviorGraphStack* stack = GetCameraBehStack();

		if ( !IsFollowing() )
		{
			Bool followSuccess = stack->ActivateConstraint( target, CNAME( FOLLOW_ACTIVATION ), CNAME( FOLLOW_TARGET ), 0.f );
			ASSERT( followSuccess );
		}
		else
		{
			Bool followSuccess = stack->ChangeConstraintTarget( target, CNAME( FOLLOW_ACTIVATION ), CNAME( FOLLOW_TARGET ), 0.f );
			ASSERT( followSuccess );
		}

		// Cache target node
		SetFollowTarget( target );
	}
}

//////////////////////////////////////////////////////////////////////////
// Rotations

Bool CCamera::IsRotationAllowed() const
{
	return m_allowRotation && !m_rotationLocked && CanWork() && !HasLookAt() && !IsFocused();
}

void CCamera::ResetRotationAlpha()
{
	if ( IsRotationAllowed() )
	{
		m_rotationAlpha = 0.0f;
	}
}

void CCamera::Rotate( Float leftRightAng, Float upDownAng )
{
	if ( CanWork() && m_rotationAlpha > 0.0f )
	{
		CBehaviorGraphStack* stack = GetRootAnimatedComponent()->GetBehaviorStack();

		stack->SetBehaviorVariable( CNAME( ANGLE_LEFT_RIGHT ), leftRightAng * m_rotationAlpha );
		stack->SetBehaviorVariable( CNAME( ANGLE_UP_DOWN ), upDownAng  * m_rotationAlpha );
	}
}

void CCamera::ResetRotation(	Bool smoothly /* = true */, 
								Bool horizontal /* = true */, 
								Bool vertical /* = true */, 
								Float duration /* = 0.5f */ )
{
	if ( CanWork() )
	{
		CBehaviorGraphStack* stack = GetRootAnimatedComponent()->GetBehaviorStack();

		Bool ret = true;

		ret &= stack->SetBehaviorVariable( CNAME( VAR_RESET_ROT_DURATION ), duration );

		if ( horizontal )
		{
			const CName eventName = smoothly ? CNAME( EVT_RESET_ROT_HOR ) : CNAME( EVT_HARD_RESET_ROT_HOR );

			ret &= stack->SetBehaviorVariable( CNAME( ANGLE_LEFT_RIGHT ), 0.f );
			// You can't wirte proper angle because target position will be valid after locomotion tick and update transform
			// This is not teh best solution but i haven't got time for it
			ret &= stack->SetBehaviorVariable( CNAME( RESET_ANGLE_LEFT_RIGHT ), 1024.f );
			ret &= stack->GenerateBehaviorEvent( eventName );
		}

		if ( vertical )
		{
			const CName eventName = smoothly ? CNAME( EVT_RESET_ROT_VER ) : CNAME( EVT_HARD_RESET_ROT_VER );

			ret &= stack->SetBehaviorVariable( CNAME( ANGLE_UP_DOWN ), 0.f );
			ret &= stack->SetBehaviorVariable( CNAME( RESET_ANGLE_UP_DOWN ), 0.f );
			ret &= stack->GenerateBehaviorEvent( eventName );
		}

		ASSERT( ret );
	}
}

void CCamera::ResetRotation(	Bool smoothly, 
								Float horAngle, 
								Float verAngle, 
								Float duration /* = 0.5f */ )
{
	if ( CanWork() )
	{
		CBehaviorGraphStack* stack = GetRootAnimatedComponent()->GetBehaviorStack();

		Bool ret = true;

		ret &= stack->SetBehaviorVariable( CNAME( VAR_RESET_ROT_DURATION ), duration );

		{
			const CName eventName = smoothly ? CNAME( EVT_RESET_ROT_HOR ) : CNAME( EVT_HARD_RESET_ROT_HOR );

			horAngle -= GetWorldYaw();

			ret &= stack->SetBehaviorVariable( CNAME( ANGLE_LEFT_RIGHT ), 0.f );
			ret &= stack->SetBehaviorVariable( CNAME( RESET_ANGLE_LEFT_RIGHT ), horAngle );
			ret &= stack->GenerateBehaviorEvent( eventName );
		}

		{
			const CName eventName = smoothly ? CNAME( EVT_RESET_ROT_VER ) : CNAME( EVT_HARD_RESET_ROT_VER );

			ret &= stack->SetBehaviorVariable( CNAME( ANGLE_UP_DOWN ), 0.f );
			ret &= stack->SetBehaviorVariable( CNAME( RESET_ANGLE_UP_DOWN ), verAngle );
			ret &= stack->GenerateBehaviorEvent( eventName );
		}

		ASSERT( ret );
	}
}

void CCamera::LockRotation()
{
	// Reset cached rotation delta angles
	Rotate( 0.f, 0.f );

	ResetRotationAlpha();

	m_rotationLocked.Set();
}

void CCamera::UnlockRotation()
{
	m_rotationLocked.Unset();
}

Bool CCamera::GetRotationAngles( Float& updown, Float& leftRight ) const
{
	if ( CanWork() )
	{
		CBehaviorGraphStack* stack = GetRootAnimatedComponent()->GetBehaviorStack();

		const Float* lrPtr = stack->GetBehaviorFloatVariablePtr( CNAME( ANGLE_LEFT_RIGHT ) );
		const Float* udPtr = stack->GetBehaviorFloatVariablePtr( CNAME( ANGLE_UP_DOWN ) );

		if ( lrPtr && udPtr )
		{
			leftRight = *lrPtr;
			updown = *udPtr;
		}
	}

	return false;
}

void CCamera::ReloadInputConfig()
{

}

void CCamera::SyncRotationsFromConstraintedPose()
{
	if ( CanWork() )
	{
		CAnimatedComponent* ac = GetRootAnimatedComponent();

		const Int32 horIndex = m_boneOrbit;
		const Int32 verIndex = m_boneLookAt;

		const EulerAngles horAngles = ac->GetBoneMatrixLocalSpace( horIndex ).ToEulerAngles();
		const EulerAngles verAngles = ac->GetBoneMatrixLocalSpace( verIndex ).ToEulerAngles();

		Float horAngle = EulerAngles::NormalizeAngle( horAngles.Yaw );		// Z
		Float verAngle = EulerAngles::NormalizeAngle( verAngles.Pitch );	// X

		ResetRotation( false, horAngle, verAngle );
	}
}

void CCamera::ForceBehaviorCameraPose()
{
	CAnimatedComponent* ac = GetRootAnimatedComponent();
	if ( ac )
	{
		ac->ForceBehaviorPose();
	}
}

/************************************************************************/
/* Scripts                                                              */
/************************************************************************/

void CCamera::funcRotate( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, leftRightSpeed, 0.0f );
	GET_PARAMETER( Float, upDownSpeed, 0.0f );
	FINISH_PARAMETERS;
	Rotate( leftRightSpeed, upDownSpeed );
}

void CCamera::funcFollow( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CEntity >, dest, NULL );
	FINISH_PARAMETERS;

	CEntity *pDest = dest.Get();
	if ( pDest )
	{
		Follow( pDest );
	}
}

void CCamera::funcFollowWithRotation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CEntity >, dest, NULL );
	FINISH_PARAMETERS;

	CEntity *pDest = dest.Get();
	if ( pDest )
	{
		CAnimatedComponent* root = pDest->GetRootAnimatedComponent();
		if ( root && root->GetTrajectoryBone() != -1 )
		{
			FollowWithRotation( pDest, root->GetTrajectoryBone() );
		}
		else
		{
			Follow( pDest );
		}
	}
}

void CCamera::funcLookAt( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CNode >, target, NULL );
	GET_PARAMETER_OPT( Float, duration, 0.f );
	GET_PARAMETER_OPT( Float, actTime, 0.f );
	FINISH_PARAMETERS;

	CNode *pTarget = target.Get();
	if ( pTarget )
	{
		LookAt( pTarget, duration, actTime );
	}
}

void CCamera::funcLookAtStatic( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, staticTarget, Vector::ZERO_3D_POINT );
	GET_PARAMETER_OPT( Float, duration, 0.f );
	GET_PARAMETER_OPT( Float, actTime, 0.f );
	FINISH_PARAMETERS;

	LookAt( staticTarget, duration, actTime );
}

void CCamera::funcLookAtBone( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CAnimatedComponent >, target, NULL );
	GET_PARAMETER( String, bone, String::EMPTY );
	GET_PARAMETER_OPT( Float, duration, 0.f );
	GET_PARAMETER_OPT( Float, actTime, 0.f );
	FINISH_PARAMETERS;

	CAnimatedComponent *pTarget = target.Get();
	if ( pTarget )
	{
		LookAt( pTarget, bone, duration, actTime );
	}
}

void CCamera::funcLookAtDeactivation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Float, deactTime, 0.0f );
	FINISH_PARAMETERS;
	LookAtDeactivation( deactTime );
}

void CCamera::funcHasLookAt( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( HasLookAt() );
}

void CCamera::funcGetLookAtTargetPosition( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, GetLookAtTargetPosition() );
}

void CCamera::funcFocusOn( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CNode >, target, NULL );
	GET_PARAMETER_OPT( Float, duration, 0.f );
	GET_PARAMETER_OPT( Float, actTime, 0.f );
	FINISH_PARAMETERS;

	CNode *pTarget = target.Get();
	if ( pTarget )
	{
		FocusOn( pTarget, duration, actTime );
	}
}

void CCamera::funcFocusOnStatic( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, staticTarget, Vector::ZERO_3D_POINT );
	GET_PARAMETER_OPT( Float, duration, 0.f );
	GET_PARAMETER_OPT( Float, actTime, 0.f );
	FINISH_PARAMETERS;

	FocusOn( staticTarget, duration, actTime );
}

void CCamera::funcFocusOnBone( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CAnimatedComponent >, target, NULL );
	GET_PARAMETER( String, bone, String::EMPTY );
	GET_PARAMETER_OPT( Float, duration, 0.f );
	GET_PARAMETER_OPT( Float, actTime, 0.f );
	FINISH_PARAMETERS;

	CAnimatedComponent *pTarget = target.Get();
	if ( pTarget )
	{
		FocusOn( pTarget, bone, duration, actTime );
	}
}

void CCamera::funcFocusDeactivation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Float, deactTime, 0.0f );
	FINISH_PARAMETERS;
	FocusDeactivation( deactTime );
}

void CCamera::funcSetActive( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Float, blendTime, 0.f );
	GET_PARAMETER_OPT( Float, weight, 1.f );
	FINISH_PARAMETERS;

	SetActive( blendTime );
}

void CCamera::funcIsFocused( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsFocused() );
}

void CCamera::funcGetFocusTargetPosition( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, GetFocusTargetPosition() );
}

void CCamera::funcIsActive( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsActive() );
}

void CCamera::funcIsOnStack( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsOnStack() );
}

void CCamera::funcGetCameraDirection( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Vector value;
	if ( GetCameraDirection( value ) )
	{
		RETURN_STRUCT( Vector, value );
	}
}

void CCamera::funcGetCameraPosition( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Vector value;
	if ( GetCameraPosition( value ) )
	{
		RETURN_STRUCT( Vector, value );
	}
}

void CCamera::funcGetCameraMatrixWorldSpace( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Matrix value;
	if ( GetCameraMatrixWorldSpace( value ) )
	{
		RETURN_STRUCT( Matrix, value );
	}
}

void CCamera::funcSetFov( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, fov, 0.0f );
	FINISH_PARAMETERS;
	SetFov( fov );
}

void CCamera::funcGetFov( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_FLOAT( GetFov() );
}

void CCamera::funcSetZoom( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, zoom, 0.0f );
	FINISH_PARAMETERS;
	SetZoom( zoom );
}

void CCamera::funcGetZoom( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_FLOAT( GetZoom() );
}

void CCamera::funcReset( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	Reset();
}

void CCamera::funcResetRotation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Bool, smoothly, true );
	GET_PARAMETER_OPT( Bool, hor, true );
	GET_PARAMETER_OPT( Bool, ver, true );
	GET_PARAMETER_OPT( Float, duration, 0.5f );
	FINISH_PARAMETERS;

	ResetRotation( smoothly, hor, ver, duration );
}

void CCamera::funcResetRotationTo( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, smoothly, true );
	GET_PARAMETER( Float, hor, 0.f );
	GET_PARAMETER_OPT( Float, ver, 0.f );
	GET_PARAMETER_OPT( Float, duration, 0.5f );
	FINISH_PARAMETERS;

	ResetRotation( smoothly, hor, ver, duration );
}

Bool ParseCameraViewString( const String& viewString, Vector& position, EulerAngles& rotation )
{
	String viewText = viewString;
	viewText.Trim();
	if ( viewText.BeginsWith( TXT("[[") ) && viewText.EndsWith( TXT("]]") ) )
	{
		viewText = viewText.MidString( 2, viewText.GetLength() - 4 );
		size_t chloc;
		if ( viewText.FindCharacter( TXT('|'), chloc ) )
		{
			String posPart = viewText.LeftString( chloc );
			String rotPart = viewText.MidString( chloc + 1 );
			Vector mayPosition;
			EulerAngles mayRotation;
			if ( FromString( posPart, mayPosition ) && FromString( rotPart, mayRotation ) )
			{
				position = mayPosition;
				rotation = mayRotation;
				return true;
			}
		}
	}
	return false;
}
