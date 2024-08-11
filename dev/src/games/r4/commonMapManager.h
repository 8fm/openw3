/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/gameSystem.h"
#include "../../common/game/gameplayStorage.h"
#include "../../common/game/communitySystem.h"
#include "../../common/game/actionPoint.h"
#include "../../common/game/storyPhaseLocation.h"
#include "../../common/engine/gameTimeManager.h"
#include "../../common/engine/layerInfo.h"


#include "r4JournalManager.h"
#include "r4GameResource.h"

#include "EntityMappinsResource.h"
#include "questMappinsResource.h"
#include "areaMappinsResource.h"
#include "r4interiorAreaComponent.h"
#include "minimapManager.h"

#include "../../common/core/set.h"

//#define MINIMAP_TRACKING_LIMIT_TO_SETTLEMENTS

// pin types
RED_DECLARE_NAME( Enemy )
RED_DECLARE_NAME( EnemyDead )
RED_DECLARE_NAME( Horse )
RED_DECLARE_NAME( MonsterNest )
RED_DECLARE_NAME( PlaceOfPower )
RED_DECLARE_NAME( GenericFocus )
RED_DECLARE_NAME( Companion )
RED_DECLARE_NAME( Rift )
RED_DECLARE_NAME( Whetstone )
RED_DECLARE_NAME( Entrance )
RED_DECLARE_NAME( Herb )
RED_DECLARE_NAME( User1 )
RED_DECLARE_NAME( User2 )
RED_DECLARE_NAME( User3 )
RED_DECLARE_NAME( User4 )
RED_DECLARE_NAME( RoadSign )
RED_DECLARE_NAME( Harbor )
RED_DECLARE_NAME( HubExit )
RED_DECLARE_NAME( NoticeBoard )
RED_DECLARE_NAME( NoticeBoardFull )
RED_DECLARE_NAME( PlayerStash )
RED_DECLARE_NAME( QuestLoot )
RED_DECLARE_NAME( StoryQuest )
RED_DECLARE_NAME( ChapterQuest )
RED_DECLARE_NAME( SideQuest )
RED_DECLARE_NAME( MonsterQuest )
RED_DECLARE_NAME( TreasureQuest )
RED_DECLARE_NAME( QuestReturn )
RED_DECLARE_NAME( HorseRace )
RED_DECLARE_NAME( BoatRace )
RED_DECLARE_NAME( QuestBelgard )
RED_DECLARE_NAME( QuestCoronata )
RED_DECLARE_NAME( QuestVermentino )
RED_DECLARE_NAME( QuestAvailable )
RED_DECLARE_NAME( MagicLamp )
RED_DECLARE_NAME( PointOfInterestMappin )
RED_DECLARE_NAME( NotDiscoveredPOI )
RED_DECLARE_NAME( TreasureHuntMappin )
RED_DECLARE_NAME( TreasureHuntMappinDisabled )
RED_DECLARE_NAME( SpoilsOfWar )
RED_DECLARE_NAME( SpoilsOfWarDisabled )
RED_DECLARE_NAME( BanditCamp )
RED_DECLARE_NAME( BanditCampDisabled )
RED_DECLARE_NAME( BanditCampfire )
RED_DECLARE_NAME( BanditCampfireDisabled )
RED_DECLARE_NAME( BossAndTreasure )
RED_DECLARE_NAME( BossAndTreasureDisabled )
RED_DECLARE_NAME( Contraband )
RED_DECLARE_NAME( ContrabandDisabled )
RED_DECLARE_NAME( ContrabandShip )
RED_DECLARE_NAME( ContrabandShipDisabled )
RED_DECLARE_NAME( RescuingTown )
RED_DECLARE_NAME( RescuingTownDisabled )
RED_DECLARE_NAME( DungeonCrawl )
RED_DECLARE_NAME( DungeonCrawlDisabled )
RED_DECLARE_NAME( Hideout )
RED_DECLARE_NAME( HideoutDisabled )

class CR4InteriorAreaComponent;
class CR4MapTracking;

enum EDynamicMapPinUpdateStep
{
	DPUS_None,
	DPUS_Processing,
};

enum EMapPinGroupType
{
	MPGT_FastTravel		= 1,
	MPGT_Quest			= 2,
	MPGT_Dynamic		= 4,
	MPGT_Static			= 8,
	MPGT_Knowable		= 16,
	MPGT_All			= -1,
};

/////////////////////////////////////////////////////////////////////////

struct SQuestPinData
{
	String							m_name;
	CName							m_tag;
	float							m_radius;
	EJournalMapPinType				m_mapPinType;
	eQuestType						m_questType;
	CGUID							m_objectiveGUID;
	CGUID							m_mapPinGUID;
	Uint32							m_world;
	TDynArray< THandle< CEntity > >	m_entities;

public:
	SQuestPinData( const String& name, const CName& tag, float radius, EJournalMapPinType mapPinType, eQuestType questType, const CGUID& objectiveGUID, const CGUID& mapPinGUID, Uint32 world )
		: m_name( name )
		, m_tag( tag )
		, m_radius( radius )
		, m_mapPinType( mapPinType )
		, m_questType( questType )
		, m_objectiveGUID( objectiveGUID )
		, m_mapPinGUID( mapPinGUID )
		, m_world( world )
	{}

	Bool operator==( const SQuestPinData& other ) const
	{
		return m_tag == other.m_tag;
	}
};

/////////////////////////////////////////////////////////////////////////

struct SCustomMapPinDefinition
{
	DECLARE_RTTI_STRUCT( SCustomMapPinDefinition );

	CName	m_tag;
	CName	m_type;
};

BEGIN_CLASS_RTTI( SCustomMapPinDefinition );
PROPERTY( m_tag );
PROPERTY( m_type );
END_CLASS_RTTI();

/////////////////////////////////////////////////////////////////////////

struct SMapPathDefinition
{
	DECLARE_RTTI_STRUCT( SMapPathDefinition );

	CName	m_tag;
	Float	m_lineWidth;
	Float	m_lineSegmentLength;
	Color	m_color;

	SMapPathDefinition( const CName& tag = CName::NONE, Float lineWidth = 1, Float lineSegmentLength = 5, const Color& color = Color::WHITE )
		: m_tag( tag )
		, m_lineWidth( lineWidth )
		, m_lineSegmentLength( lineSegmentLength )
		, m_color( color )
	{}
};

BEGIN_CLASS_RTTI( SMapPathDefinition );
	PROPERTY( m_tag );
	PROPERTY( m_lineWidth );
	PROPERTY( m_lineSegmentLength );
	PROPERTY( m_color );
END_CLASS_RTTI();

/////////////////////////////////////////////////////////////////////////

struct SMapPathInstance
{
	DECLARE_RTTI_STRUCT( SMapPathInstance );

	Int32	m_id;
	Vector	m_position;
	TDynArray< Vector > m_splinePoints;
	Int32	m_color;
	Float	m_lineWidth;
	Bool	m_isAddedToMinimap;

	SMapPathInstance()
		: m_id( 0 )
		, m_position( Vector::ZEROS )
		, m_color( 0xFFFFFFFF )
		, m_lineWidth( 1 )
		, m_isAddedToMinimap( false )
	{}
};

BEGIN_CLASS_RTTI( SMapPathInstance );
	PROPERTY( m_id );
	PROPERTY( m_position )
	PROPERTY( m_splinePoints )
	PROPERTY( m_color )
	PROPERTY( m_lineWidth )
	PROPERTY( m_isAddedToMinimap )
END_CLASS_RTTI();

/////////////////////////////////////////////////////////////////////////

class CCachedShopkeeperData
{
public:
	Int32				m_id;
	CName				m_tag;
	CName				m_type;
	THandle< CEntity >	m_entity;
	Vector				m_position;
	Bool				m_enabled;
	Bool				m_initialized;
	Bool				m_entityExists;
	Bool				m_cacheable;

public:
	CCachedShopkeeperData( const CName& tag = CName::NONE, Int32 id = 0, const CName& type = CName::NONE, const CEntity* entity = nullptr, const Vector& position = Vector::ZEROS, Bool enable = true, Bool initialize = false )
		: m_id( id )
		, m_tag( tag )
		, m_type( type )
		, m_entity( entity )
		, m_position( position )
		, m_enabled( enable )
		, m_initialized( initialize )
		, m_entityExists( !!entity )
		, m_cacheable( true )
	{}

	RED_INLINE void SetEntityAndPosition( const CEntity* entity, const Vector& position )
	{
		m_id = static_cast< Int32 >( ::GetHash( reinterpret_cast< Uint64 >( entity ) ) );
		m_entity = entity;
		m_position = position;
		m_initialized = true;
		m_entityExists = true;
	}

	RED_INLINE Int32 GetId() const					{	return m_id;	}
	RED_INLINE const CEntity* GetEntity() const		{	return m_entity.Get();	}
	RED_INLINE const CName& GetTag() const			{	return m_tag;	}
	RED_INLINE const CName& GetType() const			{	return m_type;	}
	RED_INLINE void SetType( const CName& type)		{	m_type = type;	}
	RED_INLINE Bool IsCacheable() const				{	return m_cacheable;	}
	RED_INLINE void SetCacheable( Bool cacheable )	{	m_cacheable = cacheable;	}
	RED_INLINE const Vector& GetPosition() const	{	return m_position;	}
	RED_INLINE void SetPosition( const Vector& position )	{	m_position = position;	}
	RED_INLINE Bool GetEntityExists() const			{	return m_entityExists;	}
	RED_INLINE void SetEntityExists( Bool entityExists ) {	m_entityExists = entityExists;	}
	RED_INLINE Bool IsInitialzed() const			{	return m_initialized;	}
	RED_INLINE void SetInitialized( Bool initialized ) {	m_initialized = initialized;	}
	RED_INLINE Bool IsEnabled() const				{	return m_enabled;	}
	RED_INLINE void Enable( Bool enable )			{	m_enabled = enable;	}
	RED_INLINE Bool CanMapPinBeShown() const		{	return m_enabled && m_initialized;	}
};

/////////////////////////////////////////////////////////////////////////

class CCachedWorldData
{
private:
	String										m_worldName;
	Bool										m_visited;
	THashSet< CName >							m_entityMapPinsLoadedForDLC;
	THashSet< CName >							m_questMapPinsLoadedForDLC;
	TDynArray< SEntityMapPinInfo >				m_entityMapPins;
	THashMap< CName, SQuestMapPinInfo >			m_staticQuestMapPins;

	THashMap< CName, SQuestMapPinInfo >			m_cachedQuestMapPins;
	THashMap< CName, CCachedShopkeeperData >	m_cachedShopkeepers;
	THashSet< CName >							m_cachedNoticeboardMapPins;
	TDynArray< Vector >							m_cachedBoatMapPins;

public:
	CCachedWorldData();
	void Initialize( const String& worldName );
	void Uninitialize();
	RED_INLINE Bool IsInitialized() const { return !m_worldName.Empty(); }
	RED_INLINE void SetVisited() { m_visited = true; }
	RED_INLINE Bool IsVisited() const { return m_visited; }
	
	RED_INLINE const TDynArray< SEntityMapPinInfo >& GetEntityMapPins() const { return m_entityMapPins; }
	RED_INLINE const THashMap< CName, SQuestMapPinInfo >& GetStaticQuestMapPins() const { return m_staticQuestMapPins; }
	RED_INLINE       THashMap< CName, SQuestMapPinInfo >& GetStaticQuestMapPins()       { return m_staticQuestMapPins; }
	RED_INLINE const THashMap< CName, SQuestMapPinInfo >& GetCachedQuestMapPins() const { return m_cachedQuestMapPins; }
	RED_INLINE       THashMap< CName, SQuestMapPinInfo >& GetCachedQuestMapPins()       { return m_cachedQuestMapPins; }
	RED_INLINE const THashMap< CName, CCachedShopkeeperData >& GetShopkeeperData() const { return m_cachedShopkeepers; }
	RED_INLINE       THashMap< CName, CCachedShopkeeperData >& GetShopkeeperData()       { return m_cachedShopkeepers; }
	RED_INLINE const THashSet< CName >& GetCachedNoticeboardMapPins() const { return m_cachedNoticeboardMapPins; }
	RED_INLINE       THashSet< CName >& GetCachedNoticeboardMapPins()       { return m_cachedNoticeboardMapPins; }
	RED_INLINE const TDynArray< Vector >& GetCachedBoatMapPins() const { return m_cachedBoatMapPins; }
	RED_INLINE       TDynArray< Vector >& GetCachedBoatMapPins()       { return m_cachedBoatMapPins; }
	void EnableShopkeeper( const CName& tag, Bool enable );
	void UpdateShopkeeperData( const CNewNPC* entity, CScriptedComponent* component );
	void UpdateShopkeeperAgents();

	Bool AreEntityMapPinsLoadedForDLC( const CName& dlcId ) const;
	Bool AreQuestMapPinsLoadedForDLC( const CName& dlcId ) const;
	Bool LoadEntityMapPinsForDLC( const CName& dlcId, TDynArray< SEntityMapPinInfo > & mapPins );
	Bool LoadQuestMapPinsForDLC( const CName& dlcId, TDynArray< SQuestMapPinInfo > & mapPins );

	Bool OnSaveGame( IGameSaver* saver );
	Bool OnLoadGame( IGameLoader* loader );

private:
	Bool LoadEntityMapPins();
	Bool LoadQuestMapPins();

	void AddNewShopkeeperData( const CName& tag, Int32 id, const CName& type, const CNewNPC* entity, const Vector& position, Bool enable, Bool initialize );
};

/////////////////////////////////////////////////////////////////////////

class CUserMapPinData
{
public:
	Uint32		m_id;
	Int32		m_area;
	Vector2		m_position;
	Int32		m_type;
public:
	CUserMapPinData( Int32 area = -1, const Vector& position = Vector::ZEROS, Int32 type = 0 )
		: m_area( area )
		, m_position( position )
		, m_type( type )

	{
		m_id = ::GetHash( position );
	}
};

/////////////////////////////////////////////////////////////////////////

class CCommonMapManager : public IGameSystem, public IGameSaveSection
{
	DECLARE_ENGINE_CLASS( CCommonMapManager, IGameSystem, 0 )

private:
	static const Int32								INVALID_MAPPIN_ID			= 0xffffffff;

	static const Int32								WAYPOINT_USER_MAP_PIN_TYPE	= 0;
	static const Int32								WAYPOINT_USER_PIN_LIMIT		= 1;
	static const Int32								OTHER_USER_PIN_LIMIT		= 20;


	// persistent stuff, loaded in constructor
	TDynArray< SCustomMapPinDefinition >			m_customMapPinDefinition;
	TSet< CName >									m_knowableEntityMapPinTypes;
	TSet< CName >									m_discoverableEntityMapPinTypes;
	THashMap< CName, CName >						m_disableableEntityMapPinTypes;
	const CClass*									m_shopComponentClass;
	const CClass*									m_herbClass;
	const CClass*									m_lootClass;
	const CClass*									m_clueClass;
	const CFunction*								m_getGameplayVisibilityFunction;

	THashSet< String >								m_disabledMapPins;

	// general stuff
	Int32											m_currentArea;
	String											m_currentWorldPath;
	// quest map pins
    Bool											m_invalidatedQuestMapPinData;
	TDynArray< SQuestPinData >						m_questMapPinData;
	THashMap< TPair<CGUID, CGUID>, Bool >			m_questMapPinStates;
	// quest loot pins
	TDynArray< THandle< CEntity > >					m_questLootEntities;
	// user map pins
	TDynArray< CUserMapPinData >					m_userMapPins;
	Uint32											m_waypointUserPinLimit;
	Uint32											m_otherUserPinLimit;
	// area map pins
	TDynArray< SAreaMapPinInfo >					m_areaMapPins;
	// cached world info
	THashMap< String, CCachedWorldData >			m_cachedWorldDataMap;

	// set of worlds for which entity map pins have been already initialized
	THashSet< String >								m_entityMapPinsInitializedFor;

	// static map pins
	Bool											m_invalidatedEntityMapPins;
	// custom entity map pins
	THashMap< CName, CName >						m_customEntityMapPins;
	THashMap< CName, CName >						m_customAgentMapPins;
	// discoverable types, discovered & disabled map pin entities
	TSet< CName >									m_knownEntityMapPinTags;
	TSet< CName >									m_discoveredEntityMapPinTags;
	TSet< CName >									m_disabledEntityMapPinTags;
	TSet< CName >									m_discoveredAgentEntityTags;
	// paths
	THashMap< CName, SMapPathDefinition >			m_discoveredPaths;
	// interiors
	CInteriorsRegistry								m_interiorsRegistry;
	TDynArray< THandle< CR4InteriorAreaComponent > > m_playerInteriors;
	TDynArray< THandle< CGameplayEntity > >			m_boatEntities;

	// current map pin instances
	THashMap< Int32, SCommonMapPinInstance >		m_mapPins;
	TSortedArray< Int32 >							m_currDynamicMapPinIds;
	TSortedArray< Int32 >							m_prevDynamicMapPinIds;
	Int32											m_highlightedMapPin;
	// current map path instances
	THashMap< Int32, SMapPathInstance >				m_mapPaths;
	TSortedArray< Int32 >							m_currMapPathIds;
	TSortedArray< Int32 >							m_prevMapPathIds;

	// highlighted map pin tags
	TDynArray< CName >								m_highlightableMapPins;

	// border interval/position/rotation
	Uint32											m_borderCount;
	Float											m_borderInterval;
	Vector											m_borderPlayerPosition;
	EulerAngles										m_borderPlayerRotation;
	Bool											m_isInBorder;

	// hint waypoints
	Bool											m_mapTrackingEnabled;
	CR4MapTracking*									m_mapTracking;
	
	Vector											m_hintWaypointStartPos;
	Vector											m_hintWaypointTargetPos;
	TDynArray< Vector >								m_hintWaypoints;

	// minimap manager
	CMinimapManager*								m_minimapManager;

	// closest fast travel point
	Vector											m_closestFastTravelPointPos;
	Vector											m_closestFastTravelPointLastPlayerPos;

	// other stuff
	Float											m_pathsUpdateInterval;
	EDynamicMapPinUpdateStep						m_dynamicPinsUpdateStep;
	Float											m_dynamicPinsUpdateInterval;
	Float											m_dynamicPinsPositionUpdateInterval;
	Float											m_staticPinsUpdateInterval;
	Float											m_highlightingUpdateInterval;

	// arrays used when looking for entities, placed here to avoid allocations
	TDynArray< TPointerWrapper< CGameplayEntity > > m_nearbyEntitiesWrappers;
	TDynArray< THandle< CGameplayEntity > >			m_nearbyEntitiesHandles;
	TDynArray< CEntity* >							m_entities;
	TDynArray< SAgentStub* >						m_agentStubs;

	Uint32											m_showPinsInfoFlags;

	Bool											m_isValidClosestFastTravelPoint;
	Bool											m_showHintWaypoints;
	Bool											m_showKnownEntities;
	Bool											m_showDisabledEntities;
	Bool											m_showFocusClues;

	Bool											m_showPathsInfo;
	Bool											m_showQuestAgentsInfo;
	Bool											m_showShopkeepersInfo;
	Bool											m_showInteriorInfo;
	Bool											m_showUserPinsInfo;
	Bool											m_isFastTravellingEnabled;
	Bool											m_useInteriorsForQuestMapPins;

	static const Uint32 DYNAMIC_MAPPIN_UPDATE_COUNT;
	static const Uint32 MINIMAP_PINS_COUNT;
	static const float  ENEMY_SEARCH_RADIUS;
	static const float  HERB_SEARCH_RADIUS;
	static const float  MONSTER_CLUE_SEARCH_RADIUS;
	static const float  DYNAMIC_MAPPIN_VISIBLILITY_VERTICAL_THRESHOLD;
	static const float  MONSTER_CLUE_MAPPIN_VISIBLILITY_VERTICAL_THRESHOLD;
	static const float  PATHS_UPDATE_INTERVAL;
	static const float  DYNAMIC_PINS_UPDATE_INTERVAL;
	static const float  DYNAMIC_PINS_POSITION_UPDATE_INTERVAL;
	static const float  STATIC_PINS_UPDATE_INTERVAL;
	static const float  HIGHLIGHTING_UPDATE_INTERVAL;
	static const float  HINT_WAYPOINT_RADIUS;
	static const float  HINT_WAYPOINT_RADIUS_PERCENT_THRESHOLD;
	static const float  HINT_WAYPOINT_UPDATE_DISTANCE_THRESHOLD;

	static const Int32	ADDED_PINS_PER_FRAME_LIMIT;
	static const Int32	DELETED_PINS_PER_FRAME_LIMIT;

	static const CName	m_questName[];
	static const CName	m_userPinName[];

public:
	static const Uint32	EXPANSION_PACK_COUNT;

public:
	CCommonMapManager();
	virtual ~CCommonMapManager();
	
	virtual void Initialize() override;
	virtual void Shutdown() override;
	virtual void OnGameStart( const CGameInfo& gameInfo );
	virtual void OnGameEnd( const CGameInfo& gameInfo );
	virtual void OnWorldStart( const CGameInfo& gameInfo );
	virtual void OnWorldEnd( const CGameInfo& gameInfo );
	virtual Bool OnSaveGame( IGameSaver* saver );
			void OnLoadGame( IGameLoader* loader );
	virtual void Tick( Float timeDelta );
	virtual void OnGenerateDebugFragments( CRenderFrame* frame );

public:
	RED_INLINE CMinimapManager* GetMinimapManager() const { return m_minimapManager; }
	RED_INLINE Int32 GetCurrentArea() const { return m_currentArea; }
	Int32 GetCurrentJournalArea();
	void UpdateCurrentArea();
	Int32 GetAreaFromWorldPath( const String& worldPath ) const;
	const CName& GetCurrentLocalisationName() const;
	const CName& GetLocalisationNameFromWorldPath( const String& worldPath ) const;
	Int32 ConvertJournalAreaToRealArea( Int32 journalArea ) const;
	Int32 ConvertFakeAreaToRealArea( Int32 fakeArea ) const;

	Int32 FindInteriorContainingPoint( const Vector& position, CR4InteriorAreaComponent** foundInteriors, Int32 maxElements );
	CR4InteriorAreaComponent* GetInteriorFromPosition( const Vector& position );

	void InvalidateQuestMapPinData();
	void ScheduleUpdateDynamicMapPins();
	void ScheduleUpdateStaticMapPins();
	Bool IsJournalQuestMapPinEnabled( const CGUID& objectiveGUID, const CGUID& mapPinGUID );
	void EnableJournalQuestMapPin( const CGUID& objectiveGUID, const CGUID& mapPinGUID, Bool enabled );
	void DeleteJournalQuestMapPin( const CGUID& objectiveGUID, const CGUID& mapPinGUID );

	Bool IsEntityMapPinKnown( const CName& tag );
	void SetEntityMapPinKnown( const CName& tag, Bool set = true );
	Bool IsEntityMapPinDiscovered( const CName& tag );
	void SetEntityMapPinDiscovered( const CName& tag, Bool set = true );
	Bool IsEntityMapPinDisabled( const CName& tag );
	void SetEntityMapPinDisabled( const CName& tag, Bool set = true );

	Bool LoadEntityMapPinsForDLC( const CName& dlcId, const String& worldPath, const String& filename );
	Bool LoadQuestMapPinsForDLC( const CName& dlcId, const String& worldPath, const String& filename );
	void ExportGlobalMapPins();
	void ExportEntityMapPins( Uint32 expansionPackIndex = 0 );
	void ExportQuestMapPins( Uint32 expansionPackIndex = 0 );
	void UpdateMapPinEntities( Bool updateAndSave );

	void SetHighlightableMapPinsFromObjective( const CJournalQuestObjective* highlightedObjective );
	void ForceUpdateDynamicMapPins();

	const TDynArray< SEntityMapPinInfo >* GetEntityMapPins( const String& worldPath );

	void OnAttachedBoat( const CGameplayEntity* boatEntity );
	void OnDetachedBoat( const CGameplayEntity* boatEntity );

	void OnAttachedInterior( CR4InteriorAreaComponent* interiorComponent );
	void OnDetachedInterior( CR4InteriorAreaComponent* interiorComponent );
	void OnPlayerEnteredInterior( const CR4InteriorAreaComponent* interiorComponent );
	void OnPlayerExitedInterior( const CR4InteriorAreaComponent* interiorComponent );
	void OnPlayerChanged();

	void AddAreaMapPin( Int32 areaType, Int32 areaPinX, Int32 areaPinY, const String& worldPath, const CName& requiredChunk, const CName& localisationName , const CName& localisationDescription );
	void RemAreaMapPin( Int32 areaType );

protected:
	void CreateMinimapManager();
	void DeleteMinimapManager();

	void OnNotifyPlayerEnteredInterior( const CR4InteriorAreaComponent* interiorComponent );
	void OnNotifyPlayerExitedInterior();

	void AddMapPinToMinimap( SCommonMapPinInstance& pin );
	void MoveMapPinOnMinimap( SCommonMapPinInstance& pin );
	void HighlightMapPinOnMinimap( SCommonMapPinInstance& pin );
	void UpdateMapPinOnMinimap( SCommonMapPinInstance& pin );
	void DeleteMapPinOnMinimap( SCommonMapPinInstance& pin );
	void UpdateDistanceToHighlightedMapPinOnMinimap( Float questDistance, Float userDistance );
	void AddWaypointsToMinimap( const TDynArray< Vector >& waypoints );
	void DeleteWaypointsOnMinimap();

	CCachedWorldData* GetWorldCachedData( const String& path );
	CCachedWorldData* GetCurrentWorldCachedData();
	void EnableShopkeeper( const CName& tag, Bool enable );

	Uint32 GetUserMapPinCount( Bool onlyWaypoints );

	void UpdateQuestMapPinData();
	void CacheActiveQuestMapPins( THashMap< CName, SQuestMapPinInfo >& cachedQuestMapPins );
	void CacheNoticeboardMapPins( THashSet< CName >& cachedNoticeboardMapPins );
	void CacheBoatMapPins( TDynArray< Vector >& cachedBoatMapPins );
	void UpdateDynamicMapPins( Float timeDelta );
	void UpdateDynamicMapPins_Initialization();
	Bool UpdateDynamicMapPins_Processing( Bool processAll = false );
	void UpdateDynamicMapPins_Finalization();
	void UpdateDynamicMapPinsPositionsAndVisibility();
	Bool IsEntityVisible( const CEntity* entity ) const;
	Bool IsQuestMapPinEnabled( const CJournalQuestObjective* objective, const CJournalQuestMapPin* mapPin );
	Bool GetQuestMapPinPositions( const CName& tag, TDynArray< Vector >& positions );
	Bool GetQuestMapPinPositionFromEntities( const CName& tag, TDynArray< Vector >& positions );
	Bool GetQuestMapPinPositionFromCommunities( const CName& tag, TDynArray< Vector >& positions );
	Bool PlaceQuestMapPinsFromEntities( const SQuestPinData& mapPinData );
	Bool PlaceCustomMapPinsFromEntities( const SCustomMapPinDefinition& definition, const CGameplayEntity* entityToIgnore );
	void PlaceShopkeeperMapPins();
	void PlacePlayerCompanionMapPin( const CNewNPC* companion );
	void PlacePlayerHorseMapPin( const CGameplayEntity* entityToIgnore );
	void PlaceBoatMapPins( const CGameplayEntity* entityToIgnore );
	void PlaceUserMapPins();
	void PlaceQuestMapPinsFromCommunities( const SQuestPinData& mapPinData );
	void PlaceQuestMapPinFromFastTravelPoint( const SQuestPinData& mapPinData );
	void PlacePossibleQuestMapPinsFromCommunities( const CName& tag, const CName& type );
	Bool PlaceDynamicMapPinWithInteriors( const CEntity* entity,                 const CName& tag, const CName& type, float radius, const CGUID& guid, Bool isDynamic );
	Bool PlaceDynamicMapPinWithInteriors( const TDynArray< CEntity* >& entities, const CName& tag, const CName& type, float radius, const CGUID& guid, Bool isDynamic );
	Bool PlaceDynamicMapPinWithInteriors( const SAgentStub* agentStub,           const CName& tag, const CName& type, float radius, const CGUID& guid, Bool isDynamic );
	void PlaceDynamicMapPin(              const CEntity* entity,                 const CName& tag, const CName& type, float radius, const CGUID& guid, Bool isDynamic );
	void PlaceDynamicMapPin(              const TDynArray< CEntity* >& entities, const CName& tag, const CName& type, float radius, const CGUID& guid, Bool isDynamic );
	void PlaceDynamicMapPin(              const SAgentStub* agentStub,           const CName& tag, const CName& type, float radius, const CGUID& guid, Bool isDynamic );
	void PlaceDynamicMapPin(			  Int32 id, const Vector& position,		 const CName& tag, const CName& type, float radius, const CGUID& guid, Bool isDynamic );

	void DeleteOldDynamicMapPins();
	Uint32 GetMapPinTypePriority( const CName& type );

	void UpdateStaticMapPins();

	void LoadMapPinInfoFromScripts(); 
	void LoadAreaMapPins();
	void AddEntityMapPins( const TDynArray< SEntityMapPinInfo >& entityMapPins );

	void AddMapPin(										const CEntity* entity,								const CName& tag, Uint32 customNameId, const CName& extraTag, const CName& type, const CName& visibleType, float radius, const CGUID& guid, Bool isDynamic, Bool isDiscovered );
	void AddMapPin( Int32 id,							const CEntity* entity,								const CName& tag, Uint32 customNameId, const CName& extraTag, const CName& type, const CName& visibleType, float radius, const CGUID& guid, Bool isDynamic, Bool isDiscovered );
	void AddMapPin( Int32 id, const Vector& position,	const TDynArray< THandle< CEntity > > &entities,	const CName& tag, Uint32 customNameId, const CName& extraTag, const CName& type, const CName& visibleType, float radius, const CGUID& guid, Bool isDynamic, Bool isDiscovered, Bool isKnown, Bool isDisabled );
	void UpdateMapPin( SCommonMapPinInstance* instance,														const CName& tag, Uint32 customNameId, const CName& extraTag, const CName& type, float radius, const CGUID& guid, Bool isDynamic, Bool isDiscovered );
	void DeleteMapPin( const CEntity* entity );
	void DeleteMapPin( Int32 id );
	void SetMapPinAlternateVersion( Int32 id, Int32 alternateVersion );
	void SetMapPinHighlightable( const CName& tag );
	void SetAllMapPinsUnhighlightable();
	void UpdateHighlightableMapPins();
	void FindMapPinInstancesByTag( const CName& tag, TDynArray< SCommonMapPinInstance* >& instances );
	Bool IsKnowableMapPinType( const CName& type );
	Bool IsDiscoverableMapPinType( const CName& type );
	Bool IsDisableableMapPinType( const CName& type );
	void InvalidateStaticMapPin( const CName& entityName );
	void InvalidateClosestFastTravelPoint();
	const Vector& GetClosestFastTravelPoint();
	CName GetMapPinVisibleType( const CName& type, Bool isKnown, Bool isDiscovered, Bool isDisabled );
	
	void GetInteriorEntrances( const CR4InteriorAreaComponent* interiorComponent, TDynArray< CEntity* >& entrances );
	const CEntity* GetClosestInteriorEntranceToPosition( const CR4InteriorAreaComponent* interiorComponent, const Vector3& entityPosition );

	// waypoint
	void UpdateHintWaypoints( Float timeDelta );
	void ShowHintWaypoints( Bool show );
	Bool GetHintWaypointTargetPosition( const Vector& playerPos, Vector& targetPos );
	const SCommonMapPinInstance* GetHighlightedMapPin();
	void FindWaypoints( const Vector& startPos, const Vector& targetPos, Float targetRadius );

	// paths
	void UpdateMapPaths();
	void PlaceMapPath( const CName& tag, Float lineWidth, Float segmentLength, const Color& color );
	void DeleteOldMapPaths();

	// 
	Bool IsWorldAvailable( Int32 area );
	CName GetWorldContentTag( Int32 area );
	Uint32 GetWorldPercentCompleted( Int32 area );

	virtual void OnScriptReloaded() override;
	void CacheScriptedClasses();
	void CacheScriptedFunctions();

	// other
public:
	static Uint32 GetPinRenderingPriority( const CName& type );
	static Bool IsNoticeboardType( const CName& type );
	static Bool IsQuestPinType( const CName& type );
	static Bool IsUserPinType( const CName& type );
	static Bool IsEnemyType( const CName& type );
	static Bool GetEntityMappinsResourcePath( const String& worldPath, String& resourcePath, Uint32 expansionPackIndex );
	static Bool GetQuestMappinsResourcePath( const String& worldPath, String& resourcePath, Uint32 expansionPackIndex );

private:
	static Vector GetCenterPosition( const TDynArray< CEntity* >& entities );
	static Bool GetEntityMappinsResourceFile( const String& worldPath, String& resourceFile, Uint32 expansionPackIndex );
	static Bool GetQuestMappinsResourceFile( const String& worldPath, String& resourceFile, Uint32 expansionPackIndex );
	static Bool GetResourcePath( const String& worldPath, String& resourcePath, const String& extensionToStrip, const String& extensionToAdd, Uint32 expansionPackIndex );
	static Bool GetFileFromPath( const String& path, String& file, Uint32 expansionPackIndex );
	static void UpdateTags( CEntity* entity, const CName& tag );
	static const CName& GetQuestNameFromType( EJournalMapPinType mapPinType, eQuestType questType );
	static Float GetVisibleRadius( const CName& type, Float radius );
	static Bool CanPinBePointedByArrow( const CName& pinType );
	static Bool CanPinBeAddedToMinimap( const CName& pinType );
#ifndef NO_EDITOR
	static void CollectMapPinsFromJournal( THashMap< CName, SQuestMapPinInfo >& mapPins );
#endif //NO_EDITOR

private:
	void funcInitializeMinimapManager( CScriptStackFrame& stack, void* result );
	void funcSetHintWaypointParameters( CScriptStackFrame& stack, void* result );
	void funcOnChangedMinimapRadius( CScriptStackFrame& stack, void* result );
	void funcIsFastTravellingEnabled( CScriptStackFrame& stack, void* result );
	void funcEnableFastTravelling( CScriptStackFrame& stack, void* result );
	void funcIsEntityMapPinDiscovered( CScriptStackFrame& stack, void* result );
	void funcSetEntityMapPinDiscovered( CScriptStackFrame& stack, void* result );
	void funcIsEntityMapPinKnown( CScriptStackFrame& stack, void* result );
	void funcSetEntityMapPinKnown( CScriptStackFrame& stack, void* result );
	void funcIsEntityMapPinDisabled( CScriptStackFrame& stack, void* result );
	void funcSetEntityMapPinDisabled( CScriptStackFrame& stack, void* result );
	void funcIsQuestPinType( CScriptStackFrame& stack, void* result );
	void funcIsUserPinType( CScriptStackFrame& stack, void* result );
	void funcGetUserPinNames( CScriptStackFrame& stack, void* result );
	void funcShowKnownEntities( CScriptStackFrame& stack, void* result );
	void funcCanShowKnownEntities( CScriptStackFrame& stack, void* result );
	void funcShowDisabledEntities( CScriptStackFrame& stack, void* result );
	void funcCanShowDisabledEntities( CScriptStackFrame& stack, void* result );
	void funcShowFocusClues( CScriptStackFrame& stack, void* result );
	void funcShowHintWaypoints( CScriptStackFrame& stack, void* result );
	void funcAddQuestLootContainer( CScriptStackFrame& stack, void* result );
	void funcDeleteQuestLootContainer( CScriptStackFrame& stack, void* result );
	void funcCacheMapPins( CScriptStackFrame& stack, void* result );
	void funcGetMapPinInstances( CScriptStackFrame& stack, void* result );
	void funcGetMapPinTypeByTag( CScriptStackFrame& stack, void* result );
	void funcGetHighlightedMapPinTag( CScriptStackFrame& stack, void* result );
	void funcTogglePathsInfo( CScriptStackFrame& stack, void* result );
	void funcToggleQuestAgentsInfo( CScriptStackFrame& stack, void* result );
	void funcToggleShopkeepersInfo( CScriptStackFrame& stack, void* result );
	void funcToggleInteriorInfo( CScriptStackFrame& stack, void* result );
	void funcToggleUserPinsInfo( CScriptStackFrame& stack, void* result );
	void funcTogglePinsInfo( CScriptStackFrame& stack, void* result );
	void funcExportGlobalMapPins( CScriptStackFrame& stack, void* result );
	void funcExportEntityMapPins( CScriptStackFrame& stack, void* result );
	void funcGetAreaMapPins( CScriptStackFrame& stack, void* result );
	void funcGetEntityMapPins( CScriptStackFrame& stack, void* result );
	void funcUseMapPin( CScriptStackFrame& stack, void* result );
	void funcUseInteriorsForQuestMapPins( CScriptStackFrame& stack, void* result );
	void funcEnableShopkeeper( CScriptStackFrame& stack, void* result );
	void funcEnableMapPath( CScriptStackFrame& stack, void* result );
	void funcEnableDynamicMappin( CScriptStackFrame& stack, void* result );
	void funcInvalidateStaticMapPin( CScriptStackFrame& stack, void* result );
	void funcToggleUserMapPin( CScriptStackFrame& stack, void* result );
	void funcGetUserMapPinLimits( CScriptStackFrame& stack, void* result );
	void funcGetUserMapPinCount( CScriptStackFrame& stack, void* result );
	void funcGetUserMapPinByIndex( CScriptStackFrame& stack, void* result );
	void funcGetUserMapPinIndexById( CScriptStackFrame& stack, void* result );
	void funcGetIdOfFirstUser1MapPin( CScriptStackFrame& stack, void* result );
	void funcGetCurrentArea( CScriptStackFrame& stack, void* result );
	void funcNotifyPlayerEnteredBorder( CScriptStackFrame& stack, void* result );
	void funcNotifyPlayerExitedBorder( CScriptStackFrame& stack, void* result );
	void funcStartBorderTimer( CScriptStackFrame& stack, void* result );
	void funcStopBorderTimer( CScriptStackFrame& stack, void* result );
	void funcGetBorderTimer( CScriptStackFrame& stack, void* result );
	void funcSetBorderPositionAndRotation( CScriptStackFrame& stack, void* result );
	void funcIsWorldAvailable( CScriptStackFrame& stack, void* result );
	void funcGetWorldContentTag( CScriptStackFrame& stack, void* result );
	void funcGetWorldPercentCompleted( CScriptStackFrame& stack, void* result );
	void funcDisableMapPin( CScriptStackFrame& stack, void* result );
	void funcGetDisabledMapPins( CScriptStackFrame& stack, void* result );

private:
	ASSING_R4_GAME_SYSTEM_ID( GSR4_CommonMapManager );
};

BEGIN_CLASS_RTTI( CCommonMapManager )
	PARENT_CLASS( IGameSystem )
	NATIVE_FUNCTION( "InitializeMinimapManager", funcInitializeMinimapManager )
	NATIVE_FUNCTION( "SetHintWaypointParameters", funcSetHintWaypointParameters )
	NATIVE_FUNCTION( "OnChangedMinimapRadius", funcOnChangedMinimapRadius )
	NATIVE_FUNCTION( "IsFastTravellingEnabled", funcIsFastTravellingEnabled )
	NATIVE_FUNCTION( "EnableFastTravelling", funcEnableFastTravelling )
	NATIVE_FUNCTION( "IsEntityMapPinKnown", funcIsEntityMapPinKnown )
	NATIVE_FUNCTION( "SetEntityMapPinKnown", funcSetEntityMapPinKnown )
	NATIVE_FUNCTION( "IsEntityMapPinDiscovered", funcIsEntityMapPinDiscovered )
	NATIVE_FUNCTION( "SetEntityMapPinDiscovered", funcSetEntityMapPinDiscovered )
	NATIVE_FUNCTION( "IsEntityMapPinDisabled", funcIsEntityMapPinDisabled )
	NATIVE_FUNCTION( "SetEntityMapPinDisabled", funcSetEntityMapPinDisabled )
	NATIVE_FUNCTION( "IsQuestPinType", funcIsQuestPinType )
	NATIVE_FUNCTION( "IsUserPinType", funcIsUserPinType )
	NATIVE_FUNCTION( "GetUserPinNames", funcGetUserPinNames )
	NATIVE_FUNCTION( "ShowKnownEntities", funcShowKnownEntities )
	NATIVE_FUNCTION( "CanShowKnownEntities", funcCanShowKnownEntities )
	NATIVE_FUNCTION( "ShowDisabledEntities", funcShowDisabledEntities )
	NATIVE_FUNCTION( "CanShowDisabledEntities", funcCanShowDisabledEntities )
	NATIVE_FUNCTION( "ShowFocusClues", funcShowFocusClues )
	NATIVE_FUNCTION( "ShowHintWaypoints", funcShowHintWaypoints )
	NATIVE_FUNCTION( "AddQuestLootContainer", funcAddQuestLootContainer )
	NATIVE_FUNCTION( "DeleteQuestLootContainer", funcDeleteQuestLootContainer )
	NATIVE_FUNCTION( "CacheMapPins", funcCacheMapPins )
	NATIVE_FUNCTION( "GetMapPinInstances", funcGetMapPinInstances )
	NATIVE_FUNCTION( "GetMapPinTypeByTag", funcGetMapPinTypeByTag )
	NATIVE_FUNCTION( "GetHighlightedMapPinTag", funcGetHighlightedMapPinTag )
	NATIVE_FUNCTION( "TogglePathsInfo", funcTogglePathsInfo )
	NATIVE_FUNCTION( "ToggleQuestAgentsInfo", funcToggleQuestAgentsInfo )
	NATIVE_FUNCTION( "ToggleShopkeepersInfo", funcToggleShopkeepersInfo )
	NATIVE_FUNCTION( "ToggleInteriorInfo", funcToggleInteriorInfo )
	NATIVE_FUNCTION( "ToggleUserPinsInfo", funcToggleUserPinsInfo )
	NATIVE_FUNCTION( "TogglePinsInfo", funcTogglePinsInfo )
	NATIVE_FUNCTION( "ExportGlobalMapPins", funcExportGlobalMapPins );
	NATIVE_FUNCTION( "ExportEntityMapPins", funcExportEntityMapPins );
	NATIVE_FUNCTION( "GetAreaMapPins", funcGetAreaMapPins )
	NATIVE_FUNCTION( "GetEntityMapPins", funcGetEntityMapPins )
	NATIVE_FUNCTION( "UseMapPin", funcUseMapPin )
	NATIVE_FUNCTION( "UseInteriorsForQuestMapPins", funcUseInteriorsForQuestMapPins )
	NATIVE_FUNCTION( "EnableShopkeeper", funcEnableShopkeeper )
	NATIVE_FUNCTION( "EnableMapPath", funcEnableMapPath )
	NATIVE_FUNCTION( "EnableDynamicMappin", funcEnableDynamicMappin )
	NATIVE_FUNCTION( "InvalidateStaticMapPin", funcInvalidateStaticMapPin )
	NATIVE_FUNCTION( "ToggleUserMapPin", funcToggleUserMapPin )
	NATIVE_FUNCTION( "GetUserMapPinLimits", funcGetUserMapPinLimits )
	NATIVE_FUNCTION( "GetUserMapPinCount", funcGetUserMapPinCount )
	NATIVE_FUNCTION( "GetUserMapPinByIndex", funcGetUserMapPinByIndex )
	NATIVE_FUNCTION( "GetUserMapPinIndexById", funcGetUserMapPinIndexById )
	NATIVE_FUNCTION( "GetIdOfFirstUser1MapPin", funcGetIdOfFirstUser1MapPin )
	NATIVE_FUNCTION( "GetCurrentArea", funcGetCurrentArea )
	NATIVE_FUNCTION( "NotifyPlayerEnteredBorder", funcNotifyPlayerEnteredBorder )
	NATIVE_FUNCTION( "NotifyPlayerExitedBorder", funcNotifyPlayerExitedBorder )
	NATIVE_FUNCTION( "IsWorldAvailable", funcIsWorldAvailable )
	NATIVE_FUNCTION( "GetWorldContentTag", funcGetWorldContentTag )
	NATIVE_FUNCTION( "GetWorldPercentCompleted", funcGetWorldPercentCompleted )
	NATIVE_FUNCTION( "DisableMapPin", funcDisableMapPin )
	NATIVE_FUNCTION( "GetDisabledMapPins", funcGetDisabledMapPins )
END_CLASS_RTTI()
