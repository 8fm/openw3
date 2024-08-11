/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeConditionHasTarget.h"

#include "behTreeInstance.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeConditionHasCombatTargetDefinition )

	
Bool CBehTreeNodeConditionHasCombatTargetInstance::ConditionCheck()
{
	return m_owner->GetCombatTarget().Get() != nullptr;
}