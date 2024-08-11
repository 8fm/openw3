/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeTicketGet.h"

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeCombatTicketManagedGetDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeDecoratorInstance*	CBehTreeNodeCombatTicketManagedGetDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeCombatTicketManagedGetInstance
////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeCombatTicketManagedGetInstance::CheckTicket()
{
	m_ticket->ManagerRequest( m_ticketRequestValidTime );
	if ( !m_ticket->HasTicket() )
	{
		Float importance;
		Uint16 ticketsCount;
		ComputeTicketImportance( importance, ticketsCount );
		if ( !m_ticket->CanAquireTicket( importance, ticketsCount ) )
		{
			return false;
		}
	}
	return true;
}
Bool CBehTreeNodeCombatTicketManagedGetInstance::IsAvailable()
{
	if ( !m_isActive && !CheckTicket() )
	{
		DebugNotifyAvailableFail();
		return false;
	}

	return Super::IsAvailable();
}

Int32 CBehTreeNodeCombatTicketManagedGetInstance::Evaluate()
{
	if ( !m_isActive && !CheckTicket() )
	{
		DebugNotifyAvailableFail();
		return -1;
	}

	return Super::Evaluate();
}

void CBehTreeNodeCombatTicketManagedGetInstance::Update()
{
	if ( m_nextImportanceUpdate < m_owner->GetLocalTime() )
	{
		Float importance;
		Uint16 ticketsCount;
		ComputeTicketImportance( importance, ticketsCount );
		if ( !m_ticket->UpdateImportance( importance ) && m_failsWhenTicketIsLost )
		{
			// we just have lost our ticket (because of importance decrease)
			Complete( BTTO_FAILED );
			return;
		}
		DelayImportanceUpdate();
	}

	IBehTreeNodeDecoratorInstance::Update();
}
Bool CBehTreeNodeCombatTicketManagedGetInstance::Activate()
{
	Float importance;
	Uint16 ticketsCount;
	ComputeTicketImportance( importance, ticketsCount );
	if ( !m_ticket->Aquire( importance, ticketsCount, true ) )
	{
		DebugNotifyActivationFail();
		return false;
	}

	if ( !IBehTreeNodeDecoratorInstance::Activate() )
	{
		m_ticket->FreeTicket();
		DebugNotifyActivationFail();
		return false;
	}

	if ( m_locksTicket )
	{
		m_ticket->LockTicket( true );
	}

	//m_nextImportanceUpdate = m_owner->GetLocalTime() + m_importanceUpdateDelay;
	return true;
}
void CBehTreeNodeCombatTicketManagedGetInstance::Deactivate()
{
	if ( m_freesTicket )
	{
		m_ticket->FreeTicket();
	}

	IBehTreeNodeDecoratorInstance::Deactivate();
}

Bool CBehTreeNodeCombatTicketManagedGetInstance::OnEvent( CBehTreeEvent& e )
{
	if ( e.m_eventName == CNAME( AI_LostTicket ) && m_failsWhenTicketIsLost )
	{
		SGameplayEventParamCName* params = e.m_gameplayEventData.Get< SGameplayEventParamCName >();
		if ( params && params->m_value == m_ticket->GetTicketName() )
		{
			// Importance update will make npc. We don't want to proceed with AI_LostTicket instantly
			m_nextImportanceUpdate = 0.f;
		}
		return false;
	}

	return Super::OnEvent( e );
}

