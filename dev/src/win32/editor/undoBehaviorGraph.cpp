
#include "build.h"
#include "../../common/engine/behaviorGraphBlendMultipleNode.h"
#include "../../common/engine/behaviorGraphRandomNode.h"
#include "../../common/engine/behaviorGraphAnimationSwitchNode.h"
#include "undoBehaviorGraph.h"
#include "undoProperty.h"
#include "behaviorVariableEditor.h"
#include "../../common/engine/behaviorGraph.h"

IMPLEMENT_ENGINE_CLASS( CUndoBehaviorGraphSetRoot );

CUndoBehaviorGraphSetRoot::CUndoBehaviorGraphSetRoot( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* editor )
	: CUndoGraphStep( undoManager, editor )
	{ }

/*static*/ 
void CUndoBehaviorGraphSetRoot::CreateStep( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* editor )
{
	CUndoBehaviorGraphSetRoot* step = new CUndoBehaviorGraphSetRoot( undoManager, editor );
	step->m_node = editor->GetRootNode();
	step->PushStep();
}

void CUndoBehaviorGraphSetRoot::DoStep()
{
	CEdBehaviorGraphEditor* editor = dynamic_cast< CEdBehaviorGraphEditor* >( GetEditor() );
	ASSERT( editor != NULL );
	CBehaviorGraphContainerNode* curNode = editor->GetRootNode();
	editor->SetRootNode( m_node );
	m_node = curNode;
	GraphStructureModified();
}

/*virtual*/ 
void CUndoBehaviorGraphSetRoot::DoUndo()
{
	DoStep();
}

/*virtual*/ 
void CUndoBehaviorGraphSetRoot::DoRedo()
{
	DoStep();
}

/*virtual*/ 
String CUndoBehaviorGraphSetRoot::GetName()
{
	return TXT("setting visible graph");
}


//-----------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoBehaviorGraphContainerNodeInput );

/*static*/ 
void CUndoBehaviorGraphContainerNodeInput::CreateAddingStep( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CBehaviorGraphContainerNode* block, Type type, const CName& name )
{
	CUndoBehaviorGraphContainerNodeInput* step = new CUndoBehaviorGraphContainerNodeInput( undoManager, graphEditor, block, type, name, true );
	step->PushStep();
}

/*static*/ 
void CUndoBehaviorGraphContainerNodeInput::CreateRemovingStep( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CBehaviorGraphContainerNode* block, Type type, const CName& name )
{
	CUndoBehaviorGraphContainerNodeInput* step = new CUndoBehaviorGraphContainerNodeInput( undoManager, graphEditor, block, type, name, false );
	step->PushStep();
}

CUndoBehaviorGraphContainerNodeInput::CUndoBehaviorGraphContainerNodeInput( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CBehaviorGraphContainerNode* block, Type type, const CName& name, Bool adding )
	: CUndoGraphSocketSnaphot( undoManager, graphEditor, block )
	, m_type( type )
	, m_name( name )
	, m_adding( adding )
{
}

void CUndoBehaviorGraphContainerNodeInput::DoStep( Bool adding )
{
	CBehaviorGraphContainerNode* block = SafeCast< CBehaviorGraphContainerNode >( GetBlock() );
	TDynArray< CName >* socketStack;

	switch ( m_type )
	{
	case ANIM_INPUT:
		socketStack = &block->m_animationInputs;
		break;
	case VALUE_INPUT:
		socketStack = &block->m_valueInputs;
		break;
	case VECTOR_VALUE_INPUT:
		socketStack = &block->m_vectorValueInputs;
		break;
	case MIMIC_INPUT:
		socketStack = &block->m_mimicInputs;
		break;
	default:
		ASSERT( false, TXT( "Not supported BehaviorGraph input type" ) );
		return;
	}

	if ( adding )
	{
		socketStack->PushBack( m_name );
	}
	else
	{
		socketStack->Remove( m_name );
	}
}

/*virtual*/ 
void CUndoBehaviorGraphContainerNodeInput::DoUndo()
{
	DoStep( !m_adding );
	CUndoGraphSocketSnaphot::DoUndo(); // calls GraphStructureModified
}

/*virtual*/ 
void CUndoBehaviorGraphContainerNodeInput::DoRedo()
{
	DoStep( m_adding );
	CUndoGraphSocketSnaphot::DoRedo(); // calls GraphStructureModified
}

/*virtual*/ 
String CUndoBehaviorGraphContainerNodeInput::GetName()
{
	String res = m_adding ? TXT( "adding " ) : TXT( "removing " );
	switch ( m_type )
	{
	case ANIM_INPUT:
		return res + TXT( "animation input" );
	case VALUE_INPUT:
		return res + TXT( "value input" );
	case VECTOR_VALUE_INPUT:
		return res + TXT( "vector value input" );
	case MIMIC_INPUT:
		return res + TXT( "mimic input" );
	default:
		ASSERT( false, TXT( "Not supported BehaviorGraph input type" ) );
		return res + TXT( "?" );
	}
}

// -------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoBehaviorGraphBlendNodeInput );

/*static*/ 
void CUndoBehaviorGraphBlendNodeInput::CreateAddingStep( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CBehaviorGraphBlendMultipleNode* block )
{
	CUndoBehaviorGraphBlendNodeInput* step = new CUndoBehaviorGraphBlendNodeInput( undoManager, graphEditor, block, true );
	step->PushStep();
}

/*static*/ 
void CUndoBehaviorGraphBlendNodeInput::CreateRemovingStep( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CBehaviorGraphBlendMultipleNode* block )
{
	CUndoBehaviorGraphBlendNodeInput* step = new CUndoBehaviorGraphBlendNodeInput( undoManager, graphEditor, block, false );
	step->PushStep();
}

CUndoBehaviorGraphBlendNodeInput::CUndoBehaviorGraphBlendNodeInput( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CBehaviorGraphBlendMultipleNode* block, Bool adding )
	: CUndoGraphSocketSnaphot( undoManager, graphEditor, block )
	, m_adding( adding )
{
	m_inputValues = block->m_inputValues;
}

void CUndoBehaviorGraphBlendNodeInput::DoStep( Bool adding )
{
	CBehaviorGraphBlendMultipleNode* block = SafeCast< CBehaviorGraphBlendMultipleNode >( GetBlock() );
	Swap( m_inputValues, block->m_inputValues );
}

/*virtual*/ 
void CUndoBehaviorGraphBlendNodeInput::DoUndo()
{
	DoStep( !m_adding );
	CUndoGraphSocketSnaphot::DoUndo(); // calls GraphStructureModified
}

/*virtual*/ 
void CUndoBehaviorGraphBlendNodeInput::DoRedo()
{
	DoStep( m_adding );
	CUndoGraphSocketSnaphot::DoRedo(); // calls GraphStructureModified
}

/*virtual*/ 
String CUndoBehaviorGraphBlendNodeInput::GetName()
{
	return m_adding ? TXT("adding input") : TXT("removing input");
}

// -------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoBehaviorGraphRandomNodeInput );

/*static*/ 
void CUndoBehaviorGraphRandomNodeInput::CreateAddingStep( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CBehaviorGraphRandomNode* block )
{
	CUndoBehaviorGraphRandomNodeInput* step = new CUndoBehaviorGraphRandomNodeInput( undoManager, graphEditor, block, true );
	step->PushStep();
}

/*static*/ 
void CUndoBehaviorGraphRandomNodeInput::CreateRemovingStep( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CBehaviorGraphRandomNode* block )
{
	CUndoBehaviorGraphRandomNodeInput* step = new CUndoBehaviorGraphRandomNodeInput( undoManager, graphEditor, block, false );
	step->PushStep();
}

CUndoBehaviorGraphRandomNodeInput::CUndoBehaviorGraphRandomNodeInput( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CBehaviorGraphRandomNode* block, Bool adding )
	: CUndoGraphSocketSnaphot( undoManager, graphEditor, block )
	, m_adding( adding )
{
	m_weights   = block->m_weights;
	m_cooldowns = block->m_cooldowns;
}

void CUndoBehaviorGraphRandomNodeInput::DoStep( Bool adding )
{
	CBehaviorGraphRandomNode* block = SafeCast< CBehaviorGraphRandomNode >( GetBlock() );
	Swap( m_weights,   block->m_weights );
	Swap( m_cooldowns, block->m_cooldowns );
	GraphStructureModified();
}

/*virtual*/ 
void CUndoBehaviorGraphRandomNodeInput::DoUndo()
{
	CUndoGraphSocketSnaphot::DoUndo();
	DoStep( !m_adding );
}

/*virtual*/ 
void CUndoBehaviorGraphRandomNodeInput::DoRedo()
{
	CUndoGraphSocketSnaphot::DoRedo();
	DoStep( m_adding );
}

/*virtual*/ 
String CUndoBehaviorGraphRandomNodeInput::GetName()
{
	return m_adding ? TXT("adding input") : TXT("removing input");
}

// -------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoBehaviorGraphSwitchNodeInput );

/*static*/ 
void CUndoBehaviorGraphSwitchNodeInput::CreateAddingStep( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CBehaviorGraphAnimationSwitchNode* block )
{
	CUndoBehaviorGraphSwitchNodeInput* step = new CUndoBehaviorGraphSwitchNodeInput( undoManager, graphEditor, block, true );
	step->PushStep();
}

/*static*/ 
void CUndoBehaviorGraphSwitchNodeInput::CreateRemovingStep( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CBehaviorGraphAnimationSwitchNode* block )
{
	CUndoBehaviorGraphSwitchNodeInput* step = new CUndoBehaviorGraphSwitchNodeInput( undoManager, graphEditor, block, false );
	step->PushStep();
}

CUndoBehaviorGraphSwitchNodeInput::CUndoBehaviorGraphSwitchNodeInput( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CBehaviorGraphAnimationSwitchNode* block, Bool adding )
	: CUndoGraphSocketSnaphot( undoManager, graphEditor, block )
	, m_adding( adding )
{
}

void CUndoBehaviorGraphSwitchNodeInput::DoStep( Bool adding )
{
	SafeCast< CBehaviorGraphAnimationSwitchNode >( GetBlock() )->m_inputNum += adding ? 1 : -1;
}

/*virtual*/ 
void CUndoBehaviorGraphSwitchNodeInput::DoUndo()
{
	DoStep( !m_adding );
	CUndoGraphSocketSnaphot::DoUndo(); // calls GraphStructureModified
}

/*virtual*/ 
void CUndoBehaviorGraphSwitchNodeInput::DoRedo()
{
	DoStep( m_adding );
	CUndoGraphSocketSnaphot::DoRedo(); // calls GraphStructureModified
}

/*virtual*/ 
String CUndoBehaviorGraphSwitchNodeInput::GetName()
{
	return m_adding ? TXT("adding input") : TXT("removing input");
}

//---------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoBehaviorGraphVariableExistance );

/*static*/ 
void CUndoBehaviorGraphVariableExistance::CreateAddingStep( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CEdBehaviorVariableEditor* editor, const CName& name, Type type )
{
	CUndoBehaviorGraphVariableExistance* step = new CUndoBehaviorGraphVariableExistance( undoManager, graphEditor, editor, name, type, true );
	step->PushStep();
}

/*static*/ 
void CUndoBehaviorGraphVariableExistance::CreateRemovingStep( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CEdBehaviorVariableEditor* editor, const CName& name, Type type )
{
	CUndoBehaviorGraphVariableExistance* step = new CUndoBehaviorGraphVariableExistance( undoManager, graphEditor, editor, name, type, false );
	step->PushStep();
}

CUndoBehaviorGraphVariableExistance::CUndoBehaviorGraphVariableExistance( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CEdBehaviorVariableEditor* editor, const CName& name, Type type, Bool adding )
	: CUndoGraphStep( undoManager, graphEditor )
	, m_editor( editor )
	, m_type( type )
	, m_name( name )
	, m_adding( adding )
{
}

void CUndoBehaviorGraphVariableExistance::DoStep( Bool adding )
{
	CBehaviorGraph* graph = m_editor->GetBehaviorGraph();

	switch ( m_type )
	{
	case SCALAR:
		if ( adding )
		{	
			graph->GetVariables().AddVariable( m_name );
		}
		else
		{
			graph->GetVariables().RemoveVariable( m_name );
		}
		m_editor->RefreshVariablesList();
		break;

	case VECTOR:
		if ( adding )
		{		
			graph->GetVectorVariables().AddVariable( m_name );
		}
		else
		{
			graph->GetVectorVariables().RemoveVariable( m_name );
		}
		m_editor->RefreshVectorVariablesList();
		break;

	case EVENT:
		if ( adding )
		{
			graph->GetEvents().AddEvent( CName( m_name ) );
		}
		else
		{
			graph->GetEvents().RemoveEvent( CName( m_name ) );
		}
		m_editor->RefreshEventsList();
		break;

	case INTERNALSCALAR:
		if ( adding )
		{	
			graph->GetInternalVariables().AddVariable( m_name );
		}
		else
		{
			graph->GetInternalVariables().RemoveVariable( m_name );
		}
		m_editor->RefreshInternalVariablesList();
		break;

	case INTERNALVECTOR:
		if ( adding )
		{		
			graph->GetInternalVectorVariables().AddVariable( m_name );
		}
		else
		{
			graph->GetInternalVectorVariables().RemoveVariable( m_name );
		}
		m_editor->RefreshInternalVectorVariablesList();
		break;
	}

	GraphStructureModified();
}

/*virtual*/ 
void CUndoBehaviorGraphVariableExistance::DoUndo()
{
	DoStep( !m_adding );
}

/*virtual*/ 
void CUndoBehaviorGraphVariableExistance::DoRedo()
{
	DoStep( m_adding );
}

/*virtual*/ 
String CUndoBehaviorGraphVariableExistance::GetName()
{
	String name = m_adding ? TXT("adding ") : TXT("removing ");

	switch ( m_type )
	{
	case SCALAR:
		name += TXT("variable");
		break;

	case VECTOR:
		name += TXT("vector variable");
		break;

	case EVENT:
		name += TXT("event");
		break;
	}

	return name;
}

// --------------------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoBehaviourGraphVariableChange );

CUndoBehaviourGraphVariableChange::CUndoBehaviourGraphVariableChange( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CEdBehaviorVariableEditor* editor, const CName& name, const Vector& value, Type type )
	: CUndoGraphStep( undoManager, graphEditor )
	, m_editor( editor )
	, m_prevVal( value )
	, m_name( name )
	, m_type( type )
{
}

/*static*/ 
CUndoBehaviourGraphVariableChange* CUndoBehaviourGraphVariableChange::PrepareStep( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CEdBehaviorVariableEditor* editor, const CName& name, const Vector& value, Type type )
{
	CUndoBehaviourGraphVariableChange* step = undoManager.SafeGetStepToAdd< CUndoBehaviourGraphVariableChange >();

	if ( !step )
	{
		step = new CUndoBehaviourGraphVariableChange( undoManager, graphEditor, editor, name, value, type );
		undoManager.SetStepToAdd( step );
	}

	step->m_newVal = value;
	return step;
}

/*static*/ 
void CUndoBehaviourGraphVariableChange::PrepareScalarStep( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CEdBehaviorVariableEditor* editor, const CName& name, Float value )
{
	PrepareStep( undoManager, graphEditor, editor, name, Vector( value, 0, 0 ), SCALAR );
}

/*static*/ 
void CUndoBehaviourGraphVariableChange::PrepareVectorStep( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CEdBehaviorVariableEditor* editor, const CName& name, const Vector& value )
{
	PrepareStep( undoManager, graphEditor, editor, name, value, VECTOR );
}

/*static*/ 
void CUndoBehaviourGraphVariableChange::PrepareInternalScalarStep( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CEdBehaviorVariableEditor* editor, const CName& name, Float value )
{
	PrepareStep( undoManager, graphEditor, editor, name, Vector( value, 0, 0 ), INTERNALSCALAR );
}

/*static*/ 
void CUndoBehaviourGraphVariableChange::PrepareInternalVectorStep( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CEdBehaviorVariableEditor* editor, const CName& name, const Vector& value )
{
	PrepareStep( undoManager, graphEditor, editor, name, value, INTERNALVECTOR );
}

/*static*/ 
void CUndoBehaviourGraphVariableChange::FinalizeStep( CEdUndoManager& undoManager )
{
	if ( CUndoBehaviourGraphVariableChange* step = undoManager.SafeGetStepToAdd< CUndoBehaviourGraphVariableChange >() )
	{
		step->PushStep();
	}
}

void CUndoBehaviourGraphVariableChange::DoStep()
{
	switch ( m_type )
	{
	case SCALAR:
		m_editor->SetVariable( m_name, m_prevVal.X );
		m_editor->m_variableProperties->Get().RefreshValues();
		m_editor->RefreshControlList();
		m_editor->UpdateValueSlider();
		break;

	case INTERNALSCALAR:
		m_editor->SetInternalVariable( m_name, m_prevVal.X );
		m_editor->m_variableProperties->Get().RefreshValues();
		m_editor->RefreshControlList();
		m_editor->UpdateInternalValueSlider();
		break;

	case VECTOR:
		m_editor->SetVariable( m_name, m_prevVal );
		m_editor->m_variableProperties->Get().RefreshValues();
		m_editor->RefreshControlList();
		m_editor->ShowVectorInPreview();
		m_editor->UpdateVectorValueSlider();
		break;

	case INTERNALVECTOR:
		m_editor->SetInternalVariable( m_name, m_prevVal );
		m_editor->m_variableProperties->Get().RefreshValues();
		m_editor->RefreshControlList();
		m_editor->ShowVectorInPreview();
		m_editor->UpdateInternalVectorValueSlider();
		break;
	}

	Swap( m_prevVal, m_newVal );
	// DO NOT call GraphStructureModified here - it resets the variables
}

/*virtual*/ 
void CUndoBehaviourGraphVariableChange::DoUndo()
{ 
	DoStep();
}

/*virtual*/ 
void CUndoBehaviourGraphVariableChange::DoRedo()
{
	DoStep();
}

/*virtual*/ 
String CUndoBehaviourGraphVariableChange::GetName()
{
	return TXT("variable change");
}
