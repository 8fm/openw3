/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeTicketManager.h"

#include "r4BehTreeInstance.h"
#include "behTreeTicketAlgorithm.h"


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeTicketDecoratorDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeDecoratorInstance*	CBehTreeNodeCombatTicketManagerDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeTicketDecoratorInstance
////////////////////////////////////////////////////////////////////////
CBehTreeNodeCombatTicketManagerInstance::CBehTreeNodeCombatTicketManagerInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_ticketsCount( Uint16( def.m_ticketsCount.GetVal( context ) ) )
	, m_importanceUpdateDelay( def.m_importanceUpdateDelay )
	, m_nextImportanceUpdate( 0.f )
	, m_ticketAlgorithm( def.m_ticketAlgorithm ? def.m_ticketAlgorithm->SpawnInstance( owner, context ) : NULL )
{
	SBehTreeEvenListeningData e;
	e.m_eventName = CNAME( AI_ForceImmediateTicketImportanceUpdate );
	e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	context.AddEventListener( e, this );
}

void CBehTreeNodeCombatTicketManagerInstance::OnDestruction()
{
	SBehTreeEvenListeningData e;
	e.m_eventName = CNAME( AI_ForceImmediateTicketImportanceUpdate );
	e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	m_owner->RemoveEventListener( e, this );

	Super::OnDestruction();
}

void CBehTreeNodeCombatTicketManagerInstance::ComputeTicketImportance( Float& importance, Uint16& count )
{
	importance = 100.f;
	count = m_ticketsCount;
	if ( m_ticketAlgorithm )
	{
		importance = m_ticketAlgorithm->CalculateTicketImportance( this, *m_ticket, count );
	}
}
void CBehTreeNodeCombatTicketManagerInstance::ForceImportanceUpdate()
{
	m_nextImportanceUpdate = 0.f;
}
void CBehTreeNodeCombatTicketManagerInstance::DelayImportanceUpdate()
{
	m_nextImportanceUpdate = m_owner->GetLocalTime() + m_importanceUpdateDelay * GEngine->GetRandomNumberGenerator().Get< Float >( 0.5f , 1.5f );
}

void CBehTreeNodeCombatTicketManagerInstance::Update()
{
	CBehTreeTicketData* ticket = &(*m_ticket);
	if ( ticket->HasTicket() )
	{
		if ( m_nextImportanceUpdate < m_owner->GetLocalTime() )
		{
			DelayImportanceUpdate();

			Float importance;
			Uint16 ticketsCount;
			ComputeTicketImportance( importance, ticketsCount );

			if ( !ticket->UpdateImportance( importance ) )
			{
				ticket->NotifyOfTicketLost();
			}
		}
	}
	else if ( ticket->HasPendingRequest() )
	{
		Float importance;
		Uint16 ticketsCount;
		ComputeTicketImportance( importance, ticketsCount );

		if ( ticket->Aquire( importance, ticketsCount, false ) )
		{
			//m_nextImportanceUpdate = m_owner->GetLocalTime() + m_importanceUpdateDelay * FRand( 0.5f, 1.5f );
			MarkActiveBranchDirty();
		}
	}

	ticket->ClearRequest();

	Super::Update();
}
void CBehTreeNodeCombatTicketManagerInstance::Deactivate()
{
	m_ticket->FreeTicket();
	Super::Deactivate();
}

Bool CBehTreeNodeCombatTicketManagerInstance::OnListenedEvent( CBehTreeEvent& e )
{
	if ( e.m_eventName == CNAME( AI_ForceImmediateTicketImportanceUpdate ) )
	{
		SGameplayEventParamCName* param = e.m_gameplayEventData.Get< SGameplayEventParamCName >();
		if ( !param || param->m_value != m_ticket->GetTicketName() )
		{
			return false;
		}

		CBehTreeTicketData* ticket = &(*m_ticket);
		
		if ( ticket->HasTicket() || ticket->GetTicket().HasValidRequest() )
		{
			DelayImportanceUpdate();
			Float importance;
			Uint16 ticketsCount;
			ComputeTicketImportance( importance, ticketsCount );
			ticket->SetImportance( importance );
		}

		return true;
	}
	return Super::OnListenedEvent( e );
}

