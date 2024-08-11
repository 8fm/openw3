/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeCasualMovementDecorator.h"

#include "movableRepresentationPathAgent.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeCasualMovementDecoratorDefinition )

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeCasualMovementDecoratorInstance
///////////////////////////////////////////////////////////////////////////////

Bool CBehTreeNodeCasualMovementDecoratorInstance::Activate()
{
	if ( Super::Activate() )
	{
		CActor* actor = m_owner->GetActor();
		if ( CMovingAgentComponent* mac = actor->GetMovingAgentComponent() )
		{
			if ( CPathAgent* pathAgent = mac->GetPathAgent() )
			{
				pathAgent->AddForbiddenPathfindFlag( PathLib::NF_ROUGH_TERRAIN );
			}
		}

		return true;
	}
	return false;
}
void CBehTreeNodeCasualMovementDecoratorInstance::Deactivate()
{
	CActor* actor = m_owner->GetActor();
	if ( CMovingAgentComponent* mac = actor ? actor->GetMovingAgentComponent() : nullptr )
	{
		if ( CPathAgent* pathAgent = mac->GetPathAgent() )
		{
			pathAgent->RemoveForbiddenPathfindFlag( PathLib::NF_ROUGH_TERRAIN );
		}
	}

	Super::Deactivate();
}