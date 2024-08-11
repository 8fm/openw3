/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeNodeTicketDecoratorBase.h"

class CBehTreeNodeCombatTicketReleaseInstance;

class CBehTreeNodeCombatTicketReleaseDefinition : public IBehTreeNodeCombatTicketDecoratorBaseDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeCombatTicketReleaseDefinition, IBehTreeNodeCombatTicketDecoratorBaseDefinition, CBehTreeNodeCombatTicketReleaseInstance, TicketRelease )

protected:
	Bool					m_releaseOnActivation;
	Bool					m_releaseOnDeactivation;
	Bool					m_releaseOnCompletion;
public:

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeCombatTicketReleaseDefinition )
	PARENT_CLASS( IBehTreeNodeCombatTicketDecoratorBaseDefinition )
	PROPERTY_EDIT( m_releaseOnActivation, TXT("Release ticket on behavior activation") )
	PROPERTY_EDIT( m_releaseOnDeactivation, TXT("Release ticket on behavior deactivation") )
	PROPERTY_EDIT( m_releaseOnCompletion, TXT("Release ticket on behavior completion") )
END_CLASS_RTTI()

class CBehTreeNodeCombatTicketReleaseInstance : public IBehTreeNodeCombatTicketDecoratorBaseInstance
{
	typedef IBehTreeNodeCombatTicketDecoratorBaseInstance Super;

protected:
	Bool					m_releaseOnActivation;
	Bool					m_releaseOnDeactivation;
	Bool					m_releaseOnCompletion;

public:
	typedef CBehTreeNodeCombatTicketReleaseDefinition Definition;

	CBehTreeNodeCombatTicketReleaseInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_releaseOnActivation( def.m_releaseOnActivation )
		, m_releaseOnDeactivation( def.m_releaseOnDeactivation )
		, m_releaseOnCompletion( def.m_releaseOnCompletion )							{}

	Bool Activate() override;
	void Deactivate() override;
	void Complete( eTaskOutcome outcome ) override;
};