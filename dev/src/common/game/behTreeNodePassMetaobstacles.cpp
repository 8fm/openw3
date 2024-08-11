/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodePassMetaobstacles.h"

#include "behTreeInstance.h"
#include "movableRepresentationPathAgent.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeDecoratorPassMetaobstaclesDefinition )

Bool CBehTreeNodeDecoratorPassMetaobstaclesInstance::Activate()
{
	if ( Super::Activate() )
	{
		CActor* actor = m_owner->GetActor();
		CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
		if ( mac )
		{
			CPathAgent* pathAgent = mac->GetPathAgent();
			pathAgent->AddCollisionFlags( PathLib::CT_IGNORE_METAOBSTACLE );
		}
		return true;
	}

	DebugNotifyActivationFail();
	return false;
}
void CBehTreeNodeDecoratorPassMetaobstaclesInstance::Deactivate()
{
	CActor* actor = m_owner->GetActor();
	CMovingAgentComponent* mac = actor ? actor->GetMovingAgentComponent() : NULL;
	if ( mac )
	{
		CPathAgent* pathAgent = mac->GetPathAgent();
		pathAgent->RemoveCollisionFlags( PathLib::CT_IGNORE_METAOBSTACLE );
	}

	Super::Deactivate();
}