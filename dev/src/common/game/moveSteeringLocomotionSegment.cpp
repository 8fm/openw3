#include "build.h"
#include "moveSteeringLocomotionSegment.h"

#include "../../common/engine/behaviorGraphStack.h"
#include "movementGoal.h"
#include "movementTargeter.h"
#include "moveSteeringBehavior.h"
#include "../engine/renderFrame.h"


///////////////////////////////////////////////////////////////////////////////

RED_DEFINE_STATIC_NAME( HardLock  )

///////////////////////////////////////////////////////////////////////////////

CMoveLSSteering::CMoveLSSteering( Bool switchToEntityRepresentation )
	: m_switchToEntityRepresentation( switchToEntityRepresentation )
	, m_includeStaticTargeters( false )
	, m_forceNoTimeout( false )
{
}

CMoveLSSteering::~CMoveLSSteering()
{
	for ( TDynArray< IMovementTargeter* >::iterator it = m_targeters.Begin(); it != m_targeters.End(); ++it )
	{
		if ( *it )
		{
			(*it)->Release();
		}
	}
	m_targeters.Clear();
}

void CMoveLSSteering::OnSerialize( IFile& file )
{
	for ( TDynArray< IMovementTargeter* >::iterator it = m_targeters.Begin(); it != m_targeters.End(); ++it )
	{
		if ( *it )
		{
			(*it)->OnSerialize( file );
		}
	}
}

CMoveLSSteering& CMoveLSSteering::AddTargeter( IMovementTargeter* targeter )
{
	if ( !targeter )
	{
		return *this;
	}

	ASSERT( !m_targeters.Exist( targeter ) );
	m_targeters.PushBackUnique( targeter );

	return *this;
}
CMoveLSSteering& CMoveLSSteering::RemoveTargeter( IMovementTargeter* targeter )
{
	if ( !targeter )
	{
		return *this;
	}
	m_targeters.Remove( targeter );

	return *this;
}

Bool CMoveLSSteering::Activate( CMovingAgentComponent& agent )
{
	if ( m_switchToEntityRepresentation )
	{
		agent.ForceEntityRepresentation( true, CMovingAgentComponent::LS_Default );
	}

	m_goalLocked = false;
	m_goal.Clear();

	// reset the stop timer
	m_stopTimer = 0.0f;
	m_stopTimerOn = false;



	return true;
}
void CMoveLSSteering::OnSteeringBehaviorChanged( CMovingAgentComponent* owner, CMoveSteeringBehavior* prevSteeringGraph, InstanceBuffer* prevRuntimeData, CMoveSteeringBehavior* newSteeringGraph, InstanceBuffer* newRuntimeData )
{	
	if( prevSteeringGraph )
	{
		prevSteeringGraph->Deactivate( owner, prevRuntimeData );
	}

	if( newSteeringGraph )
	{
		newSteeringGraph->Activate( owner, newRuntimeData );
	}
}

ELSStatus CMoveLSSteering::Tick( Float timeDelta, CMovingAgentComponent& agent )
{
	ELSStatus status			= LS_Completed;

	// begin a frame
	m_movingAgent				= &agent;
	m_timeDelta					= timeDelta;
	status						= LS_InProgress;

	OnTick();

	// a behavior event was used - wait until it clears out
	if ( !m_goal.m_expectedBehNotification.Empty() )
	{
		Int32 count = (Int32)m_goal.m_expectedBehNotification.Size();
		CBehaviorGraphStack* stack = agent.GetBehaviorStack();
		ASSERT( stack );

		for ( Int32 i = count - 1; i >= 0; --i )
		{
			m_goal.m_expectedBehNotification[i].m_timeout -= timeDelta;
			if ( stack->DeactivationNotificationReceived( m_goal.m_expectedBehNotification[ i ].m_name ) || m_goal.m_expectedBehNotification[i].m_timeout < 0 )
			{
				m_goal.m_expectedBehNotification.EraseFast( m_goal.m_expectedBehNotification.Begin() + i );
			}
		}
	}

	// end a frame
	if ( m_goal.m_expectedBehNotification.Empty() )
	{
		status = CalcGoal( timeDelta );

		ResetSteering();

		// process regular movement
		agent.GetCurrentSteeringBehavior()->CalculateMovement( *this, *agent.GetCurrentSteeringRuntimeData(), timeDelta, agent.GetSteeringBehaviorListener() );

		// update the stop timer
		if ( m_stopTimerOn )
		{
			m_stopTimer += timeDelta;
		}
	}
	m_movingAgent = NULL;


	// check the stop timer - if it exceeded a reasonable amount of time, and the segment's not already completed - fail the movement
	if ( !m_forceNoTimeout && status != LS_Completed && m_stopTimer > m_goal.GetMaxWaitTime() )
	{
		status = LS_Failed;
	}

	return status;
}

void CMoveLSSteering::Deactivate( CMovingAgentComponent& agent )
{
	OnDeactivate(&agent);

	agent.ResetMoveRequests();
	//agent.SetMoveRotation( 0 );
	//agent.SetMoveSpeedRel( 0 );

	if ( m_switchToEntityRepresentation )
	{
		agent.ForceEntityRepresentation( false );
	}
}
ELSStatus CMoveLSSteering::CalcGoal( Float timeDelta )
{
	ASSERT( m_movingAgent, TXT( "Method called outside a locomotion frame" ) );

	// calculate the goal
	m_goal.Clear();

	for ( TDynArray< IMovementTargeter* >::const_iterator it = m_targeters.Begin(); it != m_targeters.End(); ++it )
	{
		(*it)->UpdateChannels( *m_movingAgent, m_goal, timeDelta );
	}

	m_movingAgent->UpdateGoal( m_goal, timeDelta );

	if ( !m_goal.IsSet() )
	{
		return LS_Completed;
	}

	{
		// verify that the goal can be reached ( that we haven't blocked anywhere )
		Vector currPos = m_movingAgent->GetAgentPosition();
		Vector heading = m_movingAgent->GetHeading();
		Vector collChecker = currPos + heading * m_movingAgent->GetAbsoluteMoveSpeed() * timeDelta;
		Vector lineDir( currPos );
		Bool obstaclesInWay = m_movingAgent->GetFirstCollision( collChecker, lineDir );
		Bool blockingCollisionOccured = ( obstaclesInWay && m_movingAgent->GetAbsoluteMoveSpeed() < 1e-1 );
		
		EnableStopTimer( blockingCollisionOccured );
	}

	return LS_InProgress;
}

void CMoveLSSteering::EnableStopTimer( Bool enable )
{
	if ( m_stopTimerOn != enable )
	{
		m_stopTimer = 0.0f;
	}
	m_stopTimerOn = enable;
}

void CMoveLSSteering::GenerateDebugFragments( CMovingAgentComponent& agent, CRenderFrame* frame )
{
	// draw the targeters
	for ( TDynArray< IMovementTargeter*	>::iterator it = m_targeters.Begin();
		it != m_targeters.End(); ++it )
	{
		(*it)->GenerateDebugFragments( agent, frame );
	}

	// draw the last calculated steering output
	Vector agentPos = agent.GetAgentPosition();
	EulerAngles agentRot = agent.GetWorldRotation();

	Vector accelDir = agentPos + EulerAngles::YawToVector( agentRot.Yaw ) * m_speed * 2.0f;
	Vector torqueDir = agentPos + EulerAngles::YawToVector( m_rotation + agentRot.Yaw );
	frame->AddDebugLine( agentPos, accelDir, Color::GREEN );
	frame->AddDebugLine( agentPos, torqueDir, Color::RED );

	// add a debug arrow marking the actual heading
	if ( m_goal.IsHeadingGoalSet() )
	{
		frame->AddDebugLineWithArrow( agentPos, agentPos + m_goal.GetHeadingToGoal(), 1.0f, 0.1f, 0.1f, Color::YELLOW, true );
		//frame->AddDebugText( agentPos + m_goal.GetHeadingToGoal(), TXT("h2g"), 0, 0 );
	}
}

void CMoveLSSteering::GenerateDebugPage( TDynArray< String >& debugLines ) const
{
	debugLines.PushBack( TXT( "CMoveLSSteering" ) );
	TDynArray< String > targetersLines;
	for( TDynArray< IMovementTargeter* >::const_iterator targeterIter = m_targeters.Begin();
		targeterIter != m_targeters.End(); ++targeterIter )
	{
		const IMovementTargeter* targeter = *targeterIter;
		targeter->GenerateDebugPage( targetersLines );
	}

	for( TDynArray< String >::const_iterator lineIter = targetersLines.Begin();
		lineIter != targetersLines.End(); ++lineIter )
	{
		debugLines.PushBack( String( TXT( "  " ) ) + *lineIter );
	}
}

Bool CMoveLSSteering::CanBeCanceled() const
{
	return true;
}

CMoveLSSteering *const CMoveLSSteering::AsCMoveLSSteering()
{
	return this;
}