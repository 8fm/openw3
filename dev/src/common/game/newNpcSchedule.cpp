/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "newNpcSchedule.h"

#include "storyPhaseLocation.h"
#include "communitySystem.h"


const TDynArray< CSStoryPhaseTimetableACategoriesTimetableEntry >& NewNPCScheduleProxy::GetTimetable() const
{
	if ( m_schedule != NULL )
	{
		return m_schedule->m_timetable;
	}
	static TDynArray< CSStoryPhaseTimetableACategoriesTimetableEntry > s_emptyTimetable;
	return s_emptyTimetable;
}

NewNPCSchedule::NewNPCSchedule()
	: m_isUsingLastActionPoint( false )
	, m_activeActionPointID( ActionPointBadID )
	, m_lastActionPointID( ActionPointBadID )
	, m_lastUsedTimetabEntry( NULL )
	, m_storyPhase( NULL )
{

}

void NewNPCSchedule::AquireNextAP( const CNewNPC* npc, Bool forceSearch /*= false */, Float maxDistance /* = 0.0f */ )
{
	CCommunitySystem* communitySystem = GCommonGame->GetSystem< CCommunitySystem >();
	if ( communitySystem == NULL )
	{
		return;
	}

	CActionPointManager* actionPointManager = communitySystem->GetActionPointManager();
	if ( actionPointManager == NULL )
	{
		return;
	}

	if ( m_activeActionPointID != ActionPointBadID && forceSearch == false && m_isUsingLastActionPoint )
	{
		// we have an AP assigned and we're told to stick to it - bail
		return;
	}

	// memorize the last used action point
	m_lastActionPointID = m_activeActionPointID;

	// if the agent is working somewhere, release that AP
	if ( m_activeActionPointID != ActionPointBadID )
	{
		actionPointManager->SetFree( m_activeActionPointID, CActionPointManager::REASON_COMMUNITY );
	}

	// find new
	if ( forceSearch == false &&
		m_activeActionPointID != ActionPointBadID && 
		actionPointManager->HasPreferredNextAPs( m_activeActionPointID ) )
	{
		// Use next action point in sequence
		m_activeActionPointID = actionPointManager->FindNextFreeActionPointInSequence( m_activeActionPointID );
		if ( m_activeActionPointID != ActionPointBadID && npc && maxDistance > 0.0f )
		{
			const Float distanceSq = actionPointManager->GetAP( m_activeActionPointID )->GetWorldPositionRef().DistanceSquaredTo( npc->GetWorldPositionRef() );
			if ( distanceSq > maxDistance * maxDistance )
			{
				m_activeActionPointID = ActionPointBadID;
			}
		}
	}
	else
	{
		m_activeActionPointID = ActionPointBadID;

		if ( m_timetable.Empty() )
		{
			// no timetable - nothing to look for
			return;
		}

		SActionPointFilter apFilter;
		communitySystem->FindCurrentActionPointFilter( m_timetable, apFilter );
		apFilter.m_onlyFree = true;
		apFilter.m_askingNPC = npc;
		if ( npc && maxDistance > 0.0f )
		{
			apFilter.m_sphere = Sphere( npc->GetWorldPositionRef(), maxDistance );
		}

		if ( actionPointManager->FindActionPoint( m_activeActionPointID, apFilter ) != FAPR_Success )
		{
			m_activeActionPointID = ActionPointBadID;
		}
		else
		{
			m_actionCategoryName = apFilter.m_category;
		}
	}

	if ( m_activeActionPointID != ActionPointBadID )
	{
		actionPointManager->SetReserved( m_activeActionPointID, CActionPointManager::REASON_COMMUNITY, npc );
	}
}

