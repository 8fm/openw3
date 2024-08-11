/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */
#pragma once


struct SStoryPhaseLocation;

/// NPC Schedule
class NewNPCSchedule
{
public:
	const SStoryPhaseLocation*									m_storyPhase;
	TDynArray< CSStoryPhaseTimetableACategoriesTimetableEntry > m_timetable;
	const CSStoryPhaseTimetableACategoriesTimetableEntry*       m_lastUsedTimetabEntry;

	//! Action Points
	TActionPointID			m_activeActionPointID;				//!< Holds AP ID all the time when NPC is using it
	TActionPointID			m_lastActionPointID;				//!< Holds AP ID that was used before
	//THandle< CLayerInfo >	m_activeLayerInfo;
	CName					m_actionCategoryName;				//!< Action category 
	//TagList					m_actionPointTags;													// Deprecated
	Bool					m_isUsingLastActionPoint;			//!< NPC will try to use the last action point on which it worked

public:
	NewNPCSchedule();

	void AquireNextAP( const CNewNPC* npc, Bool forceSearch = false, Float maxDistance = 0.0f );
};

///////////////////////////////////////////////////////////////////////////////

class NewNPCScheduleProxy
{
private:
	NewNPCSchedule*															m_schedule;

public:
	NewNPCScheduleProxy() : m_schedule( NULL ) {}

	//! Sets the proxied schedule
	RED_INLINE void Set( NewNPCSchedule& schedule ) { m_schedule = &schedule; }

	RED_INLINE NewNPCSchedule* Get() const { return m_schedule; }

	RED_INLINE TActionPointID GetActiveAP() const { return m_schedule != NULL ? m_schedule->m_activeActionPointID : ActionPointBadID; }

	RED_INLINE TActionPointID GetLastAP() const { return m_schedule != NULL ? m_schedule->m_lastActionPointID : ActionPointBadID; }

	RED_INLINE CName GetActionCategoryName() const { return m_schedule != NULL ? m_schedule->m_actionCategoryName : CName::NONE; }

	RED_INLINE Bool UsesLastAP() const { return m_schedule != NULL ? m_schedule->m_isUsingLastActionPoint : false; }

	RED_INLINE void AquireNextAP( const CNewNPC* npc, Float maxDistance = 0.0f ) const { if ( m_schedule != NULL ) { m_schedule->AquireNextAP( npc, false, maxDistance ); } }	// Not sure if necessary

	const TDynArray< CSStoryPhaseTimetableACategoriesTimetableEntry >& GetTimetable() const;
};
