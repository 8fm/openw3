/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeBroadcastReactionScene.h"
#include "../../common/game/behTreeReactionManager.h"
#include "../../common/engine/weatherManager.h"

IMPLEMENT_ENGINE_CLASS( SReactionSceneEvent )

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeCombatTicketManagedGetDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeDecoratorInstance*	CBehTreeNodeBroadcastReactionSceneDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeCombatTicketManagedGetInstance
////////////////////////////////////////////////////////////////////////

Bool CBehTreeNodeBroadcastReactionSceneInstance::Activate()
{
	m_ownerReactionSceneComponent = m_owner->GetActor()->FindComponent< CReactionSceneActorComponent >();	
	return Super::Activate();
}

void CBehTreeNodeBroadcastReactionSceneInstance::UpdateImpl()
{		
	const Vector& playerPos = GGame->GetPlayerEntity()->GetWorldPositionRef();		
	const Vector& NPCPos = m_ownerReactionSceneComponent->GetEntity()->GetWorldPositionRef();
	const Float DIST = 20.f;

	//giving priority to closer NPC's to broadcast chat requests	
	if ( IsRaining() || playerPos.DistanceSquaredTo(NPCPos) >= DIST*DIST )
	{
		return;
	}

	if( !m_owner->GetActor()->WasVisibleLastFrame() )
	{
		return;
	}

	CNewNPC* npc = m_owner->GetNPC();

	if( !npc )
	{
		return;
	}

	if( npc->IsInGameplayScene() || npc->IsInNonGameplayScene() )
	{
		return;
	}

	
	CBehTreeWorkData* workData = m_workData.Get();
	Bool isAtWork = workData && workData->IsBeingPerformed();	

	if( isAtWork  && ( !workData->GetIsConscious() || workData->GetJTType() == 1 /* EJTT_Praying */ ) )
	{
		return;
	}

	if( !m_ownerReactionSceneComponent.Get()->CanPlayInScene() )
	{
		return;
	}

	if( m_ticket->HasTicket() )
	{
		return;
	}

	Int32 randomChat = GEngine->GetRandomNumberGenerator().Get< Int32 >( 0 , m_reactionScenesToBroadcast.SizeInt() );

	Int32 tries = 0;

	// loop to find scene, that can be played
	do 
	{
		if( m_reactionScenesToBroadcast[ randomChat ].m_reactionScene == CNAME( GreetingAction ) && isAtWork )
		{
			//working npcs cant greet
			continue;
		}
		if( !m_reactionScenesToBroadcast[ randomChat ].m_workOnlyBroadcast || isAtWork )
		{			
			if( m_reactionScenesToBroadcast[ randomChat ].m_inputsAsymetric &&  m_ownerReactionSceneComponent->IfActorHasSceneInput( m_reactionScenesToBroadcast[ randomChat ].m_requiredSceneInputs[ 0 ] ) )
			{
				break;
			}

			if( !m_reactionScenesToBroadcast[ randomChat ].m_inputsAsymetric &&  m_ownerReactionSceneComponent->IfActorHasSceneInput( m_reactionScenesToBroadcast[ randomChat ].m_requiredSceneInputs ) )
			{
				break;
			}
		}
		randomChat = ( randomChat + 1 ) % m_reactionScenesToBroadcast.SizeInt();		

	} while( ++tries < m_reactionScenesToBroadcast.SizeInt() );

	//if npc can't play any scene, return
	if( tries >= m_reactionScenesToBroadcast.SizeInt() )
	{
		return;
	}

	m_ticket->ManagerRequest( m_updateInterval );
	
	if ( !m_ticket->Aquire( 100, 1, false ) )
	{
		return;
	}
	if( !m_ticket->HasTicket() )
	{
		return;
	}

	m_ticket->LockTicket( true );

	CBehTreeReactionManager* reactionManager = GCommonGame->GetBehTreeReactionManager();
	
	//randomChat = Min( randomChat, m_reactionScenesToBroadcast.SizeInt() - 1 );
	//randomChat = Max( randomChat, 0 );	
	
	if( m_reactionScenesToBroadcast[ randomChat ].m_inputsAsymetric )
	{
		m_ownerReactionSceneComponent->SetRequiredSceneInput( m_reactionScenesToBroadcast[ randomChat ].m_requiredSceneInputs[ 1 ] );
	}
	else
	{
		m_ownerReactionSceneComponent->SetRequiredSceneInputs( m_reactionScenesToBroadcast[ randomChat ].m_requiredSceneInputs );
	}	

	reactionManager->CreateReactionEvent( m_owner->GetActor(), m_reactionScenesToBroadcast[ randomChat ].m_reactionScene , 20, 15, -1, 10, true, false, false, Vector() );
	if( !m_ownerReactionSceneComponent.Get()->GetSceneStartedSuccesfully() )
	{
		m_ticket->FreeTicket();
	}	
}

void CBehTreeNodeBroadcastReactionSceneInstance::Update()
{
	if( m_ownerReactionSceneComponent.Get() && m_nextUpdateTime <= m_owner->GetLocalTime() )
	{
		m_nextUpdateTime = m_owner->GetLocalTime() + m_updateInterval;
		UpdateImpl();		
	}

	IBehTreeNodeDecoratorInstance::Update();
}

Bool CBehTreeNodeBroadcastReactionSceneInstance::IsRaining()
{
	const static Float RAIN_TREESHOLD = 0.01f;

	if( CWorld* world = GGame->GetActiveWorld() )
	{
		if( CEnvironmentManager* envManager = world->GetEnvironmentManager() )
		{
			CWeatherManager* weatherManager = envManager->GetWeatherManager();

			if( weatherManager )
			{
				Float rainStrength = weatherManager->GetEffectStrength( EWeatherEffectType::WET_RAIN );
				return rainStrength > RAIN_TREESHOLD;
			}
		}
	}
	return false;
}
