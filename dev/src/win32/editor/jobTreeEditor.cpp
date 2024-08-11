/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/engine/behaviorGraphStack.h"
#include "../../common/game/actionPointComponent.h"
#include "../../common/game/movingAgentComponent.h"
#include "../../common/game/actor.h"
#include "../../common/game/definitionsManager.h"
#include "../../common/game/jobTreeLeaf.h"
#include "../../common/game/jobTreeNode.h"
#include "../../common/game/jobTree.h"
#include "../../common/core/depot.h"

#include "jobTreeEditor.h"
#include "jobTreePreviewPanel.h"
#include "propertiesPage.h"
#include "undoJobTreeEditor.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/engine/skeletalAnimationContainer.h"
#include "../../common/engine/dynamicLayer.h"

#define ID_ADD_ENTER_ANIMATION			4000
#define ID_ADD_LEAVE_ANIMATION			4001
#define ID_ADD_FAST_LEAVE_ANIMATION		4002
#define ID_ADD_NODE_AS_LEAF				4003
#define ID_ADD_NODE_AS_NODE				4004
#define ID_DELETE_JOBTREE_ITEM			4005

// Event table
BEGIN_EVENT_TABLE( CEdJobTreeEditor, wxSmartLayoutPanel )
	EVT_TOOL( XRCID("AddNode"), CEdJobTreeEditor::OnAddNodeToolItem )
	EVT_TOOL( XRCID("DeleteNode"), CEdJobTreeEditor::OnRemoveNodeToolItem )
	EVT_TOOL( XRCID("MoveUpTool"), CEdJobTreeEditor::OnMoveUp )
	EVT_TOOL( XRCID("MoveDownTool"), CEdJobTreeEditor::OnMoveDown )
	EVT_TOOL( XRCID("CopyTool"), CEdJobTreeEditor::OnCopyItem )
	EVT_TOOL( XRCID("PasteItemTool"), CEdJobTreeEditor::OnPasteItem )
	EVT_TREE_SEL_CHANGED( XRCID( "TreeView" ), CEdJobTreeEditor::OnNodeSelected )
	EVT_CLOSE( CEdJobTreeEditor::OnClose )
	EVT_TOOL( XRCID("playButton"), CEdJobTreeEditor::OnPlay )
	EVT_TOOL( XRCID("playWorldButton"), CEdJobTreeEditor::OnPlayWorld )
	EVT_TOOL( XRCID("stopButton"), CEdJobTreeEditor::OnStop )
	EVT_CHOICE( XRCID("SpeedChoiceBox"), CEdJobTreeEditor::OnSpeedChoice )
	EVT_TEXT( XRCID( "SpeedTextBox" ), CEdJobTreeEditor::OnCustomSpeedValue )
	EVT_TEXT_ENTER( XRCID( "animFilter"), CEdJobTreeEditor::OnAnimFilter )
	EVT_CHECKBOX( XRCID( "animFilterCheck"), CEdJobTreeEditor::OnAnimFilterCheck )
	EVT_TREE_SEL_CHANGED( XRCID( "AnimationTree" ), CEdJobTreeEditor::OnAnimationSelected )
	EVT_TREE_ITEM_ACTIVATED( XRCID( "AnimationTree" ), CEdJobTreeEditor::OnAnimationActivated )
	EVT_CHECKBOX( XRCID( "allAnimSetsCheck"), CEdJobTreeEditor::OnAllAnimsetsCheck )

	EVT_MENU( XRCID("menuItemSave"), CEdJobTreeEditor::OnSave )
	EVT_MENU( XRCID("menuItemUndo"), CEdJobTreeEditor::OnUndo )
	EVT_MENU( XRCID("menuItemRedo"), CEdJobTreeEditor::OnRedo )
END_EVENT_TABLE()

CEdJobTreeEditor::CEdJobTreeEditor( wxWindow* parent, CJobTree* tree )
: wxSmartLayoutPanel( parent, TXT("JobTreeEditor"), false )
, m_jobTree( tree )
, m_jobContext( NULL )
, m_selectedObject( NULL )
, m_currentAction( NULL )
, m_usePreviewEntityData( false )
, m_filterMode( BAFM_BeginsWith )
, m_allAnimsets( false )
, m_previewingTree( false )
{
	// Set icon
	wxIcon iconSmall;
	iconSmall.CopyFromBitmap( SEdResources::GetInstance().LoadBitmap( _T("IMG_MATERIALS") ) );
	SetIcon( iconSmall );

	wxString caption = wxString::Format( wxT("JobTree Editor - %s"), tree->GetFile()->GetDepotPath().AsChar() );
	SetTitle( wxString::Format( caption ) );

	// Create properties
	{
		wxPanel* pp = XRCCTRL( *this, "PropertiesPanel", wxPanel );
		PropertiesPageSettings settings;
		m_properties = new CEdPropertiesPage( pp, settings, nullptr );
		pp->GetSizer()->Add( m_properties, 1, wxEXPAND );
		m_properties->Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdJobTreeEditor::OnPropertiesChanged ), NULL, this );
		pp->Layout();
	}

	// Tree structure panel
	m_treeView = XRCCTRL( *this, "TreeView", wxTreeListCtrl );
	m_treeView->AddColumn( TXT("Node"), 300 );
	m_treeView->SetColumnEditable( 0, false );
	m_treeView->Connect( wxEVT_COMMAND_TREE_ITEM_MENU, wxTreeEventHandler( CEdJobTreeEditor::OnTreeItemContextMenu ), NULL, this );

	// Settings
	{
		wxPanel* pp = XRCCTRL( *this, "m_settingsPropertyPanel", wxPanel );
		PropertiesPageSettings settings;
		m_settingsPropertyPanel = new CEdPropertiesPage( pp, settings, nullptr );
		pp->GetSizer()->Add( m_settingsPropertyPanel, 1, wxEXPAND );
		pp->Layout();
	}
	m_settingsPropertyPanel->SetObject( &tree->m_settings );

	// Movement speed setup controls
	m_speedChoice = XRCCTRL( *this, "SpeedChoiceBox", wxChoice );
	m_speedValue = XRCCTRL( *this, "SpeedTextBox", wxTextCtrl );
	EJobMovementMode movementMode = m_jobTree->GetMovementMode();
	Int32 speedSelection;
	switch( movementMode )
	{
	case JMM_Walk:
		speedSelection = 0;
		break;
	case JMM_Run:
		speedSelection = 1;
		break;
	case JMM_CustomSpeed:
		speedSelection = 2;
		break;
	}
	m_speedChoice->SetSelection( speedSelection );
	UpdateCustomSpeedField();

	// Setup images
	{
		wxImageList* images = new wxImageList( 16, 16, true, 1 );
		images->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_BDI_EVENT") ) );
		images->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PRE") ) );
		images->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_POST") ) );
		images->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_ARROW_RIGHT") ) );
		images->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_ARROW_LEFT") ) );
		images->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_BDI_RUN") ) );
		images->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_ARROW_LEFT_RED") ) );
		m_treeView->AssignImageList( images );
	}

	// Populate controls with job tree data
	m_rootNode = tree->GetRootNode();
	if ( !m_rootNode )
	{
		m_rootNode = ::CreateObject< CJobTreeNode >( tree );
		// Set root items to None by default
		m_rootNode->m_leftItem = CName::NONE;
		m_rootNode->m_rightItem = CName::NONE;

		m_jobTree->SetRootNode( m_rootNode );
	}
	
	// Setup preview panel
	wxPanel* rp = XRCCTRL( *this, "PreviewPanel", wxPanel );
	wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );

	m_previewPanel = new CEdJobTreePreviewPanel( rp, this );
	m_previewPanel->m_widgetManager->SetWidgetSpace( RPWS_Local );
	sizer1->Add( m_previewPanel, 1, wxEXPAND, 0 );
	rp->SetSizer( sizer1 );
	rp->Layout();

	m_jobTree->AddToRootSet();
	LoadTree();
	m_previewPanel->SetJobTree( m_rootNode );

	m_animationTree = XRCCTRL( *this, "AnimationTree", wxTreeCtrl );
	FillAnimationTree();

	// Fill preview categories choice box
	m_previewCategoryChoice = XRCCTRL( *this, "m_previewCategoryChoice", wxChoice );

	wxSplitterWindow* treeSplitterWindow = XRCCTRL( *this, "TreeSplitter", wxSplitterWindow );
	if ( treeSplitterWindow ) 
	{
		treeSplitterWindow->SetSashPosition( treeSplitterWindow->GetSize().GetX() * 0.33f );
	}

	wxSplitterWindow* previewPropertySplitterWindow = XRCCTRL( *this, "PreviewPropertySplitter", wxSplitterWindow );
	if ( previewPropertySplitterWindow ) 
	{
		previewPropertySplitterWindow->SetSashPosition( previewPropertySplitterWindow->GetSize().GetX() * 0.66f );
	}

	wxSplitterWindow* previewSplitterWindow = XRCCTRL( *this, "PreviewSplitter", wxSplitterWindow );
	if ( previewSplitterWindow ) 
	{
		previewSplitterWindow->SetSashPosition( previewSplitterWindow->GetSize().GetY() * 0.66f );
	}

	{
		m_undoManager = new CEdUndoManager( this );
		m_undoManager->AddToRootSet();
		m_undoManager->SetMenuItems( GetMenuBar()->FindItem( XRCID("menuItemUndo") ), GetMenuBar()->FindItem( XRCID("menuItemRedo") ) );
		m_properties->SetUndoManager( m_undoManager );
		m_settingsPropertyPanel->SetUndoManager( m_undoManager );
	}

	// Update and finalize layout
	Layout();
	UpdateToolbarState();
	Show();

	FillPreviewCategoriesChoice();
}

CEdJobTreeEditor::~CEdJobTreeEditor()
{
	m_previewPanel->Stop();

	if( m_actorEntity.IsValid() )
	{
		m_actorEntity.Get()->Destroy();
		m_actorEntity = THandle< CEntity >::Null();
		GGame->GetActiveWorld()->DelayedActions();
	}

	{
		m_undoManager->RemoveFromRootSet();
		m_undoManager->Discard();
		m_undoManager = NULL;
	}

	delete m_jobContext;
	//if ( m_jobTree )
	//{
	//	m_jobTree->RemoveFromRootSet();
	//	m_jobTree = NULL;
	//}
}

void CEdJobTreeEditor::LoadTree()
{
	// prepare tree view
	m_treeView->Freeze();
	m_treeView->DeleteRoot();

	CJobTreeItem *treeItem = new CJobTreeItem();
	// Create root treeItem
	wxTreeItemId rootID = m_treeView->AddRoot( TXT("Root") );
	wxTreeItemId treeID = m_treeView->AppendItem( rootID, TXT("JobTreeRoot"), 0, 0, treeItem );
	treeItem->m_itemID = treeID;
	treeItem->m_node = m_rootNode;
	treeItem->m_itemType = JTIT_Node;

	// Fill tree contents
	FillTreeContents();

	// Show tree structure
	m_treeView->ExpandAll( treeID );

	// End update
	m_treeView->Thaw();
	m_treeView->Refresh();

	UpdateNodeIconsAndLabels( treeID );
}

void CEdJobTreeEditor::FillNodeContents( wxTreeItemId treeItemId )
{
	if ( !treeItemId.IsOk() )
	{
		return;
	}

	// Get item data
	wxTreeItemData* itemData = m_treeView->GetItemData( treeItemId );
	if ( !itemData )
	{
		return;
	}

	CJobTreeItem* treeItem = dynamic_cast<CJobTreeItem*>( itemData );
	if ( !treeItem )
	{
		return;
	}

	// Should be called only for tree items representing nodes
	if ( treeItem->m_itemType != JTIT_Node && treeItem->m_itemType != JTIT_TreeLeaf )
	{
		ASSERT( 0 && "Fill node contents called for non-node tree item!" );
		return;
	}

	CJobTreeNode* node = treeItem->m_node;
	ASSERT( node );

	if ( node->m_onEnterAction && !node->m_onLeaveAction && !node->m_onFastLeaveAction && node->m_childNodes.Empty() )
	{
		// This is a leaf of the tree, show it as one element of the tree, instead of a node with enter action
		treeItem->m_itemType = JTIT_TreeLeaf;

		m_treeView->SetItemImage( treeItemId, 5, wxTreeItemIcon_Normal );
		m_treeView->SetItemImage( treeItemId, 5, wxTreeItemIcon_Selected );
		m_treeView->SetItemText( treeItemId, node->m_onEnterAction->GetAnimName().AsString().AsChar() );
	}
	else
	{
		if ( node->m_onEnterAction )
		{
			// Append on enter action
			CJobTreeItem* onEnterActionItem = new CJobTreeItem();
			onEnterActionItem->m_action = node->m_onEnterAction;
			onEnterActionItem->m_itemType = JTIT_EnterAction;

#ifndef NO_EDITOR
			onEnterActionItem->m_action->SetAnimType( CName( TXT("EnterAnim") ) );
#endif

			onEnterActionItem->m_itemID = m_treeView->AppendItem( treeItemId, node->m_onEnterAction->GetAnimName().AsString().AsChar(), 3, 3, onEnterActionItem );
		}

		// Recurse child nodes
		TDynArray<CJobTreeNode*> childNodes = node->m_childNodes;
		for ( Uint32 i = 0; i<childNodes.Size(); ++i )
		{
			AppendNode( &treeItemId, childNodes[i] );
		}

		if ( node->m_onLeaveAction )
		{
			// Append on enter action
			CJobTreeItem* onLeaveActionItem = new CJobTreeItem();
			onLeaveActionItem->m_action = node->m_onLeaveAction;
			onLeaveActionItem->m_itemType = JTIT_ExitAction;

#ifndef NO_EDITOR
			onLeaveActionItem->m_action->SetAnimType( CName( TXT("ExitAnim") ) );
#endif

			onLeaveActionItem->m_itemID = m_treeView->AppendItem( treeItemId, node->m_onLeaveAction->GetAnimName().AsString().AsChar(), 4, 4, onLeaveActionItem );
		}

		if ( node->m_onFastLeaveAction )
		{
			// Append on enter action
			CJobTreeItem* onFastLeaveActionItem = new CJobTreeItem();
			onFastLeaveActionItem->m_action = node->m_onFastLeaveAction;
			onFastLeaveActionItem->m_itemType = JTIT_FastExitAction;

#ifndef NO_EDITOR
			onFastLeaveActionItem->m_action->SetAnimType( CName( TXT("FASTExitAnim") ) );
#endif

			onFastLeaveActionItem->m_itemID = m_treeView->AppendItem( treeItemId, node->m_onFastLeaveAction->GetAnimName().AsString().AsChar(), 6, 6, onFastLeaveActionItem );
		}
	}

	UpdateNodeIconsAndLabels( treeItemId );
}

wxTreeItemId CEdJobTreeEditor::AppendNode( wxTreeItemId* wxParent, CJobTreeNode* node )
{
	// Create tree item
	CJobTreeItem *treeItem = new CJobTreeItem();
	wxTreeItemId createdID = m_treeView->AppendItem( *wxParent, TXT(""), 0, 0, treeItem );
	treeItem->m_itemID = createdID;
	treeItem->m_node = node;
	treeItem->m_itemType = JTIT_Node;

	FillNodeContents( createdID );

	wxTreeItemData* parentItem = m_treeView->GetItemData( *wxParent );
	if ( parentItem )
	{
		CJobTreeItem* parentNodeItem = static_cast< CJobTreeItem* >( parentItem );
		ASSERT( parentNodeItem );
		parentNodeItem->m_itemType = JTIT_Node;
	}

	// Show tree structure
	m_treeView->ExpandAll( *wxParent );

	// return created item id
	return createdID;
}

void CEdJobTreeEditor::OnPropertiesChanged( wxCommandEvent& event )
{
	CEdPropertiesPage::SPropertyEventData* eventData = static_cast<CEdPropertiesPage::SPropertyEventData*>( event.GetClientData() );

	// make sure all parent nodes have categories of their child
	if ( eventData->m_propertyName.AsString() == TXT("validCategories") )
	{
		wxTreeItemId itemID = m_treeView->GetSelection();
		if ( itemID.IsOk() )
		{
			// Show node's properties
			CJobTreeItem* treeItem = static_cast<CJobTreeItem*>( m_treeView->GetItemData( itemID ) );
			CJobTreeNode* selectedNode = treeItem->m_node;

			PropagateCategories( selectedNode, selectedNode->m_validCategories );

			FillPreviewCategoriesChoice();
		}

		
		//ExtractCategories( m_rootNode, categories );
	}
	else if ( eventData->m_propertyName.AsString() == TXT("skipForNPCSpawnedInAP") )
	{
		wxTreeItemId itemID = m_treeView->GetSelection();
		if ( itemID.IsOk() )
		{
			// Show node's properties
			CJobTreeItem* treeItem = static_cast<CJobTreeItem*>( m_treeView->GetItemData( itemID ) );
			CJobTreeNode* selectedNode = treeItem->m_node;
			selectedNode->m_onEnterAction->GetAnimName();
			
		}
	}

	StoreSelection();
	LoadTree();
	RestoreSelection();
}

void CEdJobTreeEditor::OnTreeItemContextMenu( wxTreeEvent& event )
{
	wxMenu menu;

	wxTreeItemId subjectItem = event.GetItem();
	wxTreeItemData* itemData = m_treeView->GetItemData( subjectItem );
	if ( !itemData )
	{
		return;
	}

	CJobTreeItem* subjectJobTreeItem = static_cast< CJobTreeItem* >( itemData );
	ASSERT( subjectJobTreeItem );

	Bool anyOptionAdded = false;

	if ( subjectJobTreeItem->m_itemType == JTIT_Node && !subjectJobTreeItem->m_node->m_onEnterAction )
	{
		// This is a node and it is missing the on enter action, provide an option to add it
		menu.Append( ID_ADD_ENTER_ANIMATION, TEXT("Add enter animation"), wxEmptyString );
		menu.Connect( ID_ADD_ENTER_ANIMATION, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdJobTreeEditor::OnAddEnterAnimation ), new CJobTreeContextMenuItem( subjectJobTreeItem ), this );
		anyOptionAdded = true;
	}

	if ( ( subjectJobTreeItem->m_itemType == JTIT_Node || subjectJobTreeItem->m_itemType == JTIT_TreeLeaf ) && !subjectJobTreeItem->m_node->m_onLeaveAction )
	{
		// This is a node and it is missing the on leave action, provide an option to add it
		menu.Append( ID_ADD_LEAVE_ANIMATION, TEXT("Add leave animation"), wxEmptyString );
		menu.Connect( ID_ADD_LEAVE_ANIMATION, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdJobTreeEditor::OnAddLeaveAnimation ), new CJobTreeContextMenuItem( subjectJobTreeItem ), this );
		anyOptionAdded = true;
	}

	if ( ( subjectJobTreeItem->m_itemType == JTIT_Node || subjectJobTreeItem->m_itemType == JTIT_TreeLeaf ) && !subjectJobTreeItem->m_node->m_onFastLeaveAction )
	{
		// This is a node and it is missing the on fast leave action, provide an option to add it
		menu.Append( ID_ADD_FAST_LEAVE_ANIMATION, TEXT("Add fast leave animation"), wxEmptyString );
		menu.Connect( ID_ADD_FAST_LEAVE_ANIMATION, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdJobTreeEditor::OnAddFastLeaveAnimation ), new CJobTreeContextMenuItem( subjectJobTreeItem ), this );
		anyOptionAdded = true;
	}

	if ( anyOptionAdded )
	{
		menu.AppendSeparator();
	}

	if ( subjectJobTreeItem->m_itemType == JTIT_Node || subjectJobTreeItem->m_itemType == JTIT_TreeLeaf )
	{
		// Add an option to add empty child node
		menu.Append( ID_ADD_NODE_AS_NODE, TEXT("Add child node"), wxEmptyString );
		menu.Connect( ID_ADD_NODE_AS_NODE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdJobTreeEditor::OnAddNodeToolItem ), NULL, this );
		menu.AppendSeparator();
	}

	menu.Append( ID_DELETE_JOBTREE_ITEM, TEXT("Remove"), wxEmptyString );
	menu.Connect( ID_DELETE_JOBTREE_ITEM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdJobTreeEditor::OnRemoveNodeToolItem ), NULL, this );

	// Show menu
	PopupMenu( &menu );
}

void CEdJobTreeEditor::ExtractCategories( CJobTreeNode* node, TDynArray<CName>& categories )
{
	ASSERT( 0 && "Not used" );
	if ( 0 )
	{
		if ( !node ) 
		{
			return;
		}
		if ( node->m_childNodes.Empty() )
		{
			for ( Uint32 i=0; i<node->m_validCategories.Size(); ++i )
			{
				if ( node->m_validCategories[i] != CName::NONE )
				{
					categories.PushBackUnique( node->m_validCategories[i] );
				}	
			}
			return;
		}
		TDynArray<CName> tempCategories;
		for ( Uint32 i=0; i<node->m_childNodes.Size(); ++i )
		{
			ExtractCategories( node->m_childNodes[i], tempCategories );
			categories.PushBackUnique( tempCategories );
			tempCategories.Clear();
		}
		node->m_validCategories.PushBackUnique( categories );
	}
	
}

void CEdJobTreeEditor::PropagateCategories( CJobTreeNode* node, const TDynArray<CName>& categories )
{
	TDynArray< CJobTreeNode* >& childNodes = node->m_childNodes;
	if ( categories.Size() )
	{
		node->m_validCategories.PushBackUnique( categories );
	}
	for ( Uint32 i=0; i<childNodes.Size(); ++i )
	{
		childNodes[i]->m_validCategories.Clear();
		PropagateCategories( childNodes[i], categories );
	}
}

void CEdJobTreeEditor::UpdateToolbarState()
{
	wxToolBar* toolbar = XRCCTRL( *this, "treeToolbar", wxToolBar );
	wxTreeItemId itemID = m_treeView->GetSelection();
	if ( itemID.IsOk() )
	{
		toolbar->Enable();

		wxTreeItemId parentID = m_treeView->GetItemParent( itemID );

		if ( CJobTreeItem* item = dynamic_cast< CJobTreeItem* >( m_treeView->GetItemData( itemID ) ) )
		{
			Bool nodeOrLeaf = ( item->m_itemType == JTIT_Node || item->m_itemType == JTIT_TreeLeaf );

			toolbar->EnableTool( XRCID("DeleteNode"),    parentID.IsOk() && m_treeView->GetRootItem() != parentID && ( item->m_node || item->m_action ) );
			toolbar->EnableTool( XRCID("AddNode"),       nodeOrLeaf );
			toolbar->EnableTool( XRCID("PasteItemTool"), nodeOrLeaf && m_itemToPasteData.m_itemType != JTIT_Invalid );

			if ( CJobTreeItem* parent = dynamic_cast< CJobTreeItem* >( m_treeView->GetItemData( parentID ) ) )
			{
				Uint32 index = parent->m_node->m_childNodes.GetIndex( item->m_node );
				toolbar->EnableTool( XRCID("MoveUpTool"),   nodeOrLeaf && index >= 1 );
				toolbar->EnableTool( XRCID("MoveDownTool"), nodeOrLeaf && index < parent->m_node->m_childNodes.Size()-1 );
			}
		}
	}
	else
	{
		toolbar->Disable();
	}
}

void CEdJobTreeEditor::OnNodeSelected( wxTreeEvent& event )
{
	UpdateToolbarState();

	// Grab item
	wxTreeItemId itemID = m_treeView->GetSelection();
	if ( itemID.IsOk() )
	{
		// Show node's properties
		wxTreeItemData* itemData = m_treeView->GetItemData( itemID );
		if ( !itemData )
		{
			return;
		}

		CJobTreeItem* treeItem = dynamic_cast<CJobTreeItem*>( itemData );
		if ( !treeItem )
		{
			return;
		}

		m_currentAction = NULL;
		if ( treeItem->m_itemType == JTIT_Node )
		{
			// Set properties source
			m_properties->SetObject( treeItem->m_node );
		}
		else if ( treeItem->m_itemType == JTIT_EnterAction || treeItem->m_itemType == JTIT_ExitAction || treeItem->m_itemType == JTIT_FastExitAction )
		{
			m_properties->SetObject( treeItem->m_action );
			m_currentAction = treeItem->m_action;
		}
		else if ( treeItem->m_itemType == JTIT_TreeLeaf )
		{
			m_properties->SetObject( treeItem->m_node->m_onEnterAction );
			m_currentAction = treeItem->m_node->m_onEnterAction;
		}

		if ( m_previewPanel && ! m_previewingTree )
		{
			if ( m_previewPanel->GetActor() )
			{
				m_previewPanel->GetActor()->Teleport(Vector::ZEROS, EulerAngles::ZEROS);
			}
			if ( m_currentAction )
			{
				m_previewPanel->PlayAnimation( m_currentAction->GetAnimName(), m_currentAction->GetItemName() );
			}
			else
			{
				m_previewPanel->Stop();
			}
		}
	}
}

void CEdJobTreeEditor::OnAddNodeToolItem( wxCommandEvent& event )
{
	wxTreeItemId selectedItem = m_treeView->GetSelection();
	// Check if we have target parent selected
	if ( selectedItem.IsOk() )
	{
		// Fetch jobTreeNode from parent tree view item
		CJobTreeItem* treeItem = static_cast<CJobTreeItem*>( m_treeView->GetItemData( selectedItem ) );
		if ( treeItem && ( treeItem->m_itemType == JTIT_Node || treeItem->m_itemType == JTIT_TreeLeaf ) )
		{
			// Create new node
			CJobTreeNode* newNode = CreateObject<CJobTreeNode>( treeItem->m_node );
			treeItem->m_node->m_childNodes.PushBack( newNode );
			
			newNode->m_validCategories.PushBack( treeItem->m_node->m_validCategories );

			CUndoJobEditorNodeExistance::CreateCreationStep( *m_undoManager, this, treeItem->m_node, newNode );

			// Rebuild tree starting from parent of the added node
			UpdateTreeFragment( FindTreeItemForObject( treeItem->m_node ) );
			m_treeView->SelectItem( FindTreeItemForObject( newNode ) );
		}
		return;
	}
}

void CEdJobTreeEditor::OnRemoveNodeToolItem( wxCommandEvent& event )
{
	wxTreeItemId selectedItem = m_treeView->GetSelection();

	if ( !selectedItem.IsOk() )
	{
		return;
	}

	m_treeView->UnselectAll();

	// We don't want to delete the whole tree here
	if ( !m_treeView->GetItemParent( selectedItem ).IsOk() || m_treeView->GetRootItem() == m_treeView->GetItemParent( selectedItem ) ) return;

	// Prepare parent for the bad news
	wxTreeItemId parentID = m_treeView->GetItemParent( selectedItem );
	CJobTreeItem* itemToDeleteParent = static_cast<CJobTreeItem*> (m_treeView->GetItemData( parentID ) );
	CJobTreeItem* itemToDeleteData = static_cast<CJobTreeItem*> (m_treeView->GetItemData( selectedItem ) );
	ASSERT( itemToDeleteParent && itemToDeleteData );

	if ( itemToDeleteData->m_node )
	{
		CJobTreeNode* parentNode = itemToDeleteParent->m_node;
		CJobTreeNode* toDeleteNode = itemToDeleteData->m_node;

		// Delete node
		CUndoJobEditorNodeExistance::CreateDeletionStep( *m_undoManager, this, itemToDeleteParent->m_node, itemToDeleteData->m_node );
		parentNode->m_childNodes.Remove( toDeleteNode );
		itemToDeleteData->m_node = NULL;
		m_treeView->Delete( selectedItem );
	}
	else if ( itemToDeleteData->m_action )
	{
		CJobTreeNode* parentNode = itemToDeleteParent->m_node;

		// Delete node
		if ( itemToDeleteData->m_itemType == JTIT_EnterAction )
		{
			CUndoJobActionExistance::CreateDeletionStep( *m_undoManager, this, parentNode, CUndoJobActionExistance::Enter );
			parentNode->m_onEnterAction = NULL;
		}
		else if ( itemToDeleteData->m_itemType == JTIT_ExitAction )
		{
			CUndoJobActionExistance::CreateDeletionStep( *m_undoManager, this, parentNode, CUndoJobActionExistance::Leave );
			parentNode->m_onLeaveAction = NULL;
		}
		else if ( itemToDeleteData->m_itemType == JTIT_FastExitAction )
		{
			CUndoJobActionExistance::CreateDeletionStep( *m_undoManager, this, parentNode, CUndoJobActionExistance::FastLeave );
			parentNode->m_onFastLeaveAction = NULL;
		}

		m_treeView->Delete( selectedItem );
	}

	// Rebuild action categories
	//TDynArray<CName> categories;
	//ExtractCategories( m_rootNode, categories );

	UpdateTreeFragment( parentID );
}

void CEdJobTreeEditor::OnOK( wxCommandEvent& event )
{
	Hide();
}

void CEdJobTreeEditor::OnMove( Bool up )
{
	wxTreeItemId selectedItem = m_treeView->GetSelection();
	wxTreeItemId selectedParent = m_treeView->GetItemParent( selectedItem );

	Uint32 numItems = m_treeView->GetChildrenCount( selectedParent, false );
	if ( numItems == 1 ) return;

	wxTreeItemId sibling = up ? m_treeView->GetPrevSibling( selectedItem ) : m_treeView->GetNextSibling( selectedItem );
	if ( !sibling.IsOk() ) return;

	CJobTreeItem* siblingData = static_cast<CJobTreeItem*>( m_treeView->GetItemData( sibling ) );
	CJobTreeItem* selectedData = static_cast<CJobTreeItem*>( m_treeView->GetItemData( selectedItem ) );
	CJobTreeItem* parentData = static_cast<CJobTreeItem*>( m_treeView->GetItemData( selectedParent ) );

	if ( selectedData->m_itemType != JTIT_Node && selectedData->m_itemType != JTIT_TreeLeaf )
	{
		return;
	}
	if ( siblingData->m_itemType != JTIT_Node && siblingData->m_itemType != JTIT_TreeLeaf )
	{
		return;
	}

	CJobTreeNode* parentNode = parentData->m_node;
	CJobTreeNode* selectedNode = selectedData->m_node;
	CJobTreeNode* siblingNode = siblingData->m_node;

	CUndoJobEditorNodeMove::CreateStep( *m_undoManager, this, parentNode, selectedNode, up );

	StoreSelection();

	// Switch positions
	Uint32 index = parentNode->m_childNodes.GetIndex( selectedNode );
	parentNode->m_childNodes[index] = siblingNode;

	if ( up )
	{
		ASSERT( index >= 1 );
		parentNode->m_childNodes[index-1] = selectedNode;
	}
	else
	{
		ASSERT( index < parentNode->m_childNodes.Size()-1 );
		parentNode->m_childNodes[index+1] = selectedNode;
	}

	// Recreate tree view branch
	m_treeView->UnselectAll();

	FillTreeContents();

	RestoreSelection();
}

void CEdJobTreeEditor::OnMoveUp( wxCommandEvent& event )
{
	OnMove( true );
}

void CEdJobTreeEditor::OnMoveDown( wxCommandEvent& event )
{
	OnMove( false );
}

void CEdJobTreeEditor::UpdateNodeIconsAndLabels( wxTreeItemId itemId )
{
	wxTreeItemIdValue cookie;
	if ( itemId.IsOk() )
	{
		CJobTreeItem* item = static_cast< CJobTreeItem* >( m_treeView->GetItemData( itemId ) );	
		ASSERT( item );

		if ( item->m_itemType == JTIT_TreeLeaf )
		{
			m_treeView->SetItemImage( itemId, 5, wxTreeItemIcon_Normal );
			m_treeView->SetItemImage( itemId, 5, wxTreeItemIcon_Selected );
			m_treeView->SetItemText( itemId, item->m_node->m_onEnterAction->GetAnimName().AsString().AsChar() );
		}
		else if ( item->m_itemType == JTIT_EnterAction )
		{
			m_treeView->SetItemImage( itemId, 3, wxTreeItemIcon_Normal );
			m_treeView->SetItemImage( itemId, 3, wxTreeItemIcon_Selected );

			m_treeView->SetItemText( itemId, item->m_action->GetAnimName().AsString().AsChar() );
		}
		else if ( item->m_itemType == JTIT_ExitAction )
		{
			m_treeView->SetItemImage( itemId, 4, wxTreeItemIcon_Normal );
			m_treeView->SetItemImage( itemId, 4, wxTreeItemIcon_Selected );

			m_treeView->SetItemText( itemId, item->m_action->GetAnimName().AsString().AsChar() );
		}
		else if ( item->m_itemType == JTIT_FastExitAction )
		{
			m_treeView->SetItemImage( itemId, 6, wxTreeItemIcon_Normal );
			m_treeView->SetItemImage( itemId, 6, wxTreeItemIcon_Selected );
			m_treeView->SetItemText( itemId, item->m_action->GetAnimName().AsString().AsChar() );
		}
		else if ( item->m_itemType == JTIT_Node )
		{
			m_treeView->SetItemImage( itemId, 0, wxTreeItemIcon_Normal );
			m_treeView->SetItemImage( itemId, 0, wxTreeItemIcon_Selected );

			String nodeDescription;
			if ( item->m_node->m_selectionMode == SM_RANDOM )
			{
				nodeDescription += TXT("random");
			}
			else
			{
				nodeDescription += TXT("sequence");
			}

			nodeDescription += TXT(" x");
			nodeDescription += ToString( (Int32)item->m_node->m_iterations );
			
			if ( item->m_node->m_looped )
				nodeDescription += TXT(" - LOOPED");				

			m_treeView->SetItemText( itemId, nodeDescription.AsChar() );

			Int32 childrenCount = m_treeView->GetChildrenCount( itemId, false );
			wxTreeItemId child = m_treeView->GetFirstChild( itemId, cookie );
			for ( Int32 i=0; i<childrenCount; ++i )
			{
				UpdateNodeIconsAndLabels( child );
				child = m_treeView->GetNextSibling( child );
			}
		}
	}
}

void CEdJobTreeEditor::OnCopyItem( wxCommandEvent& event )
{
	wxTreeItemId itemID = m_treeView->GetSelection();
	if ( !itemID.IsOk() )
	{
		return;
	}

	// Release the previous copied item to prevent from a leak
	if ( m_itemToPasteData.m_action != NULL )
	{
		m_itemToPasteData.m_action->RemoveFromRootSet();
	}
	if ( m_itemToPasteData.m_node != NULL )
	{
		m_itemToPasteData.m_node->RemoveFromRootSet();
	}

	// Copy aside to be pasted later
	m_itemToPasteData = *dynamic_cast< CJobTreeItem* >( m_treeView->GetItemData( itemID ) );

	// Prevent the objects inside from being collected
	if ( m_itemToPasteData.m_action )
	{
		m_itemToPasteData.m_action->AddToRootSet();
	}
	if ( m_itemToPasteData.m_node )
	{
		m_itemToPasteData.m_node->AddToRootSet();
	}

	UpdateToolbarState();
}

void CEdJobTreeEditor::OnPasteItem( wxCommandEvent& event )
{
	if ( !m_treeView->GetSelection().IsOk() )
	{
		return;
	}

	if ( m_itemToPasteData.m_itemType == JTIT_Invalid )
	{
		return;
	}

	wxTreeItemId selectedItem = m_treeView->GetSelection();
	CJobTreeItem* selectedItemData = static_cast< CJobTreeItem* >( m_treeView->GetItemData( selectedItem ) );
		
	if ( !( selectedItemData->m_itemType == JTIT_Node || selectedItemData->m_itemType == JTIT_TreeLeaf )  )
	{
		// Can't paste into non-node item
		return;
	}

	CJobTreeNode* selectedNode = selectedItemData->m_node;
	ASSERT( selectedNode );

	CJobTreeNode* clonedNode = NULL;
	CJobActionBase* jobActionBase = NULL;
	switch( m_itemToPasteData.m_itemType )
	{
	case JTIT_Node:
	case JTIT_TreeLeaf:
		{
			clonedNode = static_cast< CJobTreeNode* >( m_itemToPasteData.m_node->Clone( selectedNode ) );
			// Append it
			selectedNode->m_childNodes.PushBack( clonedNode );
			CUndoJobEditorNodeExistance::CreateCreationStep( *m_undoManager, this, selectedNode, clonedNode );
		}			
		break;
	case JTIT_EnterAction:
		{
			if ( selectedNode->m_onEnterAction )
			{
				// Already has an enter action
				return;
			}
			selectedNode->m_onEnterAction = dynamic_cast< CJobAction* >( m_itemToPasteData.m_action->Clone( selectedNode ) );
			CUndoJobActionExistance::CreateCreationStep( *m_undoManager, this, selectedNode, CUndoJobActionExistance::Enter );
		}
		break;
	case JTIT_ExitAction:
		{
			if ( selectedNode->m_onLeaveAction )
			{
				// Already has a leave action
				return;
			}
			selectedNode->m_onLeaveAction = dynamic_cast< CJobAction* >( m_itemToPasteData.m_action->Clone( selectedNode ) );
			CUndoJobActionExistance::CreateCreationStep( *m_undoManager, this, selectedNode, CUndoJobActionExistance::Leave );
		}
		break;
	case JTIT_FastExitAction:
		{
			if ( selectedNode->m_onFastLeaveAction )
			{
				// Already has a fast leave action
				return;
			}
			selectedNode->m_onFastLeaveAction = dynamic_cast< CJobForceOutAction* >( m_itemToPasteData.m_action->Clone( selectedNode ) );
			CUndoJobActionExistance::CreateCreationStep( *m_undoManager, this, selectedNode, CUndoJobActionExistance::FastLeave );
		}
		break;		
	}

	UpdateTreeFragment( FindTreeItemForObject( selectedNode ) );
}

void CEdJobTreeEditor::OnPlay( wxCommandEvent& event )
{
	String category = m_previewCategoryChoice->GetStringSelection().c_str();

	m_previewPanel->Play( CName( category ) );
	m_previewingTree = true;
}

void CEdJobTreeEditor::OnStop( wxCommandEvent& event )
{
	m_previewPanel->Stop();

	if( m_actorEntity.IsValid() )
	{
		m_actorEntity.Get()->Destroy();
		GGame->GetActiveWorld()->DelayedActions();
		m_actorEntity = THandle< CEntity >::Null();
	}

	m_previewingTree = false;
}

void CEdJobTreeEditor::OnPlayWorld( wxCommandEvent& event )
{
	ASSERT( GGame != NULL );

	// Initialize job context
	if ( m_jobContext == NULL )
	{
		m_jobContext = new SJobTreeExecutionContext;
	}
	m_jobContext->Reset();
	m_jobContext->m_currentCategories.PushBackUnique( CName( m_previewCategoryChoice->GetStringSelection().c_str() ) );

	// Get world
	CWorld* world = GGame->GetActiveWorld();
	if( world == NULL )
	{
		wxMessageBox( TXT( "You need to have world opened" ) );
		return;
	}

	// Get actor template
	if ( m_previewPanel->GetActor() == NULL  )
	{
		wxMessageBox( TXT( "You need to select actor in preview first" ) );
		return;
	}

	CEntityTemplate* entityTemplate = m_previewPanel->GetActor()->GetEntityTemplate();
	ASSERT( entityTemplate != NULL );

	const CJobActionBase* action = m_rootNode->GetNextAction( *m_jobContext );
	if( action == NULL )
	{
		wxMessageBox( TXT( "Empty job tree..." ) );
		return;
	}

	String actionPointName = action->GetPlace().AsString();

	// Find entity for previewing
	TDynArray< CEntity* > selectedEntities;
	world->GetSelectionManager()->GetSelectedEntities( selectedEntities );
	
	if( selectedEntities.Size() == 0 )
	{
		wxMessageBox( TXT( "You need to select entity on the world" ) );
		return;
	}

	CComponent* actionPointComponent = NULL;

	// Find needed component
	for( TDynArray< CEntity* >::iterator entityIter = selectedEntities.Begin();
		entityIter != selectedEntities.End(); ++entityIter )
	{
		CEntity* entity = *entityIter;
		ASSERT( entity != NULL );

		// Find
		actionPointComponent = entity->FindComponent( actionPointName );
		if( actionPointComponent != NULL )
		{
			break;
		}
	}

	if( actionPointComponent == NULL )
	{
		wxMessageBox( String::Printf( TXT( "Selected entity does not have component '%s'" ),
			actionPointName.AsChar() ).AsChar() );
		return;
	}

	// Spawn entity
	if( m_actorEntity.IsValid() )
	{
		m_actorEntity.Get()->Destroy();
		GGame->GetActiveWorld()->DelayedActions();
		m_actorEntity = THandle< CEntity >::Null();
	}

	EntitySpawnInfo info;
	info.m_template = entityTemplate;
	info.m_spawnPosition = actionPointComponent->GetWorldPosition();
	info.m_spawnRotation = actionPointComponent->GetWorldRotation();
	// use pointer as randomizer in entity name
	info.m_name = String::Printf( TXT( "JobPreviewActorEntity_%d" ), ( Int32 ) this );

	m_actorEntity = THandle< CEntity >::Null();
	CEntity* actorEntity = world->GetDynamicLayer()->CreateEntitySync( info );
	if ( actorEntity == nullptr )
	{
		wxMessageBox( TXT( "Error spawning entity" ) );
		return;
	}
	m_actorEntity = actorEntity;

	// Wait for attaching
	world->DelayedActions();

	if ( CAnimatedComponent* ac = actorEntity->GetRootAnimatedComponent() )
	{
		ac->SetUseExtractedMotion( true );
		if ( CMovingAgentComponent* const mac = Cast< CMovingAgentComponent >( ac ) )
		{
			mac->ForceEntityRepresentation( true );
			mac->GetRepresentationStack()->OnActivate( info.m_spawnPosition, info.m_spawnRotation );
		}
	}

	actorEntity->Teleport( info.m_spawnPosition, info.m_spawnRotation );

	CActor* const actor = SafeCast< CActor >( actorEntity );
	
#ifndef NO_EDITOR
	if ( const CActionPointComponent* const apComponent = Cast< CActionPointComponent >( actionPointComponent ) )
	{
		if ( apComponent->GetIgnoreCollisions() )
		{
			for ( ComponentIterator< CAnimatedComponent > it( actor ); it; ++it )
			{
				(*it)->SetAllowConstraintsUpdate( false );
			}
		}
	}
#endif

	// Start working
	if ( actor->GetAgent() && actor->GetAgent()->GetBehaviorStack() )
	{
		// Setup slot
		SBehaviorSlotSetup slotSetup;
		slotSetup.m_blendIn = action->GetBlendIn();
		slotSetup.m_blendOut = action->GetBlendOut();
		slotSetup.m_listener = this;

		actor->GetAgent()->GetBehaviorStack()->PlaySlotAnimation( CEdJobTreePreviewPanel::JTP_SLOT_NAME, action->GetAnimName(), &slotSetup );
	}	
	m_previewingTree = true;
}

void CEdJobTreeEditor::OnSlotAnimationEnd( const CBehaviorGraphAnimationBaseSlotNode * sender, CBehaviorGraphInstance& instance, EStatus status )
{
	if ( status == ISlotAnimationListener::S_Finished || status == ISlotAnimationListener::S_BlendOutStarted )
	{
		const CJobActionBase* action = m_rootNode->GetNextAction( *m_jobContext );
		if ( action )
		{
			SBehaviorSlotSetup slotSetup;
			slotSetup.m_blendIn = action->GetBlendIn();
			slotSetup.m_blendOut = action->GetBlendOut();
			slotSetup.m_listener = this;

			sender->PlayAnimation( instance, action->GetAnimName(), &slotSetup );
		}
	}
}

void CEdJobTreeEditor::OnSave( wxCommandEvent& event )
{
	// Validate job tree
	TDynArray< String > logLines;
	if ( !m_rootNode->Validate( logLines ) )
	{
		String fullLog;
		for ( Uint32 i=0; i<logLines.Size(); ++i )
		{
			VALIDATION_FAIL( logLines[i].AsChar() );
			fullLog += TXT("- ") + logLines[i] + TXT("\n");
		}
		wxMessageBox( fullLog.AsChar(), TXT("Validation failed! Job tree WAS NOT SAVED!"), wxOK | wxICON_ERROR|wxCENTRE );
		return;
	}

	// Save
	m_jobTree->Save();
}

void CEdJobTreeEditor::OnUndo( wxCommandEvent& event )
{
	m_undoManager->Undo();
}

void CEdJobTreeEditor::OnRedo( wxCommandEvent& event )
{
	m_undoManager->Redo();
}

void CEdJobTreeEditor::OnSpeedChoice( wxCommandEvent& event )
{
	Int32 selection = m_speedChoice->GetSelection();
	EJobMovementMode mode;
	switch( selection )
	{
	case 0:
		mode = JMM_Walk;
		break;
	case 1:
		mode = JMM_Run;
		break;
	case 2:
		mode = JMM_CustomSpeed;
		break;
	}

	m_jobTree->SetMovementMode( mode );

	UpdateCustomSpeedField();
}

void CEdJobTreeEditor::UpdateCustomSpeedField()
{
	if ( m_jobTree->GetMovementMode() == JMM_CustomSpeed )
	{
		m_speedValue->Show();
		m_speedValue->SetValue( ToString( m_jobTree->GetCustomSpeed() ).AsChar() );
	}
	else
	{
		m_speedValue->Hide();
	}
}

void CEdJobTreeEditor::OnCustomSpeedValue( wxCommandEvent& event )
{
	Float speed;
	FromString<Float>( m_speedValue->GetValue().wc_str(), speed );
	m_jobTree->SetCustomSpeed( speed );
}

void CEdJobTreeEditor::OnSettingsChanged( wxCommandEvent& event )
{
	// Not used for now
}

void CEdJobTreeEditor::OnAddEnterAnimation( wxCommandEvent& event )
{
	CJobTreeContextMenuItem* contextItem = ( CJobTreeContextMenuItem* )( event.m_callbackUserData );
	ASSERT( contextItem );
	CJobTreeItem* item = contextItem->m_item;
	ASSERT( item );

	CJobTreeNode* node = item->m_node;
	ASSERT( node );
	ASSERT( !node->m_onEnterAction );

	node->m_onEnterAction = CreateObject< CJobAction >( node );
	CUndoJobActionExistance::CreateCreationStep( *m_undoManager, this, node, CUndoJobActionExistance::Enter );

	UpdateTreeFragment( FindTreeItemForObject( node ) );
	m_treeView->SelectItem( FindTreeItemForObject( node->m_onEnterAction ) );
}

void CEdJobTreeEditor::OnAddLeaveAnimation( wxCommandEvent& event )
{
	CJobTreeContextMenuItem* contextItem = ( CJobTreeContextMenuItem* )( event.m_callbackUserData );
	ASSERT( contextItem );
	CJobTreeItem* item = contextItem->m_item;
	ASSERT( item );

	CJobTreeNode* node = item->m_node;
	ASSERT( node );
	ASSERT( !node->m_onLeaveAction );

	node->m_onLeaveAction = CreateObject< CJobAction >( node );
	CUndoJobActionExistance::CreateCreationStep( *m_undoManager, this, node, CUndoJobActionExistance::Leave );

	UpdateTreeFragment( FindTreeItemForObject( node ) );
	m_treeView->SelectItem( FindTreeItemForObject( node->m_onLeaveAction ) );
}

void CEdJobTreeEditor::OnAddFastLeaveAnimation( wxCommandEvent& event )
{
	CJobTreeContextMenuItem* contextItem = ( CJobTreeContextMenuItem* )( event.m_callbackUserData );
	ASSERT( contextItem );
	CJobTreeItem* item = contextItem->m_item;
	ASSERT( item );

	CJobTreeNode* node = item->m_node;
	ASSERT( node );
	ASSERT( !node->m_onFastLeaveAction );

	node->m_onFastLeaveAction = CreateObject< CJobForceOutAction >( node );
	CUndoJobActionExistance::CreateCreationStep( *m_undoManager, this, node, CUndoJobActionExistance::FastLeave );

	UpdateTreeFragment( FindTreeItemForObject( node ) );
	m_treeView->SelectItem( FindTreeItemForObject( node->m_onFastLeaveAction ) );
}

wxTreeItemId CEdJobTreeEditor::FindTreeItemForObject( wxTreeItemId startItemId, const CObject* object )
{
	wxTreeItemData* startItemData = m_treeView->GetItemData( startItemId );
	if ( startItemData )
	{
		CJobTreeItem* startJobTreeItem = static_cast< CJobTreeItem* >( startItemData );
		ASSERT( startJobTreeItem );
		if ( ( ( CObject* )startJobTreeItem->m_node ) == object || ( ( CObject* )startJobTreeItem->m_action ) == object )
		{
			// start item is the item we want, return it
			return startItemId;
		}
		if ( startJobTreeItem->m_itemType == JTIT_TreeLeaf )
		{
			ASSERT( startJobTreeItem->m_node && startJobTreeItem->m_node->m_onEnterAction );
			if ( ( CObject* )startJobTreeItem->m_node->m_onEnterAction == object )
			{
				return startItemId;
			}
		}
	}

	Int32 childrenCount = m_treeView->GetChildrenCount( startItemId, false );
	wxTreeItemIdValue cookie;
	wxTreeItemId child = m_treeView->GetFirstChild( startItemId, cookie );
	for ( Int32 i=0; i<childrenCount; ++i )
	{
		wxTreeItemId candidate = FindTreeItemForObject( child, object );
		if ( candidate.IsOk() )
		{
			return candidate;
		}
		child = m_treeView->GetNextSibling( child );
	}

	// Not found, return invalid tree item id
	return wxTreeItemId();
}

wxTreeItemId CEdJobTreeEditor::FindTreeItemForObject( const CObject* object )
{
	wxTreeItemId root = m_treeView->GetRootItem();

	return FindTreeItemForObject( root, object );
}

void CEdJobTreeEditor::FillTreeContents()
{
	wxTreeItemId rootId = m_treeView->GetRootItem();
	wxTreeItemIdValue cookie;
	wxTreeItemId rootNodeId = m_treeView->GetFirstChild( rootId, cookie );
	if ( m_treeView->HasChildren( rootNodeId ) )
	{
		m_treeView->DeleteChildren( rootNodeId );
	}

	m_treeView->Freeze();
	FillNodeContents( rootNodeId );
	m_treeView->Thaw();
	m_treeView->Refresh();
}

void CEdJobTreeEditor::StoreSelection()
{
	wxTreeItemId selectedId = m_treeView->GetSelection();
	if ( selectedId )
	{
		wxTreeItemData* selectedItemData = m_treeView->GetItemData( selectedId );
		if ( selectedItemData )
		{
			CJobTreeItem* selectedJobTreeItem = static_cast< CJobTreeItem* >( selectedItemData );
			ASSERT( selectedJobTreeItem );

			if ( selectedJobTreeItem->m_node )
			{
				m_selectedObject = selectedJobTreeItem->m_node;
			}
			else if ( selectedJobTreeItem->m_action )
			{
				m_selectedObject = selectedJobTreeItem->m_action;
			}

			ASSERT( m_selectedObject );
		}
	}
}

void CEdJobTreeEditor::RestoreSelection()
{
	if ( m_selectedObject )
	{
		wxTreeItemId foundItem = FindTreeItemForObject( m_selectedObject );
		if ( foundItem.IsOk() )
		{
			m_treeView->SelectItem( foundItem );
		}

		m_selectedObject = NULL;
	}
}

void CEdJobTreeEditor::OnActionPreviewStarted( const CJobActionBase* action )
{
	wxTreeItemId rootId = m_treeView->GetRootItem();
	wxTreeItemIdValue cookie;
	wxTreeItemId rootNodeId = m_treeView->GetFirstChild( rootId, cookie );

	wxTreeItemId playedActionItem = FindTreeItemForObject( rootNodeId, action );
	if ( playedActionItem.IsOk() ) 
	{
		m_treeView->SetItemBackgroundColour( playedActionItem, wxColour( 255, 255, 0 ) );
	}
}

void CEdJobTreeEditor::OnActionPreviewEnded( const CJobActionBase* action )
{
	wxTreeItemId rootId = m_treeView->GetRootItem();
	wxTreeItemIdValue cookie;
	wxTreeItemId rootNodeId = m_treeView->GetFirstChild( rootId, cookie );

	wxTreeItemId playedActionItem = FindTreeItemForObject( rootNodeId, action );
	if ( playedActionItem.IsOk() ) 
	{
		m_treeView->SetItemBackgroundColour( playedActionItem, wxColour( 255, 255, 255 ) );
	}
}

void CEdJobTreeEditor::FillPreviewCategoriesChoice()
{
	if ( !m_rootNode )
	{
		return;
	}

	String previousSelection;
	if ( !m_previewCategoryChoice->GetStringSelection().IsEmpty() )
	{
		// Remember previous selection
		previousSelection = m_previewCategoryChoice->GetStringSelection().c_str();
	}

	TDynArray< CName > categoriesAvailable = m_rootNode->GetCategories();
	
	m_previewCategoryChoice->Clear();
	for ( Uint32 i=0; i<categoriesAvailable.Size(); ++i )
	{
		m_previewCategoryChoice->Append( categoriesAvailable[i].AsString().AsChar() );
	}

	if ( !previousSelection.Empty() )
	{
		// Try to restore previous selection
		if ( !m_previewCategoryChoice->SetStringSelection( previousSelection.AsChar() ) )
		{
			// Failed, previous option is no longer available, activate first available
			m_previewCategoryChoice->SetSelection( 0 );
		}
	}
	else
	{
		// No previous selection, activate first
		m_previewCategoryChoice->SetSelection( 0 );
	}
}

void CEdJobTreeEditor::OnAnimationSelected( wxTreeEvent& event )
{
	if ( m_previewingTree )
	{
		return;
	}
	wxTreeItemId selectedItem = event.GetItem();
	if ( m_previewPanel->GetActor() )
	{
		m_previewPanel->GetActor()->Teleport(Vector::ZEROS, EulerAngles::ZEROS);
	}
	if ( m_animationTree->HasChildren( selectedItem ) == false )
	{
		m_previewPanel->PlayAnimation( CName( m_animationTree->GetItemText( selectedItem ) ) );
	}
	else
	{
		m_previewPanel->Stop();
	}
	
}

void CEdJobTreeEditor::ActivateAnimation( CJobActionBase* action, const CName& animName )
{
	action->SetAnimName( animName );
	m_properties->RefreshValues();

	wxTreeItemId treeItem = FindTreeItemForObject( action );
	ASSERT ( treeItem != NULL );
	m_treeView->SetItemText( treeItem, animName.AsString().AsChar() );
}

void CEdJobTreeEditor::OnAnimationActivated( wxTreeEvent& event )
{
	if ( m_currentAction != NULL )
	{
		CUndoJobEditorAnimationActivated::CreateStep( *m_undoManager, this, m_currentAction );

		ActivateAnimation( m_currentAction, CName( m_animationTree->GetItemText( event.GetItem() ) ) );
	}
}


void CEdJobTreeEditor::OnAnimFilter( wxCommandEvent& event )
{
	m_filterMode = XRCCTRL( *this, "animFilterCheck", wxCheckBox )->IsChecked() ? BAFM_Contain : BAFM_BeginsWith;
	wxString str = event.GetString();
	m_filterText = str.wc_str();

	FillAnimationTree();
}

void CEdJobTreeEditor::OnAnimFilterCheck( wxCommandEvent& event )
{
	m_filterMode = event.IsChecked() ? BAFM_Contain : BAFM_BeginsWith;
}

void CEdJobTreeEditor::OnAllAnimsetsCheck( wxCommandEvent& event )
{
	m_allAnimsets = event.IsChecked() ? true : false;
	FillAnimationTree();
}

Bool CEdJobTreeEditor::FilterAnimName( const CName& animName ) const
{
	// Slow version for testing
	if ( m_filterMode == BAFM_BeginsWith )
	{
		// I know, i know 'AsString'
		return !animName.AsString().BeginsWith( m_filterText );
	}
	else
	{
		return !animName.AsString().ContainsSubstring( m_filterText );
	}
}

void CEdJobTreeEditor::FillAnimationTree()
{
	m_animationTree->DeleteAllItems();
	wxTreeItemId rootItem = m_animationTree->AddRoot( wxT( "Animations" ) );

	TSkeletalAnimationSetsArray animsets;

	if ( m_usePreviewEntityData == false )
	{
		const C2dArray& workAnimationsCSV = SAnimationsCategoriesResourcesManager::GetInstance().Get2dArray();
		if ( workAnimationsCSV.Empty() == false )
		{
			if ( m_allAnimsets ) //load all available animsets
			{
				TDynArray< String > animsetPaths = workAnimationsCSV.GetColumn< String >( TXT( "animsetDepotPath" ) );
				for( TDynArray< String >::const_iterator pathIter = animsetPaths.Begin(); pathIter != animsetPaths.End(); ++pathIter )
				{
					CSkeletalAnimationSet* loadedAnimset = LoadResource< CSkeletalAnimationSet >( *pathIter );
					if ( loadedAnimset != nullptr )
					{
						animsets.PushBack( loadedAnimset );
					}
					else
					{
						WARN_EDITOR( TXT( "Animations list contains missing file: %ls" ), pathIter->AsChar() );
					}
				}
			}
			else //load only those concerned in this particular job tree
			{
				TDynArray< String > friendlyNames = workAnimationsCSV.GetColumn< String >( TXT( "friendlyName" ) );
				TDynArray< String > animsetPaths = workAnimationsCSV.GetColumn< String >( TXT( "animsetDepotPath" ) );
				TDynArray< CName > categoriesAvailable = m_rootNode->GetCategories();

				for ( Uint32 i=0; i<categoriesAvailable.Size(); ++i )
				{
					String availableCatName = categoriesAvailable[i].AsString().AsChar();

					for( Uint32 j=0; j<friendlyNames.Size(); ++j )
					{
						//only get the anim sets for the categories concerned in this job tree
						if ( availableCatName == friendlyNames[j].AsChar() )
						{
							CSkeletalAnimationSet* loadedAnimset = LoadResource< CSkeletalAnimationSet >( animsetPaths[j] );
							if ( loadedAnimset != nullptr )
							{
								animsets.PushBack( loadedAnimset );
							}
							else
							{
								WARN_EDITOR( TXT( "Animations list contains missing file: %ls" ), animsetPaths[ j ].AsChar() );
							}
						}
					}
				}
			}
		}
		else
		{
			m_usePreviewEntityData = true;
		}
	}

	if ( m_usePreviewEntityData == true )
	{
		CEntity* entity = m_previewPanel->GetActor();
		if ( entity != NULL && entity->GetRootAnimatedComponent() != NULL )
		{
			animsets = entity->GetRootAnimatedComponent()->GetAnimationContainer()->GetAnimationSets();
		}
	}


	Bool useFilter = !m_filterText.Empty();


	for ( auto animsetIter = animsets.Begin(); animsetIter != animsets.End(); ++animsetIter )
	{
		const CSkeletalAnimationSet* animationSet = ( *animsetIter ).Get();
		wxTreeItemId animsetItemId = m_animationTree->AppendItem( rootItem, animationSet->GetDepotPath().AsChar() );

		const TDynArray< CSkeletalAnimationSetEntry* >& animationEntries = animationSet->GetAnimations();

		for ( TDynArray< CSkeletalAnimationSetEntry* >::const_iterator animationIter = animationEntries.Begin(); animationIter != animationEntries.End(); ++animationIter )
		{
			CSkeletalAnimationSetEntry* animationEntry = *animationIter;

			if ( useFilter && FilterAnimName( animationEntry->GetName() ) )
			{
				continue;
			}

			wxTreeItemId animationItemId = m_animationTree->AppendItem( animsetItemId, animationEntry->GetName().AsString().AsChar() );

			m_animationTree->SortChildren( animsetItemId );

			if ( useFilter )
			{
				m_animationTree->Expand( animsetItemId );
			}
		}
	}

	if ( useFilter )
	{
		m_animationTree->Expand( m_animationTree->GetRootItem() );
	}

	m_animationTree->SortChildren( m_animationTree->GetRootItem() );
	m_animationTree->Expand( rootItem );
	m_animationTree->EnsureVisible( rootItem );
}

void CEdJobTreeEditor::OnPreviewActorChanged()
{
	if ( m_previewPanel != NULL && m_usePreviewEntityData == true )
	{
		FillAnimationTree();
	}
}

void CEdJobTreeEditor::UpdateTreeFragment( wxTreeItemId treeItemId )
{
	m_treeView->DeleteChildren( treeItemId );
	FillNodeContents( treeItemId );
}
