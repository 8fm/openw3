/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeConditionIsTargetThePlayer.h"

#include "../../common/game/behTreeInstance.h"
#include "../../common/game/player.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeConditionIsTargetThePlayerDefinition )

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionIsTargetThePlayerInstance
///////////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeConditionIsTargetThePlayerInstance::ConditionCheck()
{
	CNode* target = m_useCombatTarget ? m_owner->GetCombatTarget().Get() : m_owner->GetActionTarget().Get();
	
	return target->IsA< CPlayer >();
}