/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeConditionIsInGuardArea.h"

#include "behTreeInstance.h"
#include "../engine/areaComponent.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeConditionIsActionTargetInGuardAreaDefinition )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeConditionIsCustomTargetInGuardAreaDefinition )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeConditionIsThisActorInGuardAreaDefinition )

///////////////////////////////////////////////////////////////////////////////
// IBehTreeNodeConditionIsInGuardAreaInstance
///////////////////////////////////////////////////////////////////////////////
IBehTreeNodeConditionIsInGuardAreaInstance::IBehTreeNodeConditionIsInGuardAreaInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_guardAreaPtr( owner )
{

}
Bool IBehTreeNodeConditionIsInGuardAreaInstance::ConditionCheck()
{
	CAreaComponent* guardArea = m_guardAreaPtr->GetGuardArea();
	if ( !guardArea )
	{
		// NOTICE: its intuitive, that we threat whole world as our guard area if we have none area set.
		return true;
	}

	Vector targetPos;
	if( !GetTarget( targetPos ) )
	{
		return false;
	}

	return guardArea->TestPointOverlap( targetPos );
}

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionIsActionTargetInGuardAreaInstance
///////////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeConditionIsActionTargetInGuardAreaInstance::GetTarget( Vector& outPosition )
{
	CNode* actionTarget = m_owner->GetActionTarget().Get();
	if ( !actionTarget )
	{
		return false;
	}
	outPosition = actionTarget->GetWorldPositionRef();
	return true;
}


///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionIsActionTargetInGuardAreaInstance
///////////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeConditionIsThisActorInGuardAreaInstance::GetTarget( Vector& outPosition )
{	
	outPosition = m_owner->GetActor()->GetWorldPositionRef();
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionIsCustomTargetInGuardAreaDefinition
///////////////////////////////////////////////////////////////////////////////
CBehTreeNodeConditionIsCustomTargetInGuardAreaInstance::CBehTreeNodeConditionIsCustomTargetInGuardAreaInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_customTargetPtr( owner )
{

}
Bool CBehTreeNodeConditionIsCustomTargetInGuardAreaInstance::GetTarget( Vector& outPosition )
{
	outPosition = m_customTargetPtr->GetTarget();
	return true;
}
