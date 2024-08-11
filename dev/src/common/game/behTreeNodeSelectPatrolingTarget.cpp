#include "build.h"

#include "behTreeNodeSelectPatrolingTarget.h"
#include "behTreeInstance.h"
#include "patrolPointComponent.h"
#include "../engine/tagManager.h"

IBehTreeNodeDecoratorInstance* CBehTreeNodeSelectPatrolingTargetDecoratorDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

Bool CBehTreeNodeSelectPatrolingTargetDecoratorInstance::Activate()
{
	CTagManager* tagMgr = GGame->GetActiveWorld()->GetTagManager();
	CNode* target = tagMgr->GetTaggedNodeClosestTo( m_pointsGroupTag, GetOwner()->GetActor()->GetWorldPosition() );
	if ( target )
	{
		m_owner->SetActionTarget( target );		
		if ( !Super::Activate() )
		{			
			m_owner->SetActionTarget( NULL );
			DebugNotifyActivationFail();
			return false;
		}

		m_currentTarget =  Cast< CEntity >( target )->FindComponent< CPatrolPointComponent >( );
		return true;
	}

	DebugNotifyActivationFail();
	return false;
}