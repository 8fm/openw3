#include "build.h"
#include "moveCurveLocomotionSegment.h"
#include "../engine/renderFrame.h"

CMoveLSCurve::CMoveLSCurve( const Vector& target, Float duration, Bool rightShift, const Float* heading )
	: m_target( target )
	, m_duration( duration )
	, m_heading( heading ? *heading : CEntity::HEADING_ANY )
	, m_disableCollisions( false )
	, m_controlPoint0( Vector::ZEROS )
	, m_controlPoint1( Vector::ZEROS )
	, m_startPosition( Vector::ZEROS )
	, m_initialTargetDistance( 0.f )
	, m_curveProgress( 0.f )
	, m_velocity( 0.f )
	, m_rightShiftCurve( rightShift )
{

}

Bool CMoveLSCurve::Activate( CMovingAgentComponent& agent )
{
	if ( m_duration > 0.f )
	{
		////
		m_startPosition = agent.GetWorldPosition();
		m_curveProgress = 0.0f;			
		m_initialTargetDistance = m_startPosition.DistanceTo( m_target);
		m_velocity = m_initialTargetDistance/m_duration;
		UpdateControlPoints( m_target );
		////

		m_shiftPos = m_target - agent.GetWorldPosition();
		m_shiftRot = 0.0f; 

		m_timer = 0.0f;
		m_finished =  false;

		// reset agent's state
		// Set component data
		agent.ForceSetRelativeMoveSpeed( 0.f );
		agent.SetMoveRotation( 0.f );
		agent.SetMoveRotationSpeed( 0.f );

		if ( m_disableCollisions )
		{
			agent.ForceEntityRepresentation( true );
		}
	}

	return true;
}

void CMoveLSCurve::Deactivate( CMovingAgentComponent& agent )
{
	m_controlPoint0 = Vector::ZEROS;
	m_controlPoint1 = Vector::ZEROS;
	m_startPosition = Vector::ZEROS;
	m_initialTargetDistance = 0.f;
	m_curveProgress = 0.f;
	m_velocity = 0.f;

	// reset agent's state
	// Set component data
	agent.SetMoveHeading( 0 );
	agent.ForceSetRelativeMoveSpeed( 0.f );
	agent.SetMoveRotation( 0.f );
	agent.SetMoveRotationSpeed( 0.f );

	if ( m_disableCollisions )
	{
		// restore the agent's state
		agent.ForceEntityRepresentation( false );
	}
}

ELSStatus CMoveLSCurve::Tick( Float timeDelta, CMovingAgentComponent& agent )
{
	if( m_finished )
	{
		return LS_Completed;
	}

	if ( m_duration > 0 )
	{
		// Approximated 
		Float dt = ( m_velocity*timeDelta ) / m_initialTargetDistance;

		m_curveProgress += dt;

		if( m_curveProgress >= 1.0f )
		{
			m_finished = true;
			m_curveProgress = 1.0f;
			m_velocity = 0.0f;
		}

		// Calculate positions
		Vector oldPos = agent.GetWorldPosition();
		Vector newPos = CalculateBezier( m_curveProgress, m_startPosition, m_controlPoint0, m_controlPoint1, m_target );

		m_timer += timeDelta;

		agent.AddDeltaMovement(newPos-oldPos, EulerAngles(0.f, 0.f, 0.f));

		return LS_InProgress;		
	}

	return LS_Completed;
}

Vector CMoveLSCurve::CalculateBezier( Float t, const Vector& p0, const Vector& p1, const Vector& p2, const Vector& p3 )
{
	const Float m = 1 - t;
	const Float m2 = m * m;
	const Float t2 = t * t;

	return p0*m2*m + p1*3*t*m2 + p2*3*t2*m + p3*t*t2;
}

// Assumes single curve with one, shared control point
void CMoveLSCurve::UpdateControlPoints( const Vector& targetPos )
{
	Vector midpoint = ( targetPos + m_startPosition ) * 0.5f;
	m_controlPoint0 = midpoint - targetPos;
	m_controlPoint0.Normalize3();

	// Rotation
	Float oldX = m_controlPoint0.X;
	m_controlPoint0.X = m_rightShiftCurve ? -m_controlPoint0.Y : m_controlPoint0.Y;
	m_controlPoint0.Y = m_rightShiftCurve ? oldX : -oldX;

	m_controlPoint0 += midpoint;

	m_controlPoint1 = m_controlPoint0;
}

void CMoveLSCurve::GenerateDebugFragments( CMovingAgentComponent& agent, CRenderFrame* frame )
{
	frame->AddDebugLine( agent.GetAgentPosition(), m_target, Color::LIGHT_GREEN, true );
	frame->AddDebugSphere( m_target, 0.5f, Matrix::IDENTITY, Color::LIGHT_GREEN );
}

void CMoveLSCurve::GenerateDebugPage( TDynArray< String >& debugLines ) const
{
	debugLines.PushBack( TXT( "CMoveLSCurve") );
	debugLines.PushBack( String::Printf( TXT( "  target: ( %.2f, %.2f, %.2f )" ),
		m_target.X, m_target.Y, m_target.Z ) );
}
