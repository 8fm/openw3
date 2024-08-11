/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once


#include "behTreeNodeCondition.h"


class CBehTreeNodeConditionChanceInstance;

class CBehTreeNodeConditionChanceDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionChanceDefinition, CBehTreeNodeConditionDefinition, CBehTreeNodeConditionChanceInstance, ConditionChance )
protected:
	CBehTreeValFloat			m_chance;
	Float						m_resultValidFor;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

public:
	CBehTreeNodeConditionChanceDefinition()
		: m_chance( 50.f )
		, m_resultValidFor( 10.f )															{}
};

BEGIN_CLASS_RTTI( CBehTreeNodeConditionChanceDefinition )
	PARENT_CLASS( CBehTreeNodeConditionDefinition )
	PROPERTY_EDIT( m_chance, TXT("Chance of node to be available in %. [0..100]") )
	PROPERTY_EDIT( m_resultValidFor, TXT("Time in which randomized result is valid") )
END_CLASS_RTTI()

class CBehTreeNodeConditionChanceInstance : public CBehTreeNodeConditionInstance
{
	typedef CBehTreeNodeConditionInstance Super;
protected:
	// setup
	Float						m_chance;
	Float						m_resultValidFor;

	// runtime
	Float						m_lastTestTimeout;
	Bool						m_lastTestOutcome;

	Bool						ConditionCheck() override;
public:
	typedef CBehTreeNodeConditionChanceDefinition Definition;

	CBehTreeNodeConditionChanceInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_chance( def.m_chance.GetVal( context ) / 100.f )							// NOTICE: change % to [0..1]
		, m_resultValidFor( def.m_resultValidFor )
		, m_lastTestTimeout( -1.f )														{}
};