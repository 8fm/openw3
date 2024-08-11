/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeConditionIsAtWork.h"


BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeIsAtWorkConditionDefinition )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeCanUseChatSceneConditionDefinition )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeIsSittingInInteriorConditionDefinition )

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeIsAtWorkConditionInstance
///////////////////////////////////////////////////////////////////////////////

Bool CBehTreeNodeIsAtWorkConditionInstance::ConditionCheck()
{
	CBehTreeWorkData& workData = *m_workDataPtr;

	return workData.IsBeingPerformed();
}

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeCanUseChatSceneConditionInstance
///////////////////////////////////////////////////////////////////////////////

Bool CBehTreeNodeCanUseChatSceneConditionInstance::ConditionCheck()
{
	CBehTreeWorkData& workData = *m_workDataPtr;

	return workData.IsBeingPerformed() && workData.GetIsConscious();
}

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeIsSittingInInteriorConditionInstance
///////////////////////////////////////////////////////////////////////////////

Bool CBehTreeNodeIsSittingInInteriorConditionInstance::ConditionCheck()
{
	CBehTreeWorkData& workData = *m_workDataPtr;

	if ( !workData.IsBeingPerformed() || !workData.GetIsSitting() )
	{
		return false;
	}

	CNewNPC* npc = m_owner->GetNPC();
	
	if ( !npc )
	{
		return false;
	}

	if ( npc->IsInInterior() )
	{
		return false;
	}

	return true;
}
