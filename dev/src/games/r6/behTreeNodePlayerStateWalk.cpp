/*
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "behTreeNodePlayerStateWalk.h"
#include "r6behTreeInstance.h"
#include "..\..\common\game\movingPhysicalAgentComponent.h"


//////////////////////////////////////////////////////////////////////////
////////////////////////       DEFINITION       //////////////////////////
//////////////////////////////////////////////////////////////////////////
CBehTreeNodePlayerStateWalkDefinition::CBehTreeNodePlayerStateWalkDefinition()
	: m_maxSpeed( 4.5f )
	, m_accel( 75.f )
	, m_decel( 45.f )
	, m_brake( 150.f )
	, m_dotToBrake( 0.4f )
{
}

IBehTreeNodeInstance* CBehTreeNodePlayerStateWalkDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent /*= nullptr */ ) const 
{
	return new Instance( *this, owner, context, parent );
}

//////////////////////////////////////////////////////////////////////////
////////////////////////        INSTANCE        //////////////////////////
//////////////////////////////////////////////////////////////////////////
CBehTreeNodePlayerStateWalkInstance::CBehTreeNodePlayerStateWalkInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: IBehTreeNodePlayerStateInstance( def, owner, context, parent )
	, m_maxSpeed ( def.m_maxSpeed.GetVal( context ) )
	, m_accel( def.m_accel.GetVal( context ) )
	, m_decel( def.m_decel.GetVal( context ) )
	, m_brake( def.m_brake.GetVal( context ) )
	, m_dotToBrake( def.m_dotToBrake.GetVal( context ) )
{
	CName tmpName = def.m_uniqueAnimation.GetVal( context );
	if( tmpName != CName::NONE )
	{
		m_stateName = tmpName;
	}
}

CBehTreeNodePlayerStateWalkInstance::~CBehTreeNodePlayerStateWalkInstance()
{
}

Bool CBehTreeNodePlayerStateWalkInstance::Activate()
{
	return Super::Activate();
}

void CBehTreeNodePlayerStateWalkInstance::Update()
{
	//// no locomotion segment == not moving
	//if ( nullptr == m_movementSegment )
	//{
	//	Complete( BTTO_FAILED );
	//	return;
	//}

	// get the working moving agent component
	CMovingPhysicalAgentComponent* mac = m_component.Get();
	if ( nullptr == mac || false == mac->IsMotionEnabled() || nullptr == mac->GetBehaviorStack() )
	{
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
		InputHelpers::VecReduce3( newVelocity, m_decel * timeDelta );
	}
	else
	{
		const Float dot = Vector::Dot3( movementData.m_velocity.Normalized3(), wsInput );

		Vector accelVec = wsInput * ( ( dot <= m_dotToBrake ) ? m_accel : m_brake );
		const Float maxSpeed = m_maxSpeed * rawInputMag;
		InputHelpers::VecAddLimited3( newVelocity, accelVec * timeDelta, maxSpeed ); 
	}

	// remember calculated values in shared structure
	movementData.m_velocity = newVelocity;
	movementData.m_requestedFacingDirection = EulerAngles::NormalizeAngle( camDirector->GetCameraForward().AsVector2().Yaw() );
	movementData.m_requestedMovementDirection = EulerAngles::NormalizeAngle( wsInput.AsVector2().Yaw() );

	// pass the values to behavior graph
	movementData.SetBehaviorVariables( mac->GetBehaviorStack() );

	// pass new velocity to direct locomotion segment
	newVelocity*= timeDelta;
	GetPLC()->AddTranslation( newVelocity );

	// complete the block if movement is finished
	if ( movementData.m_velocity.SquareMag3() <= FLT_EPSILON )
	{
		// movement finished
		Complete( BTTO_SUCCESS );
	}
}

void CBehTreeNodePlayerStateWalkInstance::Deactivate()
{
	Super::Deactivate();
}

Bool CBehTreeNodePlayerStateWalkInstance::IsAvailable()
{
	CInputManager* inputManager = GGame->GetInputManager();
	if ( nullptr == inputManager )
	{
		// can't walk without input
		DebugNotifyAvailableFail();
		return false;
	}

	//if ( nullptr == m_movementSegment )
	//{
	//	m_movementSegment = m_movementDataPtr->m_playerLocomotionController->m_movementSegment;
	//	R6_ASSERT( m_movementSegment );
	//	// no movement segment, can't move
	//	return false;
	//}

	CMovingAgentComponent* mac = m_component.Get();
	if ( nullptr == mac || false == mac->IsMotionEnabled() )
	{
		DebugNotifyAvailableFail();
		return false;
	}

	const Vector2 rawInput( inputManager->GetActionValue( CNAME( GI_AxisLeftX ) ), inputManager->GetActionValue( CNAME( GI_AxisLeftY ) ) );
	if ( rawInput.Mag() <= InputHelpers::INPUT_EPSILON )
	{
		// zero input, no walking

		SPlayerMovementData& movementData = GetMovementData();
		if ( movementData.m_velocity.Mag3() <= InputHelpers::INPUT_EPSILON )
		{
			// allready stopped	
			DebugNotifyAvailableFail();
			return false;
		}
	}

	return true;
}

Bool CBehTreeNodePlayerStateWalkInstance::Interrupt()
{
	return Super::Interrupt();
}
