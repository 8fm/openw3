#include "build.h"
#include "behTreeNodeFinishAnimations.h"
#include "behTreeNode.h"
#include "behTreeInstance.h"
#include "actorActionWork.h"
#include "..\engine\behaviorGraphStack.h"

IBehTreeNodeInstance* CBehTreeNodeFinishAnimationsDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

void CBehTreeNodeFinishAnimationsInstance::Update()
{
	CActor* actor = m_owner->GetActor();
	ASSERT( actor );
	if( actor->GetRootAnimatedComponent() )
	{	
		CBehaviorGraphStack * stack = actor->GetRootAnimatedComponent()->GetBehaviorStack();
		if ( stack )
		{
			IBehaviorGraphSlotInterface slot;
			if ( stack->GetSlot( CNAME( NPC_ANIM_SLOT ), slot) )
			{
				if ( slot.GetAnimation() )
				{
					return;
				}
			}
		}
	}
	Complete(BTTO_SUCCESS);
}

IBehTreeNodeInstance* CBehTreeNodeBreakAnimationsDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

void CBehTreeNodeBreakAnimationsInstance::Update()
{
	CActor* actor = m_owner->GetActor();
	CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
	if ( mac )
	{
		CBehaviorGraphStack* stack = mac->GetBehaviorStack();
		stack->StopSlotAnimation( CNAME( NPC_ANIM_SLOT ) );
	}

	Complete( BTTO_SUCCESS );
}
