#include "build.h"
#include "movementTargeter.h"
#include "movementGoal.h"
#include "moveGlobalPathPlanner.h"
#include "../engine/renderFrame.h"

///////////////////////////////////////////////////////////////////////////////

void IMovementTargeter::Release()
{
	// default implementation for classes that are managed externally
}

Bool IMovementTargeter::IsFinished() const
{
	return true;
}

void IMovementTargeter::OnSerialize( IFile& file )
{
	// default implementation for classes that are managed externally
}

void IMovementTargeter::GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame )
{

}
void IMovementTargeter::GenerateDebugPage( TDynArray< String >& debugLines ) const
{

}

Vector IMovementTargeter::Seek( const CMovingAgentComponent& host, const Vector& pos ) const
{
	Vector direction = ( pos - host.GetAgentPosition() ).Normalized2();

	return direction;
}

Vector IMovementTargeter::Flee( const CMovingAgentComponent& host, const Vector& pos ) const
{
	Vector direction = ( host.GetAgentPosition() - pos ).Normalized2();

	return direction;
}

Vector IMovementTargeter::Pursue( const CMovingAgentComponent& host, const CMovingAgentComponent& pursuedAgent ) const
{
	Vector predictedTargetPos = pursuedAgent.GetAgentPosition() + pursuedAgent.GetVelocity();
	
	Vector velocity = Seek( host, predictedTargetPos );
	return velocity;
}

Float IMovementTargeter::FaceTarget( const CMovingAgentComponent& host, const Vector& pos ) const
{
	Vector	dirToTarget				= ( pos - host.GetAgentPosition() ).Normalized2();
	Float	yawToTarget				= dirToTarget.ToEulerAngles().Yaw;

	return yawToTarget;
}

///////////////////////////////////////////////////////////////////////////////

RED_DEFINE_STATIC_NAME( UpdateChannels );

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CMoveTRGScript );

CMoveTRGScript::CMoveTRGScript()
	: m_agent( NULL )
{
}

CMoveTRGScript::~CMoveTRGScript()
{
}

void CMoveTRGScript::Release()
{
	Discard();
}

void CMoveTRGScript::OnSerialize( IFile& file )
{
	CObject* obj = Cast< CObject >( this );
	ASSERT( obj );

	file << obj;
}

void CMoveTRGScript::UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta )
{
	ASSERT( m_agent.Get() == NULL );
	m_agent = &const_cast< CMovingAgentComponent& >( agent );
	m_timeDelta = timeDelta;

	CallFunctionRef( this, CNAME( UpdateChannels ), goal );

	m_agent = NULL;
}

// ------------------------------------------------------------------------
// Scripting support
// ------------------------------------------------------------------------

void CMoveTRGScript::funcSetHeadingGoal( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SMoveLocomotionGoal, goal, SMoveLocomotionGoal() );
	GET_PARAMETER( Vector, heading, Vector( 0.0f, 0.0f, 0.0f ) );
	FINISH_PARAMETERS;

	CMovingAgentComponent* agent = m_agent.Get();
	if ( agent )
	{
		goal.SetHeadingGoal( *agent, heading.AsVector2() );
	}
}

void CMoveTRGScript::funcSetOrientationGoal( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SMoveLocomotionGoal, goal, SMoveLocomotionGoal() );
	GET_PARAMETER( Float, orientation, 0.0f );
	GET_PARAMETER_OPT( Bool, alwaysSet, false );
	FINISH_PARAMETERS;

	CMovingAgentComponent* agent = m_agent.Get();
	if ( agent )
	{
		goal.SetOrientationGoal( *agent, orientation, alwaysSet );
	}
}

void CMoveTRGScript::funcSetSpeedGoal( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SMoveLocomotionGoal, goal, SMoveLocomotionGoal() );
	GET_PARAMETER( Float, speed, 0.0f );
	FINISH_PARAMETERS;

	goal.SetSpeedGoal( speed );
}

void CMoveTRGScript::funcSetMaxWaitTime( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SMoveLocomotionGoal, goal, SMoveLocomotionGoal() );
	GET_PARAMETER( Float, time, 0.0f );
	FINISH_PARAMETERS;

	goal.SetMaxWaitTime( time );
}

void CMoveTRGScript::funcMatchDirectionWithOrientation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SMoveLocomotionGoal, goal, SMoveLocomotionGoal() );
	GET_PARAMETER( Bool, enable, false );
	FINISH_PARAMETERS;

	goal.MatchMoveDirectionWithOrientation( enable );
}

void CMoveTRGScript::funcSetFulfilled( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SMoveLocomotionGoal, goal, SMoveLocomotionGoal() );
	GET_PARAMETER( Bool, isFulfilled, true );
	FINISH_PARAMETERS;

	goal.SetFulfilled( isFulfilled );
}


// ------------------------------------------------------------------------
// Steering API
// ------------------------------------------------------------------------

void CMoveTRGScript::funcSeek( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, pos, Vector::ZERO_3D_POINT );
	FINISH_PARAMETERS;

	CMovingAgentComponent* pAgent = m_agent.Get();
	if ( !pAgent )
	{
		RETURN_STRUCT( Vector, Vector::ZERO_3D_POINT );
	}
	else
	{
		Vector velocity = Seek( *pAgent, pos );
		RETURN_STRUCT( Vector, velocity );
	}
}

void CMoveTRGScript::funcFlee( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, pos, Vector::ZERO_3D_POINT );
	FINISH_PARAMETERS;

	CMovingAgentComponent* pAgent = m_agent.Get();
	if ( !pAgent )
	{
		RETURN_STRUCT( Vector, Vector::ZERO_3D_POINT );
	}
	else
	{
		Vector velocity = Flee( *pAgent, pos );
		RETURN_STRUCT( Vector, velocity );
	}
}

void CMoveTRGScript::funcPursue( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CMovingAgentComponent >, pursuedAgent, NULL );
	FINISH_PARAMETERS;

	CMovingAgentComponent* pAgent = m_agent.Get();
	CMovingAgentComponent* pPursuedAgent = pursuedAgent.Get();
	if ( !pAgent || !pPursuedAgent )
	{
		RETURN_STRUCT( Vector, Vector::ZERO_3D_POINT );
	}
	else
	{
		Vector velocity = Pursue( *pAgent, *pPursuedAgent );
		RETURN_STRUCT( Vector, velocity );
	}
}

void CMoveTRGScript::funcFaceTarget( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, pos, Vector::ZERO_3D_POINT );
	FINISH_PARAMETERS;

	Float orientation = 0.0f;
	CMovingAgentComponent* pAgent = m_agent.Get();
	if ( pAgent )
	{
		orientation = FaceTarget( *pAgent, pos );
	}

	RETURN_FLOAT( orientation );
}

///////////////////////////////////////////////////////////////////////////////

CStaticMovementTargeter::CStaticMovementTargeter( const CMovingAgentComponent& agent )
	: m_agent( agent ) 
	, m_rotationTargetType( SMT_None )
	, m_hasOrientation( false )
	, m_clamping( false )
{
}

CStaticMovementTargeter::~CStaticMovementTargeter()
{
	m_rotationTargetType = SMT_None;
	m_hasOrientation = false;
}

void CStaticMovementTargeter::SetRotation( const THandle< CNode >& target, Bool clamping )
{
	m_hasOrientation = true;
	m_dynamicRotationTarget = target;
	m_rotationTargetType = SMT_Dynamic;
	m_clamping = clamping;
}

void CStaticMovementTargeter::SetRotation( const Vector& target, Bool clamping )
{
	m_hasOrientation = true;
	m_staticRotationTarget = target;
	m_rotationTargetType = SMT_Static;
	m_clamping = clamping;
}

void CStaticMovementTargeter::ClearRotation()
{
	m_hasOrientation = false;
	m_rotationTargetType = SMT_None;
}

void CStaticMovementTargeter::Update()
{
	m_hasOrientation = CalcTarget( m_orientationTrgPos );
}

void CStaticMovementTargeter::UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta )
{
	ASSERT( &agent == &m_agent );

	if ( m_hasOrientation )
	{
		Vector agentPos = agent.GetWorldPosition();

		goal.SetOrientationGoal( agent, ( m_orientationTrgPos - agentPos ).ToEulerAngles().Yaw, true, m_clamping );
	}
}

Bool CStaticMovementTargeter::CalcTarget( Vector& pos ) const
{
	switch( m_rotationTargetType )
	{
	case SMT_Dynamic:
		{
			CNode* target = m_dynamicRotationTarget.Get();
			
			if ( target != NULL )
			{
				pos = target->GetWorldPosition();
				return true;
			}
			else
			{
				return false;
			}
		}

	case SMT_Static:
		{
			pos = m_staticRotationTarget;
			return true;
		}

	case SMT_None:
	default:
		{
			return false;
		}
	}
}

void CStaticMovementTargeter::GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame )
{
	if ( m_hasOrientation )
	{
		Vector agentPos = agent.GetWorldPosition();

		frame->AddDebugText( agentPos + Vector( 0,0,1.8f ), TXT("Rotation target enabled"), true, Color::RED );

		frame->AddDebugLine( agentPos, m_orientationTrgPos, Color::RED );
		frame->AddDebugSphere( m_orientationTrgPos, 0.3f, Matrix::IDENTITY, Color::RED );
	}
}

///////////////////////////////////////////////////////////////////////////////

void CStaticMovementTargeter::GenerateDebugPage( TDynArray< String >& debugLines ) const
{
	debugLines.PushBack( TXT( "CStaticMovementTargeter" ) );
}