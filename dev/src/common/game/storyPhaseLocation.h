/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once


///////////////////////////////////////////////////////////////////////////////

class CCommunity;
struct CSTableEntry;
struct CSStoryPhaseEntry;
struct CSEntitiesEntry;
struct CSStoryPhaseTimetableEntry;
class IGameSaver;
class IGameLoader;
class CCommunitySystem;
struct SActiveStoryPhaseLocation;

///////////////////////////////////////////////////////////////////////////////

// unique identifier of story phase
struct SStoryPhaseLocation
{
	enum SActiveStoryPhaseLocationWorlValidation
	{
		ASPLWV_Unknown,
		ASPLWV_Valid,
		ASPLWV_Invalid
	};

	THandle< CCommunity >						m_community;
	CSTableEntry*								m_communityEntry;
	CSStoryPhaseEntry*							m_communityStoryPhaseEntry;
	CSEntitiesEntry*							m_communityEntitiesEntry;
	SActiveStoryPhaseLocationWorlValidation		m_isWorldValid;

	SStoryPhaseLocation();
	SStoryPhaseLocation( CCommunity *community, CSTableEntry *communityEntry, CSStoryPhaseEntry *communityStoryPhase );

	Bool operator==( const SStoryPhaseLocation &arg ) const;

	Bool DoesReplace( const SStoryPhaseLocation &arg ) const;

	const CSStoryPhaseTimetableEntry* GetAgentStoryPhaseTimetableEntry() const;

	void SaveState( IGameSaver* saver );
	Uint32 CalculateHash();

	virtual void MarkDirty()					{ m_isWorldValid = ASPLWV_Unknown; }
	Bool IsWorldValid();
};

struct SHashedStoryPhaseLocation : public SStoryPhaseLocation
{
private:
	typedef SStoryPhaseLocation Super;

	Uint32									m_hash;
public:
	SHashedStoryPhaseLocation()
		: Super()
		, m_hash( 0xbaadf00d )													{}
	SHashedStoryPhaseLocation( CCommunity *community, CSTableEntry *communityEntry, CSStoryPhaseEntry *communityStoryPhase )
		: Super( community, communityEntry, communityStoryPhase )				{ m_hash = CalculateHash(); }
	SHashedStoryPhaseLocation( const SStoryPhaseLocation& loc )
		: Super( loc )															{ m_hash = CalculateHash(); }
	SHashedStoryPhaseLocation( const SHashedStoryPhaseLocation& h )
		: Super( h )
		, m_hash( h.m_hash )													{}

	Bool operator<( const SHashedStoryPhaseLocation& l ) const;
};

///////////////////////////////////////////////////////////////////////////////

struct SActiveStoryPhaseLocation : public SHashedStoryPhaseLocation
{
private:
	typedef SHashedStoryPhaseLocation Super;
	static const Int32 AGENTS_UNCALCULATED = -1;	

public:
	struct Order
	{
		static Bool Less( const SHashedStoryPhaseLocation& l1, const SHashedStoryPhaseLocation& l2 ) { return l1 < l2; }
	};
	typedef TSortedArray< SActiveStoryPhaseLocation, Order > List;

	Int32										m_numSpawnedAgents;	

	SActiveStoryPhaseLocation()
		: Super()
		, m_numSpawnedAgents( AGENTS_UNCALCULATED )								{}
	SActiveStoryPhaseLocation( CCommunity *community, CSTableEntry *communityEntry, CSStoryPhaseEntry *communityStoryPhase )
		: Super( community, communityEntry, communityStoryPhase )
		, m_numSpawnedAgents( AGENTS_UNCALCULATED )								
		{}


	Int32 GetNumberSpawnedAgents( CCommunitySystem* communitySystem );

	static Bool RestoreState( SStoryPhaseLocation& loc, IGameLoader* loader, const List& phasesLookup );

	void AddSpawned()				{ if ( m_numSpawnedAgents >= 0 ) ++m_numSpawnedAgents; }
	void DelSpawned()				{ if ( m_numSpawnedAgents > 0 ) --m_numSpawnedAgents; }
	void MarkDirty() override		{ m_numSpawnedAgents = -1; Super::MarkDirty(); }	
};

