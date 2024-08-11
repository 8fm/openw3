
#include "build.h"
#include "undoQuestGraph.h"
#include "undoProperty.h"
#include "../../common/game/questGraph.h"
#include "../../common/game/questVariedInputsBlock.h"
#include "../../common/game/questRandomBlock.h"

IMPLEMENT_ENGINE_CLASS( CUndoQuestGraphBlockIO );

/*static*/ 
void CUndoQuestGraphBlockIO::CreateAddingStep( CEdUndoManager& undoManager, CEdQuestGraphEditor* graphEditor, CQuestGraphBlock* block, Type type )
{
	CUndoQuestGraphBlockIO* step = new CUndoQuestGraphBlockIO( undoManager, graphEditor, block, type, true );
	step->PushStep();
}

/*static*/ 
void CUndoQuestGraphBlockIO::CreateRemovingStep( CEdUndoManager& undoManager, CEdQuestGraphEditor* graphEditor, CQuestGraphBlock* block, Type type )
{
	CUndoQuestGraphBlockIO* step = new CUndoQuestGraphBlockIO( undoManager, graphEditor, block, type, false );
	step->PushStep();
}

CUndoQuestGraphBlockIO::CUndoQuestGraphBlockIO( CEdUndoManager& undoManager, CEdQuestGraphEditor* graphEditor, CQuestGraphBlock* block, Type type, Bool adding )
	: CUndoGraphSocketSnaphot( undoManager, graphEditor, block )
	, m_type( type )
	, m_adding( adding )
{
}

void CUndoQuestGraphBlockIO::DoStep( Bool adding )
{
	switch ( m_type )
	{
	case TERMINATION_INPUT:
		{
			CQuestGraphBlock* b = SafeCast< CQuestGraphBlock >( GetBlock() );
			b->m_hasTerminationInput = adding;
		}
		break;
	case PATCH_OUTPUT:
		{
			CQuestGraphBlock* b = SafeCast< CQuestGraphBlock >( GetBlock() );
			b->m_hasPatchOutput = adding;
		}
		break;
	default:
		ASSERT( false, TXT( "Not supported QuestGraph IO type" ) );
		break;
	}
}

/*virtual*/ 
void CUndoQuestGraphBlockIO::DoUndo()
{
	DoStep( !m_adding );
	CUndoGraphSocketSnaphot::DoUndo(); // calls GraphStructureModified
}

/*virtual*/ 
void CUndoQuestGraphBlockIO::DoRedo()
{
	DoStep( m_adding );
	CUndoGraphSocketSnaphot::DoRedo(); // calls GraphStructureModified
}

/*virtual*/ 
String CUndoQuestGraphBlockIO::GetName()
{
	String res = m_adding ? TXT( "adding " ) : TXT( "removing " );
	switch ( m_type )
	{
	case TERMINATION_INPUT:
		return res + TXT( "termination input" );
	case PATCH_OUTPUT:
		return res + TXT( "patch output" );
	default:
		ASSERT( false, TXT( "Not supported QuestGraph IO type" ) );
		return res + TXT( "?" );
	}
}

// --------------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoQuestGraphRandomBlockOutput );

/*static*/ 
void CUndoQuestGraphRandomBlockOutput::CreateAddingStep( CEdUndoManager& undoManager, CEdQuestGraphEditor* graphEditor, CQuestRandomBlock* block )
{
	CUndoQuestGraphRandomBlockOutput* step = new CUndoQuestGraphRandomBlockOutput( undoManager, graphEditor, block, true );
	step->PushStep();
}

/*static*/ 
void CUndoQuestGraphRandomBlockOutput::CreateRemovingStep( CEdUndoManager& undoManager, CEdQuestGraphEditor* graphEditor, CQuestRandomBlock* block )
{
	CUndoQuestGraphRandomBlockOutput* step = new CUndoQuestGraphRandomBlockOutput( undoManager, graphEditor, block, false );
	step->PushStep();
}

CUndoQuestGraphRandomBlockOutput::CUndoQuestGraphRandomBlockOutput( CEdUndoManager& undoManager, CEdQuestGraphEditor* graphEditor, CQuestRandomBlock* block, Bool adding )
	: CUndoGraphSocketSnaphot( undoManager, graphEditor, block )
	, m_adding( adding )
{
	m_randomOutputs = block->m_randomOutputs;
}

void CUndoQuestGraphRandomBlockOutput::DoStep()
{
	Swap( SafeCast< CQuestRandomBlock >( GetBlock() )->m_randomOutputs, m_randomOutputs );	
}

/*virtual*/ 
void CUndoQuestGraphRandomBlockOutput::DoUndo()
{
	DoStep();
	CUndoGraphSocketSnaphot::DoUndo(); // calls GraphStructureModified
}

/*virtual*/ 
void CUndoQuestGraphRandomBlockOutput::DoRedo()
{
	DoStep();
	CUndoGraphSocketSnaphot::DoRedo(); // calls GraphStructureModified
}

/*virtual*/ 
String CUndoQuestGraphRandomBlockOutput::GetName()
{
	return m_adding ? TXT("adding output") : TXT("removing output");
}

// --------------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoQuestGraphVariedInputBlock );

/*static*/ 
void CUndoQuestGraphVariedInputBlock::CreateAddingStep( CEdUndoManager& undoManager, CEdQuestGraphEditor* graphEditor, CQuestVariedInputsBlock* block )
{
	CUndoQuestGraphVariedInputBlock* step = new CUndoQuestGraphVariedInputBlock( undoManager, graphEditor, block, true );
	step->PushStep();
}

/*static*/ 
void CUndoQuestGraphVariedInputBlock::CreateRemovingStep( CEdUndoManager& undoManager, CEdQuestGraphEditor* graphEditor, CQuestVariedInputsBlock* block )
{
	CUndoQuestGraphVariedInputBlock* step = new CUndoQuestGraphVariedInputBlock( undoManager, graphEditor, block, false );
	step->PushStep();
}

CUndoQuestGraphVariedInputBlock::CUndoQuestGraphVariedInputBlock( CEdUndoManager& undoManager, CEdQuestGraphEditor* graphEditor, CQuestVariedInputsBlock* block, Bool adding )
	: CUndoGraphSocketSnaphot( undoManager, graphEditor, block )
	, m_adding( adding )
{
}

void CUndoQuestGraphVariedInputBlock::DoStep( Bool adding )
{
	SafeCast< CQuestVariedInputsBlock >( GetBlock() )->m_inputsCount += adding ? 1 : -1;
}

/*virtual*/ 
void CUndoQuestGraphVariedInputBlock::DoUndo()
{
	DoStep( !m_adding );
	CUndoGraphSocketSnaphot::DoUndo(); // calls GraphStructureModified
}

/*virtual*/ 
void CUndoQuestGraphVariedInputBlock::DoRedo()
{
	DoStep( m_adding );
	CUndoGraphSocketSnaphot::DoRedo(); // calls GraphStructureModified
}

/*virtual*/ 
String CUndoQuestGraphVariedInputBlock::GetName()
{
	return m_adding ? TXT("adding input") : TXT("removing input");
}

// --------------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoQuestGraphPushPop );

/*static*/ 
void CUndoQuestGraphPushPop::CreatePushingStep( CEdUndoManager& undoManager, CEdQuestGraphEditor* graphEditor, TDynArray< CQuestGraph* >* stack )
{
	CUndoQuestGraphPushPop* step = new CUndoQuestGraphPushPop( undoManager, graphEditor, stack, true );
	step->PushStep();
}

/*static*/ 
void CUndoQuestGraphPushPop::CreatePoppingStep( CEdUndoManager& undoManager, CEdQuestGraphEditor* graphEditor, TDynArray< CQuestGraph* >* stack )
{
	CUndoQuestGraphPushPop* step = new CUndoQuestGraphPushPop( undoManager, graphEditor, stack, false );
	step->m_graph = stack->Back();
	step->PushStep();
}

CUndoQuestGraphPushPop::CUndoQuestGraphPushPop( CEdUndoManager& undoManager, CEdQuestGraphEditor* graphEditor, TDynArray< CQuestGraph* >* stack, Bool pushing )
	: CUndoGraphStep( undoManager, graphEditor )
	, m_stack( stack )
	, m_pushing( pushing )
	, m_graph( NULL )
{
}

void CUndoQuestGraphPushPop::DoStep( Bool pushing )
{
	CEdQuestGraphEditor* editor = dynamic_cast< CEdQuestGraphEditor* >( GetEditor() );
	ASSERT( editor != NULL );

	if ( pushing )
	{
		ASSERT( m_graph );
		m_stack->PushBack( m_graph );
		editor->SetGraph( m_graph );
	}
	else
	{
		m_graph = m_stack->PopBack();
		editor->SetGraph( m_stack->Back() );
	}

	GraphStructureModified();	
}

/*virtual*/ 
void CUndoQuestGraphPushPop::DoUndo()
{
	DoStep( !m_pushing );
}

/*virtual*/ 
void CUndoQuestGraphPushPop::DoRedo()
{
	DoStep( m_pushing );
}

/*virtual*/ 
String CUndoQuestGraphPushPop::GetName()
{
	return m_pushing ? TXT("entering the sub-graph") : TXT("returning to the parent graph");
}
