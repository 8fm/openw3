#include "build.h"
#include "encounterEditor.h"

#include "../../common/core/diskFile.h"

#include "../../common/game/encounter.h"
#include "../../common/game/spawnTree.h"
#include "../../common/game/edSpawnTreeNode.h"

#include "gridEditor.h"
#include "gridCustomTypes.h"
#include "gridCustomColumns.h"
#include "undoManager.h"
#include "spawnTreeNodeProxy.h"


BEGIN_EVENT_TABLE( CEdEncounterEditor, wxSmartLayoutPanel )
	EVT_MENU( XRCID( "fileSave" ), CEdEncounterEditor::OnSave )
	EVT_MENU( XRCID( "editUndo" ), CEdEncounterEditor::OnUndo )
	EVT_MENU( XRCID( "editRedo" ), CEdEncounterEditor::OnRedo )
	EVT_MENU( XRCID( "editDelete" ), CEdEncounterEditor::OnDeleteSelection )
	EVT_MENU( XRCID( "viewAutoLayout" ), CEdEncounterEditor::OnAutoLayout )
	EVT_MENU( XRCID( "viewZoomExtents" ), CEdEncounterEditor::OnZoomExtents )
END_EVENT_TABLE()

enum
{
	ID_GRID_SPLITTER = wxID_HIGHEST,
	ID_ADD_CREATURE_DEF,
	ID_DEL_CREATURE_DEF
};

CEdEncounterEditor::CEdEncounterEditor( wxWindow* parent, CEncounter* encounter )
	: wxSmartLayoutPanel( parent, TXT("EncounterEditor"), false )
	, m_editedObject( encounter )
{
	GetOriginalFrame()->Bind( wxEVT_ACTIVATE, &CEdEncounterEditor::OnActivate, this );
	SetTitle( TXT("Encounter Editor") );
	CommonInit();
}

CEdEncounterEditor::CEdEncounterEditor( wxWindow* parent, CSpawnTree* spawnTree )
	: wxSmartLayoutPanel( parent, TXT("EncounterEditor"), false )
	, m_editedObject( spawnTree )
{
	GetOriginalFrame()->Bind( wxEVT_ACTIVATE, &CEdEncounterEditor::OnActivate, this );
	CDiskFile* file = spawnTree->GetFile();
	if ( file )
	{
		String title = String::Printf( TXT("Spawn Tree Editor - %s"), file->GetFileName().AsChar() );
		SetTitle( title.AsChar() );
	}
	else
	{
		SetTitle( TXT("Spawn Tree Editor ") );
	}
	CommonInit();
}

CEdEncounterEditor::~CEdEncounterEditor()
{
	SaveOptionsToConfig();
	SEvents::GetInstance().UnregisterListener( this );

	m_editedObject->RemoveFromRootSet();
	m_editedObject = NULL;

	{
		m_undoManager->RemoveFromRootSet();
		m_undoManager->Discard();
		m_undoManager = NULL;
	}
}

void CEdEncounterEditor::CommonInit()
{
	ASSERT ( m_hWnd );

	m_timeSinceLastDebugUpdate = 0;

	m_editedObject->AddToRootSet();

	// undo manager
	{
		m_undoManager = new CEdUndoManager( this );
		m_undoManager->AddToRootSet();
		m_undoManager->SetMenuItems( GetMenuBar()->FindItem( XRCID( "editUndo" ) ), GetMenuBar()->FindItem( XRCID( "editRedo" ) ) );
	}

	// Properties panel
	{
		wxPanel* propsPanel = XRCCTRL( *this, "propertiesPanel", wxPanel );
		wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
		propsPanel->SetSizer( sizer );

		PropertiesPageSettings settings;
		m_propsBrowser = new CEdPropertiesBrowserWithStatusbar( propsPanel, settings, m_undoManager );
		m_propsBrowser->Get().Bind( wxEVT_COMMAND_PROPERTY_CHANGED, &CEdEncounterEditor::OnPropertiesChanged, this );
		m_propsBrowser->Get().Bind( wxEVT_COMMAND_REFRESH_PROPERTIES, &CEdEncounterEditor::OnPropertiesRefresh, this );
		sizer->Add( m_propsBrowser, 1, wxEXPAND, 0 );
		propsPanel->Layout();
	}

	// Tree panel
	{
 		wxPanel* rightPanel = XRCCTRL( *this, "treePanel", wxPanel );
 		wxBoxSizer* rightPanelSizer = new wxBoxSizer( wxVERTICAL );
 		rightPanel->SetSizer( rightPanelSizer );

		// Creature definitions grid
 		if ( dynamic_cast< ICreatureDefinitionContainer* >( m_editedObject ) )
 		{
			wxSplitterWindow* splitter = new wxSplitterWindow( rightPanel, ID_GRID_SPLITTER, wxDefaultPosition, wxDefaultSize, wxSP_3D | wxSP_LIVE_UPDATE );
			rightPanelSizer->Add( splitter, 1, wxEXPAND, 0 );

			// ------------- tree

			wxPanel* treePanel = new wxPanel( splitter );
 			wxBoxSizer* treePanelSizer = new wxBoxSizer( wxVERTICAL );
			treePanel->SetSizer( treePanelSizer );

 			m_treeEditor = new CEdSpawnTreeEditor( treePanel, this );
			m_treeEditor->SetUndoManager( m_undoManager );
			treePanelSizer->Add( m_treeEditor, 1, wxEXPAND, 0 );

			// ------------- grid

 			wxPanel* gridPanel = new wxPanel( splitter );
 			wxBoxSizer* gridPanelSizer = new wxBoxSizer( wxVERTICAL );
			gridPanel->SetSizer( gridPanelSizer );

			m_creatureDefToolbar = new wxToolBar( gridPanel, wxID_ANY );
			gridPanelSizer->Add( m_creatureDefToolbar, 0, wxEXPAND, 1 );
			wxBitmap addBmp = SEdResources::GetInstance().LoadBitmap( TXT("IMG_PB_ADD") );
			wxBitmap delBmp = SEdResources::GetInstance().LoadBitmap( TXT("IMG_PB_DELETE") );
			m_creatureDefToolbar->AddTool( ID_ADD_CREATURE_DEF, TXT("Add Creature Definition"), addBmp, wxT("Add Creature Definition") );
			m_creatureDefToolbar->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEncounterEditor::OnAddCreatureDef, this, ID_ADD_CREATURE_DEF );
			m_creatureDefToolbar->AddTool( ID_DEL_CREATURE_DEF, TXT("Remove Creature Definition"), delBmp, wxT("Remove Creature Definition") );
			m_creatureDefToolbar->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEncounterEditor::OnRemoveCreatureDef, this, ID_DEL_CREATURE_DEF );
			m_creatureDefToolbar->Realize();

			m_creatureDefGrid = new CGridEditor( gridPanel );
			m_creatureDefGrid->Bind( wxEVT_GRID_VALUE_CHANGED, &CEdEncounterEditor::OnCreatureListGridValueChanged, this );
			m_creatureDefGrid->Bind( wxEVT_GRID_ELEMENTS_CHANGED, &CEdEncounterEditor::OnCreatureListGridElementsChanged, this );
			m_creatureDefGrid->Bind( wxEVT_GRID_SELECT_CELL, &CEdEncounterEditor::OnCreatureListGriSelectCell, this );
			m_creatureDefGrid->RegisterCustomType( new CGridTagListDesc );

			m_creatureDefGrid->SetUndoManager( m_undoManager );
			gridPanelSizer->Add( m_creatureDefGrid, 0, wxEXPAND, 1 );

			// ---------------

			splitter->SplitHorizontally( treePanel, gridPanel, 350 );

			UpdateGrid();
 		}
		else
		{
 			m_treeEditor = new CEdSpawnTreeEditor( rightPanel, this );
			m_treeEditor->SetUndoManager( m_undoManager );
 			rightPanelSizer->Add( m_treeEditor, 1, wxEXPAND, 0 );
		}

 		rightPanel->Layout();
	}

	ICreatureDefinitionContainer* editedObjAsNode = dynamic_cast< ICreatureDefinitionContainer* >( m_editedObject );
	ASSERT ( editedObjAsNode );
	m_treeEditor->SetTree( editedObjAsNode );

	SEvents::GetInstance().RegisterListener( CNAME( EditorTick ), this );
	SEvents::GetInstance().RegisterListener( CNAME( FileReloadConfirm ), this );
	SEvents::GetInstance().RegisterListener( CNAME( FileReload ), this );
	SEvents::GetInstance().RegisterListener( CNAME( VersionControlStatusChanged ), this );
	SEvents::GetInstance().RegisterListener( CNAME( SpawnTreeCreatureListChanged ), this );
	SEvents::GetInstance().RegisterListener( CNAME( GameStarted ), this );
	SEvents::GetInstance().RegisterListener( CNAME( GameEnded ), this );

	UpdateMenuState();
	UpdateToolbarState();
	UpdateProperties();

	LoadOptionsFromConfig();
}

void CEdEncounterEditor::OnPropertiesChanged( wxCommandEvent& event )
{
	UpdateTree( true );
}

void CEdEncounterEditor::OnPropertiesRefresh( wxCommandEvent& event )
{
	// This event is invoked after changing a property to refresh other properties that may depend on it.
	//UpdateProperties();
}

void CEdEncounterEditor::OnDeleteSelection( wxCommandEvent& event )
{
	m_treeEditor->DeleteSelection();
}

void CEdEncounterEditor::OnUndo( wxCommandEvent& event )
{
	m_undoManager->Undo();
}

void CEdEncounterEditor::OnRedo( wxCommandEvent& event )
{
	m_undoManager->Redo();
}

void CEdEncounterEditor::OnSave( wxCommandEvent& event )
{
	if ( CResource* res = Cast< CResource >( m_editedObject ) )
	{
		res->Save();
		SEvents::GetInstance().QueueEvent( CNAME( EditorSpawnTreeResourceModified ), CreateEventData( res ) );
	}
}

void CEdEncounterEditor::OnAutoLayout( wxCommandEvent& event )
{
	m_treeEditor->AutoLayout();
}

void CEdEncounterEditor::OnZoomExtents( wxCommandEvent& event )
{
	m_treeEditor->ZoomExtents();
}

void CEdEncounterEditor::OnShowStats( wxCommandEvent& event )
{
	m_treeEditor->OnShowStats( event );
}

void CEdEncounterEditor::UpdateTree( Bool lazy )
{
	if ( lazy )
	{
		RunLaterOnce( [ this ]() {
			m_treeEditor->ForceLayoutUpdate();
			m_treeEditor->Repaint();
		} );
	}
	else
	{
		m_treeEditor->ForceLayoutUpdate();
		m_treeEditor->Repaint();
	}
}

void CEdEncounterEditor::UpdateProperties()
{
	TDynArray< IScriptable* > selection = m_treeEditor->GetSelectedObjects();

	// if nothing is selected, show properties of the root node
 	if ( selection.Empty() )
 	{
		selection.PushBack( m_treeEditor->GetRootNodeProxy() );
 	}

	// translate selection
	TDynArray< IScriptable* > objectsToBeEdited;
	Bool readOnly = false;
	for ( Uint32 i = 0, n = selection.Size(); i != n; ++i )
	{
		if ( CEdSpawntreeNodeProxy* proxy = Cast< CEdSpawntreeNodeProxy >( selection[ i ] ) )
		{
			if ( m_treeEditor->IsLocked( *selection[ i ] ) )
			{
				readOnly = true;
			}

			objectsToBeEdited.PushBack( proxy->GetSpawnTreeNode()->GetObjectForPropertiesEdition() );
		}
	}

	m_propsBrowser->Get().SetObjects( objectsToBeEdited );
	m_propsBrowser->Get().SetReadOnly( readOnly );
}


void CEdEncounterEditor::SaveOptionsToConfig()
{
	SaveLayout( TXT("/Frames/EncounterEditor") );
	
	CUserConfigurationManager& config = SUserConfigurationManager::GetInstance();

	wxSplitterWindow* propsSplitter = XRCCTRL( *this, "propsSplitter", wxSplitterWindow );	
	config.Write( TXT("/Frames/EncounterEditor/Layout/PropsSplitter"), propsSplitter->GetSashPosition() );

	if ( wxSplitterWindow* gridSplitter = wxDynamicCast( FindWindow( ID_GRID_SPLITTER ), wxSplitterWindow ) )
	{
		config.Write( TXT("/Frames/EncounterEditor/Layout/GridSplitter"), gridSplitter->GetSashPosition() );
	}
}

void CEdEncounterEditor::LoadOptionsFromConfig()
{
	LoadLayout( TXT("/Frames/EncounterEditor") );

	CUserConfigurationManager& config = SUserConfigurationManager::GetInstance();

	wxSplitterWindow* propsSplitter = XRCCTRL( *this, "propsSplitter", wxSplitterWindow );
	propsSplitter->SetSashPosition( config.Read( TXT("/Frames/EncounterEditor/Layout/PropsSplitter"), propsSplitter->GetSashPosition() ) );
	
	if ( wxSplitterWindow* gridSplitter = wxDynamicCast( FindWindow( ID_GRID_SPLITTER ), wxSplitterWindow ) )
	{
		gridSplitter->SetSashPosition( config.Read( TXT("/Frames/EncounterEditor/Layout/GridSplitter"), gridSplitter->GetSashPosition() ) );
	}
}

/*virtual*/
void CEdEncounterEditor::DispatchEditorEvent( const CName& name, IEdEventData* data ) /*override*/
{
	if ( name == CNAME( EditorTick ) )
	{
		if ( m_treeEditor->IsDebugMode() )
		{
			m_timeSinceLastDebugUpdate += GetEventData< Float >( data );

			if ( m_timeSinceLastDebugUpdate >= 1.0 )
			{
				UpdateTree();
				m_timeSinceLastDebugUpdate = 0.0;
			}
		}
	}
	else if ( name == CNAME( FileReloadConfirm ) )
 	{
 		CResource* res = GetEventData< CResource* >( data );
 		if ( res == m_editedObject )
 		{
			// This is to include the resource in the 'files in editors' section on the Reload dialog
 			SEvents::GetInstance().QueueEvent( CNAME( FileReloadToConfirm ), CreateEventData( CReloadFileInfo( res, NULL, GetTitle().wc_str() ) ) );
 		}
 	}
	else if ( name == CNAME( VersionControlStatusChanged ) )
	{
		UpdateAll();
	}
 	else if ( name == CNAME( FileReload ) )
	{
		const CReloadFileInfo& reloadInfo = GetEventData< CReloadFileInfo >( data );

		if ( reloadInfo.m_newResource->IsA< CSpawnTree >() )
		{
			CSpawnTree* oldTree = SafeCast< CSpawnTree >( reloadInfo.m_oldResource );
			CSpawnTree* newTree = SafeCast< CSpawnTree >( reloadInfo.m_newResource );

			if ( oldTree == m_editedObject )
			{
				// The resource was re-created, so we must update the reference
				// Note: reloaded resource is removed from the root set, so it's not needed to do this here
				m_editedObject = newTree;
				m_editedObject->AddToRootSet();
				ICreatureDefinitionContainer* editedObjAsNode = dynamic_cast< ICreatureDefinitionContainer* >( m_editedObject );
				ASSERT ( editedObjAsNode );
				m_treeEditor->SetTree( editedObjAsNode );
				UpdateAll();

				m_undoManager->ClearHistory();

				OnGraphSelectionChanged();
				wxTheFrame->GetAssetBrowser()->OnEditorReload( newTree, this );
			}
		}
	}
	else if ( name == CNAME( SpawnTreeCreatureListChanged ) )
	{
		if ( GetEventData< CObject* >( data ) == m_editedObject )
		{
			UpdateProperties();
			UpdateGrid();
		}
	}
	else if ( name == CNAME( GameStarted ) || name == CNAME( GameEnded ) )
	{
		UpdateAll();
	}
}

void CEdEncounterEditor::UpdateMenuState()
{
	Bool thereIsSelection = !m_treeEditor->GetSelectedObjects().Empty();
	GetMenuBar()->FindItem( XRCID( "editDelete" ) )->Enable( thereIsSelection );

	Bool resourceIsEdited = m_editedObject->IsA< CResource >();
	GetMenuBar()->FindItem( XRCID( "fileSave" ) )->Enable( resourceIsEdited );
}

/*virtual*/ 
void CEdEncounterEditor::OnGraphSelectionChanged() /*override*/
{
	UpdateProperties();
	UpdateMenuState();
}

void CEdEncounterEditor::UpdateToolbarState()
{
	Int32 curRow = m_creatureDefGrid->GetGridCursorRow();
	m_creatureDefToolbar->EnableTool( ID_DEL_CREATURE_DEF, curRow != -1 );
	m_creatureDefToolbar->Enable( !IsLocked() );

}
Bool CEdEncounterEditor::IsLocked()const
{
	if ( GGame->IsActive() )
	{
		return true;
	}
	if ( CResource* res = Cast< CResource >( m_editedObject ) )
	{
		if ( !res->CanModify() )
		{
			return true;
		}
	}
	return false;
}
void CEdEncounterEditor::UpdateGrid()
{
	if ( ICreatureDefinitionContainer* creatureDefCont = dynamic_cast< ICreatureDefinitionContainer* >( m_editedObject ) )
	{
		TDynArray< CEncounterCreatureDefinition* > creatureDefs;
		creatureDefCont->GetCreatureDefinitions( creatureDefs );
		m_creatureDefObjs = CastArray< CObject >( creatureDefs );
		m_creatureDefGrid->SetObjects( m_creatureDefObjs, false );
		m_creatureDefGrid->Enable( !IsLocked() );
		m_creatureDefGrid->GetParent()->GetSizer()->Layout();
	}
}

void CEdEncounterEditor::OnCreatureListGridValueChanged( wxCommandEvent& event )
{
	m_propsBrowser->Get().RefreshValues();
	m_editedObject->MarkModified();
}

void CEdEncounterEditor::OnCreatureListGridElementsChanged( wxCommandEvent& event )
{
	m_propsBrowser->Get().RefreshValues();
	m_editedObject->MarkModified();
}

void CEdEncounterEditor::OnCreatureListGriSelectCell( wxGridEvent& event )
{
	UpdateToolbarState();
}

void CEdEncounterEditor::UpdateAll()
{
	UpdateTree();
	UpdateProperties();
	UpdateGrid();
	UpdateMenuState();
	UpdateToolbarState();
}

void CEdEncounterEditor::OnAddCreatureDef( wxCommandEvent& event )
{
	ICreatureDefinitionContainer* creatureDefCont = dynamic_cast< ICreatureDefinitionContainer* >( m_editedObject );
	ASSERT( creatureDefCont );

	creatureDefCont->AddCreatureDefinition();

	UpdateGrid();

	m_editedObject->MarkModified();
}

void CEdEncounterEditor::OnRemoveCreatureDef( wxCommandEvent& event )
{
	Int32 curRow = m_creatureDefGrid->GetGridCursorRow();
	if ( curRow == -1 )
	{
		return;
	}

	ASSERT( curRow < static_cast< Int32 >( m_creatureDefObjs.Size() ) );

	ICreatureDefinitionContainer* creatureDefCont = dynamic_cast< ICreatureDefinitionContainer* >( m_editedObject );
	ASSERT( creatureDefCont );

	CEncounterCreatureDefinition* creatureDef = SafeCast< CEncounterCreatureDefinition >( m_creatureDefObjs[ curRow ] );
	creatureDefCont->RemoveCreatureDefinition( creatureDef );

	UpdateGrid();

	m_editedObject->MarkModified();
}

void CEdEncounterEditor::OnActivate( wxActivateEvent& event )
{
	if ( event.GetActive() )
	{
		if ( wxTheFrame )
		{
			wxTheFrame->SetUndoHistoryFrameManager( m_undoManager, String( GetTitle().c_str() ) );
		}
	}

	event.Skip();
}
