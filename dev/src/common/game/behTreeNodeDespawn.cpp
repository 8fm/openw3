#include "build.h"
#include "behTreeNodeDespawn.h"

#include "behTreeInstance.h"

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDespawnDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeInstance* CBehTreeNodeDespawnDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDespawnInstance
////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeDespawnInstance::Activate()
{
	CActor* actor = m_owner->GetActor();
	if ( actor->IsAttached() )
	{
		actor->Destroy();
	}
	return true;
}