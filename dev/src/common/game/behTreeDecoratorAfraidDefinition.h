#pragma once
#include "behtreedecorator.h"
#include "behTreeNodeAtomicAction.h"

class CBehTreeDecoratorAfraidInstance;

////////////////////////////////////////////////////////////////////////
// Decorator that sets actor afraid state, and resets it on deactivation.
////////////////////////////////////////////////////////////////////////
class CBehTreeDecoratorAfraidDefinition :
	public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeDecoratorAfraidDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeDecoratorAfraidInstance, SetIsAfraid );

protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

public:
	CBehTreeDecoratorAfraidDefinition() {}

};

BEGIN_CLASS_RTTI( CBehTreeDecoratorAfraidDefinition );
PARENT_CLASS( IBehTreeNodeDecoratorDefinition );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// INSTANCE
////////////////////////////////////////////////////////////////////////
class CBehTreeDecoratorAfraidInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
public:
	typedef CBehTreeDecoratorAfraidDefinition Definition;

	CBehTreeDecoratorAfraidInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	Bool Activate() override;
	void Deactivate() override;

private:
};


