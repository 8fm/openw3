#include "build.h"
#include "behTreeDecoratorSteeringGraph.h"


////////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorSteeringGraphDefinition
////////////////////////////////////////////////////////////////////////////

IBehTreeNodeDecoratorInstance* CBehTreeDecoratorSteeringGraphDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

////////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorSteeringGraphDefinition
////////////////////////////////////////////////////////////////////////////

CBehTreeDecoratorSteeringGraphInstance::CBehTreeDecoratorSteeringGraphInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, CBehTreeSteeringGraphCommonInstance( def, owner, context )
{

}
CBehTreeDecoratorSteeringGraphInstance::~CBehTreeDecoratorSteeringGraphInstance()
{

}

Bool CBehTreeDecoratorSteeringGraphInstance::Activate()
{
	ActivateSteering( m_owner );
	if ( Super::Activate() )
	{
		return true;
	}
	DeactivateSteering( m_owner );
	DebugNotifyActivationFail();
	return false;
}
void CBehTreeDecoratorSteeringGraphInstance::Deactivate()
{
	Super::Deactivate();
	DeactivateSteering( m_owner );
}