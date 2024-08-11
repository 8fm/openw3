/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeTicketLock.h"

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeCombatTicketLockDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeDecoratorInstance* CBehTreeNodeCombatTicketLockDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeCombatTicketLockInstance
////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeCombatTicketLockInstance::Activate()
{
	if ( Super::Activate() )
	{
		m_ticket->LockTicket( true );
		return true;
	}

	DebugNotifyActivationFail();
	return false;
}

void CBehTreeNodeCombatTicketLockInstance::Deactivate()
{
	m_ticket->LockTicket( false );
	Super::Deactivate();
}