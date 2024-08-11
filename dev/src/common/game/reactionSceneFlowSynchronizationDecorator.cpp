#include "build.h"

#include "reactionSceneFlowSynchronizationDecorator.h"
#include "reactionSceneActor.h"
#include "behTreeInstance.h"

IBehTreeNodeDecoratorInstance* CBehTreeNodeReactionSFlowSynchroDecoratorDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{		
	return new Instance( *this, owner, context, parent );
}

CBehTreeNodeReactionSFlowSynchroDecoratorInstance::CBehTreeNodeReactionSFlowSynchroDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: CBehTreeNodeConditionInstance( def, owner, context, parent )
	, m_allowedRole( def.m_allowedRole )
	, m_isBlocikng( def.m_isBlocikng )
{
}

Bool CBehTreeNodeReactionSFlowSynchroDecoratorInstance::ConditionCheck()
{
	CReactionSceneActorComponent* actorCmp = m_owner->GetActor()->FindComponent< CReactionSceneActorComponent >();
	Bool condResult = ( actorCmp && actorCmp->GetRole() == m_allowedRole );// ? true : false;		

	return condResult;
	
}