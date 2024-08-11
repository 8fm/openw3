#include "build.h"

#include "behTreeNodeFlee.h"

#include "../../common/game/behTreeInstance.h"


IMPLEMENT_ENGINE_CLASS( SBecomeScaredEventData )

// Sending scared events

IBehTreeNodeInstance* CBehTreeNodeSendScaredEventDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

CBehTreeNodeSendScaredEventInstance::CBehTreeNodeSendScaredEventInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: IBehTreeNodeInstance( def, owner, context, parent )	
	, m_becomeScared( def.m_becomeScared )
	, m_scaredTime( def.m_scaredTime.GetVal( context ) )
	, m_scaredTimeMax( def.m_scaredTimeMax.GetVal( context ) )
{

}

void CBehTreeNodeSendScaredEventInstance::Update()
{
	if( m_becomeScared )
	{
		if( m_scaredTimeMax < m_scaredTime )
		{
			m_scaredTimeMax = m_scaredTime;
		}
		Float scaredTime = GEngine->GetRandomNumberGenerator().Get< Float >( m_scaredTime , m_scaredTimeMax );
		SBecomeScaredEventData data( m_owner->GetActionTarget().Get(), scaredTime );		
		m_owner->GetActor()->SignalGameplayEvent( CNAME( AI_BecomeScared ), &data, SBecomeScaredEventData::GetStaticClass() );
	}
	else
	{
		m_owner->GetActor()->SignalGameplayEvent( CNAME( AI_StopBeingScared ) );
	}
	
	Complete( IBehTreeNodeInstance::BTTO_SUCCESS );
}

// Receiving scared events

IBehTreeNodeDecoratorInstance*	CBehTreeNodeReceiveScaredEventDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

CBehTreeNodeReceiveScaredEventInstance::CBehTreeNodeReceiveScaredEventInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: IBehTreeNodeDecoratorInstance( def, owner, context, parent )	
	, m_isScared( false )
	, m_scareTimeout( 0 )
{	
	SBehTreeEvenListeningData eBecome;
	eBecome.m_eventName = CNAME( AI_BecomeScared );
	eBecome.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	context.AddEventListener( eBecome, this );

	SBehTreeEvenListeningData eStop;
	eStop.m_eventName = CNAME( AI_StopBeingScared );
	eStop.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	context.AddEventListener( eStop, this );
}

void CBehTreeNodeReceiveScaredEventInstance::OnDestruction()
{
	SBehTreeEvenListeningData eBecome;
	eBecome.m_eventName = CNAME( AI_BecomeScared );
	eBecome.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	m_owner->RemoveEventListener( eBecome, this );

	SBehTreeEvenListeningData eStop;
	eStop.m_eventName = CNAME( AI_StopBeingScared );
	eStop.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	m_owner->RemoveEventListener( eStop, this );

	Super::OnDestruction();
}

Bool CBehTreeNodeReceiveScaredEventInstance::OnListenedEvent( CBehTreeEvent& e ) 
{
	if( e.m_eventName == CNAME( AI_BecomeScared ) )
	{
		m_isScared = true;
		SBecomeScaredEventData* eventData = e.m_gameplayEventData.Get< SBecomeScaredEventData >();
		m_threatSource = eventData->m_threadSource.Get();
		m_scareTimeout = m_owner->GetLocalTime() + eventData->m_scaredTime;
		m_owner->SetActionTarget( m_threatSource );
		return true;
	}
	else if( e.m_eventName == CNAME( AI_StopBeingScared ) )
	{
		m_isScared = false;
		m_threatSource = NULL;
		return true;
	}
	return false;
}

void CBehTreeNodeReceiveScaredEventInstance::Update()
{
	if( m_scareTimeout < m_owner->GetLocalTime() )
	{
		m_isScared = false;		
	}
	Super::Update();
}

Bool CBehTreeNodeReceiveScaredEventInstance::IsAvailable()
{	
	if( m_isScared )
	{
		return Super::IsAvailable();
	}

	DebugNotifyAvailableFail();
	return false;
}

Int32 CBehTreeNodeReceiveScaredEventInstance::Evaluate()
{
	if ( m_isScared )
	{
		return Super::Evaluate();
	}

	DebugNotifyAvailableFail();
	return -1;
}

void CBehTreeNodeReceiveScaredEventInstance::Deactivate() 
{
	m_isScared = false;
	Super::Deactivate();
}