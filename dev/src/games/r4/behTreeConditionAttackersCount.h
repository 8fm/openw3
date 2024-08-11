/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/game/behTreeNodeCondition.h"

////////////////////////////////////////////////////////////////////////
// Definition
////////////////////////////////////////////////////////////////////////
class CBehTreeConditionAttackersCountInstance;
class CBehTreeNodeConditionAttackersCountDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionAttackersCountDefinition, CBehTreeNodeConditionDefinition, CBehTreeConditionAttackersCountInstance, ConditionAttackersCount );
protected:
	ECompareFunc			m_compare;
	CBehTreeValInt			m_count;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeConditionAttackersCountDefinition( )
		:  m_compare( CF_Greater )
		,  m_count( 0 )
	{}
};

BEGIN_CLASS_RTTI(CBehTreeNodeConditionAttackersCountDefinition);
	PARENT_CLASS(CBehTreeNodeConditionDefinition);	
	PROPERTY_EDIT( m_compare,	TXT("Compare function") );
	PROPERTY_EDIT( m_count,		TXT("Number of attackers to compare to") );
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// Instance
class CBehTreeConditionAttackersCountInstance : public CBehTreeNodeConditionInstance
{
protected:
	typedef CBehTreeNodeConditionInstance Super;

	ECompareFunc			m_compare;
	Int32					m_count;

	Bool ConditionCheck() override;

public:
	typedef CBehTreeNodeConditionAttackersCountDefinition Definition;

	CBehTreeConditionAttackersCountInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_compare( def.m_compare )
		, m_count( def.m_count.GetVal( context ) )						{}
};