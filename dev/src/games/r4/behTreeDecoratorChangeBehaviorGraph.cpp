#include "build.h"
#include "behTreeDecoratorChangeBehaviorGraph.h"
#include "../../common/game/behTreeInstance.h"
#include "../../common/engine/behaviorGraphStack.h"
#include "../../common/core/dataError.h"
#include "../../common/engine/utils.h"

IBehTreeNodeDecoratorInstance* CBehTreeDecoratorChangeBehaviorGraphDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const 
{
	if ( owner->GetNPC() == nullptr )
	{
		DATA_HALT( DES_Minor, CResourceObtainer::GetResource( owner->GetActor() ), TXT( "BehTree" ), TXT( "ChangeBehaviorGraph node works only for CNewNPC entities" ) );
	}
	return new Instance( *this, owner, context, parent );
}

Bool CBehTreeDecoratorChangeBehaviorGraphInstance::AsyncLoadGraph()
{
	// Currently all graphs are loaded at entity startup! So we only check here if graphs are available.


	CActor* actor = m_owner->GetActor();
	if ( !actor )
	{
		return false;
	}
	CAnimatedComponent* animComponent = actor->GetRootAnimatedComponent();
	if ( !animComponent )
	{
		return false;
	}

	CBehaviorGraphStack* stack = animComponent->GetBehaviorStack();
	if ( !m_behGraphNameActivate.Empty() )
	{
		 if ( m_invalidActivationGraph || !stack->IsGraphAvailable( m_behGraphNameActivate ) )
		 {
			 m_invalidActivationGraph = true;

			if ( !m_activateIfBehaviorUnavailable )
			{
				return false;
			}
		}
	}

	if ( !m_behGraphNameDeactivate.Empty() )
	{
		if ( m_invalidDeactivationGraph || !stack->IsGraphAvailable( m_behGraphNameDeactivate ) )
		{
			m_invalidDeactivationGraph = true;

			if ( !m_activateIfBehaviorUnavailable )
			{
				return false;
			}
		}
	}

	return true;
}

Bool CBehTreeDecoratorChangeBehaviorGraphInstance::ActivateGraph( CName graph )
{
	CActor* actor = m_owner->GetActor();
	if( !actor )
	{
		return false;
	}

	CAnimatedComponent* animComponent = actor->GetRootAnimatedComponent();
	if( ! animComponent )
	{
		return false;
	}

	CBehaviorGraphStack* stack = animComponent->GetBehaviorStack();
	if( !stack )
	{
		return false;
	}

	if ( !stack->ActivateBehaviorInstances( graph ) && !m_activateIfBehaviorUnavailable )
	{
		return false;
	}

	return true;
}

Bool CBehTreeDecoratorChangeBehaviorGraphInstance::IsAvailable()
{
	if ( !AsyncLoadGraph() )
	{
		DebugNotifyAvailableFail();
		return false;
	}

	return Super::IsAvailable();
}

Int32 CBehTreeDecoratorChangeBehaviorGraphInstance::Evaluate()
{
	if ( !AsyncLoadGraph() )
	{
		DebugNotifyAvailableFail();
		return -1;
	}

	return Super::Evaluate();
}

Bool CBehTreeDecoratorChangeBehaviorGraphInstance::Activate()
{
	if ( !AsyncLoadGraph() )
	{
		DebugNotifyActivationFail();
		return false;
	}

	if ( m_behGraphNameActivate && !m_invalidActivationGraph )
	{
		if ( !ActivateGraph( m_behGraphNameActivate ) )
		{
			DebugNotifyActivationFail();
			return false;
		}
	}
	
	if ( !Super::Activate() )
	{
		if ( m_behGraphNameDeactivate && !m_invalidDeactivationGraph )
		{
			ActivateGraph( m_behGraphNameDeactivate );
		}

		DebugNotifyActivationFail();
		return false;
	}

	return true;
}

void CBehTreeDecoratorChangeBehaviorGraphInstance::Deactivate()
{
	if ( m_behGraphNameDeactivate && !m_invalidDeactivationGraph )
	{
		ActivateGraph( m_behGraphNameDeactivate );
	}
	Super::Deactivate();
}
