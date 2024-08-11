#include "build.h"

#include "reactionSceneEndNode.h"
#include "reactionSceneActor.h"
#include "behTreeInstance.h"

IBehTreeNodeInstance* CBehTreeNodeReactionSceneEndNodeDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

CBehTreeNodeReactionSceneEndNodeInstance::CBehTreeNodeReactionSceneEndNodeInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: IBehTreeNodeInstance( def, owner, context, parent )	
{

}

Bool CBehTreeNodeReactionSceneEndNodeInstance::Activate()
{
	CReactionSceneActorComponent* actorCmp = m_owner->GetActor()->FindComponent< CReactionSceneActorComponent >();	
	actorCmp->FinishRole( false );

	CReactionScene* scene = actorCmp->GetReactionScene();
	if( scene && scene->IfAllRolesFinished() )
	{
		scene->EndScene();
	}

	return true;
}

void CBehTreeNodeReactionSceneEndNodeInstance::Update()
{
	// just wait
	// we can add some idle task here
}