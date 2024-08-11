/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeSnapToNavigation.h"


BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeDecoratorSnapToNavigationDefinition )


Bool CBehTreeNodeDecoratorSnapToNavigationInstance::Activate()
{
	if ( !Super::Activate() )
	{
		return false;
	}

	if ( m_performActivation )
	{
		CMovingAgentComponent* mac = m_owner->GetActor()->GetMovingAgentComponent();
		if ( mac )
		{
			mac->SnapToNavigableSpace( m_snapOnActivation );
		}
	}

	return true;
}

void CBehTreeNodeDecoratorSnapToNavigationInstance::Deactivate()
{
	if ( m_performDeactivation )
	{
		CMovingAgentComponent* mac = m_owner->GetActor()->GetMovingAgentComponent();
		if ( mac )
		{
			mac->SnapToNavigableSpace( m_snapOnDeactivation );
		}
	}

	Super::Deactivate();
}
