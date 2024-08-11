#include "build.h"

#include "encounterSpawnGroupCounter.h"

void CEncounterSpawnGroupCounter::RegisterActorInGroup( Int32 group )
{
	ASSERT( group >= 0 );
	ASSERT( group < MAX_GROUPS_LIMIT );

	if( group >= 0 && group < MAX_GROUPS_LIMIT )
	{
		++m_spawnedGroupCounters[ group ].m_count;
	}
}

void CEncounterSpawnGroupCounter::UnregisterActorFromGroup( Int32 group )
{
	ASSERT( group >= 0 );
	ASSERT( group < MAX_GROUPS_LIMIT );
	ASSERT( m_spawnedGroupCounters[ group ].m_count > 0 );

	if( group >= 0 && group < MAX_GROUPS_LIMIT )
	{
		--m_spawnedGroupCounters[ group ].m_count;
	}
}

void CEncounterSpawnGroupCounter::ResetConters()
{
	for( Int32 i=0; i<MAX_GROUPS_LIMIT; ++i )
	{
		m_spawnedGroupCounters[ i ].m_count = 0;
	}
}