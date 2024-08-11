#include "build.h"

#include "behTreeNodeConditionReactionEvent.h"
#include "behTreeInstance.h"
#include "..\engine\game.h"
#include "behTreeReactionData.h"
#include "reactionSceneActor.h"

IBehTreeNodeDecoratorInstance*	CBehTreeNodeConditionReactionEventDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

CBehTreeNodeConditionReactionEventInstance::CBehTreeNodeConditionReactionEventInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: CBehTreeNodeConditionInstance( def, owner, context, parent )
	, m_reactionData( NULL )
	, m_counterData( owner, CNAME( ReactionCounter ) )
	, m_eventName( def.m_eventName.GetVal( context ) )
	, m_cooldownDistanceSq( def.m_cooldownDistance.GetVal( context ) )
	, m_cooldownTimeout( def.m_cooldownTimeout.GetVal( context ) )
	, m_nextActivationTime( 0.0f )
	, m_setActionTarget( true )
	, m_dontSetActionTargetEdit( def.m_dontSetActionTargetEdit.GetVal( context ) )	
	, m_reactionTreeComplated( false )
	, m_counterTicket( false )
	, m_workData( owner )
{
	m_cooldownDistanceSq *= m_cooldownDistanceSq;

	SBehTreeEvenListeningData e;
	e.m_eventName = m_eventName;
	e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	context.AddEventListener( e, this );
}

void CBehTreeNodeConditionReactionEventInstance::OnDestruction()
{
	SBehTreeEvenListeningData e;
	e.m_eventName = m_eventName;
	e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	m_owner->RemoveEventListener( e, this );
	Super::OnDestruction();
}

Bool CBehTreeNodeConditionReactionEventInstance::IsAvailable()
{
	if ( !Super::IsAvailable() )
	{
		if ( m_counterTicket )
		{
			m_counterData->ModifyCounterNoNegative( -1 );
			m_counterTicket = false;
		}
		return false;
	}
	return true;
}

Int32 CBehTreeNodeConditionReactionEventInstance::Evaluate()
{
	Int32 r = Super::Evaluate();
	if ( m_counterTicket && r <= 0 )
	{
		m_counterData->ModifyCounterNoNegative( -1 );
		m_counterTicket = false;
	}
	return r;
}

Bool CBehTreeNodeConditionReactionEventInstance::Activate()
{
	CBehTreeReactionEventData* reactionData = m_reactionData.Get();
	if( reactionData )
	{
		//m_debugEventName = reactionData->GetEventName();
		if( m_setActionTarget && !m_dontSetActionTargetEdit )
		{
			m_owner->SetActionTarget( reactionData->GetInvoker() );
		}		

		CReactionSceneActorComponent* scenaActor = m_owner->GetActor()->FindComponent< CReactionSceneActorComponent >();
		if( scenaActor && reactionData->GetReactionScene() )
		{
			scenaActor->SetReactionScene( reactionData->GetReactionScene() );
		}
		
		Bool superActivated = Super::Activate();
		if( !superActivated && m_counterTicket )
		{
			m_counterData->ModifyCounterNoNegative( -1 );
			m_counterTicket = false;
		}

		return superActivated;
	}

	DebugNotifyActivationFail();
	return false;
}
void CBehTreeNodeConditionReactionEventInstance::Deactivate()
{
	Super::Deactivate();
	m_nextActivationTime = m_owner->GetLocalTime() + m_cooldownTimeout;
	if( m_counterTicket )
	{
		m_counterData->ModifyCounterNoNegative( -1 );
		m_counterTicket = false;
	}

	CReactionSceneActorComponent* scenaActor = m_owner->GetActor()->FindComponent< CReactionSceneActorComponent >();
	if( scenaActor )
	{
		scenaActor->LeaveScene( );
	}	

	if( m_reactionData )
	{
		m_reactionData->IncrementRecipientCounter();
	}

	m_reactionData = nullptr;
}


Bool CBehTreeNodeConditionReactionEventInstance::ConditionCheck()
{
	 CBehTreeReactionEventData* reactionData = m_reactionData.Get();
	 // Reaction data tests
	 if( reactionData && !reactionData->HasExpired() )
	 {
		 CActor* actor = m_owner->GetActor();
		 const Vector& pos = actor->GetWorldPositionRef();
		 if ( m_isActive || reactionData->IsInRange( pos ) )
		 {
			 CEntity* invoker = reactionData->GetInvoker();
			 // Reaction event tests
			 if ( m_nextActivationTime <= m_owner->GetLocalTime() || ( invoker && m_cooldownDistanceSq <= invoker->GetWorldPositionRef().DistanceSquaredTo( pos ) ) )
			 {
				 return true;
			 }
		 }

	}
	
	return false;
}

Bool CBehTreeNodeConditionReactionEventInstance::OnListenedEvent( CBehTreeEvent& e )
{
	CBehTreeWorkData* workData = m_workData.Get();
	if( workData )
	{
		if( workData->GetIgnoreHardReactions() || workData->IsInImmediateActivation( m_owner ) )
		{
			return false;
		}
	}

	if ( e.m_gameplayEventData.m_customDataType == CBehTreeReactionEventData::GetStaticClass() )
	{
		CBehTreeReactionEventData* reactionData = static_cast< CBehTreeReactionEventData* >( e.m_gameplayEventData.m_customData );
		if( reactionData )
		{
			//m_debugEventName = reactionData->GetEventName();
			CBehTreeReactionEventData* currentReactionData = m_reactionData.Get();

			if( currentReactionData && !currentReactionData->GetInvoker() )
			{
				currentReactionData = nullptr;
			}

			if( currentReactionData )
			{
				// Another reaction of the same type
				if( currentReactionData != reactionData )
				{
					// If closer to the other invoker, change reaction
					const Vector& actorPos = m_owner->GetActor()->GetPosition();
					Float distToOldInvoker = currentReactionData->GetInvoker()->GetPosition().DistanceSquaredTo( actorPos );
					Float distToNewInvoker = reactionData->GetInvoker()->GetPosition().DistanceSquaredTo( actorPos );
					if( distToNewInvoker < distToOldInvoker )
					{
						m_reactionData = reactionData;
					}
				}
				else if( !m_isActive )
				{
					if( !m_counterTicket )
					{
						m_counterData->ModifyCounter( 1 );
						m_counterTicket = true;
					}
				}
			}
			// New reaction or previously cancelled
			else
			{
				m_reactionData = reactionData;
				if( ConditionCheck() )
				{
					m_reactionData.Get()->ReportRecipient();
					if( !m_counterTicket )
					{
						m_counterData->ModifyCounter( 1 );
						m_counterTicket = true;
					}
					return true;
				}
				m_reactionData = nullptr;
			}
			return !m_isActive;
		}
	}

	return false;
}
