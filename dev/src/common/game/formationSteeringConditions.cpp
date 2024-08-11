/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "formationSteeringConditions.h"

#include "movementCommandBuffer.h"
#include "movementGoal.h"
#include "formationMemberDataSlot.h"
#include "formationSteeringInput.h"

IMPLEMENT_ENGINE_CLASS( IFormationSteeringCondition )
IMPLEMENT_ENGINE_CLASS( CFormationIsMovingSteeringCondition )
IMPLEMENT_ENGINE_CLASS( CFormationIsBrokenSteeringCondition )

///////////////////////////////////////////////////////////////////////////////
// IFormationSteeringCondition
///////////////////////////////////////////////////////////////////////////////
String IFormationSteeringCondition::GetConditionName() const
{
	return TXT("Formation");
}

///////////////////////////////////////////////////////////////////////////////
// CFormationIsMovingSteeringCondition
///////////////////////////////////////////////////////////////////////////////
Bool CFormationIsMovingSteeringCondition::Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const
{
	SFormationSteeringInput* input = SFormationSteeringInput::GetGeneralFormationData( comm.GetGoal() );
	if ( !input )
	{
		return false;
	}

	return input->m_leaderData->IsFormationMoving();
}

String CFormationIsMovingSteeringCondition::GetConditionName() const
{
	return TXT("IsFormationMoving");
}

///////////////////////////////////////////////////////////////////////////////
// CFormationIsBrokenSteeringCondition
///////////////////////////////////////////////////////////////////////////////
Bool CFormationIsBrokenSteeringCondition::Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const
{
	SFormationSteeringInput* input = SFormationSteeringInput::GetGeneralFormationData( comm.GetGoal() );
	if ( !input )
	{
		return false;
	}

	CAISlotFormationLeaderData* leaderData = input->m_leaderData->AsSlotFormationLeaderData();
	if ( !leaderData )
	{
		return false;
	}
	return leaderData->GetCurrentBreakRatio() >= m_howMuchBroken;
}

String CFormationIsBrokenSteeringCondition::GetConditionName() const
{
	return TXT("IsFormationBroken");
}