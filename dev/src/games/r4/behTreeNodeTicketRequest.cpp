/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeTicketRequest.h"

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeCombatTicketRequestDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeDecoratorInstance* CBehTreeNodeCombatTicketRequestDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeCombatTicketRequestInstance
////////////////////////////////////////////////////////////////////////

Bool CBehTreeNodeCombatTicketRequestInstance::Activate()
{
	if ( Super::Activate() )
	{
		if ( m_requestWhileActive )
		{
			m_ticket->ManagerPersistantRequest();
			m_hasPermanentRequest = true;
		}

		return true;
	}

	DebugNotifyActivationFail();
	return false;
}
void CBehTreeNodeCombatTicketRequestInstance::Deactivate()
{
	if ( m_hasPermanentRequest )
	{
		m_ticket->ClearRequest();
		m_hasPermanentRequest = false;
	}

	if ( m_requestOnInterruption && m_child->IsActive() )
	{
		m_ticket->ManagerRequest( m_ticketRequestValidTime );
	}

	Super::Deactivate();
}
void CBehTreeNodeCombatTicketRequestInstance::Complete( eTaskOutcome outcome )
{
	if ( m_hasPermanentRequest )
	{
		m_ticket->ClearRequest();
		m_hasPermanentRequest = false;
	}

	if ( m_requestOnCompletion )
	{
		m_ticket->ManagerRequest( m_ticketRequestValidTime );
	}
	
	Super::Complete( outcome );
}
