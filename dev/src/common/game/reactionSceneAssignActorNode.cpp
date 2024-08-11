#include "build.h"

#include "reactionSceneAssignActorNode.h"
#include "reactionSceneActor.h"
#include "behTreeInstance.h"

IBehTreeNodeInstance* CBehTreeNodeReactionSceneAssignActorNodeDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

CBehTreeNodeReactionSceneAssignActorNodeInstance::CBehTreeNodeReactionSceneAssignActorNodeInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: IBehTreeNodeInstance( def, owner, context, parent )
	, m_roleName( def.m_roleName )
{

}

Bool CBehTreeNodeReactionSceneAssignActorNodeInstance::Activate()
{
	CReactionSceneActorComponent* actorCmp = m_owner->GetActor()->FindComponent< CReactionSceneActorComponent >();
	
	if( actorCmp && !actorCmp->GetRole() )
	{
		CReactionScene* reactionScene = actorCmp->GetReactionScene();
		if( reactionScene && reactionScene->IfNeedsActorToRole( m_roleName ) )
		{
			reactionScene->AssignActorToRole( actorCmp, m_roleName );
			return true;
		}		
	}

	DebugNotifyActivationFail();
	return false;
}

Bool CBehTreeNodeReactionSceneAssignActorNodeInstance::IsAvailable()
{
	CReactionSceneActorComponent* actorCmp = m_owner->GetActor()->FindComponent< CReactionSceneActorComponent >();
	if( !actorCmp || actorCmp->GetRole())
	{
		DebugNotifyAvailableFail();
		return false;
	}

	CReactionScene* reactionScene = actorCmp->GetReactionScene();
	if( !reactionScene || !reactionScene->IfNeedsActorToRole( m_roleName ) || reactionScene->IfAllActrorsCollected() )
	{
		DebugNotifyAvailableFail();
		return false;
	}

	return true;
}