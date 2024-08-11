#include "build.h"
#include "dialogEditorSpeakerInput.h"
#include "../../common/game/storyScene.h"

wxIMPLEMENT_CLASS( CEdStorySceneLineSpeakerInput, CEdTextControl );

CEdStorySceneLineSpeakerInput::CEdStorySceneLineSpeakerInput( CStoryScene* scene, wxWindow* parent, long style )
	: CEdTextControl( parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, style )
	, m_scene( scene )
{
	ASSERT( m_scene != NULL, TXT( "Cannot create CEdStorySceneLineSpeakerInput without valid CStoryScene" ) );

	Bind( wxEVT_EDTC_STRING_UPDATED, &CEdStorySceneLineSpeakerInput::OnStringUpdated, this );
	Bind( wxEVT_KILL_FOCUS, &CEdStorySceneLineSpeakerInput::OnAutoCompleteBoxFocusLost, this );
	Bind( wxEVT_CLOSE_WINDOW, &CEdStorySceneLineSpeakerInput::OnAutoCompleteBoxFocusLost, this );

	Bind( wxEVT_CHAR, &CEdStorySceneLineSpeakerInput::OnCharPressed, this );
	Bind( wxEVT_MOUSEWHEEL, &CEdStorySceneLineSpeakerInput::OnAutoCompleteMouseWheel, this );
}

CEdStorySceneLineSpeakerInput::~CEdStorySceneLineSpeakerInput()
{

}

wxListBox* CEdStorySceneLineSpeakerInput::CreateNewAutoCompleteBox()
{
	m_isDeleting = false;
	m_lastCursorPosition = 1;

	m_autoCompleteDialog = new wxDialog
	(
		this,
		wxID_ANY,
		wxEmptyString,
		wxDefaultPosition,
		wxSize( 200, 100 ),
		wxSTAY_ON_TOP | wxBORDER_NONE
	);
	
	m_autoCompleteDialog->SetSizer( new wxBoxSizer( wxVERTICAL ) );

	m_autoCompleteBox = new wxListBox
	(
		m_autoCompleteDialog,
		wxID_ANY,
		wxDefaultPosition,
		wxSize( 200, 100 ),
		0,
		NULL,
		(
			wxLB_NEEDED_SB |
			wxLB_SINGLE |
			wxLB_SORT |
			wxRAISED_BORDER |
			wxWANTS_CHARS
		)
	);

	m_autoCompleteBox->Bind( wxEVT_COMMAND_LISTBOX_SELECTED, &CEdStorySceneLineSpeakerInput::OnAutoCompleteBoxSelectionChange, this );
	m_autoCompleteBox->Bind( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, &CEdStorySceneLineSpeakerInput::OnAutoCompleteBoxSelectionChange, this );
	m_autoCompleteBox->Bind( wxEVT_MOUSEWHEEL, &CEdStorySceneLineSpeakerInput::OnAutoCompleteMouseWheel, this );

	wxPoint boxPosition = CalculateAutoCompleteBoxPosition();
	m_autoCompleteDialog->SetPosition( boxPosition );

	m_autoCompleteDialog->GetSizer()->Add( m_autoCompleteBox, 1, wxEXPAND|wxALL );
	m_autoCompleteDialog->Layout();
	m_autoCompleteDialog->Show();
	
	SetFocus();

	RefreshAutoCompleteBoxContents();

	return m_autoCompleteBox;
}

void CEdStorySceneLineSpeakerInput::CloseAutoComplete()
{
	wxString currentInputValue = GetValue();
	if ( m_autoCompleteBox->GetStrings().Index( currentInputValue ) == wxNOT_FOUND && m_autoCompleteBox->GetCount() > 0 )
	{
		m_autoCompleteBox->Select( 0 );
		TriggerAutoCompleteBoxSelectionEvent();
	}

	m_autoCompleteBox->Unbind( wxEVT_COMMAND_LISTBOX_SELECTED, &CEdStorySceneLineSpeakerInput::OnAutoCompleteBoxSelectionChange, this );
	m_autoCompleteBox->Unbind( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, &CEdStorySceneLineSpeakerInput::OnAutoCompleteBoxSelectionChange, this );
	m_autoCompleteBox->Unbind( wxEVT_MOUSEWHEEL, &CEdStorySceneLineSpeakerInput::OnAutoCompleteMouseWheel, this );

	m_autoCompleteBox->Hide();
	m_autoCompleteBox->Destroy();
	m_autoCompleteBox = NULL;

	m_autoCompleteDialog->Hide();
	m_autoCompleteDialog->Destroy();
	m_autoCompleteDialog = NULL;
}

void CEdStorySceneLineSpeakerInput::FilterContents( const wxString& prefix, wxArrayString& filteredContents )
{
	filteredContents.Clear();
	wxArrayString autoCompleteContents;
	GetAutoCompleteContents( autoCompleteContents );

	for ( Uint32 i = 0; i < autoCompleteContents.GetCount(); ++i )
	{
		if ( autoCompleteContents[ i ].Lower().StartsWith( prefix.Lower().wc_str() ) == true )
		{
			filteredContents.Add( autoCompleteContents[ i ] );
		}
	}
}

wxPoint CEdStorySceneLineSpeakerInput::CalculateAutoCompleteBoxPosition()
{
	ASSERT( m_autoCompleteBox != NULL );

	wxPoint boxPosition = GetScreenPosition();

	Int32 autoCompleteBoxHeight = m_autoCompleteBox->GetSize().y;
	if ( boxPosition.y + autoCompleteBoxHeight < GetScreenRect().GetBottom() - 10 )
	{
		boxPosition.y += GetCharHeight();
	}
	else
	{
		boxPosition.y -= autoCompleteBoxHeight;
	}
	boxPosition.x += GetSize().x / 2;

	return boxPosition;
}

void CEdStorySceneLineSpeakerInput::ChangeAutoCompleteBoxSelection( Bool selectNext )
{
	if ( m_autoCompleteBox == NULL || m_autoCompleteBox->IsShown() == false )
	{
		return;
	}

	Uint32 selected = m_autoCompleteBox->GetSelection();

	Uint32 indexToSelect = selected;
	if ( selected >= 0 && selected < m_autoCompleteBox->GetCount() )
	{
		if ( selectNext == true && selected < m_autoCompleteBox->GetCount() - 1 )
		{
			indexToSelect = selected + 1;
		}
		else if ( selectNext == false && selected > 0 )
		{
			indexToSelect = selected - 1;
		}	
	}
	else
	{
		if ( selectNext == true )
		{
			indexToSelect = 0;
		}
		else
		{
			return;
		}
	}

	m_autoCompleteBox->Select( indexToSelect );

	TriggerAutoCompleteBoxSelectionEvent();
}

void CEdStorySceneLineSpeakerInput::RefreshAutoCompleteBoxContents()
{
	if ( m_autoCompleteBox == NULL )
	{
		return;
	}

	wxArrayString filteredContents;
	FilterContents( GetRange( 0, GetInsertionPoint() ), filteredContents );
	if ( filteredContents.IsEmpty() == true )
	{
		m_autoCompleteBox->Hide();
	}
	else
	{
		m_autoCompleteBox->Set( filteredContents );
		m_autoCompleteBox->Show();

		if ( m_isDeleting == false )
		{
			wxString textFieldValue = GetValue();
			Int32 textFieldValueIndexInContext = m_autoCompleteBox->FindString( textFieldValue );
			
			m_autoCompleteBox->SetSelection
			( 
				textFieldValueIndexInContext == wxNOT_FOUND ? 0 : textFieldValueIndexInContext,
				true
			);

			TriggerAutoCompleteBoxSelectionEvent();
		}
	}
}

void CEdStorySceneLineSpeakerInput::TriggerAutoCompleteBoxSelectionEvent()
{
	wxCommandEvent selectEvent( wxEVT_COMMAND_LISTBOX_SELECTED );
	selectEvent.SetString( m_autoCompleteBox->GetString( m_autoCompleteBox->GetSelection() ) );
	m_autoCompleteBox->GetEventHandler()->ProcessEvent( selectEvent );
}

void CEdStorySceneLineSpeakerInput::OnStringUpdated( wxCommandEvent& event )
{
	wxTextCtrl* eventSource = static_cast<wxTextCtrl*>( event.GetEventObject() );
	if ( m_autoCompleteBox == NULL && eventSource->IsModified() == true )
	{
		CreateNewAutoCompleteBox();
	}
	else
	{
		RefreshAutoCompleteBoxContents();
	}

	event.Skip();
}

void CEdStorySceneLineSpeakerInput::OnAutoCompleteBoxFocusLost( wxEvent& event )
{
	wxWindow* focusedWindow = wxWindow::FindFocus();
	if
	(
		m_autoCompleteBox != NULL &&
		focusedWindow != m_autoCompleteBox &&
		focusedWindow != m_autoCompleteDialog &&
		focusedWindow != this
	)
	{
		CloseAutoComplete();
	}
	SetSelection(0,0);
	event.Skip();
}

void CEdStorySceneLineSpeakerInput::OnCharPressed( wxKeyEvent& event )
{
	if ( m_autoCompleteBox == NULL || m_autoCompleteBox->IsShown() == false )
	{
		m_lastCursorPosition = GetInsertionPoint();

		if ( IsAlwaysAllowedKey( event.GetKeyCode() ) == true || CanAppendCharacter( event.GetUnicodeKey() ) == true )
		{
			event.Skip();
		}
		else
		{
			SetInsertionPoint( 0 );
			CreateNewAutoCompleteBox();
		}
		return;
	}

	m_isDeleting = false;
	if ( event.GetKeyCode() == WXK_DOWN || event.GetKeyCode() == WXK_UP )
	{
		SetForegroundColour( *wxBLACK );
		ChangeAutoCompleteBoxSelection( event.GetKeyCode() == WXK_DOWN );
		return;
	}
	if ( event.GetKeyCode() == WXK_BACK || event.GetKeyCode() == WXK_DELETE )
	{
		m_isDeleting = true;
		long caretPosition = GetInsertionPoint();
		if ( caretPosition > 0 )
		{
			m_lastCursorPosition = caretPosition - 1;
		}
	}

	if ( event.GetKeyCode() == WXK_RETURN ||  m_isDeleting == true  || CanAppendCharacter( event.GetUnicodeKey() ) == true )
	{
		event.Skip();
	}
}

void CEdStorySceneLineSpeakerInput::OnAutoCompleteBoxSelectionChange( wxCommandEvent& event )
{
	m_lastCursorPosition = GetInsertionPoint();
	ChangeValue( event.GetString() );
	
	wxCommandEvent fieldChangeEvent( event );
	fieldChangeEvent.SetEventObject( this );
	AddPendingEvent( fieldChangeEvent );

	SetFocus();

	SetSelection( m_lastCursorPosition, GetLastPosition() );
}

Bool CEdStorySceneLineSpeakerInput::CanAppendCharacter( wxChar character )
{
	if ( character == '$' && ( m_autoCompleteBox == NULL || m_lastCursorPosition == 0 ) )
	{
		//m_lastCursorPosition += 1;
		return true;
	}


	Uint32 testedCharactersIndex = m_lastCursorPosition;
	wxString characterString = character;

	wxArrayString availableStrings;
	
	if ( m_autoCompleteBox != NULL )
	{
		availableStrings = m_autoCompleteBox->GetStrings();
	}
	else
	{
		GetAutoCompleteContents( availableStrings );
	}

	for ( Uint32 i = 0; i < availableStrings.GetCount(); ++i )
	{
		wxString availableStringCharacterAtTestedIndex
			= availableStrings[ i ].Mid( testedCharactersIndex, 1 );

		Int32 characterCompareRelation 
			= characterString.CmpNoCase( availableStringCharacterAtTestedIndex );
		if ( characterCompareRelation == 0 )
		{
			return true;
		}
		else if ( characterCompareRelation < 0 )
		{
			return false;
		}
	}
	
	
	return false;
}

Bool CEdStorySceneLineSpeakerInput::IsAlwaysAllowedKey( Int32 keycode )
{
	return keycode == WXK_DOWN
		|| keycode == WXK_UP
		|| keycode == WXK_LEFT
		|| keycode == WXK_RIGHT
		|| keycode == WXK_BACK
		|| keycode == WXK_DELETE
		|| keycode == WXK_INSERT
		|| keycode == WXK_HOME
		|| keycode == WXK_END
		|| keycode == WXK_PAGEDOWN
		|| keycode == WXK_PAGEUP
		|| keycode == WXK_RETURN
		|| keycode == WXK_TAB
		|| keycode == '$'
		|| keycode == 3
		|| keycode == 22
		|| keycode == 24;
}

void CEdStorySceneLineSpeakerInput::OnAutoCompleteMouseWheel( wxMouseEvent& event )
{
	if ( m_autoCompleteBox != NULL )
	{
		SetForegroundColour( *wxBLACK );
		ChangeAutoCompleteBoxSelection( event.GetWheelRotation() < 0 );
	}
	else
	{
		event.Skip();
	}
	
}

void CEdStorySceneLineSpeakerInput::GetAutoCompleteContents( wxArrayString& autoCompleteContents )
{
	ASSERT( m_scene != NULL );

	TDynArray< CName >	sceneActorNames;
	m_scene->CollectVoicetags( sceneActorNames, true );

	for ( Uint32 i = 0; i < sceneActorNames.Size(); ++i )
	{
		if ( sceneActorNames[ i ] == CName::NONE )
		{
			continue;
		}
		autoCompleteContents.Add( sceneActorNames[ i ].AsString().AsChar() );
	}
	autoCompleteContents.Sort();
}

Bool CEdStorySceneLineSpeakerInput::CustomArrowTraverseRule( wxKeyEvent & event )
{
	return ( event.GetKeyCode() == WXK_DOWN || event.GetKeyCode() == WXK_UP ) && IsAutoCompleteBoxOpened();
}

void CEdStorySceneLineSpeakingToInput::GetAutoCompleteContents( wxArrayString& autoCompleteContents )
{
	autoCompleteContents.Add( TXT("ALL") );
	CEdStorySceneLineSpeakerInput::GetAutoCompleteContents( autoCompleteContents );
}
