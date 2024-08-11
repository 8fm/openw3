/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeNodeTicketManager.h"


////////////////////////////////////////////////////////////////////////
// Node that takes, frees combat ticket, and is
// available only when he can get ticket, completes when ticket is lost.
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeCombatTicketManagedGetDefinition : public CBehTreeNodeCombatTicketManagerDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeCombatTicketManagedGetDefinition, CBehTreeNodeCombatTicketManagerDefinition, CBehTreeNodeCombatTicketManagedGetInstance, TicketGet );

protected:
	Bool												m_locksTicket;
	Bool												m_freesTicket;
	Bool												m_failsWhenTicketIsLost;
	Float												m_ticketRequestValidTime;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

public:
	CBehTreeNodeCombatTicketManagedGetDefinition()
		: m_locksTicket( false )
		, m_freesTicket( true )
		, m_failsWhenTicketIsLost( true )
		, m_ticketRequestValidTime( 2.f )								{}
};

BEGIN_CLASS_RTTI( CBehTreeNodeCombatTicketManagedGetDefinition );
	PARENT_CLASS( CBehTreeNodeCombatTicketManagerDefinition );
	PROPERTY_EDIT( m_locksTicket, TXT("Locks obtained ticket") );
	PROPERTY_EDIT( m_freesTicket, TXT("Frees ticket on deactivation") );
	PROPERTY_EDIT( m_failsWhenTicketIsLost, TXT("Node automatically completes whenever it loose ticket") );
	PROPERTY_EDIT( m_ticketRequestValidTime, TXT("How long ticket request stays valid") );
END_CLASS_RTTI();

class CBehTreeNodeCombatTicketManagedGetInstance : public CBehTreeNodeCombatTicketManagerInstance
{
	typedef CBehTreeNodeCombatTicketManagerInstance Super;
protected:
	Bool										m_locksTicket;
	Bool										m_freesTicket;
	Bool										m_failsWhenTicketIsLost;
	Float										m_ticketRequestValidTime;

	Bool CheckTicket();
public:
	typedef CBehTreeNodeCombatTicketManagedGetDefinition Definition;

	CBehTreeNodeCombatTicketManagedGetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_locksTicket( def.m_locksTicket )
		, m_freesTicket( def.m_freesTicket )
		, m_failsWhenTicketIsLost( def.m_failsWhenTicketIsLost )
		, m_ticketRequestValidTime( def.m_ticketRequestValidTime )	{}

	void Update() override;
	Bool Activate() override;
	void Deactivate() override;

	Bool IsAvailable() override;
	Int32 Evaluate() override;

	Bool OnEvent( CBehTreeEvent& e ) override;
};