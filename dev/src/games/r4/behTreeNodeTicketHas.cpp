/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeTicketHas.h"

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeCombatTicketHasDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeDecoratorInstance* CBehTreeNodeCombatTicketHasDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return m_lockTicket ?
		new CBehTreeNodeCombatTicketHasAndLockInstance( *this, owner, context, parent ) :
		new CBehTreeNodeCombatTicketHasInstance( *this, owner, context, parent );
}


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeCombatTicketHasInstance
////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeCombatTicketHasInstance::IsAvailable()
{
	if ( !m_isActive && (!m_ticket->HasTicket() ^ m_ifNotHave) )
	{
		DebugNotifyAvailableFail();
		return false;
	}

	return Super::IsAvailable();
}
Int32 CBehTreeNodeCombatTicketHasInstance::Evaluate()
{
	if ( !m_isActive && (!m_ticket->HasTicket() ^ m_ifNotHave) )
	{
		DebugNotifyAvailableFail();
		return -1;
	}
	return Super::Evaluate();
}

void CBehTreeNodeCombatTicketHasInstance::Update()
{
	if ( m_failsWhenTicketIsLost )
	{
		if ( (!m_ticket->HasTicket()) ^ m_ifNotHave )
		{
			Complete( BTTO_FAILED );
			return;
		}
	}

	Super::Update();
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeCombatTicketHasAndLockInstance
////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeCombatTicketHasAndLockInstance::Activate()
{
	if ( Super::Activate() )
	{
		m_ticket->LockTicket( true );
		return true;
	}

	DebugNotifyActivationFail();
	return false;
}
void CBehTreeNodeCombatTicketHasAndLockInstance::Deactivate()
{
	Super::Deactivate();
	m_ticket->LockTicket( false );
}