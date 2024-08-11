/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
* DrUiD
*/

#include "build.h"


#ifndef NO_DEBUG_SERVER

//////////////////////////////////////////////////////////////////////////
// headers
#include "../../common/engine/debugServerHelpers.h"
#include "../../common/engine/debugServerManager.h"
#include "r4GameDebuggerCommands.h"

#include "../../common/engine/areaComponent.h"
#include "../../common/game/player.h"
#include "../../common/game/movableRepresentationPhysicalCharacter.h"
#include "../../common/game/movingPhysicalAgentComponent.h"
#include "../../common/game/behTreeGuardAreaData.h"
#include "../../common/game/communitySystem.h"
#include "../../common/game/communityAgentStub.h"
#include "../../common/game/communityUtility.h"
#include "../../common/game/encounter.h"


//////////////////////////////////////////////////////////////////////////
// names
RED_DEFINE_STATIC_NAME( GetStat );
RED_DEFINE_STATIC_NAME( GetStatMax );
RED_DEFINE_STATIC_NAME( IsImmortal );
RED_DEFINE_STATIC_NAME( IsInvulnerable );
RED_DEFINE_STATIC_NAME( IsAlive );


//////////////////////////////////////////////////////////////////////////
// implementations

//////////////////////////////////////////////////////////////////////////
//
// getplayer
Uint32 CDebugServerCommandGetPlayer::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	RED_UNUSED( owner );

	if ( !GGame->GetActiveWorld() || !DBGSRV_CALL( IsAttached() ) )
		return 0;

	// init
	CEntity* playerEnt = GGame->GetPlayerEntity();
	CGameplayEntity* playerGPEnt = Cast<CGameplayEntity>( playerEnt );
	CEntity* player = playerEnt;
	CPlayer* playerActor = Cast<CPlayer>( playerEnt );

	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	/*1*/packet.WriteString( "player" );

	// fill data
	if ( player )
	{
		// top level function
		const CFunction* topFunction = player->GetTopLevelFunction();

		// get hp (vitality)
		Float statHPMax = 0, statHP = 0;
		{
			CallFunctionRet< Float >( player, CNAME( GetStatMax ), 0 /* BCS_Vitality */, statHPMax );
			CallFunctionRet< Float >( player, CNAME( GetStat ), 0 /* BCS_Vitality */, statHP );
		}

		// immortality
		Bool invulnerable = false, immortal = false;
		{
			CallFunctionRet< Bool >( player, CNAME( IsImmortal ), immortal );
			CallFunctionRet< Bool >( player, CNAME( IsInvulnerable ), invulnerable );
		}

		// is alive - from scripts because is overridden
		Bool isAlive = false;
		{
			CallFunctionRet< Bool >( player, CNAME( IsAlive ), isAlive );
		}

		// mac
		CMovingPhysicalAgentComponent* mac = Cast< CMovingPhysicalAgentComponent >( playerActor->GetMovingAgentComponent() );
		CMRStack* stack = mac ? mac->GetRepresentationStack() : nullptr;
		const Bool isCustomMov = mac ? mac->IsAnimatedMovement() : false;;
		const Bool isStaticColEnabled = mac ? mac->IsStaticCollisionsEnabled() : false;
		const Bool isSnalToNavSpace = mac ? mac->IsSnapedToNavigableSpace() : false;


		/////////////////////////
		// base
		const Uint64 uid = reinterpret_cast<Uint64>( player );
		/*2*/packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( uid ).AsChar() ) );
		/*3*/packet.WriteString( player->GetClass()->GetName().AsAnsiChar() );
		const Vector& pos = player->GetWorldPositionRef();
		/*4*/packet.WriteString( UNICODE_TO_ANSI( String::Printf( TXT( "%0.3f %0.3f %0.3f %0.3f" ), pos.X, pos.Y, pos.Z, pos.W ).AsChar() ) );

		// names
		/*5*/packet.WriteString( UNICODE_TO_ANSI( player->GetDisplayName().AsChar() ) );
		/*6*/packet.WriteString( UNICODE_TO_ANSI( player->GetFriendlyName().AsChar() ) );
		/*7*/packet.WriteString( UNICODE_TO_ANSI( player->GetTemplate()->GetFriendlyName().AsChar() ) );

		// common
		/*8*/packet.WriteString( UNICODE_TO_ANSI( ToString( statHP ).AsChar() ) );
		/*9*/packet.WriteString( UNICODE_TO_ANSI( ToString( statHPMax ).AsChar() ) );
		/*10*/packet.WriteString( UNICODE_TO_ANSI( player->GetCurrentStateName().AsString().AsChar() ) );
		/*11*/packet.WriteString( topFunction ? UNICODE_TO_ANSI( topFunction->GetName().AsString().AsChar() ) : "None" );
		/*12*/packet.WriteString( UNICODE_TO_ANSI( ToString( player->GetComponents().Size() ).AsChar() ) );
		/*13*/packet.WriteString( UNICODE_TO_ANSI( player->GetTags().ToString().AsChar() ) );
		/*14*/packet.WriteString( UNICODE_TO_ANSI( playerActor->GetAttitudeGroup().AsString().AsChar() ) );
		/*15*/packet.WriteString( UNICODE_TO_ANSI( playerActor->GetVoiceTag().AsString().AsChar() ) );

		// states
		/*16*/packet.WriteString( UNICODE_TO_ANSI( ToString( isAlive ).AsChar() ) );
		/*17*/packet.WriteString( UNICODE_TO_ANSI( ToString( invulnerable ).AsChar() ) );
		/*18*/packet.WriteString( UNICODE_TO_ANSI( ToString( immortal ).AsChar() ) );

		// mac
		/*19*/packet.WriteString( mac ? "yes" : "no" ); // CMovingPhysicalAgentComponent
		/*20*/packet.WriteString( isCustomMov ? "true" : "false" );
		/*21*/packet.WriteString( UNICODE_TO_ANSI( ToString( isStaticColEnabled ).AsChar() ) );
		/*22*/packet.WriteString( "false" );			// /*deprecated - UNICODE_TO_ANSI( ToString( isSnapToSurface ).AsChar() )*/
		/*23*/packet.WriteString( UNICODE_TO_ANSI( ToString( isSnalToNavSpace ).AsChar() ) );

		// layer info
		const Uint64 layerUid = reinterpret_cast<Uint64>( playerActor->GetLayer() );
		/*24.6*/packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( layerUid ).AsChar() ) );

		// mac/representation stack
		if ( stack )
		{
			const Uint32 repCount = stack->GetRepresentationsCount();
			/*25*/packet.WriteString( UNICODE_TO_ANSI( ToString( repCount+1 ).AsChar() ) );
			/*26*/packet.WriteString( UNICODE_TO_ANSI( ( String( TXT( "Active: " ) ) + stack->GetActiveRepresentation()->GetName().AsString() + TXT( ": " ) + ToString( stack->GetStackPosition() ) ).AsChar() ) );
			 
			for ( Uint32 no = 0; no < repCount; ++no )
			{
				IMovableRepresentation* rep = stack->GetRepresentationByIndex( no );
				if ( rep )
				/*27..27+no*/packet.WriteString( UNICODE_TO_ANSI( ( ToString( no ) + TXT( " - " ) + rep->GetName().AsString() + TXT( ": " ) + ToString( rep->GetRepresentationPosition() ) ).AsChar() ) );
			}
		}

		const EulerAngles& playerRot = player->GetRotation();
		/*27+no+1.13*/packet.WriteString( UNICODE_TO_ANSI( ToString( playerRot.Yaw ).AsChar() ) );

		// new mac - after representations refactor
		const Bool isPhysRequest = mac ? mac->IsPhysicalRepresentationRequested() : false;
		const Bool isPhysEnabled = mac ? mac->IsPhysicalRepresentationEnabled() : false;
		const Uint16 physEnableFlags = mac ? mac->GetPhysicalRepresentationEnableFlags() : 0;
		const Uint16 physDisableFlags = mac ? mac->GetPhysicalRepresentationDisableFlags() : 0;
		const Uint16 entityForcedFlags = mac ? mac->GetEntityRepresentationForceFlags() : 0;
		/*27+no + 2*/packet.WriteString( isPhysRequest ? "true" : "false" );
		/*27+no + 3*/packet.WriteString( isPhysEnabled ? "true" : "false" );
		/*27+no + 4*/packet.WriteString( UNICODE_TO_ANSI( ToString( physEnableFlags ).AsChar() ) );
		/*27+no + 5*/packet.WriteString( UNICODE_TO_ANSI( ToString( physDisableFlags ).AsChar() ) );
		/*27+no + 6*/packet.WriteString( UNICODE_TO_ANSI( ToString( entityForcedFlags ).AsChar() ) );
	}

	// send
	Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );

	// processed
	return 1;
};
//////////////////////////////////////////////////////////////////////////
//
// getnpcs
Uint32 CDebugServerCommandGetNPCs::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	RED_UNUSED( owner );

	if ( !GGame->GetActiveWorld() || !DBGSRV_CALL( IsAttached() ) )
		return 0;

	// init
	Uint32 packetsCount = 0;

	for ( CCommonGame::ActorIterator npc = CCommonGame::ActorIterator(); npc; ++npc )
	{
		if( (*npc)->IsA<CNewNPC>() )
		{
			///////////////////
			// init
			CActor* actor = (*npc);
			CNewNPC* newNpc = static_cast<CNewNPC*>( *npc );
			Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
			/*1*/packet.WriteString( "npc" );


			////////////////////
			// fill data
			// top level function
			const CFunction* topFunction = (*npc)->GetTopLevelFunction();

			// get hp (vitality)
			Float statHPMax = 0, statHP = 0;
			{
				CallFunctionRet< Float >( (*npc), CNAME( GetStatMax ), 0 /* BCS_Vitality */, statHPMax );
				CallFunctionRet< Float >( (*npc), CNAME( GetStat ), 0 /* BCS_Vitality */, statHP );
			}

			// states
			const Bool isExtControlled = actor->IsExternalyControlled();
			const Bool isHiddenInGame = actor->IsHiddenInGame();

			// immortality
			Bool invulnerable = false, immortal = false;
			{
				CallFunctionRet< Bool >( (*npc), CNAME( IsImmortal ), immortal );
				CallFunctionRet< Bool >( (*npc), CNAME( IsInvulnerable ), invulnerable );
			}

			// is alive - from scripts because is overridden
			Bool isAlive = false;
			{
				CallFunctionRet< Bool >( (*npc), CNAME( IsAlive ), isAlive );
			}

			// Behavior tree
			if ( !actor )
			{
				WARN_ENGINE( TXT( "newnpc without actor: %s" ), (*npc)->GetFriendlyName().AsChar() );
			}

			CBehTreeMachine* behTreeMachine = actor ? actor->GetBehTreeMachine() : nullptr;
			CBehTreeInstance* ai = behTreeMachine ? behTreeMachine->GetBehTreeInstance() : nullptr;

			// mac
			CMovingPhysicalAgentComponent* mac = Cast< CMovingPhysicalAgentComponent >( (*npc)->GetMovingAgentComponent() );
			CMRStack* stack = mac ? mac->GetRepresentationStack() : nullptr;
			const Bool isCustomMov = mac ? mac->IsAnimatedMovement() : false;
			const Bool isStaticColEnabled = mac ? mac->IsStaticCollisionsEnabled() : false;
			const Bool isSnalToNavSpace = mac ? mac->IsSnapedToNavigableSpace() : false;

			// community/encounter
			CCommunitySystem *cs = GCommonGame->GetSystem< CCommunitySystem >();


			///////////////////
			// base
			const Uint64 uid = reinterpret_cast<Uint64>( *npc );
			/*2*/packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( uid ).AsChar() ) );
			/*3*/packet.WriteString( (*npc)->GetClass()->GetName().AsAnsiChar() );
			const Vector& pos = (*npc)->GetWorldPositionRef();
			/*4*/packet.WriteString( UNICODE_TO_ANSI( String::Printf( TXT( "%0.3f %0.3f %0.3f %0.3f" ), pos.X, pos.Y, pos.Z, pos.W ).AsChar() ) );

			// names
			String displayName = (*npc)->GetDisplayName();
			CDebugServerHelpers::ConvertPolishToAnsi( displayName );
			/*5*/packet.WriteString( UNICODE_TO_ANSI( displayName.AsChar() ) );
			/*6*/packet.WriteString( UNICODE_TO_ANSI( (*npc)->GetFriendlyName().AsChar() ) );
			/*7*/packet.WriteString( (*npc)->GetTemplate() ? UNICODE_TO_ANSI( (*npc)->GetTemplate()->GetFriendlyName().AsChar() ) : "<null template>" );

			// common
			/*8*/packet.WriteString( UNICODE_TO_ANSI( ToString( statHP ).AsChar() ) );
			/*9*/packet.WriteString( UNICODE_TO_ANSI( ToString( statHPMax ).AsChar() ) );
			/*10*/packet.WriteString( UNICODE_TO_ANSI( (*npc)->GetCurrentStateName().AsString().AsChar() ) );
			/*11*/packet.WriteString( topFunction ? UNICODE_TO_ANSI( topFunction->GetName().AsString().AsChar() ) : "None" );
			/*12*/packet.WriteString( UNICODE_TO_ANSI( ToString( (*npc)->GetComponents().Size() ).AsChar() ) );
			/*13*/packet.WriteString( UNICODE_TO_ANSI( (*npc)->GetAttitudeGroup().AsString().AsChar() ) );
			/*14*/packet.WriteString( UNICODE_TO_ANSI( (*npc)->GetTags().ToString().AsChar() ) );
			/*15*/packet.WriteString( UNICODE_TO_ANSI( (*npc)->GetVoiceTag().AsString().AsChar() ) );

			// npc states
			/*16*/packet.WriteString( isAlive ? "true" : "false" );
			/*17*/packet.WriteString( invulnerable ? "true" : "false" );
			/*18*/packet.WriteString( immortal ? "true" : "false" );
			/*19.12*/packet.WriteString( isExtControlled ? "true" : "false" );
			/*20.12*/packet.WriteString( isHiddenInGame ? "true" : "false" );

			// combat info
			/*21.4*/packet.WriteString( actor ? (actor->IsAttackableByPlayer() ? "true" : "false") : "<actor is null>" );
			/*22.4*/packet.WriteString( actor ? UNICODE_TO_ANSI( ToString( actor->GetAttitude( GCommonGame->GetPlayer() ) ).AsChar() ) : "<actor is null>" );
			/*23.4*/packet.WriteString( actor ? UNICODE_TO_ANSI( actor->GetTarget() ? actor->GetTarget()->GetFriendlyName().AsChar() : TXT("NULL") ) : "<actor is null>" );

			// Guard area debug
			CBehTreeGuardAreaData* guardAreaData = ai ? CBehTreeGuardAreaData::Find( ai ) : nullptr;
			if ( ai && guardAreaData )
			{		
				// guard area
				CAreaComponent* guardArea = guardAreaData->GetGuardArea();
				if ( guardArea )
				{
					String guardAreaString = (guardArea->GetEntity()->GetFriendlyName() + TXT( " " ) + CDebugServerHelpers::GetStringFromUid( reinterpret_cast<Uint64>( guardArea->GetEntity() ) ) );
					/*24.4*/packet.WriteString( UNICODE_TO_ANSI( guardAreaString.AsChar() ) );
				}
				else
				{
					/*24.4*/packet.WriteString( "<NULL>" );
				}

				// pursuit area
				CAreaComponent* pursuitArea = guardAreaData->GetPursuitArea();
				if ( pursuitArea )
				{
					String pursuitAreaString = (pursuitArea->GetEntity()->GetFriendlyName() + TXT( " " ) + CDebugServerHelpers::GetStringFromUid( reinterpret_cast<Uint64>( pursuitArea->GetEntity() ) ) );
					/*25.4*/packet.WriteString( UNICODE_TO_ANSI( pursuitAreaString.AsChar() ) );
				}
				else
				{
					/*25.4*/packet.WriteString( UNICODE_TO_ANSI( ToString( guardAreaData->GetPursuitRange() ).AsChar() ) );
				}
			}
			else
			{
				/*24.4*/packet.WriteString( "No guard area used" );
				/*25.4*/packet.WriteString( "No pursuit range/area used" );
			}

			// mac
			/*26*/packet.WriteString( mac ? "yes" : "no" ); // CMovingPhysicalAgentComponent
			/*27*/packet.WriteString( isCustomMov ? "true" : "false" );
			/*28*/packet.WriteString( UNICODE_TO_ANSI( ToString( isStaticColEnabled ).AsChar() ) );
			/*29*/packet.WriteString( "false" );			// /*deprecated - UNICODE_TO_ANSI( ToString( isSnapToSurface ).AsChar() )*/
			/*30*/packet.WriteString( UNICODE_TO_ANSI( ToString( isSnalToNavSpace ).AsChar() ) );
			
			// layer info
			const Uint64 layerUid = reinterpret_cast<Uint64>( (*npc)->GetLayer() );
			/*31.6*/packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( layerUid ).AsChar() ) );

			// mac/representation stack
			if ( stack )
			{
				const Uint32 repCount = stack->GetRepresentationsCount();
				/*32*/packet.WriteString( UNICODE_TO_ANSI( ToString( repCount+1 ).AsChar() ) );
				/*33*/packet.WriteString( UNICODE_TO_ANSI( ( String( TXT( "Active: " ) ) + stack->GetActiveRepresentation()->GetName().AsString() + TXT( ": " ) + ToString( stack->GetStackPosition() ) ).AsChar() ) );

				for ( Uint32 no = 0; no < repCount; ++no )
				{
					IMovableRepresentation* rep = stack->GetRepresentationByIndex( no );
					if ( rep )
					{
						/*34..34+no*/packet.WriteString( UNICODE_TO_ANSI( ( ToString( no ) + TXT( " - " ) + rep->GetName().AsString() + TXT( ": " ) + ToString( rep->GetRepresentationPosition() ) ).AsChar() ) );
					}
					else
					{
						/*34..34+no*/packet.WriteString( "null" );
					}
				}
			}
			else
			{
				/*32*/packet.WriteString( "0" );
				/*33*/packet.WriteString( "Active: null" );
				/*34..34+no*/packet.WriteString( "null" );
			}

			// encounter
			if ( actor )
			{
				TDynArray<IActorTerminationListener*>& terminationListenrs = actor->GetTerminationListeners();

				// looking 4 encounter
				CEncounter* encounter = nullptr;
				for ( Uint32 i=0; i < terminationListenrs.Size(); ++i  )
				{
					encounter = terminationListenrs[ i ]->AsEncounter();
					if ( encounter )
						break;
				}
				if ( encounter )
				{
					/*35+no + 1.12*/packet.WriteString( UNICODE_TO_ANSI( encounter->GetFriendlyName().AsChar() ) );
				}
				else
				{
					/*35+no + 1.12*/packet.WriteString( "<no encounter>" );
				}
			}
			else
			{
				/*35+no + 1.12*/packet.WriteString( "<isn't an actor>" );
			}

			// community
			const SAgentStub* agentStub = cs->FindStubForNPC( newNpc );
			/*35+no + 2.12*/packet.WriteString( agentStub ? "true" : "false" );

			// agent stub info
			if ( agentStub )
			{
				// story phase
				const CName storyPhase = agentStub->GetActivePhaseName();
				/*35+no + 3.12*/packet.WriteString( (storyPhase == CName::NONE) ? "default" : UNICODE_TO_ANSI( storyPhase.AsString().AsChar() ) );

				// spawnset
				/*35+no + 4.12*/packet.WriteString( UNICODE_TO_ANSI( agentStub->GetSpawnsetName().AsChar() ) );

				// agent stub state
				/*35+no + 5.12*/packet.WriteString( UNICODE_TO_ANSI( CCommunityUtility::GetFriendlyAgentStateName( agentStub->m_state ).AsChar() ) );
			}
			else
			{
				/*35+no + 3.12*/packet.WriteString( "n/a" );
				/*35+no + 4.12*/packet.WriteString( "n/a" );
				/*35+no + 5.12*/packet.WriteString( "n/a" );
			}

			// NPC Action Schedule
			const NewNPCScheduleProxy& npcSchedule = newNpc->GetActionSchedule();
			if ( npcSchedule.GetActiveAP() != ActionPointBadID )
			{
				/*35+no + 6.12*/packet.WriteString( UNICODE_TO_ANSI( cs->GetActionPointManager()->GetFriendlyAPName( npcSchedule.GetActiveAP() ).AsChar() ) );
			}
			else
			{
				/*35+no + 6.12*/packet.WriteString( "empty" );
			}
			/*35+no + 7.12*/packet.WriteString( npcSchedule.UsesLastAP() ? "true" : "false" );

			// actor action work

			/*35+no + 8.12*/packet.WriteString( "not working" );
			/*35+no + 9.12*/packet.WriteString( "n/a" );
			/*35+no + 10.12*/packet.WriteString( "n/a" );
			/*35+no + 11.12*/packet.WriteString( "n/a" );

			// new mac - after representations refactor
			const Bool isPhysRequest = mac ? mac->IsPhysicalRepresentationRequested() : false;
			const Bool isPhysEnabled = mac ? mac->IsPhysicalRepresentationEnabled() : false;
			const Uint16 physEnableFlags = mac ? mac->GetPhysicalRepresentationEnableFlags() : 0;
			const Uint16 physDisableFlags = mac ? mac->GetPhysicalRepresentationDisableFlags() : 0;
			const Uint16 entityForcedFlags = mac ? mac->GetEntityRepresentationForceFlags() : 0;
			/*35+no + 12*/packet.WriteString( isPhysRequest ? "true" : "false" );
			/*35+no + 13*/packet.WriteString( isPhysEnabled ? "true" : "false" );
			/*35+no + 14*/packet.WriteString( UNICODE_TO_ANSI( ToString( physEnableFlags ).AsChar() ) );
			/*35+no + 15*/packet.WriteString( UNICODE_TO_ANSI( ToString( physDisableFlags ).AsChar() ) );
			/*35+no + 16*/packet.WriteString( UNICODE_TO_ANSI( ToString( entityForcedFlags ).AsChar() ) );

			// behtree
			if ( behTreeMachine )
			{
				/*35+no + 17*/packet.WriteString( UNICODE_TO_ANSI( behTreeMachine->GetInfo().AsChar() ) );

				String text;
				//behTreeMachine->DescribeAIState( text );
				if ( text.Size() )
					/*35+no + 18*/packet.WriteString( UNICODE_TO_ANSI( text.AsChar() ) );
				else
					/*35+no + 18*/packet.WriteString( "<empty>" );
			}
			else
			{
				/*35+no + 17*/packet.WriteString( "<n/a>" );
				/*35+no + 18*/packet.WriteString( "<n/a>" );
			}

			///////////////////////
			// send
			Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
			++packetsCount;
		}
	}

	// end of npc list
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	/*1*/packet.WriteString( "npc" );
	Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );

	// processed
	return ++packetsCount;
}

#endif


//////////////////////////////////////////////////////////////////////////
// EOF
