#include "build.h"
#include "tagEditorHandler.h"
#include "tagEditor.h"

#include "dialogEditorActions.h"
#include "tagListUpdater.h"
#include "dialogEditorPage.h"
#include "../../common/game/storySceneSection.h"

CEdTagMiniEditor* CEdTagEditorHandler::m_tagEditor = NULL;

CEdTagEditorHandler::CEdTagEditorHandler( CEdSceneEditorScreenplayPanel* dialogEditor, CTagListProvider* tagListUpdater )
	: m_storySceneEditor( dialogEditor )
	, m_handledWindow( NULL )
	, m_tagListProvider( tagListUpdater )
	, m_blockInputWhenEditorHidden( true )
{
}

void CEdTagEditorHandler::ConnectTo( wxWindow* window )
{
	window->Connect( wxEVT_SET_FOCUS, 
		wxFocusEventHandler( CEdTagEditorHandler::OnFocus ), NULL, this );
}

TagList CEdTagEditorHandler::MakeTagsFromString( String sTags )
{
	TagList tagList;

	TDynArray<String> tags;
	String(sTags).Slice(tags, TXT(";"));

	TDynArray<String>::iterator tagIter = tags.Begin(),
		tag_last = tags.End();
	for(; tagIter != tag_last; ++tagIter)
	{
		(*tagIter).Trim();
		if ( tagIter->Empty() == false && m_tagListProvider->IsTagAllowed( *tagIter ) == true )
		{
			tagList.AddTag( CName( *tagIter ) );
		}
	}

	return tagList;
}

void CEdTagEditorHandler::SaveEditedValues()
{
	//ASSERT( !"TODO" );

	CStorySceneSection* section 
		= Cast< CStorySceneSection >( static_cast< CObject* >( m_handledWindow->GetClientData() ) );
	wxTextCtrl* eventSource = static_cast< wxTextCtrl* >( m_handledWindow );

	if ( section != NULL )
	{
		String sTags = eventSource->GetValue().wc_str();

		section->SetTags( MakeTagsFromString( sTags ) );
	}
}

void CEdTagEditorHandler::CreateAndShowTagEditor( wxTextCtrl* eventSource )
{
	

	if ( m_tagEditor != NULL )
	{
		wxCommandEvent fakeEvent;
		m_tagEditor->OnCancel(fakeEvent);
		m_tagEditor = NULL;
	}

	if ( eventSource->GetClientData() == NULL )
	{
		return;
	}

	m_handledWindow = eventSource;
	
//	m_handledWindow->Freeze();

	CreateTagEditor(eventSource);

	//sm_tagEditor->SetBlockKeyboardArrows( true );
//	m_tagEditor->SetAllowOnlyValidKeyPresses( true );
//	m_tagEditor->SetSaveOnClose( true );
//	m_tagEditor->Hide();

//	m_handledWindow->Thaw();

	wxPoint topRightPosition = m_storySceneEditor->GetDialogTextPanel()->GetScreenRect().GetTopRight();
	wxPoint topLeftPosition = topRightPosition - wxPoint( m_tagEditor->GetSize().x, 0 );
	m_tagEditor->SetPosition( topLeftPosition );

	if ( m_tagListProvider != NULL )
	{
		TDynArray< CTagListProvider* > providers;
		providers.PushBack( m_tagListProvider );
		m_tagEditor->SetTagListProviders( providers, true );
	}

	m_tagEditor->Connect( wxEVT_TAGEDITOR_OK, wxCommandEventHandler( CEdTagEditorHandler::OnOk ), NULL, this );
	m_tagEditor->Connect( wxEVT_TAGEDITOR_CANCEL, wxCommandEventHandler( CEdTagEditorHandler::OnCancel ), NULL, this );
	eventSource->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( CEdTagEditorHandler::OnEnter ), NULL, this );
	eventSource->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( CEdTagEditorHandler::OnFocusLost ), NULL, this );
	eventSource->Connect( wxEVT_CHAR, wxKeyEventHandler( CEdTagEditorHandler::OnKeyDown ), NULL, this );
	eventSource->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( CEdTagEditorHandler::OnLeftDoubleClick ), NULL, this );

	m_tagEditor->Show();

	m_handledWindow->SetFocus();
}
void CEdTagEditorHandler::CreateTagEditor( wxTextCtrl* eventSource )
{
	CStorySceneSection* section = Cast< CStorySceneSection >( static_cast< CObject* >( m_handledWindow->GetClientData() ) );

	ASSERT( section != NULL );

	m_tagEditor = new CEdTagMiniEditor( eventSource, section->GetTags().GetTags(), eventSource );
}

void CEdTagEditorHandler::OnEnter( wxCommandEvent& event )
{
	if ( m_handledWindow == NULL )
	{
		return;
	}

	SaveEditedValues();
	if ( wxStaticCast( m_handledWindow, wxTextCtrl )->IsEmpty() )
	{
		//event.Skip();
	}

	m_handledWindow->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( CEdTagEditorHandler::OnEnter ), NULL, this );
	m_handledWindow->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( CEdTagEditorHandler::OnFocusLost ), NULL, this );

	wxCommandEvent commandEvent( wxEVT_COMMAND_MENU_SELECTED );
	commandEvent.SetId( wxID_STORYSCENEEDITOR_REFRESHDIALOG );
	m_handledWindow->GetEventHandler()->AddPendingEvent( commandEvent );

	m_handledWindow = NULL;

	if ( m_tagEditor != NULL )
	{
		wxCommandEvent fakeEvent;
		m_tagEditor->OnOK(fakeEvent);
		m_tagEditor = NULL;
	}
}

void CEdTagEditorHandler::OnCancel( wxCommandEvent& event )
{
	if ( m_handledWindow != NULL && m_tagEditor != NULL )
	{
		wxCommandEvent commandEvent( wxEVT_COMMAND_MENU_SELECTED );
		commandEvent.SetId( wxID_STORYSCENEEDITOR_REFRESHDIALOG );
		m_handledWindow->GetEventHandler()->AddPendingEvent( commandEvent );
	}

	m_handledWindow = NULL;
	m_tagEditor     = NULL;
}

void CEdTagEditorHandler::OnFocus( wxFocusEvent& event )
{
	if ( event.GetEventObject()->IsKindOf( CLASSINFO( wxTextCtrl ) ) == false )
	{
		event.Skip();
		return;
	}

	wxTextCtrl* eventSource = static_cast< wxTextCtrl* >( event.GetEventObject() );

	if ( m_handledWindow == eventSource )
	{
		return;
	}

	CreateAndShowTagEditor(eventSource);


	event.Skip();
}

void CEdTagEditorHandler::OnFocusLost( wxFocusEvent& event )
{
	wxWindow* focusedWindow = wxWindow::FindFocus();
	
	if ( m_handledWindow == NULL || focusedWindow == NULL )
	{
		event.Skip();
		return;
	}

	if ( m_handledWindow->FindWindow( focusedWindow->GetId() ) == NULL )
	{
		wxCommandEvent commandEvent;
		commandEvent.SetEventObject( m_handledWindow );
		OnEnter( commandEvent );
	}
}

void CEdTagEditorHandler::OnLeftDoubleClick( wxMouseEvent& event )
{
	if ( m_tagEditor == NULL )
	{
		CreateAndShowTagEditor( wxStaticCast( event.GetEventObject(), wxTextCtrl ) );
	}
	event.Skip();
}

void CEdTagEditorHandler::OnKeyDown( wxKeyEvent& event )
{
	Bool shouldIgnoreKeyPress = ( m_blockInputWhenEditorHidden == true ) 
		&& ( event.GetKeyCode() >= 32 && event.GetKeyCode() < 127 )
		&& ( m_tagEditor == NULL );

	event.Skip( shouldIgnoreKeyPress == false );
}

void CEdTagEditorHandler::OnOk( wxCommandEvent& event )
{
	if ( m_handledWindow )
	{
		wxCommandEvent dummyEvent( wxEVT_COMMAND_TEXT_ENTER );
		dummyEvent.SetEventObject( m_handledWindow );
		m_handledWindow->GetEventHandler()->ProcessEvent( dummyEvent );
	}

	m_handledWindow = NULL;
	m_tagEditor     = NULL;
}