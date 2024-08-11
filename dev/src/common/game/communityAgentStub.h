/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../../common/engine/idTag.h"
#include "agent.h"
#include "communityAgentState.h"
#include "storyPhaseLocation.h"
#include "../engine/pathlib.h"


///////////////////////////////////////////////////////////////////////////////

class CJobSpawnEntity;
class ICommunityDebugPage;
enum ECommMapPinType;
class CActionPointManager;

///////////////////////////////////////////////////////////////////////////////

/// Initialization info for spawning agent stub
struct StubSpawnData
{
	IdTag		m_suggestedTag;			//!< Preferred IdTag to assign to the stub, if not given an IdTag will be allocated from story phase IdTagPool
	Vector		m_position;				//!< Initial position
	Float		m_rotation;				//!< Initial rotation

	StubSpawnData()
		: m_position( 0.0f, 0.0f, 0.0f )
		, m_rotation( 0.0f )
	{};
};

///////////////////////////////////////////////////////////////////////////////

/// Debugging information for agent stub
struct SAgentStubDebugInfo
{
	enum EVisibilityState
	{
		VS_NoPlayer,
		VS_NoValidSpawnPos,
		VS_NotStreamed,
		VS_VisibilityArea,
		VS_TooFarFromPlayer,
		VS_WorkingOnUnloadedLayer,
		VS_Dead,
		VS_Despawning,
		VS_IsNpcAlready,
		VS_NoPathEngineWorld,
		VS_NoWorld,
		VS_ToSpawn
	};

	EVisibilityState	m_visibilityState;
	String				m_partitionName;
};

///////////////////////////////////////////////////////////////////////////////

class IWorkQuery
{
public:
	virtual ~IWorkQuery() {}

	virtual void AcquireNextAP() = 0;
};

///////////////////////////////////////////////////////////////////////////////
class CCommunityAgent
{
public:
	CCommunityAgent();

	// setters
	void		SetAgentCategory( Uint32 category )								{ m_agentCategory = category; }	

	// external interface
	Bool		IsEnabled() const												{ return m_isEnabled; }
	void		SetEnabled( Bool b )											{ m_isEnabled = b; }	
	void		MoveTo( const Vector3& destination );
	void		SetPosition( const Vector3& destination );
	const Vector3&	GetPosition() const;
	Bool		TestLocation( const Vector3& pos ) const;
	Bool		FindSafeSpot( const Vector3& pos, Float radius, Vector3& outPos ) const;
	Bool		TestCurrentPosition() const;

protected:
	Uint32					m_agentCategory;
	Float					m_personalSpace;	
	mutable PathLib::AreaId	m_currentArea;
	mutable Vector3			m_position;	
	Bool					m_isEnabled;	
};

///////////////////////////////////////////////////////////////////////////////

class CNullWorkQuery : public IWorkQuery
{
public:
	void AcquireNextAP() {}
};

///////////////////////////////////////////////////////////////////////////////
/// Agent stub
struct SAgentStub : public IAgent, public IWorkQuery, public Red::System::NonCopyable
{
public:
	IdTag					m_idTag;							//!< Unique IdTag
	ECommunityAgentState	m_state;							//!< State of the stub
	mutable CCommunityAgent	m_communityAgent;
	Vector					m_spawnPos;							//!< Position to spawn at, calculated at ShouldSpawn
	Float					m_agentYaw;							//!< Rotation, kept for simplicity
	Float					m_processingTimer;					//!< Processing timer
	CName					m_appearance;						//!< Selected appearance
	SAgentStubDebugInfo		m_debugInfo;						//!< Stub debugging information
	IJobEntitySpawn*		m_jobSpawnEntity;					//!< Entity spawning job
	Bool					m_teleportToDestination;
	Int32						m_mapPinId;
	THandle< CNewNPC >		m_npc;								//!< Handle to spawned NPC
	Int32						m_hiddenOnStartTicksLeftHACK;		//!< Yeah I know, it's stupid if it isn't yours :P
	SpawnTreeDespawnerId	m_despawnId;
	CGUID					m_guid;
	Uint32					m_ownerWorldHash;
	// temp data
	mutable Vector			m_tmpWorldPos;

private:
	SStoryPhaseLocation		m_storyPhaseCurrentOwner;			//!< Data owner

	CCommunitySystem&		m_cs;
	THandle< CActionPointManager >	m_apMan;
	const CTimeManager&		m_timeMgr;
	NewNPCSchedule			m_schedule;

    Bool                    m_forceSpawn;
	Bool					m_disabled;


public:
	SAgentStub( CCommunitySystem& cs, THandle< CActionPointManager > apMan, const CTimeManager& timeMgr, const IdTag& idTag );
	~SAgentStub();

	//! Processes the stub-related logics
	void Tick( Float timeDelta, const GameTime& currGameTime );

	//! Sets a story phase that owns this stub
	void SetStoryPhaseOwner( const SHashedStoryPhaseLocation &storyPhase, CSEntitiesEntry *communityEntitiesEntry );

	//! Replaces the previous schedule with a new one
	Bool ChangeSchedule( const SStoryPhaseLocation &storyPhase );

	//! Is this only a simulation stub not a spawned NPC ?
	Bool IsOnlyStub() const { return !m_npc.Get(); }

	//! Is this stub in a working state ?
	Bool IsWorking() const { return m_state == CAS_WorkInProgress; }

	//! Should we start in action point
	Bool IsStartingInAP() const;

	//! Should we use the same action point
	Bool IsUsingLastAP() const;

	//! Is this stub always spawned ?
	Bool IsAlwaysSpawned() const;

	//! Get stub debug info
	void GetDebugInfo( ICommunityDebugPage& debugPage ) const;

	//! Was agent stub spawned into NPC ?
	Bool WasSpawned() const { return m_appearance == CName::NONE ? false : true; }

	//! Tells if the agent is spawned into NPC ( currently or before the game state was saved )
	RED_INLINE Bool IsSpawned() const { return m_state == CAS_Spawned; }

	RED_INLINE Bool IsFunctional() const { return m_state != CAS_Unknown && m_state != CAS_ToDespawn && m_state != CAS_Despawning; }

	//! Returns the work schedule of the stub
	RED_INLINE const NewNPCSchedule& GetSchedule() const { return m_schedule; }

	RED_INLINE SStoryPhaseLocation* GetStoryPhaseCurrentOwner(){ return &m_storyPhaseCurrentOwner; }

	//! Returns the spawnset from which created the stub
	RED_INLINE const CCommunity* GetParentSpawnset() const { return m_storyPhaseCurrentOwner.m_community.Get(); }

	//! Returns the active story phase under which the stub operates
	RED_INLINE CSStoryPhaseEntry* GetActivePhase() const  { return m_storyPhaseCurrentOwner.m_communityStoryPhaseEntry; }

	//! Returns the line from a spawnset that activated the stub
	RED_INLINE CSTableEntry* GetActiveSpawnsetLine() const { return m_storyPhaseCurrentOwner.m_communityEntry; }

	//! Returns the spawnset entities entry
	RED_INLINE CSEntitiesEntry* GetEntitiesEntry() const { return m_storyPhaseCurrentOwner.m_communityEntitiesEntry; }

	//! Returns true if community agent is disabled (now used in case of corrupted data)
	RED_INLINE Bool IsDisabled() const { return m_disabled; }

	//! Returns the initializers that should initialize a spawned NPC or initialize data on story phase change
	CCommunityInitializers* GetInitializers( Bool storyPhaseInitializers = false ) const;

	//! Checks if the stub was spawned from the specified spawnset
	RED_INLINE Bool IsSpawnedFromSpawnset( CCommunity* spawnset ) const { return m_storyPhaseCurrentOwner.m_community == spawnset; }

	//! Checks if the layer the agent is currently working on is loaded
	Bool IsActionPointLoaded() const;

	//! Can this stub be stolen by given table entry
	CSEntitiesEntry* FindMatchForSteal( CSTableEntry& communityEntry );

	//! Should NPC be spawned outside camera (and despawned on community deactivation)
	Bool IsHiddenSpawn() const;

	//! Returns true if stub is tracked
	Bool HasMappin() const { return m_mapPinId != -1; }

	ECommMapPinType GetMapPinType() const;

	CName GetMapPinTag() const;

	Bool IsNonQuestMapPinTypeSet() const;

	Bool IsForceDespawned() const { return (GetEntitiesEntry() != NULL ? GetEntitiesEntry()->IsForceDespawned() : true); }

    Bool ForceSpawn() const { return m_forceSpawn; };
    void SetForceSpawn() { m_forceSpawn = true; };

	CEntity* GetSpawnJobEntity();

	// ------------------------------------------------------------------------
	// Debug
	// ------------------------------------------------------------------------

	//! Returns a debug description of the stub
	String GetDescription() const;

	//! Returns the name of the active story phase for this stub
	CName GetActivePhaseName() const;

	//! Returns the name of the parent spawnset from which the stub was created
	String GetSpawnsetName() const;

	CName GetAppearanceName() const { return m_appearance; }

    RED_INLINE const THandle< CCommunity >& GetCommunity() const { return m_storyPhaseCurrentOwner.m_community; };

	RED_INLINE const CGUID& GetGUID() const { return m_guid; }
	// ------------------------------------------------------------------------
	// IAgent implementation
	// ------------------------------------------------------------------------
	const Vector& GetWorldPositionRef() const;
	Bool OnLODChanged( EAgentLOD newLod );
	const ISpawnStrategy& GetSpawnStrategy() const;
	EAgentLOD GetLOD() const { return m_npc.Get() != NULL ? ALOD_CloseDistance : ALOD_Invisible; }

	// -------------------------------------------------------------------------
	// IWorkQuery implementation
	// -------------------------------------------------------------------------
	void AcquireNextAP();

public:
	//! Save state to the save game
	void SaveState( IGameSaver* writer );

	//! Restore state from save game
	static SAgentStub* RestoreState( IGameLoader* loader, CCommunitySystem* community, const SActiveStoryPhaseLocation::List& phasesLookup, const GameTime& currGameTime  );

	void DespawnAgent( Bool isForceDespawn = false );
	void DespawnAgentStub();

	enum EAgentSpawnResult
	{
		ASR_Spawned,
		ASR_Loading,
		ASR_Failed
	};

    EAgentSpawnResult SpawnAgentNPC( Bool asyncLoad = true );
	void CalculateSpawnPosition();
private:
	Bool DespawnAgentNPC();
	void OnTimetableChanged();

	//! Find despawn point
	Bool FindDespawnPoint( Vector &despawnPoint /* out */ );

	void ChangeAgentStubState( ECommunityAgentState newState );

	Bool CheckAgentStubApTimetableMatch();
	Bool DoesAPMatchTimetable( const TDynArray< CSStoryPhaseTimetableACategoriesTimetableEntry > &timetable, const TActionPointID apID, CName *matchCategory = NULL  );
	void PlaceCommunityAgent( const Vector& pos ) const;	

	// ------------------------------------------------------------------------
	// AI processing
	// ------------------------------------------------------------------------
	void ProcessSpawnedAgentStub();
	ECommunityAgentState InitAfterCreated( Float deltaTime );
    ECommunityAgentState WaitingToSpawnInSpawnPoint( Float deltaTime );
	ECommunityAgentState SetActionPointForAgentStub();
	ECommunityAgentState ProcessReadyToWork();
	ECommunityAgentState ProcessAgentStubWithNoActionPointFound( Float timeDelta );
	ECommunityAgentState ProcessWorkInProgressAgentStub( Float timeDelta );
	ECommunityAgentState ProcessMovingToActionPointAgentStub( Float timeDelta );

	void HACKSetStartInAPAndHide();
};

///////////////////////////////////////////////////////////////////////////////
