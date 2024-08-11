/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "formationSteeringInput.h"

#include "movementGoal.h"

IMPLEMENT_ENGINE_CLASS( SFormationSteeringInput )


void SFormationSteeringInput::SetGeneralFormationData( SMoveLocomotionGoal& goal, SFormationSteeringInput* formationData )
{
	goal.TSetFlag( GetGoalCustomParameterName(), formationData );
}
SFormationSteeringInput* SFormationSteeringInput::GetGeneralFormationData( const SMoveLocomotionGoal& goal )
{
	SFormationSteeringInput* formationData = NULL;
	goal.TGetFlag( GetGoalCustomParameterName(), formationData );
	return formationData;
}