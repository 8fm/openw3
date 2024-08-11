/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
* DrUiD
*/

#include "build.h"


#ifndef NO_DEBUG_SERVER

//////////////////////////////////////////////////////////////////////////
// headers
#include "../engine/debugServerHelpers.h"
#include "../engine/debugServerManager.h"
#include "gameDebuggerCommands.h"

#include "../core/diskFile.h"

#include "../engine/worldIterators.h"
#include "../engine/layerGroup.h"
#include "../engine/layerInfo.h"
#include "../engine/areaComponent.h"
#include "../engine/triggerAreaComponent.h"
#include "../engine/physicsCharacterWrapper.h"
#include "../physics/physicsRagdollWrapper.h"

#include "communityUtility.h"
#include "communityAgentStub.h"
#include "actionPointComponent.h"
#include "doorComponent.h"
#include "encounter.h"
#include "communitySystem.h"
#include "communityUtility.h"
#include "communityAgentStub.h"
#include "aiSpawnTreeParameters.h"
#include "movingPhysicalAgentComponent.h"
#include "movableRepresentationPhysicalCharacter.h"


//////////////////////////////////////////////////////////////////////////
// const
const Uint32 layersFrequency = 100;
const Uint32 actionPointsFrequency = 75;
const Uint32 waypointsFrequency = 75;


//////////////////////////////////////////////////////////////////////////
// implementations

//////////////////////////////////////////////////////////////////////////
//
// getworld
Uint32 CDebugServerCommandGetWorld::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	RED_UNUSED( owner );

	if ( !GGame->GetActiveWorld() || !DBGSRV_CALL( IsAttached() ) )
		return 0;

	// init
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	/*1*/packet.WriteString( "world" );

	// fill data
	const String& worldName = GGame->GetActiveWorld()->GetFriendlyName();
	const Uint32 worldNameHash = ::GetHash( worldName );

	/*2*/packet.WriteString( UNICODE_TO_ANSI( worldName.AsChar() ) );
	/*3*/packet.WriteString( UNICODE_TO_ANSI( ToString( worldNameHash ).AsChar() ) );

	// send
	Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );

	// processed
	return 1;
};
//////////////////////////////////////////////////////////////////////////
//
// getcamera
Uint32 CDebugServerCommandGetCamera::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	RED_UNUSED( owner );

	// init
	const Bool ActiveAndAttached = (GGame->GetActiveWorld() && DBGSRV_CALL( IsAttached() ));

	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	/*1*/packet.WriteString( "camera" );

	// get free cam
	Vector freeCamPos;
	Vector freeCamOrient;
	const Bool freeCamEnabled = GGame->IsFreeCameraEnabled();
	if ( freeCamEnabled )
	{
		GGame->GetFreeCameraWorldPosition( &freeCamPos, NULL, &freeCamOrient );
	}

	// pos
	const Vector& cameraPos = freeCamEnabled ? freeCamPos : (ActiveAndAttached ? GGame->GetActiveWorld()->GetCameraPosition() : Vector::ZEROS);
	/*2*/packet.WriteString( UNICODE_TO_ANSI( ToString( cameraPos.X ).AsChar() ) );
	/*3*/packet.WriteString( UNICODE_TO_ANSI( ToString( cameraPos.Y ).AsChar() ) );
	/*4*/packet.WriteString( UNICODE_TO_ANSI( ToString( cameraPos.Z ).AsChar() ) );
	/*5*/packet.WriteString( UNICODE_TO_ANSI( ToString( cameraPos.W ).AsChar() ) );

	// orient
	const Vector& cameraOrient = freeCamEnabled ? freeCamOrient : (ActiveAndAttached ? GGame->GetActiveWorld()->GetCameraForward() : Vector::EY);
	/*6*/packet.WriteString( UNICODE_TO_ANSI( ToString( cameraOrient.X ).AsChar() ) );
	/*7*/packet.WriteString( UNICODE_TO_ANSI( ToString( cameraOrient.Y ).AsChar() ) );
	/*8*/packet.WriteString( UNICODE_TO_ANSI( ToString( cameraOrient.Z ).AsChar() ) );
	/*9*/packet.WriteString( UNICODE_TO_ANSI( ToString( cameraOrient.W ).AsChar() ) );

	// yaw
	const Float yaw = EulerAngles::YawFromXY( cameraOrient.X, cameraOrient.Y );
	/*10*/packet.WriteString( UNICODE_TO_ANSI( ToString( yaw ).AsChar() ) );

	// send
	Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );

	// processed
	return 1;
}
//////////////////////////////////////////////////////////////////////////
//
// getlayers
Uint32 CDebugServerCommandGetLayers::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	RED_UNUSED( owner );

	if ( !GGame->GetActiveWorld() || !DBGSRV_CALL( IsAttached() ) )
		return 0;

	PC_SCOPE_PIX( DbgSrv_Gdbg_GetLayers );

	// init
	CLayerGroup* layerGroup = GGame->GetActiveWorld()->GetWorldLayers();
	TDynArray<CLayerInfo*> layers;
	layerGroup->GetLayers( layers, false, true );

	// populating layers list
	Uint32 stringLen = 0;
	const Uint32 layersCount = layers.Size();
	Uint32 layerParts = 0;

	// init packet
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	/*1*/packet.WriteString( "layers" );

	for ( Uint32 i = 0; i < layersCount; ++i )
	{
		String layerPath;
		layerPath.Reserve( 100 );
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
			// get name
			layerPath += layers[i]->GetShortName(); 

			// init
			const Uint64 layerUid = reinterpret_cast<Uint64>( layers[i] );
			const Bool isLoaded = layers[i]->IsLoaded();
			const Uint32 entsCount = isLoaded ? layers[i]->GetLayer()->GetEntities().Size() : 0;
			const Uint32 compsCount = isLoaded ? layers[i]->GetLayer()->GetNumAttachedComponents() : 0;

			// compute layer data
			layerPath += String::Printf( TXT( " %u %u %u %ls" ), !!isLoaded, entsCount, compsCount, CDebugServerHelpers::GetStringFromUid( layerUid ).AsChar() );

			/*1+i..layersCount*/packet.WriteString( UNICODE_TO_ANSI( layerPath.AsChar() ) );

			stringLen += layerPath.GetLength();
			//LOG_ENGINE( TXT( "> dbgsrv: layer %u %u" ), i, layerPath.GetLength() );
		}

		// send layers part - every 'n' layers
		if ( stringLen && ((i == layersCount-1) || (i%layersFrequency)==(layersFrequency-1)) )
		{
			// next part
			++layerParts;

			//LOG_ENGINE( TXT( "> dbgsrv: sending layer names part: %u/%u bytes" ), layerParts, stringLen );

			// send
			Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );

			// clear before next sent
			stringLen = 0;
			packet.Clear( RED_NET_CHANNEL_DEBUG_SERVER );
			/*1*/packet.WriteString( "layers" );
		}
	}

	// end of layers list
	packet.Clear( RED_NET_CHANNEL_DEBUG_SERVER );
	/*1*/packet.WriteString( "layers" );
	Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );

	// processed
	return layerParts;
};
//////////////////////////////////////////////////////////////////////////
//
// getinventory
Uint32 CDebugServerCommandGetInventory::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	RED_UNUSED( owner );

	if ( data.Size() == 0 || data[0].GetLength() == 0 || !DBGSRV_CALL( IsAttached() ) )
		return 0;

	// get entity uid
	Uint64 uid = CDebugServerHelpers::GetUidFromString( data[0] );

	// check for entity
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
		Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
		/*1*/packet.WriteString( "inventory" );

		// fill data
		// uid
		/*2*/packet.WriteString( UNICODE_TO_ANSI( data[0].AsChar() ) );

		CInventoryComponent* inventory = actorGPEnt->GetInventoryComponent();
		if ( inventory )
		{
			// count
			/*3*/packet.WriteString( UNICODE_TO_ANSI( ToString( inventory->GetItemCount() ).AsChar() ) );

			// inventory items
			const Uint32 invCount = inventory->GetItemCount();
			for( Uint32 i = 0; i < invCount; i++ )
			{
				const SInventoryItem* item = inventory->GetItem( (SItemUniqueId)i );
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
		Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
	}

	// processed
	return 1;
}
//////////////////////////////////////////////////////////////////////////
//
// getwaypoint
Uint32 CDebugServerCommandGetWaypoint::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	RED_UNUSED( owner );

	if ( !GGame->GetActiveWorld() || !DBGSRV_CALL( IsAttached() ) )
		return 0;

	PC_SCOPE_PIX( DbgSrv_Gdbg_GetWaypoint );

	// get uid
	Uint64 uid = CDebugServerHelpers::GetUidFromString( data[0] );
	if ( !uid )
		return 0;

	// init
	CWorld* gworld = GGame->GetActiveWorld();

	WorldAttachedEntitiesIterator entIter( GGame->GetActiveWorld() );
	for ( ; entIter; ++entIter )
	{
		if ( reinterpret_cast<Uint64>( *entIter ) == uid )
		{
			ComponentIterator<CWayPointComponent> it( *entIter );
			if ( it )
			{
				// init
				Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
				/*1*/packet.WriteString( "waypointFull" );

				// fill data
				// base
				const Uint64 uid = reinterpret_cast<Uint64>( *entIter );
				/*2*/packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( uid ).AsChar() ) );
				/*3*/packet.WriteString( (*it)->GetClass()->GetName().AsAnsiChar() );
				const Vector& pos = (*entIter)->GetPosition();
				/*4*/packet.WriteString( UNICODE_TO_ANSI( String::Printf( TXT( "%0.3f %0.3f %0.3f %0.3f" ), pos.X, pos.Y, pos.Z, pos.W ).AsChar() ) );

				// names
				/*5*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetDisplayName().AsChar() ) );
				/*6*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetFriendlyName().AsChar() ) );
				/*7*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetTemplate() ? (*entIter)->GetTemplate()->GetFriendlyName().AsChar() : TXT("None") ) );

				// common
				/*8*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetTags().ToString().AsChar() ) );

				// layer info
				const Uint64 layerUid = reinterpret_cast<Uint64>( (*entIter)->GetLayer() );
				/*9.6*/packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( layerUid ).AsChar() ) );

				// send
				Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
			}
		}
	}

	// processed
	return 1;
}
//////////////////////////////////////////////////////////////////////////
//
// getwaypoints - deprecated
Uint32 CDebugServerCommandGetWaypoints::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	RED_UNUSED( owner );

	if ( !GGame->GetActiveWorld() || !DBGSRV_CALL( IsAttached() ) )
		return 0;

	PC_SCOPE_PIX( DbgSrv_Gdbg_GetWaypoints );

	Uint32 packetsCount = 0;
	WorldAttachedEntitiesIterator entIter( GGame->GetActiveWorld() );
	for ( ; entIter; ++entIter )
	{
		for ( ComponentIterator<CWayPointComponent> it( *entIter ); it; ++it )
		{
			if ( (*it)->IsExactlyA<CWayPointComponent>() )
			{
				// init
				Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
				/*1*/packet.WriteString( "waypoint" );

				// fill data
				// base
				const Uint64 uid = reinterpret_cast<Uint64>( *entIter );
				/*2*/packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( uid ).AsChar() ) );
				/*3*/packet.WriteString( (*entIter)->GetClass()->GetName().AsAnsiChar() );
				const Vector& pos = (*entIter)->GetPosition();
				/*4*/packet.WriteString( UNICODE_TO_ANSI( String::Printf( TXT( "%0.3f %0.3f %0.3f %0.3f" ), pos.X, pos.Y, pos.Z, pos.W ).AsChar() ) );

				// names
				/*5*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetDisplayName().AsChar() ) );
				/*6*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetFriendlyName().AsChar() ) );
				/*7*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetTemplate() ? (*entIter)->GetTemplate()->GetFriendlyName().AsChar() : TXT("None") ) );

				// common
				/*8*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetTags().ToString().AsChar() ) );

				// layer info
				const Uint64 layerUid = reinterpret_cast<Uint64>( (*entIter)->GetLayer() );
				/*9.6*/packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( layerUid ).AsChar() ) );

				// send
				Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
				++packetsCount;
			}
		}
	}

	// end of waypoint list
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	/*1*/packet.WriteString( "waypoint" );
	Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );

	// processed
	return ++packetsCount;
}
//////////////////////////////////////////////////////////////////////////
//
// getwaypointsList
Uint32 CDebugServerCommandGetWaypointsList::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	RED_UNUSED( owner );

	if ( !GGame->GetActiveWorld() || !DBGSRV_CALL( IsAttached() ) )
		return 0;

	PC_SCOPE_PIX( DbgSrv_Gdbg_GetWaypointsList );

	// init
	CWorld* gworld = GGame->GetActiveWorld();

	// init packet
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	/*1*/packet.WriteString( "waypointsList" );

	// counting loop
	Uint32 entCount = 0;
	WorldAttachedEntitiesIterator entIter( gworld );
	for ( ; entIter; ++entIter )
	{
		ComponentIterator<CWayPointComponent> it( *entIter );
		if ( it && (*it) && (*it)->IsA<CWayPointComponent>() && !((*it)->IsA<CActionPointComponent>()) )
			++entCount;
	}

	// sending loop
	Uint32 wpsParts = 0;
	entIter = WorldAttachedEntitiesIterator( gworld );
	for ( int i = 0; entIter; ++entIter )
	{
		ComponentIterator<CWayPointComponent> it( *entIter );
		if ( it )
		{
			if ( (*it)->IsA<CWayPointComponent>() && !((*it)->IsA<CActionPointComponent>()) )
			{
				// base
				const Uint64 uid = reinterpret_cast<Uint64>( *entIter );
				CLayer* layer = (*entIter)->GetLayer();
				const Uint64 layerUid = reinterpret_cast<Uint64>( layer );
				const Vector& pos = (*entIter)->GetPosition();

				const String ap = String::Printf( TXT( "%ls|%ls|%ls|%ls|%0.2f|%0.2f" ), CDebugServerHelpers::GetStringFromUid( uid ).AsChar(), CDebugServerHelpers::GetStringFromUid( layerUid ).AsChar(), (*entIter)->GetDisplayName().AsChar(), (*entIter)->GetTags().ToString().AsChar(), pos.X, pos.Y );
				/*2..2+i*/packet.WriteString( UNICODE_TO_ANSI( ap.AsChar() ) );
				++i;

				// send part of data - every 'n' waypoints
				if ( ((i == entCount) || (i%waypointsFrequency)==(waypointsFrequency-1)) )
				{
					// next part
					++wpsParts;

					//LOG_ENGINE( TXT( "> dbgsrv: sending WP part: %u %u" ), wpsParts, Red::Network::Manager::GetInstance()->GetOutgoingPacketsCount() );

					// send
					Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );

					// clear before next sent
					packet.Clear( RED_NET_CHANNEL_DEBUG_SERVER );
					/*1*/packet.WriteString( "waypointsList" );
				}
			}
		}
	}

	// end of waypoints list
	packet.Clear( RED_NET_CHANNEL_DEBUG_SERVER );
	/*1*/packet.WriteString( "waypointsList" );
	Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );

	// processed
	return ++wpsParts;
}
//////////////////////////////////////////////////////////////////////////
//
// getstubs
Uint32 CDebugServerCommandGetStubs::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	RED_UNUSED( owner );

	if ( !GGame->GetActiveWorld() || !DBGSRV_CALL( IsAttached() ) )
		return 0;

	CCommunitySystem* cs = GCommonGame->GetSystem<CCommunitySystem>();
	if ( !cs )
		return 0;

	PC_SCOPE_PIX( DbgSrv_Gdbg_GetStubs );

	Uint32 packetsCount = 0;
	TDynArray<SAgentStub*>& agentStubs = cs->GetAgentsStubs();
	for ( TDynArray<SAgentStub*>::const_iterator agentStub = agentStubs.Begin(); agentStub != agentStubs.End(); ++agentStub )
	{
		// init
		Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
		/*1*/packet.WriteString( "stub" );

		// fill data
		// base
		const Uint64 uid = reinterpret_cast<Uint64>( (*agentStub) );
		/*2*/packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( uid ).AsChar() ) );
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
		/*11*/packet.WriteString( UNICODE_TO_ANSI( spawnSet->GetFile()->GetFileName().AsChar() ) );

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
		Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
		++packetsCount;
	}

	// end of stub list
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	/*1*/packet.WriteString( "stub" );
	Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );

	// processed
	return ++packetsCount;;
}
//////////////////////////////////////////////////////////////////////////
//
// getactionpoint
Uint32 CDebugServerCommandGetActionPoint::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	RED_UNUSED( owner );

	if ( !GGame->GetActiveWorld() || !DBGSRV_CALL( IsAttached() ) )
		return 0;

	PC_SCOPE_PIX( DbgSrv_Gdbg_GetActionPoint );

	// get uid
	Uint64 uid = CDebugServerHelpers::GetUidFromString( data[0] );
	if ( !uid )
		return 0;

	// init
	CWorld* gworld = GGame->GetActiveWorld();
	CActionPointManager* apMan = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();
	if ( !apMan )
		return 0;

	WorldAttachedEntitiesIterator entIter( gworld );
	for ( ; entIter; ++entIter )
	{
		if ( reinterpret_cast<Uint64>( *entIter ) == uid )
		{
			ComponentIterator<CActionPointComponent> it( *entIter );
			if ( it )
			{
				// init
				CActionPointComponent* actionPoint = (*it);
				TActionPointID apID = actionPoint->GetID();

				// init packet
				Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
				/*1*/packet.WriteString( "actionpointFull" );

				// base
				/*2*/packet.WriteString( UNICODE_TO_ANSI( data[0].AsChar() ) );
				/*3*/packet.WriteString( (*it)->GetClass()->GetName().AsAnsiChar() );
				const Vector& pos = (*entIter)->GetPosition();
				/*4*/packet.WriteString( UNICODE_TO_ANSI( String::Printf( TXT( "%0.3f %0.3f %0.3f %0.3f" ), pos.X, pos.Y, pos.Z, pos.W ).AsChar() ) );

				// names
				/*5*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetDisplayName().AsChar() ) );
				/*6*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetFriendlyName().AsChar() ) );
				/*7*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetTemplate() ? (*entIter)->GetTemplate()->GetFriendlyName().AsChar() : TXT("None") ) );

				// common
				/*8*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetTags().ToString().AsChar() ) );

				// layer info
				const Uint64 layerUid = reinterpret_cast<Uint64>( (*entIter)->GetLayer() );
				/*9.6*/packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( layerUid ).AsChar() ) );

				// categories
				String categories;
				for ( TDynArray< TAPCategory >::const_iterator apCategory = actionPoint->GetActionCategories().Begin();
					apCategory != actionPoint->GetActionCategories().End(); )
				{
					categories += apCategory->AsString();
					if ( ++apCategory != actionPoint->GetActionCategories().End() )
					{
						categories += TXT(", ");
					}
				}
				/*10.10*/packet.WriteString( categories.GetLength() ? UNICODE_TO_ANSI( categories.AsChar() ) : "<empty>" );

				// is free
				if ( apMan->IsFree( apID ) )
					/*11.10*/packet.WriteString( "free" );
				else
					/*11.10*/packet.WriteString( "occupied" );

				// preferred next AP
				if ( actionPoint->HasPreferredNextAPs() )
				{
					String nextAP = CDebugServerHelpers::GetStringFromArrayOfCNames( actionPoint->GetPreferredNextAPsTagList().GetTags() );
					/*12.10*/packet.WriteString( UNICODE_TO_ANSI( nextAP.AsChar() ) );

					TActionPointID nextPrefApID = apMan->FindNextFreeActionPointInSequence( apID );
					if ( nextPrefApID != ActionPointBadID )
						/*13.10*/packet.WriteString( UNICODE_TO_ANSI( nextPrefApID.ToString().AsChar() ) );
					else
						/*13.10*/packet.WriteString( "<last action point in sequence>" );
				}
				else
				{
					/*12.10*/packet.WriteString( "No preferred next APs" );
					/*13.10*/packet.WriteString( "<last action point in sequence>" );
				}

				// component tags
				/*14.10*/packet.WriteString( UNICODE_TO_ANSI( actionPoint->GetTags().ToString().AsChar() ) );

				// id
				/*15.10*/packet.WriteString( UNICODE_TO_ANSI( apID.ToString().AsChar() ) );

				// debug info
				Uint32 problemReportsCount = 0;
				Bool flagInvalidWpPos = false, flagMissingWp = false;
				apMan->GetDebugInfo( apID, problemReportsCount, flagInvalidWpPos, flagMissingWp );
				/*16.10*/packet.WriteString( UNICODE_TO_ANSI( ToString( problemReportsCount ).AsChar() ) );
				/*17.10*/packet.WriteString( flagInvalidWpPos ? "true" : "false" );
				/*18.10*/packet.WriteString( flagMissingWp ? "true" : "false" );

				// job tree
				/*19.10*/packet.WriteString( actionPoint->GetJobTreeRes().Get() ? UNICODE_TO_ANSI( actionPoint->GetJobTreeRes().Get()->GetFriendlyName().AsChar() ) : "<no job tree>" );

				// ignore collisions
				/*20.10*/packet.WriteString( actionPoint->GetIgnoreCollisions() ? "true" : "false" );

				// breakable
				/*21.10*/packet.WriteString( actionPoint->IsBreakable() ? "true" : "false" );
				/*22.10*/packet.WriteString( actionPoint->IsBreakableByCutscene() ? "true" : "false" );

				// placement importance
				/*23.10*/packet.WriteString( actionPoint->GetPlacementImportance() ? "true" : "false" );

				// is active on start
				/*24.10*/packet.WriteString( actionPoint->IsActiveOnStart() ? "true" : "false" );

				// disable soft reactions
				/*25.10*/packet.WriteString( actionPoint->GetDisableSoftRactions() ? "disabled" : "enabled" );

				// send
				Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
			}
		}
	}

	// processed
	return 1;
}
//////////////////////////////////////////////////////////////////////////
//
// getactionpoints - deprecated
Uint32 CDebugServerCommandGetActionPoints::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	RED_UNUSED( owner );

	if ( !GGame->GetActiveWorld() || !DBGSRV_CALL( IsAttached() ) )
		return 0;

	PC_SCOPE_PIX( DbgSrv_Gdbg_GetActionPoints );

	// init
	Uint32 packetsCount = 0;
	CWorld* gworld = GGame->GetActiveWorld();
	CActionPointManager* apMan = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();
	if ( !apMan )
		return 0;

	WorldAttachedEntitiesIterator entIter( gworld );
	for ( ; entIter; ++entIter )
	{
		for ( ComponentIterator<CActionPointComponent> it( *entIter ); it; ++it )
		{
			if ( (*it)->IsExactlyA<CActionPointComponent>() )
			{
				// init
				CActionPointComponent* actionPoint = (*it);
				TActionPointID apID = actionPoint->GetID();

				// init packet
				Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
				/*1*/packet.WriteString( "actionpoint" );

				// base
				const Uint64 uid = reinterpret_cast<Uint64>( *entIter );
				/*2*/packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( uid ).AsChar() ) );
				/*3*/packet.WriteString( (*entIter)->GetClass()->GetName().AsAnsiChar() );
				const Vector& pos = (*entIter)->GetPosition();
				/*4*/packet.WriteString( UNICODE_TO_ANSI( String::Printf( TXT( "%0.3f %0.3f %0.3f %0.3f" ), pos.X, pos.Y, pos.Z, pos.W ).AsChar() ) );

				// names
				/*5*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetDisplayName().AsChar() ) );
				/*6*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetFriendlyName().AsChar() ) );
				/*7*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetTemplate() ? (*entIter)->GetTemplate()->GetFriendlyName().AsChar() : TXT("None") ) );

				// common
				/*8*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetTags().ToString().AsChar() ) );

				// layer info
				const Uint64 layerUid = reinterpret_cast<Uint64>( (*entIter)->GetLayer() );
				/*9.6*/packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( layerUid ).AsChar() ) );

				// categories
				String categories;
				for ( TDynArray< TAPCategory >::const_iterator apCategory = actionPoint->GetActionCategories().Begin();
					apCategory != actionPoint->GetActionCategories().End(); )
				{
					categories += apCategory->AsString();
					if ( ++apCategory != actionPoint->GetActionCategories().End() )
					{
						categories += TXT(", ");
					}
				}
				/*10.10*/packet.WriteString( categories.GetLength() ? UNICODE_TO_ANSI( categories.AsChar() ) : "<empty>" );

				// is free
				if ( apMan->IsFree( apID ) )
					/*11.10*/packet.WriteString( "free" );
				else
					/*11.10*/packet.WriteString( "occupied" );

				// preferred next AP
				if ( actionPoint->HasPreferredNextAPs() )
				{
					String nextAP = CDebugServerHelpers::GetStringFromArrayOfCNames( actionPoint->GetPreferredNextAPsTagList().GetTags() );
					/*12.10*/packet.WriteString( UNICODE_TO_ANSI( nextAP.AsChar() ) );

					TActionPointID nextPrefApID = apMan->FindNextFreeActionPointInSequence( apID );
					if ( nextPrefApID != ActionPointBadID )
						/*13.10*/packet.WriteString( UNICODE_TO_ANSI( nextPrefApID.ToString().AsChar() ) );
					else
						/*13.10*/packet.WriteString( "<last action point in sequence>" );
				}
				else
				{
					/*12.10*/packet.WriteString( "No preferred next APs" );
					/*13.10*/packet.WriteString( "<last action point in sequence>" );
				}

				// component tags
				/*14.10*/packet.WriteString( UNICODE_TO_ANSI( actionPoint->GetTags().ToString().AsChar() ) );

				// id
				/*15.10*/packet.WriteString( UNICODE_TO_ANSI( apID.ToString().AsChar() ) );

				// debug info
				Uint32 problemReportsCount = 0;
				Bool flagInvalidWpPos = false, flagMissingWp = false;
				apMan->GetDebugInfo( apID, problemReportsCount, flagInvalidWpPos, flagMissingWp );
				/*16.10*/packet.WriteString( UNICODE_TO_ANSI( ToString( problemReportsCount ).AsChar() ) );
				/*17.10*/packet.WriteString( flagInvalidWpPos ? "true" : "false" );
				/*18.10*/packet.WriteString( flagMissingWp ? "true" : "false" );

				// job tree
				/*19.10*/packet.WriteString( actionPoint->GetJobTreeRes().Get() ? UNICODE_TO_ANSI( actionPoint->GetJobTreeRes().Get()->GetFriendlyName().AsChar() ) : "<no job tree>" );

				// ignore collisions
				/*20.10*/packet.WriteString( actionPoint->GetIgnoreCollisions() ? "true" : "false" );

				// breakable
				/*21.10*/packet.WriteString( actionPoint->IsBreakable() ? "true" : "false" );
				/*22.10*/packet.WriteString( actionPoint->IsBreakableByCutscene() ? "true" : "false" );

				// placement importance
				/*23.10*/packet.WriteString( actionPoint->GetPlacementImportance() ? "true" : "false" );

				// is active on start
				/*24.10*/packet.WriteString( actionPoint->IsActiveOnStart() ? "true" : "false" );

				// disable soft reactions
				/*25.10*/packet.WriteString( actionPoint->GetDisableSoftRactions() ? "disabled" : "enabled" );

				// send
				Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
				++packetsCount;
			}
		}
	}

	// end of actionpoint list
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	/*1*/packet.WriteString( "actionpoint" );
	Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );

	// processed
	return ++packetsCount;;
}
//////////////////////////////////////////////////////////////////////////
//
// getactionpointsList
Uint32 CDebugServerCommandGetActionPointsList::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	RED_UNUSED( owner );

	if ( !GGame->GetActiveWorld() || !DBGSRV_CALL( IsAttached() ) )
		return 0;

	PC_SCOPE_PIX( DbgSrv_Gdbg_GetActionPointsList );

	// init
	CWorld* gworld = GGame->GetActiveWorld();
	CActionPointManager* apMan = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();
	if ( !apMan )
		return 0;

	// init packet
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	/*1*/packet.WriteString( "actionpointList" );

	// counting loop
	Uint32 entCount = 0;
	WorldAttachedEntitiesIterator entIter( gworld );
	for ( ; entIter; ++entIter )
	{
		ComponentIterator<CActionPointComponent> it( *entIter );
		if ( it && (*it) && (*it)->IsExactlyA<CActionPointComponent>() )
			++entCount;
	}

	// sending loop
	Uint32 apsParts = 0;
	entIter = WorldAttachedEntitiesIterator( gworld );
	for ( int i = 0; entIter; ++entIter )
	{
		ComponentIterator<CActionPointComponent> it( *entIter );
		if ( it )
		{
			if ( (*it)->IsExactlyA<CActionPointComponent>() )
			{
				// base
				const Uint64 uid = reinterpret_cast<Uint64>( *entIter );
				CLayer* layer = (*entIter)->GetLayer();
				const Uint64 layerUid = reinterpret_cast<Uint64>( layer );
				const Vector& pos = (*entIter)->GetPosition();

				const String ap = String::Printf( TXT( "%ls|%ls|%ls|%ls|%0.2f|%0.2f" ), CDebugServerHelpers::GetStringFromUid( uid ).AsChar(), CDebugServerHelpers::GetStringFromUid( layerUid ).AsChar(), (*entIter)->GetDisplayName().AsChar(), (*entIter)->GetTags().ToString().AsChar(), pos.X, pos.Y );
				/*2..2+i*/packet.WriteString( UNICODE_TO_ANSI( ap.AsChar() ) );
				++i;

				// send part of data - every 'n' action points
				if ( ((i == entCount) || (i%actionPointsFrequency)==(actionPointsFrequency-1)) )
				{
					// next part
					++apsParts;

					//LOG_ENGINE( TXT( "> dbgsrv: sending AP part: %u %u" ), apsParts, Red::Network::Manager::GetInstance()->GetOutgoingPacketsCount() );

					// send
					Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );

					// clear before next sent
					packet.Clear( RED_NET_CHANNEL_DEBUG_SERVER );
					/*1*/packet.WriteString( "actionpointList" );
				}
			}
		}
	}

	// end of actionpoint list
	packet.Clear( RED_NET_CHANNEL_DEBUG_SERVER );
	/*1*/packet.WriteString( "actionpointList" );
	Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );

	// processed
	return ++apsParts;
}
//////////////////////////////////////////////////////////////////////////
//
// getencounters
Uint32 CDebugServerCommandGetEncounters::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	RED_UNUSED( owner );

	if ( !GGame->GetActiveWorld() || !DBGSRV_CALL( IsAttached() ) )
		return 0;

	PC_SCOPE_PIX( DbgSrv_Gdbg_GetEncounters );

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
				CTriggerAreaComponent* triggerArea = encounter->GetTriggerArea();
				CEncounterParameters* params = encounter->GetEncounterParameters();

				// init packet
				Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
				/*1*/packet.WriteString( "encounter" );

				// base
				const Uint64 uid = reinterpret_cast<Uint64>( *entIter );
				/*2*/packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( uid ).AsChar() ) );
				/*3*/packet.WriteString( encounter->GetClass()->GetName().AsAnsiChar() );
				const Vector& pos = encounter->GetPosition();
				/*4*/packet.WriteString( UNICODE_TO_ANSI( String::Printf( TXT( "%0.3f %0.3f %0.3f %0.3f" ), pos.X, pos.Y, pos.Z, pos.W ).AsChar() ) );

				// names
				/*5*/packet.WriteString( UNICODE_TO_ANSI( encounter->GetDisplayName().AsChar() ) );
				/*6*/packet.WriteString( UNICODE_TO_ANSI( encounter->GetFriendlyName().AsChar() ) );
				/*7*/packet.WriteString( UNICODE_TO_ANSI( encounter->GetTemplate() ? encounter->GetTemplate()->GetFriendlyName().AsChar() : TXT("None") ) );

				// common
				/*8*/packet.WriteString( UNICODE_TO_ANSI( encounter->GetTags().ToString().AsChar() ) );
				/*9*/packet.WriteString( UNICODE_TO_ANSI( ToString( encounter->GetComponents().Size() ).AsChar() ) );

				// layer info
				const Uint64 layerUid = reinterpret_cast<Uint64>( encounter->GetLayer() );
				/*10.6*/packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( layerUid ).AsChar() ) );

				// flags
				/*11.10*/packet.WriteString( encounter->IsEnabled() ? "true" : "false" );
				/*12.10*/packet.WriteString( encounter->IsIgnoreAreaTrigger() ? "true" : "false" );
				/*13.10*/packet.WriteString( encounter->IsFullRespawnScheduled() ? "true" : "false" );
				/*14.10*/packet.WriteString( encounter->GetSpawnPoints().IsInitialized() ? "true" : "false" );
				/*15.10*/packet.WriteString( triggerArea ? (triggerArea->IsEnabled() ? "true" : "false") : "<trigger is NULL>" );		

				// trigger area shape
				const TDynArray<Vector>* points = encounter->GetWorldPoints();
				const Uint32 pointCount = points ? points->Size() : 0;
				/*16.10*/packet.WriteString( UNICODE_TO_ANSI( ToString( pointCount ).AsChar() ) );
				if ( pointCount && points )
				{
					for ( Uint32 i = 0; i < pointCount; ++i )
					{
						/*17.10 ... 17+i.10*/packet.WriteString( UNICODE_TO_ANSI( String::Printf( TXT( "%0.3f %0.3f" ), (*points)[i].X, (*points)[i].Y ).AsChar() ) );
					}
				}

				// encounter parameters
				Bool subParamsFound = false;
				Bool subParamsData = false;
				CEntity* guardAreaEntity = nullptr;
				if ( params )
				{
					TDynArray< THandle< IAISpawnTreeSubParameters > >& subParams = params->GetGlobalDefaults();
					if  ( !subParams.Empty() && subParams[0] )
					{
						IAISpawnTreeSubParameters* param = subParams[0].Get();
						// guard area params
						if ( param->IsA<CGuardAreaParameters>() )
						{
							/*17+i+1.10*/packet.WriteString( "CGuardAreaParameters" );

							// init
							CGuardAreaParameters* guardArea = static_cast<CGuardAreaParameters*>( param );
							subParamsFound = true;
							subParamsData = true;

							// guard area entity
							String guardAreaEntityString = TXT( "<no guard area entity>" );
							guardAreaEntity = guardArea->GetGuardArea();
							if ( guardAreaEntity )
							{
								guardAreaEntityString =	guardAreaEntity->GetFriendlyName() + TXT( " " ) +
									CDebugServerHelpers::GetStringFromUid( reinterpret_cast<Uint64>( guardAreaEntity ) );
							}
							/*17+i+2.10*/packet.WriteString( UNICODE_TO_ANSI( guardAreaEntityString.AsChar() ) );

							// guard area pursuit entity
							String guardAreaPursuitEntityString = TXT( "<no guard area pursuit entity>" );
							CEntity* guardAreaPursuitEntity = guardArea->GetGuardPursuitArea();
							if ( guardAreaPursuitEntity )
							{
								guardAreaPursuitEntityString = guardAreaPursuitEntity->GetFriendlyName() + TXT( " " ) +
									CDebugServerHelpers::GetStringFromUid( reinterpret_cast<Uint64>( guardAreaPursuitEntity ) );
							}
							/*17+i+3.10*/packet.WriteString( UNICODE_TO_ANSI( guardAreaPursuitEntityString.AsChar() ) );

							// guard area pursuit range
							/*17+i+4.10*/packet.WriteString( UNICODE_TO_ANSI( ToString( guardArea->GetGuardPursuitRange() ).AsChar() ) );
						}

						// idle beh default params
						else if ( param->IsA<CIdleBehaviorsDefaultParameters>() )
						{
							/*17+i+1.10*/packet.WriteString( "CIdleBehaviorsDefaultParameters" );

							// init
							CIdleBehaviorsDefaultParameters* idleBehDef = static_cast<CIdleBehaviorsDefaultParameters*>( param );
							subParamsFound = true;
							subParamsData = true;

							// action points area entity
							String actionPointaAreaEntityString = TXT( "<no action points area entity>" );
							CEntity* actionPointaAreaEntity = idleBehDef->GetActionPointsArea();
							if ( actionPointaAreaEntity )
							{
								actionPointaAreaEntityString = actionPointaAreaEntity->GetFriendlyName() + TXT( " " ) +
									CDebugServerHelpers::GetStringFromUid( reinterpret_cast<Uint64>( actionPointaAreaEntity ) ) + TXT( " " );
							}
							/*17+i+2.10*/packet.WriteString( UNICODE_TO_ANSI( actionPointaAreaEntityString.AsChar() ) );

							// wander area entity
							String wanderAreaEntityString = TXT( "<no wander area entity>" );
							CEntity* wanderAreaEntity = idleBehDef->GetWanderArea();
							if ( wanderAreaEntity )
							{
								wanderAreaEntityString = wanderAreaEntity->GetFriendlyName() + TXT( " " ) +
									CDebugServerHelpers::GetStringFromUid( reinterpret_cast<Uint64>( wanderAreaEntity ) ) + TXT( " " );
							}
							/*17+i+3.10*/packet.WriteString( UNICODE_TO_ANSI( wanderAreaEntityString.AsChar() ) );

							// wander points tag
							/*17+i+4.10*/packet.WriteString( idleBehDef->GetWanderPointsTag().AsAnsiChar() );
						}

						// unknown params
						else
						{
							/*17+i+1.10*/packet.WriteString( subParams[0]->GetClass()->GetName().AsAnsiChar() );
							subParamsFound = true;
						}
					}

					// null params
					if ( !subParams.Empty() && !subParams[0] )
					{
						/*17+i+1.10*/packet.WriteString( "[null params]" );
						subParamsFound = true;

						WARN_ENGINE( TXT( "Missing [null] encounter params: %s" ), encounter->GetFriendlyName().AsChar() );
					}
				}
				if ( !subParamsFound )
				{
					/*17+i+1.10*/packet.WriteString( "[no params]" );
				}
				if ( !subParamsData )
				{
					/*17+i+2.10*/packet.WriteString( "null" );
					/*17+i+3.10*/packet.WriteString( "null" );
					/*17+i+4.10*/packet.WriteString( "null" );
				}
				
				// spawn phases
				TDynArray<CName> spawnPhasesArray;
				if ( encounter->GetRootNode() )
					encounter->GetSpawnPhases( spawnPhasesArray );
				String spawnPhases = CDebugServerHelpers::GetStringFromArrayOfCNames( spawnPhasesArray );
				/*17+i+5.10*/packet.WriteString( spawnPhases.Size() ? UNICODE_TO_ANSI( spawnPhases.AsChar() ) : "None" );

				// send
				Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
				++packetsCount;
			}
		}
	}

	// end of encounter list
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	/*1*/packet.WriteString( "encounter" );
	Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );

	// processed
	return ++packetsCount;
}
//////////////////////////////////////////////////////////////////////////
//
// getencounter
Uint32 CDebugServerCommandGetEncounter::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	RED_UNUSED( owner );

	if ( !GGame->GetActiveWorld() || !DBGSRV_CALL( IsAttached() ) )
		return 0;

	PC_SCOPE_PIX( DbgSrv_Gdbg_GetEncounter );

	// get entity uid
	Uint64 uid = CDebugServerHelpers::GetUidFromString( data[0] );
	if ( !uid )
		return 0;
	
	CWorld* gworld = GGame->GetActiveWorld();
	WorldAttachedEntitiesIterator entIter( gworld );
	for ( ; entIter; ++entIter )
	{
		if ( reinterpret_cast<Uint64>( *entIter ) == uid )
		{
			if ( CEncounter* encounter = Cast<CEncounter>( *entIter ) )
			{
				// init
				CTriggerAreaComponent* triggerArea = encounter->GetTriggerArea();
				CEncounterParameters* params = encounter->GetEncounterParameters();

				// init packet
				Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
				/*1*/packet.WriteString( "encounterFull" );

				// base
				const Uint64 uid = reinterpret_cast<Uint64>( *entIter );
				/*2*/packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( uid ).AsChar() ) );
				/*3*/packet.WriteString( encounter->GetClass()->GetName().AsAnsiChar() );
				const Vector& pos = encounter->GetPosition();
				/*4*/packet.WriteString( UNICODE_TO_ANSI( String::Printf( TXT( "%0.3f %0.3f %0.3f %0.3f" ), pos.X, pos.Y, pos.Z, pos.W ).AsChar() ) );

				// names
				/*5*/packet.WriteString( UNICODE_TO_ANSI( encounter->GetDisplayName().AsChar() ) );
				/*6*/packet.WriteString( UNICODE_TO_ANSI( encounter->GetFriendlyName().AsChar() ) );
				/*7*/packet.WriteString( UNICODE_TO_ANSI( encounter->GetTemplate() ? encounter->GetTemplate()->GetFriendlyName().AsChar() : TXT("None") ) );

				// common
				/*8*/packet.WriteString( UNICODE_TO_ANSI( encounter->GetTags().ToString().AsChar() ) );
				/*9*/packet.WriteString( UNICODE_TO_ANSI( ToString( encounter->GetComponents().Size() ).AsChar() ) );

				// layer info
				const Uint64 layerUid = reinterpret_cast<Uint64>( encounter->GetLayer() );
				/*10.6*/packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( layerUid ).AsChar() ) );

				// flags
				/*11.10*/packet.WriteString( encounter->IsEnabled() ? "true" : "false" );
				/*12.10*/packet.WriteString( encounter->IsIgnoreAreaTrigger() ? "true" : "false" );
				/*13.10*/packet.WriteString( encounter->IsFullRespawnScheduled() ? "true" : "false" );
				/*14.10*/packet.WriteString( encounter->GetSpawnPoints().IsInitialized() ? "true" : "false" );
				/*15.10*/packet.WriteString( triggerArea ? (triggerArea->IsEnabled() ? "true" : "false") : "<trigger is NULL>" );		

				// trigger area shape
				const TDynArray<Vector>* points = encounter->GetWorldPoints();
				const Uint32 pointCount = points ? points->Size() : 0;
				/*16.10*/packet.WriteString( UNICODE_TO_ANSI( ToString( pointCount ).AsChar() ) );
				if ( pointCount && points )
				{
					for ( Uint32 i = 0; i < pointCount; ++i )
					{
						/*17.10 ... 17+i.10*/packet.WriteString( UNICODE_TO_ANSI( String::Printf( TXT( "%0.3f %0.3f" ), (*points)[i].X, (*points)[i].Y ).AsChar() ) );
					}
				}

				// encounter parameters
				Bool subParamsFound = false;
				Bool subParamsData = false;
				CEntity* guardAreaEntity = nullptr;
				if ( params )
				{
					TDynArray< THandle< IAISpawnTreeSubParameters > >& subParams = params->GetGlobalDefaults();
					if  ( !subParams.Empty() && subParams[0] )
					{
						IAISpawnTreeSubParameters* params = subParams[0].Get();
						// guard area params
						if ( params->IsA<CGuardAreaParameters>() )
						{
							/*17+i+1.10*/packet.WriteString( "CGuardAreaParameters" );

							// init
							CGuardAreaParameters* guardArea = static_cast<CGuardAreaParameters*>( params );
							subParamsFound = true;
							subParamsData = true;

							// guard area entity
							String guardAreaEntityString = TXT( "<no guard area entity>" );
							guardAreaEntity = guardArea->GetGuardArea();
							if ( guardAreaEntity )
							{
								guardAreaEntityString =	guardAreaEntity->GetFriendlyName() + TXT( " " ) +
									CDebugServerHelpers::GetStringFromUid( reinterpret_cast<Uint64>( guardAreaEntity ) );
							}
							/*17+i+2.10*/packet.WriteString( UNICODE_TO_ANSI( guardAreaEntityString.AsChar() ) );

							// guard area pursuit entity
							String guardAreaPursuitEntityString = TXT( "<no guard area pursuit entity>" );
							CEntity* guardAreaPursuitEntity = guardArea->GetGuardPursuitArea();
							if ( guardAreaPursuitEntity )
							{
								guardAreaPursuitEntityString = guardAreaPursuitEntity->GetFriendlyName() + TXT( " " ) +
									CDebugServerHelpers::GetStringFromUid( reinterpret_cast<Uint64>( guardAreaPursuitEntity ) );
							}
							/*17+i+3.10*/packet.WriteString( UNICODE_TO_ANSI( guardAreaPursuitEntityString.AsChar() ) );

							// guard area pursuit range
							/*17+i+4.10*/packet.WriteString( UNICODE_TO_ANSI( ToString( guardArea->GetGuardPursuitRange() ).AsChar() ) );
						}

						// idle beh default params
						else if ( params->IsA<CIdleBehaviorsDefaultParameters>() )
						{
							/*17+i+1.10*/packet.WriteString( "CIdleBehaviorsDefaultParameters" );

							// init
							CIdleBehaviorsDefaultParameters* idleBehDef = static_cast<CIdleBehaviorsDefaultParameters*>( params );
							subParamsFound = true;
							subParamsData = true;

							// action points area entity
							String actionPointaAreaEntityString = TXT( "<no action points area entity>" );
							CEntity* actionPointaAreaEntity = idleBehDef->GetActionPointsArea();
							if ( actionPointaAreaEntity )
							{
								actionPointaAreaEntityString = actionPointaAreaEntity->GetFriendlyName() + TXT( " " ) +
									CDebugServerHelpers::GetStringFromUid( reinterpret_cast<Uint64>( actionPointaAreaEntity ) ) + TXT( " " );
							}
							/*17+i+2.10*/packet.WriteString( UNICODE_TO_ANSI( actionPointaAreaEntityString.AsChar() ) );

							// wander area entity
							String wanderAreaEntityString = TXT( "<no wander area entity>" );
							CEntity* wanderAreaEntity = idleBehDef->GetWanderArea();
							if ( wanderAreaEntity )
							{
								wanderAreaEntityString = wanderAreaEntity->GetFriendlyName() + TXT( " " ) +
									CDebugServerHelpers::GetStringFromUid( reinterpret_cast<Uint64>( wanderAreaEntity ) ) + TXT( " " );
							}
							/*17+i+3.10*/packet.WriteString( UNICODE_TO_ANSI( wanderAreaEntityString.AsChar() ) );

							// wander points tag
							/*17+i+4.10*/packet.WriteString( idleBehDef->GetWanderPointsTag().AsAnsiChar() );
						}

						// unknown params
						else
						{
							/*17+i+1.10*/packet.WriteString( subParams[0]->GetClass()->GetName().AsAnsiChar() );
							subParamsFound = true;
						}
					}

					// null params
					if ( !subParams.Empty() && !subParams[0] )
					{
						/*17+i+1.10*/packet.WriteString( "[null params]" );
						subParamsFound = true;

						WARN_ENGINE( TXT( "Missing [null] encounter params: %s" ), encounter->GetFriendlyName().AsChar() );
					}
				}
				if ( !subParamsFound )
				{
					/*17+i+1.10*/packet.WriteString( "[no params]" );
				}
				if ( !subParamsData )
				{
					/*17+i+2.10*/packet.WriteString( "null" );
					/*17+i+3.10*/packet.WriteString( "null" );
					/*17+i+4.10*/packet.WriteString( "null" );
				}

				// spawn phases
				TDynArray<CName> spawnPhasesArray;
				if ( encounter->GetRootNode() )
					encounter->GetSpawnPhases( spawnPhasesArray );
				String spawnPhases = CDebugServerHelpers::GetStringFromArrayOfCNames( spawnPhasesArray );
				/*17+i+5.10*/packet.WriteString( spawnPhases.Size() ? UNICODE_TO_ANSI( spawnPhases.AsChar() ) : "None" );

				// send
				Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );

				// processed
				return 1;
			}
			break;
		}
	}

	// not found
	return 0;
}
//////////////////////////////////////////////////////////////////////////
//
// getcharactercontroller - get character controller data
Uint32 CDebugServerCommandGetCharacterController::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	RED_UNUSED( owner );

	if ( data.Size() == 0 || data[0].GetLength() == 0 || !DBGSRV_CALL( IsAttached() ) )
		return 0;

	// get entity uid
	Uint64 uid = CDebugServerHelpers::GetUidFromString( data[0] );

	// check for entity
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

	// get player
	else
	{
		actor = Cast<CGameplayEntity>( GGame->GetPlayerEntity() );
	}

	if ( !actor )
		return 0;

	CAnimatedComponent* animComponent = actor->GetRootAnimatedComponent();
	if ( animComponent )
	{
		CMovingPhysicalAgentComponent* mpac = Cast<CMovingPhysicalAgentComponent>( animComponent );
		if ( mpac )
		{
			// init
			Bool isCharacterControllerReady = false;
			Bool isRagdollOnGround = false;
			Bool isRagdollReady = false;
			Uint64 uid = 0;
#ifdef USE_PHYSX

			CPhysicsCharacterWrapper* ccWrapper = mpac->GetPhysicalCharacter()->GetCharacterController();
			CPhysicsRagdollWrapper* ragdollWrapper = animComponent->GetRagdollPhysicsWrapper();
			Vector ragdollPos, ragdollDelta, ragdollDist = Vector::ZEROS;
			uid = reinterpret_cast<Uint64>( ccWrapper );
			isCharacterControllerReady = ccWrapper->IsReady();
			isRagdollReady = ragdollWrapper != nullptr && ragdollWrapper->IsReady();
			isRagdollOnGround = ccWrapper->IsRagdollOnGround();

			if ( ccWrapper && ccWrapper->GetRagdollData( ragdollPos, ragdollDelta ) )
			{
				ragdollDist = actor->GetPosition() - ragdollPos;
			}
			else
#endif
			{
				ragdollPos = Vector::ZEROS;
				ragdollDelta = Vector::ZEROS;
			}

			// init packet
			Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
			/*1*/packet.WriteString( "charactercontroller" );

			// fill data
			// uid
			/*2*/packet.WriteString( UNICODE_TO_ANSI( data[0].AsChar() ) );

			/*3*/packet.WriteString( UNICODE_TO_ANSI( ToString( (Uint32)mpac->GetCurrentPhysicsState() ).AsChar() ) );
			/*4*/packet.WriteString( UNICODE_TO_ANSI( ToString( mpac->GetWaterLevel() ).AsChar() ) );
			/*5*/packet.WriteString( UNICODE_TO_ANSI( ToString( mpac->GetSubmergeDepth() ).AsChar() ) );
			/*6*/packet.WriteString( UNICODE_TO_ANSI( ToString( (Uint32)mpac->GetInteractionPriority() ).AsChar() ) );
			/*7*/packet.WriteString( UNICODE_TO_ANSI( ToString( (Uint32)mpac->GetOriginalInteractionPriority() ).AsChar() ) );
			/*8*/packet.WriteString( UNICODE_TO_ANSI( ToString( mpac->GetVirtualRadius() ).AsChar() ) );
			/*9*/packet.WriteString( UNICODE_TO_ANSI( ToString( mpac->GetPhysicalRadius() ).AsChar() ) );
			/*10*/packet.WriteString( UNICODE_TO_ANSI( ToString( mpac->GetCurrentRadius() ).AsChar() ) );
			/*11*/packet.WriteString( UNICODE_TO_ANSI( ToString( mpac->GetHeight() ).AsChar() ) );
			/*12*/packet.WriteString( UNICODE_TO_ANSI( ToString( mpac->GetSlopePitch() ).AsChar() ) );
			/*13*/packet.WriteString( UNICODE_TO_ANSI( ToString( mpac->GetPhysicalPitch() ).AsChar() ) );
			/*14*/packet.WriteString( UNICODE_TO_ANSI( ToString( mpac->GetTerrainNormal() ).AsChar() ) );
			/*15*/packet.WriteString( UNICODE_TO_ANSI( ToString( mpac->GetRagdollPushMultiplier() ).AsChar() ) );
			/*16*/packet.WriteString( UNICODE_TO_ANSI( ToString( mpac->GetEmergeSpeed() ).AsChar() ) );
			/*17*/packet.WriteString( UNICODE_TO_ANSI( ToString( mpac->GetSubmergeSpeed() ).AsChar() ) );
			/*18*/packet.WriteString( UNICODE_TO_ANSI( ToString( mpac->GetSpeedMul() ).AsChar() ) );
			/*19*/packet.WriteString( UNICODE_TO_ANSI( ToString( mpac->GetSlidingState() ).AsChar() ) );
			/*20*/packet.WriteString( UNICODE_TO_ANSI( ToString( mpac->GetSlideCoef() ).AsChar() ) );
			/*21*/packet.WriteString( UNICODE_TO_ANSI( ToString( mpac->GetSlidingDir() ).AsChar() ) );
			/*22*/packet.WriteString( UNICODE_TO_ANSI( ToString( mpac->GetVirtualControllerCount() ).AsChar() ) );
			/*23*/packet.WriteString( UNICODE_TO_ANSI( ToString( mpac->GetLastMoveVector() ).AsChar() ) );
			/*24*/packet.WriteString( UNICODE_TO_ANSI( ToString( mpac->GetCurrentMovementVectorRef() ).AsChar() ) );
			/*25*/packet.WriteString( UNICODE_TO_ANSI( ToString( mpac->GetInternalVelocity() ).AsChar() ) );
			/*26*/packet.WriteString( UNICODE_TO_ANSI( ToString( mpac->GetExternalDisp() ).AsChar() ) );
			/*27*/packet.WriteString( UNICODE_TO_ANSI( ToString( mpac->GetInputDisp() ).AsChar() ) );
			/*28*/packet.WriteString( UNICODE_TO_ANSI( ToString( mpac->GetPlatformLocalPos() ).AsChar() ) );
			/*29*/packet.WriteString( UNICODE_TO_ANSI( ToString( mpac->GetPlatformRotation() ).AsChar() ) );
			/*30*/packet.WriteString( UNICODE_TO_ANSI( ToString( mpac->GetPushingTime() ).AsChar() ) );
			/*31*/packet.WriteString( mpac->IsPhysicalMovementEnabled() ? "true" : "false" );
			/*32*/packet.WriteString( mpac->IsAnimatedMovement() ? "true" : "false" );
			/*33*/packet.WriteString( mpac->IsGravity() ? "true" : "false" );
			/*34*/packet.WriteString( mpac->IsBehaviorCallbackNeeded() ? "true" : "false" );
			/*35*/packet.WriteString( mpac->IsAdditionalVerticalSlidingIterationEnabled() ? "true" : "false" );
			/*36*/packet.WriteString( mpac->IsDiving() ? "true" : "false" );
			/*37*/packet.WriteString( mpac->IsSwimming() ? "true" : "false" );
			/*38*/packet.WriteString( mpac->IsCharacterCollisionsEnabled() ? "true" : "false" );
			/*39*/packet.WriteString( mpac->IsStaticCollisionsEnabled() ? "true" : "false" );
			/*40*/packet.WriteString( mpac->IsDynamicCollisionsEnabled() ? "true" : "false" );
			/*41*/packet.WriteString( mpac->IsCollidingDown() ? "true" : "false" );
			/*42*/packet.WriteString( mpac->IsCollidingUp() ? "true" : "false" );
			/*43*/packet.WriteString( mpac->IsCollidingSide() ? "true" : "false" );
			/*44*/packet.WriteString( mpac->CanPhysicalMove() ? "true" : "false" );
			/*45*/packet.WriteString( mpac->ShouldPhysicalMove() ? "true" : "false" );
			/*46*/packet.WriteString( mpac->IsFalling() ? "true" : "false" );
			/*47*/packet.WriteString( mpac->IsTeleport() ? "true" : "false" );
			/*48*/packet.WriteString( mpac->IsStandingOnDynamic() ? "true" : "false" );
			/*49*/packet.WriteString( mpac->IsSliding() ? "true" : "false" );
			/*50*/packet.WriteString( mpac->IsOnPlatform() ? "true" : "false" );
			/*51*/packet.WriteString( mpac->IsCollisionPredictionEnabled() ? "true" : "false" );
			/*52*/packet.WriteString( mpac->IsNearWater() ? "true" : "false" );
			/*53*/packet.WriteString( mpac->CanPush() ? "true" : "false" );
			/*54*/packet.WriteString( mpac->IsShapeHit() ? "true" : "false" );
			/*55*/packet.WriteString( mpac->IsSlidingEnabled() ? "true" : "false" );
			/*56*/packet.WriteString( mpac->IsUpdatingVirtualRadius() ? "true" : "false" );

			/*57*/packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( uid ).AsChar() ) );
			/*58*/packet.WriteString( ccWrapper->IsReady() ? "true" : "false" );
			/*58*/packet.WriteString(  isCharacterControllerReady ? "true" : "false" );
#ifdef USE_PHYSX
			/*58*/packet.WriteString( ccWrapper->IsReady() ? "true" : "false" );
#endif
			// ragdoll
			/*59*/packet.WriteString( isRagdollReady ? "true" : "false" );
			/*60*/packet.WriteString( UNICODE_TO_ANSI( ToString( ragdollPos ).AsChar() ) );
			/*61*/packet.WriteString( UNICODE_TO_ANSI( ToString( ragdollDelta ).AsChar() ) );
			/*62*/packet.WriteString( UNICODE_TO_ANSI( ( ToString( ragdollDist ) + TXT( " (" ) + ToString( ragdollDist.Mag3() ) + TXT( ")" ) ).AsChar() ) );
#ifdef USE_PHYSX
			/*63*/packet.WriteString( ccWrapper->IsRagdollOnGround() ? "true" : "false" );
#endif
			// collision grid
			for ( Uint32 side = 0; side < 9; ++side )
			{
				/*64..64+side*/packet.WriteString( mpac->GetGroundGridCollisionOn(side) ? "true" : "false" );
			}

			// send
			Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
		}
	}

	// processed
	return 1;
}
//////////////////////////////////////////////////////////////////////////
//
// getdoors
Uint32 CDebugServerCommandGetDoors::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	RED_UNUSED( owner );

	if ( !GGame->GetActiveWorld() || !DBGSRV_CALL( IsAttached() ) )
		return 0;

	PC_SCOPE_PIX( DbgSrv_Gdbg_GetDoors );

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
				Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
				/*1*/packet.WriteString( "door" );

				// base
				const Uint64 uid = reinterpret_cast<Uint64>( *entIter );
				/*2*/packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( uid ).AsChar() ) );
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

				// layer info
				const Uint64 layerUid = reinterpret_cast<Uint64>( (*entIter)->GetLayer() );
				/*12.6*/packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( layerUid ).AsChar() ) );

				// send
				Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
				++packetsCount;
			}
		}
	}

	// end of doors list
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	/*1*/packet.WriteString( "door" );
	Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );

	// processed
	return ++packetsCount;
}
//////////////////////////////////////////////////////////////////////////
//
// getarea
Uint32 CDebugServerCommandGetArea::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	RED_UNUSED( owner );

	if ( !GGame->GetActiveWorld() || !DBGSRV_CALL( IsAttached() ) )
		return 0;
	if ( data.Size() == 0 || data[0].GetLength() == 0  )
		return 0;

	PC_SCOPE_PIX( DbgSrv_Gdbg_GetArea );

	// get entity uid
	Uint64 uid = CDebugServerHelpers::GetUidFromString( data[0] );
	if ( !uid )
		return 0;

	// looking for area component
	WorldAttachedEntitiesIterator entIter( GGame->GetActiveWorld() );
	for ( ; entIter; ++entIter )
	{
		if ( reinterpret_cast<Uint64>( *entIter ) == uid )
		{
			ComponentIterator<CAreaComponent> it( *entIter );
			if ( it )
			{
				// init
				CAreaComponent* area = static_cast<CAreaComponent*>( *it );
				const CAreaComponent::TAreaPoints& points = area->GetWorldPoints();
				const Uint32 pointCount = points.Size();

				// init packet
				Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
				/*1*/packet.WriteString( "area" );

				// base
				const Uint64 uid = reinterpret_cast<Uint64>( *entIter );
				/*2*/packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( uid ).AsChar() ) );
				/*3*/packet.WriteString( (*it)->GetClass()->GetName().AsAnsiChar() );
				const Vector& pos = (*entIter)->GetPosition();
				/*4*/packet.WriteString( UNICODE_TO_ANSI( String::Printf( TXT( "%0.3f %0.3f %0.3f %0.3f" ), pos.X, pos.Y, pos.Z, pos.W ).AsChar() ) );
				const Uint64 layerUid = reinterpret_cast<Uint64>( (*entIter)->GetLayer() );
				/*5*/packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( layerUid ).AsChar() ) );


				// names
				/*6*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetDisplayName().AsChar() ) );
				/*7*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetFriendlyName().AsChar() ) );
				/*8*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetTemplate() ? (*entIter)->GetTemplate()->GetFriendlyName().AsChar() : TXT("None") ) );

				// common
				/*9*/packet.WriteString( UNICODE_TO_ANSI( (*entIter)->GetTags().ToString().AsChar() ) );

				// area
				/*10*/packet.WriteString( UNICODE_TO_ANSI( ToString( pointCount ).AsChar() ) );
				if ( pointCount )
				{
					for ( Uint32 j = 0; j < pointCount; ++j )
					{
						/*10 ... 10+j*/packet.WriteString( UNICODE_TO_ANSI( String::Printf( TXT( "%0.3f %0.3f" ), points[j].X, points[j].Y ).AsChar() ) );
					}
				}

				// send
				Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );

				// processed
				return 1;
			}
			break;
		}
	}

	// not found
	return 0;
}

#endif


//////////////////////////////////////////////////////////////////////////
// EOF
