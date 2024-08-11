#pragma once

#include "../../common/game/behTreeDecorator.h"

#include "behTreeTicketData.h"

class IBehTreeNodeCombatTicketDecoratorBaseInstance;
	
class IBehTreeNodeCombatTicketDecoratorBaseDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( IBehTreeNodeCombatTicketDecoratorBaseDefinition, IBehTreeNodeDecoratorDefinition, IBehTreeNodeCombatTicketDecoratorBaseInstance, TicketDecorator );

protected:
	CName												m_ticketName;
	EBehTreeTicketSourceProviderType					m_ticketsProvider;

public:
	IBehTreeNodeCombatTicketDecoratorBaseDefinition();
	
	String			GetNodeCaption() const override;
	void			OnPropertyPostChange( IProperty* property ) override;
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeNodeCombatTicketDecoratorBaseDefinition );
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition );
	PROPERTY_EDIT( m_ticketName, TXT("Name of a ticket type (use prefix 'TICKET_')") );
	PROPERTY_EDIT( m_ticketsProvider, TXT("Tickets source provider") );
END_CLASS_RTTI();


class IBehTreeNodeCombatTicketDecoratorBaseInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	CBehTreeTicketDataPtr								m_ticket;

public:
	typedef IBehTreeNodeCombatTicketDecoratorBaseDefinition Definition;

	IBehTreeNodeCombatTicketDecoratorBaseInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
	void OnDestruction() override;
	Bool OnListenedEvent( CBehTreeEvent& e ) override;
};