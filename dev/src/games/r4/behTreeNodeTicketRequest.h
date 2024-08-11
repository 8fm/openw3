/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeNodeTicketDecoratorBase.h"

class CBehTreeNodeCombatTicketRequestInstance;

class CBehTreeNodeCombatTicketRequestDefinition : public IBehTreeNodeCombatTicketDecoratorBaseDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeCombatTicketRequestDefinition, IBehTreeNodeCombatTicketDecoratorBaseDefinition, CBehTreeNodeCombatTicketRequestInstance, TicketRequest )
protected:
	Float							m_ticketRequestValidTime;
	Bool							m_requestOnCompletion;
	Bool							m_requestOnInterruption;
	Bool							m_requestWhileActive;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeCombatTicketRequestDefinition )
	PARENT_CLASS( IBehTreeNodeCombatTicketDecoratorBaseDefinition )
	PROPERTY_EDIT( m_ticketRequestValidTime, TXT("Time in which ticket request is valid and taked into consideration") );
	PROPERTY_EDIT( m_requestOnCompletion, TXT("Do request on node completion") );
	PROPERTY_EDIT( m_requestOnInterruption, TXT("Do request on node completion") );
	PROPERTY_EDIT( m_requestWhileActive, TXT("Do request on node completion") );
END_CLASS_RTTI()

class CBehTreeNodeCombatTicketRequestInstance : public IBehTreeNodeCombatTicketDecoratorBaseInstance
{
	typedef IBehTreeNodeCombatTicketDecoratorBaseInstance Super;

protected:
	Float							m_ticketRequestValidTime;
	Bool							m_requestOnCompletion;
	Bool							m_requestOnInterruption;
	Bool							m_requestWhileActive;
	Bool							m_hasPermanentRequest;

public:
	typedef CBehTreeNodeCombatTicketRequestDefinition Definition;

	CBehTreeNodeCombatTicketRequestInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_ticketRequestValidTime( def.m_ticketRequestValidTime )
		, m_requestOnCompletion( def.m_requestOnCompletion )
		, m_requestOnInterruption( def.m_requestOnInterruption )
		, m_requestWhileActive( def.m_requestWhileActive )
		, m_hasPermanentRequest( false )									{}

	Bool Activate() override;
	void Deactivate() override;
	void Complete( eTaskOutcome outcome ) override;
};