#include "build.h"
#include "undoManager.h"
#include "questEditorUndoStep.h"
#include "../../common/engine/graphContainer.h"

CQuestEditorAddBlockUndoStep::CQuestEditorAddBlockUndoStep( CEdUndoManager *undoManager, IGraphContainer *graph, CGraphBlock *block, wxPoint position )
: IUndoStep( *undoManager )
, m_graph( graph )
, m_block( block )
, m_position( position )
{
	ASSERT( m_graph );
	ASSERT( m_block );

	DoRedo();
	PushStep();
}

void CQuestEditorAddBlockUndoStep::DoUndo()
{
	if ( m_graph->ModifyGraphStructure() )
	{
		TDynArray< CGraphBlock* >& blocks = m_graph->GraphGetBlocks();
		if ( blocks.Exist( m_block ) )
		{
			blocks.Remove( m_block );
			m_block->BreakAllLinks();
			m_block->OnDestroyed();
			m_graph->GraphStructureModified();
		}
	}
}

void CQuestEditorAddBlockUndoStep::DoRedo()
{
	if ( m_graph->ModifyGraphStructure() )
	{
		GraphBlockSpawnInfo info( m_block->GetClass() );
		info.m_position = Vector( m_position.x, m_position.y, 0, 1 );;

		m_graph->GraphGetBlocks().PushBack( m_block );
		m_block->OnSpawned( info );
		m_block->OnRebuildSockets();
		m_graph->GraphStructureModified();
	}
}
