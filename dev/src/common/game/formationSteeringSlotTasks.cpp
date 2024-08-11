/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "formationSteeringSlotTasks.h"

#include "formationMemberDataSlot.h"
#include "movementCommandBuffer.h"



IMPLEMENT_ENGINE_CLASS( CFormationSteerToSlotSteeringTask )
IMPLEMENT_ENGINE_CLASS( CFormationCatchupSlotSteeringTask )

///////////////////////////////////////////////////////////////////////////////
// CFormationSteerToSlotSteeringTask
///////////////////////////////////////////////////////////////////////////////
String CFormationSteerToSlotSteeringTask::GetTaskName() const
{
	return TXT("Steer to formation slot");
}

void CFormationSteerToSlotSteeringTask::CalculateSteeringInput( IMovementCommandBuffer& comm, InstanceBuffer& data, SFormationSteeringInput& formationData, Vector2& outHeading, Float& outHeadingRatio, Float& outOverrideSpeed, Float& outOverrideSpeedImportance ) const
{
	SFormationSteeringInput* input = SFormationSteeringInput::GetGeneralFormationData( comm.GetGoal() );
	if ( !input )
	{
		//RED_HALT( TXT("Steering graph error! Steer to formation slot used out of formation movement!") );
		return;
	}

	CSlotFormationMemberData* slotMember = input->MemberData()->AsSlotFormationMemberData();
	if ( !slotMember )
	{
		//RED_HALT( TXT("Steering graph error! Steer to formation slot used for non-slot formation!") );
		return;
	}

	CAISlotFormationLeaderData* leader = slotMember->CastLeader( input->LeaderData() );
	CFormationSlotInstance* slot = slotMember->GetSlot( leader );

	if ( !slot )
	{
		return;
	}
	CMovingAgentComponent& mac = comm.GetAgent();

	Vector2 headingInput( 0.f, 0.f );
	Float speedInput = 1.f;

	Float headingWeight = 0.f;
	Float speedWeight = 0.f;

	const auto& constraints = slot->GetConstraints();
	for ( auto it = constraints.Begin(), end = constraints.End(); it != end; ++it )
	{
		CFormationSlotInstance* instance = (*it).GetReferencedSlot( *leader );
		if ( !instance )
		{
			continue;
		}
		(*it).ApplyToSteering( mac, *slotMember, *leader, instance, headingInput, speedInput, headingWeight, speedWeight );
	}

	if ( headingWeight )
	{
		outHeading = headingInput;
		outHeadingRatio = Min( headingWeight, 1.f );
	}

	if ( speedWeight )
	{
		outOverrideSpeed = speedInput;
		outOverrideSpeedImportance = m_speedImportance;
	}
}

///////////////////////////////////////////////////////////////////////////////
// CFormationCatchupSlotSteeringTask
///////////////////////////////////////////////////////////////////////////////

String CFormationCatchupSlotSteeringTask::GetTaskName() const
{
	return TXT("Catch up slot");
}

void CFormationCatchupSlotSteeringTask::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	SFormationSteeringInput* input = SFormationSteeringInput::GetGeneralFormationData( comm.GetGoal() );
	if ( !input )
	{
		return;
	}
	CSlotFormationMemberData* slotMember = input->MemberData()->AsSlotFormationMemberData();
	if ( !slotMember )
	{
		return;
	}

	Float distanceToSlot = slotMember->GetDistanceToSlot();
	if ( distanceToSlot <= m_toleranceDistance )
	{
		return;
	}

	// never run if we are ahead of formation
	const Vector& currentPos = comm.GetAgent().GetWorldPositionRef();
	const Vector& desiredPosition = slotMember->GetDesiredPosition();

	Vector2 diff = desiredPosition.AsVector2() - currentPos.AsVector2();
	Vector2 leaderOrientation = EulerAngles::YawToVector2( input->LeaderData()->GetLeaderOrientation() );
	
	if ( diff.Dot( leaderOrientation ) < 0.f )
	{
		return;
	}

	// Possibly todo: get full cosinus (normalize) and scale output accordingly. Currently algorithm output is not smooth.

	Float importance =
		distanceToSlot >= m_maxDistance ?
		m_speedImportance :
		m_speedImportance * ( (distanceToSlot - m_toleranceDistance) / (m_maxDistance - m_toleranceDistance) );

	comm.AddSpeed( m_cachupSpeed, importance );
}