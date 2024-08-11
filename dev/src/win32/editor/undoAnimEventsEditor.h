/**
  * Copyright © 2013 CD Projekt Red. All Rights Reserved.
  */
#pragma once

#include "undoTimeLine.h"

class CEdAnimEventsEditor;
class CSkeletalAnimationSetEntry;

class CUndoAnimEventsAnimChange : public CUndoTimelineStep
{
    DECLARE_ENGINE_CLASS( CUndoAnimEventsAnimChange, CUndoTimelineStep, 0 );	   

public:
	static void CreateStep( CEdUndoManager& undoManager, CEdTimeline* timeline, CEdAnimEventsEditor* editor );

private:
	CUndoAnimEventsAnimChange() {}
	CUndoAnimEventsAnimChange( CEdUndoManager& undoManager, CEdTimeline* timeline, CEdAnimEventsEditor* editor );

	CSkeletalAnimationSetEntry* GetAnimEntry( ) const;
	void DoStep();

    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;

	CEdAnimEventsEditor*        m_editor;
	CSkeletalAnimationSetEntry* m_animEntry;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoAnimEventsAnimChange, CUndoTimelineStep );
