#pragma once

#include "../redContainers/doubleList.h"
#include "../engine/layer.h"
#include "../engine/entity.h"

//////////////////////////////////////////////////////////////////////////

class CEncounterSpawnGroupCounter
{
	struct SSpawnGroupCounter
	{
		Uint32	m_count;

		SSpawnGroupCounter()
			: m_count( 0 ) {}
	};
	static const Int32				MAX_GROUPS_LIMIT = 16;
	SSpawnGroupCounter				m_spawnedGroupCounters[ MAX_GROUPS_LIMIT ];	

public:
	RED_INLINE Int32 GetGroupCount( Int32 group ) { return ( group < 0 || group >= MAX_GROUPS_LIMIT ) ? 0 :  m_spawnedGroupCounters[group].m_count; }
	void RegisterActorInGroup( Int32 group );
	void UnregisterActorFromGroup( Int32 group );
	void ResetConters();
};
