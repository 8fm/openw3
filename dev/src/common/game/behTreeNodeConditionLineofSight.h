#pragma once

#include "behTreeNodeCondition.h"

class CBehTreeNodeConditionLineofSightInstance;
class CBehTreeNodeConditionLineofSightToNamedTargetInstance;
////////////////////////////////////////////////////////////////////////
// Definition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionLineofSightDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionLineofSightDefinition, CBehTreeNodeConditionDefinition, CBehTreeNodeConditionLineofSightInstance, ConditionLineofSight );

protected:
	CBehTreeValBool m_useCombatTarget;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

public:
	CBehTreeNodeConditionLineofSightDefinition() 
		: CBehTreeNodeConditionDefinition()
		, m_useCombatTarget( false )
	{
	}	
};


BEGIN_CLASS_RTTI(CBehTreeNodeConditionLineofSightDefinition);
	PARENT_CLASS(CBehTreeNodeConditionDefinition);
	PROPERTY_EDIT( m_useCombatTarget, TXT("false: use action target. true: use combat target") )
END_CLASS_RTTI();

class CBehTreeNodeConditionLineofSightInstance : public CBehTreeNodeConditionInstance
{
protected:
	Bool m_useCombatTarget;

	Bool ConditionCheck() override;

	Bool TestLOS( const Vector& sourcePos, const CNode* targetPos ) const;

public:
	typedef CBehTreeNodeConditionLineofSightDefinition Definition;

	CBehTreeNodeConditionLineofSightInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );	
};

////////////////////////////////////////////////////////////////////////
// Check to named target
class CBehTreeNodeConditionLineofSightToNamedTargetDefinition : public CBehTreeNodeConditionLineofSightDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionLineofSightToNamedTargetDefinition, CBehTreeNodeConditionLineofSightDefinition, CBehTreeNodeConditionLineofSightToNamedTargetInstance, ConditionLineofSightToNamedTarget );

protected:	
	CBehTreeValCName			m_targetName;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

public:
	CBehTreeNodeConditionLineofSightToNamedTargetDefinition() : CBehTreeNodeConditionLineofSightDefinition()
	{
	}

	String GetNodeCaption() const override;
};

BEGIN_CLASS_RTTI(CBehTreeNodeConditionLineofSightToNamedTargetDefinition);
	PARENT_CLASS(CBehTreeNodeConditionLineofSightDefinition);	
	PROPERTY_EDIT( m_targetName		, TXT( "" ) );
END_CLASS_RTTI();


class CBehTreeNodeConditionLineofSightToNamedTargetInstance : public CBehTreeNodeConditionInstance
{
protected:
	CName m_targetName;

	Bool ConditionCheck() override;

public:
	typedef CBehTreeNodeConditionLineofSightToNamedTargetDefinition Definition;

	CBehTreeNodeConditionLineofSightToNamedTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL )
		: CBehTreeNodeConditionInstance( def, owner, context, parent )
		, m_targetName( def.m_targetName.GetVal( context ) )
	{
	}	
};