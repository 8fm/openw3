/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeClosestNonFriendlyTargetSelection.h"

#include "r4BehTreeInstance.h"


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeClosestNonFriendlyTargetSelectionDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeInstance* CBehTreeNodeClosestNonFriendlyTargetSelectionDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeClosestNonFriendlyTargetSelectionInstance
////////////////////////////////////////////////////////////////////////
CBehTreeNodeClosestNonFriendlyTargetSelectionInstance::CBehTreeNodeClosestNonFriendlyTargetSelectionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_testDelay( def.m_testDelay )
	, m_choiceTimeout( 0.f )
{
	SBehTreeEvenListeningData eventListenerData;
	eventListenerData.m_eventName = CNAME( BeingHit );
	eventListenerData.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	context.AddEventListener( eventListenerData, this );

	eventListenerData.m_eventName = CNAME( ReevaluateCombatTarget );
	eventListenerData.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	context.AddEventListener( eventListenerData, this );

	eventListenerData.m_eventName = CNAME( NoticedObjectReevaluation );
	eventListenerData.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	context.AddEventListener( eventListenerData, this );
}


CActor* CBehTreeNodeClosestNonFriendlyTargetSelectionInstance::FindTarget()
{
	Float currTime = m_owner->GetLocalTime();
	if ( m_choiceTimeout < currTime )
	{
		m_choiceTimeout = currTime + m_testDelay;

		CNewNPC* npc = m_owner->GetNPC();
		if ( !npc )
		{
			return NULL;
		}

		Float minDistSq = FLT_MAX;

		CActor* bestGuy = NULL;
		CActor* currentGuy = m_currentChoice.Get();

		const auto& noticedGuyz = npc->GetNoticedObjects();
		for ( auto it = noticedGuyz.Begin(), end = noticedGuyz.End(); it != end; ++it )
		{
			CActor* a = it->m_actorHandle.Get();
			if ( !a || npc->GetAttitude( a ) == AIA_Friendly )
			{
				continue;
			}

			Float distSq = npc->GetWorldPositionRef().DistanceSquaredTo( a->GetWorldPositionRef() );

			// currently selected non-friendly get special 'bonus' and is selected by default
			if ( a == currentGuy)
			{
				distSq *= 0.9f * 0.9f;
			}

			if ( distSq < minDistSq )
			{
				minDistSq	= distSq;
				bestGuy		= a;
			}
		}

		m_currentChoice = bestGuy;
		return bestGuy;
	}
	return m_currentChoice.Get();
}

void CBehTreeNodeClosestNonFriendlyTargetSelectionInstance::OnDestruction()
{
	SBehTreeEvenListeningData eventListenerData;
	eventListenerData.m_eventName = CNAME( BeingHit );
	eventListenerData.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	m_owner->RemoveEventListener( eventListenerData, this );

	eventListenerData.m_eventName = CNAME( ReevaluateCombatTarget );
	eventListenerData.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	m_owner->RemoveEventListener( eventListenerData, this );

	eventListenerData.m_eventName = CNAME( NoticedObjectReevaluation );
	eventListenerData.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	m_owner->RemoveEventListener( eventListenerData, this );

	Super::OnDestruction();
}

Bool CBehTreeNodeClosestNonFriendlyTargetSelectionInstance::IsAvailable()
{
	CActor* newTarget = FindTarget();
	if ( newTarget != m_owner->GetCombatTarget().Get() )
	{
		return true;
	}

	DebugNotifyAvailableFail();
	return false;
}
Bool CBehTreeNodeClosestNonFriendlyTargetSelectionInstance::Activate()
{
	CActor* newTarget = FindTarget();
	m_owner->SetCombatTarget( newTarget, false );
	return Super::Activate();
}
void CBehTreeNodeClosestNonFriendlyTargetSelectionInstance::Update()
{
	Complete( BTTO_SUCCESS );
}
Bool CBehTreeNodeClosestNonFriendlyTargetSelectionInstance::OnListenedEvent( CBehTreeEvent& e )
{
	m_choiceTimeout = 0.f;
	return true;
}