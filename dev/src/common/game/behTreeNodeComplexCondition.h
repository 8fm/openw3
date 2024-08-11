#pragma once

#include "behTreeNodeCondition.h"

class CBehTreeNodeComplexConditionInstance;
class IBehTreeAtomicConditionInstance;

////////////////////////////////////////////////////////////////////////
// Complex condition node
class CBehTreeNodeComplexConditionDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeComplexConditionDefinition, CBehTreeNodeConditionDefinition, CBehTreeNodeComplexConditionInstance, ComplexCondition );
public:
	CBehTreeNodeComplexConditionDefinition()
		: m_condition( NULL )											{}

	 Bool IsValid() const override;
protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

	IBehTreeAtomicCondition*				m_condition;
};


BEGIN_CLASS_RTTI(CBehTreeNodeComplexConditionDefinition);
	PARENT_CLASS(CBehTreeNodeConditionDefinition);	
	PROPERTY_INLINED( m_condition, TXT("Condition test atoms") );
END_CLASS_RTTI();

class CBehTreeNodeComplexConditionInstance : public CBehTreeNodeConditionInstance
{
	typedef CBehTreeNodeConditionInstance Super;
protected:
	Bool ConditionCheck() override;

	IBehTreeAtomicConditionInstance*		m_condition;

public:
	typedef CBehTreeNodeComplexConditionDefinition Definition;

	CBehTreeNodeComplexConditionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );
};
