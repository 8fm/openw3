/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "undoStep.h"

class CUndoToolSwitch : public IUndoStep
{
	CUndoToolSwitch() {}
	DECLARE_ENGINE_CLASS( CUndoToolSwitch, IUndoStep, 0 );

private:
	Bool			m_undoToolStart;
	CEdToolsPanel *	m_toolsPanel;
	CClass *		m_tool;

protected:
	virtual void DoUndo();
	virtual void DoRedo();

	CUndoToolSwitch( CEdUndoManager& undoManager, CClass * tool, CEdToolsPanel * toolsPanel, Bool undoToolStart )
		: IUndoStep           ( undoManager )
		, m_undoToolStart( undoToolStart )
		, m_toolsPanel( toolsPanel )
		, m_tool( tool )
	{}

public:
	virtual String GetName()
	{
		return m_tool->GetDefaultObject<CObject>()->GetFriendlyName() + ( m_undoToolStart ? TXT(" opened") : TXT(" closed") );
	}

	static void CreateStep( CEdUndoManager& undoManager, CClass * tool, CEdToolsPanel * toolsPanel, Bool undoToolStart );
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoToolSwitch, IUndoStep );
