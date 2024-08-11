/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

//////////////////////////////////////////////////////////////////////////
// headers
#include "build.h"
#include "../engine/gameDebugger.h"
#include "../engine/gameDebuggerCommands.h"

#ifndef NO_GAME_DEBUGGER

#include "../core/diskFile.h"
#include "../engine/layerGroup.h"
#include "../engine/layerInfo.h"

#include "communityUtility.h"
#include "communityAgentStub.h"
#include "actionPointComponent.h"
#include "doorComponent.h"
#include "encounter.h"
#include "communitySystem.h"
#include "communityUtility.h"
#include "communityAgentStub.h"


//////////////////////////////////////////////////////////////////////////
// names
RED_DEFINE_STATIC_NAME( IsAlive );


//////////////////////////////////////////////////////////////////////////
//
// getworld
Uint32 CGameDebuggerCommandGetWorld::ProcessCommand( const String& data )
{
	if ( !GGame->GetActiveWorld() || GAMEDBG_CALL( IsAttached() ) )
		return 0;
	
	// init
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_GAME_DEBUGGER );
	/*1*/packet.WriteString( "world" );

	// fill data
	const String& worldName = GGame->GetActiveWorld()->GetFriendlyName();
	const Uint32 worldNameHash = ::GetHash( worldName );

	/*2*/packet.WriteString( UNICODE_TO_ANSI( worldName.AsChar() ) );
	/*3*/packet.WriteString( UNICODE_TO_ANSI( ToString( worldNameHash ).AsChar() ) );

	// send
	Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_GAME_DEBUGGER, packet );

	// processed
	return 1;
};
//////////////////////////////////////////////////////////////////////////
//
// getcamera
Uint32 CGameDebuggerCommandGetCamera::ProcessCommand( const String& data )
{
	if ( !GGame->GetActiveWorld() || GAMEDBG_CALL( IsAttached() ) )
		return 0;

	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_GAME_DEBUGGER );
	/*1*/packet.WriteString( "camera" );

	// pos
	const Vector& cameraPos = GGame->GetActiveWorld()->GetCameraPosition();
	/*2*/packet.WriteString( UNICODE_TO_ANSI( ToString( cameraPos.X ).AsChar() ) );
	/*3*/packet.WriteString( UNICODE_TO_ANSI( ToString( cameraPos.Y ).AsChar() ) );
	/*4*/packet.WriteString( UNICODE_TO_ANSI( ToString( cameraPos.Z ).AsChar() ) );
	/*5*/packet.WriteString( UNICODE_TO_ANSI( ToString( cameraPos.W ).AsChar() ) );

	// orient
	const Vector& cameraOrient = GGame->GetActiveWorld()->GetCameraForward();
	/*6*/packet.WriteString( UNICODE_TO_ANSI( ToString( cameraOrient.X ).AsChar() ) );
	/*7*/packet.WriteString( UNICODE_TO_ANSI( ToString( cameraOrient.Y ).AsChar() ) );
	/*8*/packet.WriteString( UNICODE_TO_ANSI( ToString( cameraOrient.Z ).AsChar() ) );
	/*9*/packet.WriteString( UNICODE_TO_ANSI( ToString( cameraOrient.W ).AsChar() ) );

	// yaw
	const Float yaw = EulerAngles::YawFromXY( cameraOrient.X, cameraOrient.Y );
	/*10*/packet.WriteString( UNICODE_TO_ANSI( ToString( yaw ).AsChar() ) );

	// send
	Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_GAME_DEBUGGER, packet );

	// processed
	return 1;
}
//////////////////////////////////////////////////////////////////////////
//
// getlayers
Uint32 CGameDebuggerCommandGetLayers::ProcessCommand( const String& data )
{
	if ( !GGame->GetActiveWorld() || GAMEDBG_CALL( IsAttached() ) )
		return 0;

	// init
	CLayerGroup* layerGroup = GGame->GetActiveWorld()->GetWorldLayers();
	TDynArray<CLayerInfo*> layers;
	layerGroup->GetLayers( layers, false, true );

	// populating layers list
	Uint32 stringLen = 0;
	const Uint32 layersCount = layers.Size();
	Uint32 layerParts = 0;

	// init
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_GAME_DEBUGGER );
	/*1*/packet.WriteString( "layers" );

	for ( Uint32 i = 0; i < layersCount; ++i )
	{
		String layerPath;
		Bool streamingTile = false;

		// compute layer name
		CLayerGroup* parentGroup = layers[i]->GetLayerGroup();
		while ( parentGroup )
		{
			if ( parentGroup->GetName() != TXT( "streaming_tiles" ) )
			{
				if ( parentGroup->GetParentGroup() )
					layerPath = parentGroup->GetName() + TXT( "/" ) + layerPath;
				parentGroup = parentGroup->GetParentGroup();
			}
			else
			{
				streamingTile = true;
				parentGroup = nullptr;
			}
		}

		// add layer to list
		if ( !streamingTile )
		{
			layerPath += layers[i]->GetShortName(); 

			// update layer status
			if ( layers[i]->IsLoaded() )
			{
				layerPath += String::Printf( TXT( "- %i/%i" ), layers[i]->GetLayer()->GetEntities().Size(), layers[i]->GetLayer()->GetNumAttachedComponents() );
			}

			/*1+i..layersCount*/packet.WriteString( UNICODE_TO_ANSI( layerPath.AsChar() ) );

			stringLen += layerPath.GetLength();
		}

		// send layers part
		if ( stringLen && (i == layersCount-1 || (i%25)==24) )
		{
			LOG_ENGINE( TXT( "> GDC: sending layer part: %i/%i bytes" ), ++layerParts, stringLen );

			// send
			Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_GAME_DEBUGGER, packet );

			// clear before next sent
			stringLen = 0;
			packet.Clear( RED_NET_CHANNEL_GAME_DEBUGGER );
			/*1*/packet.WriteString( "layers" );
		}
	}

	// end of layers list
	packet.Clear( RED_NET_CHANNEL_GAME_DEBUGGER );
	/*1*/packet.WriteString( "layers" );
	Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_GAME_DEBUGGER, packet );

	// processed
	return 1;
};
//////////////////////////////////////////////////////////////////////////
//
// getinventory
Uint32 CGameDebuggerCommandGetInventory::ProcessCommand( const String& data )
{
	if ( data.GetLength() == 0 || GAMEDBG_CALL( IsAttached() ) )
		return 0;

	// get entity uid
	Uint64 uid = CGameDebuggerHelpers::GetUidFromString( data );

	// check for inventory
	CGameplayEntity* actor = nullptr;
	if ( uid )
	{
		CCommonGame::ActorIterator actorIter;
		for ( CCommonGame::ActorIterator npc = CCommonGame::ActorIterator(); npc; ++npc )
		{
			if( (*npc)->IsA<CNewNPC>() && reinterpret_cast<Uint64>( *npc ) == uid )
			{
				actor = (*npc);
				break;
			}
		}
	}

	// player inventory
	else
	{
		actor = Cast<CGameplayEntity>( GGame->GetPlayerEntity() );
	}

	if ( !actor )
		return 0;

	// try to cast to gameplay entity
	CGameplayEntity* actorGPEnt = Cast<CGameplayEntity>( actor );
	if ( actorGPEnt )
	{
		// init
		Red::Network::ChannelPacket packet( RED_NET_CHANNEL_GAME_DEBUGGER );
		/*1*/packet.WriteString( "inventory" );

		// fill data
		// uid
		/*2*/packet.WriteString( UNICODE_TO_ANSI( data.AsChar() ) );

		CInventoryComponent* inventory = actorGPEnt->GetInventoryComponent();
		if ( inventory )
		{
			// count
			/*3*/packet.WriteString( UNICODE_TO_ANSI( ToString( inventory->GetItemCount() ).AsChar() ) );

			// inventory items
			const Uint32 invCount = inventory->GetItemCount();
			for( Uint32 i = 0; i < invCount; i++ )
			{
				const SInventoryItem* item = inventory->GetItem( i );
				if ( item )
					/*4+i..invCount*/packet.WriteString( UNICODE_TO_ANSI( item->GetInfo().AsChar() ) );
				else
					/*4+i..invCount*/packet.WriteString( "NULL" );
			};
		}
		else
		{
			/*3*/packet.WriteString( "0" );
		}

		// send
		Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_GAME_DEBUGGER, packet );
	}

	// processed
	return 1;
}
//////////////////////////////////////////////////////////////////////////
//
// getwaypoints
Uint32 CGameDebuggerCommandGetWaypoints::ProcessCommand( const String& data )
{
	if ( !GGame->GetActiveWorld() || GAMEDBG_CALL( IsAttached() ) )
		return 0;

	Uint32 packetsCount = 0;
	WorldAttachedEntitiesIterator entIter( GGame->GetActiveWorld() );
	for ( ; entIter; ++entIter )
	{
		for ( ComponentIterator<CWayPointComponent> it( *entIter ); it; ++it )
		{
			if ( (*it)->IsExactlyA<CWayPointComponent>() )
			{
				// init
				Red::Network::ChannelPacket packet( RED_NET_CHANNEL_GAME_DEBUGGER );
				/*1*/packet.WriteString( "waypoint" );

				// fill data
				// base
				const Uint64 uid = reinterpret_cast<Uint64>( *entIter );
				/*2*/packet.WriteString( UNICODE_TO_ANSI( CGameDebuggerHelpers::GetStringFromUid( uid ).AsChar() ) );
				/*3*/packet.WriteString( (*entIter)->GetClass()->GetName().AsAnsiChar() );
				const Vector& pos = (*entIter)->GetPosition();
				/*4*/packet.WriteString( UNICODE_TO_ANSI( String::Printf( TXT( "%0.3f %0.3f %0.3f %0.3f" ), pos.X, pos.Y, pos.Z, pos.W ).AsChar() ) );

				// names
				/*5*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetDisplayName().AsChar() ) );
				/*6*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetFriendlyName().AsChar() ) );
				/*7*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetTemplate() ? (*entIter)->GetTemplate()->GetFriendlyName().AsChar() : TXT("None") ) );

				// common
				/*8*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetTags().ToString().AsChar() ) );

				// send
				Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_GAME_DEBUGGER, packet );
				++packetsCount;
			}
		}
	}

	// end of waypoint list
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_GAME_DEBUGGER );
	/*1*/packet.WriteString( "waypoint" );
	Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_GAME_DEBUGGER, packet );

	// processed
	return ++packetsCount;
}
//////////////////////////////////////////////////////////////////////////
//
// getstubs
Uint32 CGameDebuggerCommandGetStubs::ProcessCommand( const String& data )
{
	if ( !GGame->GetActiveWorld() || GAMEDBG_CALL( IsAttached() ) )
		return 0;

	CCommunitySystem* cs = GCommonGame->GetSystem<CCommunitySystem>();
	if ( !cs )
		return 0;

	Uint32 packetsCount = 0;
	TDynArray<SAgentStub*>& agentStubs = cs->GetAgentsStubs();
	for ( TDynArray<SAgentStub*>::const_iterator agentStub = agentStubs.Begin(); agentStub != agentStubs.End(); ++agentStub )
	{
		// init
		Red::Network::ChannelPacket packet( RED_NET_CHANNEL_GAME_DEBUGGER );
		/*1*/packet.WriteString( "stub" );

		// fill data
		// base
		const Uint64 uid = reinterpret_cast<Uint64>( (*agentStub) );
		/*2*/packet.WriteString( UNICODE_TO_ANSI( CGameDebuggerHelpers::GetStringFromUid( uid ).AsChar() ) );
		const Vector& pos = (*agentStub)->m_communityAgent.GetPosition();
		/*3*/packet.WriteString( UNICODE_TO_ANSI( String::Printf( TXT( "%0.3f %0.3f %0.3f %0.3f" ), pos.X, pos.Y, pos.Z, pos.W ).AsChar() ) );

		// stub index
		Int32 stubIndex = cs->GetStubIndex( *agentStub );
		/*4*/packet.WriteString( UNICODE_TO_ANSI( ToString( stubIndex ).AsChar() ) );

		// state name
		/*5*/packet.WriteString( UNICODE_TO_ANSI( CCommunityUtility::GetFriendlyAgentStateName( (*agentStub)->m_state ).AsChar() ) );

		// is spawned
		/*6*/packet.WriteString( (*agentStub)->m_npc.Get() ? "1" : "0" );

		// processing timer
		/*7*/packet.WriteString( UNICODE_TO_ANSI( ToString( (*agentStub)->m_processingTimer ).AsChar() ) );

		// m_appearance
		/*8*/packet.WriteString( UNICODE_TO_ANSI( (*agentStub)->m_appearance.AsString().AsChar() ) );

		// parent spawn set
		const CCommunity* spawnSet = (*agentStub)->GetParentSpawnset();
		const CName storyPhaseName = spawnSet->GetActivePhaseName();
		/*9*/packet.WriteString( UNICODE_TO_ANSI( spawnSet->GetFriendlyName().AsChar() ) );
		/*10*/packet.WriteString( UNICODE_TO_ANSI( spawnSet->GetFile()->GetDepotPath().AsChar() ) );
		/*11*/packet.WriteString( UNICODE_TO_ANSI( spawnSet->GetFile()->GetName().AsChar() ) );

		// active phase
		/*12*/packet.WriteString( (storyPhaseName != CName::NONE) ? UNICODE_TO_ANSI( storyPhaseName.AsString().AsChar() ) : "Default" );
		/*13*/packet.WriteString( UNICODE_TO_ANSI( (*agentStub)->GetActivePhase()->m_comment.AsChar() ) );

		// active spawn set line
		/*14*/packet.WriteString( UNICODE_TO_ANSI( (*agentStub)->GetActiveSpawnsetLine()->m_comment.AsChar() ) );
		/*15*/packet.WriteString( UNICODE_TO_ANSI( (*agentStub)->GetActiveSpawnsetLine()->m_entryID.AsChar() ) );

		// current AP name
		String currAPFriendlyName( TXT( "None" ) );
		const NewNPCSchedule& schedule = (*agentStub)->GetSchedule();
		if ( schedule.m_activeActionPointID != ActionPointBadID )
			currAPFriendlyName = cs->GetActionPointManager()->GetFriendlyAPName( schedule.m_activeActionPointID );
		/*16*/packet.WriteString( UNICODE_TO_ANSI( currAPFriendlyName.AsChar() ) );

		// entities
		CSEntitiesEntry* entities = (*agentStub)->GetEntitiesEntry();
		if ( entities )
		{
			/*17*/packet.WriteString( UNICODE_TO_ANSI( entities->m_entitySpawnTags.ToString().AsChar() ) );

			const Bool entityTemplate = entities->m_entityTemplate.IsLoaded();
			/*18*/packet.WriteString( entityTemplate ? UNICODE_TO_ANSI( entities->m_entityTemplate.Get()->GetFriendlyName().AsChar() ) : "Not Loaded" );

			// appearances
			const Uint32 appCount = entities->m_appearances.Size();
			/*19*/packet.WriteString( UNICODE_TO_ANSI( ToString( appCount ).AsChar() ) );
			for ( Uint32 app = 0; app < appCount; ++app )
			{
				/*20..20+app*/packet.WriteString( entities->m_appearances[app].AsAnsiChar() );
			}
		}

		// send
		Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_GAME_DEBUGGER, packet );
		++packetsCount;
	}

	// end of stub list
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_GAME_DEBUGGER );
	/*1*/packet.WriteString( "stub" );
	Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_GAME_DEBUGGER, packet );

	// processed
	return ++packetsCount;;
}
//////////////////////////////////////////////////////////////////////////
//
// getactionpoints
Uint32 CGameDebuggerCommandGetActionPoints::ProcessCommand( const String& data )
{
	if ( !GGame->GetActiveWorld() || GAMEDBG_CALL( IsAttached() ) )
		return 0;

	Uint32 packetsCount = 0;
	CWorld* gworld = GGame->GetActiveWorld();
	WorldAttachedEntitiesIterator entIter( gworld );
	for ( ; entIter; ++entIter )
	{
		for ( ComponentIterator<CActionPointComponent> it( *entIter ); it; ++it )
		{
			if ( (*it)->IsExactlyA<CActionPointComponent>() )
			{
				// init
				Red::Network::ChannelPacket packet( RED_NET_CHANNEL_GAME_DEBUGGER );
				/*1*/packet.WriteString( "actionpoint" );

				// base
				const Uint64 uid = reinterpret_cast<Uint64>( *entIter );
				/*2*/packet.WriteString( UNICODE_TO_ANSI( CGameDebuggerHelpers::GetStringFromUid( uid ).AsChar() ) );
				/*3*/packet.WriteString( (*entIter)->GetClass()->GetName().AsAnsiChar() );
				const Vector& pos = (*entIter)->GetPosition();
				/*4*/packet.WriteString( UNICODE_TO_ANSI( String::Printf( TXT( "%0.3f %0.3f %0.3f %0.3f" ), pos.X, pos.Y, pos.Z, pos.W ).AsChar() ) );

				// names
				/*5*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetDisplayName().AsChar() ) );
				/*6*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetFriendlyName().AsChar() ) );
				/*7*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetTemplate() ? (*entIter)->GetTemplate()->GetFriendlyName().AsChar() : TXT("None") ) );

				// common
				/*8*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetTags().ToString().AsChar() ) );

				// actionpoint
				// todo: additional data

				// send
				Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_GAME_DEBUGGER, packet );
				++packetsCount;
			}
		}
	}

	// end of actionpoint list
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_GAME_DEBUGGER );
	/*1*/packet.WriteString( "actionpoint" );
	Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_GAME_DEBUGGER, packet );

	// processed
	return ++packetsCount;;
}
//////////////////////////////////////////////////////////////////////////
//
// getencounters
Uint32 CGameDebuggerCommandGetEncounters::ProcessCommand( const String& data )
{
	if ( !GGame->GetActiveWorld() || GAMEDBG_CALL( IsAttached() ) )
		return 0;

	Uint32 packetsCount = 0;
	CWorld* gworld = GGame->GetActiveWorld();
	WorldAttachedEntitiesIterator entIter( gworld );
	for ( ; entIter; ++entIter )
	{
		if ( (*entIter)->IsExactlyA<CEncounter>() )
		{
			if ( CEncounter* encounter = Cast<CEncounter>( *entIter ) )
			{
				// init
				Red::Network::ChannelPacket packet( RED_NET_CHANNEL_GAME_DEBUGGER );
				/*1*/packet.WriteString( "encounter" );

				// base
				const Uint64 uid = reinterpret_cast<Uint64>( *entIter );
				/*2*/packet.WriteString( UNICODE_TO_ANSI( CGameDebuggerHelpers::GetStringFromUid( uid ).AsChar() ) );
				/*3*/packet.WriteString( (*entIter)->GetClass()->GetName().AsAnsiChar() );
				const Vector& pos = (*entIter)->GetPosition();
				/*4*/packet.WriteString( UNICODE_TO_ANSI( String::Printf( TXT( "%0.3f %0.3f %0.3f %0.3f" ), pos.X, pos.Y, pos.Z, pos.W ).AsChar() ) );

				// names
				/*5*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetDisplayName().AsChar() ) );
				/*6*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetFriendlyName().AsChar() ) );
				/*7*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetTemplate() ? (*entIter)->GetTemplate()->GetFriendlyName().AsChar() : TXT("None") ) );

				// common
				/*8*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetTags().ToString().AsChar() ) );
				/*9*/packet.WriteString( UNICODE_TO_ANSI( ToString( (*entIter)->GetComponents().Size() ).AsChar() ) );

				// encounter
				// todo: additional data

				// send
				Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_GAME_DEBUGGER, packet );
				++packetsCount;
			}
		}
	}

	// end of encounter list
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_GAME_DEBUGGER );
	/*1*/packet.WriteString( "encounter" );
	Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_GAME_DEBUGGER, packet );

	// processed
	return ++packetsCount;
}
//////////////////////////////////////////////////////////////////////////
//
// getdoors
Uint32 CGameDebuggerCommandGetDoors::ProcessCommand( const String& data )
{
	if ( !GGame->GetActiveWorld() || GAMEDBG_CALL( IsAttached() ) )
		return 0;

	Uint32 packetsCount = 0;
	CWorld* gworld = GGame->GetActiveWorld();
	WorldAttachedEntitiesIterator entIter( gworld );
	for ( ; entIter; ++entIter )
	{
		for ( ComponentIterator<CDoorComponent> it( *entIter ); it; ++it )
		{
			if( (*it)->IsExactlyA<CDoorComponent>() )
			{
				// init
				Red::Network::ChannelPacket packet( RED_NET_CHANNEL_GAME_DEBUGGER );
				/*1*/packet.WriteString( "door" );

				// base
				const Uint64 uid = reinterpret_cast<Uint64>( *entIter );
				/*2*/packet.WriteString( UNICODE_TO_ANSI( CGameDebuggerHelpers::GetStringFromUid( uid ).AsChar() ) );
				/*3*/packet.WriteString( (*entIter)->GetClass()->GetName().AsAnsiChar() );
				const Vector& pos = (*entIter)->GetPosition();
				/*4*/packet.WriteString( UNICODE_TO_ANSI( String::Printf( TXT( "%0.3f %0.3f %0.3f %0.3f" ), pos.X, pos.Y, pos.Z, pos.W ).AsChar() ) );

				// names
				/*5*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetDisplayName().AsChar() ) );
				/*6*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetFriendlyName().AsChar() ) );
				/*7*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetTemplate() ? (*entIter)->GetTemplate()->GetFriendlyName().AsChar() : TXT("None") ) );

				// common
				/*8*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetTags().ToString().AsChar() ) );

				// door
				/*9*/packet.WriteString( UNICODE_TO_ANSI( ToString( (Uint32)(*it)->GetCurrentState() ).AsChar() ) );
				/*10*/packet.WriteString( UNICODE_TO_ANSI( ToString( !!(*it)->IsMoving() ).AsChar() ) );
				/*11*/packet.WriteString( UNICODE_TO_ANSI( ToString( !!(*it)->IsLocked() ).AsChar() ) );

				// send
				Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_GAME_DEBUGGER, packet );
				++packetsCount;
			}
		}
	}

	// end of doors list
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_GAME_DEBUGGER );
	/*1*/packet.WriteString( "door" );
	Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_GAME_DEBUGGER, packet );

	// processed
	return ++packetsCount;
}

#endif


//////////////////////////////////////////////////////////////////////////
// EOF
