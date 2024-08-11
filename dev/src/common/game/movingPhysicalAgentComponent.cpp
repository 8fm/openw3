/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "movingPhysicalAgentComponent.h"

#include "../engine/environmentManager.h"
#include "../engine/physicsCharacterWrapper.h"
#include "../engine/physicsCharacterVirtualController.h"
#include "../physics/physicsSettings.h"
#include "../physics/physicsWorld.h"
#include "../physics/physicsWorldUtils.h"
#include "../engine/renderFrame.h"
#include "../engine/rigidMeshComponent.h"
#include "../engine/layer.h"
#include "../engine/layerInfo.h"
#include "../engine/characterControllerManager.h"

#include "movableRepresentationPhysicalCharacter.h"
#include "movableRepresentationPathAgent.h"


IMPLEMENT_ENGINE_CLASS( CMovingPhysicalAgentComponent );

CMovingPhysicalAgentComponent::CMovingPhysicalAgentComponent()
	: m_physRepresentation( nullptr )
	, m_physWorld( nullptr )
{
}

CMovingPhysicalAgentComponent::~CMovingPhysicalAgentComponent()
{
}

void CMovingPhysicalAgentComponent::CreateCustomMoveRepresentations( CWorld* world )
{
	PC_SCOPE_PIX( CMovingPhysicalAgentComponent_OnAttached );

    world->GetPhysicsWorld( m_physWorld );
    ASSERT( !m_physRepresentation );

	CEntityTemplate *templ = GetEntity()->GetEntityTemplate();
	const CCharacterControllerParam* params( nullptr );
	if ( templ )
	{
		params = templ->FindGameplayParamT< CCharacterControllerParam > ( true );
	}
	if( params && params->m_customMovableRep )
	{
		m_physRepresentation = params->m_customMovableRep->CreateRepresentation( this );
	}
#ifdef USE_PHYSX
	else 
	{
		m_physRepresentation = new CMRPhysicalCharacter( *this );
	}
#endif
	if( m_physRepresentation)
	{
		AddRepresentation( m_physRepresentation, false );
	}

	TBaseClass::CreateCustomMoveRepresentations( world );
}

void CMovingPhysicalAgentComponent::OnAttached( CWorld *world )
{
	TBaseClass::OnAttached( world );

	// Save original push priority
	SetOriginalInteractionPriority( GetInteractionPriority() );

	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_PhysXTraceVisualization );
    world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_PhysXPlatforms );
}

void CMovingPhysicalAgentComponent::OnDetached( CWorld* world )
{
	ASSERT( world->GetPhysicsWorld( m_physWorld ) );

	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_PhysXTraceVisualization );
    world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_PhysXPlatforms );

	delete m_physRepresentation;
	m_physRepresentation = nullptr;

	m_physWorld = nullptr;

	TBaseClass::OnDetached( world );
}


void CMovingPhysicalAgentComponent::OnSkeletonChanged()
{
	if ( m_physRepresentation != nullptr && m_physRepresentation->GetCharacterController() != nullptr )
	{
#ifdef USE_PHYSX
		m_physRepresentation->GetCharacterController()->OnParentSkeletonChanged();
#endif
	}
}

IMovableRepresentation* CMovingPhysicalAgentComponent::DetermineMoveRepresentation() const
{
	if ( !m_isDisabled )
	{
		if ( !IsEntityRepresentationForced() )
		{
			if ( IsPhysicalRepresentationRequested() )
			{
				return m_physRepresentation;
			}
			else
			{
				return m_pathAgent;
			}
		}
		else
		{
			return m_meshRepresentation;
		}
	}

	return nullptr;
}

void CMovingPhysicalAgentComponent::SetAnimatedMovement( Bool enable )
{
#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		m_physRepresentation->GetCharacterController()->SetAnimated( enable );
	}
#endif

	if ( m_pathAgent )
	{
		m_pathAgent->SetAnimatedMovement( enable );
	}
}

Bool CMovingPhysicalAgentComponent::IsAnimatedMovement()
{
	if ( m_pathAgent )
	{
		return m_pathAgent->IsAnimatedMovement();
	}

#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->IsAnimated();
	}
#endif

	return false;
}

void CMovingPhysicalAgentComponent::SetGravity( Bool enable )
{
	if ( m_physRepresentation )
	{
		m_physRepresentation->SetGravity( enable );
	}
}

Bool CMovingPhysicalAgentComponent::IsGravity()
{
	if ( m_physRepresentation )
	{
		return m_physRepresentation->IsGravity();
	}
	return false;
}

void CMovingPhysicalAgentComponent::SetNeedsBehaviorCallback(Bool enable)
{
	if ( m_physRepresentation )
	{
		m_physRepresentation->SetNeedsBehaviorCallback( enable );
	}
}

Bool CMovingPhysicalAgentComponent::IsBehaviorCallbackNeeded() const
{
	if ( m_physRepresentation )
	{
		return m_physRepresentation->IsBehaviorCallbackNeeded();
	}
	return true;
}

void CMovingPhysicalAgentComponent::EnableAdditionalVerticalSlidingIteration( const Bool enable )
{
#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		m_physRepresentation->GetCharacterController()->EnableAdditionalVerticalSlidingIteration( enable );
	}
#endif
}

Bool CMovingPhysicalAgentComponent::IsAdditionalVerticalSlidingIterationEnabled()
{
#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->IsAdditionalVerticalSlidingIterationEnabled();
	}
#endif
	return false;
}

const Bool CMovingPhysicalAgentComponent::CanPhysicalMove()
{
#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->CanMove( m_physRepresentation->GetCharacterController()->GetPosition() );
	}
#endif
	return false;
}

const Bool CMovingPhysicalAgentComponent::ShouldPhysicalMove()
{
#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->ShouldMove();
	}
#endif
	return false;
}

const Bool CMovingPhysicalAgentComponent::IsFalling()
{
#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->IsFalling();
	}
#endif
	return false;
}

const Bool CMovingPhysicalAgentComponent::IsTeleport()
{
#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->IsTeleport();
	}
#endif
	return false;
}

const Bool CMovingPhysicalAgentComponent::IsStandingOnDynamic()
{
#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->IsStandingOnDynamic();
	}
#endif
	return false;
}

const Float CMovingPhysicalAgentComponent::GetRagdollPushMultiplier()
{
#ifdef USE_PHYSX
	if( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->GetRagdollPushMultiplier();
	}
#endif
	return 0.0f;
}

void CMovingPhysicalAgentComponent::SetRagdollToSwimming( Bool enable )
{
	ASSERT( m_representationStack->GetActiveRepresentation() == m_physRepresentation );

	if ( m_physRepresentation )
	{
		m_physRepresentation->SetRagdollToSwimming( enable );
	}
}

Bool CMovingPhysicalAgentComponent::IsPhysicalMovementEnabled() const
{
	if ( m_physRepresentation )
	{
		return m_physRepresentation->IsPhysicalMovementEnabled();
	}
	return false;
}

void CMovingPhysicalAgentComponent::SetSwimming( Bool enable )
{
	if ( m_physRepresentation )
	{
		m_physRepresentation->SetSwimming( enable );
	}
}

Bool CMovingPhysicalAgentComponent::IsSwimming() const
{
	#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->IsSwimming();
	}
	#endif
	return false;
}

Float CMovingPhysicalAgentComponent::GetWaterLevel()
{
	if( m_physRepresentation )
	{
		return m_physRepresentation->GetWaterLevel();
	}
	return -10000.0f;
}

Float CMovingPhysicalAgentComponent::GetSubmergeDepth()
{
#ifdef USE_PHYSX
	if( m_physRepresentation && m_physRepresentation->GetCharacterController() && m_physRepresentation->GetCharacterController()->IsReady() )
	{		
		return m_physRepresentation->GetCharacterController()->GetPosition().Z - m_physRepresentation->GetWaterLevel();
	}
#endif
	return 10000;
}

void CMovingPhysicalAgentComponent::SetDiving( Bool diving )
{
	if ( m_physRepresentation )
	{
		m_physRepresentation->SetDiving( diving );
	}
}

Bool CMovingPhysicalAgentComponent::IsDiving() const
{
	if( m_physRepresentation )
	{
		return m_physRepresentation->IsDiving();
	}
	return false;
}

const Float CMovingPhysicalAgentComponent::GetEmergeSpeed()
{
#ifdef USE_PHYSX
	if( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->GetEmergeSpeed();
	}
#endif
	return 0.0f;
}

const Float CMovingPhysicalAgentComponent::GetSubmergeSpeed()
{
#ifdef USE_PHYSX
	if( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->GetSubmergeSpeed();
	}
#endif
	return 0.0f;
}

const Uint32 CMovingPhysicalAgentComponent::GetVirtualControllerCount()
{
#ifdef USE_PHYSX
	if( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->GetVirtualControllers().Size();
	}
#endif
	return 0;
}

const Bool CMovingPhysicalAgentComponent::IsSliding()
{
#ifdef USE_PHYSX
	if( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->IsSliding();
	}
#endif
	return false;
}

const Uint32 CMovingPhysicalAgentComponent::GetSlidingState()
{
#ifdef USE_PHYSX
	if( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->GetSlidingState();
	}
#endif
	return 0;
}

const Float CMovingPhysicalAgentComponent::GetSlideCoef()
{
#ifdef USE_PHYSX
	if( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->GetSlideCoef();
	}
#endif
	return 0.0f;
}

const Vector CMovingPhysicalAgentComponent::GetSlidingDir()
{
#ifdef USE_PHYSX
	if( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->GetSlidingDir();
	}
#endif
	return Vector::ZEROS;
}

const Vector CMovingPhysicalAgentComponent::GetLastMoveVector()
{
#ifdef USE_PHYSX
	if( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->GetLastMoveVector();
	}
#endif
	return Vector::ZEROS;
}

const Vector CMovingPhysicalAgentComponent::GetCurrentMovementVectorRef()
{
#ifdef USE_PHYSX
	if( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->GetCurrentMovementVectorRef();
	}
#endif
	return Vector::ZEROS;
}

const Vector CMovingPhysicalAgentComponent::GetInternalVelocity()
{
#ifdef USE_PHYSX
	if( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->GetInternalVelocity();
	}
#endif
	return Vector::ZEROS;
}

const Vector CMovingPhysicalAgentComponent::GetExternalDisp()
{
#ifdef USE_PHYSX
	if( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->GetExternalDisp();
	}
#endif
	return Vector::ZEROS;
}

const Vector CMovingPhysicalAgentComponent::GetInputDisp()
{
#ifdef USE_PHYSX
	if( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->GetInputDisp();
	}
#endif
	return Vector::ZEROS;
}

const Float CMovingPhysicalAgentComponent::GetSpeedMul()
{
#ifdef USE_PHYSX
	if( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->GetSpeedMul();
	}
#endif
	return -1.0f;
}

const Bool CMovingPhysicalAgentComponent::IsOnPlatform()
{
#ifdef USE_PHYSX
	if( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->IsOnPlatform();
	}
#endif
	return false;
}

const Vector& CMovingPhysicalAgentComponent::GetPlatformLocalPos()
{
#ifdef USE_PHYSX
	if( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->GetPlatformLocalPos();
	}
#endif
	return Vector::ZEROS;
}

const Float CMovingPhysicalAgentComponent::GetPlatformRotation()
{
#ifdef USE_PHYSX
	if( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->GetPlatformRotation();
	}
#endif
	return 0.0f;
}

const Bool CMovingPhysicalAgentComponent::IsCollisionPredictionEnabled()
{
#ifdef USE_PHYSX
	if( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->IsCollisionPredictionEnabled();
	}
#endif
	return false;
}

const Bool CMovingPhysicalAgentComponent::IsNearWater()
{
#ifdef USE_PHYSX
	if( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->IsNearWater();
	}
#endif
	return false;
}

const Bool CMovingPhysicalAgentComponent::CanPush()
{
#ifdef USE_PHYSX
	if( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->CanPush();
	}
#endif
	return false;
}

const Bool CMovingPhysicalAgentComponent::IsShapeHit()
{
#ifdef USE_PHYSX
	if( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->IsShapeHit();
	}
#endif
	return false;
}

const Bool CMovingPhysicalAgentComponent::IsSlidingEnabled()
{
#ifdef USE_PHYSX
	if( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->IsSlidingEnabled();
	}
#endif
	return false;
}

const Bool CMovingPhysicalAgentComponent::IsUpdatingVirtualRadius()
{
#ifdef USE_PHYSX
	if( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->IsUpdatingVirtualRadius();
	}
#endif
	return false;
}

const Float	CMovingPhysicalAgentComponent::GetPushingTime()
{
#ifdef USE_PHYSX
	if( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->GetPushingTime();
	}
#endif
	return 0.0f;
}

void CMovingPhysicalAgentComponent::CreateCharacterCapsuleWS( FixedCapsule& capsule ) const
{
#ifdef USE_PHYSX
	if( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		const Float height = m_physRepresentation->GetCharacterController()->GetCurrentHeight();
		const Float radius = m_physRepresentation->GetCharacterController()->GetCurrentCharacterRadius();

		static Float factor1 = 1.1f;
		static Float factor2 = 1.2f;
		const Float radius2 = Min< Float >( radius, height / 2.0f - 0.1f );
		capsule.Set( m_physRepresentation->GetRepresentationPosition(), factor1 * radius2, factor2 * height );
	}
#endif
}

void CMovingPhysicalAgentComponent::funcIsPhysicalMovementEnabled( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsPhysicalRepresentationEnabled() );
}

void CMovingPhysicalAgentComponent::funcSetAnimatedMovement( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, enable, false );
	FINISH_PARAMETERS;

	SetAnimatedMovement( enable );
}

void CMovingPhysicalAgentComponent::funcIsAnimatedMovement( CScriptStackFrame& stack, void* result )
{
	Bool res = false;
	FINISH_PARAMETERS;

#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		res = m_physRepresentation->GetCharacterController()->IsAnimated();
	}
#endif
	RETURN_BOOL( res );
}

void CMovingPhysicalAgentComponent::funcGetPhysicalState( CScriptStackFrame& stack, void* result )
{
	ECharacterPhysicsState res = CPS_Count;
	FINISH_PARAMETERS;

#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		res = m_physRepresentation->GetCharacterController()->GetState();
	}
#endif
	RETURN_ENUM( res );
}

void CMovingPhysicalAgentComponent::funcSetGravity( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, flag, false );
	FINISH_PARAMETERS;

	SetGravity( flag );
}

void CMovingPhysicalAgentComponent::funcSetBehaviorCallbackNeed( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, flag, false );
	FINISH_PARAMETERS;

	SetNeedsBehaviorCallback( flag );
}

void CMovingPhysicalAgentComponent::funcSetSwimming( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, flag, false );
	FINISH_PARAMETERS;

	SetSwimming( flag );
}

void CMovingPhysicalAgentComponent::funcGetWaterLevel( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_FLOAT( GetWaterLevel() );
}

void CMovingPhysicalAgentComponent::funcGetSubmergeDepth( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_FLOAT( GetSubmergeDepth() );
}

void CMovingPhysicalAgentComponent::funcSetDiving( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, diving, false );
	FINISH_PARAMETERS;

	SetDiving( diving );
}

void CMovingPhysicalAgentComponent::funcIsDiving( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( IsDiving() );
}

void CMovingPhysicalAgentComponent::funcSetEmergeSpeed( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, value, GGame->GetGameplayConfig().m_emergeSpeed );
	FINISH_PARAMETERS;

	if( m_physRepresentation )
	{
		m_physRepresentation->SetEmergeSpeed( value );
	}
}

void CMovingPhysicalAgentComponent::funcGetEmergeSpeed( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Float emergeSpeed = GGame->GetGameplayConfig().m_emergeSpeed;

	if( m_physRepresentation )
	{
		emergeSpeed = m_physRepresentation->GetEmergeSpeed();
	}

	RETURN_FLOAT( emergeSpeed );
}

void CMovingPhysicalAgentComponent::funcSetSubmergeSpeed( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, value, GGame->GetGameplayConfig().m_submergeSpeed );
	FINISH_PARAMETERS;

	if( m_physRepresentation )
	{
		m_physRepresentation->SetSubmergeSpeed( value );
	}
}

void CMovingPhysicalAgentComponent::funcGetSubmergeSpeed( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Float submergeSpeed = GGame->GetGameplayConfig().m_submergeSpeed;

	if( m_physRepresentation )
	{
		submergeSpeed = m_physRepresentation->GetSubmergeSpeed();
	}

	RETURN_FLOAT( submergeSpeed );
}

void CMovingPhysicalAgentComponent::funcSetRagdollPushingMul( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, value, 1.0f );
	FINISH_PARAMETERS;

	#ifdef USE_PHYSX
	if( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		m_physRepresentation->GetCharacterController()->SetRagdollPushMultiplier( value );
	}
	#endif
}

void CMovingPhysicalAgentComponent::funcGetRagdollPushingMul( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Float pushingMul = 1.0f;

	#ifdef USE_PHYSX
	if( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		pushingMul = m_physRepresentation->GetCharacterController()->GetRagdollPushMultiplier();
	}
	#endif

	RETURN_FLOAT( pushingMul );
}

void CMovingPhysicalAgentComponent::funcSetPushable( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, flag, false );
	FINISH_PARAMETERS;

	HALT( "deprecated - use interaction prioprites instead" );
}

void CMovingPhysicalAgentComponent::funcIsOnGround( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Bool ret = true;
	if ( m_physRepresentation && IsPhysicalRepresentationEnabled() )
	{
		ret = IsCollidingDown();
	}
	RETURN_BOOL( ret );
}

void CMovingPhysicalAgentComponent::funcIsCollidesWithCeiling( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Bool ret = false;

	#ifdef USE_PHYSX
	if ( m_physRepresentation && IsPhysicalRepresentationEnabled() )
	{
		ret = IsCollidingUp();
	}
	#endif

	RETURN_BOOL( ret );
}

void CMovingPhysicalAgentComponent::funcIsCollidesOnSide( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Bool ret = false;

	#ifdef USE_PHYSX
	if ( m_physRepresentation && IsPhysicalRepresentationEnabled() )
	{
		ret = IsCollidingSide();
	}
	#endif

	RETURN_BOOL( ret );
}

void CMovingPhysicalAgentComponent::funcIsFalling( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Bool ret = false;
	if ( m_physRepresentation && m_physRepresentation->IsPhysicalMovementEnabled() )
	{
		ret = m_physRepresentation->IsFalling();
	}
	RETURN_BOOL( ret );
}
//////////////////////////////////////////////////////////////////////////
//
// check is sliding
void CMovingPhysicalAgentComponent::funcIsSliding( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Bool ret = false;
	if ( m_physRepresentation && m_physRepresentation->IsPhysicalMovementEnabled() )
	{
		ret = m_physRepresentation->IsSliding();
	}
	RETURN_BOOL( ret );
}
//////////////////////////////////////////////////////////////////////////
//
// get current sliding coef
void CMovingPhysicalAgentComponent::funcGetSlideCoef( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Float coef = 0.0f;
	if ( m_physRepresentation && m_physRepresentation->IsPhysicalMovementEnabled() )
	{
		coef = m_physRepresentation->GetSlideCoef();
	}
	RETURN_FLOAT( coef );
}
//////////////////////////////////////////////////////////////////////////
//
// get sliding direction
void CMovingPhysicalAgentComponent::funcGetSlideDir( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Vector dir = Vector::ZEROS;
	if ( m_physRepresentation && m_physRepresentation->IsPhysicalMovementEnabled() )
	{
		dir = m_physRepresentation->GetSlidingDir();
	}
	RETURN_STRUCT( Vector, dir );
}
//////////////////////////////////////////////////////////////////////////
//
// sliding speed
void CMovingPhysicalAgentComponent::funcSetSlidingSpeed( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, speed, 1.0f );
	FINISH_PARAMETERS;

	#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		m_physRepresentation->GetCharacterController()->SetSlidingSpeed( speed );
	}
	#endif
}
//////////////////////////////////////////////////////////////////////////
//
// sliding limits
void CMovingPhysicalAgentComponent::funcSetSlidingLimits( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, min, GGame->GetGameplayConfig().m_slidingLimitMin );
	GET_PARAMETER( Float, max, GGame->GetGameplayConfig().m_slidingLimitMax );
	FINISH_PARAMETERS;

#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		m_physRepresentation->GetCharacterController()->SetSlidingLimits( min, max );
	}
#endif
}
//////////////////////////////////////////////////////////////////////////
//
// sliding enable/disable
void CMovingPhysicalAgentComponent::funcSetSliding( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, enable, false );
	FINISH_PARAMETERS;

#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		m_physRepresentation->GetCharacterController()->SetSliding( enable );
	}
#endif
}
//////////////////////////////////////////////////////////////////////////
//
// enable addition iteration in vertical sliding to solve physX problems
void CMovingPhysicalAgentComponent::funcEnableAdditionalVerticalSlidingIteration( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, enable, false );
	FINISH_PARAMETERS;

	EnableAdditionalVerticalSlidingIteration( enable );
}
//////////////////////////////////////////////////////////////////////////
//
// check is flag enabled
void CMovingPhysicalAgentComponent::funcIsAdditionalVerticalSlidingIterationEnabled( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( IsAdditionalVerticalSlidingIterationEnabled() );
}
//////////////////////////////////////////////////////////////////////////
//
// terrain limits
void CMovingPhysicalAgentComponent::funcSetTerrainLimits( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, min, GGame->GetGameplayConfig().m_terrainInfluenceLimitMin );
	GET_PARAMETER( Float, max, GGame->GetGameplayConfig().m_terrainInfluenceLimitMax );
	FINISH_PARAMETERS;

#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		m_physRepresentation->GetCharacterController()->SetTerrainLimits( min, max );
	}
#endif
}
void CMovingPhysicalAgentComponent::funcSetTerrainInfluence( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, mul, GGame->GetGameplayConfig().m_terrainInfluenceMul );
	FINISH_PARAMETERS;

#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		m_physRepresentation->GetCharacterController()->SetTerrainInfluenceMultiplier( mul );
	}
#endif
}

void CMovingPhysicalAgentComponent::funcApplyVelocity( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, vel, Vector::ZEROS );
	FINISH_PARAMETERS;

	if( m_physRepresentation )
	{
		m_physRepresentation->ApplyVelocity( vel );
	}
}

void CMovingPhysicalAgentComponent::funcRegisterEventListener( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle<IScriptable>, listener, nullptr );
	FINISH_PARAMETERS;

	if( m_physRepresentation )
	{
		m_physRepresentation->RegisterEventListener( listener );
	}
}

void CMovingPhysicalAgentComponent::funcUnregisterEventListener( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle<IScriptable>, listener, nullptr );
	FINISH_PARAMETERS;

	if( m_physRepresentation )
	{
		m_physRepresentation->UnregisterEventListener( listener );
	}
}

void CMovingPhysicalAgentComponent::funcGetCapsuleHeight( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Float returnValue( SCCTDefaults::DEFAULT_HEIGHT );
#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		returnValue = m_physRepresentation->GetCharacterController()->GetCurrentHeight();
	}
#endif

	RETURN_FLOAT( returnValue );
}

void CMovingPhysicalAgentComponent::funcGetCapsuleRadius( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Float returnValue = m_baseRadius;
#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		returnValue = m_physRepresentation->GetCharacterController()->GetCurrentCharacterRadius();
	}
#endif

	RETURN_FLOAT( returnValue );
}

void CMovingPhysicalAgentComponent::funcGetSlopePitch( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_FLOAT( GetSlopePitch() );
}
//////////////////////////////////////////////////////////////////////////
//
// terrain normal
void CMovingPhysicalAgentComponent::funcGetTerrainNormal( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, damped, false );
	FINISH_PARAMETERS;

	RETURN_STRUCT( Vector, GetTerrainNormal( damped ) );
}

void CMovingPhysicalAgentComponent::funcGetTerrainNormalWide( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Vector, normalAverage, Vector::EY );
	GET_PARAMETER_REF( Vector, normalGlobal, Vector::EY );
	GET_PARAMETER( Vector, directionToCheck, Vector::EX );
	GET_PARAMETER( Float, separationH, 0.5f );
	GET_PARAMETER( Float, separationF, 1.0f );
	GET_PARAMETER( Float, separationB, 0.4f );

	FINISH_PARAMETERS;

#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		m_physRepresentation->GetCharacterController()->ProbeTerrainWide( normalAverage, normalGlobal, directionToCheck, separationH, separationF, separationB );
		normalAverage.Normalize3();
	}
#endif
}
//////////////////////////////////////////////////////////////////////////
//
// set pitch
void CMovingPhysicalAgentComponent::funcSetVirtualControllersPitch( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, pitch, 0.0f );
	FINISH_PARAMETERS;

#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		m_physRepresentation->GetCharacterController()->SetVirtualControllersPitch( pitch );
	}
#endif
}
//////////////////////////////////////////////////////////////////////////
//
// return collision data count
void CMovingPhysicalAgentComponent::funcGetCollisionDataCount( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		RETURN_INT( m_physRepresentation->GetCharacterController()->GetCollisionObstaclesDataCount() );
		return;
	}
	#endif

	RETURN_INT( 0 );
}
//////////////////////////////////////////////////////////////////////////
//
// return collision data by index
void CMovingPhysicalAgentComponent::funcGetCollisionData( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint32, index, 0 );
	FINISH_PARAMETERS;

	#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		RETURN_STRUCT( SCollisionData, m_physRepresentation->GetCharacterController()->GetCollisionObstaclesData( index ) );
		return;
	}
	#endif

	RETURN_STRUCT( SCollisionData, SCollisionData::EMPTY );
}
//////////////////////////////////////////////////////////////////////////
//
// return collision character data count
void CMovingPhysicalAgentComponent::funcGetCollisionCharacterDataCount( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		RETURN_INT( m_physRepresentation->GetCharacterController()->GetCollisionCharactersDataCount() );
		return;
	}
#endif

	RETURN_INT( 0 );
}
//////////////////////////////////////////////////////////////////////////
//
// return collision data by index
void CMovingPhysicalAgentComponent::funcGetCollisionCharacterData( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint32, index, 0 );
	FINISH_PARAMETERS;

#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		RETURN_STRUCT( SCollisionData, m_physRepresentation->GetCharacterController()->GetCollisionCharactersData( index ) );
		return;
	}
#endif

	RETURN_STRUCT( SCollisionData, SCollisionData::EMPTY );
}
//////////////////////////////////////////////////////////////////////////
//
// returns if there is a raycast collision in the drid below the character
void CMovingPhysicalAgentComponent::funcGetGroundGridCollisionOn( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( ECollisionSides, side, CS_CENTER );
	FINISH_PARAMETERS;

#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		RETURN_BOOL( m_physRepresentation->GetCharacterController()->GetGroundGridCollisionOn(side) );
		return;
	}
#endif

	RETURN_BOOL( true );
}

//////////////////////////////////////////////////////////////////////////
//
// enable/disable collision prediction
void CMovingPhysicalAgentComponent::funcEnableCollisionPrediction( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, enable, false );
	FINISH_PARAMETERS;

#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		m_physRepresentation->GetCharacterController()->EnableCollisionPrediction( enable );
	}
#endif
}
//////////////////////////////////////////////////////////////////////////
//
// enable/disable virtual controller
void CMovingPhysicalAgentComponent::EnableVirtualController( const CName& virtualControllerName, const Bool enabled )
{
	// call superclass
	CMovingAgentComponent::EnableVirtualController( virtualControllerName, enabled );

	#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() && virtualControllerName != CName::NONE )
	{
		auto& vccArray = m_physRepresentation->GetCharacterController()->GetVirtualControllers();
		const Uint32 vCount = vccArray.Size();
		for( Uint32 vi=0; vi<vCount; ++vi)
		{
			CVirtualCharacterController& vcc = vccArray[vi];
			if( vcc.GetName() == virtualControllerName )
			{
				vcc.SetEnabled( enabled );
				return;
			}
		}

		RED_LOG_ERROR( RED_LOG_CHANNEL( CharacterController ), TXT("Virtual controller with name %s not found!"), virtualControllerName.AsString().AsChar() );
	}
	#endif
}
//////////////////////////////////////////////////////////////////////////
//
// enable/disable virtual controller collision response
void CMovingPhysicalAgentComponent::funcEnableVirtualControllerCollisionResponse( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, virtualControllerName, CName::NONE );
	GET_PARAMETER( Bool, enable, false );
	FINISH_PARAMETERS;

	#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		// If setting radius for virtual controller
		if ( virtualControllerName != CName::NONE )
		{
			auto& vccArray = m_physRepresentation->GetCharacterController()->GetVirtualControllers();
			const Uint32 vCount = vccArray.Size();
			for( Uint32 vi = 0; vi < vCount; ++vi )
			{
				CVirtualCharacterController& vcc = vccArray[vi];
				if( vcc.GetName() == virtualControllerName )
				{
					vcc.EnableCollisionResponse( enable );
					return;
				}
			}

			RED_LOG_ERROR( RED_LOG_CHANNEL( CharacterController ), TXT("Virtual controller with name %s not found!"), virtualControllerName.AsString().AsChar() );
		}
	}
	#endif
}
//////////////////////////////////////////////////////////////////////////
//
// return material name
void CMovingPhysicalAgentComponent::funcGetMaterialName( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	const SPhysicalMaterial* material = GetCurrentStandPhysicalMaterial();
	CName name = material ? material->m_name : CNAME( default );
	RETURN_NAME( name );
}

void CMovingPhysicalAgentComponent::ForceMoveToPosition( const Vector& position, Bool resetZAxis )
{
    m_physRepresentation->ForceMoveToPosition(position, resetZAxis);
}

// Generate editor rendering fragments
void CMovingPhysicalAgentComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	TBaseClass::OnGenerateEditorFragments( frame, flags );

	// Generate fragments for controllers in preview
	if( flags == SHOW_PhysXTraceVisualization && GetWorld()->GetPreviewWorldFlag() )
	{
		CEntityTemplate* eTemplate = GetEntity()->GetEntityTemplate();
		if ( eTemplate )
		{
			const CCharacterControllerParam* ccParams = eTemplate->FindGameplayParamT< CCharacterControllerParam > ( true );
			if( ccParams != nullptr )
			{
				const Matrix& transform = GetLocalToWorld();				
				const Vector position = transform.GetTranslation();
				FixedCapsule caps( position, ccParams->m_physicalRadius, ccParams->m_height );
				frame->AddDebugCapsule( caps, Matrix::IDENTITY, Color::YELLOW );

				const Uint32 ccno = ccParams->m_virtualControllers.Size();
				if( ccno > 0 )
				{	
					for( Uint32 i=0; i<ccno; ++i )
					{
						const SVirtualControllerParams& vccParam = ccParams->m_virtualControllers[i];
						Int32 boneIndex = FindBoneByName( vccParam.m_boneName );
						if ( boneIndex != -1 )
						{
							if ( vccParam.m_localOffsetInModelSpace )
							{
								Matrix boneMatrix = GetBoneMatrixWorldSpace( boneIndex );
								Vector acOffset = transform.TransformPoint( vccParam.m_localOffset ) - transform.GetTranslationRef();
								caps.Set( boneMatrix.GetTranslationRef() + acOffset, vccParam.m_radius, vccParam.m_height );
							}
							else
							{
								caps.Set( GetBoneMatrixWorldSpace( boneIndex ).TransformPoint( vccParam.m_localOffset ), vccParam.m_radius, vccParam.m_height );
							}
						}
						else
						{
							caps.Set( transform.TransformPoint( vccParam.m_localOffset ), vccParam.m_radius, vccParam.m_height );
						}
						frame->AddDebugCapsule( caps, Matrix::IDENTITY, vccParam.m_enabled ? Color::MAGENTA : Color::DARK_MAGENTA );
					}
				}
			}
		}
	}

	if ( m_physRepresentation  )
	{
		m_physRepresentation->OnGenerateEditorFragments( frame, flags );
	}
}

InteractionPriorityType CMovingPhysicalAgentComponent::GetInteractionPriority() const
{
	InteractionPriorityType returnValue( SCCTDefaults::DEFAULT_INTERACTION_PRIORITY );
#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		returnValue = m_physRepresentation->GetCharacterController()->GetInteractionPriority();
	}
#endif
	return returnValue;
}

InteractionPriorityType CMovingPhysicalAgentComponent::SetInteractionPriority( InteractionPriorityType interactionPriority )
{
	InteractionPriorityType returnValue( SCCTDefaults::DEFAULT_INTERACTION_PRIORITY );
#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		returnValue = m_physRepresentation->GetCharacterController()->SetInteractionPriority( interactionPriority );
	}
#endif
	return returnValue;
}

void CMovingPhysicalAgentComponent::SetOriginalInteractionPriority( InteractionPriorityType interactionPriority )
{
	m_pushPriorityOriginal	= interactionPriority;
}

void CMovingPhysicalAgentComponent::RestoreOriginalInteractionPriority()
{
	SetInteractionPriority( m_pushPriorityOriginal );
}

InteractionPriorityType CMovingPhysicalAgentComponent::GetOriginalInteractionPriority()
{
	return m_pushPriorityOriginal;
}

CMovingPhysicalAgentComponent* CMovingPhysicalAgentComponent::SetUnpushableTarget( CMovingPhysicalAgentComponent* targetMAC )
{
	CMovingPhysicalAgentComponent *returnValue = nullptr;
#ifdef USE_PHYSX
	if ( m_physRepresentation == nullptr )
    {
        ASSERT( m_physRepresentation );
        return nullptr;
    }

	if( m_physRepresentation->GetCharacterController() == nullptr )
    {
        ASSERT( m_physRepresentation->GetCharacterController() );
        return nullptr;
    }
		
	if ( targetMAC )
	{	
		if ( targetMAC->m_physRepresentation )
		{
			CPhysicsCharacterWrapper *wrapper = m_physRepresentation->GetCharacterController()->SetUnpushableTarget( targetMAC->m_physRepresentation->GetCharacterController() );
			if ( wrapper )
			{
				CComponent *comp = nullptr;
				if ( wrapper->GetParent( comp ) )
				{
					returnValue = Cast< CMovingPhysicalAgentComponent > ( comp );
				}
			}
		}
	}
	else if ( m_physRepresentation->GetCharacterController() )
	{
		m_physRepresentation->GetCharacterController()->SetUnpushableTarget( nullptr );
	}
#endif

	return returnValue;
}

InteractionPriorityType CMovingPhysicalAgentComponent::GetActualInteractionPriority() const
{
#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->GetInteractionPriority();
	}
#endif
	return SCCTDefaults::DEFAULT_INTERACTION_PRIORITY;
}

void CMovingPhysicalAgentComponent::InvalidatePhysicsCache()
{
#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		m_physRepresentation->GetCharacterController()->InvalidatePhysicsCache();
	}
#endif
}

void CMovingPhysicalAgentComponent::EnableStaticCollisions( Bool enable )
{
#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		m_physRepresentation->GetCharacterController()->EnableStaticCollisions( enable );
	}
#endif
}

Bool CMovingPhysicalAgentComponent::IsStaticCollisionsEnabled()
{
#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->IsStaticCollisionsEnabled();
	}
#endif
	return false;
}

void CMovingPhysicalAgentComponent::EnableDynamicCollisions( Bool enable )
{
#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		m_physRepresentation->GetCharacterController()->EnableDynamicCollisions( enable );
	}
#endif
}

Bool CMovingPhysicalAgentComponent::IsDynamicCollisionsEnabled()
{
#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->IsDynamicCollisionsEnabled();
	}
#endif
	return false;
}

const Bool CMovingPhysicalAgentComponent::GetGroundGridCollisionOn( const Uint32 dir )
{
#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->GetGroundGridCollisionOn( (ECollisionSides)dir );
	}
#endif
	return false;
}

void CMovingPhysicalAgentComponent::EnableCombatMode( Bool combat )
{
	// call superclass
	CMovingAgentComponent::EnableCombatMode( combat );

	// do nothing for now
	// we can set virtual radiuses via SetVirtualRadius()
}

void CMovingPhysicalAgentComponent::ResetCollisionCharactersData()
{
#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		m_physRepresentation->GetCharacterController()->ResetCollisionCharactersData();
	}
#endif
}

void CMovingPhysicalAgentComponent::ResetCollisionObstaclesData()
{
#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		m_physRepresentation->GetCharacterController()->ResetCollisionObstaclesData();
	}
#endif
}

void CMovingPhysicalAgentComponent::SetVirtualRadius( const CName& radiusName, Bool immediately, const CName& virtualControllerName )
{
	// call superclass
	CMovingAgentComponent::SetVirtualRadius( radiusName, immediately, virtualControllerName );

#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		CEntityTemplate *templ = GetEntity()->GetEntityTemplate();
		const CCharacterControllerParam* params( nullptr );
		if ( templ )
		{
			params = templ->FindGameplayParamT< CCharacterControllerParam > ( true );
			if ( params )
			{
				const Uint32 count = params->m_radiuses.Size();
				for ( Uint32 i=0; i<count; ++i )
				{
					// found?
					if ( params->m_radiuses[i].m_name == radiusName )
					{
                        // set on virtual controller
                        if ( virtualControllerName != CName::NONE )
                        {
                            auto& vccArray = m_physRepresentation->GetCharacterController()->GetVirtualControllers();
                            const Uint32 vCount = vccArray.Size();
                            for( Uint32 vi=0; vi<vCount; ++vi)
                            {
                                CVirtualCharacterController& vcc = vccArray[vi];
                                if( vcc.GetName() == virtualControllerName )
                                {
                                    vcc.SetVirtualRadius( params->m_radiuses[i].m_radius );
                                    return;
                                }
                            }

                            RED_LOG_ERROR( RED_LOG_CHANNEL( CharacterController ), TXT("Virtual controller with name %s not found!"), virtualControllerName.AsString().AsChar() );
                        }

                        // or on main controller
                        else
						{
						    m_physRepresentation->GetCharacterController()->SetVirtualRadius( params->m_radiuses[i].m_radius, immediately );
						}

						return;
					}
				}
				
				// not found?
				RED_LOG_ERROR( RED_LOG_CHANNEL( CharacterController ), TXT("Radius with name %s not found!"), radiusName.AsString().AsChar() );
				m_physRepresentation->GetCharacterController()->ResetVirtualRadius();
			}
		}
	}
#endif
}

const Float CMovingPhysicalAgentComponent::GetVirtualRadius()
{
#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->GetVirtualRadius();
	}
#endif

	return -1.0f;
}

void CMovingPhysicalAgentComponent::ResetVirtualRadius( const CName& virtualControllerName )
{
	// call superclass
	CMovingAgentComponent::ResetVirtualRadius( virtualControllerName );

#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
        // If resetting radius for virtual controller
        if( virtualControllerName != CName::NONE )
        {
            auto& vccArray = m_physRepresentation->GetCharacterController()->GetVirtualControllers();
            const Uint32 vCount = vccArray.Size();
            for( Uint32 vi=0; vi<vCount; ++vi)
            {
                CVirtualCharacterController& vcc = vccArray[vi];
                if( vcc.GetName() == virtualControllerName )
                {
                    vcc.ResetVirtualRadius();
                    return;
                }
            }

            RED_LOG_ERROR( RED_LOG_CHANNEL( CharacterController ), TXT("Virtual controller with name %s not found!"), virtualControllerName.AsString().AsChar() );
        }
        // If resetting radius for main controller
        else
		    m_physRepresentation->GetCharacterController()->ResetVirtualRadius();
	}
#endif
}

void CMovingPhysicalAgentComponent::SetHeight( const Float height )
{
	// call superclass
	CMovingAgentComponent::SetHeight( height );

#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		m_physRepresentation->GetCharacterController()->SetHeight( height );
	}
#endif
}

const Float CMovingPhysicalAgentComponent::GetHeight()
{
#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->GetCurrentHeight();
	}
#endif

	return -1.0f;
}

void CMovingPhysicalAgentComponent::ResetHeight()
{
	// call superclass
	CMovingAgentComponent::ResetHeight();

#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		m_physRepresentation->GetCharacterController()->ResetHeight();
	}
#endif
}

Bool CMovingPhysicalAgentComponent::DoTraceZTest( const Vector& pointWS, Vector& outPosition ) const
{
#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->DoTraceZTest( pointWS, outPosition );
	}
#endif

	return false;
}

Float CMovingPhysicalAgentComponent::GetSlopePitch() const
{
	return m_physRepresentation ? m_physRepresentation->GetSlopePitch() : 0.f;
}

Float CMovingPhysicalAgentComponent::GetPhysicalPitch() const
{
	#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		return m_physRepresentation->GetCharacterController()->GetVirtualControllerPitch();
	}
	#endif

	return 0.0f;
}

Bool CMovingPhysicalAgentComponent::IsCollidingDown() const
{
	Bool ret = true;

#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		ret = m_physRepresentation->GetCharacterController()->DownCollision();
	}
#endif

	return ret;
}

Bool CMovingPhysicalAgentComponent::IsCollidingUp() const
{
	Bool ret = false;

#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		ret = m_physRepresentation->GetCharacterController()->UpCollision();
	}
#endif

	return ret;
}

Bool CMovingPhysicalAgentComponent::IsCollidingSide() const
{
	Bool ret = false;

#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		ret = m_physRepresentation->GetCharacterController()->SideCollision();
	}
#endif

	return ret;
}

Vector CMovingPhysicalAgentComponent::GetTerrainNormal( Bool damped ) const
{
	Vector normal = Vector::EY;

#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		normal = m_physRepresentation->GetCharacterController()->GetTerrainNormal( damped );
		normal.Normalize3();
	}
#endif

	return normal;
}

Float CMovingPhysicalAgentComponent::GetPhysicalRadius() const
{
	Float radius = 0.0f;

#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		radius = m_physRepresentation->GetCharacterController()->GetPhysicalRadius();
	}
#endif

	return radius;
}

const Float CMovingPhysicalAgentComponent::GetCurrentRadius()
{
	Float radius = 0.0f;

#ifdef USE_PHYSX
	if ( m_physRepresentation && m_physRepresentation->GetCharacterController() )
	{
		radius = m_physRepresentation->GetCharacterController()->GetCurrentCharacterRadius();
	}
#endif

	return radius;
}

void CMovingPhysicalAgentComponent::onCharacterTouch( THandle< IScriptable > m_triggeredComponent, SActorShapeIndex& m_triggeredBodyIndex )
{
	Bool isOwnerNPC = GetEntity()->IsA< CNewNPC >();
	if( isOwnerNPC )
	{
		CRigidMeshComponent* rigidMesh = Cast< CRigidMeshComponent >( m_triggeredComponent.Get() );
		if( rigidMesh && rigidMesh->GetPhysicalCollision() == CPhysicalCollision( CNAME( RigidBody ) ) )
		{
			CEntity* entity = rigidMesh->GetEntity();
			CLayer* layer = entity->GetLayer();	
			rigidMesh->SetNoDisolves( false );
			layer->DestroyEntity( entity );									
		}
	}
}

const SPhysicalMaterial* CMovingPhysicalAgentComponent::GetCurrentStandPhysicalMaterial() const
{
	{
		const SPhysicalMaterial* ret = TBaseClass::GetCurrentStandPhysicalMaterial();
		if ( ret )
		{
			return ret;
		}
	}
	
#ifdef USE_PHYSX
	CPhysicsCharacterWrapper* wrapper = m_physRepresentation->GetCharacterController();
	if( wrapper && wrapper->IsReady() && m_physRepresentation->IsPhysicalMovementEnabled() )
	{
		Float waterLevel = wrapper->GetWaterLevel();
		Float footPosition = wrapper->GetPosition().Z;
		if( waterLevel == -10000.0f || waterLevel < footPosition )
		{
			return wrapper->GetCurrentStandPhysicalMaterial();
		}
		else if( waterLevel - footPosition < SPhysicsSettings::m_characterFootstepWaterLevelLimit )
		{
			static const SPhysicalMaterial* material = GPhysicEngine->GetMaterial( CNAME( water_shallow ) );
			return material;
		}
		else
		{
			static const SPhysicalMaterial* material = GPhysicEngine->GetMaterial( CNAME( water_deep ) );
			return material;
		}
	}
	else if( CEntity* entity = GetEntity() )
	{
		if( CActor* actor = Cast< CActor >( entity ) )
		{
			if( CWorld* world = GetWorld() )
			{
				Vector footPosition = Vector::ZEROS;
				if(wrapper && wrapper->IsReady() )
				{
					footPosition = wrapper->GetPosition();
				}
				else
				{
					footPosition = actor->GetHeadPosition();

					CPhysicsWorld* physicsWorld = nullptr;
					if( world->GetPhysicsWorld( physicsWorld ) )
					{
						const CCharacterControllerParam* param = m_physRepresentation->GetCharacterControllerParam();
						Float height = param->m_height;

						footPosition += ( physicsWorld->GetGravityVector().Normalized4() * height );
					}
				}
				Float footPositionZ = footPosition.Z;
				Float waterLevel = -10000.0f;

				waterLevel = world->GetWaterLevel( footPosition, 1 );

				if( waterLevel == -10000.0f || waterLevel < footPositionZ )
				{
					CPhysicsWorld* physicsWorld = nullptr;
					if( world->GetPhysicsWorld( physicsWorld ) )
					{
						const CCharacterControllerParam* param = m_physRepresentation->GetCharacterControllerParam();
						return CPhysicsCharacterWrapper::GetCurrentStandPhysicalMaterial( actor->GetHeadPosition(), param->m_height * 2.0f, param->m_collisionType, physicsWorld );
					}
				}
				else if( waterLevel - footPositionZ < SPhysicsSettings::m_characterFootstepWaterLevelLimit )
				{
					static const SPhysicalMaterial* material = GPhysicEngine->GetMaterial( CNAME( water_shallow ) );
					return material;
				}
				else
				{
					static const SPhysicalMaterial* material = GPhysicEngine->GetMaterial( CNAME( water_deep ) );
					return material;
				}
			}
		}
	}
#endif
	return nullptr;
}

ECharacterPhysicsState CMovingPhysicalAgentComponent::GetCurrentPhysicsState() const
{
#ifdef USE_PHYSX
	if( !m_physRepresentation ) 
	{
		return TBaseClass::GetCurrentPhysicsState();
	}
	CPhysicsCharacterWrapper* wrapper = m_physRepresentation->GetCharacterController();
	if ( wrapper && m_representationStack->GetActiveRepresentation() == m_physRepresentation )
	{
#ifdef USE_PHYSX
		return wrapper->GetState();
#endif
	}
#endif // USE_PHYSX

	return TBaseClass::GetCurrentPhysicsState();
};

CMovingPhysicalAgentComponent::SeparateFunctor::SeparateFunctor( CMovingPhysicalAgentComponent* self, CCharacterControllersManager* mgr, ResolveSeparationContext& context )
	: m_context( context )
	, m_self( self )
	, m_mgr( mgr )
	, m_colliders( 0 )
	, m_separation( Vector::ZEROS )
{
}

Bool CMovingPhysicalAgentComponent::SeparateFunctor::operator()( const CActorsManagerMemberData& element )
{
#ifdef USE_PHYSX
	CActor* actor = element.m_actor;
	CMovingAgentComponent* colliderMAC = actor->GetMovingAgentComponent();
	if ( colliderMAC && colliderMAC->IsCharacterCollisionsEnabled() )
	{
		CMovingPhysicalAgentComponent* colliderMPAC = Cast<CMovingPhysicalAgentComponent>( colliderMAC );	

		const Bool alreadyTested = m_context.testedPairs.Exist( MakePair( colliderMPAC, m_self ) );

		if ( !alreadyTested && colliderMPAC && colliderMPAC != m_self && colliderMPAC->GetPhysicalCharacter() )
		{
			CPhysicsCharacterWrapper* collider = colliderMPAC->GetPhysicalCharacter()->GetCharacterController();
			if ( collider )
			{
				Vector thisPos = m_self->GetRepresentationStack()->GetStackPosition();
				Vector colliderPos = colliderMPAC->GetRepresentationStack()->GetStackPosition();

				auto sepResult = 
					m_mgr->ResolveControllerSeparation( 
						m_self->GetPhysicalCharacter()->GetCharacterController(), collider,
						thisPos, colliderPos,
						m_self->m_deltaPosition + m_separation, colliderMPAC->m_deltaPosition
					);

				m_context.testedPairs.Insert( MakePair( m_self, colliderMPAC ) );

				++m_colliders;

				if ( sepResult.m_first != Vector::ZEROS )
				{
					m_separation += sepResult.m_first;
					m_self->m_wasSeparatedBy = colliderMPAC;
				}

				if ( sepResult.m_second != Vector::ZEROS )
				{
					colliderMPAC->m_representationStack->OnSeparate( sepResult.m_second );
					colliderMPAC->m_moved = true;
					colliderMPAC->m_wasSeparatedBy = m_self;
				}
			}
		}
	}
#endif // USE_PHYSX

	return true;
}

Uint32 CMovingPhysicalAgentComponent::ResolveSeparation( ResolveSeparationContext& context )
{
#ifdef USE_PHYSX
	if ( CPhysicsCharacterWrapper* wrapper = m_physRepresentation->GetCharacterController() )
	{
		if ( ( HasMoved() || wrapper->IsUpdatingVirtualRadius() ) && IsCharacterCollisionsEnabled() )
		{
			CCharacterControllersManager* mgr = GetWorld()->GetCharacterControllerManager().Get();
			CActorsManager* actorsManager = GCommonGame->GetActorsManager();

			PC_SCOPE( FinalizeMovement_ResolveSeparation_Resolve );

			Box box( Vector::ZEROS, m_avoidanceRadius );
			box.Extrude( m_deltaPosition.Mag2() );
			SeparateFunctor separator( this, mgr, context );

			if ( CActor* actor = Cast< CActor >( GetEntity() ) )
			{
				actorsManager->TQuery( *actor, separator, box, true, nullptr, 0 );
			}
			else
			{
				actorsManager->TQuery( GetWorldPosition(), separator, box, true, nullptr, 0 );
			}

			if ( !separator.GetSeparation().AsVector2().IsAlmostZero() )
			{
				m_representationStack->OnSeparate( separator.GetSeparation() );
				m_moved = true;
			}

			return separator.GetColliders();
		}
	}
#endif

	return 0;
}
