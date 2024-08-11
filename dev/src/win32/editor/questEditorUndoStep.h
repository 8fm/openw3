#pragma once

class CEdUndoManager;

#include "undoStep.h"

class CQuestEditorAddBlockUndoStep : public IUndoStep
{

private:

	IGraphContainer *m_graph;
	CGraphBlock *m_block;
	wxPoint m_position;

protected:

	virtual void DoUndo();
	virtual void DoRedo();

public:

	CQuestEditorAddBlockUndoStep( CEdUndoManager *undoManager, IGraphContainer *graph, CGraphBlock *block, wxPoint position );

	virtual String GetName() { return String::Printf( TXT("Add block %s"), m_block->GetCaption() ); }
};