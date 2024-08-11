/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeEditor.h"
#include "behTreeGraphEditor.h"
#include "undoBehTreeEditor.h"
#include "../../common/game/behTree.h"
#include "../../common/game/behTreeNode.h"
#include "../../common/game/behTreeScriptedNode.h"
#include "../../common/game/behTreeDecorator.h"
#include "../../common/game/behTreeNodeTemplate.h"
#include "../../common/game/behTreeMachine.h"
#include "itemSelectionDialogs/behaviorTreeBlockTypeSelector.h"
#include "itemSelectionDialogs/behaviorTreeScriptTaskSelector.h"
#include "popupNotification.h"

enum
{
	ID_BLOCK_SELECTOR_DIALOG = wxID_HIGHEST,
	ID_SET_TASK,
	ID_COPY_NODE,
	ID_CUT_NODE,
	ID_DECORATE,
	ID_PASTE_DECORATOR,
	ID_COPY_BRANCH,
	ID_CUT_BRANCH,
	ID_PASTE_BRANCH,
	ID_APPLY_DEFAULT_LAYOUT,
	ID_BREAKPOINT,
	ID_CONTINUE,
	ID_READ_ONLY
};

//#define BEHTREE_EDITOR_LOCK_INGAME	

BEGIN_EVENT_TABLE( CEdBehTreeGraphEditor, CEdTreeEditor )
	EVT_KEY_DOWN( CEdBehTreeGraphEditor::OnKeyDown )
END_EVENT_TABLE()

IBehTreeNodeDefinition*	CEdBehTreeGraphEditor::s_copiedBranch = NULL;
Int32				CEdBehTreeGraphEditor::s_numEditors = 0;

const wxColour	CEdBehTreeGraphEditor::NODE_BREAKPOINT_BG_COLOR = wxColour( 255, 0, 0 );


CEdBehTreeGraphEditor::CEdBehTreeGraphEditor( wxWindow * parent, IHook * hook )
	: CEdTreeEditor( parent, hook )
	, m_editedItem( NULL )
{
	s_numEditors++;

	// Collect allowed component classes
	class CNamer : public CClassHierarchyMapper::CClassNaming
	{
		void GetClassName( CClass* classId, String& outName ) const override
		{
			CName className = CBehNodesManager::GetInstance().GetClassName( classId );
			if ( !className.Empty() )
			{
				outName = className.AsString();
			}
			else
			{
				CClassHierarchyMapper::CClassNaming::GetClassName( classId, outName );
			}
		}
	} cNamer;

	CClassHierarchyMapper::MapHierarchy( ClassID< IBehTreeNodeDefinition >(), m_blockClasses, cNamer );

	SEvents::GetInstance().RegisterListener( CNAME( EditorPostUndoStep ), this );
	SEvents::GetInstance().RegisterListener( CNAME( GameStarted ), this );
	SEvents::GetInstance().RegisterListener( CNAME( GameEnded ), this );
}

CEdBehTreeGraphEditor::~CEdBehTreeGraphEditor()
{
	SEvents::GetInstance().UnregisterListener( this );

	s_numEditors--;

	if( s_numEditors <= 0 )
	{
		if( s_copiedBranch )
		{
			s_copiedBranch->RemoveFromRootSet();
			s_copiedBranch->Discard();
			s_copiedBranch = NULL;
		}
	}
}

void CEdBehTreeGraphEditor::SetTree( IBehTreeEditedItem* editedItem )
{
	if ( m_editedItem == editedItem )
		return;

	m_editedItem = editedItem;

	// Cancel selection
	DeselectAllObjects( false );
	SetActiveItem( NULL );

	// Correct positions
	IBehTreeNodeDefinition* rootNode = m_editedItem ? m_editedItem->GetRootNode() : NULL;
	if( rootNode )
	{
		rootNode->CorrectChildrenPositions();
	}

	// Update layout
	ForceLayoutUpdate();

	// Scale
	ZoomExtents();
}
void CEdBehTreeGraphEditor::PreSave()
{
	IBehTreeNodeDefinition* rootNode = m_editedItem ? m_editedItem->GetRootNode() : NULL;
	if ( !rootNode )
	{
		return;
	}

	TDynArray< IBehTreeNodeDefinition* > nodes;
	rootNode->CollectNodes( nodes );
	for ( IBehTreeNodeDefinition* node : nodes)	
	{
		IBehTreeTaskDefinition* task = node->GetTask();
		if ( task )
		{
			task->Refactor();
		}
	}
}

void CEdBehTreeGraphEditor::PostSave()
{
}

void CEdBehTreeGraphEditor::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( EditorPostUndoStep ) )
	{
		if ( GetEventData< CEdUndoManager* >( data ) == GetUndoManager() )
		{
			// possibly tree structure has been changed
			IBehTreeNodeDefinition* rootNode = m_editedItem->GetRootNode();
			if( rootNode )
			{
				rootNode->CorrectChildrenOrder();
			}
			// do all repainting and stuffz
			TreeStructureModified();
			ForceLayoutUpdate();
			Repaint();
		}
	}
	else if ( name == CNAME( GameStarted ) || name == CNAME( GameEnded ) )
	{
		ForceLayoutUpdate();
		Repaint();
	}
}

String CEdBehTreeGraphEditor::GetBlockName( IScriptable & block ) const
{
	IBehTreeNodeDefinition* node = SafeCast< IBehTreeNodeDefinition >( &block );
	return node->GetNodeCaption();
}

String CEdBehTreeGraphEditor::GetBlockComment( IScriptable & block ) const
{
	IBehTreeNodeDefinition* node = SafeCast< IBehTreeNodeDefinition >( &block );
	return node->GetComment();
}

Bool CEdBehTreeGraphEditor::CanHaveChildren( IScriptable & block ) const
{
	IBehTreeNodeDefinition* node = Cast< IBehTreeNodeDefinition >( &block );
	return !node->IsTerminal();
}

Bool CEdBehTreeGraphEditor::IsLocked( IScriptable & block ) const
{
#ifdef BEHTREE_EDITOR_LOCK_INGAME
	if ( GGame->IsActive() )
	{
		return true;
	}
#endif
	if( !m_editedItem->CanModify() )
	{
		return true;
	}
	return false;
}

void CEdBehTreeGraphEditor::PaintCanvas( Int32 width, Int32 height )
{
	CEdTreeEditor::PaintCanvas( width, height );
	if( m_editor->IsInDebugMode() )
	{
		CObject * machineParent = m_editor->GetMachine()->GetParent();
		if( machineParent && machineParent->IsA< CEntity >() )
		{
			Float scale = GetScale();
			wxPoint offset = GetOffset();

			SetScale( 1.0f, false );
			SetOffset( wxPoint( 0, 0 ), false );
			
			const String& name = SafeCast< CEntity >( machineParent )->GetName();
			const String* state;
			static const String stateStopped = TXT("(machine stopped)");
			static const String stateRunning = TXT("(machine running)"); 
			if( m_editor->GetMachine()->IsStopped() )
			{
				state = &stateStopped;
			}
			else
			{
				state = &stateRunning;
			}

			DrawText( wxPoint( 10, 10 ), GetGdiBoldFont(), String::Printf( TXT("DEBUGGING: %s %s"), name.AsChar(), (*state).AsChar() ).AsChar(), *wxRED );
			
			SetScale( scale, false );
			SetOffset( offset, false );
		}
	}
}

void CEdBehTreeGraphEditor::ForceLayoutUpdate()
{
	ClearLayout();

	if ( !m_editedItem )
	{
		return;
		}

	FillNodeLayout( m_editedItem->GetRootNode(), NULL );
}

void CEdBehTreeGraphEditor::FillNodeLayout( IBehTreeNodeDefinition * node, LayoutInfo * parentLayout )
{
	if ( ! node )
		return;

	LayoutInfo * layout = UpdateBlockLayout( node, parentLayout );

	switch ( node->GetEditorNodeType() )
	{
	default:
	case IBehTreeNodeDefinition::NODETYPE_DEFAULT:
		layout->m_bgColor = wxColour(0x57,0xb4,0xd2);
		break;
	case IBehTreeNodeDefinition::NODETYPE_DECORATOR:
		layout->m_bgColor = wxColour(0xff,0xff,0x00);
		break;
	case IBehTreeNodeDefinition::NODETYPE_COMPOSITE:
		layout->m_bgColor = wxColour(0x00,0xff,0x00);
		break;
	case IBehTreeNodeDefinition::NODETYPE_SCRIPTED:
		layout->m_bgColor = wxColour(0x00,0x00,0xff);
		break;
	case IBehTreeNodeDefinition::NODETYPE_METANODE:
		layout->m_bgColor = wxColour(0xff,0x00,0x00);
		break;
	case IBehTreeNodeDefinition::NODETYPE_UNIQUE:
		layout->m_bgColor = wxColour(0xe2,0x47,0xd2);
		break;
	case IBehTreeNodeDefinition::NODETYPE_COMMENT:
		layout->m_bgColor = wxColour(0xd0,0xd0,0xd0);
		break;
	}

#ifdef EDITOR_AI_DEBUG
	if( m_editor->IsInDebugMode() )
	{
		CBehTreeMachine* machine = m_editor->GetMachine();
		layout->m_bgColor = CEdTreeEditor::LayoutInfo::INVALID_BG_COLOR;

		CBehTreeMachine::EBreakpointStatus status;
		if( machine->IsBreakpointSet( node, status ) && ( status == CBehTreeMachine::BS_Breaked ) )
		{
			layout->m_bgColor = NODE_BREAKPOINT_BG_COLOR;
		}
	}
#endif	//EDITOR_AI_DEBUG

	if( !node->IsTerminal() )
	{
		for ( Int32 i = 0, n = node->GetNumChildren(); i < n; ++i )
		{
			FillNodeLayout( const_cast< IBehTreeNodeDefinition* >( node->GetChild( i ) ), layout );
		}
	}
}

Bool CEdBehTreeGraphEditor::LoadBlockPosition( IScriptable * block, wxPoint & pos )
{
	if ( IBehTreeNodeDefinition * node = Cast< IBehTreeNodeDefinition >( block ) )
	{
		pos.x = node->GetGraphPosX();
		pos.y = node->GetGraphPosY();
		return true;
	}

	return false;
}

Bool CEdBehTreeGraphEditor::SaveBlockPosition( IScriptable * block, const wxPoint & pos )
{
	if ( IBehTreeNodeDefinition * node = Cast< IBehTreeNodeDefinition >( block ) )
	{
		node->SetGraphPosition( pos.x, pos.y );
		return true;
	}

	return false;
}

void CEdBehTreeGraphEditor::DrawBlockLayout( LayoutInfo & layout )
{
	CEdTreeEditor::DrawBlockLayout( layout );

	IBehTreeNodeDefinition * node = Cast< IBehTreeNodeDefinition >( layout.m_owner );
	if ( node )
	{					
#ifdef EDITOR_AI_DEBUG
		if( m_editor->IsInDebugMode() )
		{
			CBehTreeMachine* machine = m_editor->GetMachine();
			ASSERT( machine );
			CBehTreeMachine::EBreakpointStatus status;
			if( machine->IsBreakpointSet( node, status ) )
			{
				DrawRoundedRect( layout.m_windowPos.x, layout.m_windowPos.y, layout.m_windowSize.x, layout.m_windowSize.y, wxColour( 255, 0, 255 ), 8, 3 );
			}
		}
#endif //EDITOR_AI_DEBUG
	}
}

//! Called when move ended
void CEdBehTreeGraphEditor::OnMoveEnded()
{
#ifdef BEHTREE_EDITOR_LOCK_INGAME
	if( GGame->IsActive() )
		return;
#endif

	if( !m_editedItem->CanModify() )
	{
		return;
	}

	if( GetSelectedObjects().Size() > 0 )
	{
		IBehTreeNodeDefinition* rootNode = m_editedItem->GetRootNode();
		if( rootNode )
		{
			if( rootNode->CorrectChildrenOrder() )
			{
				TreeStructureModified();
			}

			// Update layout
			ForceLayoutUpdate();

			Repaint();
		}
	}
}

void CEdBehTreeGraphEditor::DeleteSelectedObjects( bool askUser /* = true */ )
{
#ifdef BEHTREE_EDITOR_LOCK_INGAME
	if( GGame->IsActive() )
		return;
#endif

	// Ask the question
	if ( askUser && GetSelectedObjects().Size() )
	{
		if ( !YesNo( TXT("Sure to delete selection ?") ) )
		{
			return;
		}
	}

	if ( ! m_editedItem->CanModify() )
	{
		ShowMustCheckoutInfo();
		return;
	}
	
	CleanUpSelection( true, true );

	IBehTreeNodeDefinition* rootNode = m_editedItem->GetRootNode();
	for ( Uint32 i = 0; i < GetSelectedObjects().Size(); ++i )
	{
		IBehTreeNodeDefinition* node = Cast< IBehTreeNodeDefinition >( GetSelectedObjects()[i] );
		if ( ! node )
			continue;

		if ( node == rootNode )
		{
			m_editedItem->SetRootNode( NULL );
		}
		else
		{
			CObject* parent = node->GetParent();
			IBehTreeNodeDefinition * parentNode = Cast< IBehTreeNodeDefinition >( parent );
			ASSERT( parentNode );
			if ( parentNode )
			{
				CUndoBehTreeBlockExistance::PrepareDeletionStep( *GetUndoManager(), node );
				parentNode->RemoveChild( node );
			}
		}
	}

	CUndoBehTreeBlockExistance::FinalizeStep( *GetUndoManager() );

	// Mark modified
	TreeStructureModified();

	// selection is actually reset after destroying the component, which prevents deleted objects from being included in the properties pages
	DeselectAllObjects();

	ForceLayoutUpdate();
	// Redraw
	Repaint();
}

void CEdBehTreeGraphEditor::FillTaskClasses()
{
	static const Int32 PREXIES_COUNT = 4;
	static const Int32 SUFIXES_COUNT = 2;
	static const String PREFIXES[ PREXIES_COUNT ] =
	{
		TXT("CBehTree"),
		TXT("IBehTree"),
		TXT("CBT"),
		TXT("BT")
	};
	static const String SUFIXES[ SUFIXES_COUNT ] =
	{
		TXT("Def"),
		TXT("Definition")
	};
	struct CTaskNames : public CClassHierarchyMapper::CClassNaming
	{
		void GetClassName( CClass* classId, String& outName ) const override
		{
			IBehTreeTaskDefinition::GetTaskNameStatic( classId, outName );
		}
	};
	m_taskClasses.Clear();
	CClassHierarchyMapper::MapHierarchy( ClassID< IBehTreeTaskDefinition >(), m_taskClasses, CTaskNames() );
}


void CEdBehTreeGraphEditor::OnOpenContextMenu()
{
	wxMenu menu;

	m_contextMenuItem = GetActiveItem() ? SafeCast< IBehTreeNodeDefinition >( GetActiveItem()->m_owner ) : NULL;

	if( m_editor->IsInDebugMode() )
	{
		if( m_contextMenuItem )
		{
			menu.Append( ID_BREAKPOINT, TXT( "Toggle breakpoint" ) );
			menu.Connect( ID_BREAKPOINT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehTreeGraphEditor::OnBreakpointToggle ), NULL, this );

			menu.Append( ID_CONTINUE, TXT( "Continue" ) );
			menu.Connect( ID_CONTINUE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehTreeGraphEditor::OnBreakpointContinue ), NULL, this );
		}
	}
	else
	{
		Bool gameIsActive =
#ifdef BEHTREE_EDITOR_LOCK_INGAME
			GGame->IsActive();
#else
			false;
#endif
		Bool canModify = m_editedItem->CanModify();
		Bool isEditable = !gameIsActive && canModify;
		Bool canHaveChildren = m_editedItem->GetRootNode() == NULL || ( m_contextMenuItem && m_contextMenuItem->CanAddChild() );

		if ( canHaveChildren && isEditable )
		{
			menu.Append( ID_BLOCK_SELECTOR_DIALOG, TXT( "Create block" ) );
			menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdBehTreeGraphEditor::OnSpawnBlock, this, ID_BLOCK_SELECTOR_DIALOG );
		}

		if( m_contextMenuItem && isEditable )
		{
			if ( m_contextMenuItem->IsSupportingTasks() )
			{
				FillTaskClasses();
				menu.Append( ID_SET_TASK, TXT( "Set script task" ) );
				menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdBehTreeGraphEditor::OnSetTask, this, ID_SET_TASK );
			}
			if ( m_contextMenuItem->GetNumChildren() <= 1 )
			{
				menu.Append( ID_CUT_NODE, TXT( "Cut node" ) );
				menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdBehTreeGraphEditor::OnCutNode, this, ID_CUT_NODE );
			}
			if ( s_copiedBranch && s_copiedBranch->GetNumChildren() == 0 && s_copiedBranch->CanAddChild() )
			{
				menu.Append( ID_PASTE_DECORATOR, TXT( "Paste decorator" ) );
				menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdBehTreeGraphEditor::OnPasteDecorator, this, ID_PASTE_DECORATOR );
			}
			menu.Append( ID_DECORATE, TXT("Decorate") );
			menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdBehTreeGraphEditor::OnSetDecorator, this, ID_DECORATE );

		}

		menu.AppendSeparator();

		if( m_contextMenuItem )
		{
			if ( m_contextMenuItem->GetNumChildren() > 0 )
			{
				menu.Append( ID_COPY_NODE, TXT("Copy node") );
				menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdBehTreeGraphEditor::OnCopyNode, this, ID_COPY_NODE );
			}
			menu.Append( ID_COPY_BRANCH, TXT("Copy branch") );
			menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdBehTreeGraphEditor::OnCopyBranch, this, ID_COPY_BRANCH );
			if( isEditable )
			{
				menu.Append( ID_CUT_BRANCH, TXT("Cut branch") );
				menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdBehTreeGraphEditor::OnCutBranch, this, ID_CUT_BRANCH );
			}
			
		}

		menu.AppendSeparator();

		if( s_copiedBranch && canHaveChildren && isEditable )
		{
			menu.Append( ID_PASTE_BRANCH, TXT("Paste branch") );
			menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdBehTreeGraphEditor::OnPasteBranch, this, ID_PASTE_BRANCH );
		}

		menu.AppendSeparator();

		if ( gameIsActive )
		{
			menu.Append( ID_READ_ONLY, TXT("Game is active [READ ONLY]") );
			menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdBehTreeGraphEditor::OnGameActive, this, ID_READ_ONLY );
		}
		else if( !canModify )
		{
			menu.Append( ID_READ_ONLY, TXT("File is locked. Check Out!") );
			menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdBehTreeGraphEditor::OnReadOnly, this, ID_READ_ONLY );
		}
		else if ( !m_contextMenuItem || m_contextMenuItem->GetNumChildren() > 0 )
		{
			menu.Append( ID_APPLY_DEFAULT_LAYOUT, TXT("Auto layout branch") );
			menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdBehTreeGraphEditor::OnApplyDefaultLayout, this, ID_APPLY_DEFAULT_LAYOUT );
		}
		
	}

	// Show menu
	wxMenuUtils::CleanUpSeparators( &menu );
	PopupMenu( &menu );
}

void CEdBehTreeGraphEditor::ShowMustCheckoutInfo()
{
	wxMessageBox( TXT("Cannot modify. Checkout file!"), TXT("Behavior Tree Editor") );	
}

void CEdBehTreeGraphEditor::OnReadOnly( wxEvent& )
{
	if( m_editedItem->CheckOut() )
	{
		ForceLayoutUpdate();
		Repaint();
	}
}

void CEdBehTreeGraphEditor::OnGameActive( wxEvent& )
{
	wxMessageBox( TXT("Cannot modify. Game is currently active. Close it."), TXT("Behavior Tree Editor") );	
}

void CEdBehTreeGraphEditor::OnSetTask( wxCommandEvent& event )
{
	if( !m_contextMenuItem )
	{
		return;
	}

#ifdef BEHTREE_EDITOR_LOCK_INGAME
	if( !GGame->IsActive() )
#endif
	{
		if ( m_editedItem->CanModify() )
		{
			IBehTreeTaskDefinition* task = m_contextMenuItem->GetTask();

			if( !task || YesNo( TXT( "Are you sure you want to change task?" ) ) )
			{
				CEdBehaviorTreeScriptTaskSelectorDialog selector( this, m_taskClasses, task );
				if ( CClass* taskClass = selector.Execute() )
				{
					IBehTreeNodeDefinition* state = m_contextMenuItem;

					THandle< IBehTreeTaskDefinition > task = taskClass->CreateObject< IBehTreeTaskDefinition >();
					task->OnCreatedInEditor();
					if ( state->SetTask( task ) )
					{
						m_editor->OnGraphSelectionChanged();		
						ForceLayoutUpdate();		
						Repaint();
					}
				}
			}
		}
		else
		{
			ShowMustCheckoutInfo();
		}
	}
}

void CEdBehTreeGraphEditor::OnApplyDefaultLayout( wxCommandEvent& event )
{
	IBehTreeNodeDefinition* parentNode = m_contextMenuItem ? m_contextMenuItem : m_editedItem->GetRootNode();

	if ( !parentNode )
	{
		return;
	}

	AutoLayout( parentNode );
}

void CEdBehTreeGraphEditor::OnSetDecorator( wxCommandEvent& event )
{
#ifdef BEHTREE_EDITOR_LOCK_INGAME
	if( GGame->IsActive() )
	{
		return;
	}
#endif
	if( !m_editedItem->CanModify() )
	{
		return;
	}

	CEdBehaviorTreeDecoratorBlockTypeSelectorDialog selector( this, m_blockClasses, IBehTreeNodeDefinition::GetStaticClass() );
	if ( CClass* blockClass = selector.Execute() )
	{
		IBehTreeNodeDefinition * childNode = m_contextMenuItem;
		if( childNode )
		{
			IBehTreeNodeDefinition* parentNode = childNode->GetParentNode();

			LayoutInfo * layout = NULL;
			if ( parentNode )
			{
				layout = GetLayout( parentNode );
				ASSERT( layout );
			}

			IBehTreeNodeDecoratorDefinition * node = ::CreateObject< IBehTreeNodeDecoratorDefinition >( blockClass, parentNode );
			if ( !node )
			{
				return;
			}

			Int32 baseX = childNode->GetGraphPosX();
			Int32 baseY = childNode->GetGraphPosY();
			node->SetGraphPosition( baseX, baseY );
			childNode->OffsetNodesPosition( OFFSET_X_SINGLE, OFFSET_Y_SINGLE );

			if( !parentNode )
			{
				m_editedItem->SetRootNode( node );

				childNode->SetParent( node );
				node->AddChild( childNode );
				CUndoBehTreeBlockExistance::PrepareCreationStepWithOffset( *GetUndoManager(), childNode, OFFSET_X_SINGLE, OFFSET_Y_SINGLE );
			}
			else
			{
				// correct hierarchy
				CUndoBehTreeBlockExistance::PrepareDeletionStep( *GetUndoManager(), childNode );
				parentNode->RemoveChild( childNode );

				parentNode->AddChild( node );
				CUndoBehTreeBlockExistance::PrepareCreationStep( *GetUndoManager(), node );

				childNode->SetParent( node );
				node->AddChild( childNode );
				CUndoBehTreeBlockExistance::PrepareCreationStepWithOffset( *GetUndoManager(), childNode, OFFSET_X_SINGLE, OFFSET_Y_SINGLE );

				// correct parent child indexing based on graph position
				parentNode->CorrectChildrenOrder();
			}

			CUndoBehTreeBlockExistance::FinalizeStep( *GetUndoManager(), TXT("create decorator") );
			UpdateBlockLayout( node, layout );
		}

		ForceLayoutUpdate();

		// Mark modified
		TreeStructureModified();

		Repaint();
	}
}

void CEdBehTreeGraphEditor::OnSpawnBlock( wxCommandEvent& event )
{
#ifdef BEHTREE_EDITOR_LOCK_INGAME
	if( !GGame->IsActive() )
#endif
	{
		if ( m_editedItem->CanModify() )
		{
			CEdBehaviorTreeBlockTypeSelectorDialog selector( this, m_blockClasses );
			if ( CClass* blockClass = selector.Execute() )
			{
				if ( m_editedItem->GetRootNode() == NULL )
				{
					CObject* parentObject = m_editedItem->GetParentObject();
					if ( parentObject )
					{
						IBehTreeNodeDefinition * node = ::CreateObject< IBehTreeNodeDefinition >( blockClass, parentObject );
						m_editedItem->SetRootNode( node );
						UpdateBlockLayout( node, NULL );
						ZoomExtents();

						CUndoBehTreeBlockExistance::PrepareCreationStep( *GetUndoManager(), node );
						CUndoBehTreeBlockExistance::FinalizeStep( *GetUndoManager() );
					}
				}
				else if ( m_contextMenuItem )
				{
					IBehTreeNodeDefinition * parent = m_contextMenuItem;
					if( parent && parent->CanAddChild() )
					{
						LayoutInfo * parentLayout = GetLayout( parent );
						ASSERT( parentLayout );

						IBehTreeNodeDefinition * node = ::CreateObject< IBehTreeNodeDefinition >( blockClass, parent );
						if( parent->GetNumChildren() > 0 )
						{
							const IBehTreeNodeDefinition* lastChild = parent->GetChild( parent->GetNumChildren()-1 );
							Int32 x = lastChild->GetGraphPosX();
							Int32 y = lastChild->GetGraphPosY();
							node->SetGraphPosition( x + OFFSET_X_MANY, y+OFFSET_Y_MANY );
						}
						else
						{
							Int32 x = parent->GetGraphPosX();
							Int32 y = parent->GetGraphPosY();
							node->SetGraphPosition( x + OFFSET_X_SINGLE, y + OFFSET_Y_SINGLE );
						}

						parent->AddChild( node );				
						CUndoBehTreeBlockExistance::PrepareCreationStep( *GetUndoManager(), node );

						UpdateBlockLayout( node, parentLayout );

						CUndoBehTreeBlockExistance::FinalizeStep( *GetUndoManager() );
					}
				}

				// Mark modified
				TreeStructureModified();

				Repaint();
			}
		}
		else
		{
			ShowMustCheckoutInfo();
		}
	}
}

void CEdBehTreeGraphEditor::OnCutNode( wxCommandEvent& event )
{
#ifdef BEHTREE_EDITOR_LOCK_INGAME
	if( GGame->IsActive() )
	{
		return;
	}
#endif

	if ( !m_contextMenuItem )
	{
		return;
	}

	if ( ! m_editedItem->CanModify() )
	{
		ShowMustCheckoutInfo();
		return;
	}

	IBehTreeNodeDefinition* node = m_contextMenuItem;
	if( node && node->GetNumChildren() <= 1 )
	{
		Int32 xDiff = 0;
		Int32 yDiff = 0;
		IBehTreeNodeDefinition* childNode = node->GetNumChildren() == 1 ? node->GetChild( 0 ) : NULL;
		if ( childNode )
		{
			xDiff = node->GetGraphPosX() - childNode->GetGraphPosX();
			yDiff = node->GetGraphPosY() - childNode->GetGraphPosY();

			CUndoBehTreeBlockExistance::PrepareDeletionStep( *GetUndoManager(), childNode );
			node->RemoveChild( childNode );
		}
		IBehTreeNodeDefinition* parent = node->GetParentNode();
		if( parent )
		{
			CUndoBehTreeBlockExistance::PrepareDeletionStep( *GetUndoManager(), node );
			parent->RemoveChild( node );

			if ( childNode )
			{
				childNode->OffsetNodesPosition( xDiff, yDiff );

				childNode->SetParent( parent );
				parent->AddChild( childNode );
				CUndoBehTreeBlockExistance::PrepareCreationStepWithOffset( *GetUndoManager(), childNode, xDiff, yDiff );

				parent->CorrectChildrenOrder();
			}
		}
		else
		{
			m_editedItem->SetRootNode( childNode );
		}

		CUndoBehTreeBlockExistance::FinalizeStep( *GetUndoManager(), TXT("cut node") );

		if( s_copiedBranch )
		{
			s_copiedBranch->RemoveFromRootSet();
			s_copiedBranch->Discard();
		}

		s_copiedBranch = SafeCast< IBehTreeNodeDefinition >( node->Clone( NULL ) );
		s_copiedBranch->AddToRootSet();

		DeselectAllObjects();
		ForceLayoutUpdate();
		Repaint();
	}
}
void CEdBehTreeGraphEditor::CopyNode( IBehTreeNodeDefinition* node )
{
	ASSERT( node );

	if( s_copiedBranch )
	{
		s_copiedBranch->RemoveFromRootSet();
		s_copiedBranch->Discard();
		s_copiedBranch = NULL;
	}

	s_copiedBranch = SafeCast< IBehTreeNodeDefinition >( node->Clone( NULL ) );
	s_copiedBranch->AddToRootSet();
}
void CEdBehTreeGraphEditor::CopyActiveNode()
{
	if ( GetActiveItem() )
	{
		CopyNode( Cast< IBehTreeNodeDefinition >( GetActiveItem()->m_owner ) );
	}
}

void CEdBehTreeGraphEditor::OnCopyNode( wxCommandEvent& event )
{
	if ( !m_contextMenuItem )
	{
		return;
	}

	CopyNode( m_contextMenuItem );

	while ( s_copiedBranch->GetNumChildren() > 0 )
	{
		s_copiedBranch->RemoveChild( s_copiedBranch->GetChild( s_copiedBranch->GetNumChildren()-1 ) );
	}

	String friendlyName = s_copiedBranch->GetFriendlyName();
	SEdPopupNotification::GetInstance().Show( this, TXT("COPY"), friendlyName );

}

void CEdBehTreeGraphEditor::OnCopyBranch( wxEvent& )
{
	if ( !m_contextMenuItem )
	{
		return;
	}

	CopyNode( m_contextMenuItem );

	String friendlyName = s_copiedBranch->GetFriendlyName();
	SEdPopupNotification::GetInstance().Show( this, TXT("COPY"), friendlyName );
}

void CEdBehTreeGraphEditor::OnCutBranch( wxEvent&  )
{
#ifdef BEHTREE_EDITOR_LOCK_INGAME
	if( GGame->IsActive() )
	{
		return;
	}
#endif

	if ( !m_contextMenuItem )
	{
		return;
	}

	if ( ! m_editedItem->CanModify() )
	{
		ShowMustCheckoutInfo();
		return;
	}

	CopyNode( m_contextMenuItem );

	IBehTreeNodeDefinition * node = m_contextMenuItem;	
	ASSERT( node );
	const IBehTreeNodeDefinition* parent = node->GetParentNode();
	if( parent )
	{
		IBehTreeNodeDefinition* nonConstParent = const_cast< IBehTreeNodeDefinition* >( parent );

		CUndoBehTreeBlockExistance::PrepareDeletionStep( *GetUndoManager(), node );
		nonConstParent->RemoveChild( node );
	}
	else
	{
		m_editedItem->SetRootNode( NULL );
	}

	String friendlyName = node->GetFriendlyName();
	CUndoBehTreeBlockExistance::FinalizeStep( *GetUndoManager(), TXT("cut ") + friendlyName );
	SEdPopupNotification::GetInstance().Show( this, TXT("CUT"), friendlyName );

	DeselectAllObjects();
	ForceLayoutUpdate();
	Repaint();
}

void CEdBehTreeGraphEditor::OnPasteDecorator( wxCommandEvent& event )
{
#ifdef BEHTREE_EDITOR_LOCK_INGAME
	if( GGame->IsActive() )
	{
		return;
	}
#endif

	if ( !m_contextMenuItem )
	{
		return;
	}

	if ( ! m_editedItem->CanModify() )
	{
		ShowMustCheckoutInfo();
		return;
	}

	if ( !s_copiedBranch || s_copiedBranch->GetNumChildren() > 0 || !s_copiedBranch->CanAddChild() )
	{
		return;
	};

	// Get info
	IBehTreeNodeDefinition * childNode = m_contextMenuItem;
	if( childNode )
	{
		IBehTreeNodeDefinition* parentNode = childNode->GetParentNode();

		LayoutInfo * layout = NULL;
		if ( parentNode )
		{
			layout = GetLayout( parentNode );
			ASSERT( layout );
		}

		CObject* obj =  s_copiedBranch->Clone( parentNode );
		if ( !obj )
		{
			return;
		}
		IBehTreeNodeDefinition* node = static_cast< IBehTreeNodeDefinition* >( obj );

		Int32 baseX = childNode->GetGraphPosX();
		Int32 baseY = childNode->GetGraphPosY();
		node->SetGraphPosition( baseX, baseY );
		childNode->OffsetNodesPosition( OFFSET_X_SINGLE, OFFSET_Y_SINGLE );

		if( !parentNode )
		{
			m_editedItem->SetRootNode( node );

			childNode->SetParent( node );
			node->AddChild( childNode );
			CUndoBehTreeBlockExistance::PrepareCreationStepWithOffset( *GetUndoManager(), childNode, OFFSET_X_SINGLE, OFFSET_Y_SINGLE );
		}
		else
		{
			// correct hierarchy
			CUndoBehTreeBlockExistance::PrepareDeletionStep( *GetUndoManager(), childNode );
			parentNode->RemoveChild( childNode );

			parentNode->AddChild( node );
			CUndoBehTreeBlockExistance::PrepareCreationStep( *GetUndoManager(), node );

			childNode->SetParent( node );
			node->AddChild( childNode );
			CUndoBehTreeBlockExistance::PrepareCreationStepWithOffset( *GetUndoManager(), childNode, OFFSET_X_SINGLE, OFFSET_Y_SINGLE );

			// correct parent child indexing based on graph position
			parentNode->CorrectChildrenOrder();
		}

		String friendlyName = s_copiedBranch->GetFriendlyName();
		SEdPopupNotification::GetInstance().Show( this, TXT("PASTE DECORATOR"), friendlyName );
		CUndoBehTreeBlockExistance::FinalizeStep( *GetUndoManager(), TXT("paste ") + friendlyName );

		UpdateBlockLayout( node, layout );
	}

	ForceLayoutUpdate();
}

void CEdBehTreeGraphEditor::OnPasteBranch( wxEvent& )
{	
#ifdef BEHTREE_EDITOR_LOCK_INGAME
	if( GGame->IsActive() )
	{
		return;
	}
#endif

	if ( !s_copiedBranch )
	{
		return;
	}

	if ( ! m_editedItem->CanModify() )
	{
		ShowMustCheckoutInfo();
		return;
	}

	IBehTreeNodeDefinition * node = NULL;
	Bool zoom = false;
	
	if( m_editedItem->GetRootNode() == NULL )
	{
		CObject* parentObject = m_editedItem->GetParentObject();
		if ( parentObject )
		{
			node = SafeCast< IBehTreeNodeDefinition >( s_copiedBranch->Clone( parentObject ) );
			m_editedItem->SetRootNode( node );

			Int32 oldX = node->GetGraphPosX();
			Int32 oldY = node->GetGraphPosY();

			// Root in [0,0]
			node->OffsetNodesPosition( -oldX, -oldY );
			zoom = true;
		}
		
	}
	else if ( !m_contextMenuItem )
	{
		return;
	}
	else
	{	
		IBehTreeNodeDefinition * parent = m_contextMenuItem;
		if( parent->CanAddChild() )
		{
			node = SafeCast< IBehTreeNodeDefinition >( s_copiedBranch->Clone( parent ) );
			
			Int32 offsetX, offsetY;
			if( parent->GetNumChildren() > 0 )
			{
				const IBehTreeNodeDefinition* lastChild = parent->GetChild( parent->GetNumChildren()-1 );
				Int32 x = lastChild->GetGraphPosX();
				Int32 y = lastChild->GetGraphPosY();
				Int32 oldX = node->GetGraphPosX();
				Int32 oldY = node->GetGraphPosY();
				offsetX = x + OFFSET_X_MANY - oldX;
				offsetY = y + OFFSET_Y_MANY - oldY;
			}
			else
			{
				Int32 x = parent->GetGraphPosX();
				Int32 y = parent->GetGraphPosY();
				Int32 oldX = node->GetGraphPosX();
				Int32 oldY = node->GetGraphPosY();
				offsetX = x + OFFSET_X_SINGLE - oldX;
				offsetY = y + OFFSET_Y_SINGLE - oldY;
			}

			node->OffsetNodesPosition( offsetX, offsetY );

			parent->AddChild( node );
			CUndoBehTreeBlockExistance::PrepareCreationStepWithOffset( *GetUndoManager(), node, offsetX, offsetY );
		}
	}

	String friendlyName = s_copiedBranch->GetFriendlyName();
	SEdPopupNotification::GetInstance().Show( this, TXT("PASTE "), friendlyName );
	CUndoBehTreeBlockExistance::FinalizeStep( *GetUndoManager(), TXT("paste ") + friendlyName );

	if( node )
	{
		// Select cloned nodes
		TDynArray< IBehTreeNodeDefinition* > nodes;
		node->CollectNodes( nodes );
		
		DeselectAllObjects( false );

		for( Uint32 i=0; i<nodes.Size(); i++ )
		{
			SelectObject( nodes[i], true, false );
		}

		ForceLayoutUpdate();		
		Repaint();

		if( zoom )
		{
			ZoomExtents();
		}
	}

	// Mark modified
	TreeStructureModified();
}

void CEdBehTreeGraphEditor::ResultToColor( const Bool active, wxColour& col )
{
	if( active )
	{
		col = wxColour( 0, 215, 0 );
	}
	else
	{
		col = LayoutInfo::DEFAULT_BG_COLOR;
	}
}

Bool CEdBehTreeGraphEditor::OnNodeReport( const IBehTreeNodeDefinition* node, Bool active )
{
	if ( LayoutInfo * layout = GetLayout( const_cast< IBehTreeNodeDefinition* >( node ) ) )
	{		
		ResultToColor( active, layout->m_bgColor );
		Repaint();
		return true;
	}

	return false;
}

Bool CEdBehTreeGraphEditor::OnNodeResultChanged( const IBehTreeNodeDefinition* node, const Bool active )
{
	// Find or create layout info
	if ( LayoutInfo * layout = GetLayout( const_cast< IBehTreeNodeDefinition* >( node ) ) )
	{		
		ResultToColor( active, layout->m_bgColor );
		Repaint();
		return true;
	}	

	return false;
}

void CEdBehTreeGraphEditor::ForceSetDebuggerColor( const IBehTreeNodeDefinition* node, Uint8 R, Uint8 G, Uint8 B )
{
	// Find or create layout info
	if ( LayoutInfo * layout = GetLayout( const_cast< IBehTreeNodeDefinition* >( node ) ) )
	{
		layout->m_bgColor = wxColour( R, G, B );
		Repaint();
	}	
}

void CEdBehTreeGraphEditor::OnTreeStateChanged()
{
	Repaint();
}

Bool CEdBehTreeGraphEditor::OnBreakpoint( const IBehTreeNodeDefinition* node )
{
	if ( LayoutInfo * layout = GetLayout( const_cast< IBehTreeNodeDefinition* >( node ) ) )
	{
		wxColour& col = layout->m_bgColor;
		col = NODE_BREAKPOINT_BG_COLOR;

		Repaint();

		return true;
	}

	return false;
}

void CEdBehTreeGraphEditor::OnBreakpointToggle( wxCommandEvent& event )
{
	RED_UNUSED( event );
#ifdef EDITOR_AI_DEBUG
	CBehTreeMachine* machine = m_editor->GetMachine();
	if( machine )
	{
		if( m_contextMenuItem )
		{
			CBehTreeMachine::EBreakpointStatus status = CBehTreeMachine::BS_Normal;
			if( machine->IsBreakpointSet( m_contextMenuItem, status ) )
			{
				if( status == CBehTreeMachine::BS_Breaked )
				{
					machine->SkipBreakpoint( m_contextMenuItem );
				}
				machine->ClearBreakpoint( m_contextMenuItem );				
			}
			else
			{
				machine->SetBreakpoint( m_contextMenuItem );
			}
			Repaint();
		}
	}
#endif	//EDITOR_AI_DEBUG
}

void CEdBehTreeGraphEditor::OnBreakpointContinue( wxCommandEvent& event )
{
	RED_UNUSED( event );
#ifdef EDITOR_AI_DEBUG
	CBehTreeMachine* machine = m_editor->GetMachine();
	if( machine )
	{
		if( m_contextMenuItem )
		{
			CBehTreeMachine::EBreakpointStatus status = CBehTreeMachine::BS_Normal;
			if( machine->IsBreakpointSet( m_contextMenuItem, status ) && ( status == CBehTreeMachine::BS_Breaked ) )
			{
				machine->SkipBreakpoint( m_contextMenuItem );
			}
		}
	}
#endif // EDITOR_AI_DEBUG
}

void CEdBehTreeGraphEditor::TreeStructureModified()
{
	m_editedItem->MarkModified();
}

void CEdBehTreeGraphEditor::OnKeyDown( wxKeyEvent& event )
{
	if( event.ControlDown() )
	{
		if( event.GetKeyCode() == 'C' )
		{
			m_contextMenuItem = GetActiveItem() ? SafeCast< IBehTreeNodeDefinition >( GetActiveItem()->m_owner ) : NULL;
			OnCopyBranch( event );
		}
		else if( event.GetKeyCode() == 'X' )
		{
			m_contextMenuItem = GetActiveItem() ? SafeCast< IBehTreeNodeDefinition >( GetActiveItem()->m_owner ) : NULL;
			OnCutBranch( event );
		}
		else if( event.GetKeyCode() == 'V' )
		{
			m_contextMenuItem = GetActiveItem() ? SafeCast< IBehTreeNodeDefinition >( GetActiveItem()->m_owner ) : NULL;
			OnPasteBranch( event );
		}
	}
}
