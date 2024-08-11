#include "build.h"

#include "../../common/game/storySceneSystem.h"
#include "../../common/game/gameWorld.h"
#include "../../common/game/quest.h"
#include "../../common/game/questGraph.h"
#include "../../common/game/questGraphBlock.h"
#include "../../common/game/questsSystem.h"
#include "../../common/engine/commentGraphBlock.h"
#include "../../common/engine/descriptionGraphBlock.h"
#include "questGraphEditor.h"
#include "questEditor.h"
#include "questEdTool.h"
#include "questDebugInfo.h"
#include "../../common/game/questDependencyInfo.h"
#include "../../common/game/questScopeBlock.h"
#include "../../common/game/questSceneBlock.h"
#include "../../common/game/questPhaseInputBlock.h"

namespace // anonymous
{
	class NullQuestDebugInfo : public IQuestDebugInfo
	{
	public:
		Bool IsBreakpointToggled( CQuestGraphBlock* block ) const { return false; }
		Bool IsBlockActive( CQuestGraphBlock* block ) const { return false; }
		Bool IsBlockVisited( CQuestGraphBlock* block ) const { return false; }
		Bool IsGraphInActive( CQuestGraph* graph ) const { return false; }
	};

	void CookSceneData( CQuestGraph* qg )
	{
		const TDynArray< CGraphBlock* >& blocks = qg->GraphGetBlocks();

		const Uint32 count = blocks.Size();
		for ( Uint32 i=0; i<count; ++i )
		{
			if ( CQuestGraphBlock* questBlock = Cast< CQuestGraphBlock >( blocks[ i ] ) )
			{
				CQuestScopeBlock* scopeBlock = Cast< CQuestScopeBlock >( questBlock );
				if ( scopeBlock && scopeBlock->GetGraph() )
				{
					CookSceneData( scopeBlock->GetGraph() );
				}

				if ( CQuestSceneBlock* sceneBlock = Cast< CQuestSceneBlock >( questBlock ) )
				{
					sceneBlock->CookSceneData();
				}
			}
		}
	}
}

void CQuestEditorClipboard::Copy( CEdGraphEditor* editor )
{
	CEdQuestGraphEditor* questGraphEditor = wxDynamicCast( editor, CEdQuestGraphEditor );
	if ( !questGraphEditor )
	{
		wxMessageBox( wxT("Failed to copy the quest view to the clipboard"), wxT("Error"), wxICON_ERROR|wxCENTRE|wxOK, wxTheFrame );
		return;
	}

	String filename;
	String guids;

	const TDynArray< CQuestGraph* >& phasesStack = questGraphEditor->GetPhasesStack();
	for ( Uint32 i = 0; i < phasesStack.Size(); ++i )
	{
		const CObject* parent = phasesStack[ i ]->GetParent();
		if ( !parent )
		{
			continue;
		}

		if ( i == 0 )
		{
			const CQuestPhase* questPhase = Cast< CQuestPhase >( parent );
			if ( questPhase && questPhase->GetFile() )
			{
				filename = questPhase->GetFile()->GetDepotPath();
			}
			else
			{
				wxMessageBox( wxT("Failed to copy the quest view to the clipboard"), wxT("Error"), wxICON_ERROR|wxCENTRE|wxOK, wxTheFrame );
				return;
			}
		}
		else
		{
			const CQuestPhase* questPhase = Cast< CQuestPhase >( parent );
			if ( questPhase )
			{
				// if it's embedded phase, look for a scope block with that phase in previous graph in the stack
				parent = phasesStack[ i - 1 ]->GetScopeBlockWithEmbeddedPhase( questPhase );
			}

			const CQuestScopeBlock* scopeBlock = Cast< CQuestScopeBlock >( parent );
			if ( scopeBlock )
			{
				const CGUID& guid = scopeBlock->GetGUID();
				if ( !guids.Empty() )
				{
					guids += TXT(" ");
				}
				guids += ToString( guid );
			}
		}
	}

	if ( wxTheClipboard->Open() )
	{
		String viewText = TXT("[[") + filename + TXT("|") + guids + TXT("|") + ToString( questGraphEditor->GetScale() ) + TXT("|") + ToString( questGraphEditor->GetOffset().x ) + TXT(" ") + ToString( questGraphEditor->GetOffset().y ) + TXT("]]");

		wxTheClipboard->SetData( new wxTextDataObject( viewText.AsChar() ) );
		wxTheClipboard->Close();
		wxTheClipboard->Flush();
	}
	else
	{
		wxMessageBox( wxT("Failed to copy the quest view to the clipboard"), wxT("Error"), wxICON_ERROR|wxCENTRE|wxOK, wxTheFrame );
	}
}

void CQuestEditorClipboard::Paste( CEdGraphEditor* editor )
{
	RED_UNUSED( editor );

	if ( wxTheClipboard->Open() )
	{
		wxTextDataObject data;
		if ( wxTheClipboard->GetData( data ) )
		{
			String filename;
			TDynArray< CGUID > blockGuids;
			Float scale;
			Float offsetX, offsetY;

			if ( ParseQuestViewString( String( data.GetText().c_str() ), filename, blockGuids, scale, offsetX, offsetY ) )
			{
				if ( OpenQuestEditor( filename, blockGuids, scale, offsetX, offsetY ) )
				{
					wxTheClipboard->Close();
					return;
				}
			}
		}
		wxTheClipboard->Close();
	}
	wxMessageBox( wxT("Failed to paste a quest view from the clipboard"), wxT("Error"), wxICON_ERROR|wxCENTRE|wxOK, wxTheFrame );
}

Bool CQuestEditorClipboard::ParseQuestViewString( const String& viewString, String& filename, TDynArray< CGUID >& blockGuids, Float& scale, Float& offsetX, Float& offsetY ) const
{
	String viewText = viewString;
	viewText.Trim();
	if ( viewText.BeginsWith( TXT("[[") ) && viewText.EndsWith( TXT("]]") ) )
	{
		viewText = viewText.MidString( 2, viewText.GetLength() - 4 );
		size_t chloc;
		if ( viewText.FindCharacter( TXT('|'), chloc ) )
		{
			filename = viewText.LeftString( chloc );
			if ( filename.Empty() )
			{
				return false;
			}

			viewText = viewText.MidString( chloc + 1 );
			if ( viewText.FindCharacter( TXT('|'), chloc ) )
			{
				String guidsString = viewText.LeftString( chloc );
				viewText = viewText.MidString( chloc + 1 );

				// parse guids
				blockGuids.ClearFast();
				if ( !guidsString.Empty() )
				{
					CGUID guid;
					while( guidsString.FindCharacter( TXT(' '), chloc ) )
					{
						String singleGuid = guidsString.LeftString( chloc );
						if ( guid.FromString( singleGuid.AsChar() ) )
						{
							blockGuids.PushBack( guid );
						}
						guidsString = guidsString.MidString( chloc + 1 );
					}
					// last one
					if ( !guidsString.Empty() )
					{
						if ( guid.FromString( guidsString.AsChar() ) )
						{
							blockGuids.PushBack( guid );
						}
					}
				}

				if ( viewText.FindCharacter( TXT('|'), chloc ) )
				{
					String scaleString = viewText.LeftString( chloc );
					if ( !FromString( scaleString, scale ) )
					{
						return false;
					}

					viewText = viewText.MidString( chloc + 1 );
					if ( viewText.FindCharacter( TXT(' '), chloc ) )
					{
						String offestXString = viewText.LeftString( chloc );
						String offestYString = viewText.MidString( chloc + 1 );
						if ( !FromString( offestXString, offsetX ) || !FromString( offestYString, offsetY ) )
						{
							return false;
						}

						return true;
					}
				}
			}
		}
	}
	return false;
}

Bool CQuestEditorClipboard::OpenQuestEditor( const String& filename, const TDynArray< CGUID >& blockGuids, Float scale, Float offsetX, Float offsetY )
{
	CQuestPhase* phaseResource = Cast< CQuestPhase >( GDepot->LoadResource( filename ) );
	if ( !phaseResource )
	{
		return false;
	}

	wxWindow* editor = wxTheFrame->GetAssetBrowser()->EditAsset( phaseResource );
	if ( !editor )
	{
		return false;
	}

	CEdQuestEditor* questEditor = wxDynamicCast( editor, CEdQuestEditor );
	if ( !questEditor )
	{
		return false;
	}
				
	questEditor->SetQuestPhase( phaseResource );
	for ( Uint32 i = 0; i < blockGuids.Size(); ++i )
	{
		if ( !questEditor->OpenBlock( blockGuids[ i ], true ) )
		{
			return false;
		}
	}

	questEditor->SetGraphEditorScaleAndOffset( scale, wxPoint( offsetX, offsetY ) );
	questEditor->ScheduleSetFocus();

	return true;
}

wxIMPLEMENT_CLASS( CEdQuestEditor, wxFrame );

BEGIN_EVENT_TABLE( CEdQuestEditor, wxFrame )
	EVT_MENU( XRCID( "menuItemSave" ), CEdQuestEditor::OnSave )
	EVT_MENU( XRCID( "menuItemSaveAll" ), CEdQuestEditor::OnSaveAll )
	EVT_MENU( XRCID( "menuItemExit" ), CEdQuestEditor::OnExit )
	EVT_MENU( XRCID( "menuItemUndo" ), CEdQuestEditor::OnEditUndo )
	EVT_MENU( XRCID( "menuItemRedo" ), CEdQuestEditor::OnEditRedo )
	EVT_MENU( XRCID( "menuItemCut" ), CEdQuestEditor::OnEditCut )
	EVT_MENU( XRCID( "menuItemCopy" ), CEdQuestEditor::OnEditCopy )
	EVT_MENU( XRCID( "menuItemPaste" ), CEdQuestEditor::OnEditPaste )
	EVT_MENU( XRCID( "menuItemFind" ), CEdQuestEditor::OnFind )
	EVT_MENU( XRCID( "menuItemCopyQuestView" ), CEdQuestEditor::OnCopyQuestView )
	EVT_MENU( XRCID( "menuItemPasteQuestView" ), CEdQuestEditor::OnPasteQuestView )
	EVT_MENU( XRCID( "menuItemUpdateGUIDs" ), CEdQuestEditor::OnUpdateGUIDs )
	EVT_MENU( XRCID( "menuItemUpdateCommunityGUIDS" ), CEdQuestEditor::OnUpdateCommunityGUIDs )
	EVT_MENU( XRCID( "menuItemUpgrade" ), CEdQuestEditor::OnUpgrade )
	EVT_MENU( XRCID( "menuItemCleanSourceData" ), CEdQuestEditor::OnCleanSourceData )
	EVT_MENU( XRCID( "menuItemSaveDependencyInfo" ), CEdQuestEditor::OnSaveDependencyInfo )
	EVT_MENU( XRCID( "menuItemCookSceneData" ), CEdQuestEditor::OnCookSceneData )
END_EVENT_TABLE()

CQuestEditorClipboard	CEdQuestEditor::m_questClipboard;

CEdQuestEditor::CEdQuestEditor( wxWindow* parent, CQuestPhase* questPhase, const TDynArray< IQuestEdTool* >& tools )
	: m_questPhase( nullptr )
	, m_defaultDebugInfo( new NullQuestDebugInfo() )
	, m_debugInfo( m_defaultDebugInfo )
	, m_toolsNotebook( NULL )
	, m_setFocus( false )
{
	wxXmlResource::Get()->LoadFrame( this, parent, wxT("QuestEditor") );

	// Get access to main editor widgets
	m_mainSplitter = XRCCTRL( *this, "MainSplitter", wxSplitterWindow );
	m_topPanel = XRCCTRL( *this, "topPanel", wxPanel );

	// Create properties browser and quest graph editor and split them
	PropertiesPageSettings settings;
	wxPanel* propertiesPanel = XRCCTRL( *this, "PropertiesPanel", wxPanel );

	m_propertiesBrowser = new CEdPropertiesPage( propertiesPanel, settings, nullptr );
	m_propertiesBrowser->Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdQuestEditor::OnPropertiesChanged ), NULL, this );
	propertiesPanel->GetSizer()->Add( m_propertiesBrowser, 1, wxEXPAND | wxALL );


	wxPanel *graphPanel = XRCCTRL( *this, "GraphPanel", wxPanel );
	m_questGraphEditor = new CEdQuestGraphEditor( graphPanel, this );
	graphPanel->GetSizer()->Add( m_questGraphEditor, 1, wxEXPAND | wxALL );
	m_questGraphEditor->SetHook( this );

	// initialize toolbar
	wxToolBar* toolBar = XRCCTRL( *this, "toolBar", wxToolBar );
	toolBar->Connect( XRCID( "toolFlowSequence" ), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( CEdQuestEditor::OnFlowSequence ), NULL, this );
	toolBar->Connect( XRCID( "toolFind" ), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( CEdQuestEditor::OnFind ), NULL, this );
	toolBar->Connect( XRCID( "toolClean" ), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( CEdQuestEditor::OnCleanUnusedBlocks ), NULL, this );

	// create a notebook to which we'll be adding the tools if one does not exist
	m_tools = tools;
	if ( !m_tools.Empty() )
	{
		wxPanel* toolsPanel = new wxPanel( m_mainSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
		wxBoxSizer* toolsPanelSizer = new wxBoxSizer( wxVERTICAL );

		m_toolsNotebook = new wxNotebook( toolsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
		toolsPanelSizer->Add( m_toolsNotebook, 1, wxEXPAND | wxALL, 5 );

		toolsPanel->SetSizer( toolsPanelSizer );
		toolsPanel->Layout();

		toolsPanelSizer->Fit( toolsPanel );
		m_mainSplitter->SplitHorizontally( m_topPanel, toolsPanel, 0 );

		// assign icons to the notebook
		wxImageList* images = new wxImageList( 14, 14, true, 2 );
		images->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_CHECKED_OUT") ) );
		images->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_MARKED_DELETE") ) );
		m_toolsNotebook->AssignImageList( images );

		Uint32 toolsCount = m_tools.Size();
		for ( Uint32 i = 0; i < toolsCount; ++i )
		{
			IQuestEdTool* tool = m_tools[ i ];
			ASSERT( tool, TXT( "Invalid quest tool specified" ) );

			tool->OnAttach( *this, m_toolsNotebook );
			m_toolsNotebook->AddPage( tool->GetPanel(), tool->GetToolName().AsChar(), false );
			m_toolsNotebook->SetPageImage( i, QDT_IMG_OK );
		}

		m_toolsNotebook->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( CEdQuestEditor::OnToolsPageChanged ), NULL, this );
	}	

	{
		// Create undo manager
		m_undoManager = new CEdUndoManager( m_questGraphEditor );
		m_undoManager->AddToRootSet();
		m_propertiesBrowser->SetUndoManager( m_undoManager );
		m_undoManager->SetMenuItems( GetMenuBar()->FindItem( XRCID( "menuItemUndo" ) ), GetMenuBar()->FindItem( XRCID( "menuItemRedo" ) ) );

		m_questGraphEditor->SetUndoManager( m_undoManager );
	}
	SetTitle( wxT( "Quest Editor" ) );
	// set quest phase can override title
	SetQuestPhase( questPhase );

	RepairDuplicateInputs();
	CheckForDuplicateSockets();

	Layout();
	LoadOptionsFromConfig();

	SEvents::GetInstance().RegisterListener( CNAME( EditorTick ), this );
	SEvents::GetInstance().RegisterListener( CNAME( EditorPropertyPostChange ), this );
}

CEdQuestEditor::~CEdQuestEditor()
{
	SEvents::GetInstance().UnregisterListener( this );

	SaveOptionsToConfig();

	if ( m_questPhase )
	{
		m_questPhase->RemoveFromRootSet();
		m_questPhase = NULL;
	}

	{
		m_undoManager->RemoveFromRootSet();
		m_undoManager->Discard();
		m_undoManager = NULL;
	}

	for ( TDynArray< IQuestEdTool* >::iterator it =	m_tools.Begin();
		it != m_tools.End(); ++it )
	{
		(*it)->OnDetach();
	}
	m_tools.Clear();

	delete m_defaultDebugInfo;
	m_defaultDebugInfo = NULL;
}

void CEdQuestEditor::RepairDuplicateInputs()
{
	if ( !m_questGraphEditor || m_questGraphEditor->GetPhasesStack().Empty() )
	{
		return;
	}

	CQuestGraph* topGraph = m_questGraphEditor->GetPhasesStack().Back();
	TDynArray< CQuestPhaseIOBlock* > changed;
	topGraph->RepairDuplicateInputs( changed );

	if ( changed.Size() > 0 )
	{
		String socketIDs;
		for ( CQuestPhaseIOBlock* block : changed )
		{
			socketIDs += TXT("name: ") + block->GetBlockName() + TXT(", socketID: ") + block->GetSocketID().AsString() + TXT("\n");
		}
		GFeedback->ShowMsg( TXT("SocketIDs have been changed."), TXT("Following socketIDs have been changed to remain unique:\n%s"), socketIDs.AsChar() );
	}
}

void CEdQuestEditor::CheckForDuplicateSockets() const
{
	if ( !( m_questPhase && m_questPhase->GetGraph() ) )
	{
		return;
	}

	for ( CGraphBlock* block : m_questPhase->GetGraph()->GraphGetBlocks() )
	{
		THashSet< CName > usedNames;
		for( CGraphSocket* socket : block->GetSockets() )
		{
			if ( usedNames.Find( socket->GetName() ) != usedNames.End() )
			{
				continue;
			}
			for ( CGraphSocket* otherSocket : block->GetSockets() )
			{
				if ( otherSocket == socket )
				{
					continue;
				}
				if ( otherSocket->GetName() == socket->GetName() )
				{
					GFeedback->ShowError( TXT("GraphBlock %s has duplicate sockets with name %s"), block->GetBlockName().AsChar(), socket->GetName().AsChar() );
					usedNames.Insert( socket->GetName() );
				}
			}
		}
	}
}

void CEdQuestEditor::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( EditorPropertyPostChange ) )
	{
		const CEdPropertiesPage::SPropertyEventData& propertyData = GetEventData< CEdPropertiesPage::SPropertyEventData >( data );
		if ( propertyData.m_page == m_propertiesBrowser && propertyData.m_typedObject.m_class == ClassID< CQuestGraph >() && propertyData.m_propertyName.AsString() == TXT( "isTest" ) )
		{
			ShowTestQuestWarning();
		}
	}
	else if ( name == CNAME( EditorTick ) )
	{
		if ( m_setFocus )
		{
			m_setFocus = false;
			SetFocus();
		}
	}
}

void CEdQuestEditor::OnSaveDependencyInfo( wxCommandEvent& event )
{
	m_questGraphEditor->CreateDepFromGraph();
}

void CEdQuestEditor::OnCookSceneData( wxCommandEvent& event )
{
	if ( CQuestGraph* qg = m_questPhase->GetGraph() )
	{
		CookSceneData( qg );
	}
}

void CEdQuestEditor::SetToolErrorIndicator( const IQuestEdTool& tool )
{
	ASSERT( m_toolsNotebook, TXT( "This method can't be called with no tools available" ) );
	if ( !m_toolsNotebook )
	{
		return;
	}

	// find the tool's index
	Uint32 toolsCount = m_tools.Size();
	for ( Uint32 i = 0; i < toolsCount; ++i )
	{
		if ( &tool == m_tools[ i ] && i != m_toolsNotebook->GetSelection() )
		{
			m_toolsNotebook->SetPageImage( i, QDT_IMG_ERROR );
			break;
		}
	}
}

void CEdQuestEditor::OnToolsPageChanged( wxNotebookEvent& event )
{
	if ( m_toolsNotebook )
	{
		m_toolsNotebook->SetPageImage( event.GetSelection(), QDT_IMG_OK );
	}

	event.Skip();
}

void CEdQuestEditor::OnCreateBlockContextMenu( TDynArray< SToolMenu >& subMenus, const CQuestGraphBlock* atBlock )
{
	Uint32 count = m_tools.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		m_tools[ i ]->OnCreateBlockContextMenu( subMenus, atBlock );
	}
}

void CEdQuestEditor::SaveOptionsToConfig()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

	wxSplitterWindow* mainSplitter = XRCCTRL( *this, "MainSplitter", wxSplitterWindow );
	wxSplitterWindow* topPanelSplitter = XRCCTRL( *this, "TopPanelSplitter", wxSplitterWindow );

	config.Write( TXT("/Frames/QuestEditor/MainSplitterPosition"), mainSplitter->GetSashPosition() );
	config.Write( TXT("/Frames/QuestEditor/TopPanelSplitterPosition"), topPanelSplitter->GetSashPosition() );
}

void CEdQuestEditor::LoadOptionsFromConfig()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

	wxSplitterWindow* mainSplitter = XRCCTRL( *this, "MainSplitter", wxSplitterWindow );
	wxSplitterWindow* topPanelSplitter = XRCCTRL( *this, "TopPanelSplitter", wxSplitterWindow );

	int pos = config.Read( TXT("/Frames/QuestEditor/MainSplitterPosition"), mainSplitter->GetSashPosition() );
	mainSplitter->SetSashPosition( pos );

	pos = config.Read( TXT("/Frames/QuestEditor/TopPanelSplitterPosition"), topPanelSplitter->GetSashPosition() );
	topPanelSplitter->SetSashPosition( pos );
}

void CEdQuestEditor::Repaint()
{
	m_questGraphEditor->Repaint( true );
}

void CEdQuestEditor::SetQuestPhase( CQuestPhase *questPhase )
{
	if ( m_questPhase )
	{
		m_questPhase->RemoveFromRootSet();
		m_propertiesBrowser->SetNoObject();
		m_questGraphEditor->SetQuestGraph( NULL );
	}

	m_questPhase = questPhase;

	if ( m_questPhase )
	{
		m_questPhase->AddToRootSet();

		m_questGraphEditor->SetQuestGraph( m_questPhase->GetGraph() );
		m_propertiesBrowser->SetObject( m_questPhase );

		wxString titleString = m_questPhase->GetFile()->GetFileName().AsChar();
		titleString += wxT( " - Quest Editor" );
		SetTitle( titleString );
	}
	else
	{
		SetTitle( wxT( "Quest Editor" ) );
	}
}

CQuestsSystem* CEdQuestEditor::GetQuestsSystem()
{
	return GCommonGame->GetSystem< CQuestsSystem >();
}

void CEdQuestEditor::OnGraphSelectionChanged()
{
	if ( !m_questPhase )
	{
		return;
	}

	// Grab objects
	TDynArray< CGraphBlock* > blocks;
	m_questGraphEditor->GetSelectedBlocks( blocks );

	// Select them at properties browser
	if ( blocks.Size() )
	{
		// Show blocks properties
		m_propertiesBrowser->SetObjects( CastArray< CObject >( blocks ) );
	}
	else
	{
		// Show graph properties
		m_propertiesBrowser->SetObject( m_questPhase->GetGraph() );
	}
}

void CEdQuestEditor::OnGraphStructureModified( IGraphContainer* graph )
{
	if ( !m_questPhase )
	{
		return;
	}

	// Update GUIDs
	m_questGraphEditor->CheckAndUpdateGUIDs( false );

	CResource* questPhase = static_cast<CResource*>( m_questGraphEditor->GetCurrentPhaseRoot() );
	if ( !questPhase )
	{
		questPhase = m_questPhase;
	}
	questPhase->MarkModified();
}

void CEdQuestEditor::OnPropertiesChanged( wxCommandEvent& event )
{
	if ( !m_questPhase )
	{
		return;
	}

	CResource* questPhase = static_cast<CResource*>(m_questGraphEditor->GetCurrentPhaseRoot());
	if ( !questPhase )
	{
		questPhase = m_questPhase;
	}
	questPhase->MarkModified();
	Repaint();
}

void CEdQuestEditor::OnSave( wxCommandEvent& event )
{
	if ( m_questPhase )
	{
		// Update GUIDs
		m_questGraphEditor->CheckAndUpdateGUIDs( true );

		// Save crap
		if ( CResource* questPhase = static_cast<CResource*>(m_questGraphEditor->GetCurrentPhaseRoot()) )
		{
			questPhase->Save();
		}
		else
		{
			m_questPhase->Save();
		}
	}
}

void CEdQuestEditor::OnSaveAll( wxCommandEvent& event )
{
	if ( m_questPhase )
	{
		// Update GUIDs
		m_questGraphEditor->CheckAndUpdateGUIDs( true );

		// Get all included phasses
		TDynArray<CQuestPhase *> questPhases;
		questPhases.PushBack( m_questPhase );
		m_questGraphEditor->GetAllPhases( m_questPhase->GetGraph(), questPhases );
		ASSERT( questPhases.Exist( m_questPhase ) );
		
		// Save them all
		for ( Uint32 i = 0; i < questPhases.Size(); ++i )
		{
			questPhases[i]->Save();
		}
	}
}

void CEdQuestEditor::OnExit( wxCommandEvent& event )
{
	Hide();
}

void CEdQuestEditor::OnEditUndo( wxCommandEvent& event )
{
	m_undoManager->Undo();
}

void CEdQuestEditor::OnEditRedo( wxCommandEvent& event )
{
	m_undoManager->Redo();
}

Bool CEdQuestEditor::OpenBlock( const CGUID& blockGuid, Bool enterPhase )
{
	return m_questGraphEditor->OpenBlock( blockGuid, enterPhase );
}

void CEdQuestEditor::OpenBlock( CQuestGraphBlock* block, Bool enterPhase )
{
	m_questGraphEditor->OpenBlock( block, enterPhase );
}

void CEdQuestEditor::OnEditCopy( wxCommandEvent& event )
{
	m_questGraphEditor->CopySelection();
}

void CEdQuestEditor::OnEditPaste( wxCommandEvent& event )
{
	m_questGraphEditor->Paste( NULL );
}

void CEdQuestEditor::OnEditCut( wxCommandEvent& event )
{
	m_questGraphEditor->CutSelection();
}

void CEdQuestEditor::OnFlowSequence( wxCommandEvent& event )
{
	m_questGraphEditor->ToggleFlowSequence();
}

void CEdQuestEditor::OnFind( wxCommandEvent& event )
{
	m_questGraphEditor->FindInGraph();
}

void CEdQuestEditor::OnCopyQuestView( wxCommandEvent& event )
{
	GetQuestEditorClipboard().Copy( m_questGraphEditor );
}

void CEdQuestEditor::OnPasteQuestView( wxCommandEvent& event )
{
	GetQuestEditorClipboard().Paste( nullptr );
}

void CEdQuestEditor::OnCleanUnusedBlocks( wxCommandEvent& event )
{
	m_questGraphEditor->DeleteUnusedBlocks();
}

void CEdQuestEditor::OnUpdateGUIDs( wxCommandEvent& event )
{
	m_questGraphEditor->UpdateGUIDs();
}

void CEdQuestEditor::OnUpdateCommunityGUIDs( wxCommandEvent& event )
{
	m_questGraphEditor->OnUpdateCommunityGUIDs();
}

void CEdQuestEditor::OnUpgrade( wxCommandEvent& event )
{
	m_questGraphEditor->OnUpgrade();
}

void CEdQuestEditor::OnCleanSourceData( wxCommandEvent& event )
{
	m_questPhase->CleanupSourceData();
}

void CEdQuestEditor::OnGraphSet( CQuestGraph& graph )
{
	for ( TDynArray< IQuestEdTool* >::iterator it = m_tools.Begin();
		it != m_tools.End(); ++it )
	{
		(*it)->OnGraphSet( graph );
	}

	m_questGraphEditor->DeselectAllBlocks();
}

void CEdQuestEditor::GetEditableBlockClasses( const CQuestGraph* parentGraph, TDynArray< CClass* >& outClasses )
{
	struct Pred
	{
		mutable String str1;
		mutable String str2;
		Bool operator()( CClass* a, CClass* b ) const
		{	
			str1 = a->GetDefaultObject< CGraphBlock >()->GetBlockName();
			str2 = b->GetDefaultObject< CGraphBlock >()->GetBlockName();
			return str1 < str2;
		}

	} pred;

	static TDynArray< CClass* >	blockClasses;
	blockClasses.ClearFast();
	SRTTI::GetInstance().EnumClasses( ClassID< CQuestGraphBlock >(), blockClasses );
	blockClasses.Remove( ClassID< CQuestGraphBlock >() );

	THashMap< String, wxMenu* >	groups;
	Uint32 blockIdx = 0;
	for ( TDynArray< CClass* >::const_iterator it = blockClasses.Begin(); it != blockClasses.End(); ++it )
	{
		CQuestGraphBlock* defaultBlock = (*it)->GetDefaultObject< CQuestGraphBlock >();

		if ( defaultBlock->CanBeAddedToGraph( parentGraph ) )
		{
			outClasses.PushBack( *it );
		}
	}

	outClasses.PushBack( ClassID< CDescriptionGraphBlock >() );
	outClasses.PushBack( ClassID< CCommentGraphBlock >() );

	Sort( outClasses.Begin(), outClasses.End(), pred );
}

void CEdQuestEditor::ShowTestQuestWarning()
{
	GFeedback->ShowWarn( TXT("Your modifications qualify current quest graph as test one. It won't be visible in game. If it's not your intention don't check isTest flag on graph and don't use blocks from 'Tests' group.") );
}

void CEdQuestEditor::SetGraphEditorScaleAndOffset( Float scale, const wxPoint& offset )
{
	m_questGraphEditor->SetScaleAndOffset( scale, offset );
}
