#include "build.h"

#include "behTreeDecoratorAfraidDefinition.h"
#include "behTreeDecorator.h"


BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeDecoratorAfraidDefinition )


	CBehTreeDecoratorAfraidInstance::CBehTreeDecoratorAfraidInstance(const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent)
	: Super( def, owner, context, parent )
{
}


Bool CBehTreeDecoratorAfraidInstance::Activate()
{
	Bool ret = Super::Activate();
	if( ret )
	{
		m_child->GetOwner()->GetNPC()->SetAfraid(true);
	}

	return ret;
}

void CBehTreeDecoratorAfraidInstance::Deactivate()
{
	m_child->GetOwner()->GetNPC()->SetAfraid(false);
}


