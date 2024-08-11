/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "storyPhaseListener.h"
class CScenesEntriesManager : public CStoryPhaseListener
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );
	friend class CCommunitySceneMappingListener;

public:
	// constructors
	CScenesEntriesManager( TDynArray< THandle< CCommunity > >& spawnsets )
		: CStoryPhaseListener( spawnsets ), m_nextPhaseToProcess(0), m_updateTimeout(3.f) {}

	void OnTick( Float timeDelta );

private:
	void StartSceneFromTimetable( const CCommunity & community, const CSSceneTableEntry & storyphase, const CSSceneTimetableEntry & commTimeTabEntry );

	Uint32								m_nextPhaseToProcess;
	Float								m_updateTimeout;
	THashMap< const void*, Float >		m_timeouts;	//!< SceneInput and StoryPhase timeouts
};
