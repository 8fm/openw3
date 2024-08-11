#include "build.h"
#include "behTreeConditionIsDead.h"

#include "../../common/core/gameSave.h"

#include "../../common/game/behTreeInstance.h"

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionIsDeadDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeDecoratorInstance*	CBehTreeNodeConditionIsDeadDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

////////////////////////////////////////////////////////////////////////
// CBehTreeConditionIsDeadInstance
////////////////////////////////////////////////////////////////////////
CBehTreeConditionIsDeadInstance::CBehTreeConditionIsDeadInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: CBehTreeNodeConditionInstance( def, owner, context, parent )
	, m_isDead( false )
{
	SBehTreeEvenListeningData eventListenerDeath;
	eventListenerDeath.m_eventName = CNAME( OnDeath );
	eventListenerDeath.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	context.AddEventListener( eventListenerDeath, this );

	SBehTreeEvenListeningData eventListenerRevive;
	eventListenerRevive.m_eventName = CNAME( OnRevive );
	eventListenerRevive.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	context.AddEventListener( eventListenerRevive, this );
}
void CBehTreeConditionIsDeadInstance::OnDestruction()
{
	SBehTreeEvenListeningData eventListenerDeath;
	eventListenerDeath.m_eventName = CNAME( OnDeath );
	eventListenerDeath.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	m_owner->RemoveEventListener( eventListenerDeath, this );

	SBehTreeEvenListeningData eventListenerRevive;
	eventListenerRevive.m_eventName = CNAME( OnRevive );
	eventListenerRevive.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	m_owner->RemoveEventListener( eventListenerRevive, this );

	Super::OnDestruction();
}


Bool CBehTreeConditionIsDeadInstance::ConditionCheck()
{
	return m_isDead;
}

Bool CBehTreeConditionIsDeadInstance::OnListenedEvent( CBehTreeEvent& e )
{
	if ( e.m_eventType == BTET_GameplayEvent )
	{
		if ( e.m_eventName == CNAME( OnDeath ) )
		{
			m_isDead = true;
			return true;
		}
		else if ( e.m_eventName == CNAME( OnRevive ) )
		{
			m_isDead = false;
			return true;
		}
	}
	return Super::OnEvent( e );
}

Bool CBehTreeConditionIsDeadInstance::IsSavingState() const
{
	return m_isDead;
}
void CBehTreeConditionIsDeadInstance::SaveState( IGameSaver* writer )
{
	writer->WriteValue< Bool >( CNAME( dead ), m_isDead );
}
Bool CBehTreeConditionIsDeadInstance::LoadState( IGameLoader* reader )
{
	reader->ReadValue< Bool >( CNAME( dead ), m_isDead );
	return true;
}