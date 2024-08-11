/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "movementCommandBuffer.h"

#include "../engine/behaviorGraphStack.h"

IMPLEMENT_RTTI_BITFIELD( EBehaviorVarContext );


IMovementCommandBuffer::IMovementCommandBuffer()
	: m_movingAgent( NULL )
	, m_goalLocked( false )
	, m_varToClearNum( 0 )
{}

IMovementCommandBuffer::~IMovementCommandBuffer()
{
	ClearVarsToClear();
}


void IMovementCommandBuffer::ResetSteering()
{
	m_heading.Set( 0.f, 0.f );
	m_speed = 0.f;
	m_rotation = 0.f;

	m_headingImportance = 0.f;
	m_speedImportance = 0.f;
	m_rotationImportance = 0.f;

	m_headingIsLocked = false;
	m_speedIsLocked = false;
	m_rotationIsLocked = false;
	m_destinationIsReached = false;

	//m_movingAgent->SetMoveSpeedRel( 0 );
	//m_movingAgent->SetMoveRotation( 0 );
	m_movingAgent->ResetMoveRequests();
}

void IMovementCommandBuffer::OnTick()
{
	ClearVarsToClear();
}

void IMovementCommandBuffer::OnDeactivate( CMovingAgentComponent* agent )
{
	ClearVarsToClear( agent );
}

void IMovementCommandBuffer::ClearVarsToClear( CMovingAgentComponent* agent )
{
	if ( agent == nullptr)
	{
		agent = m_movingAgent;
	}
	// clear when resetting
	ASSERT(agent != nullptr || m_varToClearNum == 0, TXT("Maybe it should be called sooner, before agent is removed?"));
	if ( agent )
	{
		if ( CBehaviorGraphStack* behStack = agent->GetBehaviorStack() )
		{
			for( Uint32 index = 0; index != m_varToClearNum; ++index )
			{
				const CName & name = m_varToClear[ index ];
				behStack->SetBehaviorVariable( name, 0.0f );
			}
		}
	}
	m_varToClearNum = 0;
}


void IMovementCommandBuffer::LockHeading( const Vector2& velocity )
{
	ASSERT( !Red::Math::NumericalUtils::IsNan( velocity.X ) && !Red::Math::NumericalUtils::IsNan( velocity.Y ) );

	m_heading = velocity.AsVector2();

	m_headingIsLocked = true;
}

void IMovementCommandBuffer::LockSpeed( Float speed )
{
	m_speed = speed;

	m_speedIsLocked = true;
}


void IMovementCommandBuffer::LockRotation( Float rotation, Bool clamped )
{
	ASSERT( !Red::Math::NumericalUtils::IsNan( rotation ) );

	// store the value for debugging purposes
	m_rotation = rotation;

	m_rotationIsLocked = true;
}

void IMovementCommandBuffer::AddHeading( const Vector2& velocity, Float importance )
{
	if ( m_headingIsLocked )
	{
		return;
	}
	ASSERT( importance > 0.f && !Red::Math::NumericalUtils::IsNan( velocity.X ) && !Red::Math::NumericalUtils::IsNan( velocity.Y ) );

	// we can change movement direction and speed now
	Float newImportance = m_headingImportance + importance;
	m_heading *= m_headingImportance / newImportance;
	m_heading += velocity * (importance / newImportance);
	m_headingImportance = newImportance;
}
void IMovementCommandBuffer::AddSpeed( Float speed, Float importance )
{
	if ( m_speedIsLocked )
	{
		return;
	}
	ASSERT( importance > 0.f );
	Float newImportance = m_speedImportance + importance;
	m_speed *= m_speedImportance / newImportance;
	m_speed += speed * (importance / newImportance);
	m_speedImportance = newImportance;
}
void IMovementCommandBuffer::AddRotation( Float rotation, Float importance )
{
	if ( m_rotationIsLocked )
	{
		return;
	}
	ASSERT( importance > 0.f );
	Float newImportance = m_rotationImportance + importance;
	m_rotation *= m_rotationImportance / newImportance;
	m_rotation += rotation * (importance / newImportance);
	m_rotationImportance = newImportance;
}

void IMovementCommandBuffer::OverrideHeading( const Vector2& velocity, Float importance )
{
	ASSERT( !Red::Math::NumericalUtils::IsNan( velocity.X ) && !Red::Math::NumericalUtils::IsNan( velocity.Y ) );

	m_heading = velocity.AsVector2();
	m_headingImportance = importance;
}

void IMovementCommandBuffer::OverrideSpeed( Float speed, Float importance )
{
	m_speed = speed;
	m_speedImportance = importance;
}

void IMovementCommandBuffer::OverrideRotation( Float rotation, Float importance )
{
	ASSERT( !Red::Math::NumericalUtils::IsNan( rotation ) );

	// store the value for debugging purposes
	m_rotation = rotation;
	m_rotationImportance = importance;
}

void IMovementCommandBuffer::ResetBehavior()
{
	m_movingAgent->ForceSetRelativeMoveSpeed( 0 );
	m_movingAgent->SetMoveRotation( 0 );
}

void IMovementCommandBuffer::SetVar( EBehaviorVarContext context, const CName& varName, Float val )
{
	if ( varName.Empty() )
	{
		return;
	}
	ASSERT( m_movingAgent, TXT( "Method called outside a locomotion frame" ) );

	// preprocess the values
	switch( context )
	{
	case BVC_Speed:
		{
			val = Clamp< Float >( val, 0.0f, m_movingAgent->GetMaxSpeed() );
			break;
		}

	case BVC_RotationAngle:
		{
			val = Clamp< Float >( val, -180.0f, 180.0f );
			val = -val / 180.0f;		// convert to a "behavior-angle"
			break;
		}
	}

	// set the variable
	CBehaviorGraphStack* stack = m_movingAgent->GetBehaviorStack();
	ASSERT( stack );

	// OPTIMIZATION NOTICE: We could access directly var id's
	stack->SetBehaviorVariable( varName, val );
}

void IMovementCommandBuffer::SetVectorVar( const CName& varName, const Vector& val )
{
	if ( varName.Empty() )
	{
		return;
	}
	ASSERT( m_movingAgent, TXT( "Method called outside a locomotion frame" ) );

	// set the variable
	CBehaviorGraphStack* stack = m_movingAgent->GetBehaviorStack();
	ASSERT( stack );

	// OPTIMIZATION NOTICE: We could access directly var id's
	stack->SetBehaviorVariable( varName, val );
}

void IMovementCommandBuffer::ClearVarOnNextTick( const CName& varName )
{
	ASSERT(m_varToClearNum < MOVEMENT_COMMAND_BUFFER_CLEAR_VAR_MAX_NUM, TXT("Increase number of variables stored to be cleared"));
	if ( m_varToClearNum < MOVEMENT_COMMAND_BUFFER_CLEAR_VAR_MAX_NUM )
	{
		m_varToClear[m_varToClearNum] = varName;
		++ m_varToClearNum;
	}
}

Bool IMovementCommandBuffer::RaiseEvent( const CName& eventName, const CName& endNotificationName, Float endNotificationTimeOut )
{
	if ( eventName.Empty() )
	{
		return false;
	}
	ASSERT( m_movingAgent, TXT( "Method called outside a locomotion frame" ) );

	CBehaviorGraphStack* stack = m_movingAgent->GetBehaviorStack();
	ASSERT( stack );
	if ( stack->GenerateBehaviorEvent( eventName ) )
	{
		// we can wait for two events that end with the same notification (or can we?)
		m_goal.AddNotification( endNotificationName, endNotificationTimeOut );
		return true;
	}
	else
	{
		return false;
	}
}

Bool IMovementCommandBuffer::RaiseForceEvent( const CName& eventName, const CName& endNotificationName, Float endNotificationTimeOut )
{
	if ( eventName.Empty() )
	{
		return false;
	}
	ASSERT( m_movingAgent, TXT( "Method called outside a locomotion frame" ) );

	CBehaviorGraphStack* stack = m_movingAgent->GetBehaviorStack();
	ASSERT( stack );
	if ( stack->GenerateBehaviorForceEvent( eventName ) )
	{
		// we can wait for two events that end with the same notification (or can we?)
		m_goal.AddNotification( endNotificationName, endNotificationTimeOut );
		return true;
	}
	else
	{
		return false;
	}
}
