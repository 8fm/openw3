/**
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */
#pragma once
#include "../game/binaryStorage.h"

///////////////////////////////////////////////////////////////////////////////

class IAgent;
class AllAgentsIterator;
class SpawnedAgentsIterator;
class NearbyAgentsIterator;
class IAgentsBudgetingStrategy;
struct SAgentStub;

///////////////////////////////////////////////////////////////////////////////

/// Stores all AI agents. Since agents are not necessarily entities, we cannot
/// relay on the CWorld class to contain them - thus a need for a separate
/// world class that will also contain various agent-related queries.
class CAgentsWorld : public Red::System::NonCopyable
{
private:

	static const Int32 MAX_NPC_SPAWNED	= 20;

	TDynArray< IAgent* >						m_allAgents;

	Uint32										m_spawnedCount;
	Uint32										m_updateIndex;
    Bool                                        m_restoringState;
	Bool										m_forceLodUpdate;

public:
	CAgentsWorld();
	~CAgentsWorld();

	//! Executes the LOD management process.
	void Update( Float deltaTime );

	// ------------------------------------------------------------------------
	// Agents management
	// ------------------------------------------------------------------------

	//! Adds a new agent to the world
	void AddAgent( IAgent* agent );

	//! Removes an agent from the world
	void RemoveAgent( IAgent* agent );

	//! Resets the world contents
	void Reset();

	void ForceFullLodUpdate();

	// ------------------------------------------------------------------------
	// Queries
	// ------------------------------------------------------------------------

#if 0
	RED_MESSAGE("FIXME>>>>> Don't make noncopyable and then copy")

	//! Returns all registered agents
	AllAgentsIterator GetAllAgents() const;

	//! Returns an iterator that will iterate over the spawned agents
	SpawnedAgentsIterator GetSpawnedAgents() const;
#endif

	//! Returns an iterator over the agents that are near the specified position.
	NearbyAgentsIterator GetNearbyAgents( const Vector& pos, Float maxDist ) const;

	//! Returns the number of all registered agents
	RED_INLINE Uint32 GetAgentsCount() const { return m_allAgents.Size(); }

	//! Checks how many agents are visible at the moment.
	RED_INLINE Uint32 GetSpawnedAgentsCount() const { return m_spawnedCount; }

    RED_INLINE void EnableRestoringState( Bool enable ) { m_restoringState = enable; }

	void OnGenerateDebugFragments( CRenderFrame* frame );

	struct StubDesc
	{
		SAgentStub* m_agent;
		Float m_distance;
		String m_communityName;
	};

	Bool Dump();
	void GetDumpMessage( const TDynArray< StubDesc >& alwaysSpawned, String& message );
	void GetAlwaysSpawnedAgents( TDynArray< StubDesc >& alwaysSpawned );
};
///////////////////////////////////////////////////////////////////////////////
// An iterator that iterates over all registered agents.
class AllAgentsIterator : public Red::System::NonCopyable
{
private:
	const TDynArray< IAgent* >&		m_agents;
	Uint32							m_currAgent;

public:
	RED_INLINE operator Bool () const
	{
		return m_currAgent < m_agents.Size();
	}

	void operator++ ()
	{
		++m_currAgent;
	}

	RED_INLINE IAgent* operator*()
	{
		return m_agents[ m_currAgent ];
	}

private:
	AllAgentsIterator( const TDynArray< IAgent* >& agents ) : m_agents( agents ), m_currAgent( 0 ) {}

	friend class CAgentsWorld;
};
///////////////////////////////////////////////////////////////////////////////
/// An iterator that can iterator over the spawned agents.
class SpawnedAgentsIterator : public Red::System::NonCopyable
{
private:
	const TDynArray< IAgent* >&		m_agents;
	Uint32							m_currAgent;

public:
	RED_INLINE operator Bool () const
	{
		return m_currAgent < m_agents.Size();
	}

	void operator++ ();

	RED_INLINE IAgent* operator*()
	{
		return m_agents[ m_currAgent ];
	}

private:
	SpawnedAgentsIterator( const TDynArray< IAgent* >& agents ) : m_agents( agents ), m_currAgent( 0 ) {}

	friend class CAgentsWorld;
};
///////////////////////////////////////////////////////////////////////////////

/// An iterator that iterates over the agents closed to the specified position
class NearbyAgentsIterator
{
private:
	TDynArray< TPointerWrapper< IAgent > >		m_agents;
	Uint32										m_currAgent;

public:
	RED_INLINE operator Bool () const
	{
		return m_currAgent < m_agents.Size();
	}

	RED_INLINE void operator++ ()
	{
		++m_currAgent;
	}

	RED_INLINE IAgent* operator*()
	{
		return m_agents[ m_currAgent ].Get();
	}

private:
	NearbyAgentsIterator( const TDynArray< TPointerWrapper< IAgent > >& agents ) : m_agents( agents ), m_currAgent( 0  ) {}

	friend class CAgentsWorld;
};

///////////////////////////////////////////////////////////////////////////////
