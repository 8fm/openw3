#include "build.h"

#include "reactionSceneAssignments.h"
#include "reactionSceneActor.h"
#include "behTreeInstance.h"

IBehTreeNodeCompositeInstance* CBehTreeNodeReactionSceneAssignmentsDefinition::SpawnCompositeInstanceInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{		
	return new Instance( *this, owner, context, parent );
}

CBehTreeNodeReactionSceneAssignmentsInstance::CBehTreeNodeReactionSceneAssignmentsInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: CBehTreeNodeEvaluatingFirstAvailableSelector( def, owner, context, parent )	
{}

Bool CBehTreeNodeReactionSceneAssignmentsInstance::IsAvailable()
{	
	CReactionSceneActorComponent* actorCmp = m_owner->GetActor()->FindComponent< CReactionSceneActorComponent >();

	if( actorCmp && !actorCmp->GetRole() )
	{
		CReactionScene* reactionScene = actorCmp->GetReactionScene();
		if( reactionScene && !reactionScene->IfAllActrorsCollected() )
		{
			return Super::IsAvailable();
		}
	}

	DebugNotifyAvailableFail();
	return false;
}