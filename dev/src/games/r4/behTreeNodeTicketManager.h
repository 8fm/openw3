/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeNodeTicketDecoratorBase.h"
#include "behTreeTicketData.h"

class CBehTreeNodeCombatTicketManagerInstance;
class CBehTreeNodeCombatTicketManagedGetInstance;
class IBehTreeTicketAlgorithmDefinition;
class IBehTreeTicketAlgorithmInstance;


////////////////////////////////////////////////////////////////////////
// Root node for ai tree branch that intends to use given ticket.
// It manages ticket requests, importance updating, frees it on
// deactivation etc.
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeCombatTicketManagerDefinition : public IBehTreeNodeCombatTicketDecoratorBaseDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeCombatTicketManagerDefinition, IBehTreeNodeCombatTicketDecoratorBaseDefinition, CBehTreeNodeCombatTicketManagerInstance, TicketManager );
protected:
	CBehTreeValInt										m_ticketsCount;
	Float												m_importanceUpdateDelay;
	IBehTreeTicketAlgorithmDefinition*					m_ticketAlgorithm;


	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

public:
	CBehTreeNodeCombatTicketManagerDefinition()
		: m_ticketsCount( 100 )
		, m_importanceUpdateDelay( 1.f )
		, m_ticketAlgorithm( NULL )										{}
};

BEGIN_CLASS_RTTI( CBehTreeNodeCombatTicketManagerDefinition );
	PARENT_CLASS( IBehTreeNodeCombatTicketDecoratorBaseDefinition );
	PROPERTY_EDIT( m_ticketsCount, TXT("Tickets to reserve") );
	PROPERTY_EDIT( m_importanceUpdateDelay, TXT("Delay between ticket importance updates") );
	PROPERTY_INLINED( m_ticketAlgorithm, TXT("Ticketing algorithm used to compute ticket property") );
END_CLASS_RTTI();

class CBehTreeNodeCombatTicketManagerInstance : public IBehTreeNodeCombatTicketDecoratorBaseInstance
{
	typedef IBehTreeNodeCombatTicketDecoratorBaseInstance Super;
protected:
	Uint16										m_ticketsCount;
	Float										m_importanceUpdateDelay;
	Float										m_nextImportanceUpdate;
	IBehTreeTicketAlgorithmInstance*			m_ticketAlgorithm;

	void ComputeTicketImportance( Float& importance, Uint16& count );
	void ForceImportanceUpdate();
	void DelayImportanceUpdate();
public:
	typedef CBehTreeNodeCombatTicketManagerDefinition Definition;

	CBehTreeNodeCombatTicketManagerInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );

	void OnDestruction() override;

	void Update() override;
	void Deactivate() override;

	Bool OnListenedEvent( CBehTreeEvent& e ) override;
};

