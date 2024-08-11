#include "build.h"
#include "factsDBEditor.h"
#include "gameFactEditor.h"
#include "../../common/engine/gameTimeManager.h"

BEGIN_EVENT_TABLE( CEdFactsDB, wxPanel )
END_EVENT_TABLE()


CEdFactsDB::CEdFactsDB()
	: m_factsDB(NULL)
	, m_selectedID(-1)
{
}

CEdFactsDB::~CEdFactsDB()
{
	OnDetach();
}

void CEdFactsDB::OnAttach( CEdQuestEditor& host, wxWindow* parent )
{
	wxXmlResource::Get()->LoadPanel( this, parent, wxT("FactsDBEditor") );
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CEdFactsDB::OnClose ), 0, this );

	m_idsListBox = XRCCTRL( *this, "idsListBox", wxListBox );
	m_idsListBox->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( CEdFactsDB::OnIDsContextMenu ), NULL, this );
	m_idsListBox->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( CEdFactsDB::OnIDSelected ), NULL, this );

	m_valuesListBox = XRCCTRL( *this, "valuesListBox", wxListBox );
	m_valuesListBox->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( CEdFactsDB::OnValuesContextMenu ), NULL, this );
	m_valuesListBox->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( CEdFactsDB::ViewFact ), NULL, this );

	m_queryResult = XRCCTRL( *this, "statusBar", wxStaticText );

	m_filterIdsButton = XRCCTRL( *this, "filterIdsButton", wxBitmapButton );
	m_filterIdsButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdFactsDB::OnFilterIDs ), NULL, this );
	m_idFilterValue = XRCCTRL( *this, "idFilterValue", wxTextCtrl );
	m_idFilterValue->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( CEdFactsDB::OnFilterIDs ), NULL, this );

	m_factEditor = new CEdGameFact( this );
	m_factEditor->Centre( wxBOTH );

	Centre( wxBOTH );
	Layout();

	// Register listener
	SEvents::GetInstance().RegisterListener( CNAME( GameStarted ), this );

	AttachToFactsDB();
}

void CEdFactsDB::OnDetach()
{
	m_factsDB = NULL;
	m_idsListBox = NULL;
	m_valuesListBox = NULL;
	m_filterIdsButton = NULL;
	m_idFilterValue = NULL;
	delete m_factEditor; m_factEditor = NULL;

	// Unregister listener
	SEvents::GetInstance().UnregisterListener( this );
}

void CEdFactsDB::OnCreateBlockContextMenu( TDynArray< SToolMenu >& subMenus, const CQuestGraphBlock* atBlock  )
{
	// we don't want to enhance the main editor's context menus with any options
}

void CEdFactsDB::OnClose( wxCloseEvent& event )
{
	Hide();
}

void CEdFactsDB::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( GameStarted ) )
	{
		AttachToFactsDB();
	}
}

void CEdFactsDB::AttachToFactsDB()
{	
	m_factsDB = GCommonGame->GetSystem< CFactsDB >();
	RefreshIDsList();
}

void CEdFactsDB::OnFactsDBChanged( const String& id )
{
	RefreshIDsList();
}

void CEdFactsDB::OnFactsDBDestroyed()
{
	m_factsDB = NULL;
	m_idsListBox->Clear();
	m_valuesListBox->Clear();
}

void CEdFactsDB::OnIDSelected( wxCommandEvent& event )
{
	m_selectedID = event.GetSelection();
	RefreshFactsList();
}

void CEdFactsDB::ViewFact( wxCommandEvent& event )
{
	Int32 selectedFactIdx = event.GetSelection();
	if ( ( selectedFactIdx < 0 ) || ( m_factsList.Size() == 0 ) )
	{
		return;
	}

	m_factEditor->Attach( *( m_factsList[ selectedFactIdx ] ) );
	m_factEditor->ShowModal();
	this->SetFocus();
}

void CEdFactsDB::OnFilterIDs(wxCommandEvent& event )
{
	m_idFilter = m_idFilterValue->GetValue().wc_str();
	RefreshIDsList();
}

void CEdFactsDB::OnAddID( wxCommandEvent& event )
{
	String newID = AskForText();
	if ( newID.GetLength() > 0 )
	{
		m_factsDB->AddID(newID);
		RefreshIDsList();
		SelectID( newID );
	}
}

void CEdFactsDB::OnAddFact( wxCommandEvent& event )
{
	if ( m_selectedID < 0 )
	{
		return;
	}

	CFactsDB::Fact newFact;
	newFact.m_time = GGame->GetEngineTime();
	m_factEditor->Attach( newFact );
	Int32 closeResult = m_factEditor->ShowModal();
	this->SetFocus();

	if ( closeResult != wxID_OK )
	{
		return;
	}

	String factID = m_idsList[ m_selectedID ];
	m_factsDB->AddFact( factID, newFact.m_value, newFact.m_time, newFact.m_expiryTime );

	RefreshIDsList();
	RefreshFactsList();
}

void CEdFactsDB::OnQuerySum( wxCommandEvent& event )
{
	if ( m_selectedID < 0 )
	{
		return;
	}

	Int32 result = m_factsDB->QuerySum( m_idsList[ m_selectedID ] );
	String resultStr = String::Printf( TXT( "Sum of values for ID '%s' = %d" ),
		m_idsList[ m_selectedID ].AsChar(), result );

	m_queryResult->SetLabel( resultStr.AsChar() );
}

void CEdFactsDB::OnQuerySumSince( wxCommandEvent& event )
{
	// run the query
	if ( m_selectedID < 0 )
	{
		return;
	}

	Float lowerTimeBound = AskForTime();

	Int32 result = m_factsDB->QuerySumSince( m_idsList[ m_selectedID ], lowerTimeBound );
	String resultStr = String::Printf( TXT( "Sum of values for ID '%s' = %d" ),
		m_idsList[ m_selectedID ].AsChar(), result );

	m_queryResult->SetLabel( resultStr.AsChar() );
}

void CEdFactsDB::OnQueryLatestValue( wxCommandEvent& event )
{
	if ( m_selectedID < 0 )
	{
		return;
	}

	Int32 result = m_factsDB->QueryLatestValue( m_idsList[ m_selectedID ] );
	String resultStr = String::Printf( TXT( "Most recently added value to ID '%s' = %d" ),
		m_idsList[ m_selectedID ].AsChar(), result );

	m_queryResult->SetLabel( resultStr.AsChar() );
}

void CEdFactsDB::RefreshIDsList()
{
	String previouslySelectedID;
	if ( m_selectedID >= 0)
	{
		previouslySelectedID = m_idsList[ m_selectedID ];
	}
	
	// cleanup current contents of the views and get fresh copies
	m_idsListBox->Clear();
	m_valuesListBox->Clear();

	m_idsList.Clear();
	m_selectedID = -1;

	m_factsList.Clear();

	if ( m_factsDB == NULL )
	{
		return;
	}
	TDynArray< String > tempIdsList;
	m_factsDB->GetIDs( tempIdsList );

	// filter the list
	Bool filteringEnabled = m_idFilter.GetLength() > 0;
	String* candidateId = NULL;
	Uint32 count = tempIdsList.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		candidateId = &( tempIdsList[ i ] );

		if ( !filteringEnabled || ( candidateId->ContainsSubstring( m_idFilter ) ) )
		{
			m_idsList.PushBack( *candidateId );
		}
	}

	// fill in the list widget
	count = m_idsList.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		m_idsListBox->Append( m_idsList[ i ].AsChar() );
	}

	SelectID( previouslySelectedID );
}

void CEdFactsDB::SelectID( const String& id )
{
	m_selectedID = -1;

	// restore the previous selection
	if ( id.GetLength() > 0 )
	{
		Uint32 count = m_idsList.Size();
		for ( Uint32 i = 0; i < count; ++i )
		{
			if ( m_idsList[ i ] == id )
			{
				m_selectedID = i;
			}
		}
	}

	if ( m_selectedID >= 0 )
	{
		m_idsListBox->SetSelection( m_selectedID, true );
		RefreshFactsList();
	}
}

void CEdFactsDB::RefreshFactsList()
{
	if (( m_factsDB == NULL ) || ( m_selectedID < 0 ))
	{
		return;
	}

	m_valuesListBox->Clear();
	m_factsList.Clear();

	m_factsDB->GetFacts( m_idsList[ m_selectedID ], m_factsList );

	Uint32 count = m_factsList.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		m_valuesListBox->Append( FactAsString( m_factsList[ i ] ).AsChar() );
	}
}

void CEdFactsDB::OnIDsContextMenu( wxMouseEvent& event )
{
	ShowContextMenu( event, m_idsListBox );
}

void CEdFactsDB::OnValuesContextMenu( wxMouseEvent& event )
{
	ShowContextMenu( event, m_valuesListBox );
}

void CEdFactsDB::ShowContextMenu( wxMouseEvent& event, wxListBox* owner )
{
	if ( !m_factsDB )
	{
		return;
	}

	ASSERT( owner != NULL );
	wxMenu contextMenu;

	wxMenuItem* addNewID = contextMenu.Append( 0, TXT( "Add new ID" ) );
	contextMenu.Connect( 0, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFactsDB::OnAddID ), NULL, this );

	if ( m_selectedID >= 0 )
	{
		wxMenuItem* addNewID = contextMenu.Append( 1, TXT( "Add new fact" ) );
		contextMenu.Connect( 1, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFactsDB::OnAddFact ), NULL, this );

		contextMenu.AppendSeparator();

		wxMenuItem* querySum = contextMenu.Append( 2, TXT( "Query sum" ) );
		contextMenu.Connect( 2, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFactsDB::OnQuerySum ), NULL, this );

		wxMenuItem* querySumSince = contextMenu.Append( 3, TXT( "Query sum since" ) );
		contextMenu.Connect( 3, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFactsDB::OnQuerySumSince ), NULL, this );

		wxMenuItem* queryLatestValue = contextMenu.Append( 4, TXT( "Query latest value" ) );
		contextMenu.Connect( 4, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFactsDB::OnQueryLatestValue), NULL, this );
	}

	owner->PopupMenu( &contextMenu );
}

String CEdFactsDB::FactAsString( const CFactsDB::Fact* fact ) const
{
	EngineTime time( fact->m_time );

	String timeStr = String::Printf( TXT( "%f" ), (float) time );
	String tmpStr = String::Printf( TXT( "Time: %s, Val: %d, Exp: %s" ), 
		timeStr.AsChar(), 
		fact->m_value, 
		( ( fact->m_expiryTime == CFactsDB::EXP_NEVER ) ? TXT( "no" ) : TXT( "yes" ) ) );

	return tmpStr;
}

String CEdFactsDB::AskForText()
{
	// create and show the dialog box
	wxDialog dialog( this, wxID_ANY, TXT( "Create ID" ), wxDefaultPosition, wxSize(-1, 250) );
	dialog.Centre( wxBOTH );

	wxBoxSizer* vertSizer = new wxBoxSizer( wxVERTICAL );

	wxPanel* entryPanel = new wxPanel( &dialog, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* entryPanelSizer = new wxBoxSizer( wxHORIZONTAL );

	wxStaticText* label = new wxStaticText( entryPanel, wxID_ANY, wxT("Fact ID:"), wxDefaultPosition, wxDefaultSize, 0 );
	label->Wrap( -1 );
	entryPanelSizer->Add( label, 0, wxALL, 5 );

	wxTextCtrl* idValue = new wxTextCtrl( entryPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	entryPanelSizer->Add( idValue, 1, wxALL, 5 );

	entryPanel->SetSizer( entryPanelSizer );
	entryPanelSizer->Fit( entryPanel );
	entryPanel->Layout();
	vertSizer->Add( entryPanel );

	wxStdDialogButtonSizer* decisionButtons = new wxStdDialogButtonSizer();
	wxButton* decisionButtonsOK = new wxButton( &dialog, wxID_OK );
	decisionButtonsOK->SetDefault();
	wxButton* decisionButtonsCancel = new wxButton( &dialog, wxID_CANCEL );
	decisionButtons->AddButton( decisionButtonsOK );
	decisionButtons->AddButton( decisionButtonsCancel );
	decisionButtons->Realize();
	vertSizer->Add( decisionButtons, 1, wxEXPAND, 5 );

	dialog.SetSizer( vertSizer );
	vertSizer->Fit( &dialog );
	dialog.Layout();

	String text;
	if ( dialog.ShowModal() == wxID_OK )
	{
		text = idValue->GetValue();
	}
	return text;
}

Float CEdFactsDB::AskForTime()
{
	wxDialog dialog( this, wxID_ANY, TXT( "Enter lower time bound" ), wxDefaultPosition, wxSize(200, 70) );
	dialog.Centre( wxBOTH );

	wxBoxSizer* vertSizer = new wxBoxSizer( wxVERTICAL );

	wxPanel* entryPanel = new wxPanel( &dialog, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* entryPanelSizer = new wxBoxSizer( wxHORIZONTAL );

	wxSpinCtrl* secondValue = new wxSpinCtrl( entryPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, INT32_MAX, 0  );
	entryPanelSizer->Add( secondValue, 1, wxALL, 5 );

	entryPanel->SetSizer( entryPanelSizer );
	entryPanelSizer->Fit( entryPanel );
	entryPanel->Layout();
	vertSizer->Add( entryPanel, 1, wxALL | wxEXPAND, 5 );

	wxStdDialogButtonSizer* decisionButtons = new wxStdDialogButtonSizer();
	wxButton* decisionButtonsOK = new wxButton( &dialog, wxID_OK );
	decisionButtonsOK->SetDefault();
	decisionButtons->AddButton( decisionButtonsOK );
	decisionButtons->Realize();
	vertSizer->Add( decisionButtons, 1, wxEXPAND, 5 );

	dialog.SetSizer( vertSizer );
	vertSizer->Fit( &dialog );
	dialog.Layout();
	dialog.ShowModal();

	return (Float) secondValue->GetValue();
}
