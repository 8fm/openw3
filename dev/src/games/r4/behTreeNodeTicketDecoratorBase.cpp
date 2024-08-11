#include "build.h"
#include "behTreeNodeTicketDecoratorBase.h"

////////////////////////////////////////////////////////////////////////
// IBehTreeNodeCombatTicketDecoratorBaseDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeCombatTicketDecoratorBaseDefinition::IBehTreeNodeCombatTicketDecoratorBaseDefinition()
	: m_ticketName( CNAME( TICKET_Melee ))
	, m_ticketsProvider( BTTSP_Combat )
{

}

String IBehTreeNodeCombatTicketDecoratorBaseDefinition::GetNodeCaption() const
{
	return String::Printf( TXT("%s %s ( %s ticket )")
		, GetNodeName().AsChar()
		, m_ticketName.AsString().AsChar()
		, m_ticketsProvider == BTTSP_Combat
		? TXT("combat")
		: TXT("global") );
}

void IBehTreeNodeCombatTicketDecoratorBaseDefinition::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName().AsString() == TXT("ticketName") )
	{
		if ( !m_ticketName.AsString().BeginsWith( TXT("TICKET_") ) )
		{
			m_ticketName = CNAME( TICKET_ );
		}
	}
}

////////////////////////////////////////////////////////////////////////
// IBehTreeNodeCombatTicketDecoratorBaseInstance
////////////////////////////////////////////////////////////////////////

IBehTreeNodeCombatTicketDecoratorBaseInstance::IBehTreeNodeCombatTicketDecoratorBaseInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_ticket( owner, def.m_ticketName, def.m_ticketsProvider )
{
	SBehTreeEvenListeningData e;
	e.m_eventName = CNAME( AI_PreAsynchronousDestruction );
	e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	context.AddEventListener( e, this );
}

void IBehTreeNodeCombatTicketDecoratorBaseInstance::OnDestruction()
{
	{
		SBehTreeEvenListeningData e;
		e.m_eventName = CNAME( AI_PreAsynchronousDestruction );
		e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		m_owner->RemoveEventListener( e, this );
	}

	Super::OnDestruction();
}

Bool IBehTreeNodeCombatTicketDecoratorBaseInstance::OnListenedEvent( CBehTreeEvent& e )
{
	// NOTICE: If ticket data object would be removed outside of main thread, it might actually lead to a concurrency problem, as ticket data are
	if ( e.m_eventName == CNAME( AI_PreAsynchronousDestruction ) )
	{
		m_ticket.Clear();
		return false;
	}

	return Super::OnListenedEvent( e );
}