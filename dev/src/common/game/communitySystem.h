/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "gameSystem.h"
#include "storyPhaseLocation.h"
#include "communityAgentState.h"
#include "communityConstants.h"
#include "actionPointDataDef.h"
#include "actionPointManagerListener.h"
#include "agentsWorld.h"
#include "spawnTreeDespawnerHandler.h"

///////////////////////////////////////////////////////////////////////////////

class CActionPointManager;
class CCommunityDebugger;
class CJobSpawnEntity;
class ICommunityDebugPage;
class CScenesEntriesManager;
struct CSStoryPhaseTimetableACategoriesTimetableEntry;
class CLayerInfo;
class CEntityTemplate;
class CWayPointComponent;
class CNewNPC;
class CAreaComponent;
struct SAgentStub;
struct StubSpawnData;
enum EFindAPResult;
class ICommunityGame;
class CAgentsWorld;
class CCommunityAgentStubTagManager;
class COldCommunitySpawnStrategy;
class CTimeManager;
class ISpawnStrategy;
class ISpawnTreeInitializerAI;

///////////////////////////////////////////////////////////////////////////////

#define PROFILE_COMMUNITY

#ifndef RED_FINAL_BUILD
#define COMMUNITY_DEBUG_STUBS
#endif

#ifdef COMMUNITY_DEBUG_STUBS
	#define SET_STUB_DEBUG_INFO( stub, field, value ) stub->m_debugInfo.field = value;
#else
	#define SET_STUB_DEBUG_INFO( stub, field, value ) ;
#endif

///////////////////////////////////////////////////////////////////////////////

class CCommunitySystem : public IGameSystem, public IAPMListener, IGameSaveSection, public IActorTerminationListener
{
//	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_AI );

	DECLARE_ENGINE_CLASS( CCommunitySystem, IGameSystem, 0 );

public:
	friend CCommunityDebugger;

	typedef SActiveStoryPhaseLocation::List ActiveStoryPhases;

	struct SDeactivatedStoryPhase
	{
		SStoryPhaseLocation m_location;
		Bool				m_isForceDespawn;
		SDeactivatedStoryPhase() {}
		SDeactivatedStoryPhase( const SStoryPhaseLocation& location, Bool isForceDespawn )
			: m_location( location ), m_isForceDespawn( isForceDespawn ) {}
	};

	enum EActivationState
	{
		AS_Inactive,
		AS_StateRestored,
		AS_Active
	};

private:
	Uint32												m_currentWorldHash;
	CAgentsWorld										m_agentsWorld;
	const CTimeManager&									m_timeMgr;

	EActivationState									m_active;					//!< Is community active? ( OnTick() will be processed )
	ActiveStoryPhases									m_activeStoryPhases;		//!< All active story phases
	TDynArray< SAgentStub* >							m_agentsStubs;				//!< Agents from current world
	TDynArray< SAgentStub* >							m_allAgentsStubs;			//!< Agents from all worlds
	TDynArray< THandle< CCommunity > >					m_registeredCommunities;	//!< Registered communities
	CScenesEntriesManager*								m_scenesEntriesManager;		//!< Scene manager ( deprecated )
	TDynArray< SDeactivatedStoryPhase >					m_deactivatedStoryPhases;	//!< Deactivated story phases
	THandle< CActionPointManager >						m_apMan;					//!< Action point manager
	THandle< CAreaComponent >							m_visibilityArea;			//!< Stub visibility area
	Float												m_prevSpawnRadius;
	Float												m_prevDespawnRadius;
	Bool												m_isCurrentlySpawning;

	COldCommunitySpawnStrategy*							m_defaultSpawnStrategy;

	THandle< ISpawnTreeInitializerAI >					m_communitySpawnInitializer;
	CSpawnTreeDespawnerHandler							m_despawnerHandler;

	CCommunityAgentStubTagManager*						m_agentStubTagManager;		//!< Tagged stubs manager
	Bool												m_wasDetachmentPerformedThisFrame;
	Bool												m_npcAttachmentPerformedThisFrame;
public:

	RED_INLINE Bool WasDetachmentPerformedThisFrame(){ return m_wasDetachmentPerformedThisFrame; }
	RED_INLINE void SetWasDetachmentPerformedThisFrame( Bool val ){ m_wasDetachmentPerformedThisFrame = val; }

	RED_INLINE Uint32 GetCurrentWorldHash() const { return m_currentWorldHash; }

	RED_INLINE Bool IfWorldHashMatch( Uint32 testedHash ){ return m_currentWorldHash == 0 || testedHash == 0 || m_currentWorldHash == testedHash; }

	//! Get the scene manager associated with the community system
	RED_INLINE CScenesEntriesManager* GetScenesEntriesManager()				{ return m_scenesEntriesManager; }

	CAgentsWorld& GetAgentsWorld() { return m_agentsWorld; }

	//! Get the list of registered spawn sets
	RED_INLINE const TDynArray< THandle< CCommunity > >& GetActiveSpawnsets() const		{ return m_registeredCommunities; }

	//! Get the active story phases
	RED_INLINE const ActiveStoryPhases& GetActiveStoryPhases() const			{ return m_activeStoryPhases; }

	RED_INLINE THandle< CActionPointManager > GetActionPointManager() const	{ return m_apMan; }

	RED_INLINE CCommunityAgentStubTagManager* GetAgentStubTagManager() const	{ return m_agentStubTagManager; }

	RED_INLINE Bool IsNPCAttachmentPerformedThisFrame() const { return m_npcAttachmentPerformedThisFrame; }
	RED_INLINE void SetNPCAttachmentPerformedThisFrame( Bool val ) { m_npcAttachmentPerformedThisFrame = val; }
public:
	CCommunitySystem();
	~CCommunitySystem();

	virtual void Initialize() override;
	virtual void Shutdown() override;

	//! Load predefined community settings
	void LoadCommunityConstants();

	//! Community was activated
	void OnCommunityActivated( CCommunity* community );

	//! Community was deactivated
	void OnCommunityDeactivated( CCommunity* community );

	//! Check if can spawn NPCs fog given community
	Bool IsCommunityValidForSpawning( const CCommunity* community ) const;

	//! Update community system
	void Tick( Float timeDelta );

	//! Generate community debug fragments
	void OnGenerateDebugFragments( CRenderFrame* frame );

	//! Needed for garbage collector
	void OnSerialize( IFile &file );

	//! Activate community story phase
	void ActivateStoryPhase( CCommunity *community, CSTableEntry *communityEntry, CSStoryPhaseEntry *communityStoryPhase );

	//! Deactivate community story phase
	void DeactivateStoryPhase( CCommunity *community, CSTableEntry *communityEntry, CSStoryPhaseEntry *communityStoryPhase );

	EFindAPResult FindCurrentActionPointFilter( const TDynArray< CSStoryPhaseTimetableACategoriesTimetableEntry >& timetable, SActionPointFilter& actionPointFilter );

	EFindAPResult GetActionPointFilter( const CSStoryPhaseTimetableACategoriesTimetableEntry *timetabEntry, SActionPointFilter& actionPointFilter );

	ISpawnTreeInitializerAI* GetCommunitySpawnInitializer() const;

	//! Find random action point to use
	EFindAPResult FindRandomActionCategoryAndLayerName( const TDynArray< CSStoryPhaseTimetableACategoriesTimetableEntry > &timetable, CName &actionCategoryName /* in out */, CLayerInfo *&layerInfo /* out */, TagList &actionPointTags /* out */, Vector* mapPinPos = NULL );

    //! Find some random actin point
    static EFindAPResult GetRandomActionCategoryAndLayer( const CSStoryPhaseTimetableACategoriesTimetableEntry *timetabEntry, CLayerInfo* &layerInfo /* out */, CName &actionCategoryName /* in out */, TagList &actionPointTags /* out */, Vector* mapPinPos = NULL );

	//! Find agent stub for given NPC
	const SAgentStub* FindStubForNPC( const CNewNPC * npc ) const;

	//! Is given NPC controlled by community system ?
	Bool IsNPCInCommunity( const CNewNPC* npc ) const;

	//! Get all spawned NPC with given paramters
	Bool GetSpawnedNPC( const TSoftHandle<CEntityTemplate>& entityTemplate, CName appearance, const Vector &worldPosition, Bool spawnOnDemand, CNewNPC *&outputNPC /* out */ );

	//! Find best despawn point to despawn a NPC, returns false if no despawn points were found
	Bool FindDespawnPoint( const TDynArray< CSStoryPhaseTimetableACategoriesTimetableEntry > *timetable, const TagList &despawnTags, Vector &despawnPoint /* out */ );

	//! Set visibility area that controls spawning of NPCs from agent stubs
	void SetVisibilityArea( CAreaComponent *area );
	Bool IsVisibilityAreaEnabled() const;

	// For debug purposes
	CAreaComponent* GetVisibilityArea() const;

	//! Reset visibility area
	void ResetVisibilityArea();

	Bool IsSpawningStabilized() const { return !m_isCurrentlySpawning; }
	void MarkAsCurrentlySpawning() { m_isCurrentlySpawning = true; }

	//! Returns the default spawn strategy
	const ISpawnStrategy* GetDefaultSpawnStrategy();

	// Active story phase notifications
	void MarkActiveStoryPhaseAgentsModified( const SHashedStoryPhaseLocation& l );
	void AddActiveStoryPhaseAgent( const SHashedStoryPhaseLocation& l );
	void DelActiveStoryPhaseAgent( const SHashedStoryPhaseLocation& l );

	//! Visibility spawn/despawn radius
	Bool SetVisibilitySpawnRadius( Float radius );
	Bool SetVisibilityDespawnRadius( Float radius );
	Bool SetVisibilityRadius( Float spawnRadius, Float despawnRadius );
	Bool SetVisibilityAreaDespawnRadius( Float areaDespawnRadius );
	Bool SetVisibilityAreaSpawnRadius( Float areaSpawnRadius );
	void GetVisibilityRadius( Float &spawnRadius /* out */, Float &despawnRadius /* out */ ) const;
	void GetPrevVisibilityRadius( Float &prevSpawnRadius /* out */, Float &prevDespawnRadius /* out */ ) const;
	void UpdatePrevVisibilityRadius();
	void GetVisibilityAreaRadius( Float &areaSpawnRadius /* out */, Float &areaDespawnRadius /* out */ ) const;
	void RestoreDefaultVisibilityRadiuses(); // debug method
	void EnableVisibilityRadiusChange( Bool enable = true ); // debug method
	Bool IsVisibilityRadiusChangeEnalbed() const { return CCommunityConstants::VISIBILITY_RADIUS_CHANGE_ENABLED; }

public:
	void RegisterContextForCommunity( CEntity* context, CCommunity* community );
	void UnregisterContextForCommunity( CEntity* context, CCommunity* community );

public:
	// ---------------------------------------------------------------------------
	// IGameSystem implementation
	// ---------------------------------------------------------------------------
	virtual void OnGameStart( const CGameInfo& gameInfo );
	virtual void OnGameEnd( const CGameInfo& gameInfo );
	virtual void OnWorldStart( const CGameInfo& gameInfo );
	virtual void OnWorldEnd( const CGameInfo& gameInfo );

	// ---------------------------------------------------------------------------
	// IAPMListener implementation
	// ---------------------------------------------------------------------------
	virtual void UpdateAPOccupation( const TActionPointID& apID, Bool& outIsOccupied ) const;
	virtual void OnAPManagerDeletion();

	// ---------------------------------------------------------------------------
	// IActorTerminationListener interface
	// ---------------------------------------------------------------------------
	void OnDeath( CActor* actor )override;
	void OnDetach( CActor* actor )override;
	// Stray actor interface ( See CStrayActorManager )
	Bool CanBeConvertedToStrayActor()const override;

	void TerminateActor( CActor* actor, Bool death );

private:
	//! Save community state to save game
	virtual bool OnSaveGame( IGameSaver* saver );

	//! Restore community state from save game
	void RestoreState( IGameLoader* loader );

	void RestoreAgentsAfterWorldChange();

private:
	void RemoveFromAllAgentsList( SAgentStub* agentStub );

	//! Tick agents
	void AgentsTick( Float timeDelta );

	//! Update despawn
	void DespawnTick();

	//! Update embedded community scenes
	void ScenesTick( Float timeDelta );
    
    // Agent Stub system
private:
	//! Update stub subsystem
	void OnAgentsStubTick( Float timeDelta );

	//! Process stub creation and destruction
	void SpawnAgentsStubsTick();

	//! Randomize community state
	void InitRandomCommunityState();

	void RegisterSpawnedStub( SAgentStub* agentStub );
	SAgentStub* CreateAgentStubAtSpawnLocation( const SStoryPhaseLocation &storyPhase );

	friend struct SAgentStub;
	SAgentStub* CreateAgentStubAfterLoad( const SStoryPhaseLocation &storyPhase, const StubSpawnData& spawnData, Bool worldMatch );

	void ProcessRespawnTimers( CSStoryPhaseEntry *communityStoryPhaseEntry );

	// Executive producer
	void SpawnAgentsStub( Int32 agentsToSpawnNum, SActiveStoryPhaseLocation &storyPhase, bool useDelayedSpawn = true );
	void DespawnAgentsStub( Int32 agentsToDespawnNum, const SActiveStoryPhaseLocation &storyPhase );

protected:

//////////////////////////////////////////////////////////////////////////////////////
// Debug
//////////////////////////////////////////////////////////////////////////////////////

#ifndef NO_DEBUG_PAGES
	void DebugTick( Float timeDelta );
#endif

public:
	// Debug status divided into pages
	Int32                        DebugGetNumStatusPages();
	void                       DebugUpdateStatusPage ( Int32 num, ICommunityDebugPage& debugPage );

	// Debug info methods
	Int32  GetAgentStubsNum() { return m_agentsStubs.Size(); }
	Int32  GetBackgroundStubsNum();
	Int32  GetNPCsNum();
	void GetStubsInfo( Int32 &stubsCount /* out */, Int32 &npcCount /* out */ );
	Bool DoesStubExist( SAgentStub *agentStub ) { return m_agentsStubs.Exist( agentStub ); }
	Int32  GetStubIndex( SAgentStub *agentStub  );
	void GetMappinTrackedStubsInfo( Int32 &questStubsFound /* out */, Int32 &merchantStubsFound /* out */, String &info /* out */ );

	TDynArray< SAgentStub* >& GetAgentsStubs() { return m_agentsStubs; }

	// Community modification debug methods
	void DebugDeactivateAllCommunitiesButThis( const String &communityDepotPath );
		
#ifdef COMMUNITY_DEBUG_STUBS
	struct SDebugData
	{
		const SAgentStub            *m_currentAgentStub;
		TActionPointID               m_currentAPID;
		const CWayPointComponent    *m_despawnWayPoint;
		SDebugData() : m_currentAgentStub( NULL ), m_currentAPID( ActionPointBadID ), m_despawnWayPoint( NULL ) {}
	};
	void SetDebugData( const SDebugData &debugData );
	SDebugData *m_debugData;

	// Only one STUB debug mode
	void DebugSetOnlyOneStubIdx( Int32 idx ) { m_dbgOnlyAgentStub = m_agentsStubs[idx]; }
	SAgentStub *m_dbgOnlyAgentStub;
#endif


	ASSIGN_GAME_SYSTEM_ID( GS_Community );
};

BEGIN_CLASS_RTTI( CCommunitySystem )
	PARENT_CLASS( IGameSystem )
	PROPERTY( m_apMan );
	PROPERTY( m_communitySpawnInitializer );
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////
