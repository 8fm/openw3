#include "build.h"
#include "movableRepresentationPhysicalCharacter.h"
#include "movingPhysicalAgentComponent.h"
#include "movableRepresentationPathAgent.h"
#include "attackRange.h"
#include "vehicle.h"
#include "../../common/physics/physicsSettings.h"
#include "../../common/engine/physicsCharacterWrapper.h"
#include "../../common/engine/physicsDataProviders.h"

///////////////////////////////////////////////////////////////////////////////

#define FILTRED_FRAMES 5

///////////////////////////////////////////////////////////////////////////////

CMRPhysicalCharacter::CMRPhysicalCharacter( CMovingPhysicalAgentComponent& host )
	: m_host( host )
	, m_characterController( NULL )
	, m_actualRotation( EulerAngles::ZEROS )
    , m_prevPosition( Vector::ZEROS )
	, m_requestedMovementDelta( Vector::ZEROS )
{
}

void CMRPhysicalCharacter::EnablePhysicalMovement( Bool b )
{
#ifdef USE_PHYSX
	if ( m_characterController )
	{
		m_characterController->Enable( b );
	}
#endif
}

Bool CMRPhysicalCharacter::IsPhysicalMovementEnabled() const
{
	#ifdef USE_PHYSX
	return m_characterController->IsEnabled();
	#endif

	return false;
}

Bool CMRPhysicalCharacter::DoTraceZTest( const Vector& pointWS, Vector& outPosition ) const
{
#ifdef USE_PHYSX
	if ( m_characterController )
	{
		return m_characterController->DoTraceZTest( pointWS, outPosition );
	}
#endif

	return false;
}

CName CMRPhysicalCharacter::GetName() const 
{ 
	return CNAME( CMRPhysicalCharacter ); 
}

CMRPhysicalCharacter::~CMRPhysicalCharacter()
{
#ifdef USE_PHYSX
	if ( m_characterController )
	{
		m_characterController->Release();
		m_characterController = NULL;
	}
#endif
}

void CMRPhysicalCharacter::DestroyCharacterController()
{
#ifdef USE_PHYSX
	if ( m_characterController )
	{
		m_characterController->Release();
		m_characterController = NULL;
	}
#endif
}

void CMRPhysicalCharacter::SpawnCharacterController( const Vector& position, const EulerAngles& orientation )
{
	ASSERT( !m_characterController );

	CWorld* world = m_host.GetWorld();
	CPhysicsWorld* physics = nullptr;
	if( !world->GetPhysicsWorld( physics ) ) return;
	
	const CCharacterControllerParam* params = GetCharacterControllerParam();

	const Bool isPlayer = m_host.GetEntity()->IsPlayer();

	SPhysicsCharacterWrapperInit init;
	init.m_initialPosition = position;
	init.m_params = params;
	init.m_combatMode = 0 != ( m_host.GetMovementFlags() & MM_CombatMode );
	init.m_needsHitCallback = true;
	init.m_needsBehaviorCallback = IsBehaviorCallbackNeeded();
	init.m_canBePlayerControlled = isPlayer;
	init.m_vehicle = (m_host.GetEntity()->FindComponent<CVehicleComponent>() != nullptr);
	init.m_onTouchCallback = &m_host;

#ifdef USE_PHYSX
#ifndef RED_FINAL_BUILD
	if( !SPhysicsSettings::m_dontCreateCharacterControllers )
#endif
	{
		m_characterController = physics->GetWrappersPool< CPhysicsCharacterWrapper, SWrapperContext >()->Create( CPhysicsWrapperParentComponentProvider( &m_host ), init );
		if ( m_characterController )
		{
			m_characterController->SetCodeCallback( CPhysicsWrapperInterface::EPCCT_OnCollision, m_host.GetEntity()->QueryPhysicalCollisionTriggerCallback(), m_host.GetEntity() );
		}
	}
#endif

	m_actualRotation = orientation;
}

void CMRPhysicalCharacter::OnInit( const Vector& position, const EulerAngles& orientation )
{
	#ifdef USE_PHYSX
	if ( !m_characterController )
	{
		SpawnCharacterController( position, orientation );
	}
	#endif
}

void CMRPhysicalCharacter::OnActivate( const Vector& position, const EulerAngles& orientation )
{	
	#ifdef USE_PHYSX
	if ( !m_characterController )
	{
		SpawnCharacterController( position, orientation );
	}
	else
	{
		OnFollowPlacement( 0.0f, position, orientation );
	}
	#endif

	// enable physical controller
	EnablePhysicalMovement( true );

	#ifdef USE_PHYSX
	if ( m_characterController )
	{
		m_characterController->ForceSetPosition( position );
		m_characterController->CorrectPosition();
	}
	#endif
}

void CMRPhysicalCharacter::OnDeactivate()
{
	// disable physical controller
	EnablePhysicalMovement( false );
}

void CMRPhysicalCharacter::ForceUpdatePhysicalContextPosition( const Vector2& position )
{
#ifdef USE_PHYSX
	ASSERT ( m_characterController );
	
	if ( SWrapperContext* const context = m_characterController->GetPhysicsWorld()->GetWrappersPool< CPhysicsCharacterWrapper, SWrapperContext >()->GetContextAt( m_characterController->GetPoolIndex() ) )
	{
		context->m_x = position.X;
		context->m_y = position.Y;
	}
#endif
}

void CMRPhysicalCharacter::OnMove( const Vector& deltaPosition, const EulerAngles& deltaRotation )
{
	PC_SCOPE( CMRPhysicalCharacter_OnMove );

	m_actualRotation += deltaRotation;

	if ( Vector::Near3( deltaPosition, Vector::ZERO_3D_POINT, 0.0001f ) )
	{
		return;
	}

	Vector finalDelta = deltaPosition;
	if ( m_host.IsSnapedToNavigableSpace() && !finalDelta.AsVector2().IsZero() )
	{
		PC_SCOPE( CMRPhysicalCharacter_OnMove_SnapToNav );

		CPathAgent* pathAgent = m_host.GetPathAgent();
		if ( pathAgent )
		{
			pathAgent->StayOnNavdata( finalDelta.AsVector3() );
		}
	}

	m_requestedMovementDelta = finalDelta;

	#ifdef USE_PHYSX
	if ( m_characterController )
	{
		if ( m_characterController->IsReady() )
		{
			PC_SCOPE( CMRPhysicalCharacter_OnMove_Move );

			m_characterController->Move( finalDelta );
		}
		else
		{
			ForceUpdatePhysicalContextPosition( m_characterController->GetPosition().AsVector2() );
		}
	}
	#endif
}

void CMRPhysicalCharacter::OnSeparate( const Vector& deltaPosition )
{
#ifdef USE_PHYSX
	if ( m_characterController && m_characterController->IsReady() )
	{
		Vector finalDelta = deltaPosition;
		if ( m_host.IsSnapedToNavigableSpace() && !finalDelta.AsVector2().IsZero() )
		{
			if ( CPathAgent* pathAgent = m_host.GetPathAgent() )
			{
				pathAgent->StayOnNavdata( finalDelta.AsVector3() );
			}
		}
		m_characterController->GetCurrentMovementVectorRef() += finalDelta;
	}
#endif
}

void CMRPhysicalCharacter::OnSetPlacement( Float timeDelta, const Vector& newPosition, const EulerAngles& newOrientation, const Bool correctZ )
{
	PC_SCOPE( CMRPhysicalCharacter_OnSetPlacement );

	m_actualRotation = newOrientation;

#ifdef USE_PHYSX
	if ( !m_characterController )
	{
		return;
	}

	m_characterController->Teleport( newPosition, correctZ );
	m_characterController->UpdateVirtualControllers( timeDelta ); // we have to keep the vcs in sync
	m_characterController->UpdateVirtualRadius( timeDelta );

	if ( !m_characterController->IsReady() )
	{
		ForceUpdatePhysicalContextPosition( newPosition.AsVector2() );
	}
#endif
}


void CMRPhysicalCharacter::OnFollowPlacement( Float timeDelta, const Vector& newPosition, const EulerAngles& newOrientation )
{
	PC_SCOPE( CMRPhysicalCharacter_OnFollowPlacement );

	m_actualRotation = newOrientation;

#ifdef USE_PHYSX 
	if ( !m_characterController )
	{
		return;
	}

	m_characterController->ForceSetRawPosition( newPosition );		// update without sycing with physix - hack to speed up shit
	m_characterController->UpdateVirtualControllers( timeDelta );	// we have to keep the vcs in sync
	m_characterController->UpdateVirtualRadius( timeDelta );

	if ( !m_characterController->IsReady() )
	{
		ForceUpdatePhysicalContextPosition( newPosition.AsVector2() );
	}
#endif
}

Vector CMRPhysicalCharacter::GetRepresentationPosition( Bool smooth /*= false*/ ) const
{
	RED_UNUSED( smooth );

#ifdef USE_PHYSX
	if ( m_characterController && m_characterController->IsReady() )
	{
		return m_characterController->GetPosition();
	}
	else
#endif
	{
		return m_host.GetWorldPosition();
	}
}

Vector CMRPhysicalCharacter::GetRepresentationPosition( Int32 index ) const
{
#ifndef USE_PHYSX
	return Vector::ZEROS;
#else
	ASSERT(index==0);
	return (m_characterController && m_characterController->IsReady()) ? m_characterController->GetPosition() : Vector::ZEROS;
#endif
}

EulerAngles CMRPhysicalCharacter::GetRepresentationOrientation() const
{
	if( m_characterController )
	{
		return m_actualRotation;
	}
	else
	{
		return m_host.GetWorldRotation();
	}
}

static CEntity* GetTopMostEntity( CNode* node )
{
	if ( node->GetTransformParent() )
	{
		return GetTopMostEntity( node->GetTransformParent()->GetParent() );
	}

	CComponent* comp = node->AsComponent();
	if ( comp )
	{
		return comp->GetEntity();
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////

void CMRPhysicalCharacter::Update1_PreSeparation( Float timeDelta )
{
#ifdef USE_PHYSX
    if ( m_characterController == nullptr )
	{
        return;
	}

	if ( !m_characterController->IsReady() )
	{
		return;
	}

	m_prevPosition = m_characterController->GetPosition();
	m_characterController->Update1_ComputeMovementVector( timeDelta );
#endif
}

//////////////////////////////////////////////////////////////////////////

Bool CMRPhysicalCharacter::Update2_PostSeparation( Float timeDelta, Bool continousMovement )
{
    if ( m_characterController == nullptr )
	{
        return false;
	}

	Bool ret( true );

#ifdef USE_PHYSX
	if( !m_characterController->IsReady() )
	{
		return false;
	}

	{
		PC_SCOPE( Update2_PostSeparation_ApplyMovement );

		if ( m_characterController->ShouldMove() )
		{
			m_characterController->Update2_ApplyMovement( timeDelta );
		}
		else
		{
			ret = false;
		}
	}

	if ( continousMovement )
	{
		const Vector2& currentMovement = m_characterController->GetLastMoveVector().AsVector2();

		// NOTICE: We do compare pure floats in 'currentMovement != m_requestedMovementDelta.AsVector2()'. Its actually hits if character is moving without obstruction, but this optimization code is quite risky and prone to just 'stop working'.
		if ( m_host.IsSnapedToNavigableSpace() && (!currentMovement.IsZero() && currentMovement != m_requestedMovementDelta.AsVector2() ) )
		{
			PC_SCOPE( Update2_PostSeparation_SnapToNav );

			CPathAgent* pathAgent = m_host.GetPathAgent();
			if ( pathAgent && pathAgent->IsOnNavdata() )
			{
				PC_SCOPE( Update2_PostSeparation_SnapToNav_Test );
				Vector newPosition = m_characterController->GetPosition();
				if ( !pathAgent->TestLocation( newPosition ) && pathAgent->TestLocation( Vector( m_prevPosition.X, m_prevPosition.Y, newPosition.Z ) ) )
				{
					PC_SCOPE( Update2_PostSeparation_SnapToNav_ForceSet );

					// cancel movement
					m_characterController->ForceSetPosition( pathAgent->GetRepresentationPosition() );
					ret = true;
				}
			}
		}
	}
#endif

	return ret;
}

void CMRPhysicalCharacter::SetGravity( Bool enable )
{
	if ( m_characterController )
	{
#ifdef USE_PHYSX
		m_characterController->SetGravity( enable );
#endif
	}
}

Bool CMRPhysicalCharacter::IsGravity() const
{
	if ( m_characterController )
	{
#ifdef USE_PHYSX
		return m_characterController->IsGravity();
#endif
	}
	return false;
}

void CMRPhysicalCharacter::SetNeedsBehaviorCallback(Bool enable)
{
	if ( m_characterController )
	{
#ifdef USE_PHYSX
		m_characterController->SetNeedsBehaviorCallback( enable );
#endif
	}
}

Bool CMRPhysicalCharacter::IsBehaviorCallbackNeeded() const
{
	if ( m_characterController )
	{
#ifdef USE_PHYSX
		m_characterController->IsBehaviorCallbackNeeded();
#endif
	}
	return true;
}

void CMRPhysicalCharacter::SetRagdollToSwimming( Bool enable )
{
	if ( m_characterController )
	{
#ifdef USE_PHYSX
		m_characterController->SetRagdollToSwimming( enable );
#endif
	}
}

void CMRPhysicalCharacter::SetSwimming( Bool enable )
{
	ASSERT( m_characterController );
	if ( m_characterController )
	{
#ifdef USE_PHYSX
		m_characterController->SetSwimming( enable );
#endif
	}
}

void CMRPhysicalCharacter::SetDiving( Bool diving )
{
	ASSERT( m_characterController );
	if ( m_characterController )
	{
#ifdef USE_PHYSX
		return m_characterController->SetDiving( diving );
#endif
	}
}

Bool CMRPhysicalCharacter::IsDiving() const
{
	if ( m_characterController )
	{
#ifdef USE_PHYSX
		return m_characterController->IsDiving();
#endif
	}
	return false;
}

Float CMRPhysicalCharacter::GetWaterLevel()
{
	if ( m_characterController )
	{
#ifdef USE_PHYSX
		return m_characterController->GetWaterLevel();
#endif
	}
	return -10000.0f;
}

void CMRPhysicalCharacter::SetEmergeSpeed( Float value )
{
	if ( m_characterController )
	{
#ifdef USE_PHYSX
		m_characterController->SetEmergeSpeed( value );
#endif
	}
}

Float CMRPhysicalCharacter::GetEmergeSpeed() const
{
	if ( m_characterController )
	{
#ifdef USE_PHYSX
		return m_characterController->GetEmergeSpeed();
#endif
	}
	return GGame->GetGameplayConfig().m_emergeSpeed;
}

void CMRPhysicalCharacter::SetSubmergeSpeed( Float value )
{
	if ( m_characterController )
	{
#ifdef USE_PHYSX
		m_characterController->SetSubmergeSpeed( value );
#endif
	}
}

Float CMRPhysicalCharacter::GetSubmergeSpeed() const
{
	if ( m_characterController )
	{
#ifdef USE_PHYSX
		return m_characterController->GetSubmergeSpeed();
#endif
	}
	return GGame->GetGameplayConfig().m_submergeSpeed;
}

Bool CMRPhysicalCharacter::IsFalling() const
{
	if ( m_characterController )
	{
#ifdef USE_PHYSX
		return m_characterController->IsFalling();
#endif
	}
	return false;
}

Bool CMRPhysicalCharacter::IsSliding() const
{
	if ( m_characterController )
	{
#ifdef USE_PHYSX
		return m_characterController->IsSliding();
#endif
	}
	return false;
}

Float CMRPhysicalCharacter::GetSlideCoef() const
{
	if ( m_characterController )
	{
#ifdef USE_PHYSX
		return m_characterController->GetSlideCoef();
#endif
	}
	return 0.0f;
}

Vector CMRPhysicalCharacter::GetSlidingDir() const
{
	Vector	direction	=	Vector::ZEROS;

	if ( m_characterController )
	{
#ifdef USE_PHYSX
		direction	= m_characterController->GetSlidingDir();
#endif
	}

	return direction;
}

void CMRPhysicalCharacter::ForceMoveToPosition( const Vector& position, Bool resetZAxis )
{
#ifdef USE_PHYSX
    if ( m_characterController && m_characterController->IsReady() )
    {
        m_characterController->ForceMoveToPosition( position, resetZAxis );
    }
#endif
}

void CMRPhysicalCharacter::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
#ifdef USE_PHYSX
	if( m_characterController && m_characterController->IsReady() )
	{
		m_characterController->OnGenerateEditorFragments( frame, flags );
	}
#endif
}

Bool CMRPhysicalCharacter::GetCollisionControllerExtents( AACylinder& outCylinder, const Vector& worldPosition ) const
{
#ifdef USE_PHYSX
	if ( m_characterController )
	{
		outCylinder.m_positionAndRadius = worldPosition;
		outCylinder.m_positionAndRadius.W = m_characterController->GetVirtualRadius();
		outCylinder.m_height = m_characterController->GetCurrentHeight();
		return true;
	}
#endif
	return false;
}
Bool CMRPhysicalCharacter::GetCollisionControllerExtents( AACylinder& outCylinder ) const
{
#ifdef USE_PHYSX
	if ( m_characterController )
	{
		outCylinder.m_positionAndRadius = m_characterController->GetPosition();
		outCylinder.m_positionAndRadius.W = m_characterController->GetVirtualRadius();
		outCylinder.m_height = m_characterController->GetCurrentHeight();
		return true;
	}
#endif
	return false;
}

void CMRPhysicalCharacter::ApplyVelocity( const Vector& vel )
{
#ifdef USE_PHYSX
	if ( m_characterController )
		m_characterController->ApplyVelocity( vel );
#endif
}

void CMRPhysicalCharacter::RegisterEventListener( const THandle<IScriptable>& listener )
{
#ifdef USE_PHYSX
	if( m_characterController )
		m_characterController->RegisterEventListener( listener );
#endif
}

void CMRPhysicalCharacter::UnregisterEventListener( const THandle<IScriptable>& listener )
{
#ifdef USE_PHYSX
	if( m_characterController )
		m_characterController->UnregisterEventListener( listener );
#endif
}

Float CMRPhysicalCharacter::GetCombatRadius() const 
{
#ifdef USE_PHYSX
	if( m_characterController )
	{
		return m_characterController->GetVirtualRadius();
	}
#endif
	return 0.5f;
}

Float CMRPhysicalCharacter::GetCurrentHeight() const 
{
#ifdef USE_PHYSX
	if( m_characterController )
	{
		return m_characterController->GetCurrentHeight();
	}
#endif
	return 0.5f;
}

Bool CMRPhysicalCharacter::DownCollision() const
{
#ifdef USE_PHYSX
	RED_ASSERT( m_characterController, TXT( "characterController must exist to perform collision checks." ) );
	if ( m_characterController && m_characterController->IsReady() )
	return m_characterController->DownCollision();
#endif

	return true;
}

const CCharacterControllerParam* CMRPhysicalCharacter::GetCharacterControllerParam()
{
	CObject* et = m_host.GetEntity()->GetTemplate();
	CEntityTemplate *templ = et ? Cast< CEntityTemplate > ( et ) : NULL;
	const CCharacterControllerParam* params = nullptr;

	if ( templ )
	{
		params = templ->FindGameplayParamT< CCharacterControllerParam > ( true );
	}

	if ( params == NULL )
	{
		params = CCharacterControllerParam::GetStaticClass()->GetDefaultObject< CCharacterControllerParam > ();
	}

	return params;

}

