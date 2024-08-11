/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../engine/gameTime.h"

#include "behTreeArbitratorPriorities.h"
#include "storySceneForcingMode.h"
#include "storyScene.h"
#include "wayPointComponent.h"
#include "spawnTreeInitializer.h"
#include "../engine/gameTimeInterval.h"

//////////////////////////////////////////////////////////////////////////

enum EStorySceneForcingMode;
class CStoryScene;
typedef CName TAPCategory;
class CCommunitySystem;
class CEntityTemplate;
class CCommunity;
class ICommunityGame;
class IEngineSpawnStrategy;
class ISpawnStrategy;
class ISpawnTreeInitializer;

//////////////////////////////////////////////////////////////////////////

class ICommunityDebugPage
{
public:
	virtual ~ICommunityDebugPage() {}

	virtual void AddText( const String& str, const Color& color ) = 0;

	virtual void FocusOn( const Vector& pos ) = 0;
};

//////////////////////////////////////////////////////////////////////////

class CCommunitySpawnStrategy : public CObject
{
	DECLARE_ENGINE_CLASS( CCommunitySpawnStrategy, CObject, 0 )
public:
	IEngineSpawnStrategy*		m_strategy;
};

BEGIN_CLASS_RTTI( CCommunitySpawnStrategy )
	PARENT_CLASS( CObject );
	PROPERTY_INLINED( m_strategy,  TXT("Strategy") );
END_CLASS_RTTI();


//////////////////////////////////////////////////////////////////////////

class CCommunityInitializers : public CObject
{
	DECLARE_ENGINE_CLASS( CCommunityInitializers, CObject, 0 )
public:
	TDynArray< ISpawnTreeInitializer* > m_initializers;
};

BEGIN_CLASS_RTTI( CCommunityInitializers )
	PARENT_CLASS( CObject );
	PROPERTY_INLINED( m_initializers,  TXT("Initializers") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

// Community Map Pin Type
enum ECommMapPinType
{
	CMPT_None,
	CMPT_Inn,
	CMPT_Shop,
	CMPT_Craft,
};

BEGIN_ENUM_RTTI( ECommMapPinType );
	ENUM_OPTION( CMPT_None );
	ENUM_OPTION( CMPT_Inn );
	ENUM_OPTION( CMPT_Shop );
	ENUM_OPTION( CMPT_Craft );
END_ENUM_RTTI();

template<>
RED_INLINE Bool FromString( const String& text, ECommMapPinType& value )
{
	if ( text == TXT("INN") )
	{
		value = CMPT_Inn;
	}
	else if ( text == TXT("SHOP") )
	{
		value = CMPT_Shop;
	}
	else if ( text == TXT("CRAFT") )
	{
		value = CMPT_Craft;
	}
	else
	{
		value = CMPT_None;
		ASSERT( !TXT("From String ECommMapPinType: unknown type") );
		return false;
	}

	return true;
};

template<> RED_INLINE String ToString( const ECommMapPinType& value )
{
	switch ( value )
	{
	case CMPT_None:
		return String::EMPTY;
	case CMPT_Inn:
		return TXT("INN");
	case CMPT_Shop:
		return TXT("SHOP");
	case CMPT_Craft:
		return TXT("CRAFT");
	default:
		ASSERT( !TXT("ToString ECommMapPinType: unknown type") );
		return String::EMPTY;
	}
}

//////////////////////////////////////////////////////////////////////////

struct CSEntitiesEntry
{
	DECLARE_RTTI_STRUCT( CSEntitiesEntry );

public:
	// WARNING: this struct has custom assign operator!
	TSoftHandle< CEntityTemplate >	m_entityTemplate;	//!< Entity template to spawn from
	TDynArray< CName >				m_appearances;      //!< If not empty, than this list overrides original entity's appearances list
	Float							m_weight;           //!< The probability of choosing this entity
	TagList							m_entitySpawnTags;  //!< Tags added to entity on spawn
	CName							m_mappinTag;
	ECommMapPinType					m_mappinType;

	CCommunityInitializers*			m_initializers;
	CCommunityInitializers*			m_despawners;
	CGUID							m_guid;				//!< Identification data

private:
	// runtime data
	TDynArray<CName>                m_appearancesPool;

public:
	// Assign operator needed for CObject cloning
	CSEntitiesEntry& operator=( const CSEntitiesEntry& other );

public:
	//! Returns a unique id assigned to this instance
	RED_INLINE const CGUID& GetGUID() const { return m_guid; }

public:
	CSEntitiesEntry();

	//! Picks one of the defined appearances at random
	Bool GetRandomAppearance( CName *appearance /* out */ );

	Bool IsForceDespawned() const;
};

BEGIN_CLASS_RTTI( CSEntitiesEntry )
	PROPERTY_EDIT       ( m_entityTemplate,  TXT("Entity Template") );
	PROPERTY_EDIT       ( m_appearances,     TXT("Entity Appearances") );
	PROPERTY_CUSTOM_EDIT( m_entitySpawnTags, TXT("Entity Spawn Tags"), TXT("TagListEditor") );
	PROPERTY_EDIT       ( m_mappinTag,       TXT("Map Pin Tag") );
	PROPERTY_EDIT       ( m_mappinType,		 TXT("Map Pin Type") );
	PROPERTY_EDIT       ( m_initializers,    TXT("Entity initializers") );
	PROPERTY			( m_guid )
	PROPERTY_EDIT		( m_despawners,		TXT("Despawn initializers") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct CSStoryPhaseSpawnTimetableEntry
{
	DECLARE_RTTI_STRUCT( CSStoryPhaseSpawnTimetableEntry );

	GameTime m_time;
	Int32      m_quantity;
	GameTime m_respawnDelay;
	Bool	 m_respawn;

	CSStoryPhaseSpawnTimetableEntry()
		: m_quantity( 1 )
		, m_respawnDelay( 0 )
		, m_respawn( false )
	{}
};

BEGIN_CLASS_RTTI( CSStoryPhaseSpawnTimetableEntry )
	PROPERTY_EDIT( m_time,             TXT("Time") );
	PROPERTY_EDIT( m_quantity,         TXT("Quantity") );
	PROPERTY_EDIT( m_respawnDelay,     TXT("Respawn delay") );
	PROPERTY_EDIT( m_respawn,      TXT("Respawn NPC after killing it") );
END_CLASS_RTTI();


//////////////////////////////////////////////////////////////////////////

struct CSStoryPhaseNames
{
	DECLARE_RTTI_STRUCT( CSStoryPhaseNames );

	CSStoryPhaseNames() {}

	CSStoryPhaseNames( const TagList &tagList )
	{
		m_tags = tagList;
	}

	operator TagList() const
	{
		return m_tags;
	}

	Bool operator==( const CSStoryPhaseNames &obj ) const
	{
		return m_tags == obj.m_tags;
	}

	TagList m_tags;
};

BEGIN_CLASS_RTTI( CSStoryPhaseNames )
	PROPERTY_EDIT( m_tags, TXT("Story phases tags") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

// dynamic data related to a story phase
struct SStoryPhaseData
{
	TDynArray< THandle< CWayPointComponent > >	m_spawnPointsPool;
	//// DM: We use engine time for spawning delays, because game world time may be paused/scaled and in effect monsters will not respawn as intended
	EngineTime									m_respawnDelay;			// Delay between respawn (when NPC was killed)
	EngineTime									m_spawnDelay;			// Delay between spawn
	Int32											m_killedNPCsNum;		// the number of killed NPCs

	SStoryPhaseData()
		: m_killedNPCsNum( 0 )
	{}
};

struct CSStoryPhaseEntry
{
	DECLARE_RTTI_STRUCT( CSStoryPhaseEntry );

public:
	String											m_comment;
	CSStoryPhaseNames								m_storyPhaseName;
	Bool											m_isHiddenSpawn;	//!< If true than NPC will be spawned only behind camera
	CCommunityInitializers*							m_initializers;
	TDynArray< CSStoryPhaseSpawnTimetableEntry >	m_spawnTimetable;
	CName											m_timetableName;
	GameTime										m_spawnDelay;
	TagList											m_spawnPointTags;
	TagList											m_despawnPointTags;
	Bool											m_startInAP;
	Bool											m_useLastAP;
	Bool											m_alwaysSpawned;	//!< If true then don't change NPC into Agent Stub
	CCommunitySpawnStrategy*						m_spawnStrategy;
	CGUID											m_guid;
	Vector                                          m_cachedMapPinPosition;

private:
	// runtime data
	SStoryPhaseData									m_rtData;

public:
	//! Returns a unique id assigned to this instance
	RED_INLINE const CGUID& GetGUID() const { return m_guid; }

	//! Returns a runtime story-phase related data
	RED_INLINE SStoryPhaseData& GetStoryPhaseData() { return m_rtData; }

public:
	CSStoryPhaseEntry();

	//! Returns a number of agents that should be spawned
	Int32	GetDesiredNumberSpawnedAgents( const GameTime& currGameTime ) const;

	//! Resets the runtime story-phase related data
	void ResetRTData();

	//! Save runtime state
	void SaveState( IGameSaver* saver );

	//! Restore runtime state
	void RestoreState( IGameLoader* loader );

	//! Allocates a spawn point for an NPC to use - virtual for the testing purposes. As soon
	//! as the code gets refactored, we can remove that
	Bool AllocateSpawnPoint( Vector& outPos, Float& outYaw, CLayerInfo*& layerInfo );

	//! Returns the spawn strategy for the stub
	const ISpawnStrategy* GetSpawnStrategy() const;

private:
	//! Updates the runtime spawnpoints pool
	Bool FillSpawnPointPool( const TDynArray< CNode* >& spawnNodes );
};

BEGIN_CLASS_RTTI( CSStoryPhaseEntry )
	PROPERTY( m_guid )
	PROPERTY_EDIT( m_comment,					TXT("Commentary") );
	PROPERTY_CUSTOM_EDIT( m_storyPhaseName,		TXT("Story sub phase name"), TXT("TagListEditor") );
	PROPERTY_EDIT( m_isHiddenSpawn,				TXT("Spawn NPC only behind camera") );	
	PROPERTY_EDIT( m_initializers,				TXT("Story phase initializer collection"));
	PROPERTY_EDIT( m_spawnTimetable,			TXT("Spawn Timetable") );
	PROPERTY_EDIT( m_timetableName,				TXT("Timetable name") );
	PROPERTY_EDIT( m_spawnDelay,				TXT("Spawn delay interval") );
	PROPERTY_CUSTOM_EDIT( m_spawnPointTags,		TXT("Spawn Point Tags"), TXT("TagListEditor") );
	PROPERTY_CUSTOM_EDIT( m_despawnPointTags,	TXT("Despawn Point Tags"), TXT("TagListEditor") );
	PROPERTY_EDIT( m_startInAP,					TXT("Spawn in action point") );
	PROPERTY_EDIT( m_useLastAP,					TXT("Use last action point") );
	PROPERTY_EDIT( m_alwaysSpawned,				TXT("Don't change NPC into stub") );
	PROPERTY_INLINED( m_spawnStrategy,			TXT("Who should be spawned and in what circumstances?") );
	PROPERTY_EDIT( m_cachedMapPinPosition,      TXT("Cached map pin") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct CSSpawnType
{
	DECLARE_RTTI_STRUCT( CSSpawnType );

	Bool operator==( const CSSpawnType &obj ) const
	{
		return m_spawnType == obj.m_spawnType;
	}

	CName m_spawnType;
};
BEGIN_CLASS_RTTI( CSSpawnType )
	PROPERTY_EDIT( m_spawnType, TXT("Spawn type") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct CSTableEntry
{
	DECLARE_RTTI_STRUCT( CSTableEntry );

public:
	// WARNING: this struct has custom assign operator!
	String								m_comment;			//!< For debug purposes only
	String								m_entryID;			//!< For debug purposes only (unique)
	TDynArray< CSEntitiesEntry >		m_entities;			//!< Entities entries
	Bool								m_alwaysSpawned;	//!< If true then don't change NPC into Agent Stub
	TDynArray< CSStoryPhaseEntry >		m_storyPhases;		//!< Phase entries
	CGUID								m_guid;				//!< GUID

private:
	CSStoryPhaseEntry*					m_activePhase;		//!< Current active phase

public:
	// Assign operator needed for CObject cloning in m_entities
	CSTableEntry& operator=( const CSTableEntry& other );

public:
	//! Is this table entry active ?
	RED_INLINE Bool IsActive() const { return m_activePhase != NULL; }

	//! Get active phase in this table entry
	RED_INLINE const CSStoryPhaseEntry* GetActivePhase() const { return m_activePhase; }

	//! Get active phase in this table entry
	RED_INLINE CSStoryPhaseEntry* GetActivePhase() { return m_activePhase; }
	
	//! Get table GUID
	RED_INLINE const CGUID& GetGUID() const { return m_guid; }

public:
	CSTableEntry();

	Bool IsActivePhaseValidWorld( CCommunity* owner );
	//! Activate phase in this table entry
	void ActivatePhase( CCommunitySystem& cs, CCommunity& owner, const CName& phasesName );

	CSStoryPhaseEntry* FindPhaseEntry( const CName& phaseName );

	//! Reset active phase
	void ResetActivePhase(  CCommunitySystem& cs, CCommunity& owner );

	//! Get some random entity crap
	Bool GetRandomWeightsEntityEntry( CSEntitiesEntry **communityEntitiesEntry /* out */ );

	//! Returns the names of all story phases defined in the spawnset entry
	void GetPhaseNames( THashSet< CName >& outNames ) const;

	//! Save state ( runtime )
	void SaveState( IGameSaver* saver );

	//! Restore state ( runtime )
	void RestoreState( IGameLoader* loader );

	//! Reset runtime data
	void ResetRTData();
};

BEGIN_CLASS_RTTI( CSTableEntry )
	PROPERTY_EDIT       ( m_comment,				TXT("Comment") );
	PROPERTY_EDIT       ( m_entryID,				TXT("Entry ID") );
	PROPERTY_EDIT       ( m_entities,				TXT("Entities") );
	PROPERTY			( m_alwaysSpawned );
	PROPERTY_EDIT       ( m_storyPhases,			TXT("Story phases") );
	PROPERTY			( m_guid );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct CSStoryPhaseTimetableACategoriesEntry
{
	DECLARE_RTTI_STRUCT( CSStoryPhaseTimetableACategoriesEntry );

	CName	m_name;
	Float	m_weight;
	TagList	m_apTags;
	//TagList	m_areaTags;

	CSStoryPhaseTimetableACategoriesEntry() : m_weight( 1.0f ) {}
};

BEGIN_CLASS_RTTI( CSStoryPhaseTimetableACategoriesEntry )
	PROPERTY_EDIT( m_name,   TXT("Category") );
	PROPERTY_EDIT( m_weight, TXT("Weight") );
	PROPERTY_EDIT( m_apTags, TXT("AP Tags") );
	//PROPERTY_CUSTOM_EDIT( m_areaTags, TXT("Tag of the working area."), TXT( "TaggedEntitySelector" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct CSLayerName
{
	DECLARE_RTTI_STRUCT( CSLayerName );

	CName			m_layerName;

private:
	mutable THandle< CLayerInfo >		m_cachedLayer;

public:
	CLayerInfo* GetCachedLayer() const{ CacheLayer(); return m_cachedLayer.Get(); }
	void CacheLayer() const;

	void ResetLayer() const;

	CSLayerName()
		: m_cachedLayer( NULL )
	{};

	RED_INLINE Bool IsNameValid(){ return m_layerName /* || m_cachedLayer */; }
	Bool GetWorldName();
};

BEGIN_CLASS_RTTI( CSLayerName )
	PROPERTY_EDIT( m_layerName, TXT("Layer name") );	
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct CSStoryPhaseTimetableActionEntry
{
	DECLARE_RTTI_STRUCT( CSStoryPhaseTimetableActionEntry );

	CSLayerName                                        m_layerName;
	TDynArray< CSStoryPhaseTimetableACategoriesEntry > m_actionCategories;

	void CacheLayer()
	{
		m_layerName.CacheLayer();
	}
	void ResetLayer()
	{
		m_layerName.ResetLayer();
	}
};

BEGIN_CLASS_RTTI( CSStoryPhaseTimetableActionEntry )
	PROPERTY_EDIT( m_layerName,        TXT("Layer name") );
	PROPERTY_EDIT( m_actionCategories, TXT("Action categories") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct CSStoryPhaseTimetableACategoriesTimetableEntry
{
	DECLARE_RTTI_STRUCT( CSStoryPhaseTimetableACategoriesTimetableEntry );

	GameTime                                      m_time;
	TDynArray< CSStoryPhaseTimetableActionEntry > m_actions;

	void CacheLayer()
	{
		for ( Uint32 i=0; i<m_actions.Size(); ++i )
		{
			m_actions[i].CacheLayer();
		}
	}

	void ResetLayer()
	{
		for ( Uint32 i=0; i<m_actions.Size(); ++i )
		{
			m_actions[i].ResetLayer();
		}
	}
};

BEGIN_CLASS_RTTI( CSStoryPhaseTimetableACategoriesTimetableEntry )
	PROPERTY_EDIT( m_time,    TXT("Time") );
	PROPERTY_EDIT( m_actions, TXT("Actions") );
END_CLASS_RTTI();

typedef TDynArray< CSStoryPhaseTimetableACategoriesTimetableEntry > TTimetable;

//////////////////////////////////////////////////////////////////////////

struct CSStoryPhaseTimetableEntry
{
	DECLARE_RTTI_STRUCT( CSStoryPhaseTimetableEntry );

	CName                                                       m_name;
	TDynArray< CSStoryPhaseTimetableACategoriesTimetableEntry > m_actionCategies;

	void CacheLayer()
	{
		for ( Uint32 i=0; i<m_actionCategies.Size(); ++i )
		{
			m_actionCategies[i].CacheLayer();
		}
	}

	void ResetLayer()
	{
		for ( Uint32 i=0; i<m_actionCategies.Size(); ++i )
		{
			m_actionCategies[i].ResetLayer();
		}
	}
};

BEGIN_CLASS_RTTI( CSStoryPhaseTimetableEntry )
	PROPERTY_EDIT( m_name,           TXT("Timetable name") );
	PROPERTY_EDIT( m_actionCategies, TXT("Action categories") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

struct CSSceneTimetableScenesEntry
{
	DECLARE_RTTI_STRUCT( CSSceneTimetableScenesEntry );

	CSSceneTimetableScenesEntry() : m_cooldownTime(30.0f), m_weight(1.0f), m_priority( BTAP_AboveIdle ), m_forceMode(SSFM_ForceNothing) {}

	THandle< CStoryScene >	m_storyScene;
	String					m_sceneInputSection;
	Float					m_cooldownTime;
	Float					m_weight;
	EArbitratorPriorities	m_priority;
	EStorySceneForcingMode	m_forceMode;
};

BEGIN_CLASS_RTTI( CSSceneTimetableScenesEntry )
	PROPERTY_EDIT( m_storyScene,        TXT("Scene")  );
	PROPERTY_EDIT( m_sceneInputSection, TXT("Scene input") );
	PROPERTY_EDIT( m_cooldownTime,      TXT("Cooldown time") );
	PROPERTY_EDIT( m_weight,            TXT("Weight") );
	PROPERTY_EDIT( m_priority,          TXT("Priority") );
	PROPERTY_EDIT( m_forceMode,			TXT("Force conditions") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct CSSceneTimetableEntry
{
	DECLARE_RTTI_STRUCT( CSSceneTimetableEntry );

	GameTimeInterval                         m_time;
	TDynArray< CSSceneTimetableScenesEntry > m_scenes;
};

BEGIN_CLASS_RTTI( CSSceneTimetableEntry )
	PROPERTY_EDIT( m_time,	TXT("Time") );
	PROPERTY_EDIT( m_scenes,TXT("Scenes") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct CSSceneTableEntry
{
	DECLARE_RTTI_STRUCT( CSSceneTableEntry );

	CSSceneTableEntry() : m_cooldownTime(0.f) {}

	CSStoryPhaseNames					m_storyPhaseName;
	String								m_entryID; // for debug purposes only
	Float								m_cooldownTime;
	TDynArray< CSSceneTimetableEntry >	m_timetable;
};

BEGIN_CLASS_RTTI( CSSceneTableEntry )
	PROPERTY_EDIT( m_storyPhaseName,	TXT("Story phase") );
	PROPERTY_EDIT( m_cooldownTime,		TXT("Cooldown time") );
	PROPERTY_EDIT( m_timetable,			TXT("Timetable") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class ISpawnsetPhaseNamesGetter
{
public:
	virtual ~ISpawnsetPhaseNamesGetter() {}

	virtual void GetSpawnsetPhaseNames( IProperty *property, THashSet<CName> &outNames ) = 0;
};

//////////////////////////////////////////////////////////////////////////

enum ECommunitySpawnsetType
{
	CST_Global,
	CST_Template,
};

BEGIN_ENUM_RTTI( ECommunitySpawnsetType );
	ENUM_OPTION( CST_Global );
	ENUM_OPTION( CST_Template );
END_ENUM_RTTI();

class CCommunity : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CCommunity, CResource, "w2comm", "Community" );

private:
	// editable static data
	TDynArray< CSTableEntry >					m_communityTable;
	TDynArray< CSStoryPhaseTimetableEntry >		m_storyPhaseTimetable;
	TDynArray< CSSceneTableEntry >				m_sceneTable;
	TDynArray< CName >							m_layers;
	ECommunitySpawnsetType						m_spawnsetType;
	
private:
	// runtime data
	CName									 m_activePhase;
	Bool                                     m_isActive;

public:
	//! Get community table entries
	RED_INLINE TDynArray< CSTableEntry >& GetCommunityTable() { return m_communityTable; }

	//! Get community table entries
	RED_INLINE const TDynArray< CSTableEntry >& GetCommunityTable() const { return m_communityTable; }

	//! Get scenes table
	RED_INLINE const TDynArray< CSSceneTableEntry >& GetScenesTable() const { return m_sceneTable; }

	//! Get action point layers table
	RED_INLINE const TDynArray< CName >& GetActionPointLayers()   const { return m_layers; }

	//! Get time table entries
	RED_INLINE TDynArray< CSStoryPhaseTimetableEntry >& GetStoryPhaseTimetable() { return m_storyPhaseTimetable; }
	
	//! Get time table entries
	RED_INLINE const TDynArray< CSStoryPhaseTimetableEntry >& GetStoryPhaseTimetable() const { return m_storyPhaseTimetable; }

	//! Is this community active ?
	RED_INLINE Bool IsActive() const { return m_isActive; }

	//! Get the name of the active phase
	RED_INLINE const CName& GetActivePhaseName() const { return m_activePhase; }

	RED_INLINE ECommunitySpawnsetType	GetSpawnsetType() const	{ return m_spawnsetType; }

	RED_INLINE void	SetSpawnsetType( const ECommunitySpawnsetType spawnsetType)	{ m_spawnsetType = spawnsetType; }

public:
	CCommunity();

	//! Resource serialization
	virtual void OnSerialize( IFile &file );

	//! Resource loaded
	virtual void OnPostLoad();

public:
	//! Cache internal data, called after attaching to world
	void CacheInternalData();

	//! Resets internal data, called when loading savegame
	void ResetInternalData();

	//! Activate phase
	void ActivatePhase( const CName& phaseName );

	//! Deactivate
	void Deactivate();

	//! Returns the names of all story phases defined in the spawnset
	void GetPhaseNames( THashSet< CName >& outNames ) const;

	//! Get scene by phase name
	const CSSceneTableEntry* GetScene( const CName& phaseName ) const;

	//! Get timetable by name
	const CSStoryPhaseTimetableEntry* GetTimetable( const CName& timetableName ) const;

	//! Get community debug status
	void GetDebugStatus( Uint32 commIdx, ICommunityDebugPage& debugPage ) const;

	//! Save spawnset state
	void SaveState( IGameSaver* saver );

	//! Called when the game state is being restored from a savegame
	void RestoreState( IGameLoader* reader );

	//! Updates the unique IDs of the objects that make up this spawnset definition
	void UpdateGUIDS();

	//! Check GUIDs, returns true if OK
	Bool CheckGUIDs();

	//! Resets the runtime related data
	void ResetRTData();

	//! Find duplicated initializers
	Bool InlinedDuplicateTest();	
};

BEGIN_CLASS_RTTI( CCommunity )
	PARENT_CLASS( CResource )
	PROPERTY_EDIT( m_communityTable, TXT("Community table") );
	PROPERTY_EDIT( m_storyPhaseTimetable, TXT("Story Phase Timetable") );
	PROPERTY( m_spawnsetType );
END_CLASS_RTTI()
