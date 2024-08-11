#include "build.h"
#include "behTreeDecoratorPoke.h"

#include "../core/gameSave.h"

#include "behTreeInstance.h"

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDurationDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeDecoratorInstance* CBehTreeNodePokeDecoratorDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDurationInstance
////////////////////////////////////////////////////////////////////////
CBehTreeNodePokeDecoratorInstance::CBehTreeNodePokeDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: IBehTreeNodeDecoratorInstance( def, owner, context, parent )
	, m_pokeEvent( def.m_pokeEvent.GetVal( context ) )
	, m_acceptPokeOnlyIfActive( def.m_acceptPokeOnlyIfActive.GetVal( context ) )
	, m_poked( false )
{
	SBehTreeEvenListeningData e;
	e.m_eventName = m_pokeEvent;
	e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	context.AddEventListener( e, this );
}
void CBehTreeNodePokeDecoratorInstance::OnDestruction()
{
	SBehTreeEvenListeningData e;
	e.m_eventName = m_pokeEvent;
	e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	m_owner->RemoveEventListener( e, this );

	Super::OnDestruction();
}

void CBehTreeNodePokeDecoratorInstance::Deactivate()
{
	m_poked = false;
	Super::Deactivate();
}

void CBehTreeNodePokeDecoratorInstance::Update()
{
	if ( m_poked && m_child->Interrupt() )
	{
		Complete( BTTO_SUCCESS );
		return;
	}
	Super::Update();
}


Bool CBehTreeNodePokeDecoratorInstance::OnListenedEvent( CBehTreeEvent& e )
{
	if ( e.m_eventName == m_pokeEvent && (!m_acceptPokeOnlyIfActive || m_isActive) )
	{
		m_poked = true;
		SGameplayEventParamInt* data = e.m_gameplayEventData.Get< SGameplayEventParamInt >();
		if ( data)
		{
			data->m_value = 1;
		}
	}
	return false;
}
Bool CBehTreeNodePokeDecoratorInstance::IsSavingState() const
{
	return m_poked;
}
void CBehTreeNodePokeDecoratorInstance::SaveState( IGameSaver* writer )
{
	writer->WriteValue< Bool >( CNAME( poked ), m_poked );
}
Bool CBehTreeNodePokeDecoratorInstance::LoadState( IGameLoader* reader )
{
	reader->ReadValue< Bool >( CNAME( poked ), m_poked );
	return true;
}