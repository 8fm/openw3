/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeExplorationQueue.h"

#include "../engine/pathlibMetalinkComponent.h"

#include "aiExplorationParameters.h"
#include "behTreeInstance.h"
#include "queueMetalinkInterface.h"



Bool IBehTreeNodeExplorationQueueDecoratorDefinition::GetInteractionPoint( CBehTreeSpawnContext& context, Vector3& interactionPoint ) const
{
	return context.GetValRef( IAIExplorationTree::GetInteractionPointParamName(), interactionPoint );
}
////////////////////////////////////////////////////////////////////////////
// IBehTreeNodeExplorationQueueDecoratorInstance
////////////////////////////////////////////////////////////////////////////
IAIQueueMetalinkInterface* IBehTreeNodeExplorationQueueDecoratorInstance::GetQueue()
{
	PathLib::IComponent* component = m_metalink.Get();
	if ( component )
	{
		// NOTICE: we do static cast because we check type in constructor
		PathLib::IMetalinkComponent* metalink = static_cast< PathLib::IMetalinkComponent* >( component );
		return metalink->GetAIQueueInterface();
	}

	return NULL;
}

IBehTreeNodeExplorationQueueDecoratorInstance::IBehTreeNodeExplorationQueueDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
{
	CComponent* component = context.GetVal< CComponent* >( IAIExplorationTree::GetMetalinkParamName(), NULL );
	if ( component )
	{
		PathLib::IComponent* pathlibComponent = component->AsPathLibComponent();
		if ( pathlibComponent )
		{
			PathLib::IMetalinkComponent* metalink = pathlibComponent->AsMetalinkComponent();
			if ( metalink )
			{
				if ( metalink->GetAIQueueInterface() )
				{
					m_metalink = pathlibComponent;
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeExplorationQueueRegisterDefinition
////////////////////////////////////////////////////////////////////////////
IBehTreeNodeDecoratorInstance* CBehTreeNodeExplorationQueueRegisterDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeExplorationQueueRegisterInstance
////////////////////////////////////////////////////////////////////////////

Float CBehTreeNodeExplorationQueueRegisterInstance::CalculateWaitingPriority( IAIQueueMetalinkInterface* queue )
{
	CActor* actor = m_owner->GetActor();

	Float distFromInteraction = (actor->GetWorldPositionRef().AsVector3() - m_interactionPoint).Mag();
	Float distPriority = distFromInteraction >= m_maxDistance ? 0.f : (distFromInteraction / m_maxDistance) * m_distancePriority;

	Float time = m_owner->GetLocalTime() - m_registeredAtTime;
	Float timeRatio = time >= m_maxTime ? 1.f : time / m_maxTime;
	Float timePriority = timeRatio * m_timePriority;

	return distPriority + timePriority;
}
CBehTreeNodeExplorationQueueRegisterInstance::CBehTreeNodeExplorationQueueRegisterInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_timePriority( def.m_timePriority )
	, m_distancePriority( def.m_distancePriority )
	, m_maxTime( def.m_maxTime )
	, m_maxDistance( def.m_maxDistance )
	, m_interactionPoint( 0.f, 0.f, 0.f )
{
	
	def.GetInteractionPoint( context, m_interactionPoint );
}

Bool CBehTreeNodeExplorationQueueRegisterInstance::Activate()
{
	IAIQueueMetalinkInterface* queue = GetQueue();
	if ( !queue )
	{
		DebugNotifyActivationFail();
		return false;
	}

	m_registeredAtTime = m_owner->GetLocalTime();
	Float priority = CalculateWaitingPriority( queue );
	CActor* actor = m_owner->GetActor();

	queue->Register( actor, priority );

	if ( !Super::Activate() )
	{
		queue->Unregister( actor );
		DebugNotifyActivationFail();
		return false;
	}

	return true;
}
void CBehTreeNodeExplorationQueueRegisterInstance::Deactivate()
{
	CActor* actor = m_owner->GetActor();
	IAIQueueMetalinkInterface* queue = GetQueue();
	if ( queue && actor )
	{
		queue->Unregister( actor );
	}
	Super::Deactivate();
}

Bool CBehTreeNodeExplorationQueueRegisterInstance::OnEvent(  CBehTreeEvent& e )
{
	if ( e.m_eventName == IAIQueueMetalinkInterface::AIPriorityEventName() )
	{
		SGameplayEventParamFloat* param = e.m_gameplayEventData.Get< SGameplayEventParamFloat >();
		if ( param )
		{
			IAIQueueMetalinkInterface* queue = GetQueue();
			if ( queue )
			{
				param->m_value = CalculateWaitingPriority( queue );
			}
		}
		return false;
	}
	return Super::OnEvent( e );
}

////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeExplorationQueueUseDefinition
////////////////////////////////////////////////////////////////////////////
IBehTreeNodeDecoratorInstance* CBehTreeNodeExplorationQueueUseDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeExplorationQueueUseInstance
////////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeExplorationQueueUseInstance::Activate()
{
	IAIQueueMetalinkInterface* queue = GetQueue();
	if ( !queue )
	{
		DebugNotifyActivationFail();
		return false;
	}

	queue->Lock( true );

	if ( !Super::Activate() )
	{
		queue->Lock( false );
		DebugNotifyActivationFail();
		return false;
	}

	m_locked = true;
	return true;
}
void CBehTreeNodeExplorationQueueUseInstance::Deactivate()
{
	if ( m_locked )
	{

	}
}

Bool CBehTreeNodeExplorationQueueUseInstance::OnEvent( CBehTreeEvent& e )
{
	if ( e.m_eventName == IAIQueueMetalinkInterface::AIReleaseLockEventName() )
	{
		if ( m_locked )
		{
			IAIQueueMetalinkInterface* queue = GetQueue();
			if ( queue )
			{
				queue->Lock( false );
			}
		}
	}
	return false;
}

