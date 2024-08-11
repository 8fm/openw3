
#include "build.h"
#include "undoJobTreeEditor.h"

#include "jobTreeEditor.h"

IMPLEMENT_ENGINE_CLASS( CUndoJobEditorAnimationActivated )

CUndoJobEditorAnimationActivated::CUndoJobEditorAnimationActivated( CEdUndoManager& undoManager, CEdJobTreeEditor* editor, CJobActionBase* action )
	: IUndoStep( undoManager )
	, m_editor( editor )
	, m_action( action )
	, m_name( action->GetAnimName() )
{ 
}

/*static*/ 
void CUndoJobEditorAnimationActivated::CreateStep( CEdUndoManager& undoManager, CEdJobTreeEditor* editor, CJobActionBase* action )
{
	CUndoJobEditorAnimationActivated* step = new CUndoJobEditorAnimationActivated( undoManager, editor, action );
	step->PushStep();
}

void CUndoJobEditorAnimationActivated::DoStep()
{
	CName nameToSet = m_name;
	m_name = m_action.Get()->GetAnimName();
	m_editor->ActivateAnimation( m_action.Get(), nameToSet );
}

/*virtual*/ 
void CUndoJobEditorAnimationActivated::DoUndo()
{
	DoStep();
}

/*virtual*/ 
void CUndoJobEditorAnimationActivated::DoRedo()
{
	DoStep();
}

/*virtual*/ 
String CUndoJobEditorAnimationActivated::GetName()
{
	return TXT("animation changed");
}

//-------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoJobEditorNodeExistance )

CUndoJobEditorNodeExistance::CUndoJobEditorNodeExistance( CEdUndoManager& undoManager, CEdJobTreeEditor* editor, CJobTreeNode* parent, CJobTreeNode* node, Bool creating )
	: IUndoStep( undoManager )
	, m_editor( editor )
	, m_parent( parent )
	, m_node( node )
	, m_creating( creating )
{
	m_index = static_cast< Uint32 >( m_parent.Get()->m_childNodes.GetIndex( m_node ) );
}

/*static*/ 
void CUndoJobEditorNodeExistance::CreateCreationStep( CEdUndoManager& undoManager, CEdJobTreeEditor* editor, CJobTreeNode* parent, CJobTreeNode* node )
{
	CUndoJobEditorNodeExistance* step = new CUndoJobEditorNodeExistance( undoManager, editor, parent, node, true );
	step->PushStep();
}

/*static*/ 
void CUndoJobEditorNodeExistance::CreateDeletionStep( CEdUndoManager& undoManager, CEdJobTreeEditor* editor, CJobTreeNode* parent, CJobTreeNode* node )
{
	CUndoJobEditorNodeExistance* step = new CUndoJobEditorNodeExistance( undoManager, editor, parent, node, false );
	step->m_node->SetParent( step );
	step->PushStep();
}

void CUndoJobEditorNodeExistance::DoStep( Bool creating )
{
	if ( creating )
	{
		m_node->SetParent( m_parent.Get() );
		m_parent.Get()->m_childNodes.Insert( m_index, m_node );
	}
	else
	{
		m_node->SetParent( this );
		m_parent.Get()->m_childNodes.Remove( m_node );
	}

	m_editor->UpdateTreeFragment( m_editor->FindTreeItemForObject( m_parent.Get() ) );
}

/*virtual*/ 
void CUndoJobEditorNodeExistance::DoUndo()
{
	DoStep( !m_creating );
}

/*virtual*/ 
void CUndoJobEditorNodeExistance::DoRedo()
{
	DoStep( m_creating );
}

/*virtual*/ 
String CUndoJobEditorNodeExistance::GetName()
{
	return m_creating ? TXT("creating node") : TXT("deleting node");
}

// --------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoJobActionExistance )

CUndoJobActionExistance::CUndoJobActionExistance( CEdUndoManager& undoManager, CEdJobTreeEditor* editor, CJobTreeNode* node, Type type, Bool creating )
	: IUndoStep( undoManager )
	, m_editor( editor )
	, m_node( node )
	, m_type( type )
	, m_creating( creating )
{
}

/*static*/ 
void CUndoJobActionExistance::CreateCreationStep( CEdUndoManager& undoManager, CEdJobTreeEditor* editor, CJobTreeNode* node, Type type )
{
	CUndoJobActionExistance* step = new CUndoJobActionExistance( undoManager, editor, node, type, true );
	step->PushStep();
}

/*static*/ 
void CUndoJobActionExistance::CreateDeletionStep( CEdUndoManager& undoManager, CEdJobTreeEditor* editor, CJobTreeNode* node, Type type )
{
	CUndoJobActionExistance* step = new CUndoJobActionExistance( undoManager, editor, node, type, false );
	step->StoreAnim();
	step->PushStep();
}

void CUndoJobActionExistance::StoreAnim()
{
	ASSERT ( m_action == NULL );

	switch ( m_type )
	{
	case Enter:
		m_action = m_node.Get()->m_onEnterAction;
		break;
	case Leave:
		m_action = m_node.Get()->m_onLeaveAction;
		break;
	case FastLeave:
		m_action = m_node.Get()->m_onFastLeaveAction;
		break;
	}

	m_action->SetParent( this );

	ASSERT ( m_action != NULL );
}

void CUndoJobActionExistance::RestoreAnim()
{
	ASSERT ( m_action != NULL );

	switch ( m_type )
	{
	case Enter:
		m_node.Get()->m_onEnterAction = static_cast< CJobAction* >( m_action );
		break;
	case Leave:
		m_node.Get()->m_onLeaveAction = static_cast< CJobAction* >( m_action );
		break;
	case FastLeave:
		m_node.Get()->m_onFastLeaveAction = static_cast< CJobForceOutAction* >( m_action );
		break;
	}

	m_action->SetParent( m_node.Get() );
	m_action = NULL;
}

void CUndoJobActionExistance::DoStep( Bool creating )
{
	if ( creating )
	{
		RestoreAnim();
	}
	else
	{
		StoreAnim();

		switch ( m_type )
		{
		case Enter:
			m_node.Get()->m_onEnterAction = NULL;
			break;
		case Leave:
			m_node.Get()->m_onLeaveAction = NULL;
			break;
		case FastLeave:
			m_node.Get()->m_onFastLeaveAction = NULL;
			break;
		}
	}

	m_editor->UpdateTreeFragment( m_editor->FindTreeItemForObject( m_node.Get() ) );
}

/*virtual*/ 
void CUndoJobActionExistance::DoUndo()
{
	DoStep( !m_creating );
}

/*virtual*/ 
void CUndoJobActionExistance::DoRedo()
{
	DoStep( m_creating );
}

/*virtual*/
String CUndoJobActionExistance::GetName()
{
	return m_creating ? TXT("creating action") : TXT("deleting action");
}

// --------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoJobEditorNodeMove )

CUndoJobEditorNodeMove::CUndoJobEditorNodeMove( CEdUndoManager& undoManager, CEdJobTreeEditor* editor, CJobTreeNode* parent, CJobTreeNode* node, Bool up )
	: IUndoStep( undoManager )
	, m_editor( editor )
	, m_parent( parent )
	, m_node( node )
	, m_up( up )
{
}

/*static*/ 
void CUndoJobEditorNodeMove::CreateStep( CEdUndoManager& undoManager, CEdJobTreeEditor* editor, CJobTreeNode* parent, CJobTreeNode* node, Bool up )
{
	CUndoJobEditorNodeMove* step = new CUndoJobEditorNodeMove( undoManager, editor, parent, node, up );
	step->PushStep();
}

void CUndoJobEditorNodeMove::DoStep( Bool up )
{
	TDynArray< CJobTreeNode* >& children = m_parent.Get()->m_childNodes;

	Uint32 nodeIdx = children.GetIndex( m_node.Get() );

	if ( up )
	{
		ASSERT( nodeIdx >= 1 );
		Swap( children[nodeIdx-1], children[nodeIdx] );
	}
	else
	{
		ASSERT( nodeIdx < children.Size()-1 );
		Swap( children[nodeIdx], children[nodeIdx+1] );
	}

	m_editor->StoreSelection();
	m_editor->m_treeView->UnselectAll();
	m_editor->FillTreeContents();
	m_editor->RestoreSelection();
}

/*virtual*/ 
void CUndoJobEditorNodeMove::DoUndo()
{
	DoStep( !m_up );
}

/*virtual*/ 
void CUndoJobEditorNodeMove::DoRedo()
{
	DoStep( m_up );
}

/*virtual*/ 
String CUndoJobEditorNodeMove::GetName()
{
	return TXT("moving node");
}
