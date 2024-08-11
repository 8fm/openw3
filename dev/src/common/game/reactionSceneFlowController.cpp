#include "build.h"

#include "reactionSceneFlowController.h"
#include "reactionSceneActor.h"
#include "behTreeInstance.h"
#include "reactionSceneFlowSynchronizationDecorator.h"

IBehTreeNodeCompositeInstance* CBehTreeNodeReactionSceneFlowControllerDefinition::SpawnCompositeInstanceInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{		
	return new Instance( *this, owner, context, parent );
}

CBehTreeNodeReactionSceneFlowControllerInstance::CBehTreeNodeReactionSceneFlowControllerInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: CBehTreeNodeSequenceCheckAvailabilityInstance( def, owner, context, parent )	
{}

Bool CBehTreeNodeReactionSceneFlowControllerInstance::IsAvailable()
{
	CReactionSceneActorComponent* actorCmp = m_owner->GetActor()->FindComponent< CReactionSceneActorComponent >();
	Bool toRet = actorCmp && actorCmp->GetRole();
	
	if( !toRet )
	{
		DebugNotifyAvailableFail();
	}

	return toRet;
}

void CBehTreeNodeReactionSceneFlowControllerInstance::Update()
{
	if( m_reactionScene.Get() && m_reactionScene.Get()->IfAllActrorsCollected() )
	{
		m_reactionScene.Get()->InitializeActionTargets();

		if( m_delayedActivation )
		{
			if( m_activeChild == INVALID_CHILD )
			{
				if( !FindAndActivateStartNode() )
				{
					Complete( IBehTreeNodeInstance::BTTO_FAILED );
				}
			}
			else
			{
			
				if( m_reactionScene.Get() && m_reactionScene.Get()->CanBeActivated( m_activeChild ) )
				{
					m_delayedActivation = false;
					if ( !m_children[ m_activeChild ]->Activate() )
					{
						if ( !m_continueSequenceOnChildFailure )
						{
							Complete( IBehTreeNodeInstance::BTTO_FAILED );
						}
					}				
				}
			}
		}
		else
		{
			//don't update if waiting for activation
			//update will be called in nex tick
			m_reactionScene.Get()->InitializeActionTargets();
			Super::Update();
		}		
	}

}

Bool CBehTreeNodeReactionSceneFlowControllerInstance::FindAndActivateStartNode()
{
	m_activeChild = 0;

	for ( ;m_activeChild < m_children.Size(); ++m_activeChild )
	{
		if ( m_children[ m_activeChild ]->IsAvailable() )
		{
			if( m_reactionScene.Get() && m_reactionScene.Get()->CanBeActivated( m_activeChild ) )
			{			
				if ( m_children[ m_activeChild ]->Activate() )
				{
					m_isActive = true;
					return true;
				}
				else if ( !m_continueSequenceOnChildFailure )
				{
					return false;
				}
			}
			else
			{
				m_delayedActivation = true;
				m_isActive = true;
				return true;
			}
		}
	}
	return false;
}

Bool CBehTreeNodeReactionSceneFlowControllerInstance::Activate()
{
	m_delayedActivation = false;
	CReactionSceneActorComponent* actorCmp = m_owner->GetActor()->FindComponent< CReactionSceneActorComponent >();
	if( actorCmp )
	{
		m_reactionScene = actorCmp->GetReactionScene();
		ASSERT( m_reactionScene.Get() && "Reaction scene is not defined" );
		if( m_reactionScene.Get() && !m_reactionScene.Get()->IfBlockingBrancesInitialized() )
		{
			CReactionScene::SceneBranches blockingBranches = 0;
			for ( Uint32 i = 0; i < m_children.Size(); ++i )
			{
				CBehTreeNodeReactionSFlowSynchroDecoratorInstance* synchroDecorator = m_children[ i ]->AsCBehTreeNodeReactionSFlowSynchroDecorator();

				//CBehTreeNodeReactionSFlowSynchroDecoratorInstance* synchroDecorator = dynamic_cast< CBehTreeNodeReactionSFlowSynchroDecoratorInstance* >( m_children[ i ] );
				if( synchroDecorator && synchroDecorator->IsBlocking() )
				{
					blockingBranches |= 1 << i;
				}
			}
			m_reactionScene.Get()->InitializeBlockingBranches( blockingBranches );
		}
	}

	if( !m_reactionScene.Get() )
	{
		ASSERT( m_reactionScene.Get() && "Reaction scene is not defined" );
		Complete( IBehTreeNodeInstance::BTTO_FAILED );
		DebugNotifyActivationFail();
		return false;
	}

	if( m_reactionScene.Get()->IfAllActrorsCollected() )
	{
		m_reactionScene.Get()->InitializeActionTargets();

		if( FindAndActivateStartNode() )
		{
			return true;
		}
		else
		{
			DebugNotifyActivationFail();
			return false;
		}
	}
	else
	{
		m_activeChild = INVALID_CHILD;
		m_delayedActivation = true;
		m_isActive = true;
		return true;
	}		

	//Super::Activate();
}

void CBehTreeNodeReactionSceneFlowControllerInstance::OnSubgoalCompleted( eTaskOutcome outcome )
{
	if( m_reactionScene.Get() )
	{
		m_reactionScene.Get()->MarkBlockingBranchAsExecuted( m_activeChild );
	}
	if ( outcome == BTTO_FAILED && !m_continueSequenceOnChildFailure )
	{
		m_activeChild = INVALID_CHILD;
		Complete( BTTO_FAILED );
		return;
	}
	for ( ++m_activeChild; m_activeChild < m_children.Size(); ++m_activeChild )
	{
		if ( m_children[ m_activeChild ]->IsAvailable() )
		{
			if( m_reactionScene.Get() && m_reactionScene.Get()->CanBeActivated( m_activeChild ) )
			{
				if ( m_children[ m_activeChild ]->Activate() )
				{
					return;
				}
				else if ( !m_continueSequenceOnChildFailure )
				{
					m_activeChild = INVALID_CHILD;
					Complete( BTTO_FAILED );
					return;
				}
			}
			else
			{
				m_delayedActivation = true;
				return;
			}
		}
	}
	m_activeChild = INVALID_CHILD;
	Complete( BTTO_SUCCESS );
}