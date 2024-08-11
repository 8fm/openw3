#include "build.h"
#include "behTreeNodeForced.h"

#include "behTreeDynamicNodeEvent.h"
#include "behTreeInstance.h"



////////////////////////////////////////////////////////////////////////
// SForcedBehaviorEventData
////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( SForcedBehaviorEventData );

CName SForcedBehaviorEventData::GetEventName()
{
	return CNAME( AI_Load_Forced );
}
CName SForcedBehaviorEventData::GetCancelEventName()
{
	return CNAME( AI_Forced_Cancel );
}
////////////////////////////////////////////////////////////////////////
// CBehTreeNodeBaseForcedBehaviorDefinition
///////////////////////////////////////////////////////////////////////
IAITree* CBehTreeNodeBaseForcedBehaviorDefinition::GetBaseDefinition( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const
{
	return context.GetVal< IAITree* >( CNAME( startingBehavior ), NULL );
}



////////////////////////////////////////////////////////////////////////
// CBehTreeNodeBaseForcedBehaviorInstance
////////////////////////////////////////////////////////////////////////
CBehTreeNodeBaseForcedBehaviorInstance::CBehTreeNodeBaseForcedBehaviorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_overridenPriority( Priority( context.GetVal( CNAME( startingBehaviorPriority ), 100 ) ) )
	, m_cancelEventName( def.GetCancelEventName() )
	, m_interruptionLevel( EAIForcedBehaviorInterruptionLevel::High )
	, m_cancelBehaviorRequested( false )
{
	SBehTreeEvenListeningData eventData;
	eventData.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	eventData.m_eventName = m_cancelEventName;
	context.AddEventListener( eventData, this );
}
void CBehTreeNodeBaseForcedBehaviorInstance::OnDestruction()
{	
	SBehTreeEvenListeningData eventData;
	eventData.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	eventData.m_eventName = m_cancelEventName;
	m_owner->RemoveEventListener( eventData, this );

	Super::OnDestruction();
}

void CBehTreeNodeBaseForcedBehaviorInstance::Update()
{
	Super::Update();

	if ( m_cancelBehaviorRequested )
	{
		if ( m_isActive )
		{
			if ( m_childNode && m_childNode->Interrupt() )
			{
				m_childNode->RegisterForDeletion();
				m_childNode = nullptr;

				m_cancelBehaviorRequested = false;

				if ( m_isActive )
				{
					Complete( BTTO_FAILED );
				}
			}
		}
		else
		{
			m_cancelBehaviorRequested = false;
		}
	}
}

Bool CBehTreeNodeBaseForcedBehaviorInstance::OnEvent( CBehTreeEvent& e )
{
	if ( e.m_eventName == CNAME( AI_CanTeleportOutOfSceneArea ) )
	{
		SGameplayEventParamInt* params = e.m_gameplayEventData.Get< SGameplayEventParamInt >();
		if ( params )
		{
			params->m_value = 0;
		}
		return false;
	}

	return Super::OnEvent( e );
}

Bool CBehTreeNodeBaseForcedBehaviorInstance::OnListenedEvent( CBehTreeEvent& e )
{
	if ( e.m_eventName == m_eventName )
	{
		{
			SForcedBehaviorEventData* eventData  = e.m_gameplayEventData.Get< SForcedBehaviorEventData >();
			if ( eventData )
			{
				eventData->m_interrupt |= eventData->m_interruptionLevel > m_interruptionLevel;
				Bool retVal = HandleSpawnEvent( *eventData );
				if ( eventData->m_isHandled == SBehTreeDynamicNodeEventData::OUTPUT_HANDLED )
				{
					m_overridenPriority = eventData->m_overridenPriority;
					eventData->m_uniqueActionId = ++m_currentUniqueBehaviorId;
					m_interruptionLevel = eventData->m_interruptionLevel;
				}
				return retVal;
			}
		}
		{
			SBehTreeDynamicNodeEventData* eventData = e.m_gameplayEventData.Get< SBehTreeDynamicNodeEventData >();
			if ( eventData )
			{
				Bool retVal = HandleSpawnEvent( *eventData );
				if ( eventData->m_isHandled == SBehTreeDynamicNodeEventData::OUTPUT_HANDLED )
				{
					m_overridenPriority = -1;
					m_interruptionLevel = EAIForcedBehaviorInterruptionLevel::High;
				}
				return retVal;
			}
		}
		{
			SDynamicNodeSaveStateRequestEventData* saveEventData = e.m_gameplayEventData.Get< SDynamicNodeSaveStateRequestEventData >();
			if ( saveEventData )
			{
				HandleSaveEvent( *saveEventData );
				return false;
			}
		}

		
		return false;
	}
	else if ( e.m_eventName == m_cancelEventName )
	{
		// cancel behavior
		SForcedBehaviorEventData* eventData  = e.m_gameplayEventData.Get< SForcedBehaviorEventData >();
		if ( eventData )
		{
			if ( m_currentUniqueBehaviorId == eventData->m_uniqueActionId || eventData->m_uniqueActionId == SForcedBehaviorEventData::IgnoreActionId )
			{
				// check if behavior can be cancelled
				if ( m_isActive )
				{
					if ( !eventData->m_interrupt || !m_childNode->Interrupt() )
					{
						if ( !eventData->m_fireEventAndForget )
						{
							m_cancelBehaviorRequested = true;
						}

						eventData->m_isHandled = SBehTreeDynamicNodeEventData::OUTPUT_DELAYED;
						return false;
					}
				}

				if ( m_childNode )
				{
					m_childNode->RegisterForDeletion();
					m_childNode = nullptr;
				}

				if ( m_isActive )
				{
					Complete( BTTO_FAILED );
				}

				eventData->m_isHandled = SBehTreeDynamicNodeEventData::OUTPUT_HANDLED;
			}
			return false;											// NO DIRTY MARKING IS NEEDED
		}
	}
	else if ( e.m_eventName == CNAME( OnPoolRequest ) )
	{
		HandlePoolRequest();
	}
	return false;
}

////////////////////////////////////////////////////////////////////
//! Evaluation
Bool CBehTreeNodeBaseForcedBehaviorInstance::IsAvailable()
{
	if ( m_childNode )
	{
		return true;
	}

	DebugNotifyAvailableFail();
	return false;
}
Int32 CBehTreeNodeBaseForcedBehaviorInstance::Evaluate()
{
	if ( m_childNode )
	{
		Int32 toRet =  ( m_overridenPriority > 0 ? m_overridenPriority : m_priority );
		if( toRet <= 0 )
		{
			DebugNotifyAvailableFail();
		}

		return toRet;
	}

	DebugNotifyAvailableFail();
	return -1;
}
void CBehTreeNodeBaseForcedBehaviorInstance::OnSubgoalCompleted( eTaskOutcome outcome )
{
	if ( m_childNode )
	{
		m_childNode->RegisterForDeletion();
		m_childNode = nullptr;
	}

	Super::OnSubgoalCompleted( outcome );
}
////////////////////////////////////////////////////////////////////////
// CBehTreeNodeBaseForcedBehaviorDefinition
////////////////////////////////////////////////////////////////////////
CName CBehTreeNodeForcedBehaviorDefinition::GetEventName( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const
{
	return CNAME( AI_Load_Forced );
}
CName CBehTreeNodeForcedBehaviorDefinition::GetCancelEventName( ) const
{
	return CNAME( AI_Forced_Cancel );
}
IBehTreeNodeInstance* CBehTreeNodeForcedBehaviorDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}
////////////////////////////////////////////////////////////////////////
// CBehTreeNodeForcedBehaviorInstance
////////////////////////////////////////////////////////////////////////
CBehTreeNodeForcedBehaviorInstance::CBehTreeNodeForcedBehaviorInstance(  const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent  )
	: CBehTreeNodeBaseForcedBehaviorInstance( def, owner, context, parent )
{
}
