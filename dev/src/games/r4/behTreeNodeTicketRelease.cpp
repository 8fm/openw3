/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeTicketRelease.h"


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeCombatTicketReleaseDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeDecoratorInstance* CBehTreeNodeCombatTicketReleaseDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeCombatTicketReleaseInstance
////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeCombatTicketReleaseInstance::Activate()
{
	if ( Super::Activate() )
	{
		if ( m_releaseOnActivation )
		{
			m_ticket->FreeTicket();
		}
		
		return true;
	}

	DebugNotifyActivationFail();
	return false;
}
void CBehTreeNodeCombatTicketReleaseInstance::Deactivate()
{
	if ( m_releaseOnDeactivation )
	{
		m_ticket->FreeTicket();
	}
	Super::Deactivate();
}
void CBehTreeNodeCombatTicketReleaseInstance::Complete( eTaskOutcome outcome )
{
	if ( m_releaseOnCompletion )
	{
		m_ticket->FreeTicket();
	}
	Super::Complete( outcome );
}
