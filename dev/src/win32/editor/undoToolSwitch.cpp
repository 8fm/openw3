#include "build.h"

#include "undoToolSwitch.h"
#include "undoManager.h"
#include "toolsPanel.h"

IMPLEMENT_ENGINE_CLASS( CUndoToolSwitch );

void CUndoToolSwitch::CreateStep( CEdUndoManager& undoManager, CClass * tool, CEdToolsPanel * toolsPanel, Bool undoToolStart )
{
	( new CUndoToolSwitch( undoManager, tool, toolsPanel, undoToolStart ) )->PushStep();
}

void CUndoToolSwitch::DoUndo()
{
	if ( m_undoToolStart )
	{
		m_toolsPanel->CancelTool();
	}
	else
	{
		m_toolsPanel->StartTool( m_tool );
	}
}

void CUndoToolSwitch::DoRedo()
{
	if ( ! m_undoToolStart )
	{
		m_toolsPanel->CancelTool();
	}
	else
	{
		m_toolsPanel->StartTool( m_tool );
	}
}