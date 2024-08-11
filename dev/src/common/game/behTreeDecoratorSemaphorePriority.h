/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeDecorator.h"

#include "behTreeCounterData.h"

class CBehTreeNodeDecoratorPriorityOnSemaphoreInstance;

class CBehTreeNodeDecoratorPriorityOnSemaphoreDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeDecoratorPriorityOnSemaphoreDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodeDecoratorPriorityOnSemaphoreInstance, PriorityOnSemaphore )
protected:
	CBehTreeValCName	m_counterName;
	CBehTreeValInt		m_counterValue;
	ECompareFunc		m_comparison;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

public:
	CBehTreeNodeDecoratorPriorityOnSemaphoreDefinition()
		: m_counterValue( 0 )
		, m_comparison( CF_Greater ) {}

	String GetNodeCaption() const override;
};

BEGIN_CLASS_RTTI(CBehTreeNodeDecoratorPriorityOnSemaphoreDefinition);
	PARENT_CLASS(IBehTreeNodeDecoratorDefinition);	
	PROPERTY_EDIT( m_counterName, TXT("Name of the counter vlaue") );
	PROPERTY_EDIT( m_counterValue, TXT("Comparison value") );
	PROPERTY_EDIT( m_comparison, TXT("Comparison function") );
END_CLASS_RTTI();


class CBehTreeNodeDecoratorPriorityOnSemaphoreInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	CBehTreeCounterDataPtr	m_counter;
	Int32					m_counterValue;
	ECompareFunc			m_comparison;
public:
	typedef CBehTreeNodeDecoratorPriorityOnSemaphoreDefinition Definition;

	CBehTreeNodeDecoratorPriorityOnSemaphoreInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	Int32 Evaluate() override;
};