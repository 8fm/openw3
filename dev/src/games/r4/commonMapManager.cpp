/**
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "commonMapManager.h"

#include "../../common/core/contentManager.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/core/depot.h"
#include "../../common/core/feedback.h"
#include "../../common/core/gameSave.h"

#if !defined( NO_SECOND_SCREEN )
#include "../../common/platformCommon/secondScreenManager.h"
#endif

#include "../../common/engine/layerGroup.h"
#include "../../common/engine/tagManager.h"
#include "../../common/engine/renderFrame.h"
#include "../../common/engine/drawableComponent.h"
#include "../../common/engine/pathComponent.h"

#include "../../common/game/gameWorld.h"
#include "../../common/game/hud.h"
#include "../../common/game/communityAgentStub.h"
#include "../../common/game/communityAgentStubTagManager.h"
#include "../../common/game/factsDB.h"

#include "focusModeController.h"
#include "mapFastTravel.h"
#include "r4GuiManager.h"
#include "r4interiorAreaComponent.h"
#include "r4MapTracking.h"
#include "r4Player.h"
#include "w3boat.h"

IMPLEMENT_ENGINE_CLASS( SMapPathDefinition )
IMPLEMENT_ENGINE_CLASS( SMapPathInstance )
IMPLEMENT_ENGINE_CLASS( SCustomMapPinDefinition )

// script functions
RED_DEFINE_STATIC_NAME( AddWaypointsToMinimap )
RED_DEFINE_STATIC_NAME( ClearWaypointsInMinimap )
RED_DEFINE_STATIC_NAME( NotifyPlayerEnteredInterior )
RED_DEFINE_STATIC_NAME( NotifyPlayerExitedInterior )
RED_DEFINE_STATIC_NAME( GetCustomMapPinDefinition )
RED_DEFINE_STATIC_NAME( GetKnowableMapPinTypes )
RED_DEFINE_STATIC_NAME( GetDiscoverableMapPinTypes )
RED_DEFINE_STATIC_NAME( GetDisableableMapPinTypes )
RED_DEFINE_STATIC_NAME( GetCompanion )
RED_DEFINE_STATIC_NAME( GetUsedVehicle )
RED_DEFINE_STATIC_NAME( GetHorseUser )
RED_DEFINE_STATIC_NAME( GetAreaMappinsFileName )
RED_DEFINE_STATIC_NAME( GetAreaMappinsData )
RED_DEFINE_STATIC_NAME( AddMapPathToMinimap )
RED_DEFINE_STATIC_NAME( DeleteMapPathsFromMinimap )
RED_DEFINE_STATIC_NAME( GetStaticMapPinType )
RED_DEFINE_STATIC_NAME( GetStaticMapPinTag )
RED_DEFINE_STATIC_NAME( UpdateDistanceToHighlightedMapPin )
RED_DEFINE_STATIC_NAME( OnStartTeleportingPlayerToPlayableArea )
RED_DEFINE_STATIC_NAME( GetCurrentJournalArea )
RED_DEFINE_STATIC_NAME( GetScriptInfo )
RED_DEFINE_STATIC_NAME( IsWeaponRepairEntity )
RED_DEFINE_STATIC_NAME( GetRegionType )
RED_DEFINE_STATIC_NAME( GetEntityType )
RED_DEFINE_STATIC_NAME( IsForcedToBeDiscoverable )

// events
RED_DEFINE_STATIC_NAME( StaticMapPinUsed );
RED_DEFINE_STATIC_NAME( OnMapPinChanged );

// entity types
RED_DEFINE_STATIC_NAME( CMonsterNestEntity )
RED_DEFINE_STATIC_NAME( CMajorPlaceOfPowerEntity )
RED_DEFINE_STATIC_NAME( W3NoticeBoard );
RED_DEFINE_STATIC_NAME( W3Stash );
RED_DEFINE_STATIC_NAME( W3FastTravelEntity );
RED_DEFINE_STATIC_NAME( W3MagicLampEntity );
RED_DEFINE_STATIC_NAME( W3TreasureHuntMappinEntity );
RED_DEFINE_STATIC_NAME( CTeleportEntity )
RED_DEFINE_STATIC_NAME( CRiftEntity )
RED_DEFINE_STATIC_NAME( W3ItemRepairObject )
RED_DEFINE_STATIC_NAME( W3MutagenDismantlingTable )
RED_DEFINE_STATIC_NAME( W3AlchemyTable )
RED_DEFINE_STATIC_NAME( W3Stables )
RED_DEFINE_STATIC_NAME( W3Bookshelf )
RED_DEFINE_STATIC_NAME( W3WitcherBed )
RED_DEFINE_STATIC_NAME( W3WitcherHouse )
RED_DEFINE_STATIC_NAME( W3EntranceEntity )
RED_DEFINE_STATIC_NAME( W3Herb )
RED_DEFINE_STATIC_NAME( W3ActorRemains )
RED_DEFINE_STATIC_NAME( W3MonsterClue )
RED_DEFINE_STATIC_NAME( W3POI_SpoilsOfWarEntity )
RED_DEFINE_STATIC_NAME( W3POI_BanditCampEntity )
RED_DEFINE_STATIC_NAME( W3POI_BanditCampfireEntity )
RED_DEFINE_STATIC_NAME( W3POI_BossAndTreasureEntity )
RED_DEFINE_STATIC_NAME( W3POI_ContrabandEntity )
RED_DEFINE_STATIC_NAME( W3POI_ContrabandShipEntity )
RED_DEFINE_STATIC_NAME( W3POI_RescuingTownEntity )
RED_DEFINE_STATIC_NAME( W3POI_DungeonCrawlEntity )
RED_DEFINE_STATIC_NAME( W3POI_HideoutEntity )
RED_DEFINE_STATIC_NAME( W3POI_PlegmundEntity )
RED_DEFINE_STATIC_NAME( W3POI_KnightErrantEntity )
RED_DEFINE_STATIC_NAME( W3POI_WineContractEntity )
RED_DEFINE_STATIC_NAME( W3POI_SignalingStakeEntity )

// savegames
RED_DEFINE_STATIC_NAME( QuestMapPinStates )
RED_DEFINE_STATIC_NAME( QuestMapPinState )
RED_DEFINE_STATIC_NAME( ObjectiveGuid )
RED_DEFINE_STATIC_NAME( MapPinGuid )
RED_DEFINE_STATIC_NAME( MapPinState )
RED_DEFINE_STATIC_NAME( KnownMapPinTags )
RED_DEFINE_STATIC_NAME( KnownMapPinTag )
RED_DEFINE_STATIC_NAME( DiscoveredMapPinTags )
RED_DEFINE_STATIC_NAME( DiscoveredMapPinTag )
RED_DEFINE_STATIC_NAME( MapPinTag )
RED_DEFINE_STATIC_NAME( DisabledMapPinTags )
RED_DEFINE_STATIC_NAME( DisabledMapPinTag )
RED_DEFINE_STATIC_NAME( DiscoveredAgentEntityTags )
RED_DEFINE_STATIC_NAME( DiscoveredAgentEntityTag )
RED_DEFINE_STATIC_NAME( CustomEntityMapPins )
RED_DEFINE_STATIC_NAME( CustomEntityMapPin )
RED_DEFINE_STATIC_NAME( CustomEntityMapPinTag )
RED_DEFINE_STATIC_NAME( CustomEntityMapPinType )
RED_DEFINE_STATIC_NAME( CustomAgentMapPins )
RED_DEFINE_STATIC_NAME( CustomAgentMapPin )
RED_DEFINE_STATIC_NAME( CustomAgentMapPinTag )
RED_DEFINE_STATIC_NAME( CustomAgentMapPinType )
RED_DEFINE_STATIC_NAME( DiscoveredPaths )
RED_DEFINE_STATIC_NAME( DiscoveredPath )
RED_DEFINE_STATIC_NAME( DiscoveredPathTag )
RED_DEFINE_STATIC_NAME( DiscoveredPathLineWidth )
RED_DEFINE_STATIC_NAME( DiscoveredPathLineSegmentLength )
RED_DEFINE_STATIC_NAME( DiscoveredPathColor )
RED_DEFINE_STATIC_NAME( UserMapPin )
RED_DEFINE_STATIC_NAME( UserMapPinsCount )
RED_DEFINE_STATIC_NAME( UserMapPinArea )
RED_DEFINE_STATIC_NAME( UserMapPinPosition )
RED_DEFINE_STATIC_NAME( UserMapPinType )
RED_DEFINE_STATIC_NAME( CachedWorldDataMap )
RED_DEFINE_STATIC_NAME( CachedWorldData )
RED_DEFINE_STATIC_NAME( CachedWorldDataPath )
RED_DEFINE_STATIC_NAME( CachedWorldVisited )
RED_DEFINE_STATIC_NAME( CachedQuestMapPinsMap )
RED_DEFINE_STATIC_NAME( CachedQuestMapPin )
RED_DEFINE_STATIC_NAME( CachedQuestMapPinTag )
RED_DEFINE_STATIC_NAME( CachedQuestMapPinType )
RED_DEFINE_STATIC_NAME( CachedQuestMapPinRadius )
RED_DEFINE_STATIC_NAME( CachedQuestMapPinPositions )
RED_DEFINE_STATIC_NAME( CachedShopkeeperDataMap )
RED_DEFINE_STATIC_NAME( CachedShopkeeperData )
RED_DEFINE_STATIC_NAME( CachedShopkeeperDataTag )
RED_DEFINE_STATIC_NAME( CachedShopkeeperDataId )
RED_DEFINE_STATIC_NAME( CachedShopkeeperDataType )
RED_DEFINE_STATIC_NAME( CachedShopkeeperDataPosition )
RED_DEFINE_STATIC_NAME( CachedShopkeeperDataInitialized )
RED_DEFINE_STATIC_NAME( CachedShopkeeperDataEnabled )
RED_DEFINE_STATIC_NAME( CachedBoatDataArray )
RED_DEFINE_STATIC_NAME( CachedBoatData )
RED_DEFINE_STATIC_NAME( CachedBoatDataPosition )

// other
RED_DEFINE_STATIC_NAME( entranceTag )
RED_DEFINE_STATIC_NAME( NoMapPin )
RED_DEFINE_STATIC_NAME( ShopkeeperEntity )
RED_DEFINE_STATIC_NAME( W3MerchantComponent )

// pin types
RED_DEFINE_NAME( Enemy )
RED_DEFINE_NAME( EnemyDead )
RED_DEFINE_NAME( Horse )
RED_DEFINE_NAME( MonsterNest )
RED_DEFINE_NAME( InfestedVineyard )
RED_DEFINE_NAME( PlaceOfPower )
RED_DEFINE_NAME( GenericFocus )
RED_DEFINE_NAME( Companion )
RED_DEFINE_NAME( Rift )
RED_DEFINE_NAME( Whetstone )
RED_DEFINE_NAME( ArmorRepairTable )
RED_DEFINE_NAME( MutagenDismantle )
RED_DEFINE_NAME( AlchemyTable )
RED_DEFINE_NAME( Stables )
RED_DEFINE_NAME( Bookshelf )
RED_DEFINE_NAME( Bed )
RED_DEFINE_NAME( WitcherHouse )
RED_DEFINE_NAME( Entrance )
RED_DEFINE_NAME( Herb )
RED_DEFINE_NAME( User1 )
RED_DEFINE_NAME( User2 )
RED_DEFINE_NAME( User3 )
RED_DEFINE_NAME( User4 )
RED_DEFINE_NAME( RoadSign );
RED_DEFINE_NAME( Harbor );
RED_DEFINE_NAME( HubExit );
RED_DEFINE_NAME( NoticeBoard )
RED_DEFINE_NAME( NoticeBoardFull )
RED_DEFINE_NAME( PlayerStash )
RED_DEFINE_NAME( PlayerStashDiscoverable )
RED_DEFINE_NAME( QuestLoot );
RED_DEFINE_NAME( StoryQuest )
RED_DEFINE_NAME( ChapterQuest )
RED_DEFINE_NAME( SideQuest )
RED_DEFINE_NAME( MonsterQuest )
RED_DEFINE_NAME( TreasureQuest )
RED_DEFINE_NAME( QuestReturn )
RED_DEFINE_NAME( HorseRace )
RED_DEFINE_NAME( BoatRace )
RED_DEFINE_NAME( QuestBelgard )
RED_DEFINE_NAME( QuestCoronata )
RED_DEFINE_NAME( QuestVermentino )
RED_DEFINE_NAME( QuestAvailable )
RED_DEFINE_NAME( MagicLamp )
//RED_DEFINE_NAME( PointOfInterestMappin )
RED_DEFINE_NAME( NotDiscoveredPOI )
RED_DEFINE_NAME( TreasureHuntMappin )
RED_DEFINE_NAME( TreasureHuntMappinDisabled )
RED_DEFINE_NAME( SpoilsOfWar )
RED_DEFINE_NAME( SpoilsOfWarDisabled )
RED_DEFINE_NAME( BanditCamp )
RED_DEFINE_NAME( BanditCampDisabled )
RED_DEFINE_NAME( BanditCampfire )
RED_DEFINE_NAME( BanditCampfireDisabled )
RED_DEFINE_NAME( BossAndTreasure )
RED_DEFINE_NAME( BossAndTreasureDisabled )
RED_DEFINE_NAME( Contraband )
RED_DEFINE_NAME( ContrabandDisabled )
RED_DEFINE_NAME( ContrabandShip )
RED_DEFINE_NAME( ContrabandShipDisabled )
RED_DEFINE_NAME( RescuingTown )
RED_DEFINE_NAME( RescuingTownDisabled )
RED_DEFINE_NAME( DungeonCrawl )
RED_DEFINE_NAME( DungeonCrawlDisabled )
RED_DEFINE_NAME( Hideout )
RED_DEFINE_NAME( HideoutDisabled )
RED_DEFINE_NAME( Plegmund )
RED_DEFINE_NAME( PlegmundDisabled )
RED_DEFINE_NAME( KnightErrant )
RED_DEFINE_NAME( KnightErrantDisabled )
RED_DEFINE_NAME( WineContract )
RED_DEFINE_NAME( WineContractDisabled )
RED_DEFINE_NAME( SignalingStake )
RED_DEFINE_NAME( SignalingStakeDisabled )

RED_DEFINE_NAME( MapPinFilterHiddenList )
RED_DEFINE_NAME( MapPinFilterHiddenSpecific )
RED_DEFINE_NAME( MapPinFilterHiddenName )
RED_DEFINE_NAME( MapBorderData )

CGatheredResource resAreaMappins( TXT("game\\world.w2am"), RGF_Startup );

CGatheredResource resHubEntityMappinsNovigrad(      TXT("game\\hub_pins\\novigrad.w2em"), RGF_Startup );
CGatheredResource resHubEntityMappinsSkellige(      TXT("game\\hub_pins\\skellige.w2em"), RGF_Startup );
CGatheredResource resHubEntityMappinsKaerMohren(    TXT("game\\hub_pins\\kaer_morhen.w2em"), RGF_Startup );
CGatheredResource resHubEntityMappinsPrologVillage( TXT("game\\hub_pins\\prolog_village.w2em"), RGF_Startup );
CGatheredResource resHubEntityMappinsWyzima(        TXT("game\\hub_pins\\wyzima_castle.w2em"), RGF_Startup );
CGatheredResource resHubEntityMappinsIslandOfMist(  TXT("game\\hub_pins\\island_of_mist.w2em"), RGF_Startup );
CGatheredResource resHubEntityMappinsSpiral(        TXT("game\\hub_pins\\spiral.w2em"), RGF_Startup );

CGatheredResource resHubQuestMappinsNovigrad(      TXT("game\\hub_pins\\novigrad.w2qm"), RGF_Startup );
CGatheredResource resHubQuestMappinsSkellige(      TXT("game\\hub_pins\\skellige.w2qm"), RGF_Startup );
CGatheredResource resHubQuestMappinsKaerMohren(    TXT("game\\hub_pins\\kaer_morhen.w2qm"), RGF_Startup );
CGatheredResource resHubQuestMappinsPrologVillage( TXT("game\\hub_pins\\prolog_village.w2qm"), RGF_Startup );
CGatheredResource resHubQuestMappinsWyzima(        TXT("game\\hub_pins\\wyzima_castle.w2qm"), RGF_Startup );
CGatheredResource resHubQuestMappinsIslandOfMist(  TXT("game\\hub_pins\\island_of_mist.w2qm"), RGF_Startup );
CGatheredResource resHubQuestMappinsSpiral(        TXT("game\\hub_pins\\spiral.w2qm"), RGF_Startup );

#pragma optimize( "", off )

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

CCachedWorldData::CCachedWorldData()
	: m_visited( false )
{
}

void CCachedWorldData::Initialize( const String& worldName )
{
	m_worldName = worldName;

	LoadEntityMapPins();
	LoadQuestMapPins();
}

void CCachedWorldData::Uninitialize()
{
	m_visited = false;
	m_worldName.ClearFast();
	m_entityMapPinsLoadedForDLC.ClearFast();
	m_questMapPinsLoadedForDLC.ClearFast();
	m_entityMapPins.ClearFast();
	m_staticQuestMapPins.ClearFast();
	m_cachedQuestMapPins.ClearFast();
	m_cachedShopkeepers.ClearFast();
	m_cachedNoticeboardMapPins.ClearFast();
	m_cachedBoatMapPins.ClearFast();
}

void CCachedWorldData::EnableShopkeeper( const CName& tag, Bool enable )
{
	CCachedShopkeeperData* shopkeeperData = m_cachedShopkeepers.FindPtr( tag );
	if ( shopkeeperData )
	{
		// enabling/disabling shopkeeper
		shopkeeperData->Enable( enable );
	}
	else
	{
		// adding new shopkeeper data, because there's new entity found (it's enabled by default, not initialized)
		AddNewShopkeeperData( tag, 0, CName::NONE, nullptr, Vector::ZEROS, enable, false );
	}
}

//
//RED_DEFINE_STATIC_NAME( keira_metz )
//

void CCachedWorldData::UpdateShopkeeperData( const CNewNPC* entity, CScriptedComponent* component )
{
	if ( !entity || !component )
	{
		return;
	}
	const TagList& tags = entity->GetTags();
	if ( tags.Size() == 0 )
	{
		return;
	}
	const CName& tag = tags.GetTag( 0 );
	if ( tag == CNAME( ShopkeeperEntity ) )
	{
		// just in case
		return;
	}

	//
	//if ( tag == CNAME( keira_metz ) )
	//{
	//	RED_LOG( RED_LOG_CHANNEL( asdfghj ), TXT("UpdateShopkeeperData") );
	//}
	//

	CCachedShopkeeperData* shopkeeperData = m_cachedShopkeepers.FindPtr( tag );
	if ( shopkeeperData )
	{
		// just updating shopkeeper position
		shopkeeperData->SetEntityAndPosition( entity, entity->GetWorldPositionRef() );
	}
	else
	{
		// adding new shopkeeper data, because there's new entity found (it's enabled by default, initialized)
		AddNewShopkeeperData( tag, 0, CName::NONE, entity, entity->GetWorldPositionRef(), true, true );
		shopkeeperData = m_cachedShopkeepers.FindPtr( tag );
	}

	if ( shopkeeperData && ( shopkeeperData->GetType() == CName::NONE ) )
	{
		CName type;
		Bool cacheable;
		if ( CallFunctionRef( component, CNAME( GetScriptInfo ), type, cacheable ) )
		{
			shopkeeperData->SetType( type );
			shopkeeperData->SetCacheable( cacheable );
		}
	}
}

void CCachedWorldData::UpdateShopkeeperAgents()
{
	CCommunitySystem* communitySystem = GCommonGame->GetSystem< CCommunitySystem >();
	if ( !communitySystem )
	{
		return;
	}

	CCommunityAgentStubTagManager* agentStubTagManager = communitySystem->GetAgentStubTagManager();
	if ( !agentStubTagManager )
	{
		return;
	}

	TDynArray< SAgentStub* > agentStubs;
	for ( THashMap< CName, CCachedShopkeeperData >::iterator it = m_cachedShopkeepers.Begin(); it != m_cachedShopkeepers.End(); ++it )
	{
		CCachedShopkeeperData& data = it->m_second;

		//
		//if ( data.GetTag() == CNAME( keira_metz ) )
		//{
		//	RED_LOG( RED_LOG_CHANNEL( asdfghj ), TXT("UpdateShopkeeperAgents") );
		//}
		//

		if ( data.GetEntityExists() && !data.GetEntity() )
		{
			agentStubs.ClearFast();
			agentStubTagManager->CollectTaggedStubs( it->m_first, agentStubs );
			if ( agentStubs.Empty() )
			{
				// no entity and no agent, deinitialize
				data.SetInitialized( false );
				data.SetEntityExists( false );
			}
			else
			{
				// no entity, set position from agent
				// but make sure position is different from [0, 0, 0]
				const Vector3& currentPosition = agentStubs[ 0 ]->m_communityAgent.GetPosition();
				if ( !currentPosition.IsZero() )
				{
					data.SetPosition( currentPosition );
					data.SetEntityExists( false );
				}
			}
		}
	}
}

Bool CCachedWorldData::AreEntityMapPinsLoadedForDLC( const CName& dlcId ) const
{
	return m_entityMapPinsLoadedForDLC.Exist( dlcId );
}

Bool CCachedWorldData::AreQuestMapPinsLoadedForDLC( const CName& dlcId ) const
{
	return m_questMapPinsLoadedForDLC.Exist( dlcId );
}

Bool CCachedWorldData::LoadEntityMapPinsForDLC( const CName& dlcId, TDynArray< SEntityMapPinInfo > & mapPins )
{
	m_entityMapPins.PushBack( mapPins );
	m_entityMapPinsLoadedForDLC.Insert( dlcId );
	return true;
}

Bool CCachedWorldData::LoadQuestMapPinsForDLC( const CName& dlcId, TDynArray< SQuestMapPinInfo > & mapPins )
{
	for ( Uint32 i = 0; i < mapPins.Size(); ++i )
	{
		m_staticQuestMapPins[ mapPins[ i ].m_tag ] = mapPins[ i ];
	}
	m_questMapPinsLoadedForDLC.Insert( dlcId );
	return true;
}

Bool CCachedWorldData::OnSaveGame( IGameSaver* saver )
{
	// visited
	saver->WriteValue( CNAME( CachedWorldVisited ), m_visited );

	// quest mappins
	CGameSaverBlock cachedQuestMapPins( saver, CNAME( CachedQuestMapPinsMap ) );
	saver->WriteValue( CNAME( Size ), m_cachedQuestMapPins.Size() );

	for ( auto it = m_cachedQuestMapPins.Begin(); it != m_cachedQuestMapPins.End(); ++it )
	{
		saver->WriteValue( CNAME( CachedQuestMapPinTag ),			it->m_second.m_tag );
		saver->WriteValue( CNAME( CachedQuestMapPinType ),			it->m_second.m_type );
		saver->WriteValue( CNAME( CachedQuestMapPinRadius ),		it->m_second.m_radius );
		saver->WriteValue( CNAME( CachedQuestMapPinPositions ),		it->m_second.m_positions );
	}

	// shopkeepers
	CGameSaverBlock cachedShopkeeperDataMap( saver, CNAME( CachedShopkeeperDataMap ) );
	saver->WriteValue( CNAME( Size ), m_cachedShopkeepers.Size() );

	for ( auto it = m_cachedShopkeepers.Begin(); it != m_cachedShopkeepers.End(); ++it )
	{
		saver->WriteValue( CNAME( CachedShopkeeperDataTag ),			it->m_second.m_tag );
		saver->WriteValue( CNAME( CachedShopkeeperDataId ),				it->m_second.m_id );
		saver->WriteValue( CNAME( CachedShopkeeperDataType ),			it->m_second.m_type );
		saver->WriteValue( CNAME( CachedShopkeeperDataPosition ),		it->m_second.m_position );
		saver->WriteValue( CNAME( CachedShopkeeperDataInitialized ),	it->m_second.m_initialized );
		saver->WriteValue( CNAME( CachedShopkeeperDataEnabled ),		it->m_second.m_enabled );
	}

	// boats
	CGameSaverBlock cachedBoatDataArray( saver, CNAME( CachedBoatDataArray ) );
	saver->WriteValue( CNAME( Size ), m_cachedBoatMapPins.Size() );

	for ( Uint32 i = 0; i < m_cachedBoatMapPins.Size(); ++i )
	{
		saver->WriteValue( CNAME( CachedBoatDataPosition ),	m_cachedBoatMapPins[ i ] );
	}

	return true;
}

Bool CCachedWorldData::OnLoadGame( IGameLoader* loader )
{
	Uint32 size = 0;

	// visited
	if ( loader->GetSaveVersion() >= SAVE_VERSION_MAP_MANAGER_BOATS )
	{
		loader->ReadValue( CNAME( CachedWorldVisited ), m_visited );
	}

	if ( loader->GetSaveVersion() >= SAVE_VERSION_MAP_MANAGER_OPTIMIZATION )
	{
		// quest mappins
		CGameSaverBlock cachedQuestMapPins( loader, CNAME( CachedQuestMapPinsMap ) );
		size = 0;
		loader->ReadValue( CNAME( Size ), size );

		for ( Uint32 i = 0; i < size; ++i )
		{
			SQuestMapPinInfo info;
			loader->ReadValue( CNAME( CachedQuestMapPinTag ),			info.m_tag );
			loader->ReadValue( CNAME( CachedQuestMapPinType ),			info.m_type );
			loader->ReadValue( CNAME( CachedQuestMapPinRadius ),		info.m_radius );
			loader->ReadValue( CNAME( CachedQuestMapPinPositions ),		info.m_positions );

			m_cachedQuestMapPins[ info.m_tag ] = info;
		}

		// shopkeepers
		CGameSaverBlock cachedShopkeeperDataMap( loader, CNAME( CachedShopkeeperDataMap ) );
		size = 0;
		loader->ReadValue< Uint32 >( CNAME( Size ), size );

		for ( Uint32 i = 0; i < size; ++i )
		{
			CName tag, type;
			Int32 id = 0;
			Vector position = Vector::ZEROS;
			Bool initialized = false, enabled = false;

			loader->ReadValue( CNAME( CachedShopkeeperDataTag ),			tag );
			loader->ReadValue( CNAME( CachedShopkeeperDataId ),				id );
			loader->ReadValue( CNAME( CachedShopkeeperDataType ),			type );
			loader->ReadValue( CNAME( CachedShopkeeperDataPosition ),		position );
			loader->ReadValue( CNAME( CachedShopkeeperDataInitialized ),	initialized );
			loader->ReadValue( CNAME( CachedShopkeeperDataEnabled ),		enabled );

			AddNewShopkeeperData( tag, id, type, nullptr, position, enabled, initialized );
		}

		// boats
		if ( loader->GetSaveVersion() >= SAVE_VERSION_MAP_MANAGER_BOATS )
		{
			CGameSaverBlock cachedBoatDataArray( loader, CNAME( CachedBoatDataArray ) );
			size = 0;
			loader->ReadValue< Uint32 >( CNAME( Size ), size );

			for ( Uint32 i = 0; i < size; ++i )
			{
				Vector position;
				loader->ReadValue( CNAME( CachedBoatDataPosition ),	position );

				m_cachedBoatMapPins.PushBack( position );
			}
		}
	}
	else
	{
/////////////////////////////////////////////////////////////////////////////////////
//
// OBSOLETE
//

	// quest mappins
	CGameSaverBlock cachedQuestMapPins( loader, CNAME( CachedQuestMapPinsMap ) );
	size = 0;
	loader->ReadValue( CNAME( Size ), size );

	for ( Uint32 i = 0; i < size; ++i )
	{
		CGameSaverBlock cachedQuestMapPin( loader, CNAME( CachedQuestMapPin ) );

		SQuestMapPinInfo info;
		loader->ReadValue( CNAME( CachedQuestMapPinTag ),			info.m_tag );
		loader->ReadValue( CNAME( CachedQuestMapPinType ),			info.m_type );
		loader->ReadValue( CNAME( CachedQuestMapPinRadius ),		info.m_radius );
		loader->ReadValue( CNAME( CachedQuestMapPinPositions ),		info.m_positions );

		m_cachedQuestMapPins[ info.m_tag ] = info;
	}

	// shopkeepers
	CGameSaverBlock cachedShopkeeperDataMap( loader, CNAME( CachedShopkeeperDataMap ) );
	size = 0;
	loader->ReadValue< Uint32 >( CNAME( Size ), size );

	for ( Uint32 i = 0; i < size; ++i )
	{
		CGameSaverBlock cachedShopkeeperData( loader, CNAME( CachedShopkeeperData ) );

		CName tag, type;
		Int32 id = 0;
		Vector position = Vector::ZEROS;
		Bool initialized = false, enabled = false;

		loader->ReadValue( CNAME( CachedShopkeeperDataTag ),			tag );
		loader->ReadValue( CNAME( CachedShopkeeperDataId ),				id );
		loader->ReadValue( CNAME( CachedShopkeeperDataType ),			type );
		loader->ReadValue( CNAME( CachedShopkeeperDataPosition ),		position );
		loader->ReadValue( CNAME( CachedShopkeeperDataInitialized ),	initialized );
		loader->ReadValue( CNAME( CachedShopkeeperDataEnabled ),		enabled );

		AddNewShopkeeperData( tag, id, type, nullptr, position, enabled, initialized );
	}

	// boats
	if ( loader->GetSaveVersion() >= SAVE_VERSION_MAP_MANAGER_BOATS )
	{
		CGameSaverBlock cachedBoatDataArray( loader, CNAME( CachedBoatDataArray ) );
		size = 0;
		loader->ReadValue< Uint32 >( CNAME( Size ), size );

		for ( Uint32 i = 0; i < size; ++i )
		{
			Vector position;
			loader->ReadValue( CNAME( CachedBoatDataPosition ),	position );
		
			m_cachedBoatMapPins.PushBack( position );
		}
	}
//
// OBSOLETE
//
/////////////////////////////////////////////////////////////////////////////////////

	}

	return true;
}

Bool CCachedWorldData::LoadEntityMapPins()
{
	// exception, mappins for bob are loaded from dlc
	if ( m_worldName.EndsWith( TXT("bob.w2w") ) )
	{
		return true;
	}

	String resourcePath;
	if ( !CCommonMapManager::GetEntityMappinsResourcePath( m_worldName, resourcePath, 0 ) )
	{
		return false;
	}

	THandle< CEntityMapPinsResource > resource = LoadResource< CEntityMapPinsResource >( resourcePath );
	if ( !resource )
	{
		return false;
	}
	m_entityMapPins = resource->GetMapPinsInfo();
	return true;
}

Bool CCachedWorldData::LoadQuestMapPins()
{
	String resourcePath;
	if ( !CCommonMapManager::GetQuestMappinsResourcePath( m_worldName, resourcePath, 0 ) )
	{
		return false;
	}

	THandle< CQuestMapPinsResource > resource = LoadResource< CQuestMapPinsResource >( resourcePath );
	if ( !resource )
	{
		return false;
	}

	TDynArray< SQuestMapPinInfo >& mapPins = resource->GetMapPinsInfo();
	for ( Uint32 i = 0; i < mapPins.Size(); ++i )
	{
		const SQuestMapPinInfo& mapPin = mapPins[ i ];
		if ( mapPin.m_tag )
		{
			m_staticQuestMapPins[ mapPin.m_tag ] = mapPin;
		}
	}
	return true;
}

void CCachedWorldData::AddNewShopkeeperData( const CName& tag, Int32 id, const CName& type, const CNewNPC* entity, const Vector& position, Bool enable, Bool initialize )
{
	if ( entity )
	{
		id = static_cast< Int32 >( ::GetHash( reinterpret_cast< Uint64 >( entity ) ) );
	}
	m_cachedShopkeepers[ tag ] = CCachedShopkeeperData( tag, id, type, entity, position, enable, initialize );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

const Uint32 CCommonMapManager::DYNAMIC_MAPPIN_UPDATE_COUNT = 75;
const Uint32 CCommonMapManager::MINIMAP_PINS_COUNT = 300;
const float  CCommonMapManager::ENEMY_SEARCH_RADIUS = 33;
const float  CCommonMapManager::HERB_SEARCH_RADIUS = 16;
const float  CCommonMapManager::MONSTER_CLUE_SEARCH_RADIUS = 10;
const float  CCommonMapManager::DYNAMIC_MAPPIN_VISIBLILITY_VERTICAL_THRESHOLD = 10;
const float  CCommonMapManager::MONSTER_CLUE_MAPPIN_VISIBLILITY_VERTICAL_THRESHOLD = 5;
const float  CCommonMapManager::PATHS_UPDATE_INTERVAL = 5;
const float  CCommonMapManager::DYNAMIC_PINS_UPDATE_INTERVAL = 0.5f;
const float  CCommonMapManager::DYNAMIC_PINS_POSITION_UPDATE_INTERVAL = 0.05f; // 0.1 seems not to be enough
const float  CCommonMapManager::STATIC_PINS_UPDATE_INTERVAL = 1.0f;
const float  CCommonMapManager::HIGHLIGHTING_UPDATE_INTERVAL = 0.3f;
const float  CCommonMapManager::HINT_WAYPOINT_RADIUS_PERCENT_THRESHOLD = 75.0f;
const float  CCommonMapManager::HINT_WAYPOINT_UPDATE_DISTANCE_THRESHOLD = 3.0f;

const Int32 CCommonMapManager::ADDED_PINS_PER_FRAME_LIMIT = 1;
const Int32 CCommonMapManager::DELETED_PINS_PER_FRAME_LIMIT = 2;

const Uint32 CCommonMapManager::EXPANSION_PACK_COUNT = 2;

const CName CCommonMapManager::m_questName[] = {	// specified in CJournalQuest
													CNAME( StoryQuest ),
													CNAME( ChapterQuest ),
													CNAME( SideQuest ),
													CNAME( MonsterQuest ),
													CNAME( TreasureQuest ),
													// specified in CJournalQuestMapPin
													CNAME( QuestReturn ),
													CNAME( HorseRace ),
													CNAME( BoatRace ),
													CNAME( QuestBelgard ),
													CNAME( QuestCoronata ),
													CNAME( QuestVermentino ) };

const CName CCommonMapManager::m_userPinName[] = {	CNAME( User1 ),
													CNAME( User2 ),
													CNAME( User3 ),
													CNAME( User4 ) };

CCommonMapManager::CCommonMapManager()
	: m_currentArea( 0 )
	, m_showPinsInfoFlags( 0 )
	, m_invalidatedQuestMapPinData( true )
	, m_showHintWaypoints( true )
	, m_showKnownEntities( true )
	, m_showDisabledEntities( true )
	, m_showFocusClues( true )
	, m_showPathsInfo( false )
	, m_showQuestAgentsInfo( false )
	, m_showShopkeepersInfo( false )
	, m_showInteriorInfo( false )
	, m_showUserPinsInfo( false )
	, m_invalidatedEntityMapPins( false )
	, m_highlightedMapPin( INVALID_MAPPIN_ID )
	, m_borderCount( 0 )
	, m_borderInterval( 0 )
	, m_isInBorder( false )
	, m_isValidClosestFastTravelPoint( false )
	, m_closestFastTravelPointPos( Vector::ZEROS )
	, m_closestFastTravelPointLastPlayerPos( Vector::ZEROS )
	, m_pathsUpdateInterval( PATHS_UPDATE_INTERVAL )
	, m_dynamicPinsUpdateStep( DPUS_None )
	, m_dynamicPinsUpdateInterval( DYNAMIC_PINS_UPDATE_INTERVAL )
	, m_dynamicPinsPositionUpdateInterval( DYNAMIC_PINS_POSITION_UPDATE_INTERVAL )
	, m_staticPinsUpdateInterval( STATIC_PINS_UPDATE_INTERVAL )
	, m_highlightingUpdateInterval( HIGHLIGHTING_UPDATE_INTERVAL )
	, m_isFastTravellingEnabled( true )
	, m_useInteriorsForQuestMapPins( false )
	, m_waypointUserPinLimit( WAYPOINT_USER_PIN_LIMIT )
	, m_otherUserPinLimit( OTHER_USER_PIN_LIMIT )
{
	m_mapTracking = new CR4MapTracking( *this );

	LoadMapPinInfoFromScripts();
	LoadAreaMapPins();

	CacheScriptedClasses();
	CacheScriptedFunctions();
}

CCommonMapManager::~CCommonMapManager()
{
}

void CCommonMapManager::Initialize()
{
	SGameSessionManager::GetInstance().RegisterGameSaveSection( this );
}

void CCommonMapManager::Shutdown()
{
	SGameSessionManager::GetInstance().UnregisterGameSaveSection( this );
}

void CCommonMapManager::OnGameStart( const CGameInfo& gameInfo )
{
	// PERSISTENT STUFF, DO NOT CLEAR IT
	//m_customMapPinDefinition.ClearFast();
	//m_knowableEntityMapPinTypes.Clear();
	//m_discoverableEntityMapPinTypes.Clear();
	//m_disableableEntityMapPinTypes.Clear();

	m_currentArea = 0;
	m_currentWorldPath.ClearFast();

	m_isInBorder = false;
	m_borderCount = 0;
	m_borderInterval = 0;

	if ( gameInfo.IsNewGame() || gameInfo.IsSavedGame() )
	{
		for( auto iter = m_cachedWorldDataMap.Begin(), end = m_cachedWorldDataMap.End(); iter != end; ++iter )
		{
			iter->m_second.Uninitialize();
		}

		m_questMapPinStates.ClearFast();
		m_knownEntityMapPinTags.Clear();
		m_discoveredEntityMapPinTags.Clear();
		m_disabledEntityMapPinTags.Clear();
		m_discoveredAgentEntityTags.Clear();
		m_discoveredPaths.Clear();
		m_customEntityMapPins.Clear();
		m_customAgentMapPins.Clear();
		m_userMapPins.Clear();
		m_disabledMapPins.Clear();
	}
	m_questMapPinData.ClearFast();
	m_questLootEntities.ClearFast();
	m_interiorsRegistry.ClearFast();
	m_playerInteriors.ClearFast();
	m_boatEntities.ClearFast();

	m_dynamicPinsUpdateStep = DPUS_None;
	m_currDynamicMapPinIds.Clear();
	m_prevDynamicMapPinIds.Clear();
	m_mapPins.Clear();
	m_currMapPathIds.Clear();
	m_prevMapPathIds.Clear();
	m_mapPaths.Clear();

	InvalidateQuestMapPinData();
	InvalidateClosestFastTravelPoint();

	CreateMinimapManager();

	// Load from savegame if applicable
	if ( gameInfo.IsSavedGame() )
	{
		OnLoadGame( gameInfo.m_gameLoadStream );
	}
}

void CCommonMapManager::OnGameEnd( const CGameInfo& gameInfo )
{
	DeleteMinimapManager();
}

void CCommonMapManager::OnWorldStart( const CGameInfo& gameInfo )
{
	UpdateCurrentArea();

	const CWorld* world = GCommonGame->GetActiveWorld();
	if ( world )
	{
		m_currentWorldPath = world->GetDepotPath();
	}

	// Need to set this to show the right video even if there's been no save yet!
	CWitcherJournalManager* journalMgr = GCommonGame->GetSystem< CWitcherJournalManager >();
	if ( journalMgr )
	{
		const String& videoFile = journalMgr->GetCurrentStorybookVideoName();
		GGame->ReplaceDefaultLoadingScreenVideo( videoFile );
	}

	CCachedWorldData* cachedData = GetWorldCachedData( m_currentWorldPath );
	if ( cachedData )
	{
		AddEntityMapPins( cachedData->GetEntityMapPins() );
		m_entityMapPinsInitializedFor.Insert( m_currentWorldPath );
	}

	CCachedWorldData* cachedWorldData = GetCurrentWorldCachedData();
	if ( cachedWorldData )
	{
		cachedWorldData->SetVisited();
	}

	m_mapTracking->CreateSearchData();
}

void CCommonMapManager::OnWorldEnd( const CGameInfo& gameInfo )
{
	m_mapTracking->DestroySearchData();
}

Bool CCommonMapManager::OnSaveGame( IGameSaver* saver )
{
	TIMER_BLOCK( time )

	CGameSaverBlock block( saver, GetStaticClass()->GetName() );

	// Map pin states
	{
		CGameSaverBlock questMapPinStatesBlock( saver, CNAME( QuestMapPinStates ) );
		saver->WriteValue( CNAME( Size ), m_questMapPinStates.Size() );

		for ( auto it = m_questMapPinStates.Begin(); it != m_questMapPinStates.End(); ++it )
		{
			saver->WriteValue( CNAME( ObjectiveGuid ),	it->m_first.m_first );
			saver->WriteValue( CNAME( MapPinGuid ),		it->m_first.m_second );
			saver->WriteValue( CNAME( MapPinState ),	it->m_second );
		}
	}

	// Known map pin tags
	{
		CGameSaverBlock knownMapPinTagsBlock( saver, CNAME( KnownMapPinTags ) );
		saver->WriteValue( CNAME( Size ), static_cast< Uint32 >( m_knownEntityMapPinTags.Size() ) ); // TSet::Size returns Int32, not Uint32!

		for ( TSet< CName >::iterator it = m_knownEntityMapPinTags.Begin(); it != m_knownEntityMapPinTags.End(); it++ )
		{
			saver->WriteValue( CNAME( MapPinTag ), *it );
		}
	}

	// Discovered map pin tags
	{
		CGameSaverBlock discoveredMapPinTagsBlock( saver, CNAME( DiscoveredMapPinTags ) );
		saver->WriteValue( CNAME( Size ), static_cast< Uint32 >( m_discoveredEntityMapPinTags.Size() ) ); // TSet::Size returns Int32, not Uint32!

		for ( TSet< CName >::iterator it = m_discoveredEntityMapPinTags.Begin(); it != m_discoveredEntityMapPinTags.End(); it++ )
		{
			saver->WriteValue( CNAME( MapPinTag ), *it );
		}
	}

	// Disabled map pin tags
	{
		CGameSaverBlock disabledMapPinTagsBlock( saver, CNAME( DisabledMapPinTags ) );
		saver->WriteValue( CNAME( Size ), static_cast< Uint32 >( m_disabledEntityMapPinTags.Size() ) ); // TSet::Size returns Int32, not Uint32!

		for ( TSet< CName >::iterator it = m_disabledEntityMapPinTags.Begin(); it != m_disabledEntityMapPinTags.End(); it++ )
		{
			saver->WriteValue( CNAME( MapPinTag ), *it );
		}
	}

	// Discovered agent entity tags
	{
		CGameSaverBlock discoveredAgentEntityTagsBlock( saver, CNAME( DiscoveredAgentEntityTags ) );
		saver->WriteValue( CNAME( Size ), static_cast< Uint32 >( m_discoveredAgentEntityTags.Size() ) ); // TSet::Size returns Int32, not Uint32!

		for ( TSet< CName >::iterator it = m_discoveredAgentEntityTags.Begin(); it != m_discoveredAgentEntityTags.End(); it++ )
		{
			saver->WriteValue( CNAME( MapPinTag ), *it );
		}
	}

	// Discovered paths
	{
		CGameSaverBlock discoveredPathsBlock( saver, CNAME( DiscoveredPaths ) );
		saver->WriteValue( CNAME( Size ), m_discoveredPaths.Size() );

		for ( THashMap< CName, SMapPathDefinition >::iterator it = m_discoveredPaths.Begin(); it != m_discoveredPaths.End(); ++it )
		{
			saver->WriteValue( CNAME( DiscoveredPathTag ),               (*it).m_second.m_tag );
			saver->WriteValue( CNAME( DiscoveredPathLineWidth ),         (*it).m_second.m_lineWidth );
			saver->WriteValue( CNAME( DiscoveredPathLineSegmentLength ), (*it).m_second.m_lineSegmentLength );
			saver->WriteValue( CNAME( DiscoveredPathColor ),             (*it).m_second.m_color );
		}
	}

	// Custom entity mappins
	{
		CGameSaverBlock customEntityMapPinsBlock( saver, CNAME( CustomEntityMapPins ) );
		saver->WriteValue( CNAME( Size ), m_customEntityMapPins.Size() );

		for ( THashMap< CName, CName >::iterator it = m_customEntityMapPins.Begin(); it != m_customEntityMapPins.End(); ++it )
		{
			saver->WriteValue( CNAME( CustomEntityMapPinTag ),  (*it).m_first );
			saver->WriteValue( CNAME( CustomEntityMapPinType ), (*it).m_second );
		}
	}

	// Custom agent mappins
	{
		CGameSaverBlock customAgentMapPinsBlock( saver, CNAME( CustomAgentMapPins ) );
		saver->WriteValue( CNAME( Size ), m_customAgentMapPins.Size() );

		for ( THashMap< CName, CName >::iterator it = m_customAgentMapPins.Begin(); it != m_customAgentMapPins.End(); ++it )
		{
			saver->WriteValue( CNAME( CustomAgentMapPinTag ),  (*it).m_first );
			saver->WriteValue( CNAME( CustomAgentMapPinType ), (*it).m_second );
		}
	}

	// User map pin
	{
		CGameSaverBlock userMapPinBlock( saver, CNAME( UserMapPin ) );
		saver->WriteValue( CNAME( UserMapPinsCount ),     m_userMapPins.Size() );

		for ( Uint32 i = 0; i < m_userMapPins.Size(); ++i )
		{
			saver->WriteValue( CNAME( UserMapPinArea ),     m_userMapPins[ i ].m_area );
			saver->WriteValue( CNAME( UserMapPinPosition ), m_userMapPins[ i ].m_position );
			saver->WriteValue( CNAME( UserMapPinType ),		m_userMapPins[ i ].m_type );
		}
	}

	// Cached world data
	{
		CGameSaverBlock cachedWorldDataMapBlock( saver, CNAME( CachedWorldDataMap ) );
		saver->WriteValue( CNAME( Size ), m_cachedWorldDataMap.Size() );

		for ( THashMap< String, CCachedWorldData >::iterator it = m_cachedWorldDataMap.Begin(); it != m_cachedWorldDataMap.End(); ++it )
		{
			saver->WriteValue( CNAME( CachedWorldDataPath ), it->m_first );

			it->m_second.OnSaveGame( saver );
		}
	}

	// Hidden Pin Filter List (new version)
	{
		CGameSaverBlock hiddenPinFilterBlock( saver, CNAME( MapPinFilterHiddenList ) );
		saver->WriteValue( CNAME( Size ), m_disabledMapPins.Size() );

		THashSet< String >::iterator end_it = m_disabledMapPins.End();
		for ( THashSet< String >::iterator it = m_disabledMapPins.Begin(); it != end_it; ++it )
		{
			saver->WriteValue( CNAME( MapPinFilterHiddenName ), *it );
		}
	}

	// Border Data
	{
		CGameSaverBlock mapBorderDataBlock( saver, CNAME( MapBorderData ) );
		saver->WriteValue( CNAME( Bool ), m_isInBorder );
		saver->WriteValue( CNAME( Vector3 ), m_borderPlayerPosition );
		saver->WriteValue( CNAME( Vector3 ), m_borderPlayerRotation );
	}

	END_TIMER_BLOCK( time )

	return true;
}

void CCommonMapManager::OnLoadGame( IGameLoader* loader )
{
	CGameSaverBlock block( loader, GetStaticClass()->GetName() );

	if ( loader->GetSaveVersion() >= SAVE_VERSION_MAP_MANAGER_OPTIMIZATION )
	{
		// Map pin states
		{
			CGameSaverBlock questMapPinStatesBlock( loader, CNAME( QuestMapPinStates ) );
			Uint32 size = loader->ReadValue< Uint32 >( CNAME( Size ) );

			for ( Uint32 i = 0; i < size; i++ )
			{
				CGUID objectiveGUID, mapPinGUID;
				Bool mapPinState = false;

				loader->ReadValue( CNAME( ObjectiveGuid ),	objectiveGUID );
				loader->ReadValue( CNAME( MapPinGuid ),		mapPinGUID );
				loader->ReadValue( CNAME( MapPinState ),	mapPinState );

				TPair< CGUID, CGUID > pair( objectiveGUID, mapPinGUID );
				m_questMapPinStates.Insert( pair, mapPinState );
			}
		}

		// Known map pin tags
		{
			CGameSaverBlock knownMapPinTagsBlock( loader, CNAME( KnownMapPinTags ) );
			Uint32 size = loader->ReadValue< Uint32 >( CNAME( Size ) );

			for ( Uint32 i = 0; i < size; i++ )
			{
				CName tag;

				loader->ReadValue( CNAME( MapPinTag ), tag );
				if ( tag != CName::NONE )
				{
					m_knownEntityMapPinTags.Insert( tag );
				}
			}
		}

		// Discovered map pin tags
		{
			CGameSaverBlock discoveredMapPinTagsBlock( loader, CNAME( DiscoveredMapPinTags ) );
			Uint32 size = loader->ReadValue< Uint32 >( CNAME( Size ) );

			for ( Uint32 i = 0; i < size; i++ )
			{
				CName tag;

				loader->ReadValue( CNAME( MapPinTag ), tag );
				if ( tag != CName::NONE )
				{
					m_discoveredEntityMapPinTags.Insert( tag );
				}
			}
		}

		// Disabled map pin tags
		{
			CGameSaverBlock disabledMapPinTagsBlock( loader, CNAME( DisabledMapPinTags ) );
			Uint32 size = loader->ReadValue< Uint32 >( CNAME( Size ) );

			for ( Uint32 i = 0; i < size; i++ )
			{
				CName tag;

				loader->ReadValue( CNAME( MapPinTag ), tag );
				if ( tag != CName::NONE )
				{
					m_disabledEntityMapPinTags.Insert( tag );
				}
			}
		}

		// Discovered agent entity tags
		{
			CGameSaverBlock discoveredAgentEntityTagsBlock( loader, CNAME( DiscoveredAgentEntityTags ) );
			Uint32 size = loader->ReadValue< Uint32 >( CNAME( Size ) );

			for ( Uint32 i = 0; i < size; i++ )
			{
				CName tag;

				loader->ReadValue( CNAME( MapPinTag ), tag );
				if ( tag != CName::NONE )
				{
					m_discoveredAgentEntityTags.Insert( tag );
				}
			}
		}

		// Discovered paths
		{
			CGameSaverBlock discoveredPathsBlock( loader, CNAME( DiscoveredPaths ) );
			Uint32 size = loader->ReadValue< Uint32 >( CNAME( Size ) );

			for ( Uint32 i = 0; i < size; i++ )
			{
				CName tag;
				Float lineWidth = 1, lineSegmentWidth = 5;
				Color color;

				loader->ReadValue( CNAME( DiscoveredPathTag ),               tag );
				loader->ReadValue( CNAME( DiscoveredPathLineWidth ),         lineWidth );
				loader->ReadValue( CNAME( DiscoveredPathLineSegmentLength ), lineSegmentWidth );
				loader->ReadValue( CNAME( DiscoveredPathColor ),             color );

				if ( tag != CName::NONE )
				{
					m_discoveredPaths[ tag ] = SMapPathDefinition( tag, lineWidth, lineSegmentWidth, color );
				}
			}
		}

		// Custom entity mappins
		{
			CGameSaverBlock customEntityMapPinsBlock( loader, CNAME( CustomEntityMapPins ) );
			Uint32 size = loader->ReadValue< Uint32 >( CNAME( Size ) );

			for ( Uint32 i = 0; i < size; i++ )
			{
				CName tag, type;

				loader->ReadValue( CNAME( CustomEntityMapPinTag ),  tag );
				loader->ReadValue( CNAME( CustomEntityMapPinType ), type );

				if ( tag != CName::NONE && type != CName::NONE )
				{
					m_customEntityMapPins[ tag ] = type;
				}
			}
		}

		// Custom agent mappins
		{
			CGameSaverBlock customAgentMapPinsBlock( loader, CNAME( CustomAgentMapPins ) );
			Uint32 size = loader->ReadValue< Uint32 >( CNAME( Size ) );

			for ( Uint32 i = 0; i < size; i++ )
			{
				CName tag, type;

				loader->ReadValue( CNAME( CustomAgentMapPinTag ),  tag );
				loader->ReadValue( CNAME( CustomAgentMapPinType ), type );

				if ( tag != CName::NONE && type != CName::NONE )
				{
					m_customAgentMapPins[ tag ] = type;
				}
			}
		}

		// User map pin
		{
			m_userMapPins.ClearFast(); // just in case, array size may vary

			CGameSaverBlock userMapPinBlock( loader, CNAME( UserMapPin ) );
			if ( loader->GetSaveVersion() >= SAVE_VERSION_MULTIPLE_USER_PINS )
			{
				Uint32 count = 0;
				loader->ReadValue( CNAME( UserMapPinsCount ), count );

				if ( loader->GetSaveVersion() >= SAVE_VERSION_EVEN_MORE_MULTIPLE_USER_PINS )
				{

					for ( Uint32 i = 0; i < count; ++i)
					{
						Int32 area, type;
						Vector2 position;

						loader->ReadValue( CNAME( UserMapPinArea ),     area );
						loader->ReadValue( CNAME( UserMapPinPosition ), position );
						loader->ReadValue( CNAME( UserMapPinType ),		type );

						m_userMapPins.PushBack( CUserMapPinData( area, position, type ) );
					}
				}
				else
				{
					for ( Uint32 i = 0; i < count; ++i)
					{
						Int32 area;
						Vector2 position;

						loader->ReadValue( CNAME( UserMapPinArea ),     area );
						loader->ReadValue( CNAME( UserMapPinPosition ), position );

						if ( area > -1 ) // otherwise ignore, area == -1 meant there was no user pin placed on map, but an array had a fixed size of 4 anyway
						{
							m_userMapPins.PushBack( CUserMapPinData( area, position, i ) ); // type = i, array index was type in previous version
						}
					}
				}
			}
			else
			{
				Int32 area;
				Vector2 position;

				loader->ReadValue( CNAME( UserMapPinArea ),     area );
				loader->ReadValue( CNAME( UserMapPinPosition ), position );

				if ( area > -1 ) // ostherwise ignore, area == -1 meant there was no user pin placed on map
				{
					m_userMapPins.PushBack( CUserMapPinData( area, position, 0 ) ); // type = 0, since it was always a waypoint in previous versions
				}
			}
		}

		// Cached world data
		{
			CGameSaverBlock cachedWorldDataMapBlock( loader, CNAME( CachedWorldDataMap ) );
			Uint32 size = 0;
			loader->ReadValue< Uint32 >( CNAME( Size ), size );

			for ( Uint32 i = 0; i < size; ++i)
			{
				String cachedWorldDataPath;
				loader->ReadValue( CNAME( CachedWorldDataPath ), cachedWorldDataPath );

				CCachedWorldData* cachedWorldData = GetWorldCachedData( cachedWorldDataPath );
				if ( cachedWorldData )
				{
					cachedWorldData->OnLoadGame( loader );
				}
			}
		}

		// Hidden Pin Filter List
		if ( loader->GetSaveVersion() < SAVE_VERSION_NEW_MAP_FILTERS )
		{
			CGameSaverBlock hiddenPinFilterBlock( loader, CNAME( MapPinFilterHiddenList ) );
			Uint32 size = 0;
			loader->ReadValue< Uint32 >( CNAME( Size ), size );

			for ( Uint32 i = 0; i < size; ++i)
			{
				CName cachedPinFilterName;
				loader->ReadValue( CNAME( MapPinFilterHiddenName ), cachedPinFilterName );
				// read value but don't do anything with it
			}
		}
		else
		{
			CGameSaverBlock hiddenPinFilterBlock( loader, CNAME( MapPinFilterHiddenList ) );
			Uint32 size = 0;
			loader->ReadValue< Uint32 >( CNAME( Size ), size );
			m_disabledMapPins.Reserve( size );

			for ( Uint32 i = 0; i < size; ++i)
			{
				String pinType;
				loader->ReadValue( CNAME( MapPinFilterHiddenName ), pinType );
				m_disabledMapPins.Insert( pinType );
			}
		}

		// Border Data
		if ( loader->GetSaveVersion() < SAVE_VERSION_MAP_BORDER_DATA )
		{
			// Do nothing ...
		}
		else
		{
			CGameSaverBlock mapBorderDataBlock( loader, CNAME( MapBorderData ) );
			loader->ReadValue( CNAME( Bool ), m_isInBorder );
			loader->ReadValue( CNAME( Vector3 ), m_borderPlayerPosition );
			loader->ReadValue( CNAME( Vector3 ), m_borderPlayerRotation );
		}
	}
	else
	{
/////////////////////////////////////////////////////////////////////////////////////
//
// OBSOLETE
//

	// Map pin states
	{
		CGameSaverBlock questMapPinStatesBlock( loader, CNAME( QuestMapPinStates ) );
		Uint32 size = loader->ReadValue< Uint32 >( CNAME( Size ) );

		for ( Uint32 i = 0; i < size; i++ )
		{
			CGameSaverBlock questMapPinStateBlock( loader, CNAME( QuestMapPinState ) );

			CGUID objectiveGUID, mapPinGUID;
			Bool mapPinState = false;

			loader->ReadValue( CNAME( ObjectiveGuid ),	objectiveGUID );
			loader->ReadValue( CNAME( MapPinGuid ),		mapPinGUID );
			loader->ReadValue( CNAME( MapPinState ),	mapPinState );

			TPair< CGUID, CGUID > pair( objectiveGUID, mapPinGUID );
			m_questMapPinStates.Insert( pair, mapPinState );
		}
	}

	// Known map pin tags
	{
		CGameSaverBlock knownMapPinTagsBlock( loader, CNAME( KnownMapPinTags ) );
		Uint32 size = loader->ReadValue< Uint32 >( CNAME( Size ) );

		for ( Uint32 i = 0; i < size; i++ )
		{
			CGameSaverBlock knownMapPinTagBlock( loader, CNAME( KnownMapPinTag ) );
			CName tag;

			loader->ReadValue( CNAME( MapPinTag ), tag );
			if ( tag != CName::NONE )
			{
				m_knownEntityMapPinTags.Insert( tag );
			}
		}
	}

	// Discovered map pin tags
	{
		CGameSaverBlock discoveredMapPinTagsBlock( loader, CNAME( DiscoveredMapPinTags ) );
		Uint32 size = loader->ReadValue< Uint32 >( CNAME( Size ) );

		for ( Uint32 i = 0; i < size; i++ )
		{
			CGameSaverBlock discoveredMapPinTagBlock( loader, CNAME( DiscoveredMapPinTag ) );
			CName tag;

			loader->ReadValue( CNAME( MapPinTag ), tag );
			if ( tag != CName::NONE )
			{
				m_discoveredEntityMapPinTags.Insert( tag );
			}
		}
	}

	// Disabled map pin tags
	{
		CGameSaverBlock disabledMapPinTagsBlock( loader, CNAME( DisabledMapPinTags ) );
		Uint32 size = loader->ReadValue< Uint32 >( CNAME( Size ) );

		for ( Uint32 i = 0; i < size; i++ )
		{
			CGameSaverBlock disabledMapPinTagBlock( loader, CNAME( DisabledMapPinTag ) );
			CName tag;

			loader->ReadValue( CNAME( MapPinTag ), tag );
			if ( tag != CName::NONE )
			{
				m_disabledEntityMapPinTags.Insert( tag );
			}
		}
	}

	// Discovered agent entity tags
	{
		CGameSaverBlock discoveredAgentEntityTagsBlock( loader, CNAME( DiscoveredAgentEntityTags ) );
		Uint32 size = loader->ReadValue< Uint32 >( CNAME( Size ) );

		for ( Uint32 i = 0; i < size; i++ )
		{
			CGameSaverBlock discoveredAgentEntityTagBlock( loader, CNAME( DiscoveredAgentEntityTag ) );
			CName tag;

			loader->ReadValue( CNAME( MapPinTag ), tag );
			if ( tag != CName::NONE )
			{
				m_discoveredAgentEntityTags.Insert( tag );
			}
		}
	}

	// Discovered paths
	{
		CGameSaverBlock discoveredPathsBlock( loader, CNAME( DiscoveredPaths ) );
		Uint32 size = loader->ReadValue< Uint32 >( CNAME( Size ) );

		for ( Uint32 i = 0; i < size; i++ )
		{
			CGameSaverBlock discoveredPathBlock( loader, CNAME( DiscoveredPath ) );

			CName tag;
			Float lineWidth = 1, lineSegmentWidth = 5;
			Color color;

			loader->ReadValue( CNAME( DiscoveredPathTag ),               tag );
			loader->ReadValue( CNAME( DiscoveredPathLineWidth ),         lineWidth );
			loader->ReadValue( CNAME( DiscoveredPathLineSegmentLength ), lineSegmentWidth );
			loader->ReadValue( CNAME( DiscoveredPathColor ),             color );

			if ( tag != CName::NONE )
			{
				m_discoveredPaths[ tag ] = SMapPathDefinition( tag, lineWidth, lineSegmentWidth, color );
			}
		}
	}

	// Custom entity mappins
	{
		CGameSaverBlock customEntityMapPinsBlock( loader, CNAME( CustomEntityMapPins ) );
		Uint32 size = loader->ReadValue< Uint32 >( CNAME( Size ) );

		for ( Uint32 i = 0; i < size; i++ )
		{
			CGameSaverBlock customEntityMapPinBlock( loader, CNAME( CustomEntityMapPin ) );

			CName tag, type;

			loader->ReadValue( CNAME( CustomEntityMapPinTag ),  tag );
			loader->ReadValue( CNAME( CustomEntityMapPinType ), type );

			if ( tag != CName::NONE && type != CName::NONE )
			{
				m_customEntityMapPins[ tag ] = type;
			}
		}
	}

	// Custom agent mappins
	{
		CGameSaverBlock customAgentMapPinsBlock( loader, CNAME( CustomAgentMapPins ) );
		Uint32 size = loader->ReadValue< Uint32 >( CNAME( Size ) );

		for ( Uint32 i = 0; i < size; i++ )
		{
			CGameSaverBlock customAgentMapPinBlock( loader, CNAME( CustomAgentMapPin ) );

			CName tag, type;

			loader->ReadValue( CNAME( CustomAgentMapPinTag ),  tag );
			loader->ReadValue( CNAME( CustomAgentMapPinType ), type );

			if ( tag != CName::NONE && type != CName::NONE )
			{
				m_customAgentMapPins[ tag ] = type;
			}
		}
	}

	// User map pin
	{
		m_userMapPins.ClearFast();

		CGameSaverBlock userMapPinBlock( loader, CNAME( UserMapPin ) );

		Int32 area;
		Vector position;

		loader->ReadValue( CNAME( UserMapPinArea ),     area );
		loader->ReadValue( CNAME( UserMapPinPosition ), position );

		if ( area > -1 ) // otherwise ignore, area == -1 meant there was no user pin placed on map
		{
			m_userMapPins.PushBack( CUserMapPinData( area, position, 0 ) ); // type = 0, since it was always a waypoint in previous versions
		}
	}

	// Cached world data
	{
		CGameSaverBlock cachedWorldDataMapBlock( loader, CNAME( CachedWorldDataMap ) );
		Uint32 size = 0;
		loader->ReadValue< Uint32 >( CNAME( Size ), size );

		for ( Uint32 i = 0; i < size; ++i)
		{
			CGameSaverBlock cachedWorldDataBlock( loader, CNAME( CachedWorldData ) );
			String cachedWorldDataPath;
			loader->ReadValue( CNAME( CachedWorldDataPath ), cachedWorldDataPath );

			CCachedWorldData* cachedWorldData = GetWorldCachedData( cachedWorldDataPath );
			if ( cachedWorldData )
			{
				cachedWorldData->OnLoadGame( loader );
			}
		}
	}

	// Hidden Pin Filter List
	{
		CGameSaverBlock hiddenPinFilterBlock( loader, CNAME( MapPinFilterHiddenList ) );
		Uint32 size = 0;
		loader->ReadValue< Uint32 >( CNAME( Size ), size );

		for ( Uint32 i = 0; i < size; ++i)
		{
			CGameSaverBlock hiddenPinFilterEntryBlock( loader, CNAME( MapPinFilterHiddenSpecific ) );
			CName cachedPinFilterName;
			loader->ReadValue( CNAME( MapPinFilterHiddenName ), cachedPinFilterName );
			// read and don't do anything with it
		}
	}
//
// OBSOLETE
//
/////////////////////////////////////////////////////////////////////////////////////
	}

	// inform scripts
	CallEvent( CNAME( OnMapPinChanged ) );
}

void CCommonMapManager::Tick( Float timeDelta )
{
	PC_SCOPE_PIX( CommonMapManager );

	const CPlayer* player = GCommonGame->GetPlayer();
	if ( !player )
	{
		return;
	}
	const Vector& playerPos = player->GetWorldPositionRef();

	//check if we have a custom marker and if the player has reached it => remove it
	for ( Int32 i = m_userMapPins.SizeInt() - 1; i >= 0; --i )
	{
		if ( m_userMapPins[ i ].m_area == m_currentArea && m_userMapPins[ i ].m_type == WAYPOINT_USER_MAP_PIN_TYPE )
		{
			if ( playerPos.DistanceSquaredTo2D( m_userMapPins[ i ].m_position ) < 4*4 )
			{
				m_userMapPins.RemoveAt( i );
			}
		}
	}

	// map border timer
	{
		if( m_borderCount > 0 )
		{
			if( m_borderInterval > 0.0f )
			{
				m_borderInterval -= timeDelta;
				if( m_borderInterval <= 0.0f )
				{
					if( CPlayer* player = GCommonGame->GetPlayer() )
					{
						CallEvent( CNAME( OnStartTeleportingPlayerToPlayableArea ), m_borderPlayerPosition, m_borderPlayerRotation );
					}
				}
			}
			else
			{
				m_borderInterval -= timeDelta;
			}
		}
		else
		{
			m_borderInterval = 0.0f;
		}

		static const Float s_borderTimerFailsafe = -0.5f;
		if( m_borderInterval < s_borderTimerFailsafe )
		{
			m_borderInterval = 0.0f;

			// If the timer has gone past s_borderTimerFailsafe then the call to 
			// OnStartTeleportingPlayerToPlayableArea probably failed. Time to
			// force a teleport back to the safe spot.
			if( CPlayer* player = GCommonGame->GetPlayer() )
			{
				CallFunction( player, CName( TXT( "OnTeleportPlayerToPlayableArea" ) ), false );
			}
		}
	}

	// delayed minimap notification
	{
		PC_SCOPE_PIX( DelayedMinimapNotification );
		if ( m_minimapManager && m_minimapManager->IsMinimapAvailable() )
		{
			Int32 addedPins = 0;
			Int32 deletedPins = 0;

			THashMap< Int32, SCommonMapPinInstance >::iterator itEnd = m_mapPins.End();
			for ( THashMap< Int32, SCommonMapPinInstance >::iterator itPin = m_mapPins.Begin(); itPin != itEnd; ++itPin )
			{
				SCommonMapPinInstance& pin = itPin->m_second;

				Bool isVisible = false;
				if ( IsDiscoverableMapPinType( pin.m_type ) )
				{
					Bool isKnown      = pin.m_isKnown;
					Bool isDiscovered = pin.m_isDiscovered;
					Bool isDisabled   = pin.m_isDisabled;

					if ( isDiscovered )
					{
						isVisible = true;
					}

					if ( IsKnowableMapPinType( pin.m_type ) )
					{
						if ( isKnown )
						{
							isVisible = true;
						}
					}

					if ( !m_showKnownEntities && IsKnowableMapPinType( pin.m_type ) && isKnown && !isDiscovered )
					{
						isVisible = false;
					}
					if ( !m_showDisabledEntities && IsDisableableMapPinType( pin.m_type ) && isDisabled && pin.m_type != CNAME( PlaceOfPower ) )
					{
						isVisible = false;
					}
				}
				else
				{
					isVisible = true;
				}

				if ( isVisible ) // if ( pin.m_isDiscovered  || ( pin.m_isKnown && m_showKnownEntities ) )
				{
					// that pins can be possibly visible on minimap
					if ( !pin.m_isAddedToMinimap )
					{
						if ( addedPins < ADDED_PINS_PER_FRAME_LIMIT )
						{
							if ( pin.m_canBeAddedToMinimap && !pin.m_invalidated && m_minimapManager->ShouldMapPinBeVisibleOnMinimap( pin, playerPos ) )
							{
								AddMapPinToMinimap( pin );
								addedPins++;
							}
						}
					}
					else
					{
						if ( deletedPins < DELETED_PINS_PER_FRAME_LIMIT )
						{
							if ( !m_minimapManager->ShouldMapPinBeVisibleOnMinimap( pin, playerPos ) )
							{
								DeleteMapPinOnMinimap( pin );
								deletedPins++;
							}
						}
					}
				}
				else
				{
					// that pins should not be visible at all
					if ( pin.m_isAddedToMinimap )
					{
						if ( deletedPins < DELETED_PINS_PER_FRAME_LIMIT )
						{
							DeleteMapPinOnMinimap( pin );
							deletedPins++;
						}
					}
				}

				if ( addedPins >= ADDED_PINS_PER_FRAME_LIMIT || deletedPins >= DELETED_PINS_PER_FRAME_LIMIT )
				{
					break;
				}
			}

			for ( THashMap< Int32, SMapPathInstance>::iterator itPath = m_mapPaths.Begin(); itPath != m_mapPaths.End(); ++itPath )
			{
				SMapPathInstance& path = itPath->m_second;
				if ( !path.m_isAddedToMinimap )
				{
					path.m_isAddedToMinimap = true;
					CallFunction( this, CNAME( AddMapPathToMinimap ), path );
					break; // break to add each path separately
				}
			}
		}
	}

	// check quest loot entities
	{
		PC_SCOPE_PIX( CheckQuestLootEntities );
		for ( Uint32 i = 0; i < m_questLootEntities.Size(); )
		{
			if ( m_questLootEntities[ i ].Get() == nullptr )
			{
				m_questLootEntities.Erase( m_questLootEntities.Begin() + i );
			}
			else
			{
				i++;
			}
		}
	}

	// quest map pins
	UpdateQuestMapPinData();

	UpdateDynamicMapPins( timeDelta );

	m_staticPinsUpdateInterval -= timeDelta;
	if ( m_staticPinsUpdateInterval < 0.0f )
	{
		UpdateStaticMapPins();
		m_staticPinsUpdateInterval = STATIC_PINS_UPDATE_INTERVAL;
	}

	m_dynamicPinsPositionUpdateInterval -= timeDelta;
	if ( m_dynamicPinsPositionUpdateInterval < 0.0f )
	{
		UpdateDynamicMapPinsPositionsAndVisibility();
		m_dynamicPinsPositionUpdateInterval = DYNAMIC_PINS_POSITION_UPDATE_INTERVAL;
	}

	m_pathsUpdateInterval -= timeDelta;
	if ( m_pathsUpdateInterval < 0.0f )
	{
		UpdateMapPaths();
		m_pathsUpdateInterval = PATHS_UPDATE_INTERVAL;
	}

	if ( m_showHintWaypoints )
	{
		UpdateHintWaypoints( timeDelta );
	}

	if ( m_minimapManager && m_minimapManager->IsMinimapAvailable() )
	{
		// highlighting mappins (doesn't need to be updated every frame)
		m_highlightingUpdateInterval -= timeDelta;
		if ( m_highlightingUpdateInterval < 0.0f )
		{
			m_highlightingUpdateInterval = HIGHLIGHTING_UPDATE_INTERVAL;

			// update highlightable map pins
			UpdateHighlightableMapPins();
		}
	}
}

void CCommonMapManager::OnGenerateDebugFragments( CRenderFrame* frame )
{
#ifndef NO_EDITOR
	const Uint32 yPos = 160;

	if ( m_showPinsInfoFlags > 0 )
	{
		if ( m_mapPins.Size() )
		{
			Uint32 i = 0;

			THashMap< Int32, SCommonMapPinInstance >::iterator itEnd = m_mapPins.End();
			for ( THashMap< Int32, SCommonMapPinInstance >::iterator itPin = m_mapPins.Begin(); itPin != itEnd; ++itPin )
			{
				const SCommonMapPinInstance& pin = itPin->m_second;

				Bool showPinInfo = false;
				if ( m_showPinsInfoFlags == MPGT_All )
				{
					showPinInfo = true;
				}
				else
				{

					if ( m_showPinsInfoFlags & MPGT_FastTravel )
					{
						if ( pin.m_type == CNAME( RoadSign ) ||
							 pin.m_type == CNAME( Harbor ) )
						{
							showPinInfo = true;
						}
					}
					if ( m_showPinsInfoFlags & MPGT_Quest )
					{
						if ( CCommonMapManager::IsQuestPinType( pin.m_type ) || pin.m_type == CNAME( QuestAvailable ) )
						{
							showPinInfo = true;
						}
					}
					if ( m_showPinsInfoFlags & MPGT_Dynamic )
					{
						if ( pin.m_isDynamic )
						{
							showPinInfo = true;
						}
					}
					if ( m_showPinsInfoFlags & MPGT_Static )
					{
						if ( !pin.m_isDynamic )
						{
							showPinInfo = true;
						}
					}
					if ( m_showPinsInfoFlags & MPGT_Knowable )
					{
						if ( IsKnowableMapPinType( pin.m_type ) )
						{
							showPinInfo = true;
						}
					}
				}

				if ( !showPinInfo )
				{
					continue;
				}

				Color textColor = Color::RED;
				if ( pin.m_isDiscovered && pin.m_isAddedToMinimap )
				{
					textColor = Color::GREEN;
				}
				else if ( pin.m_isDiscovered )
				{
					textColor = Color::YELLOW;
				}

				String line;
				line = String::Printf( TXT("%d"), i );
				frame->AddDebugScreenText( 30,  yPos + i * 19, line, 0, true, textColor, Color::BLACK, nullptr );
				line = String::Printf( TXT("%s"), pin.m_type.AsChar() );
				frame->AddDebugScreenText( 50,  yPos + i * 19, line, 0, true, textColor, Color::BLACK, nullptr );
				line = String::Printf( TXT("%s"), pin.m_visibleType.AsChar() );
				frame->AddDebugScreenText( 150,  yPos + i * 19, line, 0, true, textColor, Color::BLACK, nullptr );
				line = String::Printf( TXT("%s"), pin.m_tag.AsChar() );
				frame->AddDebugScreenText( 250, yPos + i * 19, line, 0, true, textColor, Color::BLACK, nullptr );
				line = String::Printf( TXT("%s"), pin.m_extraTag.AsChar() );
				frame->AddDebugScreenText( 440, yPos + i * 19, line, 0, true, textColor, Color::BLACK, nullptr );
				line = String::Printf( TXT("%d %d%d%d %d%d %d %d%d"), pin.m_isDynamic, pin.m_isKnown, pin.m_isDiscovered, pin.m_isDisabled, pin.m_isHighlightable, pin.m_isHighlighted, pin.m_canBePointedByArrow, pin.m_canBeAddedToMinimap, pin.m_isAddedToMinimap );
				frame->AddDebugScreenText( 540, yPos + i * 19, line, 0, true, textColor, Color::BLACK, nullptr );
				line = String::Printf( TXT("%.2f"), pin.m_radius );
				frame->AddDebugScreenText( 620, yPos + i * 19, line, 0, true, textColor, Color::BLACK, nullptr );
				line = String::Printf( TXT("(%.2f %.2f %.2f)"), pin.m_position.X, pin.m_position.Y, pin.m_position.Z );
				frame->AddDebugScreenText( 650, yPos + i * 19, line, 0, true, textColor, Color::BLACK, nullptr );
				line = String::Printf( TXT("%u"), static_cast< Uint32 >( pin.m_id ) );
				frame->AddDebugScreenText( 850, yPos + i * 19, line, 0, true, textColor, Color::BLACK, nullptr );

				i++;
			}
		}
		else
		{
			frame->AddDebugScreenText( 30,  yPos, TXT("THERE ARE NO MAPPINS HERE"), 0, true, Color::RED, Color::BLACK, nullptr );
		}

		Uint32 i = 0;

		if ( m_knownEntityMapPinTags.Size() > 0 )
		{
			frame->AddDebugScreenText( 1100,  yPos + i * 19, TXT("KNOWN MAPPINS"), 0, true, Color::WHITE, Color::BLACK, nullptr );
			i += 2;
			for ( TSet< CName >::iterator it = m_knownEntityMapPinTags.Begin(); it != m_knownEntityMapPinTags.End(); it++ )
			{
				String line;
				line = String::Printf( TXT("%s"), it->AsChar() );
				frame->AddDebugScreenText( 1100,  yPos + i * 19, line, 0, true, Color::WHITE, Color::BLACK, nullptr );
				i++;
			}
		}
		else
		{
			frame->AddDebugScreenText( 1100,  yPos + i * 19, TXT("NO KNOWN MAPPINS"), 0, true, Color::RED, Color::BLACK, nullptr );
		}

		i += 2;

		if ( m_discoveredEntityMapPinTags.Size() > 0 )
		{
			frame->AddDebugScreenText( 1100,  yPos + i * 19, TXT("DISCOVERED MAPPINS"), 0, true, Color::WHITE, Color::BLACK, nullptr );
			i += 2;
			for ( TSet< CName >::iterator it = m_discoveredEntityMapPinTags.Begin(); it != m_discoveredEntityMapPinTags.End(); it++ )
			{
				String line;
				line = String::Printf( TXT("%s"), it->AsChar() );
				frame->AddDebugScreenText( 1100,  yPos + i * 19, line, 0, true, Color::WHITE, Color::BLACK, nullptr );
				i++;
			}
		}
		else
		{
			frame->AddDebugScreenText( 1100,  yPos + i * 19, TXT("NO DISCOVERED MAPPINS"), 0, true, Color::RED, Color::BLACK, nullptr );
		}

		i += 2;

		if ( m_disabledEntityMapPinTags.Size() > 0 )
		{
			frame->AddDebugScreenText( 1100,  yPos + i * 19, TXT("DISABLED MAPPINS"), 0, true, Color::WHITE, Color::BLACK, nullptr );
			i += 2;
			for ( TSet< CName >::iterator it = m_disabledEntityMapPinTags.Begin(); it != m_disabledEntityMapPinTags.End(); it++ )
			{
				String line;
				line = String::Printf( TXT("%s"), it->AsChar() );
				frame->AddDebugScreenText( 1100,  yPos + i * 19, line, 0, true, Color::WHITE, Color::BLACK, nullptr );
				i++;
			}
		}
		else
		{
			frame->AddDebugScreenText( 1100,  yPos + i * 19, TXT("NO DISABLED MAPPINS"), 0, true, Color::RED, Color::BLACK, nullptr );
		}
	}

	if ( m_showPathsInfo )
	{
		if ( m_mapPaths.Size() )
		{
			Uint32 i = 0;
			for ( THashMap< Int32, SMapPathInstance >::iterator itPath = m_mapPaths.Begin(); 
				itPath != m_mapPaths.End();
				++itPath )
			{
				const SMapPathInstance& mapPath = itPath->m_second;

				String line;
				line = String::Printf( TXT("%d %d"), mapPath.m_id, mapPath.m_isAddedToMinimap );
				frame->AddDebugScreenText( 30,  yPos + i * 19, line, 0, true, Color::GREEN, Color::BLACK, nullptr );

				i++;
			}
		}
		else
		{
			frame->AddDebugScreenText( 30,  yPos, TXT("THERE ARE NO MAPPATHS HERE"), 0, true, Color::RED, Color::BLACK, nullptr );
		}

		Uint32 i = 0;

		if ( m_discoveredPaths.Size() > 0 )
		{
			frame->AddDebugScreenText( 1100,  yPos + i * 19, TXT("VISIBLE MAPPATHS"), 0, true, Color::WHITE, Color::BLACK, nullptr );
			i += 2;
			for ( THashMap< CName, SMapPathDefinition >::iterator it = m_discoveredPaths.Begin(); it != m_discoveredPaths.End(); ++it )
			{
				const SMapPathDefinition& mapPath = it->m_second;
				String line;
				line = String::Printf( TXT("%s   %d %d [%d %d %d %d]"), mapPath.m_tag.AsChar(), mapPath.m_lineWidth, mapPath.m_lineSegmentLength, mapPath.m_color.R, mapPath.m_color.G, mapPath.m_color.B );
				frame->AddDebugScreenText( 1100,  yPos + i * 19, line, 0, true, Color::WHITE, Color::BLACK, nullptr );
				i++;
			}
		}
		else
		{
			frame->AddDebugScreenText( 1100,  yPos + i * 19, TXT("NO VISIBLE MAPPATHS"), 0, true, Color::RED, Color::BLACK, nullptr );
		}
	}

	/////////////////////////////////////////////////////////
	//
	if ( m_showQuestAgentsInfo )
	{
		CCommunitySystem* communitySystem = GCommonGame->GetSystem< CCommunitySystem >();
		if ( !communitySystem )
		{
			return;
		}

		CCommunityAgentStubTagManager* agentStubTagManager = communitySystem->GetAgentStubTagManager();
		if ( !agentStubTagManager )
		{
			return;
		}

		TDynArray< SAgentStub* > agentStubs;

		if ( m_customAgentMapPins.Size() )
		{
			Uint32 i = 0;
			for ( THashMap< CName, CName >::iterator itAgent = m_customAgentMapPins.Begin(); 
				itAgent != m_customAgentMapPins.End();
				++itAgent )
			{
				CName tag = (*itAgent).m_first;

				agentStubTagManager->CollectTaggedStubs( tag, agentStubs );

				Bool isDiscovered = m_discoveredAgentEntityTags.Exist( tag );
				Uint32 agentsCount = agentStubs.Size();

				String line = String::Printf( TXT("%d   %ls"), agentsCount, tag.AsChar() );

				Color col = Color::WHITE;
				if ( isDiscovered )
				{
					if ( agentsCount > 0 )
					{
						col = Color::GREEN;
					}
					else
					{
						col = Color::RED;
					}
				}
				else if ( !isDiscovered )
				{
					if ( agentsCount > 0 )
					{
						col = Color::YELLOW;
					}
					else
					{
						col = Color( 255, 127, 0 );
					}
				}
				frame->AddDebugScreenText( 30,  yPos + i * 19, line, 0, true, col, Color::BLACK, nullptr );

				i++;
			}
		}
		else
		{
			frame->AddDebugScreenText( 30,  yPos, TXT("THERE ARE NO POSSIBLE QUEST AGENTS"), 0, true, Color::RED, Color::BLACK, nullptr );
		}

		Uint32 i = 0;

		if ( m_discoveredAgentEntityTags.Size() > 0 )
		{
			frame->AddDebugScreenText( 1100,  160 + i * 19, TXT("DISCOVERED AGENT TAGS"), 0, true, Color::WHITE, Color::BLACK, nullptr );
			i += 2;
			for ( TSet< CName >::iterator it = m_discoveredAgentEntityTags.Begin(); it != m_discoveredAgentEntityTags.End(); ++it )
			{
				String line;
				line = String::Printf( TXT("%s"), (*it).AsChar() );
				frame->AddDebugScreenText( 1100,  160 + i * 19, line, 0, true, Color::WHITE, Color::BLACK, nullptr );
				i++;
			}
		}
		else
		{
			frame->AddDebugScreenText( 1100,  160 + i * 19, TXT("NO DISCOVERED AGENT TAGS"), 0, true, Color::RED, Color::BLACK, nullptr );
		}
	}

	if ( m_showShopkeepersInfo )
	{
		const Uint32 yPos = 250;

		CCachedWorldData* cachedData = GetCurrentWorldCachedData();
		if ( cachedData )
		{
			const THashMap< CName, CCachedShopkeeperData >& shopkeeperData = cachedData->GetShopkeeperData();

			if ( shopkeeperData.Size() )
			{
				Uint32 i = 0;
				String line;
				for ( THashMap< CName, CCachedShopkeeperData >::const_iterator it = shopkeeperData.Begin(); it != shopkeeperData.End(); ++it )
				{
					const CCachedShopkeeperData& data = it->m_second;

					Color col = Color::WHITE;
					if ( !data.m_initialized )
					{
						col = Color::RED;
					}
					else
					{
						if ( data.m_enabled )
						{
							col = Color::GREEN;
						}
						else
						{
							col = Color::YELLOW;
						}
					}

					line = String::Printf( TXT("%d"), data.GetId() );
					frame->AddDebugScreenText( 600 + 30,  yPos + i * 19, line, 0, true, col, Color::BLACK, nullptr );
					line = String::Printf( TXT("%s"), data.GetTag().AsChar() );
					frame->AddDebugScreenText( 600 + 130,  yPos + i * 19, line, 0, true, col, Color::BLACK, nullptr );
					line = String::Printf( TXT("%s"), data.GetType().AsChar() );
					frame->AddDebugScreenText( 600 + 400,  yPos + i * 19, line, 0, true, col, Color::BLACK, nullptr );
					line = String::Printf( TXT("%.2f %.2f %.2f"), data.GetPosition().X, data.GetPosition().Y, data.GetPosition().Z );
					frame->AddDebugScreenText( 600 + 500,  yPos + i * 19, line, 0, true, col, Color::BLACK, nullptr );
					line = String::Printf( TXT("%d %d %d%d"), !!data.GetEntity(), data.IsCacheable(), data.IsInitialzed(), data.IsEnabled() );
					frame->AddDebugScreenText( 600 + 600,  yPos + i * 19, line, 0, true, col, Color::BLACK, nullptr );
					i++;
				}
			}
			else
			{
				frame->AddDebugScreenText( 630,  yPos, TXT("THERE IS NO SHOPKEEPER DATA"), 0, true, Color::RED, Color::BLACK, nullptr );
			}
		}
	}

	if ( m_showInteriorInfo )
	{
		const Uint32 yPos = 250;

		Uint32 i = 0;
		String line;

		Color col = Color::WHITE;

		if ( !m_playerInteriors.Size() )
		{
			frame->AddDebugScreenText( 100 + 30,  yPos + i * 19, TXT("NO INTERIORS"), 0, true, col, Color::BLACK, nullptr );
		}
		else
		{
			for ( Uint32 i = 0; i < m_playerInteriors.Size(); ++i )
			{
				CR4InteriorAreaComponent* interior = m_playerInteriors[ i ];

				line = String::Printf( TXT("%X [%s]"), interior, interior? interior->GetTexture().AsChar(): TXT("?") );
				frame->AddDebugScreenText( 100 + 30,  yPos + i * 19, line, 0, true, col, Color::BLACK, nullptr );
			}
		}
	}

	if ( m_showUserPinsInfo )
	{
		const Uint32 yPos = 200;

		Uint32 i = 0;
		String line;
		Color col = Color::WHITE;

		if ( !m_userMapPins.Size() )
		{
			frame->AddDebugScreenText( 100 + 30,  yPos + i * 19, TXT("NO USER PINS"), 0, true, Color::WHITE, Color::BLACK, nullptr );
		}
		else
		{
			for ( Uint32 i = 0; i < m_userMapPins.Size(); ++i )
			{
				const CUserMapPinData& data = m_userMapPins[ i ];
				line = String::Printf( TXT("%d [%ud] [%d %d] [%.2f %.2f]"), i, data.m_id, data.m_area, data.m_type, data.m_position.X, data.m_position.Y );
				if ( data.m_area == -1 )
				{
					col = Color::WHITE;
				}
				else
				{
					switch ( data.m_type )
					{
					case 0:
						col = Color::LIGHT_GREEN;
						break;
					case 1:
						col = Color::LIGHT_BLUE;
						break;
					case 2:
						col = Color::LIGHT_CYAN;
						break;
					case 3:
						col = Color::RED;
						break;
					default:
						col = Color::WHITE;
						break;
					}
				}
				frame->AddDebugScreenText( 100 + 30,  yPos + i * 19, line, 0, true, col, Color::BLACK, nullptr );
			}
		}
	}

#endif //NO_EDITOR

#ifndef NO_EDITOR_FRAGMENTS
	if ( m_showHintWaypoints )
	{
		m_mapTracking->OnGenerateDebugFragments( frame );
	}

	const CRenderFrameInfo & frameInfo = frame->GetFrameInfo();
	if ( frameInfo.IsShowFlagOn( SHOW_QuestMapPins ) )
	{
		const CCachedWorldData* cachedData = GetCurrentWorldCachedData();
		const THashMap< CName, SQuestMapPinInfo >& questMapPins = cachedData->GetStaticQuestMapPins();
		for ( THashMap< CName, SQuestMapPinInfo >::const_iterator it = questMapPins.Begin(); it != questMapPins.End(); ++it )
		{
			SQuestMapPinInfo mpi = it->m_second;
			if ( !mpi.m_positions.Empty() )
			{
				Float dist = ( mpi.m_positions[0] - frameInfo.m_camera.GetPosition() ).SquareMag3();
				Vector spherePos = mpi.m_positions[0];
				for ( Uint32 i = 1; i < mpi.m_positions.Size(); ++i )
				{
					Float tmpDist = Min( dist, ( mpi.m_positions[i] - frameInfo.m_camera.GetPosition() ).SquareMag3() );
					if ( tmpDist != dist )
					{
						dist = tmpDist;
						spherePos = mpi.m_positions[i];
					}
				}
				if ( dist < 10000.f )
				{
					frame->AddDebugSphere( spherePos, 1.f, Matrix::IDENTITY, Color( 255, 0, 255 ) );
				}
			}
		}
	}

#endif

}

Int32 CCommonMapManager::GetCurrentJournalArea()
{
	Int32 journalArea = 0;
	if ( CallFunctionRet( this, CNAME( GetCurrentJournalArea ), journalArea ) )
	{
		return journalArea;
	}
	return GetCurrentArea();
}

void CCommonMapManager::UpdateCurrentArea()
{
	const CWorld* world = GGame->GetActiveWorld();
	if ( !world )
	{
		return;
	}
	const String& path = world->GetDepotPath();

	m_currentArea = GetAreaFromWorldPath( path );
}

Int32 CCommonMapManager::GetAreaFromWorldPath( const String& worldPath ) const
{
	for ( Uint32 i = 0; i < m_areaMapPins.Size(); ++i )
	{
		if ( m_areaMapPins[ i ].m_worldPath == worldPath )
		{
			return m_areaMapPins[ i ].m_areaType;
		}
	}
	return 0;
}

const CName& CCommonMapManager::GetCurrentLocalisationName() const
{
	return GetLocalisationNameFromWorldPath( m_currentWorldPath );
}

const CName& CCommonMapManager::GetLocalisationNameFromWorldPath( const String& worldPath ) const
{
	for ( Uint32 i = 0; i < m_areaMapPins.Size(); ++i )
	{
		if ( m_areaMapPins[ i ].m_worldPath == worldPath )
		{
			return m_areaMapPins[ i ].m_localisationName;
		}
	}
	return CName::NONE;
}

// yes, I know, this is wrong
// but we have different cases: novigrad.w2w is a single world for both novigrad and velen areas
// and there separate prolog_village.w2w worlds that relate to the same area
// there's no point to call script functions as enums for areas won't change anymore
Int32 CCommonMapManager::ConvertJournalAreaToRealArea( Int32 journalArea ) const
{
	if ( journalArea == 9 )
	{
		// if velen area, return novigrad world
		return 1;
	}
	else if ( journalArea == 8 )
	{
		// if prolog village winter area, return prolog village world
		return 4;
	}
	return journalArea;
}

Int32 CCommonMapManager::ConvertFakeAreaToRealArea( Int32 fakeArea ) const
{
	if ( fakeArea == 8 )
	{
		// if prolog village winter area, return prolog village world
		return 4;
	}
	return fakeArea;
}

Int32 CCommonMapManager::FindInteriorContainingPoint( const Vector& position, CR4InteriorAreaComponent** foundInteriors, Int32 maxElements )
{
	return m_interiorsRegistry.FindInteriorContainingPoint( position, foundInteriors, maxElements );
}

CR4InteriorAreaComponent* CCommonMapManager::GetInteriorFromPosition( const Vector& position )
{
	CR4InteriorAreaComponent* foundInterior = nullptr;
	return m_interiorsRegistry.FindInteriorContainingPoint( position, &foundInterior, 1 ) ? foundInterior : nullptr;
}

void CCommonMapManager::InvalidateQuestMapPinData()
{
	m_invalidatedQuestMapPinData = true;

	// additionally invalidate closest fat travel point
	InvalidateClosestFastTravelPoint();
}

void CCommonMapManager::ScheduleUpdateDynamicMapPins()
{
	m_dynamicPinsUpdateInterval = 0;
	m_dynamicPinsPositionUpdateInterval = DYNAMIC_PINS_POSITION_UPDATE_INTERVAL;
}

void CCommonMapManager::ScheduleUpdateStaticMapPins()
{
	m_staticPinsUpdateInterval = 0;
}

Bool CCommonMapManager::IsJournalQuestMapPinEnabled( const CGUID& objectiveGUID, const CGUID& mapPinGUID )
{
	Bool* enabled = m_questMapPinStates.FindPtr( TPair< CGUID, CGUID >( objectiveGUID, mapPinGUID ) );
	if ( !enabled )
	{
		return true;
	}
	return *enabled;
}

void CCommonMapManager::EnableJournalQuestMapPin( const CGUID& objectiveGUID, const CGUID& mapPinGUID, Bool enabled )
{
	// set or update map pin state
	TPair< CGUID, CGUID > pair( objectiveGUID, mapPinGUID );
	if ( m_questMapPinStates.KeyExist( pair ) )
	{
		m_questMapPinStates.Set( pair, enabled );
	}
	else
	{
		m_questMapPinStates.Insert( pair, enabled );
	}
}

void CCommonMapManager::DeleteJournalQuestMapPin( const CGUID& objectiveGUID, const CGUID& mapPinGUID )
{
	// delete map pin state on succeeded/failed quest/phase/objective
	TPair< CGUID, CGUID > pair( objectiveGUID, mapPinGUID );
	if ( m_questMapPinStates.KeyExist( pair ) )
	{
		m_questMapPinStates.Erase( pair );
	}
}

Bool CCommonMapManager::IsEntityMapPinKnown( const CName& tag )
{
	if ( tag == CName::NONE )
	{
		return false;
	}
	return m_knownEntityMapPinTags.Exist( tag );
}

void CCommonMapManager::SetEntityMapPinKnown( const CName& tag, Bool set /*= true*/ )
{
	if ( tag == CName::NONE )
	{
		return;
	}
	if ( set )
	{
		if ( !m_knownEntityMapPinTags.Exist( tag ) )
		{
//#if !defined(NO_SECOND_SCREEN) 
//			SCSecondScreenManager::GetInstance().SendStaticMapPinDiscovered( tag );
//#endif			
			m_knownEntityMapPinTags.Insert( tag );
		}
	}
	else
	{
		if ( m_knownEntityMapPinTags.Exist( tag ) )
		{
			m_knownEntityMapPinTags.Erase( tag );
		}
	}

	TDynArray< SCommonMapPinInstance* > instances;
	FindMapPinInstancesByTag( tag, instances );
	for ( Uint32 i = 0; i < instances.Size(); i++ )
	{
		instances[ i ]->m_isKnown = set;
		instances[ i ]->m_invalidated = true;
	}
	if ( instances.Size() )
	{
		m_invalidatedEntityMapPins = true;
		ScheduleUpdateStaticMapPins();
	}
}

Bool CCommonMapManager::IsEntityMapPinDiscovered( const CName& tag )
{
	if ( tag == CName::NONE )
	{
		return false;
	}
	return m_discoveredEntityMapPinTags.Exist( tag );
}

void CCommonMapManager::SetEntityMapPinDiscovered( const CName& tag, Bool set /*= true*/ )
{
	if ( tag == CName::NONE )
	{
		return;
	}
	if ( set )
	{
		if ( !m_discoveredEntityMapPinTags.Exist( tag ) )
		{
#if !defined(NO_SECOND_SCREEN) 
			SCSecondScreenManager::GetInstance().SendStaticMapPinDiscovered( tag );
#endif			
			m_discoveredEntityMapPinTags.Insert( tag );
		}
	}
	else
	{
		if ( m_discoveredEntityMapPinTags.Exist( tag ) )
		{
			m_discoveredEntityMapPinTags.Erase( tag );
		}
	}

	TDynArray< SCommonMapPinInstance* > instances;
	FindMapPinInstancesByTag( tag, instances );
	for ( Uint32 i = 0; i < instances.Size(); i++ )
	{
		instances[ i ]->m_isDiscovered = set;
		instances[ i ]->m_invalidated = true;
	}
	if ( instances.Size() )
	{
		m_invalidatedEntityMapPins = true;
		ScheduleUpdateStaticMapPins();

		// todo check if it's fast travel
		InvalidateClosestFastTravelPoint();
	}

	// inform scripts
	CallEvent( CNAME( OnMapPinChanged ) );
}

Bool CCommonMapManager::IsEntityMapPinDisabled( const CName& tag )
{
	if ( tag == CName::NONE )
	{
		return false;
	}
	return m_disabledEntityMapPinTags.Exist( tag );
}

void CCommonMapManager::SetEntityMapPinDisabled( const CName& tag, Bool set /*= true*/ )
{
	if ( tag == CName::NONE )
	{
		return;
	}
	if ( set )
	{
		if ( !m_disabledEntityMapPinTags.Exist( tag ) )
		{
			m_disabledEntityMapPinTags.Insert( tag );
		}
	}
	else
	{
		if ( m_disabledEntityMapPinTags.Exist( tag ) )
		{
			m_disabledEntityMapPinTags.Erase( tag );
		}
	}

	TDynArray< SCommonMapPinInstance* > instances;
	FindMapPinInstancesByTag( tag, instances );
	for ( Uint32 i = 0; i < instances.Size(); i++ )
	{
		instances[ i ]->m_isDisabled = set;
		instances[ i ]->m_invalidated = true;
	}
	if ( instances.Size() )
	{
		m_invalidatedEntityMapPins = true;
		ScheduleUpdateStaticMapPins();
	}

	// inform scripts
	CallEvent( CNAME( OnMapPinChanged ) );
}

Bool CCommonMapManager::LoadEntityMapPinsForDLC( const CName& dlcId, const String& worldPath, const String& filename )
{
	CCachedWorldData* cachedData = GetWorldCachedData( worldPath );
	if ( cachedData )
	{
		if ( !cachedData->AreEntityMapPinsLoadedForDLC( dlcId ) )
		{
			CEntityMapPinsResource* resource = Cast< CEntityMapPinsResource >( GDepot->LoadResource( filename ) );
			if ( resource )
			{
				cachedData->LoadEntityMapPinsForDLC( dlcId, resource->GetMapPinsInfo() );

				if ( m_currentWorldPath == worldPath )
				{
					// if entity map pins have been already initialized for given worldPath, let's add new ones
					if ( m_entityMapPinsInitializedFor.Exist( worldPath ) )
					{
						AddEntityMapPins( resource->GetMapPinsInfo() );
					}
				}
			}
		}
	}
	return true;
}

Bool CCommonMapManager::LoadQuestMapPinsForDLC( const CName& dlcId, const String& worldPath, const String& filename )
{
	CCachedWorldData* cachedData = GetWorldCachedData( worldPath );
	if ( cachedData )
	{
		if ( !cachedData->AreQuestMapPinsLoadedForDLC( dlcId ) )
		{
			CQuestMapPinsResource* resource = Cast< CQuestMapPinsResource >( GDepot->LoadResource( filename ) );
			if ( resource )
			{
				cachedData->LoadQuestMapPinsForDLC( dlcId, resource->GetMapPinsInfo() );
			}
		}
	}
	return true;
}

void CCommonMapManager::ExportGlobalMapPins()
{
#ifndef NO_EDITOR
	String											filePath;
	TDynArray< SAreaMapPinInfo >	mappins;

	if ( !CallFunctionRef( this, CNAME( GetAreaMappinsFileName ), filePath ) ||
	     !CallFunctionRef( this, CNAME( GetAreaMappinsData ), mappins ) )
	{
		return;
	}

	CAreaMapPinsResource* areaMappinsResource = NULL;

	CDiskFile* file = GDepot->FindFile( filePath );
	if ( file )
	{
		// file exists, load and get resource
		if ( file->Load() )
		{
			areaMappinsResource = Cast< CAreaMapPinsResource >( file->GetResource() );
		}
	}
	else
	{
		// file doesn't exists, create resource
		areaMappinsResource = ::CreateObject< CAreaMapPinsResource >();
	}

	if ( areaMappinsResource )
	{
		TDynArray< SAreaMapPinInfo >& areaMapPins = areaMappinsResource->GetMapPinsInfo();
		areaMapPins.ClearFast();
		for ( Uint32 i = 0; i < mappins.Size(); i++ )
		{
			areaMapPins.PushBack( mappins[ i ] );
		}

		if ( file )
		{
			if ( !file->IsCheckedOut() )
			{
				file->CheckOut();
			}
			areaMappinsResource->Save();
		}
		else
		{
			CDirectory* resourceDir = GDepot->FindPath( filePath.AsChar() );
			ASSERT( resourceDir );
			if ( resourceDir )
			{
				String resourceFile;
				if ( GetFileFromPath( filePath, resourceFile, 0 ) )
				{
					areaMappinsResource->SaveAs( resourceDir, resourceFile );
				}
			}
		}
	}
#endif //NO_EDITOR
}

void CCommonMapManager::ExportEntityMapPins( Uint32 expansionPackIndex /*= 0*/ )
{
#ifndef NO_EDITOR
	if ( GGame->IsActive() )
	{
		GFeedback->ShowMsg( TXT("Export"), TXT("Please quit the game first") );
		return;
	}

	CWorld* world = GGame->GetActiveWorld();
	if ( !world )
	{
		return;
	}

	// some checks just in case
	String worldPath = world->GetDepotPath();
	if ( expansionPackIndex == 1 )
	{
		if ( !worldPath.EndsWith( TXT("novigrad.w2w") ) )
		{
			GFeedback->ShowError( TXT("EP1 takes place only in novigrad.w2w") );
			return;
		}
	}
	else if ( expansionPackIndex == 2 )
	{
		if ( !worldPath.EndsWith( TXT("bob.w2w") ) )
		{
			GFeedback->ShowError( TXT("EP2 takes place only in bob.w2w") );
			return;
		}
	}

	CLayerGroup* worldLayers = world->GetWorldLayers();
	if ( !worldLayers )
	{
		return;
	}

	CTagManager* tagManager = world->GetTagManager();
	if ( !tagManager )
	{
		return;
	}

	String resourcePath;

	if ( !GetEntityMappinsResourcePath( worldPath, resourcePath, expansionPackIndex ) )
	{
		return;
	}

	CEntityMapPinsResource* entityMappinsResource = NULL;

	CDiskFile* file = GDepot->FindFile( resourcePath );
	if ( file )
	{
		if ( file->Load() )
		{
			entityMappinsResource = Cast< CEntityMapPinsResource >( file->GetResource() );
		}
	}
	else
	{
		entityMappinsResource = ::CreateObject< CEntityMapPinsResource >();
	}

	if ( !entityMappinsResource )
	{
		return;
	}

	entityMappinsResource->ClearMapPinsInfo();

	TDynArray< CLayerInfo* > layers;
	world->GetWorldLayers()->GetLayers( layers, true );

	CClass* fastTravelClass  = SRTTI::GetInstance().FindClass( CNAME( W3FastTravelEntity ) );
	ASSERT( fastTravelClass );
	CClass* noticeBoardClass = SRTTI::GetInstance().FindClass( CNAME( W3NoticeBoard ) );
	ASSERT( noticeBoardClass );
	CClass* stashClass = SRTTI::GetInstance().FindClass( CNAME( W3Stash ) );
	ASSERT( stashClass );
	CClass* monsterNestClass = SRTTI::GetInstance().FindClass( CNAME( CMonsterNestEntity ) );
	ASSERT( monsterNestClass );
	CClass* majorPlaceOfPowerClass = SRTTI::GetInstance().FindClass( CNAME( CMajorPlaceOfPowerEntity ) );
	ASSERT( majorPlaceOfPowerClass );
	CClass* magicLampClass = SRTTI::GetInstance().FindClass( CNAME( W3MagicLampEntity ) );
	ASSERT( magicLampClass );
	CClass* treasureHuntMappinClass = SRTTI::GetInstance().FindClass( CNAME( W3TreasureHuntMappinEntity ) );
	ASSERT( treasureHuntMappinClass );
	CClass* teleportClass = SRTTI::GetInstance().FindClass( CNAME( CTeleportEntity ) );
	ASSERT( teleportClass );
	CClass* riftClass = SRTTI::GetInstance().FindClass( CNAME( CRiftEntity ) );
	ASSERT( riftClass );
	CClass* whetstoneClass = SRTTI::GetInstance().FindClass( CNAME( W3ItemRepairObject ) );
	ASSERT( whetstoneClass );
	CClass* mutagenDismantlingTableClass = SRTTI::GetInstance().FindClass( CNAME( W3MutagenDismantlingTable ) );
	ASSERT( mutagenDismantlingTableClass );
	CClass* alchemyTableClass = SRTTI::GetInstance().FindClass( CNAME( W3AlchemyTable ) );
	ASSERT( alchemyTableClass );
	CClass* stablesClass = SRTTI::GetInstance().FindClass( CNAME( W3Stables ) );
	ASSERT( stablesClass );
	CClass* bookshelfClass = SRTTI::GetInstance().FindClass( CNAME( W3Bookshelf ) );
	ASSERT( bookshelfClass );
	CClass* witcherBedClass = SRTTI::GetInstance().FindClass( CNAME( W3WitcherBed ) );
	ASSERT( witcherBedClass );
	CClass* witcherHouseClass = SRTTI::GetInstance().FindClass( CNAME( W3WitcherHouse ) );
	ASSERT( witcherHouseClass );
	CClass* entranceClass = SRTTI::GetInstance().FindClass( CNAME( W3EntranceEntity ) );
	ASSERT( entranceClass );
	CClass* spoilsOfWarClass = SRTTI::GetInstance().FindClass( CNAME( W3POI_SpoilsOfWarEntity ) );
	ASSERT( spoilsOfWarClass );
	CClass* banditCampClass = SRTTI::GetInstance().FindClass( CNAME( W3POI_BanditCampEntity ) );
	ASSERT( banditCampClass );
	CClass* banditCampfireClass = SRTTI::GetInstance().FindClass( CNAME( W3POI_BanditCampfireEntity ) );
	ASSERT( banditCampfireClass );
	CClass* bossAndTreasureClass = SRTTI::GetInstance().FindClass( CNAME( W3POI_BossAndTreasureEntity ) );
	ASSERT( bossAndTreasureClass );
	CClass* contrabandClass = SRTTI::GetInstance().FindClass( CNAME( W3POI_ContrabandEntity ) );
	ASSERT( contrabandClass );
	CClass* contrabandShipClass = SRTTI::GetInstance().FindClass( CNAME( W3POI_ContrabandShipEntity ) );
	ASSERT( contrabandShipClass );
	CClass* rescuingTownClass = SRTTI::GetInstance().FindClass( CNAME( W3POI_RescuingTownEntity ) );
	ASSERT( rescuingTownClass );
	CClass* dungeonCrawlClass = SRTTI::GetInstance().FindClass( CNAME( W3POI_DungeonCrawlEntity ) );
	ASSERT( dungeonCrawlClass );
	CClass* hideoutClass = SRTTI::GetInstance().FindClass( CNAME( W3POI_HideoutEntity ) );
	ASSERT( hideoutClass );
	CClass* plegmundClass = SRTTI::GetInstance().FindClass( CNAME( W3POI_PlegmundEntity ) );
	ASSERT( plegmundClass );
	CClass* knightErrantClass = SRTTI::GetInstance().FindClass( CNAME( W3POI_KnightErrantEntity ) );
	ASSERT( knightErrantClass );
	CClass* wineContractClass = SRTTI::GetInstance().FindClass( CNAME( W3POI_WineContractEntity ) );
	ASSERT( wineContractClass );
	CClass* signalingStakeClass = SRTTI::GetInstance().FindClass( CNAME( W3POI_SignalingStakeEntity ) );
	ASSERT( signalingStakeClass );

	Uint32 succeeded = 0;
	Uint32 failed = 0;
	String errorText;

	TDynArray< CLayer* > m_layersToSave;

	String expansionPackPath = String::Printf( TXT("dlc\\ep%d\\"), expansionPackIndex );

	for ( Uint32 k = 0; k < layers.Size(); k++ )
	{
		CLayerInfo* info = layers[k];

		if ( expansionPackIndex == 0 )
		{
			if ( info->IsDLC() )
			{
				continue;
			}
		}
		else if ( expansionPackIndex == 1 )
		{
			if ( !info->IsDLC() )
			{
				continue;
			}
			if ( !info->GetDepotPath().ToLower().BeginsWith( expansionPackPath ) )
			{
				continue;
			}
		}
		else if ( expansionPackIndex == 2 )
		{
			// do nothing, process all layers
		}
		else
		{
			continue;
		}

		if ( info->IsLoaded() )
		{
			CLayer* layer = info->GetLayer();

			TDynArray<CEntity*> entities;
			layer->GetEntities( entities );
			for ( Uint32 i = 0; i < entities.Size(); ++i )
			{
				CR4MapPinEntity* mappinEntity = Cast< CR4MapPinEntity >( entities[i] );
				if ( !mappinEntity )
				{
					continue;
				}
				if ( mappinEntity->ShouldBeIgnoredWhenExportingMapPins() )
				{
					continue;
				}

				// continue if it's not a proper class
				if ( !mappinEntity->IsA( fastTravelClass ) &&
				     !mappinEntity->IsA( noticeBoardClass ) &&
					 !mappinEntity->IsA( stashClass ) &&
				     !mappinEntity->IsA( monsterNestClass ) &&
				     !mappinEntity->IsA( majorPlaceOfPowerClass ) &&
					 !mappinEntity->IsA( magicLampClass ) &&
					 !mappinEntity->IsA( treasureHuntMappinClass ) &&
					 !mappinEntity->IsA( teleportClass ) &&
					 !mappinEntity->IsA( riftClass ) &&
					 !mappinEntity->IsA( whetstoneClass ) &&
					 !mappinEntity->IsA( mutagenDismantlingTableClass ) &&
					 !mappinEntity->IsA( alchemyTableClass ) &&
					 !mappinEntity->IsA( stablesClass ) &&
					 !mappinEntity->IsA( bookshelfClass ) &&
					 !mappinEntity->IsA( witcherBedClass ) &&
					 !mappinEntity->IsA( witcherHouseClass ) &&
					 !mappinEntity->IsA( entranceClass ) &&
					 !mappinEntity->IsA( spoilsOfWarClass ) &&
					 !mappinEntity->IsA( banditCampClass ) &&
					 !mappinEntity->IsA( banditCampfireClass ) &&
					 !mappinEntity->IsA( bossAndTreasureClass ) &&
					 !mappinEntity->IsA( contrabandClass ) &&
					 !mappinEntity->IsA( contrabandShipClass ) &&
					 !mappinEntity->IsA( rescuingTownClass ) &&
					 !mappinEntity->IsA( dungeonCrawlClass ) &&
					 !mappinEntity->IsA( hideoutClass ) &&
					 !mappinEntity->IsA( plegmundClass ) &&
					 !mappinEntity->IsA( knightErrantClass ) &&
					 !mappinEntity->IsA( wineContractClass ) &&
					 !mappinEntity->IsA( signalingStakeClass )
					 )
				{
					continue;
				}
				// continue if it's fast travel but hub map
				CR4FastTravelEntity* fastTravelEntity = Cast< CR4FastTravelEntity >( mappinEntity );
				if ( fastTravelEntity && fastTravelEntity->IsHubExit() )
				{
					continue;
				}

				const CName& entityName = mappinEntity->GetEntityName();

				const String missingNameStr = TXT( "MISSING_NAME_");
				size_t dummy;
				if ( entityName.Empty() || entityName.AsString().FindSubstring( missingNameStr, dummy ) )
				{
					String entityPath = mappinEntity->GetLayer()->GetDepotPath();
					CClass* entityClass = mappinEntity->GetClass();

					size_t index;
					if ( entityPath.FindSubstring( TXT(".w2l"), index, true ) )
					{
						entityPath = entityPath.LeftString( index );
					}
					entityPath += TXT( "\\" );
					entityPath += mappinEntity->GetDisplayName();

					String errorInfo = String::Printf( TXT( "ERROR: empty or temporary entityName in entity [%s] [%s]\n" ), entityClass->GetName().AsChar(), entityPath.AsChar() );
					errorText += errorInfo;

					// either continue
					failed++;
					continue;

					/*
					// or add temporary name (but do not add it to tags)
					if ( entityName.Empty() )
					{
						Char buf[ RED_GUID_STRING_BUFFER_SIZE ];
						mappinEntity->GetGUID().ToString( buf, RED_GUID_STRING_BUFFER_SIZE );
						String name = missingNameStr + buf;
						mappinEntity->SetEntityName( CName( name ) );

						layer->MarkModified();
						m_layersToSave.PushBackUnique( layer );
					}
					*/
				}
				else
				{
					if ( !mappinEntity->HasTag( entityName ) )
					{
						TagList newTagList;
						newTagList.AddTag( entityName );
						newTagList.AddTags( mappinEntity->GetTags() );
						mappinEntity->SetTags( newTagList );

						layer->MarkModified();
						m_layersToSave.PushBackUnique( layer );
					}
				}

				SEntityMapPinInfo info;

				// common
				info.m_entityName			= mappinEntity->GetEntityName();
				info.m_entityCustomNameId	= mappinEntity->GetCustomNameId();
				info.m_entityTags			= mappinEntity->GetTags();
				info.m_entityPosition		= mappinEntity->GetPosition();
				info.m_entityRadius			= mappinEntity->GetRadius();
				// not applicable in most cases
				info.m_fastTravelTeleportWayPointPosition	= Vector::ZEROS;
				info.m_fastTravelTeleportWayPointRotation	= EulerAngles::ZEROS;

				// fast travel
				if ( fastTravelClass && mappinEntity->IsA( fastTravelClass ) )
				{
					CR4FastTravelEntity* entity = Cast< CR4FastTravelEntity > ( mappinEntity );
					if ( entity )
					{
						if ( entity->IsHubExit() )
						{
							info.m_entityType						= CNAME( HubExit );
						}
						else if ( entity->CanBeReachedByBoat() )
						{
							info.m_entityType						= CNAME( Harbor );
						}
						else
						{
							info.m_entityType						= CNAME( RoadSign );
						}

						// fast travel specific
						info.m_fastTravelSpotName					= entity->GetSpotName();
						info.m_fastTravelGroupName					= entity->GetGroupName();
						info.m_fastTravelTeleportWayPointTag		= entity->GetTeleportWayPointTag();

						CEntity* wayPointEntity = tagManager->GetTaggedEntity( info.m_fastTravelTeleportWayPointTag );
						if ( wayPointEntity )
						{
							info.m_fastTravelTeleportWayPointPosition = wayPointEntity->GetPosition();
							info.m_fastTravelTeleportWayPointRotation = wayPointEntity->GetRotation();
							if ( entityMappinsResource->AddEntry( info ) )
							{
								succeeded++;
							}
						}
						else
						{
							failed++;
							String entityPath = entity->GetLayer()->GetDepotPath();

							size_t index;
							if ( entityPath.FindSubstring( TXT(".w2l"), index, true ) )
							{
								entityPath = entityPath.LeftString( index );
							}
							entityPath += TXT( "\\" );
							entityPath += entity->GetDisplayName();

							String errorInfo = String::Printf( TXT( "ERROR: no waypoint found with tag \"%s\" specified in fast travel entity \"%s\"\n\n" ), info.m_fastTravelTeleportWayPointTag.AsChar(), entityPath.AsChar() );
							errorText += errorInfo;
						}
					}
				}
				else if ( noticeBoardClass && mappinEntity->IsA( noticeBoardClass ) )
				{
					info.m_entityType = CNAME( NoticeBoard );

					if ( entityMappinsResource->AddEntry( info ) )
					{
						succeeded++;
					}
				}
				else if ( stashClass && mappinEntity->IsA( stashClass ) )
				{
					Bool forceDiscoverable = false;
					CallFunctionRet( mappinEntity, CNAME( IsForcedToBeDiscoverable ), forceDiscoverable );
					if ( forceDiscoverable )
					{
						info.m_entityType = CNAME( PlayerStashDiscoverable );
					}
					else
					{
						info.m_entityType = CNAME( PlayerStash );
					}

					if ( entityMappinsResource->AddEntry( info ) )
					{
						succeeded++;
					}
				}
				else if ( monsterNestClass && mappinEntity->IsA( monsterNestClass ) )
				{
					Int32 entityType = 0;
					CallFunctionRet( mappinEntity, CNAME( GetRegionType ), info.m_alternateVersion );
					CallFunctionRet( mappinEntity, CNAME( GetEntityType ), entityType );

					if ( entityType == 0 )
					{
						info.m_entityType = CNAME( MonsterNest );
					}
					else
					{
						info.m_entityType = CNAME( InfestedVineyard );
					}

					if ( entityMappinsResource->AddEntry( info ) )
					{
						succeeded++;
					}
				}
				else if ( majorPlaceOfPowerClass && mappinEntity->IsA( majorPlaceOfPowerClass ) )
				{
					info.m_entityType = CNAME( PlaceOfPower );

					if ( entityMappinsResource->AddEntry( info ) )
					{
						succeeded++;
					}
				}
				else if ( magicLampClass && mappinEntity->IsA( magicLampClass ) )
				{
					info.m_entityType = CNAME( MagicLamp );

					if ( entityMappinsResource->AddEntry( info ) )
					{
						succeeded++;
					}
				}
				else if ( treasureHuntMappinClass && mappinEntity->IsA( treasureHuntMappinClass ) )
				{
					info.m_entityType = CNAME( TreasureHuntMappin );
					CallFunctionRet( mappinEntity, CNAME( GetRegionType ), info.m_alternateVersion );

					if ( entityMappinsResource->AddEntry( info ) )
					{
						succeeded++;
					}
				}
				else if ( teleportClass && mappinEntity->IsA( teleportClass ) )
				{
					info.m_entityType = CNAME( Teleport );

					if ( entityMappinsResource->AddEntry( info ) )
					{
						succeeded++;
					}
				}
				else if ( riftClass && mappinEntity->IsA( riftClass ) )
				{
					info.m_entityType = CNAME( Rift );

					if ( entityMappinsResource->AddEntry( info ) )
					{
						succeeded++;
					}
				}
				else if ( whetstoneClass && mappinEntity->IsA( whetstoneClass ) )
				{
					Bool weaponRepair = false;
					if ( CallFunctionRet( mappinEntity, CNAME( IsWeaponRepairEntity ), weaponRepair ) && weaponRepair )
					{
						info.m_entityType = CNAME( Whetstone );
					}
					else
					{
						info.m_entityType = CNAME( ArmorRepairTable );
					}

					if ( entityMappinsResource->AddEntry( info ) )
					{
						succeeded++;
					}
				}
				else if ( mutagenDismantlingTableClass && mappinEntity->IsA( mutagenDismantlingTableClass ) )
				{
					// derives from alchemy table so needs to be earlier
					info.m_entityType = CNAME( MutagenDismantle );

					if ( entityMappinsResource->AddEntry( info ) )
					{
						succeeded++;
					}
				}
				else if ( alchemyTableClass && mappinEntity->IsA( alchemyTableClass ) )
				{
					info.m_entityType = CNAME( AlchemyTable );

					if ( entityMappinsResource->AddEntry( info ) )
					{
						succeeded++;
					}
				}
				else if ( stablesClass && mappinEntity->IsA( stablesClass ) )
				{
					info.m_entityType = CNAME( Stables );

					if ( entityMappinsResource->AddEntry( info ) )
					{
						succeeded++;
					}
				}
				else if ( bookshelfClass && mappinEntity->IsA( bookshelfClass ) )
				{
					info.m_entityType = CNAME( Bookshelf );

					if ( entityMappinsResource->AddEntry( info ) )
					{
						succeeded++;
					}
				}
				else if ( witcherBedClass && mappinEntity->IsA( witcherBedClass ) )
				{
					info.m_entityType = CNAME( Bed );

					if ( entityMappinsResource->AddEntry( info ) )
					{
						succeeded++;
					}
				}
				else if ( witcherHouseClass && mappinEntity->IsA( witcherHouseClass ) )
				{
					info.m_entityType = CNAME( WitcherHouse );

					if ( entityMappinsResource->AddEntry( info ) )
					{
						succeeded++;
					}
				}
				else if ( entranceClass && mappinEntity->IsA( entranceClass ) )
				{
					info.m_entityType = CNAME( Entrance );

					if ( entityMappinsResource->AddEntry( info ) )
					{
						succeeded++;
					}
				}
				else if ( spoilsOfWarClass && mappinEntity->IsA( spoilsOfWarClass ) )
				{
					info.m_entityType = CNAME( SpoilsOfWar );

					if ( entityMappinsResource->AddEntry( info ) )
					{
						succeeded++;
					}
				}
				else if ( banditCampClass && mappinEntity->IsA( banditCampClass ) )
				{
					info.m_entityType = CNAME( BanditCamp );
					CallFunctionRet( mappinEntity, CNAME( GetRegionType ), info.m_alternateVersion );

					if ( entityMappinsResource->AddEntry( info ) )
					{
						succeeded++;
					}
				}
				else if ( banditCampfireClass && mappinEntity->IsA( banditCampfireClass ) )
				{
					info.m_entityType = CNAME( BanditCampfire );

					if ( entityMappinsResource->AddEntry( info ) )
					{
						succeeded++;
					}
				}
				else if ( bossAndTreasureClass && mappinEntity->IsA( bossAndTreasureClass ) )
				{
					info.m_entityType = CNAME( BossAndTreasure );
					CallFunctionRet( mappinEntity, CNAME( GetRegionType ), info.m_alternateVersion );

					if ( entityMappinsResource->AddEntry( info ) )
					{
						succeeded++;
					}
				}
				else if ( contrabandClass && mappinEntity->IsA( contrabandClass ) )
				{
					info.m_entityType = CNAME( Contraband );

					if ( entityMappinsResource->AddEntry( info ) )
					{
						succeeded++;
					}
				}
				else if ( contrabandShipClass && mappinEntity->IsA( contrabandShipClass ) )
				{
					info.m_entityType = CNAME( ContrabandShip );

					if ( entityMappinsResource->AddEntry( info ) )
					{
						succeeded++;
					}
				}
				else if ( rescuingTownClass && mappinEntity->IsA( rescuingTownClass ) )
				{
					info.m_entityType = CNAME( RescuingTown );
					CallFunctionRet( mappinEntity, CNAME( GetRegionType ), info.m_alternateVersion );

					if ( entityMappinsResource->AddEntry( info ) )
					{
						succeeded++;
					}
				}
				else if ( dungeonCrawlClass && mappinEntity->IsA( dungeonCrawlClass ) )
				{
					info.m_entityType = CNAME( DungeonCrawl );

					if ( entityMappinsResource->AddEntry( info ) )
					{
						succeeded++;
					}
				}
				else if ( hideoutClass && mappinEntity->IsA( hideoutClass ) )
				{
					info.m_entityType = CNAME( Hideout );

					if ( entityMappinsResource->AddEntry( info ) )
					{
						succeeded++;
					}
				}
				else if ( plegmundClass && mappinEntity->IsA( plegmundClass ) )
				{
					info.m_entityType = CNAME( Plegmund );

					if ( entityMappinsResource->AddEntry( info ) )
					{
						succeeded++;
					}
				}
				else if ( knightErrantClass && mappinEntity->IsA( knightErrantClass ) )
				{
					info.m_entityType = CNAME( KnightErrant );

					if ( entityMappinsResource->AddEntry( info ) )
					{
						succeeded++;
					}
				}
				else if ( wineContractClass && mappinEntity->IsA( wineContractClass ) )
				{
					info.m_entityType = CNAME( WineContract );

					if ( entityMappinsResource->AddEntry( info ) )
					{
						succeeded++;
					}
				}
				else if ( signalingStakeClass && mappinEntity->IsA( signalingStakeClass ) )
				{
					info.m_entityType = CNAME( SignalingStake );

					if ( entityMappinsResource->AddEntry( info ) )
					{
						succeeded++;
					}
				}
			}
		}
	}

	if ( failed > 0 )
	{
		String text = String::Printf( TXT("The file \"%ls\" was NOT exported because of %d error(s). Please fix first!"), resourcePath.AsChar(), failed );
		GFeedback->ShowError( ( text + TXT( "\n\n" ) + errorText ).AsChar() );
		return;
	}

	if ( file )
	{
		if ( file->IsNotSynced() )
		{
			GFeedback->ShowMsg( TXT("Export"), TXT("%s is not synced, cannot save"), file->GetFileName().AsChar() );
			return;
		}
		if ( !file->IsCheckedOut() )
		{
			file->SilentCheckOut( true );
		}
		if ( file->IsLocal() || file->IsCheckedOut() )
		{
			entityMappinsResource->Save();
		}
		else
		{
			GFeedback->ShowMsg( TXT("Export"), TXT("Cannot save %s"), file->GetFileName().AsChar() );
			return;
		}
	}
	else
	{
		CDirectory* resourceDir = GDepot->FindPath( resourcePath.AsChar() );
		if ( !resourceDir )
		{
			resourceDir = GDepot->CreatePath( resourcePath.AsChar() );
		}
		if ( resourceDir )
		{
			String resourceFile;
			if ( GetEntityMappinsResourceFile( worldPath, resourceFile, expansionPackIndex ) )
			{
				entityMappinsResource->SaveAs( resourceDir, resourceFile );
			}
		}
		else
		{
			GFeedback->ShowMsg( TXT("Export"), TXT("Cannot save %s"), file->GetFileName().AsChar() );
			return;
		}
	}

	String text = String::Printf( TXT("%d out of %d mappins successfully exported to \"%s\""), succeeded, succeeded + failed, resourcePath.AsChar() );
	if ( errorText.Empty() )
	{
		GFeedback->ShowMsg( TXT("Export"), text.AsChar() );
	}
	else
	{
		GFeedback->ShowMsg( TXT("Export"), ( text + TXT( "\n\n" ) + errorText + TXT( "Please fix it!" ) ).AsChar() );
	}

	for ( Uint32 i = 0; i < m_layersToSave.Size(); ++i )
	{
		CDiskFile* file = m_layersToSave[ i ]->GetFile();
		if ( file )
		{
			if ( file->IsNotSynced() )
			{
				RED_LOG( RED_LOG_CHANNEL( MapPinEntityExport ), TXT("[%s] not synced, cannot save"), file->GetFileName().AsChar() );
			}
			else if ( !file->IsCheckedOut() )
			{
				file->SilentCheckOut( true );
			}

			if ( file->IsCheckedOut() )
			{
				if ( file->Save() )
				{
					RED_LOG( RED_LOG_CHANNEL( MapPinEntityExport ), TXT("[%s] saved"), file->GetFileName().AsChar() );
				}
				else
				{
					RED_LOG( RED_LOG_CHANNEL( MapPinEntityExport ), TXT("[%s] NOT saved"), file->GetFileName().AsChar() );
				}
			}
		}
	}
#endif //NO_EDITOR
}

void CCommonMapManager::ExportQuestMapPins( Uint32 expansionPackIndex /*= 0*/ )
{
#ifndef NO_EDITOR
	if ( GGame->IsActive() )
	{
		GFeedback->ShowMsg( TXT("Export"), TXT("Please quit the game first") );
		return;
	}

	CWorld* world = GGame->GetActiveWorld();
	if ( !world )
	{
		return;
	}

	CTagManager* tagManager = world->GetTagManager();
	if ( !tagManager )
	{
		return;
	}

	String worldPath = world->GetDepotPath();
	String resourcePath;

	if ( !GetQuestMappinsResourcePath( worldPath, resourcePath, expansionPackIndex ) )
	{
		return;
	}

	CQuestMapPinsResource* questMappinsResource = NULL;

	CDiskFile* file = GDepot->FindFile( resourcePath );
	if ( file )
	{
		if ( file->Load() )
		{
			questMappinsResource = Cast< CQuestMapPinsResource >( file->GetResource() );
		}
	}
	else
	{
		questMappinsResource = ::CreateObject< CQuestMapPinsResource >();
	}

	if ( !questMappinsResource )
	{
		return;
	}

	questMappinsResource->ClearMapPinsInfo();

	THashMap< CName, SQuestMapPinInfo > mapPins;
	CCommonMapManager::CollectMapPinsFromJournal( mapPins );

	RED_LOG( RED_LOG_CHANNEL( asdf ), TXT("%d"), mapPins.Size() );

	Uint32 succeeded = 0;
	for ( THashMap< CName, SQuestMapPinInfo >::iterator it = mapPins.Begin(); it != mapPins.End(); ++it )
	{
		const CName& tag = it->m_first;

		static TDynArray< CNode* > nodes;
		nodes.ClearFast();
		nodes.Reserve( 16 );
		tagManager->CollectTaggedNodes( tag, nodes );
		for ( Uint32 i = 0; i < nodes.Size(); ++i )
		{
			SQuestMapPinInfo info = it->m_second;
			info.m_positions.PushBackUnique( nodes[ i ]->GetWorldPositionRef() );
			questMappinsResource->AddEntry( info );

			RED_LOG( RED_LOG_CHANNEL( asdf ), TXT("[%s] [%s] [%d]"), info.m_tag.AsChar(), info.m_type.AsChar(), info.m_radius );

			succeeded++;
		}
	}
	RED_LOG( RED_LOG_CHANNEL( asdf ), TXT("[%d] total"), succeeded );

	if ( file )
	{
		if ( file->IsNotSynced() )
		{
			GFeedback->ShowMsg( TXT("Export"), TXT("%s is not synced, cannot save"), file->GetFileName().AsChar() );
			return;
		}
		if ( !file->IsCheckedOut() )
		{
			file->SilentCheckOut( true );
		}
		if ( file->IsLocal() || file->IsCheckedOut() )
		{
			questMappinsResource->Save();
		}
		else
		{
			GFeedback->ShowMsg( TXT("Export"), TXT("Cannot save %s"), file->GetFileName().AsChar() );
			return;
		}
	}
	else
	{
		CDirectory* resourceDir = GDepot->FindPath( resourcePath.AsChar() );
		ASSERT( resourceDir );
		if ( resourceDir )
		{
			String resourceFile;
			if ( GetQuestMappinsResourceFile( worldPath, resourceFile, expansionPackIndex ) )
			{
				questMappinsResource->SaveAs( resourceDir, resourceFile );
			}
		}
	}

	String text = String::Printf( TXT("%d mappins successfully exported to \"%s\""), succeeded, resourcePath.AsChar() );
	GFeedback->ShowMsg( TXT("Export"), text.AsChar() );
#endif //NO_EDITOR
}

RED_DEFINE_STATIC_NAME( InteractiveEntity );
RED_DEFINE_STATIC_NAME( minimapmeshrender );

void CCommonMapManager::UpdateMapPinEntities( Bool updateAndSave )
{
#ifndef NO_EDITOR
	if ( GGame->IsActive() )
	{
		GFeedback->ShowError( TXT("Quit the game before updating layers") );
		return;
	}
	CWorld* world = GGame->GetActiveWorld();
	if ( !world )
	{
		return;
	}
	CLayerGroup* worldLayers = world->GetWorldLayers();
	if ( !worldLayers )
	{
		return;
	}

	TDynArray< CLayerInfo* > layers;
	world->GetWorldLayers()->GetLayers( layers, true );

	CClass* fastTravelClass  = SRTTI::GetInstance().FindClass( CNAME( W3FastTravelEntity ) );
	ASSERT( fastTravelClass );
	CClass* noticeBoardClass = SRTTI::GetInstance().FindClass( CNAME( W3NoticeBoard ) );
	ASSERT( noticeBoardClass );
	CClass* monsterNestClass = SRTTI::GetInstance().FindClass( CNAME( CMonsterNestEntity ) );
	ASSERT( monsterNestClass );
	CClass* majorPlaceOfPowerClass = SRTTI::GetInstance().FindClass( CNAME( CMajorPlaceOfPowerEntity ) );
	ASSERT( majorPlaceOfPowerClass );
	CClass* teleportClass = SRTTI::GetInstance().FindClass( CNAME( CTeleportEntity ) );
	ASSERT( teleportClass );
	CClass* riftClass = SRTTI::GetInstance().FindClass( CNAME( CRiftEntity ) );
	ASSERT( riftClass );
	CClass* whetstoneClass = SRTTI::GetInstance().FindClass( CNAME( W3ItemRepairObject ) );
	ASSERT( whetstoneClass );

	Uint32 succeeded = 0;
	Uint32 failed = 0;
	String errorText;

	RED_LOG( RED_LOG_CHANNEL( LIVINGWORLD ), TXT("---------------------------------------------------") );

	TDynArray< CLayer* > m_layersToSave;

	for ( Uint32 k = 0; k < layers.Size(); k++ )
	{
		CLayerInfo* info = layers[k];
		if ( info->IsLoaded() )
		{
			CLayer* layer = info->GetLayer();

			TDynArray< CEntity* > entities;
			layer->GetEntities( entities );
			for ( Uint32 i = 0; i < entities.Size(); ++i )
			{
				CEntity* testEntity = entities[i];

				// fast travel
				if ( fastTravelClass && testEntity->IsA( fastTravelClass ) )
				{
					CR4FastTravelEntity* entity = Cast< CR4FastTravelEntity > ( testEntity );
					if ( entity )
					{
						CName entityName = entity->GetEntityName();

						if ( entityName != CName::NONE && !entity->HasTag( entityName ) )
						{
							if ( updateAndSave )
							{
								UpdateTags( entity, entityName );
								m_layersToSave.PushBackUnique( layer );
								RED_LOG( RED_LOG_CHANNEL( LIVINGWORLD ), TXT("tags updated [%s]"), fastTravelClass->GetName().AsChar() );
							}
							else
							{
								RED_LOG( RED_LOG_CHANNEL( LIVINGWORLD ), TXT("tags need updating [%s]"), fastTravelClass->GetName().AsChar() );
							}
						}
					}
				}
				else if ( noticeBoardClass && testEntity->IsA( noticeBoardClass ) )
				{
					CR4MapPinEntity* entity = Cast< CR4MapPinEntity > ( testEntity );
					if ( entity )
					{
						CName entityName = entity->GetEntityName();
						const TagList& tagList = entity->GetTags();
						CName firstTag;
						for ( Uint32 t = 0; t < tagList.GetTags().Size(); ++t )
						{
							CName tag = tagList.GetTag( t );
							if ( tag != CNAME( InteractiveEntity ) && tag != CNAME( minimapmeshrender ) )
							{
								firstTag = tag;
								break;
							}
						}

						if ( entityName != firstTag )
						{
							if ( updateAndSave )
							{
								entity->SetEntityName( firstTag );
								m_layersToSave.PushBackUnique( layer );
								RED_LOG( RED_LOG_CHANNEL( LIVINGWORLD ), TXT("updated [%s]:   [%s] -> [%s]"), noticeBoardClass->GetName().AsChar(), entityName.AsChar(), firstTag.AsChar() );
							}
							else
							{
								RED_LOG( RED_LOG_CHANNEL( LIVINGWORLD ), TXT("needs updating [%s]:   [%s] -> [%s]"), noticeBoardClass->GetName().AsChar(), entityName.AsChar(), firstTag.AsChar() );
							}
						}
					}
				}
				else if ( monsterNestClass && testEntity->IsA( monsterNestClass ) )
				{
					CR4MapPinEntity* entity = Cast< CR4MapPinEntity > ( testEntity );
					if ( entity )
					{
						CName entityName = entity->GetEntityName();

						if ( entityName != CName::NONE && !entity->HasTag( entityName ) )
						{
							if ( updateAndSave )
							{
								UpdateTags( entity, entityName );
								m_layersToSave.PushBackUnique( layer );
								RED_LOG( RED_LOG_CHANNEL( LIVINGWORLD ), TXT("tags updated [%s]"), monsterNestClass->GetName().AsChar() );
							}
							else
							{
								RED_LOG( RED_LOG_CHANNEL( LIVINGWORLD ), TXT("tags need updating [%s]"), monsterNestClass->GetName().AsChar() );
							}
						}
					}
				}
				else if ( majorPlaceOfPowerClass && testEntity->IsA( majorPlaceOfPowerClass ) )
				{
					CR4MapPinEntity* entity = Cast< CR4MapPinEntity > ( testEntity );
					if ( entity )
					{
						CName entityName = entity->GetEntityName();

						const TagList& tagList = entity->GetTags();
						CName firstTag;
						for ( Uint32 t = 0; t < tagList.GetTags().Size(); ++t )
						{
							CName tag = tagList.GetTag( t );
							if ( tag != CNAME( InteractiveEntity ) && tag != CNAME( minimapmeshrender ) )
							{
								firstTag = tag;
								break;
							}
						}

						if ( entityName != firstTag && firstTag != CNAME( InteractiveEntity ) )
						{
							if ( updateAndSave )
							{
								entity->SetEntityName( firstTag );
								m_layersToSave.PushBackUnique( layer );
								RED_LOG( RED_LOG_CHANNEL( LIVINGWORLD ), TXT("entityName updated [%s]:   [%s] -> [%s]"), majorPlaceOfPowerClass->GetName().AsChar(), entityName.AsChar(), firstTag.AsChar() );
							}
							else
							{
								RED_LOG( RED_LOG_CHANNEL( LIVINGWORLD ), TXT("entityName need updating [%s]:   [%s] -> [%s]"), majorPlaceOfPowerClass->GetName().AsChar(), entityName.AsChar(), firstTag.AsChar() );
							}
						}
					}
				}
				else if ( teleportClass && testEntity->IsA( teleportClass ) )
				{
					CR4MapPinEntity* entity = Cast< CR4MapPinEntity > ( testEntity );
					if ( entity )
					{
						CName entityName = entity->GetEntityName();
						const TagList& tagList = entity->GetTags();
						CName firstTag;
						for ( Uint32 t = 0; t < tagList.GetTags().Size(); ++t )
						{
							CName tag = tagList.GetTag( t );
							if ( tag != CNAME( InteractiveEntity ) && tag != CNAME( minimapmeshrender ) )
							{
								firstTag = tag;
								break;
							}
						}

						if ( entityName != firstTag )
						{
							if ( updateAndSave )
							{
								entity->SetEntityName( firstTag );
								m_layersToSave.PushBackUnique( layer );
								RED_LOG( RED_LOG_CHANNEL( LIVINGWORLD ), TXT("updated [%s]:   [%s] -> [%s]"), teleportClass->GetName().AsChar(), entityName.AsChar(), firstTag.AsChar() );
							}
							else
							{
								RED_LOG( RED_LOG_CHANNEL( LIVINGWORLD ), TXT("needs updating [%s]:   [%s] -> [%s]"), teleportClass->GetName().AsChar(), entityName.AsChar(), firstTag.AsChar() );
							}
						}
					}
				}
				else if ( riftClass && testEntity->IsA( riftClass ) )
				{
					CR4MapPinEntity* entity = Cast< CR4MapPinEntity > ( testEntity );
					if ( entity )
					{
						CName entityName = entity->GetEntityName();
						const TagList& tagList = entity->GetTags();
						CName firstTag;
						for ( Uint32 t = 0; t < tagList.GetTags().Size(); ++t )
						{
							CName tag = tagList.GetTag( t );
							if ( tag != CNAME( InteractiveEntity ) && tag != CNAME( minimapmeshrender ) )
							{
								firstTag = tag;
								break;
							}
						}

						if ( entityName != firstTag )
						{
							if ( updateAndSave )
							{
								entity->SetEntityName( firstTag );
								m_layersToSave.PushBackUnique( layer );
								RED_LOG( RED_LOG_CHANNEL( LIVINGWORLD ), TXT("updated [%s]:   [%s] -> [%s]"), riftClass->GetName().AsChar(), entityName.AsChar(), firstTag.AsChar() );
							}
							else
							{
								RED_LOG( RED_LOG_CHANNEL( LIVINGWORLD ), TXT("needs updating [%s]:   [%s] -> [%s]"), riftClass->GetName().AsChar(), entityName.AsChar(), firstTag.AsChar() );
							}
						}
					}
				}
				else if ( whetstoneClass && testEntity->IsA( whetstoneClass ) )
				{
					CR4MapPinEntity* entity = Cast< CR4MapPinEntity > ( testEntity );
					if ( entity )
					{
						CName entityName = entity->GetEntityName();
						const TagList& tagList = entity->GetTags();
						CName firstTag;
						for ( Uint32 t = 0; t < tagList.GetTags().Size(); ++t )
						{
							CName tag = tagList.GetTag( t );
							if ( tag != CNAME( InteractiveEntity ) && tag != CNAME( minimapmeshrender ) )
							{
								firstTag = tag;
								break;
							}
						}

						if ( entityName != firstTag )
						{
							if ( updateAndSave )
							{
								entity->SetEntityName( firstTag );
								m_layersToSave.PushBackUnique( layer );
								RED_LOG( RED_LOG_CHANNEL( LIVINGWORLD ), TXT("updated [%s]:   [%s] -> [%s]"), whetstoneClass->GetName().AsChar(), entityName.AsChar(), firstTag.AsChar() );
							}
							else
							{
								RED_LOG( RED_LOG_CHANNEL( LIVINGWORLD ), TXT("needs updating [%s]:   [%s] -> [%s]"), whetstoneClass->GetName().AsChar(), entityName.AsChar(), firstTag.AsChar() );
							}
						}
					}
				}
			}
		}
	}

	RED_LOG( RED_LOG_CHANNEL( LIVINGWORLD ), TXT("%d layers to save"), m_layersToSave.Size() );
	for ( Uint32 i = 0; i < m_layersToSave.Size(); ++i )
	{
		CDiskFile* file = m_layersToSave[ i ]->GetFile();
		if ( file )
		{
			if ( file->IsNotSynced() )
			{
				RED_LOG( RED_LOG_CHANNEL( LIVINGWORLD ), TXT("[%s] not synced, cannot save"), file->GetFileName().AsChar() );
			}
			else if ( !file->IsCheckedOut() )
			{
				file->SilentCheckOut( true );
			}

			if ( file->IsCheckedOut() )
			{
				if ( file->Save() )
				{
					RED_LOG( RED_LOG_CHANNEL( LIVINGWORLD ), TXT("[%s] saved"), file->GetFileName().AsChar() );
				}
				else
				{
					RED_LOG( RED_LOG_CHANNEL( LIVINGWORLD ), TXT("[%s] NOT saved"), file->GetFileName().AsChar() );
				}
			}
		}
	}
#endif //NO_EDITOR
}

void CCommonMapManager::SetHighlightableMapPinsFromObjective( const CJournalQuestObjective* highlightedObjective )
{
	if ( highlightedObjective )
	{
		SetAllMapPinsUnhighlightable();
		for ( Uint32 i = 0; i < highlightedObjective->GetNumChildren(); i += 1 )
		{
			const CJournalQuestMapPin* highlightedMapPin = Cast< CJournalQuestMapPin >( highlightedObjective->GetChild( i ) );
			if ( highlightedMapPin )
			{
				SetMapPinHighlightable( highlightedMapPin->GetMapPinID() );
			}
		}
	}
}

void CCommonMapManager::ForceUpdateDynamicMapPins()
{
	m_invalidatedQuestMapPinData = true;
	UpdateQuestMapPinData();
	// pass -1 to finish current update (if there's any) and then update all mappins immediately
	UpdateDynamicMapPins( -1 );
	UpdateHighlightableMapPins();
}

void CCommonMapManager::UpdateTags( CEntity* entity, const CName& tag )
{
#ifndef NO_EDITOR
	if ( !entity->HasTag( tag ) )
	{
		TagList tagList;
		tagList.AddTag( tag );
		tagList.AddTags( entity->GetTags() );
		entity->SetTags( tagList );
	}
#endif //NO_EDITOR
}

const CName& CCommonMapManager::GetQuestNameFromType( EJournalMapPinType mapPinType, eQuestType questType )
{
	switch ( mapPinType )
	{
	case EJMPT_Default:
		if ( questType >= QuestType_Story && questType < QuestType_Max )
		{
			return m_questName[ questType ];
		}
		return CName::NONE;
	case EJMPT_QuestReturn:
		return CNAME( QuestReturn );
	case EJMPT_HorseRace:
		return CNAME( HorseRace );
	case EJMPT_BoatRace:
		return CNAME( BoatRace );
	case EJMPT_QuestBelgard:
		return CNAME( QuestBelgard );
	case EJMPT_QuestCoronata:
		return CNAME( QuestCoronata );
	case EJMPT_QuestVermentino:
		return CNAME( QuestVermentino );
	}
	return CName::NONE;
}

Float CCommonMapManager::GetVisibleRadius( const CName& type, Float radius )
{
	if ( radius == 0 )
	{
		return 0;
	}

	if (	type == CNAME( NotDiscoveredPOI ) ||
			type == CNAME( TreasureHuntMappinDisabled ) ||
			type == CNAME( SpoilsOfWarDisabled ) ||
			type == CNAME( BanditCampDisabled ) ||
			type == CNAME( BanditCampfireDisabled ) ||
			type == CNAME( BossAndTreasureDisabled ) ||
			type == CNAME( ContrabandDisabled ) ||
			type == CNAME( ContrabandShipDisabled ) ||
			type == CNAME( RescuingTownDisabled ) ||
			type == CNAME( DungeonCrawlDisabled ) )
	{
		return 0;
	}
	return radius;
}

Bool CCommonMapManager::CanPinBePointedByArrow( const CName& pinType )
{
	if ( IsQuestPinType( pinType ) )
	{
		return true;
	}
	if ( pinType == m_userPinName[ WAYPOINT_USER_MAP_PIN_TYPE ] )
	{
		return true;
	}
	return false;
}

Bool CCommonMapManager::CanPinBeAddedToMinimap( const CName& pinType )
{
	return pinType != CNAME( Harbor ) && pinType != CNAME( WitcherHouse );
}

void CCommonMapManager::OnAttachedBoat( const CGameplayEntity* boatEntity )
{
	m_boatEntities.PushBack( THandle< CGameplayEntity >( boatEntity ) );
}

void CCommonMapManager::OnDetachedBoat( const CGameplayEntity* boatEntity )
{
	m_boatEntities.Remove( THandle< CGameplayEntity >( boatEntity ) );
}

void CCommonMapManager::OnAttachedInterior( CR4InteriorAreaComponent* interiorComponent )
{
	m_interiorsRegistry.Add( interiorComponent );
}

void CCommonMapManager::OnDetachedInterior( CR4InteriorAreaComponent* interiorComponent )
{
	m_interiorsRegistry.Remove( interiorComponent );
}

void CCommonMapManager::OnPlayerEnteredInterior( const CR4InteriorAreaComponent* interiorComponent )
{
	if ( interiorComponent )
	{
		m_playerInteriors.PushBackUnique( THandle< CR4InteriorAreaComponent >( interiorComponent ) );
		ScheduleUpdateDynamicMapPins();

		OnNotifyPlayerEnteredInterior( interiorComponent );
	}
}

void CCommonMapManager::OnPlayerExitedInterior( const CR4InteriorAreaComponent* interiorComponent )
{
	const CR4InteriorAreaComponent* prevComponent = nullptr;
	const CR4InteriorAreaComponent* currComponent = nullptr;
	if ( !m_playerInteriors.Empty() )
	{
		prevComponent = m_playerInteriors[ m_playerInteriors.Size() - 1 ].Get();
	}
	m_playerInteriors.Remove( THandle< CR4InteriorAreaComponent >( interiorComponent ) );

	if ( !m_playerInteriors.Empty() )
	{
		currComponent = m_playerInteriors[ m_playerInteriors.Size() - 1 ].Get();
	}

	if ( !currComponent )
	{
		OnNotifyPlayerExitedInterior();
	}
	else if ( prevComponent != currComponent )
	{
		OnNotifyPlayerEnteredInterior( currComponent );
	}
}

void CCommonMapManager::OnPlayerChanged()
{
	m_playerInteriors.ClearFast();
	OnNotifyPlayerExitedInterior();
}

void CCommonMapManager::CreateMinimapManager()
{
	DeleteMinimapManager();

	m_minimapManager = new CMinimapManager;
}

void CCommonMapManager::DeleteMinimapManager()
{
	if ( m_minimapManager )
	{
		delete m_minimapManager;
		m_minimapManager = nullptr;
	}
}

void CCommonMapManager::OnNotifyPlayerEnteredInterior( const CR4InteriorAreaComponent* interiorComponent )
{
	Vector position;
	Float yaw;
	String texture;

	position = interiorComponent->GetWorldPosition();
	yaw = interiorComponent->GetWorldRotation().Yaw;
	texture = interiorComponent->GetTexture();

	CObject* parent = interiorComponent->GetParent();
	if ( parent )
	{
		// if this component belongs to interior entity, use entity texture instead of component one
		const CR4InteriorAreaEntity* interiorEntity = Cast< CR4InteriorAreaEntity >( parent );
		if ( interiorEntity )
		{
			position = interiorEntity->GetWorldPosition();
			yaw = 0;
			texture = interiorEntity->GetTexture();
		}
	}

	CallFunction( this, CNAME( NotifyPlayerEnteredInterior ), position, yaw, texture );
	ScheduleUpdateDynamicMapPins();
}

void CCommonMapManager::OnNotifyPlayerExitedInterior()
{
	CallFunction( this, CNAME( NotifyPlayerExitedInterior ) );
	ScheduleUpdateDynamicMapPins();
}

void CCommonMapManager::AddMapPinToMinimap( SCommonMapPinInstance& pin )
{
	RED_ASSERT( !pin.m_isAddedToMinimap );

	if ( m_minimapManager )
	{
		m_minimapManager->AddMapPin( pin );
	}
	pin.m_isAddedToMinimap = true;

#ifndef NO_SECOND_SCREEN
	pin.AddToSecondScreen();
#endif //! NO_SECOND_SCREEN
}

void CCommonMapManager::MoveMapPinOnMinimap( SCommonMapPinInstance& pin )
{
	if ( m_minimapManager )
	{
		m_minimapManager->MoveMapPin( pin );
	}

#ifndef NO_SECOND_SCREEN
	pin.MoveOnSecondScreen();
#endif //! NO_SECOND_SCREEN
}

void CCommonMapManager::HighlightMapPinOnMinimap( SCommonMapPinInstance& pin )
{
	if ( m_minimapManager )
	{
		m_minimapManager->HighlightMapPin( pin );
	}
}

void CCommonMapManager::UpdateMapPinOnMinimap( SCommonMapPinInstance& pin )
{
	if ( m_minimapManager )
	{
		m_minimapManager->UpdateMapPin( pin );
	}
}

void CCommonMapManager::DeleteMapPinOnMinimap( SCommonMapPinInstance& pin )
{
	RED_ASSERT( pin.m_isAddedToMinimap );

	if ( m_minimapManager )
	{
		m_minimapManager->DeleteMapPin( pin );
	}
	pin.m_isAddedToMinimap = false;

#ifndef NO_SECOND_SCREEN
	SCSecondScreenManager::GetInstance().SendRemoveActualAreaDynamicMapPin( pin.m_id );
#endif
}

void CCommonMapManager::UpdateDistanceToHighlightedMapPinOnMinimap( Float questDistance, Float userDistance )
{
	if ( m_minimapManager )
	{
		m_minimapManager->UpdateDistanceToHighlightedMapPin( questDistance, userDistance );
	}
}

void CCommonMapManager::AddWaypointsToMinimap( const TDynArray< Vector >& waypoints )
{
	if ( m_minimapManager )
	{
		m_minimapManager->AddWaypoints( waypoints );
	}
}

void CCommonMapManager::DeleteWaypointsOnMinimap()
{
	if ( m_minimapManager )
	{
		m_minimapManager->DeleteWaypoints();
	}
}


CCachedWorldData* CCommonMapManager::GetWorldCachedData( const String& path )
{
	if ( path.Empty() )
	{
		return nullptr;
	}

	CCachedWorldData & cachedData = m_cachedWorldDataMap[ path ];
	if( !cachedData.IsInitialized() )
	{
		cachedData.Initialize( path );
	}

	return &cachedData;
}

CCachedWorldData* CCommonMapManager::GetCurrentWorldCachedData()
{
	return GetWorldCachedData( m_currentWorldPath );
}

void CCommonMapManager::EnableShopkeeper( const CName& tag, Bool enable )
{
	CCachedWorldData* cachedData = GetCurrentWorldCachedData();
	if ( !cachedData )
	{
		return;
	}

	cachedData->EnableShopkeeper( tag, enable );
}

Uint32 CCommonMapManager::GetUserMapPinCount( Bool onlyWaypoints )
{
	Uint32 count = 0;
	for ( Uint32 i = 0; i < m_userMapPins.Size(); ++i )
	{
		if ( m_userMapPins[ i ].m_type == WAYPOINT_USER_MAP_PIN_TYPE )
		{
			if ( onlyWaypoints )
			{
				++count;
			}
		}
		else
		{
			if ( !onlyWaypoints )
			{
				++count;
			}
		}
	}
	return count;
}

void CCommonMapManager::UpdateQuestMapPinData()
{
	PC_SCOPE_PIX( UpdateQuestMapPinData );

	if ( !m_invalidatedQuestMapPinData )
	{
		return;
	}
	m_invalidatedQuestMapPinData = false;

	m_questMapPinData.ClearFast();

	CWitcherJournalManager* manager = GCommonGame->GetSystem< CWitcherJournalManager >();
	ASSERT( manager != nullptr );

	const CJournalQuest* trackedQuest = manager->GetTrackedQuest();
	if ( trackedQuest )
	{
		TDynArray< const CJournalQuestObjective* > activeObjectives;
		manager->GetTrackedQuestObjectives( activeObjectives );

		for ( Uint32 i = 0; i < activeObjectives.Size(); ++i )
		{
			const CJournalQuestObjective* questObjective = activeObjectives[ i ];
			if ( manager->GetEntryStatus( questObjective ) != JS_Active )
			{
				continue;
			}

			Uint32 childCount = questObjective->GetNumChildren();
			for ( Uint32 j = 0; j < childCount; ++j )
			{
				const CJournalContainerEntry* entry = questObjective->GetChild( j );
				if ( entry && entry->IsA< CJournalQuestMapPin >() )
				{
					const CJournalQuestMapPin* questMapPin = Cast< CJournalQuestMapPin >( entry );

					if ( IsQuestMapPinEnabled( questObjective, questMapPin ) )
					{
						Uint32 journalArea = questObjective->GetWorld();
						Uint32 realArea = ConvertJournalAreaToRealArea( journalArea );
						m_questMapPinData.PushBackUnique( SQuestPinData( questMapPin->GetName(),
																         questMapPin->GetMapPinID(),
							                                             questMapPin->GetRadius(),
																		 questMapPin->GetType(),
																		 trackedQuest->GetType(),
																		 questObjective->GetGUID(),
																		 questMapPin->GetGUID(),
																		 realArea ) );
					}
				}
			}
		}
	}
}

void CCommonMapManager::CacheActiveQuestMapPins( THashMap< CName, SQuestMapPinInfo >& cachedQuestMapPins )
{
	TDynArray< Vector > positions;

	cachedQuestMapPins.ClearFast();

	CWitcherJournalManager* manager = GCommonGame->GetSystem< CWitcherJournalManager >();
	if ( !manager )
	{
		return;
	}

	TDynArray< const CJournalQuestObjective* > activeObjectives;
	manager->GetActiveQuestObjectives( activeObjectives );
	
	for ( Uint32 i = 0; i < activeObjectives.Size(); ++i )
	{
		const CJournalQuestObjective* questObjective = activeObjectives[ i ];
		const CJournalQuest* quest = questObjective->GetParentQuest();
		if ( !quest )
		{
			continue;
		}
		if ( manager->GetEntryStatus( questObjective ) != JS_Active )
		{
			continue;
		}

		Uint32 childCount = questObjective->GetNumChildren();
		for ( Uint32 j = 0; j < childCount; ++j )
		{
			const CJournalContainerEntry* entry = questObjective->GetChild( j );
			if ( entry && entry->IsA< CJournalQuestMapPin >() )
			{
				const CJournalQuestMapPin* questMapPin = Cast< CJournalQuestMapPin >( entry );

				if ( IsQuestMapPinEnabled( questObjective, questMapPin ) )
				{
					Uint32 journalArea = questObjective->GetWorld();
					Uint32 realArea = ConvertJournalAreaToRealArea( journalArea );
					if ( realArea == m_currentArea )
					{
						const CName& tag = questMapPin->GetMapPinID();

						positions.ClearFast();
						GetQuestMapPinPositions( tag, positions );
						if ( positions.Size() )
						{
							SQuestMapPinInfo* info = cachedQuestMapPins.FindPtr( tag );
							if ( info )
							{
								info->m_positions.PushBackUnique( positions );
							}
							else
							{
								SQuestMapPinInfo info;
								info.m_tag	= questMapPin->GetMapPinID();
								info.m_type	= CCommonMapManager::GetQuestNameFromType( questMapPin->GetType(), quest->GetType() );
								info.m_radius = questMapPin->GetRadius();
								info.m_positions.PushBackUnique( positions );

								cachedQuestMapPins[ tag ] = info;
							}
						}
					}
				}
			}
		}
	}
}

void CCommonMapManager::CacheNoticeboardMapPins( THashSet< CName >& cachedNoticeboardMapPins )
{
	cachedNoticeboardMapPins.ClearFast();

	THashMap< Int32, SCommonMapPinInstance >::iterator itEnd = m_mapPins.End();
	for ( THashMap< Int32, SCommonMapPinInstance >::iterator itPin = m_mapPins.Begin(); itPin != itEnd; ++itPin )
	{ 
		SCommonMapPinInstance& pin = itPin->m_second;
		if ( pin.m_visibleType == CNAME( NoticeBoardFull ) )
		{
			cachedNoticeboardMapPins.Insert( pin.m_tag );
		}
	}
}

void CCommonMapManager::CacheBoatMapPins( TDynArray< Vector >& cachedBoatMapPins )
{
	cachedBoatMapPins.ClearFast();

	for ( Uint32 i = 0; i < m_boatEntities.Size(); ++i )
	{ 
		const CGameplayEntity* boat = m_boatEntities[ i ];
		if ( boat )
		{
			cachedBoatMapPins.PushBack( boat->GetWorldPositionRef() );
		}
	}
}

void CCommonMapManager::UpdateDynamicMapPins( Float timeDelta )
{
	PC_SCOPE_PIX( UpdateDynamicMapPins );

	if ( timeDelta < 0 )
	{
		if ( m_dynamicPinsUpdateStep == DPUS_Processing )
		{
			// we're in the middle of update, finish previous update first!
			UpdateDynamicMapPins_Processing( true );
			UpdateDynamicMapPins_Finalization();
		}

		// do proper update immediately
		// it's needed for updating pins when game is paused (i.e. navigating among world map and journal panels)
		UpdateDynamicMapPins_Initialization();
		UpdateDynamicMapPins_Processing( true );
		UpdateDynamicMapPins_Finalization();
		return;
	}

	m_dynamicPinsUpdateInterval -= timeDelta;

	switch ( m_dynamicPinsUpdateStep )
	{
	case DPUS_None:
		if ( m_dynamicPinsUpdateInterval <= 0.0f )
		{
			m_dynamicPinsUpdateInterval = DYNAMIC_PINS_UPDATE_INTERVAL;

			UpdateDynamicMapPins_Initialization();
			m_dynamicPinsUpdateStep = DPUS_Processing;
		}
		break;

	case DPUS_Processing:
		if ( UpdateDynamicMapPins_Processing() )
		{
			UpdateDynamicMapPins_Finalization();
			m_dynamicPinsUpdateStep = DPUS_None;
		}
		break;
	}
}

void CCommonMapManager::UpdateDynamicMapPins_Initialization()
{
	//
	//RED_LOG( RED_LOG_CHANNEL( PROCESSING ), TXT("UpdateDynamicMapPins_Initialization") );
	//

	PC_SCOPE_PIX( UpdateDynamicMapPins_Initialization );

	m_currDynamicMapPinIds.ClearFast();

	CActor* player = GCommonGame->GetPlayer();
	if ( !player )
	{
		return;
	}

	for ( Uint32 i = 0; i < m_questMapPinData.Size(); i++ )
	{
		const SQuestPinData& mapPinData = m_questMapPinData[ i ];
		if ( mapPinData.m_world == 0 || mapPinData.m_world == ConvertFakeAreaToRealArea( m_currentArea ) )
		{
			if ( !PlaceQuestMapPinsFromEntities( mapPinData ) )
			{
				// place map pins from communities only if there are no map pins from entities with the same tag
				PlaceQuestMapPinsFromCommunities( mapPinData );
			}
		}
		else
		{
			// place map pin on closest fast travel point
			PlaceQuestMapPinFromFastTravelPoint( mapPinData );
		}
	}

	for ( THashMap< CName, CName >::iterator it = m_customAgentMapPins.Begin(); it != m_customAgentMapPins.End(); ++it )
	{
		if ( m_discoveredAgentEntityTags.Exist( (*it).m_first ) )
		{
			PlacePossibleQuestMapPinsFromCommunities( (*it).m_first, (*it).m_second );
		}
	}

	THandle< CNewNPC > companion;
	CallFunctionRet( GCommonGame->GetPlayer(), CNAME( GetCompanion ), companion );

	THandle< CGameplayEntity > usedEntity;
	CallFunctionRet( GCommonGame->GetPlayer(), CNAME( GetUsedVehicle ), usedEntity );

	for ( Uint32 i = 0; i < m_customMapPinDefinition.Size(); i++ )
	{
		PlaceCustomMapPinsFromEntities( m_customMapPinDefinition[ i ], usedEntity.Get() );
	}

	PlaceBoatMapPins( usedEntity.Get() );
	PlaceShopkeeperMapPins();
	PlacePlayerCompanionMapPin( companion.Get() );
	PlacePlayerHorseMapPin( usedEntity.Get() );
	PlaceUserMapPins();

	m_nearbyEntitiesWrappers.ClearFast();
	Box nearbyAreaBoundingBox = Box( Vector::ZERO_3D_POINT, ENEMY_SEARCH_RADIUS );
	GCommonGame->GetGameplayStorage()->GetClosestToEntity( *player, m_nearbyEntitiesWrappers, nearbyAreaBoundingBox, MINIMAP_PINS_COUNT, NULL, 0 );

	if ( m_nearbyEntitiesWrappers.Size() == MINIMAP_PINS_COUNT )
	{
		RED_LOG( RED_LOG_CHANNEL( CommonMapManager ), TXT("CCommonMapManager::PlaceNearbyDynamicMapPins - maximum number of entities found (%d), consider setting higher limit..."), MINIMAP_PINS_COUNT );
	}

	// entities need to be kept in handles, because they are processed in consecutive ticks
	m_nearbyEntitiesHandles.ClearFast();
	for ( Uint32 i = 0; i < m_nearbyEntitiesWrappers.Size(); ++i )
	{
		m_nearbyEntitiesHandles.PushBack( THandle< CGameplayEntity >( m_nearbyEntitiesWrappers[ i ].Get() ) );
	}
	m_nearbyEntitiesWrappers.ClearFast();
}

Bool CCommonMapManager::UpdateDynamicMapPins_Processing( Bool processAll /*= false*/ )
{
	//
	//RED_LOG( RED_LOG_CHANNEL( PROCESSING ), TXT("UpdateDynamicMapPins_Processing( %d )"), processAll );
	//
	PC_SCOPE_PIX( UpdateDynamicMapPins_Processing );

	CActor* player = GCommonGame->GetPlayer();
	if ( !player )
	{
		// return true if there is something wrong
		return true;
	}
	CFactsDB* factsManager = GCommonGame->GetSystem< CFactsDB >();
	if ( !factsManager )
	{
		// return true if there is something wrong
		return true;
	}

	Bool searchForFocusClues = false;
	if ( m_showFocusClues )
	{
		CFocusModeController* focusController = GCommonGame->GetSystem< CFocusModeController >();
		if ( focusController && focusController->IsActive() )
		{
			searchForFocusClues = true;
		}
	}

	THandle< CGameplayEntity > usedEntity;
	CallFunctionRet( GCommonGame->GetPlayer(), CNAME( GetUsedVehicle ), usedEntity );
	CGameplayEntity* entityToIgnore = usedEntity.Get();

	Bool playerInCombat = player->IsInCombat();
	Bool enabledHerbsOnMinimap = ( factsManager->QuerySum( TXT( "disable_herbs_on_minimap" ) ) == 0 );

	const Vector& playerPos = player->GetWorldPositionRef();

	Int32 initialCount = m_nearbyEntitiesHandles.SizeInt();
	//
	//RED_LOG( RED_LOG_CHANNEL( PROCESSING ), TXT("Initial count: %d"), initialCount );
	//

	// iterate backwards and delete last item each time
	for ( Int32 i = m_nearbyEntitiesHandles.SizeInt() - 1; i >= 0; --i )
	{
		if ( !processAll )
		{
			Int32 processed = initialCount - 1 - i;

			//
			//RED_LOG( RED_LOG_CHANNEL( PROCESSING ), TXT("Processed: %d, count: %d"), processed, m_nearbyEntitiesHandles.SizeInt() );
			//
			if ( processed >= DYNAMIC_MAPPIN_UPDATE_COUNT )
			{
				// max number of processed entities, quit loop and leave the rest for next tick
				break;
			}
		}

		CGameplayEntity* gameplayEntity = Cast< CGameplayEntity >( m_nearbyEntitiesHandles[ i ].Get() );
		if ( !gameplayEntity )
		{
			m_nearbyEntitiesHandles.RemoveAtFast( i );
			continue;
		}
		if ( gameplayEntity == player )
		{
			m_nearbyEntitiesHandles.RemoveAtFast( i );
			continue;
		}
		if ( gameplayEntity->HasTag( CNAME( NoMapPin ) ) )
		{
			m_nearbyEntitiesHandles.RemoveAtFast( i );
			continue;
		}

		CActor* actor = Cast< CActor >( gameplayEntity );

		CName gameplayEntityTag;
		CName customMapPinType;
		const TagList& tagList = gameplayEntity->GetTags();
		for ( Uint32 t = 0; t < tagList.Size(); ++t )
		{
			const CName& tag = tagList.GetTag( t );

			if ( m_customEntityMapPins.Find( tag, customMapPinType ) )
			{
				gameplayEntityTag = tag;
				break;
			}

			if ( actor )
			{
				// check agent tags only for actors
				if ( !m_discoveredAgentEntityTags.Exist( tag ) )
				{
					if ( m_customAgentMapPins.KeyExist( tag ) )
					{
						m_discoveredAgentEntityTags.Insert( tag );
					}
				}
			}
		}

		CName type;
		if ( customMapPinType != CName::NONE )
		{
			type = customMapPinType;
		}
		else
		{
			const Vector& entityPos = gameplayEntity->GetWorldPositionRef();
			float heightDiff = fabs( playerPos.Z - entityPos.Z );
			if ( heightDiff < DYNAMIC_MAPPIN_VISIBLILITY_VERTICAL_THRESHOLD )
			{
				if ( actor )
				{
					if ( actor->GetAttitude( player ) == AIA_Hostile )
					{
						Bool isNetrualHorse = false;
						if ( actor->HasTag( CNAME( horse ) ) )
						{
							// check if horse has rider
							THandle< CActor > horseRider;
							CallFunctionRet( actor, CNAME( GetHorseUser ), horseRider );
							if ( !horseRider.Get() )
							{
								// hostile horses that do not have a rider are actually neutral
								isNetrualHorse = true;
							}
						}
						if ( isNetrualHorse )
						{
							// neutral horses should not be displayed
						}
						else if ( actor->IsAlive() && IsEntityVisible( actor ) )
						{
							type = CNAME( Enemy );
						}
						//else
						//{
						//	type = CNAME( EnemyDead );
						//}
					}
				}
				else if ( gameplayEntity )
				{
					if ( m_herbClass && gameplayEntity->IsA( m_herbClass ) )
					{
						if ( !playerInCombat &&
							enabledHerbsOnMinimap &&
							playerPos.DistanceSquaredTo2D( entityPos ) < HERB_SEARCH_RADIUS * HERB_SEARCH_RADIUS )
						{
							if ( CallFunctionRef( gameplayEntity, CNAME( GetStaticMapPinTag ), gameplayEntityTag ) )
							{
								if ( gameplayEntityTag != CName::NONE )
								{
									type = CNAME( Herb );
								}
							}
						}
					}
					/*
					else if ( m_boatClass && gameplayEntity->IsA( m_boatClass ) )
					{
						if ( gameplayEntity != entityToIgnore )
						{
							type = CNAME( Boat );
						}
					}
					*/
					else if ( m_lootClass && gameplayEntity->IsA( m_lootClass ) )
					{
						type = CNAME( EnemyDead );
					}
				}
			}
		}

		if ( searchForFocusClues )
		{
			if ( m_clueClass && gameplayEntity->IsA( m_clueClass ) )
			{
				if ( gameplayEntity->GetFocusModeVisibility() == FMV_Clue )
				{
					const Vector& cluePos = gameplayEntity->GetWorldPositionRef();
					float heightDiff = fabs( playerPos.Z - cluePos.Z );
					if ( heightDiff < MONSTER_CLUE_MAPPIN_VISIBLILITY_VERTICAL_THRESHOLD )
					{
						if ( playerPos.DistanceSquaredTo2D( cluePos ) < MONSTER_CLUE_SEARCH_RADIUS * MONSTER_CLUE_SEARCH_RADIUS )
						{
							type = CNAME( GenericFocus );
						}
					}
				}
			}
		}

		if ( type != CName::NONE )
		{
			PlaceDynamicMapPin( gameplayEntity, gameplayEntityTag, type, 0, CGUID::ZERO, true );
		}

		m_nearbyEntitiesHandles.RemoveAtFast( i );
	}

	// return true if there is nothing to process anymore
	return m_nearbyEntitiesHandles.Size() == 0;
}

void CCommonMapManager::UpdateDynamicMapPins_Finalization()
{
	//
	//RED_LOG( RED_LOG_CHANNEL( PROCESSING ), TXT("UpdateDynamicMapPins_Finalization") );
	//
	PC_SCOPE_PIX( UpdateDynamicMapPins_Finalization );

	RED_ASSERT( m_nearbyEntitiesHandles.Size() == 0, TXT("UpdateDynamicMapPins_Finalization") );

	m_currDynamicMapPinIds.Sort();

	DeleteOldDynamicMapPins();

	m_prevDynamicMapPinIds.ResizeFast( m_currDynamicMapPinIds.Size() );
	Red::System::MemoryCopy( m_prevDynamicMapPinIds.TypedData(), m_currDynamicMapPinIds.TypedData(), m_currDynamicMapPinIds.DataSize() );
}

void CCommonMapManager::UpdateDynamicMapPinsPositionsAndVisibility()
{
	PC_SCOPE_PIX( UpdateDynamicMapPinsPositionsAndVisibility );

	Bool forceUpdate = false;

	THashMap< Int32, SCommonMapPinInstance >::iterator itEnd = m_mapPins.End();
	for ( THashMap< Int32, SCommonMapPinInstance >::iterator itPin = m_mapPins.Begin(); itPin != itEnd; ++itPin )
	{ 
		SCommonMapPinInstance& pin = itPin->m_second;
		if ( pin.m_isDynamic )
		{
			// position
			Vector currPos;
			if ( pin.GetEntityPosition( currPos ) )
			{
				if ( pin.m_position != currPos )
				{
					pin.m_position = currPos;
					if ( pin.m_isAddedToMinimap )
					{
						MoveMapPinOnMinimap( pin );
					}
				}
			}

			// visibility
			if ( !forceUpdate )
			{
				if ( IsEnemyType( pin.m_type ) || IsQuestPinType( pin.m_type ) )
				{
					for ( Uint32 i = 0; i < pin.m_entities.Size(); ++i )
					{
						if ( !IsEntityVisible( pin.m_entities[ i ] ) )
						{
							forceUpdate = true;
							break;
						}
					}
				}
			}
		}
	}

	if ( forceUpdate )
	{
		ScheduleUpdateDynamicMapPins();
	}
}

Bool CCommonMapManager::IsQuestMapPinEnabled( const CJournalQuestObjective* objective, const CJournalQuestMapPin* mapPin )
{
	TPair< CGUID, CGUID > pair( objective->GetGUID(), mapPin->GetGUID() );
	
	Bool addMapPin = false;
	auto mapPinStateIter = m_questMapPinStates.Find( pair );
	if ( mapPinStateIter != m_questMapPinStates.End() )
	{
		// get current map pin state from given objective
		addMapPin = mapPinStateIter->m_second;
	}
	else
	{
		// get initial map pin state and keep it as current
		addMapPin = mapPin->IsEnabledAtStartup();
		m_questMapPinStates.Insert( pair, addMapPin );
	}
	return addMapPin;
}

Bool CCommonMapManager::IsEntityVisible( const CEntity* entity ) const
{
	if ( !entity )
	{
		return false;
	}

	const CActor* actor = Cast< CActor >( entity );
	if ( actor )
	{
		Bool visibility = false;
		if ( m_getGameplayVisibilityFunction &&
			 m_getGameplayVisibilityFunction->Call( const_cast< CActor* >( actor ), nullptr, &visibility ) )
		{
			return visibility;
		}
	}
	return true;
}

Bool CCommonMapManager::GetQuestMapPinPositions( const CName& tag, TDynArray< Vector >& positions )
{
	if ( GetQuestMapPinPositionFromEntities( tag, positions ) )
	{
		return true;
	}
	if ( GetQuestMapPinPositionFromCommunities( tag, positions ) )
	{
		return true;
	}
	return false;
}

Bool CCommonMapManager::GetQuestMapPinPositionFromEntities( const CName& tag, TDynArray< Vector >& positions )
{
	Bool ret = false;

	positions.ClearFast();

	CGameWorld* world = GCommonGame->GetActiveWorld();
	if ( !world )
	{
		return ret;
	}
	CTagManager* tagManager = world->GetTagManager();
	if ( !tagManager )
	{
		return ret;
	}

	m_entities.ClearFast();
	tagManager->CollectTaggedEntities( tag, m_entities );

	if ( m_entities.Size() == 0 )
	{
		return ret;
	}
	for ( Uint32 i = 0; i < m_entities.Size(); i++ )
	{
		CEntity* entity = m_entities[ i ];
		if ( !entity )
		{
			continue;
		}
		if ( !IsEntityVisible( entity ) )
		{
			ret = true;
			continue;
		}

		positions.PushBack( entity->GetWorldPositionRef() );
		ret = true;
	}
	return ret;
}

Bool CCommonMapManager::GetQuestMapPinPositionFromCommunities( const CName& tag, TDynArray< Vector >& positions )
{
	Bool ret = false;

	positions.ClearFast();

	CGameWorld* world = GCommonGame->GetActiveWorld();
	if ( !world )
	{
		return ret;
	}
	CTagManager* tagManager = world->GetTagManager();
	if ( !tagManager )
	{
		return ret;
	}
	CCommunitySystem* communitySystem = GCommonGame->GetSystem< CCommunitySystem >();
	if ( !communitySystem )
	{
		return ret;
	}
	CCommunityAgentStubTagManager* agentStubTagManager = communitySystem->GetAgentStubTagManager();
	if ( !agentStubTagManager )
	{
		return ret;
	}

	m_agentStubs.ClearFast();
	agentStubTagManager->CollectTaggedStubs( tag, m_agentStubs );
	for ( Uint32 i = 0; i < m_agentStubs.Size(); ++i)
	{
		SAgentStub* agentStub = m_agentStubs[ i ];
		if ( agentStub->GetEntitiesEntry()->m_entitySpawnTags.HasTag( tag ) )
		{
			positions.PushBackUnique( agentStub->m_communityAgent.GetPosition() );
			ret = true;
		}
	}

	return ret;
}

Bool CCommonMapManager::PlaceQuestMapPinsFromEntities( const SQuestPinData& mapPinData )
{
	Bool ret = false;

	if ( !GCommonGame->GetActiveWorld() )
	{
		return ret;
	}
	CTagManager* tagManager = GCommonGame->GetActiveWorld()->GetTagManager();
	if ( !tagManager )
	{
		return ret;
	}

	m_entities.ClearFast();
	tagManager->CollectTaggedEntities( mapPinData.m_tag, m_entities );

	if ( m_entities.Size() == 0 )
	{
		return ret;
	}
	for ( Uint32 i = 0; i < m_entities.Size(); i++ )
	{
		CEntity* entity = m_entities[ i ];

		if ( !entity )
		{
			continue;
		}
		if ( !IsEntityVisible( entity ) )
		{
			ret = true;
			continue;
		}

		if ( !m_useInteriorsForQuestMapPins )
		{
			PlaceDynamicMapPin( entity, mapPinData.m_tag, CCommonMapManager::GetQuestNameFromType( mapPinData.m_mapPinType, mapPinData.m_questType ), mapPinData.m_radius, mapPinData.m_objectiveGUID, true );
			ret = true;
		}
		else
		{
			if ( PlaceDynamicMapPinWithInteriors( entity, mapPinData.m_tag, CCommonMapManager::GetQuestNameFromType( mapPinData.m_mapPinType, mapPinData.m_questType ), mapPinData.m_radius, mapPinData.m_objectiveGUID, true ) )
			{
				ret = true;
			}
		}
	}
	return ret;
}

Bool CCommonMapManager::PlaceCustomMapPinsFromEntities( const SCustomMapPinDefinition& def, const CGameplayEntity* entityToIgnore )
{
	if ( !GCommonGame->GetActiveWorld() )
	{
		return false;
	}
	CTagManager* tagManager = GCommonGame->GetActiveWorld()->GetTagManager();
	if ( !tagManager )
	{
		return false;
	}

	m_entities.ClearFast();
	tagManager->CollectTaggedEntities( def.m_tag, m_entities );

	if ( m_entities.Size() == 0 )
	{
		return false;
	}
	for ( Uint32 i = 0; i < m_entities.Size(); i++ )
	{
		CEntity* entity = m_entities[ i ];
		if ( entityToIgnore == entity )
		{
			// ignore entity we're currently use (horse, boat)
			continue;
		}
		PlaceDynamicMapPin( entity, def.m_tag, def.m_type, 0, CGUID::ZERO, true );
	}
	return true;
}

void CCommonMapManager::PlaceShopkeeperMapPins()
{
	if ( !m_shopComponentClass )
	{
		return;
	}

	if ( !GCommonGame->GetActiveWorld() )
	{
		return;
	}
	CTagManager* tagManager = GCommonGame->GetActiveWorld()->GetTagManager();
	if ( !tagManager )
	{
		return;
	}

	CCachedWorldData* cachedData = GetCurrentWorldCachedData();
	if ( !cachedData )
	{
		return;
	}

	// update shopkeepers data
	m_entities.ClearFast();
	tagManager->CollectTaggedEntities( CNAME( ShopkeeperEntity ), m_entities );
	for ( Uint32 i = 0; i < m_entities.Size(); ++i )
	{
		CNewNPC* npc = Cast< CNewNPC >( m_entities[ i ] );
		if ( npc )
		{
			for ( ComponentIterator< CScriptedComponent > it( npc ); it; ++it )
			{
				CScriptedComponent* component = *it;
				if ( component->GetClass()->IsA( m_shopComponentClass ) )
				{
					cachedData->UpdateShopkeeperData( npc, component );
					break;
				}
			}
		}
	}

	// update shopkeepers agents
	cachedData->UpdateShopkeeperAgents();

	// place shopkeeper map pins
	TDynArray< CName > dataToDelete;
	THashMap< CName, CCachedShopkeeperData >& shopkeeperData = cachedData->GetShopkeeperData();
	for ( THashMap< CName, CCachedShopkeeperData >::const_iterator it = shopkeeperData.Begin(); it != shopkeeperData.End(); ++it )
	{
		const CCachedShopkeeperData& data = it->m_second;
		if ( !data.CanMapPinBeShown() )
		{
			continue;
		}
		
		const CEntity* entity = data.GetEntity();
		if ( entity )
		{
			PlaceDynamicMapPin( entity, CName::NONE, data.GetType(), 0, CGUID::ZERO, true );
		}
		else
		{
			if ( data.IsCacheable() )
			{
				// there's no entity we need to use cached data
				PlaceDynamicMapPin( data.GetId(), data.GetPosition(), CName::NONE, data.GetType(), 0, CGUID::ZERO, true );
			}
			else
			{
				// there's no entity and map pin is not cacheable, delete
				dataToDelete.PushBack( it->m_first );
			}
		}
	}

	// delete non cacheable shopkeeper data
	for ( Uint32 i = 0; i < dataToDelete.Size(); ++i )
	{
		shopkeeperData.Erase( dataToDelete[ i ] );
	}
}

void CCommonMapManager::PlacePlayerCompanionMapPin( const CNewNPC* companion )
{
	if ( companion )
	{
		PlaceDynamicMapPin( companion, CName::NONE, CNAME( Companion ), 0, CGUID::ZERO, true );
	}
}

void CCommonMapManager::PlacePlayerHorseMapPin( const CGameplayEntity* entityToIgnore )
{
	if ( !GCommonGame )
	{
		return;
	}
	CR4Player* player = Cast< CR4Player >( GCommonGame->GetPlayer() );
	if ( !player )
	{
		return;
	}
	const CActor* horse = player->GetHorseWithInventory();
	if ( horse )
	{
		if ( horse != entityToIgnore )
		{
			PlaceDynamicMapPin( horse, CName::NONE, CNAME( Horse ), 0, CGUID::ZERO, true );
		}
	}
}

void CCommonMapManager::PlaceBoatMapPins( const CGameplayEntity* entityToIgnore )
{
	for ( Uint32 i = 0; i < m_boatEntities.Size(); ++i )
	{
		const W3Boat* boat = Cast< W3Boat >( m_boatEntities[ i ] );
		if ( boat && !boat->HasDrowned() )
		{
			if ( boat != entityToIgnore )
			{
				PlaceDynamicMapPin( boat, CName::NONE, CNAME( Boat ), 0, CGUID::ZERO, true );
			}
		}
	}
}

void CCommonMapManager::PlaceUserMapPins()
{
	for ( Uint32 index = 0; index < m_userMapPins.Size(); ++index )
	{
		if ( m_currentArea != m_userMapPins[ index ].m_area )
		{
			continue;
		}

		Int32 id = m_userMapPins[ index ].m_id;
		Vector position = Vector( m_userMapPins[ index ].m_position.X, m_userMapPins[ index ].m_position.Y, 0 );
		TDynArray< THandle< CEntity > > entityHandles;
		Int32 typeIndex = m_userMapPins[ index ].m_type;
		CName typeName;
		if ( typeIndex >= 0 && typeIndex < sizeof( m_userPinName ) / sizeof( m_userPinName[ 0 ] ) )
		{
			typeName = m_userPinName[ typeIndex ];
		}

		SCommonMapPinInstance* instance = m_mapPins.FindPtr( id );
		if ( instance )
		{
			// if position has changed
			if ( instance->m_position.AsVector2() != m_userMapPins[ index ].m_position )
			{
				instance->m_position.X = m_userMapPins[ index ].m_position.X;
				instance->m_position.Y = m_userMapPins[ index ].m_position.Y;
				MoveMapPinOnMinimap( *instance );
			}
		}

		m_currDynamicMapPinIds.PushBack( id );
		if ( !m_prevDynamicMapPinIds.Exist( id ) )
		{
			AddMapPin( id, position, entityHandles, CName::NONE, 0, CName::NONE, typeName, typeName, 0, CGUID::ZERO, false, true, false, false );
		}
	}
}

void CCommonMapManager::PlaceQuestMapPinsFromCommunities( const SQuestPinData& mapPinData )
{
	if ( !GCommonGame->GetActiveWorld() )
	{
		return;
	}
	CTagManager* tagManager = GCommonGame->GetActiveWorld()->GetTagManager();
	if ( !tagManager )
	{
		return;
	}

	CCommunitySystem* communitySystem = GCommonGame->GetSystem< CCommunitySystem >();
	if ( !communitySystem )
	{
		return;
	}

	CCommunityAgentStubTagManager* agentStubTagManager = communitySystem->GetAgentStubTagManager();
	if ( !agentStubTagManager )
	{
		return;
	}

	m_agentStubs.ClearFast();
	agentStubTagManager->CollectTaggedStubs( mapPinData.m_tag, m_agentStubs );
	for ( Uint32 i = 0; i < m_agentStubs.Size(); ++i)
	{
		SAgentStub* agentStub = m_agentStubs[ i ];
		if ( agentStub->GetEntitiesEntry()->m_entitySpawnTags.HasTag( mapPinData.m_tag ) )
		{
			if ( !m_useInteriorsForQuestMapPins )
			{
				PlaceDynamicMapPin( agentStub, mapPinData.m_tag, CCommonMapManager::GetQuestNameFromType( mapPinData.m_mapPinType, mapPinData.m_questType ), mapPinData.m_radius, mapPinData.m_objectiveGUID, false );
			}
			else
			{
				PlaceDynamicMapPinWithInteriors( agentStub, mapPinData.m_tag, CCommonMapManager::GetQuestNameFromType( mapPinData.m_mapPinType, mapPinData.m_questType ), mapPinData.m_radius, mapPinData.m_objectiveGUID, false );
			}
			return;
		}
	}
}

void CCommonMapManager::PlaceQuestMapPinFromFastTravelPoint( const SQuestPinData& mapPinData )
{
	Int32 id = static_cast< Int32 >( ::GetHash( mapPinData.m_mapPinGUID ) );
	const Vector& position = GetClosestFastTravelPoint();

	PlaceDynamicMapPin( id, position, mapPinData.m_tag, CCommonMapManager::GetQuestNameFromType( mapPinData.m_mapPinType, mapPinData.m_questType ), 0, mapPinData.m_objectiveGUID, true );
}

void CCommonMapManager::PlacePossibleQuestMapPinsFromCommunities( const CName& tag, const CName& type )
{
	if ( !GCommonGame->GetActiveWorld() )
	{
		return;
	}

	CCommunitySystem* communitySystem = GCommonGame->GetSystem< CCommunitySystem >();
	if ( !communitySystem )
	{
		return;
	}

	CCommunityAgentStubTagManager* agentStubTagManager = communitySystem->GetAgentStubTagManager();
	if ( !agentStubTagManager )
	{
		return;
	}

	m_agentStubs.ClearFast();
	agentStubTagManager->CollectTaggedStubs( tag, m_agentStubs );
	for ( Uint32 i = 0; i < m_agentStubs.Size(); ++i)
	{
		SAgentStub* agentStub = m_agentStubs[ i ];
		if ( agentStub->GetEntitiesEntry()->m_entitySpawnTags.HasTag( tag ) )
		{
			if ( !m_useInteriorsForQuestMapPins )
			{
				PlaceDynamicMapPin( agentStub, tag, type, 0, CGUID::ZERO, false );
			}
			else
			{
				PlaceDynamicMapPinWithInteriors( agentStub, tag, type, 0, CGUID::ZERO, false );
			}
			return;
		}
	}
}

Bool CCommonMapManager::PlaceDynamicMapPinWithInteriors( const CEntity* entity, const CName& tag, const CName& type, float radius, const CGUID& guid, Bool isDynamic )
{
	Bool ret = false;
	CR4InteriorAreaComponent* entityInterior = GetInteriorFromPosition( entity->GetWorldPositionRef() );
	CR4InteriorAreaComponent* playerInterior = nullptr;

	if ( !m_playerInteriors.Empty() )
	{
		playerInterior = m_playerInteriors[ m_playerInteriors.Size() - 1 ].Get();
	}

	if ( !playerInterior )
	{
		// player is not in any interior
		if ( !entityInterior )
		{
			// quest entity is not in any interior
			// place quest map pin in original position
			PlaceDynamicMapPin( entity, tag, type, radius, guid, isDynamic );
			ret = true;
		}
		else
		{
			// quest entity is in some interior
			// place quest map pin on entrance of quest interior
			const CEntity* closestEntrance = GetClosestInteriorEntranceToPosition( entityInterior, entity->GetWorldPositionRef() );
			if ( closestEntrance )
			{
				PlaceDynamicMapPin( /*&closestEntrance->GetWorldPositionRef(),*/ entity, tag, type, radius, guid, isDynamic );
				ret = true;
			}
			else
			{
				// no entrance found
				// place quest map pin in original position
				PlaceDynamicMapPin( entity, tag, type, radius, guid, isDynamic );
				ret = true;
			}
		}
	}
	else if ( playerInterior )
	{
		// player is in interior
		if ( playerInterior == entityInterior )
		{
			// player and quest entity are in the same interior
			// place quest map pin in original position
			PlaceDynamicMapPin( entity, tag, type, radius, guid, isDynamic );
			ret = true;
		}
		else
		{
			// place quest map pin on entrance of player interior
			const CEntity* closestEntrance = GetClosestInteriorEntranceToPosition( playerInterior, entity->GetWorldPositionRef() );
			if ( closestEntrance )
			{
				PlaceDynamicMapPin( /*&closestEntrance->GetWorldPositionRef(),*/ entity, tag, type, radius, guid, isDynamic );
				ret = true;
			}
			else
			{
				// no entrance found, place quest map pin in original position
				PlaceDynamicMapPin( entity, tag, type, radius, guid, isDynamic );
				ret = true;
			}
		}
	}
	return ret;
}

Bool CCommonMapManager::PlaceDynamicMapPinWithInteriors( const TDynArray< CEntity* >& entities, const CName& tag, const CName& type, float radius, const CGUID& guid, Bool isDynamic )
{
	Vector centerPosition = GetCenterPosition( entities );
	CR4InteriorAreaComponent* entityInterior = GetInteriorFromPosition( centerPosition );
	CR4InteriorAreaComponent* playerInterior = nullptr;

	if ( !m_playerInteriors.Empty() )
	{
		playerInterior = m_playerInteriors[ m_playerInteriors.Size() - 1 ].Get();
	}

	if ( !playerInterior )
	{
		// player is not in any interior
		if ( !entityInterior )
		{
			// quest entity is not in any interior
			// place quest map pin in original position
			PlaceDynamicMapPin( entities, tag, type, radius, guid, isDynamic );
		}
		else
		{
			// quest entity is in some interior
			// place quest map pin on entrance of quest interior
			const CEntity* closestEntrance = GetClosestInteriorEntranceToPosition( entityInterior, centerPosition );
			if ( closestEntrance )
			{
				PlaceDynamicMapPin( /*&closestEntrance->GetWorldPositionRef(),*/ entities, tag, type, radius, guid, isDynamic );
			}
			else
			{
				// no entrance found
				// place quest map pin in original position
				PlaceDynamicMapPin( entities, tag, type, radius, guid, isDynamic );
			}
		}
	}
	else if ( playerInterior )
	{
		// player is in interior
		if ( playerInterior == entityInterior )
		{
			// player and quest entity are in the same interior
			// place quest map pin in original position
			PlaceDynamicMapPin( entities, tag, type, radius, guid, isDynamic );
		}
		else
		{
			// place quest map pin on entrance of player interior
			const CEntity* closestEntrance = GetClosestInteriorEntranceToPosition( playerInterior, centerPosition );
			if ( closestEntrance )
			{
				PlaceDynamicMapPin( /*&closestEntrance->GetWorldPositionRef(),*/ entities, tag, type, radius, guid, isDynamic );
			}
			else
			{
				// no entrance found, place quest map pin in original position
				PlaceDynamicMapPin( entities, tag, type, radius, guid, isDynamic );
			}
		}
	}
	return true;
}

Bool CCommonMapManager::PlaceDynamicMapPinWithInteriors( const SAgentStub* agentStub, const CName& tag, const CName& type, float radius, const CGUID& guid, Bool isDynamic )
{
	const Vector& centerPosition = agentStub->m_communityAgent.GetPosition();
	CR4InteriorAreaComponent* entityInterior = GetInteriorFromPosition( centerPosition );
	CR4InteriorAreaComponent* playerInterior = nullptr;

	if ( !m_playerInteriors.Empty() )
	{
		playerInterior = m_playerInteriors[ m_playerInteriors.Size() - 1 ].Get();
	}

	if ( !playerInterior )
	{
		// player is not in any interior
		if ( !entityInterior )
		{
			// quest entity is not in any interior
			// place quest map pin in original position
			PlaceDynamicMapPin( agentStub, tag, type, radius, guid, isDynamic );
		}
		else
		{
			// quest entity is in some interior
			// place quest map pin on entrance of quest interior
			const CEntity* closestEntrance = GetClosestInteriorEntranceToPosition( entityInterior, centerPosition );
			if ( closestEntrance )
			{
				PlaceDynamicMapPin( /*&closestEntrance->GetWorldPositionRef(),*/ agentStub, tag, type, radius, guid, isDynamic );
			}
			else
			{
				// no entrance found
				// place quest map pin in original position
				PlaceDynamicMapPin( agentStub, tag, type, radius, guid, isDynamic );
			}
		}
	}
	else if ( playerInterior )
	{
		// player is in interior
		if ( playerInterior == entityInterior )
		{
			// player and quest entity are in the same interior
			// place quest map pin in original position
			PlaceDynamicMapPin( agentStub, tag, type, radius, guid, isDynamic );
		}
		else
		{
			// place quest map pin on entrance of player interior
			const CEntity* closestEntrance = GetClosestInteriorEntranceToPosition( playerInterior, centerPosition );
			if ( closestEntrance )
			{
				PlaceDynamicMapPin( /*&closestEntrance->GetWorldPositionRef(),*/ agentStub, tag, type, radius, guid, isDynamic );
			}
			else
			{
				// no entrance found, place quest map pin in original position
				PlaceDynamicMapPin( agentStub, tag, type, radius, guid, isDynamic );
			}
		}
	}
	return true;
}

void CCommonMapManager::PlaceDynamicMapPin( const CEntity* entity, const CName& tag, const CName& type, float radius, const CGUID& guid, Bool isDynamic )
{
	// guids are not unique on consoles!
	Int32 id = static_cast< Int32 >( ::GetHash( reinterpret_cast< Uint64 >( entity ) ) );

	SCommonMapPinInstance* instance = m_mapPins.FindPtr( id );
	if ( instance )
	{
		if ( !instance->IsValid() )
		{
			// lost entity, we need to delete pin
			return;
		}

		if ( type != instance->m_type )
		{
			// if type is different than type of existing map pin
			if ( GetMapPinTypePriority( type ) > GetMapPinTypePriority( instance->m_type ) )
			{
				UpdateMapPin( instance, tag, 0, CName::NONE, type, radius, guid, isDynamic, true );
				return;
			}
			else
			{
				// don't update, don't add, don't do anything
				return;
			}
		}
	}

	m_currDynamicMapPinIds.PushBack( id );
	if ( !m_prevDynamicMapPinIds.Exist( id ) )
	{
		AddMapPin( id, entity, tag, 0, CName::NONE, type, type, radius, guid, isDynamic, true );
	}
}

void CCommonMapManager::PlaceDynamicMapPin( const TDynArray< CEntity* >& entities, const CName& tag, const CName& type, float radius, const CGUID& guid, Bool isDynamic )
{
	TDynArray< THandle< CEntity > > entityHandles;

	Int32 id = 0;
	for ( Uint32 i = 0; i < entities.Size(); i++ )
	{
		// guids are not unique on consoles!
		Int32 entityId = ::GetHash( reinterpret_cast< Uint64 >( entities[ i ] ) );
		if ( id < entityId )
		{
			id = entityId;
		}
		entityHandles.PushBack( THandle< CEntity >( entities[ i ] ) );
	}
	const Vector& mapPinPosition = GetCenterPosition( entities );

	SCommonMapPinInstance* instance = m_mapPins.FindPtr( id );
	if ( instance )
	{
		if ( !instance->IsValid() )
		{
			// lost entity, we need to delete pin
			return;
		}

		if ( type != instance->m_type )
		{
			// if type is different than type of existing map pin
			if ( GetMapPinTypePriority( type ) > GetMapPinTypePriority( instance->m_type ) )
			{
				UpdateMapPin( instance, tag, 0, CName::NONE, type, radius, guid, isDynamic, true );
				return;
			}
			else
			{
				// don't update, don't add, don't do anything
				return;
			}
		}
	}

	m_currDynamicMapPinIds.PushBack( id );
	if ( !m_prevDynamicMapPinIds.Exist( id ) )
	{
		AddMapPin( id, mapPinPosition, entityHandles, tag, 0, CName::NONE, type, type, radius, guid, isDynamic, true, false, false );
	}
}

void CCommonMapManager::PlaceDynamicMapPin( const SAgentStub* agentStub, const CName& tag, const CName& type, float radius, const CGUID& guid, Bool isDynamic )
{
	TDynArray< THandle< CEntity > > entityHandles;

	const Vector& mapPinPosition = agentStub->m_communityAgent.GetPosition();

	Int32 id = static_cast< Int32 >( ::GetHash( reinterpret_cast< Uint64>( agentStub ) ) );

	SCommonMapPinInstance* instance = m_mapPins.FindPtr( id );
	if ( instance )
	{
		if ( instance->m_position != mapPinPosition )
		{
			// if position is different than position of existing map pin
			instance->m_position = mapPinPosition;
			MoveMapPinOnMinimap( *instance );
		}

		if ( type != instance->m_type )
		{
			// if type is different than type of existing map pin
			if ( GetMapPinTypePriority( type ) > GetMapPinTypePriority( instance->m_type ) )
			{
				UpdateMapPin( instance, tag, 0, CName::NONE, type, radius, guid, isDynamic, true );
				return;
			}
			else
			{
				// don't update, don't add, don't do anything
				return;
			}
		}
	}

	m_currDynamicMapPinIds.PushBack( id );
	if ( !m_prevDynamicMapPinIds.Exist( id ) )
	{
		AddMapPin( id, mapPinPosition, entityHandles, tag, 0, CName::NONE, type, type, radius, guid, isDynamic, true, false, false );
	}
}

void CCommonMapManager::PlaceDynamicMapPin( Int32 id, const Vector& mapPinPosition, const CName& tag, const CName& type, float radius, const CGUID& guid, Bool isDynamic )
{
	TDynArray< THandle< CEntity > > entityHandles;

	SCommonMapPinInstance* instance = m_mapPins.FindPtr( id );
	if ( instance )
	{
		if ( instance->m_position != mapPinPosition )
		{
			// if position is different than position of existing map pin
			instance->m_position = mapPinPosition;
			MoveMapPinOnMinimap( *instance );
		}

		if ( type != instance->m_type )
		{
			// if type is different than type of existing map pin
			if ( GetMapPinTypePriority( type ) > GetMapPinTypePriority( instance->m_type ) )
			{
				UpdateMapPin( instance, tag, 0, CName::NONE, type, radius, guid, isDynamic, true );
				return;
			}
			else
			{
				// don't update, don't add, don't do anything
				return;
			}
		}
	}

	m_currDynamicMapPinIds.PushBack( id );
	if ( !m_prevDynamicMapPinIds.Exist( id ) )
	{
		AddMapPin( id, mapPinPosition, entityHandles, tag, 0, CName::NONE, type, type, radius, guid, isDynamic, true, false, false );
	}
}

void CCommonMapManager::DeleteOldDynamicMapPins()
{
	TDynArray< Int32 > mapPinIdsToDelete;

	Uint32 ci = 0;
	for ( Uint32 pi = 0; pi < m_prevDynamicMapPinIds.Size(); )
	{
		Int32 prevHash = m_prevDynamicMapPinIds[ pi ];
		if ( ci >= m_currDynamicMapPinIds.Size() )
		{
			// id from prev ids is not in current ids, delete prev one
			mapPinIdsToDelete.PushBack( prevHash );
			pi++;
		}
		else
		{
			Int32 currHash = m_currDynamicMapPinIds[ ci ];
			if ( currHash < prevHash )
			{
				// there's just new current id
				ci++;
			}
			else if ( currHash > prevHash )
			{
				// there's no curr id for corresponding prev id, delete
				mapPinIdsToDelete.PushBack( prevHash );
				pi++;
			}
			else // do nothing, they are equal so both out there
			{
				pi++;
				ci++;
			}
		}
	}

	if ( mapPinIdsToDelete.Size() )
	{
		for ( Uint32 i = 0; i < mapPinIdsToDelete.Size(); ++i )
		{
			DeleteMapPin( mapPinIdsToDelete[ i ] );
		}		
	}
}

Uint32 CCommonMapManager::GetMapPinTypePriority( const CName& type )
{
	if ( type == CNAME( Companion ) )
	{
		return 1000;
	}
	if ( CCommonMapManager::IsQuestPinType( type ) )
	{
		return 900;
	}
	if ( type == CNAME( Horse ) )
	{
		return 800;
	}
	if ( type == CNAME( EnemyDead ) )
	{
		return 1;
	}
	if ( type ==CCommonMapManager::IsEnemyType( type ) )
	{
		// lowest priority
		return 0;
	}
	return 100;
}

void CCommonMapManager::UpdateStaticMapPins()
{
	if ( m_invalidatedEntityMapPins )
	{
		m_invalidatedEntityMapPins = false;

		CWorld* world = GCommonGame->GetActiveWorld();
		if ( !world )
		{
			return;
		}
		CTagManager* tagManager = world->GetTagManager();
		if ( !tagManager )
		{
			return;
		}

		THashMap< Int32, SCommonMapPinInstance >::iterator itEnd = m_mapPins.End();
		for ( THashMap< Int32, SCommonMapPinInstance >::iterator itPin = m_mapPins.Begin(); itPin != itEnd; ++itPin )
		{
			SCommonMapPinInstance& pin = itPin->m_second;
			if ( !pin.m_isDynamic && pin.m_invalidated )
			{
				pin.m_invalidated = false;

				if ( IsKnowableMapPinType( pin.m_type ) )
				{
					Bool updateOnMinimap = false;

					CName visibleType = GetMapPinVisibleType( pin.m_type, pin.m_isKnown, pin.m_isDiscovered, pin.m_isDisabled );
					if ( visibleType != CName::NONE && pin.m_visibleType != visibleType )
					{
						pin.m_visibleType = visibleType;
						updateOnMinimap = true;
					}

					Float visibleRadius = CCommonMapManager::GetVisibleRadius( pin.m_visibleType, pin.m_radius );
					if ( pin.m_visibleRadius != visibleRadius )
					{
						pin.m_visibleRadius = visibleRadius;
						updateOnMinimap = true;
					}
					if ( updateOnMinimap )
					{
						UpdateMapPinOnMinimap( pin );
					}
				}
				else if ( IsNoticeboardType( pin.m_type ) )
				 {
					CEntity* entity = tagManager->GetTaggedEntity( pin.m_tag );
					if ( entity )
					{
						CName visibleType;
						if ( CallFunctionRef( entity, CNAME( GetStaticMapPinType ), visibleType ) )
						{
							if ( pin.m_visibleType != visibleType )
							{
								pin.m_visibleType = visibleType;
								UpdateMapPinOnMinimap( pin );
							}
						}
					}
				}
			}
		}
	}
}

void CCommonMapManager::LoadMapPinInfoFromScripts()
{
	// get custom map pin definitions from scripts
	m_customMapPinDefinition.ClearFast();
	if ( !CallFunctionRef( this, CNAME( GetCustomMapPinDefinition ), m_customMapPinDefinition ) )
	{
		RED_HALT(  "Script function CCommonMapManager::GetDynamicMapPinDefinition not found" );
	}

	TDynArray< CName > types;
	TDynArray< CName > disabledTypes;

	// get knowable entity map pin types from scripts
	m_knowableEntityMapPinTypes.Clear();
	if ( !CallFunctionRef( this, CNAME( GetKnowableMapPinTypes ), types ) )
	{
		RED_HALT( "Script function CCommonMapManager::GetKnowableMapPinTypes not found" );
	}
	else
	{
		for ( Uint32 i = 0; i < types.Size(); i++ )
		{
			m_knowableEntityMapPinTypes.Insert( types[ i ] );
		}
	}

	// get discoverable entity map pin types from scripts
	m_discoverableEntityMapPinTypes.Clear();
	if ( !CallFunctionRef( this, CNAME( GetDiscoverableMapPinTypes ), types ) )
	{
		RED_HALT( "Script function CCommonMapManager::GetDiscoverableMapPinTypes not found" );
	}
	else
	{
		for ( Uint32 i = 0; i < types.Size(); i++ )
		{
			m_discoverableEntityMapPinTypes.Insert( types[ i ] );
		}
	}

	// get disableable entity map pins types from scripts
	m_disableableEntityMapPinTypes.Clear();
	if ( !CallFunctionRef( this, CNAME( GetDisableableMapPinTypes ), types, disabledTypes ) )
	{
		RED_HALT( "Script function CCommonMapManager::GetDisableableMapPinTypes not found" );
	}
	else
	{
		RED_ASSERT( types.Size() == disabledTypes.Size() );

		for ( Uint32 i = 0; i < types.Size(); i++ )
		{
			m_disableableEntityMapPinTypes[ types[ i ] ] = disabledTypes[ i ];
		}
	}
}

void CCommonMapManager::LoadAreaMapPins()
{
	String filePath;

	m_areaMapPins.ClearFast();

	if ( !CallFunctionRef( this, CNAME( GetAreaMappinsFileName ), filePath ) )
	{
		return;
	}
	
	THandle< CAreaMapPinsResource > resource = LoadResource< CAreaMapPinsResource >( filePath.AsChar() );
	if ( resource )
	{
		m_areaMapPins = resource->GetMapPinsInfo();
#ifndef NO_SECOND_SCREEN
		TDynArray< SAreaMapPinInfo >::const_iterator areaMapPinIterator = m_areaMapPins.Begin();
		SCSecondScreenManager::GetInstance().ClearFileNameToAreaMapping();
		while (areaMapPinIterator != m_areaMapPins.End() )
		{
			SCSecondScreenManager::GetInstance().SetFileNameToAreaMapping( areaMapPinIterator->m_worldPath, areaMapPinIterator->m_areaType );
			++areaMapPinIterator;
		}
#endif
	}
}

void CCommonMapManager::AddAreaMapPin( Int32 areaType, Int32 areaPinX, Int32 areaPinY, const String& worldPath, const CName& requiredChunk, const CName& localisationName , const CName& localisationDescription )
{
	SAreaMapPinInfo areaMapPinInfo;
	areaMapPinInfo.m_areaType	= areaType;
	areaMapPinInfo.m_position.X	= (Float)areaPinX;
	areaMapPinInfo.m_position.Y	= (Float)areaPinY;
	areaMapPinInfo.m_position.Z	= 0.0f;
	areaMapPinInfo.m_worldPath	= worldPath;
	areaMapPinInfo.m_requiredChunk	= requiredChunk;
	areaMapPinInfo.m_localisationName = localisationName;
	areaMapPinInfo.m_localisationDescription = localisationDescription;
	m_areaMapPins.PushBack( areaMapPinInfo );
}

void CCommonMapManager::RemAreaMapPin( Int32 areaType )
{
	TDynArray< SAreaMapPinInfo >::iterator end = m_areaMapPins.End();
	for( TDynArray< SAreaMapPinInfo >::iterator iter = m_areaMapPins.Begin(); iter != end; ++iter )
	{
		if( iter->m_areaType == areaType )
		{
			m_areaMapPins.Erase( iter );
			return;
		}
	}
}

void CCommonMapManager::AddEntityMapPins( const TDynArray< SEntityMapPinInfo >& entityMapPins )
{
	// dummy
	TDynArray< THandle< CEntity > > entities;
#ifdef ENTITY_MAPPIN_STATS
	THashMap< CName, Uint32 > stats;
#endif

	for ( Uint32 i = 0; i < entityMapPins.Size(); i++ )
	{
		const SEntityMapPinInfo& pin = entityMapPins[ i ];

#ifdef ENTITY_MAPPIN_STATS
		Uint32* value = stats.FindPtr( pin.m_entityType );
		if ( value )
		{
			(*value)++;
		}
		else
		{
			stats.Insert( pin.m_entityType, 1 );
		}
#endif

		// entity name + position should be unique
		String uniqueString = String::Printf( TXT("%s%f%f%f"), pin.m_entityName.AsChar(), pin.m_entityPosition.X, pin.m_entityPosition.Y, pin.m_entityPosition.Z );
		Int32 id = static_cast< Int32 >( ::GetHash< String >( uniqueString ) );
				 
		RED_LOG( RED_LOG_CHANNEL( CommonMapManager ), TXT("%d %s %s [%s] %.2f %.2f %.2f"), id, pin.m_entityName.AsChar(), pin.m_entityType.AsChar(), pin.m_entityTags.ToString().AsChar(), pin.m_entityPosition.X, pin.m_entityPosition.Y, pin.m_entityPosition.Z );
		CName groupTag = CName::NONE;
		if ( pin.m_entityType == CNAME( HubExit ) )
		{
			continue;
		}
		else if ( pin.m_entityType == CNAME( RoadSign ) ||
			      pin.m_entityType == CNAME( Harbor ) )
		{
			groupTag = pin.m_fastTravelGroupName;
		}

		Bool isDiscovered = true;
		if ( IsDiscoverableMapPinType( pin.m_entityType ) )
		{
			isDiscovered = IsEntityMapPinDiscovered( pin.m_entityName );
		}
		Bool isKnown = IsEntityMapPinKnown( pin.m_entityName );
		Bool isDisabled = IsEntityMapPinDisabled( pin.m_entityName );
		CName visibleType = pin.m_entityType;
		if ( IsKnowableMapPinType( pin.m_entityType ) || IsDisableableMapPinType( pin.m_entityType ) )
		{
			visibleType = GetMapPinVisibleType( pin.m_entityType, isKnown, isDiscovered, isDisabled );
		}

		AddMapPin( id, pin.m_entityPosition, entities, pin.m_entityName, pin.m_entityCustomNameId, groupTag, pin.m_entityType, visibleType, pin.m_entityRadius, CGUID::ZERO, false, isDiscovered, isKnown, isDisabled );
		if ( pin.m_alternateVersion )
		{
			SetMapPinAlternateVersion( id, pin.m_alternateVersion );
		}

		if ( IsNoticeboardType( pin.m_entityType ) )
		{
			InvalidateStaticMapPin( pin.m_entityName );
		}
	}

#ifdef ENTITY_MAPPIN_STATS
	RED_LOG( RED_LOG_CHANNEL( CommonMapManager ), TXT( "Entity mappins for %s" ), world->GetDepotPath().AsChar() );
	for ( THashMap< CName, Uint32 >::iterator itStat = stats.Begin(); itStat != stats.End(); ++itStat )
	{
		RED_LOG( RED_LOG_CHANNEL( CommonMapManager ), TXT( "%s %d" ), itStat->m_first.AsChar(), itStat->m_second );
	}
#endif
}

void CCommonMapManager::AddMapPin( const CEntity* entity, const CName& tag, Uint32 customNameId, const CName& extraTag, const CName& type, const CName& visibleType, float radius, const CGUID& guid, Bool isDynamic, Bool isDiscovered )
{
	if ( isDynamic && !entity )
	{
		RED_HALT( "CCommonMapManager::AddMapPin(): no entity for dynamic map pin" );
		return;
	}

	// guids are not unique on consoles!
	Int32 id = static_cast< Int32 >( ::GetHash( reinterpret_cast< Uint64 >( entity ) ) );
	AddMapPin( id, entity, tag, customNameId, extraTag, type, visibleType, radius, guid, isDynamic, isDiscovered );
}

void CCommonMapManager::AddMapPin( Int32 id, const CEntity* entity, const CName& tag, Uint32 customNameId, const CName& extraTag, const CName& type, const CName& visibleType, float radius, const CGUID& guid, Bool isDynamic, Bool isDiscovered )
{
	if ( isDynamic && !entity )
	{
		RED_HALT( "CCommonMapManager::AddMapPin(): no entity for dynamic map pin" );
		return;
	}

	const Vector& position = entity->GetWorldPositionRef();
	TDynArray< THandle< CEntity > > entities;

	entities.PushBackUnique( THandle< CEntity >( entity ) );

	AddMapPin( id, position, entities, tag, customNameId, extraTag, type, visibleType, radius, guid, isDynamic, isDiscovered, false, false );
}

void CCommonMapManager::AddMapPin( Int32 id, const Vector& position, const TDynArray< THandle< CEntity > > &entities, const CName& tag, Uint32 customNameId, const CName& extraTag, const CName& type, const CName& visibleType, float radius, const CGUID& guid, Bool isDynamic, Bool isDiscovered, Bool isKnown, Bool isDisabled )
{
	ASSERT( !m_mapPins.KeyExist( id ) );

	SCommonMapPinInstance pin;
	pin.m_id			= id;
	pin.m_tag			= tag;
	pin.m_customNameId	= customNameId;
	pin.m_extraTag		= extraTag;
	pin.m_type			= type;
	pin.m_visibleType	= visibleType;
	pin.m_position		= position;
	pin.m_radius		= radius;
	pin.m_visibleRadius	= CCommonMapManager::GetVisibleRadius( pin.m_visibleType, pin.m_radius );
	pin.m_guid			= guid;
	pin.m_isDynamic		= isDynamic;
	pin.m_isKnown		= isKnown;
	pin.m_isDiscovered	= isDiscovered;
	pin.m_isDisabled	= isDisabled;
	pin.m_entities		= entities;
	pin.m_canBePointedByArrow = CanPinBePointedByArrow( pin.m_type );
	pin.m_canBeAddedToMinimap = CanPinBeAddedToMinimap( pin.m_type );

	m_mapPins.Insert( pin.m_id, pin );
}

void CCommonMapManager::UpdateMapPin( SCommonMapPinInstance* instance, const CName& tag, Uint32 customNameId, const CName& extraTag, const CName& type, float radius, const CGUID& guid, Bool isDynamic, Bool isDiscovered )
{
	instance->m_tag				= tag;
	instance->m_customNameId	= customNameId;
	instance->m_extraTag		= extraTag;
	instance->m_type			= type;
	instance->m_visibleType		= type;
	instance->m_radius			= radius;
	instance->m_visibleRadius	= GetVisibleRadius( instance->m_visibleType, instance->m_radius );
	instance->m_guid			= guid;
	instance->m_isDynamic		= isDynamic;
	instance->m_isDiscovered	= isDiscovered;
	instance->m_isHighlightable	= false;
	instance->m_isHighlighted	= false;
	instance->m_canBePointedByArrow = CanPinBePointedByArrow( instance->m_type );

	if ( instance->m_isAddedToMinimap )
	{
		UpdateMapPinOnMinimap( *instance );
	}

#ifndef NO_SECOND_SCREEN
	instance->UpdateOnSecondScreen();
#endif // NO_SECOND_SCREEN	
}

void CCommonMapManager::DeleteMapPin( const CEntity* entity )
{
	if ( !entity )
	{
		return;
	}
	// guids are not unique on consoles!
	Int32 id = static_cast< Int32 >( ::GetHash( reinterpret_cast< Uint64 >( entity ) ) );
	DeleteMapPin( id );
}

void CCommonMapManager::DeleteMapPin( Int32 id )
{
	ASSERT( m_mapPins.KeyExist( id ) );

	SCommonMapPinInstance* pinToDelete = m_mapPins.FindPtr( id );
	if ( pinToDelete && pinToDelete->m_isAddedToMinimap )
	{
		DeleteMapPinOnMinimap( *pinToDelete );
	}
	m_mapPins.Erase( id );
}

void CCommonMapManager::SetMapPinAlternateVersion( Int32 id, Int32 alternateVersion )
{
	SCommonMapPinInstance* pinToUpdate = m_mapPins.FindPtr( id );
	if ( pinToUpdate )
	{
		pinToUpdate->m_alternateVersion = alternateVersion;
	}
}

void CCommonMapManager::SetMapPinHighlightable( const CName& tag )
{
	m_highlightableMapPins.PushBackUnique( tag );
}

void CCommonMapManager::SetAllMapPinsUnhighlightable()
{
	m_highlightableMapPins.ClearFast();

	// additionally clear update interval
	m_highlightingUpdateInterval = 0;
}

void CCommonMapManager::UpdateHighlightableMapPins()
{
	{
		PC_SCOPE_PIX( HighlightableMappins );

		THashMap< Int32, SCommonMapPinInstance >::iterator itEnd = m_mapPins.End();
		for ( THashMap< Int32, SCommonMapPinInstance >::iterator itPin = m_mapPins.Begin(); itPin != itEnd; ++itPin )
		{
			SCommonMapPinInstance& pin = itPin->m_second;
			if ( IsQuestPinType( pin.m_type ) )
			{
				pin.m_isHighlightable = m_highlightableMapPins.Exist( pin.m_tag );
			}
			else
			{
				pin.m_isHighlightable = false;
			}
		}
	}

	// update highlighted map pins
	const CEntity* player = GGame->GetPlayerEntity();
	if ( player )
	{
		Float questDistance = -1;
		Float userDistance = -1;

		{
			PC_SCOPE_PIX( HighlightMappins );

			// find closest mappin to player
			THashMap< Int32, SCommonMapPinInstance >::iterator closestPin = m_mapPins.End();
			float closestDistanceSqr = -1;
			const Vector& playerPosition = player->GetWorldPosition();

			THashMap< Int32, SCommonMapPinInstance >::iterator itEnd = m_mapPins.End();
			for ( THashMap< Int32, SCommonMapPinInstance >::iterator itPin = m_mapPins.Begin(); itPin != itEnd; ++itPin )
			{
				const SCommonMapPinInstance& pin = itPin->m_second;
				if ( pin.m_isHighlightable )
				{
					float distanceSqr = playerPosition.DistanceSquaredTo2D( pin.m_position );
					if ( closestDistanceSqr == -1 || closestDistanceSqr > distanceSqr )
					{
						closestPin = itPin;
						closestDistanceSqr = distanceSqr;
					}
				}
			}

			m_highlightedMapPin = INVALID_MAPPIN_ID;

			// highlight the closest mappin & unhighlight the rest
			itEnd = m_mapPins.End();
			for ( THashMap< Int32, SCommonMapPinInstance >::iterator itPin = m_mapPins.Begin(); itPin != itEnd; ++itPin )
			{
				SCommonMapPinInstance& pin = itPin->m_second;
				if ( itPin == closestPin )
				{
					m_highlightedMapPin = itPin->m_first;

					// that's a pin to highlight
					if ( !pin.m_isHighlighted )
					{
						pin.m_isHighlighted = true;
						if ( pin.m_isAddedToMinimap )
						{
							HighlightMapPinOnMinimap( pin );
						}
					}
				}
				else
				{
					// the rest should unhighlighted
					if ( pin.m_isHighlighted )
					{
						pin.m_isHighlighted = false;
						if ( pin.m_isAddedToMinimap )
						{
							HighlightMapPinOnMinimap( pin );
						}
					}
				}
			}

			if ( closestDistanceSqr > 0 )
			{
				questDistance = Red::Math::MSqrt( closestDistanceSqr );
			}
			// only first user map pin
			if ( !m_userMapPins.Empty() )
			{
				for ( Uint32 i = 0; i < m_userMapPins.Size(); ++i )
				{
					// find first user pin waypoint in current world
					if ( m_userMapPins[ i ].m_area == m_currentArea && m_userMapPins[ i ].m_type == WAYPOINT_USER_MAP_PIN_TYPE )
					{
						userDistance = playerPosition.DistanceTo2D( m_userMapPins[ i ].m_position );
						break;
					}
				}
			}
		}

		{
			PC_SCOPE_PIX( UpdateDistanceToHighlightedMapPinOnMinimap );
			
			UpdateDistanceToHighlightedMapPinOnMinimap( questDistance, userDistance );
		}
	}
}

void CCommonMapManager::FindMapPinInstancesByTag( const CName& tag, TDynArray< SCommonMapPinInstance* >& instances )
{
	instances.ClearFast();

	THashMap< Int32, SCommonMapPinInstance >::iterator itEnd = m_mapPins.End();
	for ( THashMap< Int32, SCommonMapPinInstance >::iterator itPin = m_mapPins.Begin(); itPin != itEnd; ++itPin )
	{
		SCommonMapPinInstance& pin = itPin->m_second;
		if ( pin.m_tag == tag )
		{
			instances.PushBack( &pin );
		}
	}
}

Bool CCommonMapManager::IsKnowableMapPinType( const CName& type )
{
	return m_knowableEntityMapPinTypes.Exist( type );
}

Bool CCommonMapManager::IsDiscoverableMapPinType( const CName& type )
{
	return m_discoverableEntityMapPinTypes.Exist( type );
}

Bool CCommonMapManager::IsDisableableMapPinType( const CName& type )
{
	return m_disableableEntityMapPinTypes.KeyExist( type );
}

void CCommonMapManager::InvalidateStaticMapPin( const CName& entityName )
{
	TDynArray< SCommonMapPinInstance* > instances;
	FindMapPinInstancesByTag( entityName, instances );
	for ( Uint32 i = 0; i < instances.Size(); i++ )
	{
		instances[ i ]->m_invalidated = true;
	}
	if ( instances.Size() )
	{
		m_invalidatedEntityMapPins = true;
		ScheduleUpdateStaticMapPins();
	}
}

void CCommonMapManager::InvalidateClosestFastTravelPoint()
{
	m_isValidClosestFastTravelPoint = false;
}

const Vector& CCommonMapManager::GetClosestFastTravelPoint()
{
	const Uint32 UPDATE_FAST_TRAVEL_POINT_DISTANCE = 50;

	const CPlayer* player = GCommonGame->GetPlayer();
	if ( !player )
	{
		return Vector::ZEROS;
	}
	const Vector& playerPos = player->GetWorldPositionRef();

	// if it's valid make sure the player is not too far away from last checked position
	if ( m_isValidClosestFastTravelPoint )
	{
		// and invalidate position if player moved some distance away
		if ( m_closestFastTravelPointLastPlayerPos.DistanceSquaredTo2D( playerPos ) > UPDATE_FAST_TRAVEL_POINT_DISTANCE * UPDATE_FAST_TRAVEL_POINT_DISTANCE )
		{
			m_isValidClosestFastTravelPoint = false;
		}
	}

	if ( !m_isValidClosestFastTravelPoint )
	{
		m_closestFastTravelPointPos = Vector::ZEROS;

		const CCachedWorldData* cachedData = GetCurrentWorldCachedData();
		if ( cachedData )
		{
			const TDynArray< SEntityMapPinInfo >& entityMapPins = cachedData->GetEntityMapPins();
			Float closestDistanceSqr = -1;
			for ( Uint32 i = 0; i < entityMapPins.Size(); ++i )
			{
				const SEntityMapPinInfo& info = entityMapPins[ i ];
				if ( info.m_entityType == CNAME( RoadSign ) )
				{
					if ( IsEntityMapPinDiscovered( info.m_entityName ) )
					{
						Float currentDistanceSqr = playerPos.DistanceSquaredTo2D( info.m_entityPosition );
						if ( ( closestDistanceSqr == -1 ) || closestDistanceSqr > currentDistanceSqr )
						{
							closestDistanceSqr = currentDistanceSqr;
							m_closestFastTravelPointPos = info.m_entityPosition;
							m_closestFastTravelPointLastPlayerPos = playerPos;
							m_isValidClosestFastTravelPoint = true;
						}
					}
				}
			}
		}
	}
	return m_closestFastTravelPointPos;
}

CName CCommonMapManager::GetMapPinVisibleType( const CName& type, Bool isKnown, Bool isDiscovered, Bool isDisabled )
{
	CName visibleType;
	if ( isDiscovered )
	{
		visibleType = type;
		if ( isDisabled )
		{
			CName* disabledType = m_disableableEntityMapPinTypes.FindPtr( type );
			if ( disabledType )
			{
				visibleType = *disabledType;
			}
		}
	}
	else if ( isKnown )
	{
		visibleType = CNAME( NotDiscoveredPOI );
	}
	return visibleType;
}

void CCommonMapManager::GetInteriorEntrances( const CR4InteriorAreaComponent* interiorComponent, TDynArray< CEntity* >& entrances )
{
	entrances.ClearFast();

	if ( !GCommonGame->GetActiveWorld() )
	{
		return;
	}
	CTagManager* tagManager = GCommonGame->GetActiveWorld()->GetTagManager();
	if ( !tagManager )
	{
		return;
	}

	const CName& entranceTag = interiorComponent->GetEntranceTag();
	if ( entranceTag == CName::NONE )
	{
		return;
	}

	tagManager->CollectTaggedEntities( entranceTag, entrances );
}

const CEntity* CCommonMapManager::GetClosestInteriorEntranceToPosition( const CR4InteriorAreaComponent* interiorComponent, const Vector3& entityPosition )
{
	TDynArray< CEntity* > entrances;
	GetInteriorEntrances( interiorComponent, entrances );

	if ( entrances.Size() == 0 )
	{
		return nullptr;
	}

	Int32 minIndex = -1;
	float minDistance = 0;

	for ( Int32 i = 0; i < entrances.SizeInt(); i++ )
	{
		float currDistance = entrances[ i ]->GetWorldPositionRef().DistanceSquaredTo2D( entityPosition );
		if ( minIndex == -1 || minDistance > currDistance )
		{
			minIndex = i;
			minDistance = currDistance;
		}
	}

	if ( minIndex > -1 )
	{
		return entrances[ minIndex ];
	}
	return NULL;
}

void CCommonMapManager::UpdateHintWaypoints( Float timeDelta )
{
	if ( !m_minimapManager || !m_minimapManager->IsMinimapAvailable() )
	{
		// until minimap is ready, there's no point to update anything
		return;
	}

	const SCommonMapPinInstance* highlightedMappin = GetHighlightedMapPin();
	if ( !highlightedMappin )
	{
		m_mapTracking->ClearTracking();
		return;
	}

	CPlayer* player = GCommonGame->GetPlayer();
	if ( !player )
	{
		return;
	}
	// test if player is inside 
#ifdef MINIMAP_TRACKING_LIMIT_TO_SETTLEMENTS
	CR4Player* r4Player = static_cast< CR4Player* >( player );
	if ( !r4Player->IsInInterior() && !r4Player->IsInSettlement() )
	{
		m_mapTracking->ClearTracking();
		return;
	}
#endif
	//
	//	// get start and target positions
	Vector startPos = player->GetWorldPosition();
	Vector targetPos = highlightedMappin->m_position;
	Float targetRadius = highlightedMappin->m_radius;

	m_mapTracking->UpdateTracking( startPos, targetPos, targetRadius );
}

void CCommonMapManager::ShowHintWaypoints( Bool show )
{
	m_showHintWaypoints = show;
	if ( !m_showHintWaypoints )
	{
		m_mapTracking->ClearTracking();
	}
}

Bool CCommonMapManager::GetHintWaypointTargetPosition( const Vector& playerPos, Vector& targetPos )
{
	if ( m_highlightedMapPin == INVALID_MAPPIN_ID )
	{
		return false;
	}
	auto itFind = m_mapPins.Find( m_highlightedMapPin );
	if ( itFind == m_mapPins.End() )
	{
		return false;
		
	}
	ASSERT( itFind->m_second.m_isHighlighted );
	targetPos = itFind->m_second.m_position;
	return true;
}

const SCommonMapPinInstance* CCommonMapManager::GetHighlightedMapPin()
{
	if ( m_highlightedMapPin == INVALID_MAPPIN_ID )
	{
		return nullptr;
	}
	auto itFind = m_mapPins.Find( m_highlightedMapPin );
	if ( itFind == m_mapPins.End() )
	{
		return nullptr;

	}
	ASSERT( itFind->m_second.m_isHighlighted );
	return &itFind->m_second;
}

void CCommonMapManager::FindWaypoints( const Vector& startPos, const Vector& targetPos, Float targetRadius )
{

}

void CCommonMapManager::UpdateMapPaths()
{
	m_currMapPathIds.ClearFast();

	for ( THashMap< CName, SMapPathDefinition >::iterator itDef = m_discoveredPaths.Begin(); itDef != m_discoveredPaths.End(); ++itDef )
	{
		const SMapPathDefinition& mapPathDefinition = itDef->m_second;

		PlaceMapPath( mapPathDefinition.m_tag, mapPathDefinition.m_lineWidth, mapPathDefinition.m_lineSegmentLength, mapPathDefinition.m_color );
	}

	m_currMapPathIds.Sort();

	DeleteOldMapPaths();

	m_prevMapPathIds.ResizeFast( m_currMapPathIds.Size() );
	Red::System::MemoryCopy( m_prevMapPathIds.TypedData(), m_currMapPathIds.TypedData(), m_currMapPathIds.DataSize() );
}

void CCommonMapManager::PlaceMapPath( const CName& tag, Float lineWidth, Float segmentLength, const Color& color )
{
	if ( tag == CName::NONE )
	{
		return;
	}

	CWorld* world = GGame->GetActiveWorld();
	if ( !world )
	{
		return;
	}

	CTagManager* tagManager = world->GetTagManager();
	if ( !tagManager )
	{
		return;
	}

	CEntity* entity = tagManager->GetTaggedEntity( tag );
	if ( entity )
	{
		// guids are not unique on consoles!
		Int32 id = static_cast< Int32 >( ::GetHash( reinterpret_cast< Uint64 >( entity ) ) );

		m_currMapPathIds.PushBack( id );
		if ( !m_prevMapPathIds.Exist( id ) )
		{
			lineWidth = Clamp< Float >( lineWidth, 0.1f, 50 );

			if ( segmentLength == 0 )
			{
				segmentLength = 5;
			}
			segmentLength = Clamp< Float >( segmentLength, 2, 10 );

			const Vector& entityPos = entity->GetWorldPositionRef();

			TDynArray< Vector > splinePoints;
			Int32 finalColor = 0;
			for ( ComponentIterator< CPathComponent > it( entity ); it; ++it )
			{
				CPathComponent* pathComponent = (*it);
				SMultiCurve& curve = pathComponent->GetCurve();

				if ( color == Color::CLEAR )
				{
					finalColor = curve.GetColor().ToUint32();
				}
				else
				{
					finalColor  = color.ToUint32();
				}
				finalColor = ( finalColor & 0xFF00FF00 )
				         | ( ( finalColor & 0x00FF0000 ) >> 16 )
				         | ( ( finalColor & 0x000000FF ) << 16 );

				Float totalLength = curve.CalculateLength();
				for ( Float length = 0; length < totalLength; length += segmentLength )
				{
					Vector point( Vector::ZEROS );
					curve.GetAbsolutePosition( length / totalLength, point );
					splinePoints.PushBack( point - entityPos );
				}
				if ( !splinePoints.Empty() )
				{
					// calculate ending point and if it's further than 0.1 m, add it to points as well
					Vector point( Vector::ZEROS );
					curve.GetAbsolutePosition( 1.0f, point );
					if ( point.DistanceSquaredTo2D( splinePoints[ splinePoints.Size() - 1] ) > 0.1 * 0.1 )
					{
						splinePoints.PushBack( point - entityPos );
					}
				}

				break;
			}

			if ( !splinePoints.Empty() )
			{
				SMapPathInstance instance;
				instance.m_id				= id;
				instance.m_position			= entity->GetWorldPositionRef();
				instance.m_splinePoints		= splinePoints;
				instance.m_color			= finalColor;
				instance.m_lineWidth		= lineWidth;
				instance.m_isAddedToMinimap	= false;

				m_mapPaths[ id ] = instance;
			}
		}
	}
}

void CCommonMapManager::DeleteOldMapPaths()
{
	TDynArray< Int32 > mapPathIds;

	Uint32 ci = 0;
	for ( Uint32 pi = 0; pi < m_prevMapPathIds.Size(); )
	{
		Int32 prevHash = m_prevMapPathIds[ pi ];
		if ( ci >= m_currMapPathIds.Size() )
		{
			// id from prev ids is not in current ids, delete prev one
			mapPathIds.PushBack( prevHash );
			//DeleteMapPin( prevHash );
			pi++;
		}
		else
		{
			Int32 currHash = m_currMapPathIds[ ci ];
			if ( currHash < prevHash )
			{
				// there's just new current id
				ci++;
			}
			else if ( currHash > prevHash )
			{
				// there's no curr id for corresponding prev id, delete
				mapPathIds.PushBack( prevHash );
				//DeleteMapPin( prevHash );
				pi++;
			}
			else // do nothing, they are equal so both out there
			{
				pi++;
				ci++;
			}
		}
	}

	if ( mapPathIds.Size() )
	{
		for ( Uint32 i = 0; i < mapPathIds.Size(); ++i )
		{
			ASSERT( m_mapPaths.KeyExist( mapPathIds[ i ] ) );
			m_mapPaths.Erase( mapPathIds[ i ] );
		}		

		CallFunction( this, CNAME( DeleteMapPathsFromMinimap ), mapPathIds );
	}
}

Bool CCommonMapManager::IsWorldAvailable( Int32 area )
{
	for ( Uint32 i = 0; i < m_areaMapPins.Size(); ++i )
	{
		if ( m_areaMapPins[ i ].m_areaType == area )
		{
			return GContentManager->IsContentAvailable( m_areaMapPins[ i ].m_requiredChunk );
		}
	}
	return true;
}

CName CCommonMapManager::GetWorldContentTag( Int32 area )
{
	for ( Uint32 i = 0; i < m_areaMapPins.Size(); ++i )
	{
		if ( m_areaMapPins[ i ].m_areaType == area )
		{
			return m_areaMapPins[ i ].m_requiredChunk;
		}
	}

	return CNAME( launch0 );
}

Uint32 CCommonMapManager::GetWorldPercentCompleted( Int32 area )
{
	for ( Uint32 i = 0; i < m_areaMapPins.Size(); ++i )
	{
		if ( m_areaMapPins[ i ].m_areaType == area )
		{
			return GContentManager->GetPercentCompleted( m_areaMapPins[ i ].m_requiredChunk );
		}
	}
	return 0;
}

Uint32 CCommonMapManager::GetPinRenderingPriority( const CName& type )
{
	if (	type == CNAME( Player ) ||
		type == CNAME( Enemy ) ||
		type == CNAME( EnemyDead ) )
	{
		return 0;
	}
	else if (	type == CNAME( StoryQuest ) ||
		type == CNAME( ChapterQuest ) ||
		type == CNAME( SideQuest ) ||
		type == CNAME( MonsterQuest ) ||
		type == CNAME( TreasureQuest ) ||
		type == CNAME( QuestAvailable ) ||
		type == CNAME( QuestReturn ) )
	{
		return 2;
	}
	return 1;
}

Bool CCommonMapManager::IsNoticeboardType( const CName& type )
{
	return type == CNAME( NoticeBoard ) || type == CNAME( NoticeBoardFull );
}

Bool CCommonMapManager::IsQuestPinType( const CName& type )
{
	for ( Uint32 i = 0; i < sizeof( m_questName ) / sizeof( m_questName[ 0 ] ); ++i )
	{
		if ( m_questName[ i ] == type )
		{
			return true;
		}
	}
	return false;
}

Bool CCommonMapManager::IsUserPinType( const CName& type )
{
	for ( Uint32 i = 0; i < sizeof( m_userPinName ) / sizeof( m_userPinName[ 0 ] ); ++i )
	{
		if ( m_userPinName[ i ] == type )
		{
			return true;
		}
	}
	return false;
}

Bool CCommonMapManager::IsEnemyType( const CName& type )
{
	return type == CNAME( Enemy );
}

Bool CCommonMapManager::GetEntityMappinsResourcePath( const String& worldPath, String& resourcePath, Uint32 expansionPackIndex )
{
	return GetResourcePath( worldPath, resourcePath, TXT("w2w"), TXT("w2em"), expansionPackIndex );
}

Bool CCommonMapManager::GetQuestMappinsResourcePath( const String& worldPath, String& resourcePath, Uint32 expansionPackIndex )
{
	return GetResourcePath( worldPath, resourcePath, TXT("w2w"), TXT("w2qm"), expansionPackIndex );
}

Vector CCommonMapManager::GetCenterPosition( const TDynArray< CEntity* >& entities )
{
	Vector centerPosition( Vector::ZEROS );
	Uint32 count = 0;

	for ( Uint32 i = 0; i < entities.Size(); i++ )
	{
		const CEntity* entity = entities[ i ];
		if ( entity )
		{
			centerPosition += entity->GetWorldPositionRef();
			count++;
		}
	}
	if ( count > 0 )
	{
		return centerPosition.Div4( ( Float )count );
	}
	return Vector::ZEROS;
}

Bool CCommonMapManager::GetEntityMappinsResourceFile( const String& worldPath, String& resourceFile, Uint32 expansionPackIndex )
{
	String resourcePath;
	if ( GetEntityMappinsResourcePath( worldPath, resourcePath, expansionPackIndex ) )
	{
		return GetFileFromPath( resourcePath, resourceFile, expansionPackIndex );
	}
	return false;
}

Bool CCommonMapManager::GetQuestMappinsResourceFile( const String& worldPath, String& resourceFile, Uint32 expansionPackIndex )
{
	String resourcePath;
	if ( GetQuestMappinsResourcePath( worldPath, resourcePath, expansionPackIndex ) )
	{
		return GetFileFromPath( resourcePath, resourceFile, expansionPackIndex );
	}
	return false;
}

Bool CCommonMapManager::GetResourcePath( const String& worldPath, String& resourcePath, const String& extensionToStrip, const String& extensionToAdd, Uint32 expansionPackIndex )
{
	size_t index = 0;
	if ( worldPath.EndsWith( extensionToStrip ) &&
		worldPath.FindSubstring( TXT( "\\" ), index, true ) )
	{
		String filename = worldPath.RightString( worldPath.GetLength() - index );

		if ( filename.FindSubstring( extensionToStrip, index, true ) )
		{
			resourcePath.ClearFast();
			if ( expansionPackIndex == 1 )
			{
				resourcePath += String::Printf( TXT( "dlc\\ep%d\\data\\"), expansionPackIndex );
			}
			else if ( expansionPackIndex == 2 )
			{
				resourcePath += TXT( "dlc\\bob\\data\\");
			}
			resourcePath += TXT( "game\\hub_pins" );
			resourcePath += filename.LeftString( index ) + extensionToAdd;
			return true;
		}
	}
	return false;
};

Bool CCommonMapManager::GetFileFromPath( const String& resourcePath, String& resourceFile, Uint32 expansionPackIndex )
{
	size_t index = 0;
	if ( resourcePath.FindSubstring( TXT("\\"), index, true ) )
	{
		resourceFile = resourcePath.RightString( resourcePath.GetLength() - index - 1 );
		return true;
	}
	return false;
}

const TDynArray< SEntityMapPinInfo >* CCommonMapManager::GetEntityMapPins( const String& worldPath )
{
	const CCachedWorldData* cachedData = GetWorldCachedData( worldPath );
	if ( cachedData )
	{
		return &cachedData->GetEntityMapPins();
	}
	return nullptr;
}

#ifndef NO_EDITOR
void CCommonMapManager::CollectMapPinsFromJournal( THashMap< CName, SQuestMapPinInfo >& mapPins )
{
	mapPins.ClearFast();

	TDynArray< String > filenames;
	GDepot->FindResourcesByExtension( TXT("journal"), filenames, true, true );

	Uint32 questCount = 0;
	Uint32 phasesCount = 0;
	Uint32 objectivesCount = 0;
	Uint32 mapPinCount = 0;
	for ( Uint32 i = 0; i < filenames.Size(); ++i )
	{
		THandle< CJournalResource > res = Cast< CJournalResource >( GDepot->LoadResource( filenames[ i ] ) );
		if ( !res )
		{
			continue;
		}
		const CJournalQuest* quest = Cast< CJournalQuest >( res->Get() );
		if ( !quest )
		{
			continue;
		}

		questCount++;

		//RED_LOG( RED_LOG_CHANNEL( asdf ), TXT("q %ls"), quest->GetName().AsChar() );

		TDynArray< CObject* > questPhases;
		TDynArray< CObject* > questObjectives;
		TDynArray< CObject* > questMapPins;

		quest->GetChildren( questPhases );
		for ( Uint32 j = 0; j < questPhases.Size(); ++j )
		{
			const CJournalQuestPhase* questPhase = Cast< CJournalQuestPhase >( questPhases[ j ] );
			if ( !questPhase )
			{
				continue;
			}
			phasesCount++;
			//RED_LOG( RED_LOG_CHANNEL( asdf ), TXT(" p %ls"), questPhase->GetName().AsChar() );

			questObjectives.ClearFast();
			questPhase->GetChildren( questObjectives );
			for ( Uint32 k = 0; k < questObjectives.Size(); ++k )
			{
				const CJournalQuestObjective* questObjective = Cast< CJournalQuestObjective >( questObjectives[ k ] );
				if ( !questObjective )
				{
					continue;
				}
				objectivesCount++;
				//RED_LOG( RED_LOG_CHANNEL( asdf ), TXT("  o %ls"), questObjective->GetName().AsChar() );

				questMapPins.ClearFast();
				questObjective->GetChildren( questMapPins );
				for ( Uint32 l = 0; l < questMapPins.Size(); ++l )
				{
					const CJournalQuestMapPin* questMapPin = Cast< CJournalQuestMapPin >( questMapPins[ l ] );
					if ( !questMapPin )
					{
						continue;
					}

					mapPinCount++;
					//RED_LOG( RED_LOG_CHANNEL( asdf ), TXT("   p %ls"), questMapPin->GetName().AsChar() );

					const CName& mapPinTag	= questMapPin->GetMapPinID();
					CName mapPinType		= CCommonMapManager::GetQuestNameFromType( questMapPin->GetType(), quest->GetType() );
					Float mapPinRadius		= questMapPin->GetRadius();

					if ( mapPinTag != CName::NONE )
					{
						if ( !mapPins.KeyExist( mapPinTag ) )
						{
							mapPins[ mapPinTag ] = SQuestMapPinInfo( mapPinTag, mapPinType, mapPinRadius );
						}
					}
				}
			}
		}
	}
	//RED_LOG( RED_LOG_CHANNEL( asdf ), TXT("Q %d P %d O %d P %d TP %s"), questCount, phasesCount, objectivesCount, mapPinCount, mapPinTags.Size() );
	//for ( THashSet< CName >::iterator it = mapPinTags.Begin(); it != mapPinTags.End(); ++it )
	//{
	//	RED_LOG( RED_LOG_CHANNEL( asdf ), TXT("[%s]"), (*it).AsChar() );
	//}
}
#endif //NO_EDITOR

void CCommonMapManager::funcInitializeMinimapManager( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CR4HudModule >, minimapModule, nullptr );
	FINISH_PARAMETERS;

	if ( m_minimapManager )
	{
		m_minimapManager->Initialize( minimapModule );
	}
}

void CCommonMapManager::funcSetHintWaypointParameters( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, removalDistance, 1.f );
	GET_PARAMETER( Float, placingDistance, 1.f );
	GET_PARAMETER( Float, refreshInterval, 1.f );
	GET_PARAMETER( Float, pathfindTolerance, 1.f );
	GET_PARAMETER( Int32, maxCount, 1 );
	FINISH_PARAMETERS;

	m_mapTracking->SetParameters(
		removalDistance,
		placingDistance,
		refreshInterval,
		pathfindTolerance,
		maxCount
		);

}

void CCommonMapManager::funcOnChangedMinimapRadius( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, minimapRadius, 1.f );
	GET_PARAMETER( Float, zoom, 1.f );
	FINISH_PARAMETERS;

	if ( m_minimapManager )
	{
		m_minimapManager->OnChangedMinimapRadius( minimapRadius );
	}
	m_mapTracking->OnChangedMapZoom( zoom );
}

void CCommonMapManager::funcIsFastTravellingEnabled( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( m_isFastTravellingEnabled );
}

void CCommonMapManager::funcEnableFastTravelling( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, enable, true );
	FINISH_PARAMETERS;

	m_isFastTravellingEnabled = enable;
}

void CCommonMapManager::funcIsEntityMapPinKnown( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, tag, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_BOOL( IsEntityMapPinKnown( tag ) );
}

void CCommonMapManager::funcSetEntityMapPinKnown( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, tag, CName::NONE );
	GET_PARAMETER_OPT( Bool, set, true );
	FINISH_PARAMETERS;

	SetEntityMapPinKnown( tag, set );
}

void CCommonMapManager::funcIsEntityMapPinDiscovered( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, tag, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_BOOL( IsEntityMapPinDiscovered( tag ) );
}

void CCommonMapManager::funcSetEntityMapPinDiscovered( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, tag, CName::NONE );
	GET_PARAMETER_OPT( Bool, set, true );
	FINISH_PARAMETERS;

	SetEntityMapPinDiscovered( tag, set );
}

void CCommonMapManager::funcIsEntityMapPinDisabled( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, tag, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_BOOL( IsEntityMapPinDisabled( tag ) );
}

void CCommonMapManager::funcSetEntityMapPinDisabled( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, tag, CName::NONE );
	GET_PARAMETER_OPT( Bool, set, true );
	FINISH_PARAMETERS;

	SetEntityMapPinDisabled( tag, set );
}

void CCommonMapManager::funcIsQuestPinType( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, type, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_BOOL( IsQuestPinType( type ) );
}

void CCommonMapManager::funcIsUserPinType( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, type, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_BOOL( IsUserPinType( type ) );
}

void CCommonMapManager::funcGetUserPinNames( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< CName >, names, TDynArray< CName >() );
	FINISH_PARAMETERS;
	
	names.ClearFast();
	for ( Uint32 i = 0; i < sizeof( m_userPinName ) / sizeof( m_userPinName[ 0 ] ); ++i )
	{
		names.PushBack( m_userPinName[ i ] );
	}
}

void CCommonMapManager::funcShowKnownEntities( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Bool, show, true );
	FINISH_PARAMETERS;

	m_showKnownEntities = show;
}

void CCommonMapManager::funcCanShowKnownEntities( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( m_showKnownEntities );
}

void CCommonMapManager::funcShowDisabledEntities( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Bool, show, true );
	FINISH_PARAMETERS;

	m_showDisabledEntities = show;
}

void CCommonMapManager::funcCanShowDisabledEntities( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( m_showDisabledEntities );
}

void CCommonMapManager::funcShowFocusClues( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Bool, show, true );
	FINISH_PARAMETERS;

	m_showFocusClues = show;
}

void CCommonMapManager::funcShowHintWaypoints( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Bool, show, true );
	FINISH_PARAMETERS;

	ShowHintWaypoints( show );
}

void CCommonMapManager::funcAddQuestLootContainer( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CEntity >, container, nullptr );
	FINISH_PARAMETERS;

	/*
	CEntity* entity = container.Get();
	if ( !entity )
	{
		return;
	}
	m_questLootEntities.PushBackUnique( container );

	AddMapPin( entity, CNAME( QuestLoot ), CName::NONE, CNAME( QuestLoot ), 0, CGUID::ZERO, false, true );
	*/
}

void CCommonMapManager::funcDeleteQuestLootContainer( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CEntity >, container, nullptr );
	FINISH_PARAMETERS;

	/*
	if ( m_questLootEntities.Exist( container ) )
	{
		m_questLootEntities.Remove( container );
		DeleteMapPin( container.Get() );
	}
	*/
}

void CCommonMapManager::funcCacheMapPins( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	CCachedWorldData* cachedData = GetCurrentWorldCachedData();
	if ( cachedData )
	{
		THashMap< CName, SQuestMapPinInfo >& cachedQuestMapPins = cachedData->GetCachedQuestMapPins();
		CacheActiveQuestMapPins( cachedQuestMapPins );

		THashSet< CName >& cachedNoticeboardMapPins = cachedData->GetCachedNoticeboardMapPins();
		CacheNoticeboardMapPins( cachedNoticeboardMapPins );

		TDynArray< Vector >& cachedBoatMapPins = cachedData->GetCachedBoatMapPins();
		CacheBoatMapPins( cachedBoatMapPins );
	}
}

void CCommonMapManager::funcGetMapPinInstances( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, worldPath, String::EMPTY );
	FINISH_PARAMETERS;

	if ( result )
	{
		TDynArray< SCommonMapPinInstance > & retVal = *(TDynArray< SCommonMapPinInstance >*) result;

		retVal.ClearFast();

		CWorld* world = GCommonGame->GetActiveWorld();
		if ( world )
		{
			const String& currWorldPath = world->GetDepotPath();
			if ( currWorldPath == worldPath )
			{
				// current world
				THashMap< Int32, SCommonMapPinInstance >::iterator itEnd = m_mapPins.End();
				for ( THashMap< Int32, SCommonMapPinInstance >::iterator itPin = m_mapPins.Begin(); itPin != itEnd; ++itPin )
				{
					const SCommonMapPinInstance& pin = itPin->m_second;
					retVal.PushBack( pin );
				}
			}
			else
			{
				// different world
				const CCachedWorldData* cachedData = GetWorldCachedData( worldPath );
				if ( cachedData )
				{
					// entity
					Bool isVisited = cachedData->IsVisited();
					const TDynArray< SEntityMapPinInfo>& entityMapPins = cachedData->GetEntityMapPins();
					const THashSet< CName >& noticeboards = cachedData->GetCachedNoticeboardMapPins();
					for ( Uint32 i = 0; i < entityMapPins.Size(); i++ )
					{
						const SEntityMapPinInfo& info = entityMapPins[ i ];

						SCommonMapPinInstance pin;

						pin.m_tag			= info.m_entityName;
						pin.m_customNameId	= info.m_entityCustomNameId;
						pin.m_extraTag		= info.m_fastTravelGroupName;
						pin.m_type			= info.m_entityType;
						pin.m_visibleType	= info.m_entityType;
						pin.m_position		= info.m_entityPosition;
						pin.m_isKnown		= IsEntityMapPinKnown( pin.m_tag );
						if ( IsDiscoverableMapPinType( pin.m_type ) )
						{
							pin.m_isDiscovered = IsEntityMapPinDiscovered( pin.m_tag );
						}
						else
						{
							pin.m_isDiscovered = true;
						}
						pin.m_isDisabled	= IsEntityMapPinDisabled( pin.m_tag );
						if ( IsKnowableMapPinType( pin.m_type ) || IsDisableableMapPinType( pin.m_type ) )
						{
							pin.m_visibleType = GetMapPinVisibleType( pin.m_type, pin.m_isKnown, pin.m_isDiscovered, pin.m_isDisabled );
						}

						if ( pin.m_type == CNAME( NoticeBoard ) )
						{
							if ( isVisited )
							{
								if ( noticeboards.Exist( pin.m_tag ) )
								{
									pin.m_visibleType = CNAME( NoticeBoardFull );
								}
							}
							else
							{
								pin.m_visibleType = CNAME( NoticeBoardFull );
							}
						}

						pin.m_radius		= info.m_entityRadius;
						pin.m_visibleRadius	= CCommonMapManager::GetVisibleRadius( pin.m_visibleType, pin.m_radius );

						retVal.PushBack( pin );
					}

					// shopkeepers
					const THashMap< CName, CCachedShopkeeperData >& shopkeeperData = cachedData->GetShopkeeperData();
					THashMap< CName, CCachedShopkeeperData >::const_iterator itEnd = shopkeeperData.End();
					for ( THashMap< CName, CCachedShopkeeperData >::const_iterator it = shopkeeperData.Begin(); it != itEnd; ++it )
					{
						const CCachedShopkeeperData& data = it->m_second;
						if ( !data.CanMapPinBeShown() )
						{
							continue;
						}
						SCommonMapPinInstance pin;

						pin.m_tag			= data.m_tag;
						pin.m_type			= data.m_type;
						pin.m_visibleType	= data.m_type;
						pin.m_radius		= 0;
						pin.m_visibleRadius	= 0;
						pin.m_guid			= CGUID::ZERO;
						pin.m_isDiscovered	= true;
						pin.m_position		= data.GetPosition();

						retVal.PushBack( pin );
					}

					// boats
					const TDynArray< Vector >& boatData = cachedData->GetCachedBoatMapPins();
					for ( Uint32 i = 0; i < boatData.Size(); ++i )
					{
						SCommonMapPinInstance pin;

						pin.m_type			= CNAME( Boat );
						pin.m_visibleType	= CNAME( Boat );
						pin.m_radius		= 0;
						pin.m_visibleRadius	= 0;
						pin.m_guid			= CGUID::ZERO;
						pin.m_isDiscovered	= true;
						pin.m_isHighlighted = false;
						pin.m_position		= boatData[ i ];

						retVal.PushBack( pin );
					}

					// quest map pins
					const THashMap< CName, SQuestMapPinInfo >& staticQuestMapPins = cachedData->GetStaticQuestMapPins();
					const THashMap< CName, SQuestMapPinInfo >& cachedQuestMapPins = cachedData->GetCachedQuestMapPins();

					CGUID highlightedQuestObjectiveGUID = CGUID::ZERO;
					CWitcherJournalManager* journalManager = GCommonGame->GetSystem< CWitcherJournalManager >();
					if ( journalManager )
					{
						const CJournalQuestObjective* highlightedQuestObjective = journalManager->GetHighlightedObjective();
						if ( highlightedQuestObjective )
						{
							highlightedQuestObjectiveGUID = highlightedQuestObjective->GetGUID();
						}
					}

					Int32 area = ConvertFakeAreaToRealArea( GetAreaFromWorldPath( worldPath ) );
					for ( Uint32 i = 0; i < m_questMapPinData.Size(); i++ )
					{
						const SQuestPinData& mapPinData = m_questMapPinData[ i ];
						if ( mapPinData.m_world == area )
						{
							const SQuestMapPinInfo* pinInfo = nullptr;
							
							pinInfo = staticQuestMapPins.FindPtr( mapPinData.m_tag );
							if ( !pinInfo )
							{
								pinInfo = cachedQuestMapPins.FindPtr( mapPinData.m_tag );
							}

							if ( pinInfo )
							{
								SCommonMapPinInstance pin;

								
								pin.m_tag			= pinInfo->m_tag;
								pin.m_type			= pinInfo->m_type;
								pin.m_visibleType	= pinInfo->m_type;
								pin.m_radius		= mapPinData.m_radius;
								pin.m_visibleRadius	= mapPinData.m_radius;
								pin.m_guid			= mapPinData.m_objectiveGUID;
								pin.m_isDiscovered	= true;
								pin.m_isHighlighted = ( highlightedQuestObjectiveGUID == mapPinData.m_objectiveGUID );
								for ( Uint32 j = 0; j < pinInfo->m_positions.Size(); ++j )
								{
									pin.m_position	= pinInfo->m_positions[ j ];
									retVal.PushBack( pin );
								}
							}
						}
					}
				}
			}
		}
	}
}

void CCommonMapManager::funcGetMapPinTypeByTag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, mapPinTag, CName::NONE );
	FINISH_PARAMETERS;

	if ( mapPinTag == CName::NONE )
	{
		RETURN_NAME( CName::NONE );
		return;
	}

	THashMap< Int32, SCommonMapPinInstance >::iterator itEnd = m_mapPins.End();
	for ( THashMap< Int32, SCommonMapPinInstance >::iterator itPin = m_mapPins.Begin(); itPin != itEnd; ++itPin )
	{
		if ( itPin->m_second.m_tag == mapPinTag )
		{
			RETURN_NAME( itPin->m_second.m_type );
			return;
		}
	}

	RETURN_NAME( CName::NONE );
}

void CCommonMapManager::funcGetHighlightedMapPinTag( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	String tag;
	const SCommonMapPinInstance* highlightedMappin = GetHighlightedMapPin();
	if ( highlightedMappin )
	{
		RETURN_NAME( highlightedMappin->m_tag );
		return;
	}

	RETURN_NAME( CName::NONE );
}

void CCommonMapManager::funcTogglePathsInfo( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Bool, showPathsInfo, true );
	FINISH_PARAMETERS;

	m_showPathsInfo = showPathsInfo;
}

void CCommonMapManager::funcToggleQuestAgentsInfo( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Bool, showQuestAgentsInfo, true );
	FINISH_PARAMETERS;

	m_showQuestAgentsInfo = showQuestAgentsInfo;
}

void CCommonMapManager::funcToggleShopkeepersInfo( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Bool, showShopkeepersInfo, true );
	FINISH_PARAMETERS;

	m_showShopkeepersInfo = showShopkeepersInfo;
}

void CCommonMapManager::funcToggleInteriorInfo( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Bool, showInteriorInfo, true );
	FINISH_PARAMETERS;

	m_showInteriorInfo = showInteriorInfo;
}

void CCommonMapManager::funcToggleUserPinsInfo( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Bool, showUserPinsInfo, true );
	FINISH_PARAMETERS;

	m_showUserPinsInfo = showUserPinsInfo;
}

void CCommonMapManager::funcTogglePinsInfo( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Int32, showPinsInfoFlags, -1 );
	FINISH_PARAMETERS;

	m_showPinsInfoFlags = static_cast< Uint32 >( showPinsInfoFlags );
}

void CCommonMapManager::funcExportGlobalMapPins( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

#ifndef NO_EDITOR
	ExportGlobalMapPins();
#endif //NO_EDITOR
}

void CCommonMapManager::funcExportEntityMapPins( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

#ifndef NO_EDITOR
	ExportEntityMapPins();
#endif //NO_EDITOR
}

void CCommonMapManager::funcGetAreaMapPins( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	if ( result )
	{
		TDynArray< SAreaMapPinInfo > & retVal = *(TDynArray< SAreaMapPinInfo >*) result;
		retVal = m_areaMapPins;
	}
}

void CCommonMapManager::funcGetEntityMapPins( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, worldPath, String::EMPTY );
	FINISH_PARAMETERS;

	if ( result )
	{
		TDynArray< SEntityMapPinInfo > & retVal = *(TDynArray< SEntityMapPinInfo >*) result;
		retVal.ClearFast();
		const CCachedWorldData* cachedData = GetWorldCachedData( worldPath );
		if ( cachedData )
		{
			retVal = cachedData->GetEntityMapPins();
		}
	}
}

void CCommonMapManager::funcUseMapPin( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, pinTag, CName::NONE );
	GET_PARAMETER( Bool, onStart, true );
	FINISH_PARAMETERS;

	RETURN_BOOL( false );

	if ( pinTag != CName::NONE )
	{
		CR4Game* game = Cast< CR4Game >( GCommonGame );
		if ( game )
		{
			const SUsedFastTravelEvent event( CNAME( StaticMapPinUsed ), pinTag, onStart );
			game->OnUsedFastTravelEvent( event );
			RETURN_BOOL( true );
		}
	}
}

void CCommonMapManager::funcUseInteriorsForQuestMapPins( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, use, false );
	FINISH_PARAMETERS;

	m_useInteriorsForQuestMapPins = use;
}

void CCommonMapManager::funcEnableShopkeeper( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, tag, CName::NONE );
	GET_PARAMETER( Bool, enable, true );
	FINISH_PARAMETERS;

	EnableShopkeeper( tag, enable );
}

void CCommonMapManager::funcEnableMapPath( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, tag, CName::NONE );
	GET_PARAMETER( Bool, enable, true );
	GET_PARAMETER( Float, lineWidth, 1.0f );
	GET_PARAMETER( Float, lineSegmentLength, 5.0f );
	GET_PARAMETER( Color, color, Color::CLEAR );
	FINISH_PARAMETERS;

	if ( tag == CName::NONE )
	{
		return;
	}

	Bool update = false;
	if ( enable )
	{
		if ( !m_discoveredPaths.KeyExist( tag ) )
		{
			m_discoveredPaths[ tag ] = SMapPathDefinition( tag, lineWidth, lineSegmentLength, color );
			update = true;
		}
	}
	else
	{
		if ( m_discoveredPaths.KeyExist( tag ) )
		{
			m_discoveredPaths.Erase( tag );
			update = true;
		}
	}

	if ( update )
	{
		m_pathsUpdateInterval = 0;
	}
}

void CCommonMapManager::funcEnableDynamicMappin( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, tag, CName::NONE );
	GET_PARAMETER( Bool, enable, true );
	GET_PARAMETER( CName, type, CName::NONE );
	GET_PARAMETER_OPT( Bool, useAgents, false );
	FINISH_PARAMETERS;

	if ( !useAgents )
	{
		if ( enable )
		{
			if ( type != CName::NONE )
			{
				if ( !m_customEntityMapPins.KeyExist( tag ) )
				{
					m_customEntityMapPins[ tag ] = type;
				}
			}
		}
		else
		{
			m_customEntityMapPins.Erase( tag );
		}
	}
	else
	{
		if ( enable )
		{
			if ( type != CName::NONE )
			{
				if ( !m_customAgentMapPins.KeyExist( tag ) )
				{
					m_customAgentMapPins[ tag ] = type;
				}
			}
		}
		else
		{
			m_customAgentMapPins.Erase( tag );
		}
	}

	ScheduleUpdateDynamicMapPins();
}

void CCommonMapManager::funcInvalidateStaticMapPin( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, entityName, CName::NONE );
	FINISH_PARAMETERS;

	InvalidateStaticMapPin( entityName );
}

void CCommonMapManager::funcToggleUserMapPin( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, area, 0 );
	GET_PARAMETER( Vector, mapPinPosition, Vector::ZEROS );
	GET_PARAMETER( Int32, type, 0 );
	GET_PARAMETER( Bool, fromSelectionPanel, false );
	GET_PARAMETER_REF( Int32, idToAdd, 0 );
	GET_PARAMETER_REF( Int32, idToRemove, 0 );
	FINISH_PARAMETERS;

	// check if mappin is supposed to be removed or a new mappin is supposed to be placed at position of the existing one
	for ( Uint32 i = 0; i < m_userMapPins.Size(); ++i )
	{
		if ( m_userMapPins[ i ].m_area == area )
		{
			if ( mapPinPosition.DistanceSquaredTo2D( m_userMapPins[ i ].m_position ) < 0.1f * 0.1f )
			{
				// turn off user map pin
				idToRemove = m_userMapPins[ i ].m_id;
				m_userMapPins.RemoveAt( i );
				if ( fromSelectionPanel )
				{
					break;
				}
				else
				{
					// if not from selection panel, return because player just wants to turn it off
					RETURN_BOOL( true );
					return;
				}
			}
		}
	}


	if ( type == WAYPOINT_USER_MAP_PIN_TYPE )
	{
		// we're going to place waypoint mappin, remove existing one if exists
		for ( Uint32 i = 0; i < m_userMapPins.Size(); ++i )
		{
			if ( m_userMapPins[ i ].m_type == WAYPOINT_USER_MAP_PIN_TYPE )
			{
				idToRemove = m_userMapPins[ i ].m_id;
				m_userMapPins.RemoveAt( i );
				break;
			}
		}
	}
	else
	{
		// check if limit was hit
		if ( GetUserMapPinCount( false ) >= m_otherUserPinLimit )
		{
			RETURN_BOOL( false );
			return;
		}
	}
	m_userMapPins.PushBack( CUserMapPinData( area, mapPinPosition, type ) );
	idToAdd = m_userMapPins[ m_userMapPins.Size() - 1 ].m_id;

	RETURN_BOOL( true );
}

void CCommonMapManager::funcGetUserMapPinLimits( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Uint32, waypointUserPinLimit, 0 );
	GET_PARAMETER_REF( Uint32, otherUserPinLimit, 0 );
	FINISH_PARAMETERS;

	// allow to specify limits from scripts, but clamp them to some sane numbers so modders will not be able to go beyond them
	if ( waypointUserPinLimit >= WAYPOINT_USER_PIN_LIMIT )
	{
		waypointUserPinLimit = WAYPOINT_USER_PIN_LIMIT;
	}
	if ( otherUserPinLimit >= OTHER_USER_PIN_LIMIT )
	{
		otherUserPinLimit = OTHER_USER_PIN_LIMIT;
	}
	m_waypointUserPinLimit = waypointUserPinLimit;
	m_otherUserPinLimit    = otherUserPinLimit;
}

void CCommonMapManager::funcGetUserMapPinCount( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_INT( m_userMapPins.SizeInt() );
}

void CCommonMapManager::funcGetUserMapPinByIndex( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, index, 0 );
	GET_PARAMETER_REF( Int32, id, 0 );
	GET_PARAMETER_REF( Int32, area, -1 );
	GET_PARAMETER_REF( Float, mapPinX, 0 );
	GET_PARAMETER_REF( Float, mapPinY, 0 );
	GET_PARAMETER_REF( Int32, type, 0 );
	FINISH_PARAMETERS;

	if ( index < 0 || index >= m_userMapPins.SizeInt() )
	{
		RETURN_BOOL( false );
		return;
	}
	id		= m_userMapPins[ index ].m_id;
	area	= m_userMapPins[ index ].m_area;
	mapPinX	= m_userMapPins[ index ].m_position.X;
	mapPinY	= m_userMapPins[ index ].m_position.Y;
	type	= m_userMapPins[ index ].m_type;

	RETURN_BOOL( true );
}

void CCommonMapManager::funcGetUserMapPinIndexById( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, id, 0 );
	FINISH_PARAMETERS;

	for ( Uint32 i = 0; i < m_userMapPins.Size(); ++i )
	{
		if ( m_userMapPins[ i ].m_id == id )
		{
			RETURN_INT( i );
			return;
		}
	}

	RETURN_INT( -1 );
}

void CCommonMapManager::funcGetIdOfFirstUser1MapPin( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Int32, id, 0 );
	FINISH_PARAMETERS;

	for ( Uint32 i = 0; i < m_userMapPins.Size(); ++i )
	{
		if ( m_userMapPins[ i ].m_type == WAYPOINT_USER_MAP_PIN_TYPE )
		{
			id = m_userMapPins[ i ].m_id;
			RETURN_BOOL( true );
			return;
		}
	}
	RETURN_BOOL( false );
}

void CCommonMapManager::funcGetCurrentArea( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_INT( m_currentArea );
}

void CCommonMapManager::funcNotifyPlayerEnteredBorder( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, interval, 1 );
	GET_PARAMETER( Vector, position, Vector::ZEROS );
	GET_PARAMETER( EulerAngles, rotation, EulerAngles::ZEROS );
	FINISH_PARAMETERS;
	
	if( !m_isInBorder )
	{
		m_isInBorder = true;
		m_borderInterval = interval;
		m_borderPlayerPosition = position;
		m_borderPlayerRotation = rotation;
	}
	
	m_borderCount++;
	
	RETURN_INT( m_borderCount );
}

void CCommonMapManager::funcNotifyPlayerExitedBorder( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	
	if( m_borderCount > 0 )
	{
		m_borderCount--;
	}
	
	if( m_borderCount == 0 )
	{
		m_isInBorder = false;
	}
	
	RETURN_INT( m_borderCount );
}

void CCommonMapManager::funcIsWorldAvailable( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, area, -1 );
	FINISH_PARAMETERS;

	RETURN_BOOL( IsWorldAvailable( area ) );
}

void CCommonMapManager::funcGetWorldContentTag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, area, -1 );
	FINISH_PARAMETERS;

	RETURN_NAME( GetWorldContentTag( area ) );
}

void CCommonMapManager::funcGetWorldPercentCompleted( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, area, -1 );
	FINISH_PARAMETERS;

	RETURN_INT( GetWorldPercentCompleted( area ) );
}

void CCommonMapManager::funcDisableMapPin( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, name, String::EMPTY );
	GET_PARAMETER( Bool, disable, false );
	FINISH_PARAMETERS;

	if ( name.Empty() )
	{
		return;
	}

	if ( disable )
	{
		if ( !m_disabledMapPins.Exist( name ) )
		{
			m_disabledMapPins.Insert( name );
			RETURN_BOOL( true );
			return;
		}
	}
	else
	{
		THashSet< String >::iterator it = m_disabledMapPins.Find( name );
		if ( it != m_disabledMapPins.End() )
		{
			m_disabledMapPins.Erase( it );
			RETURN_BOOL( true );
			return;
		}
	}
	RETURN_BOOL( false );
}

void CCommonMapManager::funcGetDisabledMapPins( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	if ( result )
	{
		TDynArray< String > & retVal = *(TDynArray< String >*) result;
		retVal.ClearFast();
		for ( THashSet< String >::const_iterator it = m_disabledMapPins.Begin(); it != m_disabledMapPins.End(); ++it )
		{
			retVal.PushBack( *it );
		}
	}
}

void CCommonMapManager::OnScriptReloaded()
{
	TBaseClass::OnScriptReloaded();
	CacheScriptedClasses();
	CacheScriptedFunctions();
}

void CCommonMapManager::CacheScriptedClasses()
{
	m_shopComponentClass = SRTTI::GetInstance().FindClass( CNAME( W3MerchantComponent ) );	RED_ASSERT( m_shopComponentClass );
	m_herbClass = SRTTI::GetInstance().FindClass( CNAME( W3Herb ) );						RED_ASSERT( m_herbClass );
	m_lootClass = SRTTI::GetInstance().FindClass( CNAME( W3ActorRemains ) );				RED_ASSERT( m_lootClass );
	m_clueClass = SRTTI::GetInstance().FindClass( CNAME( W3MonsterClue ) );					RED_ASSERT( m_clueClass );
}

void CCommonMapManager::CacheScriptedFunctions()
{
	m_getGameplayVisibilityFunction = nullptr;
	const CClass* actorClass = SRTTI::GetInstance().FindClass( CNAME( CActor ) );
	if ( actorClass )
	{
		m_getGameplayVisibilityFunction = actorClass->FindFunction( CNAME( GetGameplayVisibility ) );
	}
}

IMPLEMENT_ENGINE_CLASS( CCommonMapManager )
