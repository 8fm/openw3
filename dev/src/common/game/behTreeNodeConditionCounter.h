#pragma once

#include "behTreeNodeCondition.h"
#include "behTreeCounterData.h"

class CBehTreeNodeConditionCounterInstance;
class CBehTreeNodeConditionCounterNewInstance;

////////////////////////////////////////////////////////////////////////
// Definition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionCounterDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionCounterDefinition, CBehTreeNodeConditionDefinition, CBehTreeNodeConditionCounterInstance, ConditionSemaphoreInRange );
protected:	
	CBehTreeValCName	m_counterName;
	CBehTreeValInt		m_counterLowerBound;
	CBehTreeValInt		m_counterUpperBound;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

public:
	CBehTreeNodeConditionCounterDefinition() 
		: m_counterLowerBound( 0 )
		, m_counterUpperBound( 1 )
	  {}

	String GetNodeCaption() const override;
};

BEGIN_CLASS_RTTI(CBehTreeNodeConditionCounterDefinition);
	PARENT_CLASS(CBehTreeNodeConditionDefinition);	
	PROPERTY_EDIT( m_counterName, TXT("Name of the counter vlaue") );
	PROPERTY_EDIT( m_counterLowerBound, TXT("Lower bound value (counter needs to be greater than or equal to this to return true)") );
	PROPERTY_EDIT( m_counterUpperBound, TXT("Upprt bound value (counter needs to be smaller than or equal to this to return true)") );
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// Instance
class CBehTreeNodeConditionCounterInstance : public CBehTreeNodeConditionInstance
{
	typedef CBehTreeNodeConditionInstance Super;
protected:
	CBehTreeCounterDataPtr	m_counter;
	Int32					m_counterLowerBound;
	Int32					m_counterUpperBound;

	Bool ConditionCheck() override;
public:
	typedef CBehTreeNodeConditionCounterDefinition Definition;

	CBehTreeNodeConditionCounterInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	
};

////////////////////////////////////////////////////////////////////////
// New condition counter. The same as above, but insteady of range
// we got comparison function.
class CBehTreeNodeConditionCounterNewDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionCounterNewDefinition, CBehTreeNodeConditionDefinition, CBehTreeNodeConditionCounterNewInstance, ConditionSemaphore );
protected:	
	CBehTreeValCName	m_counterName;
	CBehTreeValInt		m_counterValue;
	ECompareFunc		m_comparison;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

public:
	CBehTreeNodeConditionCounterNewDefinition() 
		: m_counterValue( 0 )
		, m_comparison( CF_Greater )
	{}

	String GetNodeCaption() const override;
};

BEGIN_CLASS_RTTI(CBehTreeNodeConditionCounterNewDefinition);
	PARENT_CLASS(CBehTreeNodeConditionDefinition);	
	PROPERTY_EDIT( m_counterName, TXT("Name of the counter vlaue") );
	PROPERTY_EDIT( m_counterValue, TXT("Comparison value") );
	PROPERTY_EDIT( m_comparison, TXT("Comparison function") );
END_CLASS_RTTI();

class CBehTreeNodeConditionCounterNewInstance : public CBehTreeNodeConditionInstance
{
	typedef CBehTreeNodeConditionInstance Super;
protected:
	CBehTreeCounterDataPtr	m_counter;
	Int32					m_counterValue;
	ECompareFunc			m_comparison;

	Bool ConditionCheck() override;
public:
	typedef CBehTreeNodeConditionCounterNewDefinition Definition;

	CBehTreeNodeConditionCounterNewInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
};