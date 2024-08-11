
/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "tagMiniEditor.h"
#include "tagEditor.h"

//#include "tagTreeItemData.h"
#include "tagListUpdater.h"

//Do not add custom headers between Header Include Start and Header Include End.
////Header Include Start
////Header Include End

#define ID_FILTER_TIMER		  12345
#define ID_REMOVE_HISTORY_TAG 54321

// Defined in tagEditor.cpp
//DEFINE_EVENT_TYPE( wxEVT_TAGEDITOR_OK )
//DEFINE_EVENT_TYPE( wxEVT_TAGEDITOR_CANCEL )
DEFINE_EVENT_TYPE( wxEVT_TAGEDITOR_SELECT )

//----------------------------------------------------------------------------
// CEdTagMiniEditor
//----------------------------------------------------------------------------
//Add Custom Events only in the appropriate block.
////Event Table Start
BEGIN_EVENT_TABLE(CEdTagMiniEditor,wxDialog)
	////Manual Code Start
	EVT_TIMER( ID_FILTER_TIMER, CEdTagMiniEditor::OnTimer )
	////Manual Code End
	
	EVT_CLOSE(CEdTagMiniEditor::OnClose)
	
	EVT_BUTTON(wxID_OK,CEdTagMiniEditor::OnOK)
	EVT_BUTTON(wxID_CANCEL,CEdTagMiniEditor::OnCancel)
END_EVENT_TABLE()
////Event Table End


CEdTagMiniEditor::CEdTagMiniEditor( wxWindow* parent, const TDynArray<CName>& currentTagList, Bool forceTagTreeVisible /*= false*/ )
    : wxDialog(parent, 1, wxT("Tag Mini Editor"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxSYSTEM_MENU | wxCLOSE_BOX)
	, m_tags( currentTagList )
	, m_filterTimer( this, ID_FILTER_TIMER )
    , m_hintMode(false)
    , m_lockOnEditTagChanged( false )
    , m_lockTagCompletion( false )
    , m_clearEditTagSelection( true )
	, m_blockKeyboardArrows( false )
	, m_allowOnlyValidKeyPresses( false )
	, m_saveOnClose( false )
	, m_isSelectionFromKeyboard( false )
	, m_forceTagTreeVisible( forceTagTreeVisible )
	, m_providersWillBeDisposedExternally( false )
{
    CreateGUIControls();

	// Format string
	String sTags;
    for (Uint32 i = 0; i < m_tags.Size(); ++i)
	{
        sTags += m_tags[i].AsString() + String(TXT("; "));
	}

    m_wxEditTag->SetValue(sTags.AsChar());

	m_availableTagsProviders.PushBack( new CHistoryTagListProvider() );
	m_availableTagsProviders.PushBack( new CWorldTagListProvider() );

	LoadOptionsFromConfig();
	// Update tag lists
    m_filter = TXT("=====UNDEFINED=====");
	UpdateTagTree();
}

CEdTagMiniEditor::CEdTagMiniEditor( wxWindow* parent, const TDynArray<CName>& currentTagList, wxTextCtrl* tagEditBox )
    : wxDialog(parent, 1, wxT("Known Tags"), wxDefaultPosition, wxDefaultSize, wxBORDER_NONE )
	, m_tags( currentTagList )
	, m_filterTimer( this, ID_FILTER_TIMER )
    , m_hintMode(true)
    , m_lockOnEditTagChanged( false )
    , m_lockTagCompletion( false )
    , m_clearEditTagSelection( true )
	, m_blockKeyboardArrows( false )
	, m_allowOnlyValidKeyPresses( false )
	, m_isSelectionFromKeyboard( false )
	, m_saveOnClose( false )
	, m_providersWillBeDisposedExternally( false )
{
    m_wxEditTag = tagEditBox;

    CreateGUIControlsHint();

	// Format string to show
	String sTags;
    for (Uint32 i = 0; i < m_tags.Size(); ++i)
	{
		sTags += m_tags[i].AsString() + TXT("; ");
	}
	m_wxEditTag->SetValue(sTags.AsChar());

	m_availableTagsProviders.PushBack( new CHistoryTagListProvider() );
	m_availableTagsProviders.PushBack( new CWorldTagListProvider() );

	LoadOptionsFromConfig();
	// Update tag lists
    m_filter = TXT("=====UNDEFINED=====");
	UpdateTagTree();
}

CEdTagMiniEditor::~CEdTagMiniEditor()
{
	m_filterTimer.Stop();
}

void CEdTagMiniEditor::OnCloseCleanup()
{
	SaveOptionsToConfig();

	SetTagListProviders( TDynArray<CTagListProvider*>(), false );

	if (m_hintMode)
	{
		m_wxEditTag->Disconnect(wxID_ANY, wxEVT_SET_FOCUS,   (wxObjectEventFunction)&CEdTagMiniEditor::OnEditTagSetFocus, NULL, this);
		m_wxEditTag->Disconnect(wxID_ANY, wxEVT_KILL_FOCUS,  (wxObjectEventFunction)&CEdTagMiniEditor::OnEditTagKillFocus, NULL, this);
		m_wxEditTag->Disconnect(wxID_ANY, wxEVT_KEY_DOWN,    (wxObjectEventFunction)&CEdTagMiniEditor::OnEditTagKeyDown, NULL, this);
		m_wxEditTag->Disconnect(wxID_ANY, wxEVT_LEFT_DOWN,   (wxObjectEventFunction)&CEdTagMiniEditor::OnEditTagMouseDown, NULL, this);
		m_wxEditTag->Disconnect(wxID_ANY, wxEVT_MIDDLE_DOWN, (wxObjectEventFunction)&CEdTagMiniEditor::OnEditTagMouseDown, NULL, this);
		m_wxEditTag->Disconnect(wxID_ANY, wxEVT_RIGHT_DOWN,  (wxObjectEventFunction)&CEdTagMiniEditor::OnEditTagMouseDown, NULL, this);
		m_wxEditTag->Disconnect(wxID_ANY, wxEVT_COMMAND_TEXT_UPDATED, (wxObjectEventFunction)&CEdTagMiniEditor::OnEditTagChanged, NULL, this);
	}
}

void CEdTagMiniEditor::CreateGUIControls()
{
	SetIcon(wxNullIcon);
	SetSize(8,8,523,428);
	Center();

	m_wxStaticText3 = new wxStaticText(this, ID_WXSTATICTEXT3, wxT("Single Click replaces the tag, Double Click additionally starts editing of the new tag."), wxPoint(9,81), wxDefaultSize, 0, wxT("m_wxStaticText3"));
	m_wxStaticText3->SetFont(wxFont(8, wxSWISS, wxNORMAL,wxNORMAL, false, wxT("Tahoma")));

	m_wxStaticText1 = new wxStaticText(this, ID_WXSTATICTEXT1, wxT("Type the Tags below, separate them with the ; symbol:"), wxPoint(5,5), wxDefaultSize, 0, wxT("m_wxStaticText1"));
	m_wxStaticText1->SetFont(wxFont(8, wxSWISS, wxNORMAL,wxNORMAL, false, wxT("Tahoma")));

	m_wxEditTag = new wxTextCtrlEx(this, ID_WXEDITTAG, wxT(""), wxPoint(5,26), wxSize(345,27), 0, wxDefaultValidator, wxT("m_wxEditTag"));
	m_wxEditTag->SetFont(wxFont(8, wxSWISS, wxNORMAL,wxNORMAL, false, wxT("Tahoma")));

	m_wxButtonOK = new wxButton(this, wxID_OK, wxT("OK"), wxPoint(354,27), wxSize(75,25), 0, wxDefaultValidator, wxT("m_wxButtonOK"));
	m_wxButtonOK->SetFont(wxFont(8, wxSWISS, wxNORMAL,wxNORMAL, false, wxT("Tahoma")));

	m_wxButtonCancel = new wxButton(this, wxID_CANCEL, wxT("Cancel"), wxPoint(433,28), wxSize(75,25), 0, wxDefaultValidator, wxT("m_wxButtonCancel"));
	m_wxButtonCancel->SetFont(wxFont(8, wxSWISS, wxNORMAL,wxNORMAL, false, wxT("Tahoma")));

	m_wxTagsList = new wxListBox( this, ID_WXTAGSTREE, wxPoint(6,99), wxSize(340,288), 0, NULL, wxLB_SORT );
	m_wxTagsList->SetFont(wxFont(8, wxSWISS, wxNORMAL,wxNORMAL, false, wxT("Tahoma")));
	
    // Connect events
    m_wxEditTag->Connect(wxID_ANY, wxEVT_SET_FOCUS,   (wxObjectEventFunction)&CEdTagMiniEditor::OnEditTagSetFocus, NULL, this);
    m_wxEditTag->Connect(wxID_ANY, wxEVT_KILL_FOCUS,  (wxObjectEventFunction)&CEdTagMiniEditor::OnEditTagKillFocus, NULL, this);
    m_wxEditTag->Connect(wxID_ANY, wxEVT_KEY_DOWN,    (wxObjectEventFunction)&CEdTagMiniEditor::OnEditTagKeyDown, NULL, this);
    m_wxEditTag->Connect(wxID_ANY, wxEVT_LEFT_DOWN,   (wxObjectEventFunction)&CEdTagMiniEditor::OnEditTagMouseDown, NULL, this);
    m_wxEditTag->Connect(wxID_ANY, wxEVT_MIDDLE_DOWN, (wxObjectEventFunction)&CEdTagMiniEditor::OnEditTagMouseDown, NULL, this);
    m_wxEditTag->Connect(wxID_ANY, wxEVT_RIGHT_DOWN,  (wxObjectEventFunction)&CEdTagMiniEditor::OnEditTagMouseDown, NULL, this);
    m_wxEditTag->Connect(wxID_ANY, wxEVT_COMMAND_TEXT_UPDATED, (wxObjectEventFunction)&CEdTagMiniEditor::OnEditTagChanged, NULL, this);
	
    m_wxTagsList->Connect(wxID_ANY, wxEVT_SET_FOCUS,    (wxObjectEventFunction)&CEdTagMiniEditor::OnEditTagSetFocus, NULL, this);
    m_wxTagsList->Connect(wxID_ANY, wxEVT_KILL_FOCUS,   (wxObjectEventFunction)&CEdTagMiniEditor::OnEditTagKillFocus, NULL, this);
	m_wxTagsList->Connect(wxID_ANY, wxEVT_RIGHT_DOWN, wxMouseEventHandler( CEdTagMiniEditor::OnTagContext ), NULL, this);
	m_wxTagsList->Connect(wxID_ANY, wxEVT_COMMAND_LISTBOX_SELECTED, (wxObjectEventFunction)&CEdTagMiniEditor::OnTagSelected, NULL, this);
	m_wxTagsList->Connect(wxID_ANY, wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, (wxObjectEventFunction)&CEdTagMiniEditor::OnTagDoubleClicked, NULL, this);
}

void CEdTagMiniEditor::CreateGUIControlsHint()
{
	SetIcon(wxNullIcon);
	SetMinSize(wxSize(150, 150));
	Center();

	// Base sizer
	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

	// Caption
	wxStaticText* text = new wxStaticText(this, wxID_ANY, wxT("Single Click replaces the tag, Double Click additionally starts editing of the new tag."), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
	text->SetFont(wxFont(8, wxSWISS, wxNORMAL,wxNORMAL, false, wxT("Tahoma")));
	text->Wrap(-1);
	sizer->Add(text, 0, wxALL|wxEXPAND, 5);

	m_wxTagsList = new wxListBox( this, ID_WXTAGSTREE, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_SORT );
	m_wxTagsList->SetFont(wxFont(8, wxSWISS, wxNORMAL,wxNORMAL, false, wxT("Tahoma")));
	sizer->Add(m_wxTagsList, 1, wxALL|wxEXPAND, 0);

	SetSizer(sizer);

    // Connect events
    m_wxEditTag->Connect(wxID_ANY, wxEVT_SET_FOCUS,   (wxObjectEventFunction)&CEdTagMiniEditor::OnEditTagSetFocus, NULL, this);
    m_wxEditTag->Connect(wxID_ANY, wxEVT_KILL_FOCUS,  (wxObjectEventFunction)&CEdTagMiniEditor::OnEditTagKillFocus, NULL, this);
    m_wxEditTag->Connect(wxID_ANY, wxEVT_KEY_DOWN,    (wxObjectEventFunction)&CEdTagMiniEditor::OnEditTagKeyDown, NULL, this);
    m_wxEditTag->Connect(wxID_ANY, wxEVT_LEFT_DOWN,   (wxObjectEventFunction)&CEdTagMiniEditor::OnEditTagMouseDown, NULL, this);
    m_wxEditTag->Connect(wxID_ANY, wxEVT_MIDDLE_DOWN, (wxObjectEventFunction)&CEdTagMiniEditor::OnEditTagMouseDown, NULL, this);
    m_wxEditTag->Connect(wxID_ANY, wxEVT_RIGHT_DOWN,  (wxObjectEventFunction)&CEdTagMiniEditor::OnEditTagMouseDown, NULL, this);
    m_wxEditTag->Connect(wxID_ANY, wxEVT_COMMAND_TEXT_UPDATED, (wxObjectEventFunction)&CEdTagMiniEditor::OnEditTagChanged, NULL, this);

    m_wxTagsList->Connect(wxID_ANY, wxEVT_SET_FOCUS,    (wxObjectEventFunction)&CEdTagMiniEditor::OnEditTagSetFocus, NULL, this);
    m_wxTagsList->Connect(wxID_ANY, wxEVT_KILL_FOCUS,   (wxObjectEventFunction)&CEdTagMiniEditor::OnEditTagKillFocus, NULL, this);
    m_wxTagsList->Connect(wxID_ANY, wxEVT_CONTEXT_MENU, (wxObjectEventFunction)&CEdTagMiniEditor::OnTagContext, NULL, this);
	m_wxTagsList->Connect(wxID_ANY, wxEVT_COMMAND_LISTBOX_SELECTED, (wxObjectEventFunction)&CEdTagMiniEditor::OnTagSelected, NULL, this);
	m_wxTagsList->Connect(wxID_ANY, wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, (wxObjectEventFunction)&CEdTagMiniEditor::OnTagDoubleClicked, NULL, this);
}

void CEdTagMiniEditor::SaveOptionsToConfig()
{
    CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Dialogs/TagMiniEditor") );

	// save frame position & size
	// get info from native window placement to keep last (not maximized) size
	WINDOWPLACEMENT wp;
	GetWindowPlacement( GetHwnd(), &wp );

	Int32 x = wp.rcNormalPosition.left;
	Int32 y = wp.rcNormalPosition.top;
	Int32 width = GetSize().x;
	Int32 height = GetSize().y;
	
	config.Write( TXT("X"), x );
	config.Write( TXT("Y"), y );
	config.Write( TXT("Width"), width );
	config.Write( TXT("Height"), height );

	for ( TagProviderArray::iterator providerIter = m_availableTagsProviders.Begin();
		providerIter != m_availableTagsProviders.End(); ++providerIter )
	{
		(*providerIter)->SaveSession();
	}
}

void CEdTagMiniEditor::LoadOptionsFromConfig()
{
    CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Dialogs/TagMiniEditor") );

	Int32 x = config.Read( TXT("X"), 50 );
	Int32 y = config.Read( TXT("Y"), 50 );
	Int32 width = config.Read( TXT("Width"), 500 );
	Int32 height = config.Read( TXT("Height"), 400 );

	width = max(50, width);
	height = max(50, height);

	if( x < -1000 )
		x = 50;
	if( y < -1000 )
		y = 50;

    SetSize( x, y, width, height );

	for ( TagProviderArray::iterator providerIter = m_availableTagsProviders.Begin();
		providerIter != m_availableTagsProviders.End(); ++providerIter )
	{
		(*providerIter)->LoadSession();
	}
}



void CEdTagMiniEditor::UpdateTagTree()
{
	String filter = ExtractFilter();
    filter.Trim();

    if (filter == m_filter)
    {
        ShowTagTreeIfNeeded();
        return;
    }

    m_filter = filter;
    
    // Begin update
	m_wxTagsList->Freeze();
    m_wxTagsList->Clear();
	
    // Create list root item
	STagNode rootTagNode(TXT("Root"));
	TDynArray<CTagListProvider *>::iterator it;
	for (it = m_availableTagsProviders.Begin(); it != m_availableTagsProviders.End(); ++it)
	{
		(*it)->GetTags( rootTagNode, m_filter );
	}

	m_filteredTags.Clear();
	BuildTree( &rootTagNode );

	THashMap< String, Uint32 >::const_iterator
		tagCurr = m_filteredTags.Begin(),
		tagLast = m_filteredTags.End();
	for ( ; tagCurr != tagLast; ++tagCurr )
	{
		if ( tagCurr->m_second > 1 )
			m_wxTagsList->Append( ( tagCurr->m_first + TXT(" (") + ToString( tagCurr->m_second ) + TXT(")") ).AsChar(), 
									const_cast<void*>( static_cast<const void*>( &*tagCurr ) ) );
		else
			m_wxTagsList->Append( tagCurr->m_first.AsChar(), const_cast<void*>( static_cast<const void*>( &*tagCurr ) ) );
	}

	// End update
	m_wxTagsList->Thaw();
	m_wxTagsList->Refresh();

    ShowTagTreeIfNeeded();
}

void CEdTagMiniEditor::BuildTree( const STagNode *tagNode )
{
	if (tagNode->GetCount() > 0)
	{
		Uint32 * count = m_filteredTags.FindPtr( tagNode->GetName() );

		if ( count )
			*count += tagNode->GetCount();
		else
			m_filteredTags.Insert( tagNode->GetName(), tagNode->GetCount() );
	}
	const TDynArray<STagNode> &childNodes = tagNode->GetChildNodes();
	for (TDynArray<STagNode>::const_iterator it = childNodes.Begin(); it != childNodes.End(); ++it)
	{
		BuildTree(&(*it));
	}
}

void CEdTagMiniEditor::OnTimer( wxTimerEvent& event )
{
    UpdateTagTree();
}

void CEdTagMiniEditor::ShowTagTreeIfNeeded()
{
    if (m_hintMode) return;

    wxWindow *focused = wxWindow::FindFocus();

    bool bNeeded;
	
	if ( !m_forceTagTreeVisible )
	{
		bNeeded = ! m_filteredTags.Empty();
		bNeeded = bNeeded && !m_filter.Empty();
		bNeeded = bNeeded && focused && (focused == m_wxTagsList || focused == m_wxEditTag);
	}
	else
		bNeeded = true;
    
    wxSize size = GetSize();
    size.SetHeight(bNeeded ? 428 : 92);
    SetSize(size);
    
    if (m_wxTagsList->IsEnabled() != bNeeded)
        m_wxTagsList->Enable(bNeeded);
}

void CEdTagMiniEditor::OnOK( wxCommandEvent& event )
{
	m_tags.Clear();

    TDynArray<String> tags;
    String(m_wxEditTag->GetValue().wc_str()).Slice(tags, TXT(";"));
    
    TDynArray<String>::iterator tag_curr = tags.Begin(),
                                tag_last = tags.End();
    for(; tag_curr != tag_last; ++tag_curr)
    {
        (*tag_curr).Trim();
        if ( ! tag_curr->Empty() )
        {
            m_tags.PushBackUnique( CName( *tag_curr ) );
            
			for ( TagProviderArray::iterator providerIter = m_availableTagsProviders.Begin();
				providerIter != m_availableTagsProviders.End(); ++providerIter )
			{
				(*providerIter)->RemeberTag( * tag_curr );
			}
        }
    }

	OnCloseCleanup();

	// Send to parent
	wxCommandEvent okPressedEvent( wxEVT_TAGEDITOR_OK );
	ProcessEvent( okPressedEvent );

	// Close window
	if ( IsModal() )
		EndModal( wxOK );
	else
		Destroy();
}

void CEdTagMiniEditor::OnCancel( wxCommandEvent& event )
{
	OnCloseCleanup();

	// Send to parent
	wxCommandEvent okPressedEvent( wxEVT_TAGEDITOR_CANCEL );
	ProcessEvent( okPressedEvent );

	// Close window
	if ( IsModal() )
		EndModal( wxCANCEL );
	else
		Destroy();
}

void CEdTagMiniEditor::OnClose( wxCloseEvent& event )
{
	wxCommandEvent fakeEvent;
	if ( m_saveOnClose == false )
	{
		OnCancel( fakeEvent );
	}
	else
	{
		// Send to parent
		wxCommandEvent okPressedEvent( wxEVT_TAGEDITOR_OK );
		ProcessEvent( okPressedEvent );
	} 
}

void CEdTagMiniEditor::OnEditTagSetFocus( wxFocusEvent& event )
{
    ShowTagTreeIfNeeded();

    if (m_clearEditTagSelection)
    {
        m_wxEditTag->SetSelection(m_wxEditTag->GetValue().Length(), m_wxEditTag->GetValue().Length());
        m_clearEditTagSelection = false;
    }

	event.Skip();
}

void CEdTagMiniEditor::OnEditTagKillFocus( wxFocusEvent& event )
{
    ShowTagTreeIfNeeded();
    event.Skip();
}

void CEdTagMiniEditor::OnEditTagChanged( wxCommandEvent& event )
{
    if (m_lockOnEditTagChanged) return;

    UpdateTagTree();
    if (!m_lockTagCompletion && !m_filteredTags.Empty() && m_filter.GetLength())
    {
		m_isSelectionFromKeyboard = true;
        ChangeTreeItemSelection( true );
    }
    m_lockTagCompletion = false;

	event.Skip();
}

void CEdTagMiniEditor::OnEditTagKeyDown( wxKeyEvent& event )
{
    if (event.GetKeyCode() == WXK_UP && m_blockKeyboardArrows == false && IsShown() )
    {
		m_isSelectionFromKeyboard = true;
		ChangeTreeItemSelection( false );
    }
    else
    if (event.GetKeyCode() == WXK_DOWN && m_blockKeyboardArrows == false && IsShown() )
    {
		m_isSelectionFromKeyboard = true;
		ChangeTreeItemSelection( true );
    }
    else
    if (event.GetKeyCode() == WXK_ESCAPE && IsShown())
    {
        wxCommandEvent fakeEvent;
        OnCancel(fakeEvent);
    }
	else if ( event.GetKeyCode() == WXK_RETURN && IsShown() )
	{
		wxCommandEvent fakeEvent( wxEVT_COMMAND_TEXT_ENTER );
		fakeEvent.SetEventObject( event.GetEventObject() );
		
		wxStaticCast( event.GetEventObject(), wxWindow )->GetEventHandler()->ProcessEvent( fakeEvent );
	}
    else
    {
		Bool wasVisible = IsShown();
        if ( event.GetKeyCode() == WXK_DELETE || event.GetKeyCode() == WXK_BACK )
		{
            m_lockTagCompletion = true;
		}
		else if ( event.GetKeyCode() >= 32 && event.GetKeyCode() < 127 )
		{
			if ( wasVisible == false )
			{
				m_clearEditTagSelection = false;
				Show();
			}
			
			if ( m_allowOnlyValidKeyPresses == true )
			{
				wxString inputCharacterString = event.GetUnicodeKey();
				String filterString = ExtractFilter() + inputCharacterString.wc_str();
				filterString.TrimLeft();
				filterString.MakeLower();
				Uint32 validTags = 0;

				for ( Uint32 i = 0; i < m_wxTagsList->GetCount(); ++i )
				{
					TPair< String, Uint32 > * tagPtr = static_cast< TPair< String, Uint32 > * >( m_wxTagsList->GetClientData( i ) );
					ASSERT( tagPtr );
					const String & tag = tagPtr->m_first;

					if (tag.BeginsWith( filterString ) )
					{
						++validTags;
						break;
					}
				}

				Bool canAppendCharacter = ( validTags != 0 );

				if ( canAppendCharacter == false )
				{
					return;
				}
			}
		}

		if ( wasVisible == true )
		{
			m_filterTimer.Start( 100, true );
		}
		

        event.Skip();
    }
}

void CEdTagMiniEditor::OnEditTagMouseDown( wxMouseEvent& event )
{
    m_filterTimer.Start( 100, true );
    event.Skip();
}

void CEdTagMiniEditor::OnTagContext( wxMouseEvent& event )
{
	m_lastMousePosition = m_wxTagsList->ScreenToClient(	wxGetMousePosition() );
    wxMenu menu;
    menu.Append(ID_REMOVE_HISTORY_TAG, TXT("Forget tag"), wxEmptyString, false );
    menu.Connect( ID_REMOVE_HISTORY_TAG, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdTagMiniEditor::OnForgetTag), NULL, this );
    PopupMenu( &menu );
}

void CEdTagMiniEditor::OnForgetTag( wxCommandEvent& event )
{
	class wxListBoxUglyHackToGetItemAtPointBack : public wxListBox
	{
	public:
		virtual int DoListHitTest(const wxPoint& point) const
		{ return wxListBox::DoListHitTest(point); }
	};

	// Get selected item
	wxListBoxUglyHackToGetItemAtPointBack *pListBox = static_cast<wxListBoxUglyHackToGetItemAtPointBack*>( m_wxTagsList );
	Int32 selId = pListBox->DoListHitTest( m_lastMousePosition );
	//Int32 selId = m_wxTagsList->GetSelection();
	if ( selId < 0 )
		return;

	TPair< String, Uint32 > * tagPtr = static_cast< TPair< String, Uint32 > * >( m_wxTagsList->GetClientData( selId ) );
	ASSERT( tagPtr );
	const String & tag = tagPtr->m_first;

	TDynArray<CTagListProvider *>::iterator it;
	for (it = m_availableTagsProviders.Begin(); it != m_availableTagsProviders.End(); ++it)
	{
		(*it)->ForgetTag( tag );
	}

	// Update tag lists
	m_filter = TXT("=====UNDEFINED=====");
	UpdateTagTree();
}

void CEdTagMiniEditor::OnTagSelected(wxCommandEvent& event )
{
	// Get selected item
	Int32 selId = m_wxTagsList->GetSelection();
	if ( selId < 0 )
		return;

	TPair< String, Uint32 > * tagPtr = static_cast< TPair< String, Uint32 > * >( m_wxTagsList->GetClientData( selId ) );
	ASSERT( tagPtr );
	String text = tagPtr->m_first;
	if ( !text.Empty() )
	{
		Int32 carretPos = m_wxEditTag->GetInsertionPoint();
		String tags = m_wxEditTag->GetValue().wc_str();

		String leftString  = tags.LeftString(carretPos).StringBefore(TXT(";"), true);
		leftString.Trim();
		if (!leftString.Empty())
			leftString += TXT("; ");

		String rightString = tags.MidString(carretPos).StringAfter(TXT(";"));
		rightString.Trim();
		if (!rightString.Empty())
			rightString += rightString.EndsWith(TXT(";"))
			? TXT(" ")
			: TXT("; ");

		text = leftString + text;
		Int32 carretPosEnd = text.GetLength();
		text += TXT("; ") + rightString;

		m_lockOnEditTagChanged = true;
		m_wxEditTag->SetValue(text.AsChar());

		m_wxEditTag->SetSelection(carretPosEnd, carretPos);
		
		m_wxEditTag->SetFocus();
		m_lockOnEditTagChanged = false;
	}
	else
	{
		m_wxEditTag->SetFocus();
	}

	m_isSelectionFromKeyboard = false;
}

void CEdTagMiniEditor::OnTagDoubleClicked(wxCommandEvent& event )
{
	// Get selected item
	Int32 selId = m_wxTagsList->GetSelection();
	if ( selId < 0 )
		return;

	wxString text = m_wxEditTag->GetValue();
	m_wxEditTag->SetSelection(text.Length(), text.Length());
	m_wxEditTag->SetFocus();
	UpdateTagTree();

	// Send to parent
	wxCommandEvent selectEvent( wxEVT_TAGEDITOR_SELECT );
	ProcessEvent( selectEvent );
}

void CEdTagMiniEditor::SetTagListProviders( const TDynArray< CTagListProvider* > updaters, Bool providersWillBeDisposedExternally )
{
	if ( ! m_providersWillBeDisposedExternally )
	{
		while ( m_availableTagsProviders.Empty() == false )
		{
			delete m_availableTagsProviders.PopBackFast();
		}
	}

	m_availableTagsProviders            = updaters;
	m_providersWillBeDisposedExternally = providersWillBeDisposedExternally;

	m_filter = TXT("=====UNDEFINED=====");
	UpdateTagTree();
}

void CEdTagMiniEditor::ChangeTreeItemSelection( Bool shouldSelectNext )
{
	if ( m_wxTagsList->GetCount() == 0 )
		return;
	
	Int32 selId     = m_wxTagsList->GetSelection();
	Int32 itemCount = m_wxTagsList->GetCount();

	if ( shouldSelectNext == true )
	{
		selId = ( selId + 1 ) % itemCount;
	}
	else
	{
		--selId;
		if ( selId < 0 )
			selId = itemCount-1;
	}
	
	m_wxTagsList->EnsureVisible( selId );
	m_wxTagsList->SetSelection( selId );
	wxCommandEvent dummyEvent;
	OnTagSelected( dummyEvent );
}

String CEdTagMiniEditor::ExtractFilter()
{
	if ( m_wxEditTag->GetForegroundColour() == *wxLIGHT_GREY && m_wxEditTag->GetValue() == wxT( "Descriptors" ) )
	{
		return TXT( "" );
	}

	// Extract filter
	Int32 carretPos = m_wxEditTag->GetInsertionPoint();
	String filter = m_wxEditTag->GetValue().wc_str();

	String leftString  = filter.LeftString(carretPos);
	if ( leftString.ContainsSubstring(TXT(";") ) )
	{
		leftString = leftString.StringAfter(TXT(";"), true);
	}
	filter = leftString;// + rightString;

	return filter;
}
