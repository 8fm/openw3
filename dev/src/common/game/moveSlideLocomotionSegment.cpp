#include "build.h"
#include "moveSlideLocomotionSegment.h"
#include "../engine/renderFrame.h"


IMPLEMENT_RTTI_ENUM( ESlideRotation );

CMoveLSSlide::CMoveLSSlide( const Vector& target, Float duration, const Float* heading, ESlideRotation rotation )
: m_target( target )
, m_duration( duration )
, m_heading( heading ? *heading : CEntity::HEADING_ANY )
, m_rotation( rotation )
, m_disableCollisions( false )
{

}

Bool CMoveLSSlide::Activate( CMovingAgentComponent& agent )
{
	if ( m_duration > 0.f )
	{
		// Set controller data
		//m_shiftPos = m_target - agent.GetAgentPosition();
		m_shiftPos = m_target - agent.GetWorldPosition();
		if ( m_heading == CEntity::HEADING_ANY )
		{
			m_shiftRot = 0.0f; 
		}
		else
		{
			Float angleDist = EulerAngles::AngleDistance( agent.GetWorldYaw(), m_heading );

			if( m_rotation == SR_Nearest )
			{
				m_shiftRot = angleDist;
			}
			else if( m_rotation == SR_Left )
			{
				m_shiftRot = EulerAngles::NormalizeAngle( angleDist );
			}
			else
			{
				if( angleDist > 0.0f )
				{
					angleDist -= 360.0f;
				}

				m_shiftRot = angleDist;			
			}
		}

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

void CMoveLSSlide::Deactivate( CMovingAgentComponent& agent )
{
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

ELSStatus CMoveLSSlide::Tick( Float timeDelta, CMovingAgentComponent& agent )
{
	if( m_finished )
	{
		return LS_Completed;
	}

	if ( m_duration > 0 )
	{
		Float prevProgress = Clamp( m_timer / m_duration, 0.f, 1.f );
		m_timer += timeDelta;
		Float currProgress = Clamp( m_timer / m_duration, 0.f, 1.f );

		Vector slide = m_shiftPos * ( currProgress - prevProgress );
		Float rot = m_shiftRot * ( currProgress - prevProgress );

		// PAKSAS TODO: do przepisania na koszerny slide
		agent.AddDeltaMovement( slide, EulerAngles(0.f, 0.f, rot) );		

		if ( currProgress == 1.f )
		{	
			m_finished = true;
		}
		
		return LS_InProgress;		
	}

	return LS_Completed;
}

void CMoveLSSlide::GenerateDebugFragments( CMovingAgentComponent& agent, CRenderFrame* frame )
{
	frame->AddDebugLine( agent.GetAgentPosition(), m_target, Color::LIGHT_GREEN, true );
	frame->AddDebugSphere( m_target, 0.5f, Matrix::IDENTITY, Color::LIGHT_GREEN );
}

void CMoveLSSlide::GenerateDebugPage( TDynArray< String >& debugLines ) const
{
	debugLines.PushBack( TXT( "CMoveLSSlide") );
	debugLines.PushBack( String::Printf( TXT( "  target: ( %.2f, %.2f, %.2f )" ),
		m_target.X, m_target.Y, m_target.Z ) );
}
