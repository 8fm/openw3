#pragma once

#include "behTreeDecorator.h"
#include "behTreeNodeAtomicAction.h"

class CBehTreeDecoratorActivePriorityInstance;

////////////////////////////////////////////////////////////////////////
// Decorator that sets sub-action target, and resets it on deactivation.
////////////////////////////////////////////////////////////////////////
class CBehTreeDecoratorActivePriorityDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeDecoratorActivePriorityDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeDecoratorActivePriorityInstance, DecoratorActivePriority );
protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
	Bool	m_evaluateChildWhenActive;
	

public:
	CBehTreeDecoratorActivePriorityDefinition() : m_evaluateChildWhenActive( false ){}
	String GetNodeCaption() const override;
};


BEGIN_CLASS_RTTI( CBehTreeDecoratorActivePriorityDefinition );
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition );
	PROPERTY_EDIT( m_evaluateChildWhenActive, TXT("") )
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// INSTANCE
////////////////////////////////////////////////////////////////////////
class CBehTreeDecoratorActivePriorityInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
public:
	typedef CBehTreeDecoratorActivePriorityDefinition Definition;

	CBehTreeDecoratorActivePriorityInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )	
		, m_evaluateChildWhenActive( def.m_evaluateChildWhenActive ){}

	Int32 Evaluate() override;
	Bool IsAvailable() override;

private:
	Bool m_evaluateChildWhenActive;
};