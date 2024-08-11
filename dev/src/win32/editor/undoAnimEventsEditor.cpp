/**
  * Copyright © 2013 CD Projekt Red. All Rights Reserved.
  */
#include "build.h"

#include "undoAnimEventsEditor.h"
#include "animEventsEditor.h"
#include "timeline.h"

IMPLEMENT_ENGINE_CLASS( CUndoAnimEventsAnimChange );

/*static*/ 
void CUndoAnimEventsAnimChange::CreateStep( CEdUndoManager& undoManager, CEdTimeline* timeline, CEdAnimEventsEditor* editor )
{
	CUndoAnimEventsAnimChange* step = new CUndoAnimEventsAnimChange( undoManager, timeline, editor );
	step->PushStep();
}

CUndoAnimEventsAnimChange::CUndoAnimEventsAnimChange( CEdUndoManager& undoManager, CEdTimeline* timeline, CEdAnimEventsEditor* editor )
	: CUndoTimelineStep( undoManager, timeline )
	, m_editor( editor )
{
	m_animEntry = GetAnimEntry();
}

CSkeletalAnimationSetEntry* CUndoAnimEventsAnimChange::GetAnimEntry( ) const
{
	if ( m_editor->m_playedAnimation )
	{
		return const_cast< CSkeletalAnimationSetEntry* >( m_editor->m_playedAnimation->GetAnimationEntry() );
	}
	else
	{
		return NULL;
	}
}

void CUndoAnimEventsAnimChange::DoStep()
{
	CSkeletalAnimationSetEntry* prevAnim = GetAnimEntry();
	m_editor->SelectAnimation( m_animEntry, true );
	m_editor->UpdateAnimListSelection();
	m_animEntry = prevAnim;
}

/*virtual*/ 
void CUndoAnimEventsAnimChange::DoUndo()
{
	DoStep();
}

/*virtual*/ 
void CUndoAnimEventsAnimChange::DoRedo()
{
	DoStep();
}

/*virtual*/ 
String CUndoAnimEventsAnimChange::GetName()
{
	return TXT("animation change");
}
