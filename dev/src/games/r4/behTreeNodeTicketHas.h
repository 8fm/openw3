/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeNodeTicketDecoratorBase.h"

class CBehTreeNodeCombatTicketHasInstance;

// Simple condition-like goal that checks if ticket is owned and if so - pass test forward
class CBehTreeNodeCombatTicketHasDefinition : public IBehTreeNodeCombatTicketDecoratorBaseDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeCombatTicketHasDefinition, IBehTreeNodeCombatTicketDecoratorBaseDefinition, CBehTreeNodeCombatTicketHasInstance, TicketHas );
protected:
	Bool					m_lockTicket;
	Bool					m_ifNotHave;
	Bool					m_failsWhenTicketIsLost;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeCombatTicketHasDefinition()
		: m_lockTicket( false )
		, m_ifNotHave( false )
		, m_failsWhenTicketIsLost( true )									{}
};

BEGIN_CLASS_RTTI( CBehTreeNodeCombatTicketHasDefinition )
	PARENT_CLASS( IBehTreeNodeCombatTicketDecoratorBaseDefinition )
	PROPERTY_EDIT( m_lockTicket, TXT("If set, node will be additionaly locking the ticket it gets") )
	PROPERTY_EDIT( m_ifNotHave, TXT("If marked node is available when ticket IS NOT owned") )
	PROPERTY_EDIT( m_failsWhenTicketIsLost, TXT("Behavior fails whenever it loose ticket") )
END_CLASS_RTTI()

class CBehTreeNodeCombatTicketHasInstance : public IBehTreeNodeCombatTicketDecoratorBaseInstance
{
	typedef IBehTreeNodeCombatTicketDecoratorBaseInstance Super;
protected:
	Bool					m_ifNotHave;
	Bool					m_failsWhenTicketIsLost;
public:
	typedef CBehTreeNodeCombatTicketHasDefinition Definition;

	CBehTreeNodeCombatTicketHasInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_ifNotHave( def.m_ifNotHave )
		, m_failsWhenTicketIsLost( def.m_failsWhenTicketIsLost )						{}

	Bool IsAvailable() override;
	Int32 Evaluate() override;
	void Update() override;
};

class CBehTreeNodeCombatTicketHasAndLockInstance : public CBehTreeNodeCombatTicketHasInstance
{
	typedef CBehTreeNodeCombatTicketHasInstance Super;
public:
	CBehTreeNodeCombatTicketHasAndLockInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )											{}

	Bool Activate() override;
	void Deactivate() override;
};
