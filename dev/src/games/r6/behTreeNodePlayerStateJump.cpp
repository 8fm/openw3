/*
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "behTreeNodePlayerStateJump.h"
#include "r6behTreeInstance.h"
#include "..\..\common\game\movingPhysicalAgentComponent.h"
#include "..\..\common\game\movableRepresentationPhysicalCharacter.h"
#include "../../common/core/dataError.h"
#include "../../common/engine/utils.h"


//////////////////////////////////////////////////////////////////////////
////////////////////////       DEFINITION       //////////////////////////
//////////////////////////////////////////////////////////////////////////
CBehTreeNodePlayerStateJumpDefinition::CBehTreeNodePlayerStateJumpDefinition()
	: m_maxSpeed( 4.5f )
	, m_accel( 75.f )
	, m_decel( 45.f )
	, m_brake( 150.f )
	, m_dotToBrake( 0.4f )

	, m_gravityUp( -18.0f )
	, m_gravityDown( -15.0f )
	, m_jumpSpeedImpulse( 10.0f )
	, m_maxVerticalSpeedAbs( 20.0f )
{
}

IBehTreeNodeInstance* CBehTreeNodePlayerStateJumpDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent /*= nullptr */ ) const 
{
	return new Instance( *this, owner, context, parent );
}

//////////////////////////////////////////////////////////////////////////
////////////////////////        INSTANCE        //////////////////////////
//////////////////////////////////////////////////////////////////////////
CBehTreeNodePlayerStateJumpInstance::CBehTreeNodePlayerStateJumpInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: IBehTreeNodePlayerStateInstance( def, owner, context, parent )
	, m_maxSpeed ( def.m_maxSpeed.GetVal( context ) )
	, m_accel( def.m_accel.GetVal( context ) )
	, m_decel( def.m_decel.GetVal( context ) )
	, m_brake( def.m_brake.GetVal( context ) )
	, m_dotToBrake( def.m_dotToBrake.GetVal( context ) )

	, m_gravityUp( def.m_gravityUp.GetVal( context ) )
	, m_gravityDown( def.m_gravityDown.GetVal( context ) )
	, m_jumpSpeedImpulse( def.m_jumpSpeedImpulse.GetVal( context ) )
	, m_maxVerticalSpeedAbs( def.m_maxVerticalSpeedAbs.GetVal( context ) )
	, m_stillJumping( false )
{
}

CBehTreeNodePlayerStateJumpInstance::~CBehTreeNodePlayerStateJumpInstance()
{
}

Bool CBehTreeNodePlayerStateJumpInstance::Activate()
{
	SPlayerMovementData& movementData = GetMovementData();
	movementData.m_velocity.Z = m_jumpSpeedImpulse;

	return Super::Activate();
}

void CBehTreeNodePlayerStateJumpInstance::Update()
{
	// get the working moving agent component
	CMovingPhysicalAgentComponent* mac = m_component.Get();
	if ( nullptr == mac || false == mac->IsMotionEnabled() || nullptr == mac->GetBehaviorStack() )
	{
		const CEntity* entity = nullptr;
		if ( nullptr == mac )
		{
			// If the component doesn't exist, extract the owner (actor)
			entity = Cast< const CEntity > ( GetOwner()->GetActor() );
		}
		else
		{
			// If the component exists, use it for error data
			entity = mac->GetEntity();
		}

		DATA_HALT( DES_Major, CResourceObtainer::GetResource( entity ), TXT( "Behaviour Tree" ), TXT( "CMovingPhysicalAgentComponent is required for Player entity." ) );
		Complete( BTTO_FAILED );
		return;
	}

	// get the input manager
	CInputManager* inputManager = GGame->GetInputManager();
	R6_ASSERT( inputManager );

	// get the camera director
	R6_ASSERT( GGame->GetActiveWorld() );
	CCameraDirector* camDirector = GGame->GetActiveWorld()->GetCameraDirector();

	// gather input values
	const Vector2 rawInput( inputManager->GetActionValue( CNAME( GI_AxisLeftX ) ), inputManager->GetActionValue( CNAME( GI_AxisLeftY ) ) );
	const Vector wsInput = InputHelpers::RawInputToWorldSpace( camDirector, rawInput );
	const Float timeDelta = m_owner->GetLocalTimeDelta();

	// values to be changed
	SPlayerMovementData& movementData = GetMovementData();
	Vector newVelocity = movementData.m_velocity;	

	// accelerate or decelerate
	const Float rawInputMag = min( rawInput.Mag(), 1.f );
	if ( rawInputMag <= InputHelpers::INPUT_EPSILON )
	{
		InputHelpers::VecReduce2( newVelocity, m_decel * timeDelta );
	}
	else
	{
		const Float dot = Vector::Dot2( movementData.m_velocity.Normalized2(), wsInput );

		Vector accelVec = wsInput * ( ( dot <= m_dotToBrake ) ? m_accel : m_brake );
		const Float maxSpeed = m_maxSpeed * rawInputMag;
		InputHelpers::VecAddLimited2( newVelocity, accelVec * timeDelta, maxSpeed ); 
	}



	{ // ported from UpdateMovementVertical in playerExplorationStateDirector.ws
		Float currAcc = ( ( newVelocity.Z > 0.0f ) ? m_gravityUp : m_gravityDown );
		newVelocity.Z += currAcc * timeDelta;
		newVelocity.Z = Clamp( newVelocity.Z, -m_maxVerticalSpeedAbs, m_maxVerticalSpeedAbs );
	}

	// remember calculated values in shared structure
	movementData.m_velocity = newVelocity;
	movementData.m_requestedFacingDirection = EulerAngles::NormalizeAngle( camDirector->GetCameraForward().AsVector2().Yaw() );
	movementData.m_requestedMovementDirection = EulerAngles::NormalizeAngle( wsInput.AsVector2().Yaw() );

	// pass the values to behavior graph
	movementData.SetBehaviorVariables( mac->GetBehaviorStack() );

	// pass new velocity to direct locomotion segment
	newVelocity *= timeDelta;
	GetPLC()->AddTranslation( newVelocity );



	{ // ported from StateChangePrecheck in explorationJump.ws
		R6_ASSERT( mac );
		const CMRPhysicalCharacter* physicalCharacter = mac->GetPhysicalCharacter();
		R6_ASSERT( physicalCharacter );

		if ( physicalCharacter->DownCollision() )
		{
			mac->GetBehaviorStack()->GenerateBehaviorEvent( CNAME( JumpGoingToLand ) );
			Complete( BTTO_SUCCESS );
			m_stillJumping = false;
		}
		else
		{
			m_stillJumping = true;
		}
	}
}

void CBehTreeNodePlayerStateJumpInstance::Deactivate()
{
	Super::Deactivate();

	SPlayerMovementData& movementData = GetMovementData();
	movementData.m_velocity.Z = 0.0f;

	m_stillJumping = false;
}

Bool CBehTreeNodePlayerStateJumpInstance::IsAvailable()
{
	R6_ASSERT( m_movementDataPtr );
	R6_ASSERT( m_movementDataPtr->m_playerLocomotionController );

	CInputManager* inputManager = GGame->GetInputManager();
	if ( nullptr == inputManager )
	{
		// can't walk without input
		DebugNotifyAvailableFail();
		return false;
	}

	CMovingAgentComponent* mac = m_component.Get();
	if ( nullptr == mac || false == mac->IsMotionEnabled() )
	{
		DebugNotifyAvailableFail();
		return false;
	}

	if( m_stillJumping || inputManager->GetActionValue( CNAME( GI_Jump ) ) > 0.0f )
	{
		return true;
	}

	DebugNotifyAvailableFail();
	return false;
}

Bool CBehTreeNodePlayerStateJumpInstance::Interrupt()
{
	return Super::Interrupt();
}
