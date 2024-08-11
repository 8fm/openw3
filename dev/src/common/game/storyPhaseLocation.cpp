#include "build.h"
#include "storyPhaseLocation.h"

#include "communityAgentStub.h"
#include "communitySystem.h"
#include "../core/diskFile.h"
#include "../core/gameSave.h"

///////////////////////////////////////////////////////////////////////////////
// SStoryPhaseLocation
///////////////////////////////////////////////////////////////////////////////
SStoryPhaseLocation::SStoryPhaseLocation() 
	: m_communityEntry( NULL )
	, m_communityStoryPhaseEntry( NULL )
	, m_communityEntitiesEntry( NULL )
	, m_isWorldValid( ASPLWV_Unknown )
{}

SStoryPhaseLocation::SStoryPhaseLocation( CCommunity *community, CSTableEntry *communityEntry, CSStoryPhaseEntry *communityStoryPhase )
	: m_community( community )
	, m_communityEntry( communityEntry )
	, m_communityStoryPhaseEntry( communityStoryPhase )
	, m_communityEntitiesEntry( NULL )
	, m_isWorldValid( ASPLWV_Unknown )
{
}

Uint32 SStoryPhaseLocation::CalculateHash()
{
	return GetPtrHash( m_community.Get() ) ^ GetPtrHash( m_communityStoryPhaseEntry );
}

Bool SStoryPhaseLocation::IsWorldValid()
{
	if( m_isWorldValid == ASPLWV_Unknown )
	{
		Bool entryValid = m_communityEntry->IsActivePhaseValidWorld( m_community );
		m_isWorldValid = ( entryValid ? ASPLWV_Valid :  ASPLWV_Invalid );
	}
	return m_isWorldValid == ASPLWV_Valid;
}

Bool SStoryPhaseLocation::operator==( const SStoryPhaseLocation &arg ) const
{
	return m_communityStoryPhaseEntry == arg.m_communityStoryPhaseEntry;
	//if ( m_communityEntitiesEntry == NULL || arg.m_communityEntitiesEntry == NULL )
	//{
	//	// m_communityStoryPhaseEntry uniquely identifies structure 
	//	return m_communityStoryPhaseEntry == arg.m_communityStoryPhaseEntry;
	//}
	//else
	//{
	//	return m_communityStoryPhaseEntry == arg.m_communityStoryPhaseEntry &&
	//		m_communityEntitiesEntry == arg.m_communityEntitiesEntry;
	//}
}

Bool SStoryPhaseLocation::DoesReplace( const SStoryPhaseLocation &arg ) const
{
	return m_community == arg.m_community && m_communityEntry == arg.m_communityEntry;
}

const CSStoryPhaseTimetableEntry* SStoryPhaseLocation::GetAgentStoryPhaseTimetableEntry() const
{
	if ( !m_community || !m_communityStoryPhaseEntry )
	{
		return NULL;
	}

	return m_community->GetTimetable( m_communityStoryPhaseEntry->m_timetableName );
}

void SStoryPhaseLocation::SaveState( IGameSaver* saver )
{
	CGameSaverBlock block( saver, CNAME(storyPhaseLocation) );

	// Community path
	const Bool hasCommunityPath = m_community && m_community->GetFile();
	saver->WriteValue( CNAME(hasCommunity), hasCommunityPath );
	if ( hasCommunityPath )
	{
		String communityPath = m_community->GetDepotPath();
		saver->WriteValue( CNAME(community), communityPath );
	}

	// Entry
	const Bool hasEntry = (m_communityEntry != NULL);
	saver->WriteValue( CNAME(hasEntry), hasEntry );
	if ( hasEntry )
	{
		const CGUID& communityEntry = m_communityEntry->GetGUID();
		saver->WriteValue( CNAME(entry), communityEntry );
	}

	// Story phase entry
	const Bool hasStoryEntry = (m_communityStoryPhaseEntry != NULL);
	saver->WriteValue( CNAME(hasStoryEntry), hasStoryEntry );
	if ( hasStoryEntry )
	{
		const CGUID& communityStoryPhaseEntry = m_communityStoryPhaseEntry->GetGUID();
		saver->WriteValue( CNAME(storyEntry), communityStoryPhaseEntry );
	}

	// Save entity entry
	const Bool hasEntityEntry = (m_communityEntitiesEntry != NULL);
	saver->WriteValue( CNAME(hasEntityEntry), hasEntityEntry );
	if ( hasEntityEntry )
	{
		const CGUID& communityEntityEntry = m_communityEntitiesEntry->GetGUID();
		saver->WriteValue( CNAME(entityEntry), communityEntityEntry );
	}
}


///////////////////////////////////////////////////////////////////////////////
// SHashedStoryPhaseLocation
///////////////////////////////////////////////////////////////////////////////
Bool SHashedStoryPhaseLocation::operator<( const SHashedStoryPhaseLocation& l ) const
{
	return
		m_hash < l.m_hash ? true :
		m_hash > l.m_hash ? false :
		m_communityStoryPhaseEntry < l.m_communityStoryPhaseEntry ? true :
		m_communityStoryPhaseEntry > l.m_communityStoryPhaseEntry ? false :
		false;
}

///////////////////////////////////////////////////////////////////////////////
// SActiveStoryPhaseLocation
///////////////////////////////////////////////////////////////////////////////
Int32 SActiveStoryPhaseLocation::GetNumberSpawnedAgents( CCommunitySystem* communitySystem )
{
	if ( m_numSpawnedAgents < 0 )
	{
		m_numSpawnedAgents = 0;

		const auto& stubsList = communitySystem->GetAgentsStubs();
		for ( auto it = stubsList.Begin(), end = stubsList.End(); it != end; ++it )
		{
			SAgentStub* stub = *it;
			if ( stub->GetActivePhase() != m_communityStoryPhaseEntry )
			{
				continue;
			}

			if ( stub->m_state == CAS_Despawning || stub->m_state == CAS_ToDespawn )
			{
				continue;
			}

			++m_numSpawnedAgents;
		}
	}

	return m_numSpawnedAgents;
}
Bool SActiveStoryPhaseLocation::RestoreState( SStoryPhaseLocation& loc, IGameLoader* loader, const List& phasesLookup )
{
	CGameSaverBlock block( loader, CNAME(storyPhaseLocation) );

	// Community path
	String communityPath;
	if ( loader->ReadValue< Bool >( CNAME(hasCommunity) ) )
	{
		loader->ReadValue( CNAME(community), communityPath );
	}

	// Entry
	CGUID communityEntry = CGUID::ZERO;
	if ( loader->ReadValue< Bool >( CNAME(hasEntry) ) )
	{
		loader->ReadValue( CNAME(entry), communityEntry );
	}

	// Story phase entry
	CGUID communityStoryPhaseEntry = CGUID::ZERO;
	if ( loader->ReadValue< Bool >( CNAME(hasStoryEntry) ) )
	{
		loader->ReadValue( CNAME(storyEntry), communityStoryPhaseEntry );
	}

	// Entity entry
	CGUID communityEntityEntry = CGUID::ZERO;
	if ( loader->ReadValue< Bool >( CNAME(hasEntityEntry) ) )
	{
		loader->ReadValue( CNAME(entityEntry), communityEntityEntry );
	}

	// Search for matching location
	for ( auto it = phasesLookup.Begin(), end = phasesLookup.End(); it != end; ++it )
	{
		const SStoryPhaseLocation& l = *it;

		if ( l.m_communityEntry && l.m_communityEntry->GetGUID() == communityEntry )
		{
			if ( l.m_communityStoryPhaseEntry && l.m_communityStoryPhaseEntry->GetGUID() == communityStoryPhaseEntry )
			{
				if ( l.m_community && l.m_community->GetFile() && l.m_community->GetFile()->GetDepotPath() == communityPath )
				{
					// Find entity entry in table entry
					CSEntitiesEntry* entityEntry = NULL;
					for ( Uint32 i=0; i<l.m_communityEntry->m_entities.Size(); ++i )
					{
						CSEntitiesEntry* test = &l.m_communityEntry->m_entities[i];
						if ( test->GetGUID() == communityEntityEntry )
						{
							// Found matching one, copy location info
							loc.m_community = l.m_community;
							loc.m_communityEntry = l.m_communityEntry;
							loc.m_communityStoryPhaseEntry = l.m_communityStoryPhaseEntry;
							loc.m_communityEntitiesEntry = test;
							return true;
						}
					}
				}
			}
		}
	}

	// Not found
	return false;
}