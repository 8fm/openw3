#include "build.h"
#if 0
#include "interactiveDialogEditor.h"
#include "interactiveDialogGraphEditor.h"
#include "idConnectorPackEditor.h"
#include "idScreenplayEditor.h"

#include "../../games/r6/idGraph.h"
#include "../../games/r6/idBasicBlocks.h"
#include "../../games/r6/idResource.h"
#include "../../games/r6/idTopic.h"
#include "../../games/r6/idGraphBlockText.h"
#include "../../games/r6/idGraphBlockBranch.h"
#include "../../games/r6/idGraphBlockChoice.h"
#include "../../games/r6/idConnector.h"
#include "../../common/core/factory.h"
#include "../../common/core/diskFile.h"


BEGIN_EVENT_TABLE( CEdInteractiveDialogEditor, wxSmartLayoutPanel )

	EVT_MENU(		XRCID( "GenAudioMenuItem" ),		CEdInteractiveDialogEditor::OnMenuGenAudio		) 
	EVT_MENU(		XRCID( "GenLipsyncMenuItem" ),		CEdInteractiveDialogEditor::OnMenuGenLipsync	) 
	EVT_MENU(		XRCID( "SaveMenuItem" ),			CEdInteractiveDialogEditor::OnMenuSave			)
	EVT_MENU(		XRCID( "ExitMenuItem" ),			CEdInteractiveDialogEditor::OnMenuExit			)
	EVT_MENU(		XRCID( "m_menuCopy" ),				CEdInteractiveDialogEditor::OnMenuCopy			)
	EVT_MENU(		XRCID( "m_menuPaste" ),				CEdInteractiveDialogEditor::OnMenuPaste			)
	EVT_MENU(		XRCID( "m_menuCut" ),				CEdInteractiveDialogEditor::OnMenuCut			)
	EVT_MENU(		XRCID( "m_menuDelete" ),			CEdInteractiveDialogEditor::OnMenuDelete		)
	EVT_MENU(		XRCID( "m_menuUndo" ),				CEdInteractiveDialogEditor::OnMenuUndo			)
	EVT_MENU(		XRCID( "m_SaveLayout" ),			CEdInteractiveDialogEditor::OnSaveLayoutToFile	)
	EVT_MENU(		XRCID( "m_LoadLayout" ),			CEdInteractiveDialogEditor::OnLoadLayoutFromFile)
	EVT_BUTTON(		XRCID( "m_upBtn" ),					CEdInteractiveDialogEditor::OnTopicUp			)
	EVT_BUTTON(		XRCID( "m_downBtn" ),				CEdInteractiveDialogEditor::OnTopicDown			)
	EVT_BUTTON(		XRCID( "m_addBtn" ),				CEdInteractiveDialogEditor::OnTopicAdd			)
	EVT_BUTTON(		XRCID( "m_removeBtn" ),				CEdInteractiveDialogEditor::OnTopicRemove		)
	EVT_BUTTON(		XRCID( "m_renameBtn" ),				CEdInteractiveDialogEditor::OnTopicRename		)
	EVT_LISTBOX(	XRCID( "m_topicsListBox" ),			CEdInteractiveDialogEditor::OnTopicSelect		)
	EVT_CLOSE(											CEdInteractiveDialogEditor::OnClose				)
END_EVENT_TABLE()

IMPLEMENT_CLASS( CEdInteractiveDialogEditor, wxSmartLayoutPanel );

const size_t CEdInteractiveDialogEditor::PAGE_INDEX_TIMELINE			( 1 );
const size_t CEdInteractiveDialogEditor::PAGE_INDEX_SCREENPLAY			( 0 );
const size_t CEdInteractiveDialogEditor::PAGE_INDEX_ITEM_PROPERTIES		( 1 );
const size_t CEdInteractiveDialogEditor::PAGE_INDEX_DIALOG_PROPERTIES	( 0 );

CEdInteractiveDialogEditor::CEdInteractiveDialogEditor( wxWindow* parent, CInteractiveDialog* dialog /*= NULL */ )
	: wxSmartLayoutPanel( parent, TEXT( "InteractiveDialogEditor" ), false )
	, m_dialog( dialog )
	, m_dialogTopic( NULL )
	, m_editedObject( NULL )
	, m_numTopics( 0 )
	, m_screenplayCaretPos( -1 )
	, m_screenplayLinePos( -1 )
	, m_fillingScreenplayNow( false )

{
	

	// Preserve edited resource
	dialog->AddToRootSet();

	// Set icon
	wxIcon iconSmall;
	iconSmall.CopyFromBitmap( SEdResources::GetInstance().LoadBitmap( TXT( "IMG_BDI_DIALOG" ) ) );
	SetIcon( iconSmall );

	// Set auto layout
	SetAutoLayout( true );





	// Prepare dialog property notebook
	m_propertiesNotebook = new CEdAuiNotebook(
		this,
		wxID_ANY,
		wxDefaultPosition,
		wxDefaultSize,
		wxAUI_NB_TOP | wxAUI_NB_TAB_EXTERNAL_MOVE | wxAUI_NB_TAB_MOVE | wxAUI_NB_TAB_SPLIT | wxAUI_NB_SCROLL_BUTTONS
	);

	// Prepare editor notebook
	m_editorNotebook = new CEdAuiNotebook(
		this,
		wxID_ANY,
		wxDefaultPosition,
		wxDefaultSize,
		wxAUI_NB_TOP | wxAUI_NB_TAB_EXTERNAL_MOVE | wxAUI_NB_TAB_MOVE | wxAUI_NB_TAB_SPLIT | wxAUI_NB_SCROLL_BUTTONS
	);

	m_graphNotebook = new CEdAuiNotebook(
		this,
		wxID_ANY,
		wxDefaultPosition,
		wxDefaultSize,
		wxAUI_NB_TOP | wxAUI_NB_TAB_EXTERNAL_MOVE | wxAUI_NB_TAB_MOVE | wxAUI_NB_TAB_SPLIT | wxAUI_NB_SCROLL_BUTTONS
	);


	m_topicNotebook = new CEdAuiNotebook(
		this,
		wxID_ANY,
		wxDefaultPosition,
		wxDefaultSize,
		0
	);



	// Prepare topics list
	wxPanel* topicList = XRCCTRL(*this, "m_topicsEd", wxPanel);
	{
		m_addBtn			= XRCCTRL( *topicList, "m_addBtn", wxButton );
		m_removeBtn			= XRCCTRL( *topicList, "m_removeBtn", wxButton );
		m_renameBtn			= XRCCTRL( *topicList, "m_renameBtn", wxButton );
		m_upBtn				= XRCCTRL( *topicList, "m_upBtn", wxButton );
		m_downBtn			= XRCCTRL( *topicList, "m_downBtn", wxButton );
		m_topicsListBox		= XRCCTRL( *topicList, "m_topicsListBox", wxListBox );
	}

	topicList->SetMinSize(wxSize(300, 600));
	//topicList->Reparent(m_topicNotebook);

	// Prepare settings for property editors
	PropertiesPageSettings settings;
	settings.m_autoExpandGroups = true;

	// Prepare dialog property editor
	wxPanel* dialogPropertiesPanel = new wxPanel(this);
	{		
		m_dialogProperties = new CEdPropertiesPage( dialogPropertiesPanel, settings );
		dialogPropertiesPanel->SetSizer( new wxBoxSizer( wxVERTICAL ) );
		dialogPropertiesPanel->GetSizer()->Add( m_dialogProperties, 1, wxEXPAND  );
		m_dialogProperties->SetObject( dialog );
		m_dialogProperties->Bind( wxEVT_COMMAND_PROPERTY_CHANGED, &CEdInteractiveDialogEditor::OnDialogPropertyPanelChanged, this );
		m_dialogProperties->Bind( wxEVT_COMMAND_PROPERTY_SELECTED, &CEdInteractiveDialogEditor::OnDialogPropertyPanelPropertySelected, this );
	}


	// Prepare selected item property editor
	wxPanel* elementPropertiesPanel = new wxPanel(this);
	{
		m_elementProperties = new CEdPropertiesPage( elementPropertiesPanel, settings );
		elementPropertiesPanel->SetSizer( new wxBoxSizer( wxVERTICAL ) );
		elementPropertiesPanel->GetSizer()->Add( m_elementProperties, 1, wxEXPAND  );
		m_elementProperties->Bind( wxEVT_COMMAND_PROPERTY_CHANGED, &CEdInteractiveDialogEditor::OnElementPropertyPanelChanged, this );
		m_elementProperties->Bind( wxEVT_COMMAND_PROPERTY_SELECTED, &CEdInteractiveDialogEditor::OnElementPropertyPanelPropertySelected, this );
	}


	// Prepare rich text control
	wxPanel* screenplayPanel = new wxPanel(this);
	{
		m_screenplayCtrl = new CEdScreenplayControl( screenplayPanel, this );
		m_screenplayCtrl->Init( screenplayPanel );
	}


	// Prepare graph editor
	wxPanel* graphPanel = new wxPanel(this);
	{		
		m_graphEditor = new CEdInteractiveDialogGraphEditor( graphPanel, this );
		graphPanel->SetSizer( new wxBoxSizer( wxVERTICAL ) );
		graphPanel->GetSizer()->Add( m_graphEditor, 1, wxEXPAND );
		m_graphEditor->SetSize( graphPanel->GetSize() ); // refresh size, for some reason it's not done automaticly
	}	

	// Prepare block menu
	wxMenu* blockMenu = GetMenuBar()->GetMenu( 2 );
	{
		m_graphEditor->FillBlockMenu( *blockMenu );
	}

	{ // dockable panels
		m_auiManager.SetManagedWindow( this );


		m_auiManager.AddPane(
			m_editorNotebook,
			wxAuiPaneInfo().Name("Editor").Center().Caption("Editor window").CloseButton(false).CaptionVisible(false)
		);

		


		m_auiManager.AddPane(
			m_topicNotebook,
			wxAuiPaneInfo().Name("Topics").Left().Caption("Topics window").MaximizeButton(true).CloseButton(false).PinButton(true).CaptionVisible(false)
		);

		m_auiManager.AddPane(
			m_propertiesNotebook,
			wxAuiPaneInfo().Name("Properties").Right().Caption("Properties window").MaximizeButton(true).CloseButton(false).PinButton(true).CaptionVisible(false)
		);

		m_auiManager.AddPane(
			m_graphNotebook,
			wxAuiPaneInfo().Name("Tech").Bottom().Caption("Technical designer window").MaximizeButton(true).CloseButton(false).PinButton(true).CaptionVisible(false)
		);


		m_propertiesNotebook->AddPage(dialogPropertiesPanel, wxT("Dialog properties"), true);
		m_propertiesNotebook->AddPage(elementPropertiesPanel, wxT("Selected item properties"), false);
		m_propertiesNotebook->SetMinSize(wxSize(600, 200));


		m_topicNotebook->AddPage(topicList, wxT("Topics"), false);
		m_topicNotebook->SetMinSize(wxSize(600, 200));


		m_editorNotebook->AddPage(screenplayPanel, wxT("Screenplay"), true);
		m_editorNotebook->AddPage(new wxPanel(this), wxT("Preview"), false);
		m_editorNotebook->SetMinSize(wxSize(200, 200));

		m_graphNotebook->AddPage(graphPanel, wxT("Graph"), true);
		m_graphNotebook->AddPage(new wxPanel(this), wxT("Timeline"), false);
		m_graphNotebook->SetMinSize(wxSize(200, 200));

		m_propertiesNotebook->Layout();
		m_topicNotebook->Layout();
		m_editorNotebook->Layout();
		m_graphNotebook->Layout();


		//m_auiManager.AddPane( m_editorNotebook);
		m_auiManager.Update();
	}

	// Load layout
	LoadOptionsFromConfig();
	Layout();

	// Select first topic at start
	OnTopicsListChanged();
	ChangeTopic( m_dialog->GetTopics().Empty() ? nullptr : m_dialog->GetTopics()[ 0 ] );

	// Fix block names when needed
	EnsureBlockNameUniqueness();
}

CEdInteractiveDialogEditor::~CEdInteractiveDialogEditor()
{
	m_auiManager.UnInit();
	m_dialog->RemoveFromRootSet();
}

void CEdInteractiveDialogEditor::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
}

void CEdInteractiveDialogEditor::SaveOptionsToConfig()
{
	SaveLayout( TXT("/Frames/InteractiveDialogEditor") );

	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Frames/InteractiveDialogEditor") );

	String layoutPerspective = m_auiManager.SaveWholeLayout().wc_str();

	config.Write( TXT("wholeLayout"), layoutPerspective );
}

void CEdInteractiveDialogEditor::LoadOptionsFromConfig()
{
	LoadLayout( TXT("/Frames/InteractiveDialogEditor") );

	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Frames/InteractiveDialogEditor") );

	String layoutPerspective = config.Read( TXT("wholeLayout"), String::EMPTY );

	if(!m_auiManager.LoadWholeLayout(layoutPerspective))
	{
		String s = TXT("layout2|name=Editor;caption=Editor window;state=1020;dir=5;layer=0;row=0;pos=0;prop=100000;bestw=20;besth=20;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Topics;caption=Topics window;state=2044;dir=4;layer=2;row=0;pos=0;prop=101545;bestw=600;besth=200;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=1822;floaty=530;floatw=616;floath=234|name=Properties;caption=Properties window;state=2044;dir=4;layer=0;row=1;pos=0;prop=42418;bestw=600;besth=200;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=60;floaty=228;floatw=616;floath=234|name=Tech;caption=Technical designer window;state=2044;dir=3;layer=1;row=0;pos=0;prop=156037;bestw=20;besth=20;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=3612;floaty=638;floatw=1919;floath=1039|dock_size(5,0,0)=22|dock_size(3,1,0)=522|dock_size(4,2,0)=175|dock_size(4,0,1)=379|{{Editor}={2f471e0052ab04ae0000586600000003=*0,1@layout2|name=dummy;caption=;state=2098174;dir=3;layer=0;row=0;pos=0;prop=100000;bestw=676;besth=227;minw=676;minh=227;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=2f471e0052ab04ae0000586600000003;caption=;state=2098172;dir=5;layer=0;row=1;pos=0;prop=100000;bestw=649;besth=330;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|dock_size(5,0,1)=651|}}{{Topics}={autoTab=*0@layout2|name=dummy;caption=;state=2098174;dir=3;layer=0;row=0;pos=0;prop=100000;bestw=86;besth=482;minw=86;minh=482;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=autoTab;caption=;state=2098172;dir=4;layer=0;row=0;pos=0;prop=100000;bestw=200;besth=200;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|dock_size(4,0,0)=202|}}{{Properties}={2f9a361052ab03ff0000316600000002=*0,1@layout2|name=dummy;caption=;state=2098174;dir=3;layer=0;row=0;pos=0;prop=100000;bestw=188;besth=219;minw=188;minh=219;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=2f9a361052ab03ff0000316600000002;caption=;state=2098172;dir=5;layer=0;row=0;pos=0;prop=100000;bestw=200;besth=200;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|dock_size(5,0,0)=20|}}{{Tech}={2f1cbe1052ab0cc700012c0500000004=*0,1@layout2|name=dummy;caption=;state=2098174;dir=3;layer=0;row=0;pos=0;prop=100000;bestw=867;besth=251;minw=867;minh=251;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=2f1cbe1052ab0cc700012c0500000004;caption=;state=2098172;dir=5;layer=0;row=1;pos=0;prop=100000;bestw=180;besth=180;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|dock_size(5,0,1)=953|}}{{end}}");
		Bool r = m_auiManager.LoadWholeLayout(s);
		ASSERT(r, TXT("Default layout loading for Interactive Dialog Editor failed."));
	}
}



void CEdInteractiveDialogEditor::OnLoadLayoutFromFile( wxCommandEvent& event )
{
	wxFileDialog propertiesFileDialog( this, wxT( "Load layout" ), wxEmptyString, wxEmptyString, wxT( "Layout file (*.aui)|*.aui" ), wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	Int32 result = propertiesFileDialog.ShowModal();

	if ( result == wxID_CANCEL )
	{
		return;
	}


	String path( propertiesFileDialog.GetPath().wx_str() );
	String loaded;
	if( !GFileManager->LoadFileToString( path, loaded, true ) )
	{
		wxMessageBox( wxT("Could not load file"), wxT("Layout file loading error"), wxICON_ERROR | wxOK);
		return;
	}

	String tmp = loaded.StringBefore(TXT("%"));
	if( tmp != TXT("InteractiveDialogEditor") )
	{
		wxMessageBox( wxT("The file is not an Interactive Dialog Editor Layout!"), wxT("Layout file loading error"), wxICON_ERROR | wxOK );
		return;
	}

	m_auiManager.LoadWholeLayout(loaded.StringAfter(TXT("%")));
}

void CEdInteractiveDialogEditor::OnSaveLayoutToFile( wxCommandEvent& event )
{
	wxFileDialog propertiesFileDialog( this, wxT( "Save layout" ), wxEmptyString, wxEmptyString, wxT( "Layout file (*.aui)|*.aui" ), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

	Int32 result = propertiesFileDialog.ShowModal();

	if ( result == wxID_CANCEL )
	{
		return;
	}

	String path( propertiesFileDialog.GetPath().wx_str() );
	GFileManager->SaveStringToFile( path, TXT("InteractiveDialogEditor%"), false );
	GFileManager->SaveStringToFile( path, m_auiManager.SaveWholeLayout().wx_str(), true );
}

void CEdInteractiveDialogEditor::OnDialogPropertyPanelChanged( wxCommandEvent& event )
{
	OnTopicsListChanged();
}

void CEdInteractiveDialogEditor::OnDialogPropertyPanelPropertySelected( wxCommandEvent& event )
{
	CBasePropItem *ai = m_dialogProperties->GetActiveItem();
	if ( NULL == ai )
	{
		return;
	}

	Uint32 num = ai->GetNumObjects();
	CIDTopic *topicToSet( NULL );

	if ( 1 == num )
	{
		// it's a thread, go and get it
		if ( CIDTopic* topicToSet = ai->GetParentObject( 0 ).As< CIDTopic >() )
		{
			ChangeTopic( topicToSet );

			if ( nullptr == topicToSet )
			{
				m_topicsListBox->Select( -1 );
				OnTopicSelectionChanged( Uint32( -1 ) );
			}
		}
	}

}

void CEdInteractiveDialogEditor::OnElementPropertyPanelChanged( wxCommandEvent& event )
{
	CIDGraphBlock *block = Cast < CIDGraphBlock > ( m_editedObject.Get() );
	if ( block )
	{
		if ( !block->IsRegular() )
		{
			block->OnRebuildSockets();
		}

		m_screenplayCtrl->SetRelativeCaretPosition( block->GetName(), 0, 0, false );
		FillScreenplay();

		m_graphEditor->Repaint();
	}
}

void CEdInteractiveDialogEditor::OnElementPropertyPanelPropertySelected( wxCommandEvent& event )
{
}

void CEdInteractiveDialogEditor::OnMenuSave( wxCommandEvent& event )
{
	m_dialog->Save();
}

void CEdInteractiveDialogEditor::OnMenuExit( wxCommandEvent& event )
{
	Close();
}

void CEdInteractiveDialogEditor::OnClose( wxCloseEvent &event )
{
	SaveOptionsToConfig();
}

void CEdInteractiveDialogEditor::ChangeTopic( CIDTopic* newTopic )
{
	m_dialogTopic = newTopic;
	m_graphEditor->SetGraph( newTopic ? newTopic->GetGraph() : NULL );

	Uint32 idx = Uint32( -1 );
	if ( newTopic )
	{
		// Find topic index
		const Uint32 numTopics = m_dialog->GetTopics().Size();
		for ( Uint32 i = 0; i < m_numTopics; ++i )
		{
			if ( m_dialog->GetTopics()[ i ] == newTopic )
			{
				idx = i;
				break;
			}
		}

		ASSERT( idx < m_numTopics );
		m_topicsListBox->Select( idx );
	}
	else
	{
		idx = Uint32( m_topicsListBox->GetSelection() );
	}

	if ( idx < m_numTopics )
	{
		OnTopicSelectionChanged( idx );
	}

	SetTitle(	String::Printf( TXT("Interactive dialog editor: file \"%s\", topic \"%s\""), 
				m_dialog->GetFile()->GetAbsolutePath().AsChar(), 
				newTopic ? newTopic->GetName().AsString().AsChar() : TXT("") ).AsChar() );

	FillScreenplay();
}

void CEdInteractiveDialogEditor::SetPropertiesObject( IReferencable* obj, Bool switchTab /* = true */ )
{
	m_elementProperties->SetObject( obj );
	m_editedObject = obj;

	if ( switchTab )
	{
		m_propertiesNotebook->SetSelection( PAGE_INDEX_ITEM_PROPERTIES );
	}
}

void CEdInteractiveDialogEditor::OnTopicAdd( wxCommandEvent& event )
{
	String topicName = InputBox( this, TXT("New topic"), TXT("Enter new topic name:"), TXT("Unnamed topic") );
	if ( topicName.Empty() )
	{
		return;
	}

	const Int32 topicIndex = m_topicsListBox->GetSelection();
	CIDTopic* topic = m_dialog->NewTopic( CName( topicName ), Uint32( topicIndex + 1 ) );
	if ( topic )
	{
		m_dialogProperties->RefreshValues();
		OnTopicsListChanged();
		m_topicsListBox->Select( topicIndex + 1 );
		ChangeTopic( topic );
	}
}

void CEdInteractiveDialogEditor::OnTopicRemove( wxCommandEvent& event )
{
	const Int32 topicIndex = m_topicsListBox->GetSelection();
	ASSERT( topicIndex >= 0 && topicIndex < Int32( m_numTopics ) );

	CIDTopic* topic = m_dialog->GetTopics()[ topicIndex ];
	if ( topic )
	{
		if ( wxYES == wxMessageBox( String::Printf( TXT("Delete topic %s?"), topic->GetName().AsChar() ).AsChar(), TXT("Are you sure?"), wxYES_NO | wxICON_QUESTION, this ) )
		{
			m_dialog->RemoveTopic( Uint32( topicIndex ) );
			m_dialogProperties->RefreshValues();
			OnTopicsListChanged();
			m_topicsListBox->Select( topicIndex - 1 );
			OnTopicSelectionChanged( Uint32( topicIndex - 1 ) );
			ChangeTopic( ( topicIndex > 0 && topicIndex - 1 < Int32( m_numTopics ) ) ? m_dialog->GetTopics()[ topicIndex - 1 ] : nullptr );
		}
	}
}

void CEdInteractiveDialogEditor::OnTopicRename( wxCommandEvent& event )
{
	const Int32 topicIndex = m_topicsListBox->GetSelection();
	ASSERT( topicIndex >= 0 && topicIndex < Int32( m_numTopics ) );

	CIDTopic* topic = m_dialog->GetTopics()[ topicIndex ];
	if ( topic )
	{
		String topicName = InputBox( this, TXT("Rename topic"), TXT("Enter new topic name:"), topic->GetName().AsChar() );
		if ( topicName.Empty() )
		{
			return;
		}

		topic->SetName( CName( topicName ) );
		m_dialogProperties->RefreshValues();
		OnTopicsListChanged();
	}
}

void CEdInteractiveDialogEditor::OnTopicUp( wxCommandEvent& event )
{
	Int32 topicIndex = m_topicsListBox->GetSelection();
	ASSERT( topicIndex > 0 && topicIndex < Int32( m_numTopics ) );

	m_dialog->SwapTopics( Uint32( topicIndex ), Uint32( topicIndex - 1 ) ); 
	m_dialogProperties->RefreshValues();
	OnTopicsListChanged();
	m_topicsListBox->Select( topicIndex - 1 );
	OnTopicSelectionChanged( Uint32( topicIndex - 1 ) );
}

void CEdInteractiveDialogEditor::OnTopicDown( wxCommandEvent& event )
{
	Int32 topicIndex = m_topicsListBox->GetSelection();
	ASSERT( topicIndex >= 0 && topicIndex < Int32( m_numTopics ) - 1 );

	m_dialog->SwapTopics( Uint32( topicIndex ), Uint32( topicIndex + 1 ) ); 
	m_dialogProperties->RefreshValues();
	OnTopicsListChanged();
	m_topicsListBox->Select( topicIndex + 1 );
	OnTopicSelectionChanged( Uint32( topicIndex + 1 ) );
}

void CEdInteractiveDialogEditor::OnTopicSelect( wxCommandEvent& event )
{
	event.Skip();

	const Uint32 sel = m_topicsListBox->GetSelection();
	ASSERT( sel < m_numTopics );

	ChangeTopic( m_dialog->GetTopics()[ sel ] );
}

void CEdInteractiveDialogEditor::RefreshTopicsList()
{
	m_topicsListBox->Clear();
	Uint32 idx = Uint32( -1 );
	for ( Uint32 i = 0; i < m_numTopics; ++i )
	{
		const CIDTopic* topic = m_dialog->GetTopics()[ i ];
		m_topicsListBox->Insert( String::Printf( TXT("[%ld]: %s"), i, topic ? ( topic->GetName() ? topic->GetName().AsString().AsChar() : TXT("Unnamed topic") ) : TXT("NULL") ).AsChar(), i );

		if ( topic && topic == m_dialogTopic )
		{
			idx = i;
			m_topicsListBox->Select( i );
		}
	}
	m_topicsListBox->Refresh();
	OnTopicSelectionChanged( idx );

}

void CEdInteractiveDialogEditor::OnTopicsListChanged()
{
	const Uint32 newNumTopics = m_dialog->GetTopics().Size();
	if ( newNumTopics < m_numTopics )
	{
		m_numTopics = newNumTopics;

		// deleted topic being edited?
		if ( m_dialog->GetTopics().Exist( m_dialogTopic ) )
		{
			ChangeTopic( NULL );
		}
	}
	m_numTopics = newNumTopics;

	RefreshTopicsList();
}

void CEdInteractiveDialogEditor::OnTopicSelectionChanged( Uint32 idx )
{
	if ( idx < m_numTopics )
	{
		if ( idx == m_numTopics - 1 )
		{
			// last topic, cannot move down
			m_downBtn->Disable();
		}
		else
		{
			m_downBtn->Enable();
		}

		if ( idx == 0 )
		{
			// first topic, cannot move up
			m_upBtn->Disable();
		}
		else
		{
			m_upBtn->Enable();
		}

		// enable remove/rename btn
		m_removeBtn->Enable();
		m_renameBtn->Enable();
	}
	else
	{
		// nothing is selected, disable both buttons
		m_downBtn->Disable();
		m_upBtn->Disable();

		// and disable remove/rename buttons
		m_removeBtn->Disable();
		m_renameBtn->Disable();
	}

	m_propertiesNotebook->SetSelection( PAGE_INDEX_DIALOG_PROPERTIES );
}

void CEdInteractiveDialogEditor::OnMenuCopy( wxCommandEvent& event )
{
	// for now, just pass to the graph editor
	if ( m_graphEditor->HasFocus() )
	{
		event.Skip();
		m_graphEditor->OnCopySelection();
	}
}

void CEdInteractiveDialogEditor::OnMenuPaste( wxCommandEvent& event )
{
	// for now, just pass to the graph editor
	if ( m_graphEditor->HasFocus() )
	{
		event.Skip();
		m_graphEditor->OnPaste();
	}
}

void CEdInteractiveDialogEditor::OnMenuCut( wxCommandEvent& event )
{
	// for now, just pass to the graph editor
	if ( m_graphEditor->HasFocus() )
	{
		event.Skip();
		m_graphEditor->OnCutSelection();
	}
}

void CEdInteractiveDialogEditor::OnMenuDelete( wxCommandEvent& event )
{
	// for now, just pass to the graph editor
	if ( m_graphEditor->HasFocus() )
	{
		event.Skip();
		m_graphEditor->OnDeleteSelection();
	}

	// workaround...
	if ( m_screenplayCtrl->HasFocus() )
	{
		if ( false == m_screenplayCtrl->HasSelection() )
		{
			const Int32 pos = m_screenplayCtrl->GetCaretPosition();
			m_screenplayCtrl->SetSelection( pos + 1, pos + 2 ); 
		}
		
		wxString selectedText = m_screenplayCtrl->GetStringSelection();

		m_screenplayCtrl->DeleteSelectedContent();

		if ( selectedText.find( '\n' ) < selectedText.length() )
		{
			ApplyScreenplayChangesIfNeeded();
		}

		HiliteBlockUnderScreenplayCaret();
	}
}

void CEdInteractiveDialogEditor::OnMenuUndo( wxCommandEvent& event )
{
	// TODO: implement ;)
	event.Skip();
}

void CEdInteractiveDialogEditor::EnsureBlockNameUniqueness()
{
	for ( CInteractiveDialog::TopicIterator it( m_dialog ); it; ++it )
	{
		( *it )->EnsureBlockNameUniqueness();
	}
}

void CEdInteractiveDialogEditor::DoPendingBlockOperations()
{
	m_graphEditor->Freeze();

	for ( Uint32 i = 0; i < m_pendingBlockOperations.Size(); ++i )
	{
		SIDBlockEditorOp& op = m_pendingBlockOperations[ i ];

		if ( op.m_op == BLOCK_AddText )
		{
			op.m_block = DoAddTextBlock();
			op.m_op = BLOCK_Update;
		}
		else if ( op.m_op == BLOCK_AddChoice )
		{
			op.m_block = DoAddChoiceBlock();
			op.m_op = BLOCK_Update;
		}
		// no else here is intentional
		if ( op.m_op == BLOCK_Update )
		{
			CIDGraphBlock* block = op.m_block.Get();
			if ( block )
			{
				if ( op.m_flags & UPDATE_Name )
				{
					DoUpdateBlockName( block, op.m_name );
				}

				if ( op.m_flags & UPDATE_Comment )
				{
					DoUpdateBlockComment( block, op.m_comment );
				}

				CIDGraphBlockText* textBlock = Cast< CIDGraphBlockText > ( block );
				if ( textBlock )
				{
					if ( op.m_flags & UPDATE_Lines )
					{
						DoUpdateTextBlockLines( textBlock, op.m_stubs );
					}
				}
				else
				{
					CIDGraphBlockChoice* choiceBlock = Cast< CIDGraphBlockChoice > ( block );
					if ( choiceBlock )
					{
						if ( op.m_flags & UPDATE_Options )
						{
							DoUpdateChoiceBlockOptions( choiceBlock, op.m_options );
						}
					}
				}
			}
		}
		else if ( op.m_op == BLOCK_Delete )
		{
			DoDeleteBlock( op.m_block.Get() );
		}
		else 
		{
			// BLOCK_DoNothing:
			// do nothing :)
		}
	}

	m_pendingBlockOperations.Clear();
	m_graphEditor->Thaw();
}

void CEdInteractiveDialogEditor::DoDeleteBlock( CIDGraphBlock* block )
{
	if ( block )
	{
		m_graphEditor->DoDeleteBlock( block );
		m_graphEditor->Repaint();
	}
}

void CEdInteractiveDialogEditor::OnGraphStructureModified( IGraphContainer* graph )
{
	if ( false == m_fillingScreenplayNow )
	{
		FillScreenplay();
	}
}

void CEdInteractiveDialogEditor::OnGraphSelectionChanged()
{

}

//------------------------------------------------------------------------------------------------------------------
// Factory 
//------------------------------------------------------------------------------------------------------------------
class CIDResourceFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CIDResourceFactory, IFactory, 0 );

public:
	CIDResourceFactory()
	{
		m_resourceClass = ClassID< CInteractiveDialog >();
	}

	virtual CResource* DoCreate( const FactoryOptions& options )
	{
		TDynArray< String > values;
		InputMultiBox( wxTheFrame, TXT("New interactive dialog"), TXT("Please add interlocutors:"), values );
		CInteractiveDialog* dialog = CreateObject< CInteractiveDialog >( options.m_parentObject );
		dialog->NewTopic( CNAME( Default ), 0 );
		for ( Uint32 i = 0; i < values.Size(); ++i )
		{
			if ( values[ i ].Empty() || values[ i ].EqualsNC( TXT("none") ) || values[ i ].EqualsNC( TXT("default") ) )
			{
				continue;
			}

			SIDInterlocutorDefinition def;
			def.m_interlocutorId = CName( values[ i ] );
			dialog->AddInterlocutor( def ); 
		}

		return dialog;
	}
};

BEGIN_CLASS_RTTI( CIDResourceFactory )
	PARENT_CLASS( IFactory )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CIDResourceFactory )



#endif