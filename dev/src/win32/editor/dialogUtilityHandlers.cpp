#include "build.h"
#include "dialogUtilityHandlers.h"

#include "dialogEditorPage.h"
#include "textControl.h"
#include "..\..\common\engine\localizedContent.h"

CEdDialogScrollOnFocusHandler::CEdDialogScrollOnFocusHandler( CEdSceneEditorScreenplayPanel* dialogEditor )
	: m_storySceneEditor( dialogEditor )
{
}

void CEdDialogScrollOnFocusHandler::ConnectTo( wxWindow* window )
{
	window->Connect( wxEVT_SET_FOCUS, 
		wxFocusEventHandler( CEdDialogScrollOnFocusHandler::OnSetFocus ), NULL, this );
	window->Connect( wxEVT_CHAR, 
		wxKeyEventHandler( CEdDialogScrollOnFocusHandler::OnCharPressed ), NULL, this );
}

void CEdDialogScrollOnFocusHandler::OnSetFocus( wxFocusEvent& event )
{
	wxWindow* eventSourceWindow = static_cast<wxWindow*>( event.GetEventObject() );

	wxScrolledWindow* scrollWindow = m_storySceneEditor->GetDialogTextPanel();

	scrollWindow->Freeze();

	
	
	int stepx, stepy;
	int startx, starty;
	
	wxSize view( scrollWindow->GetClientSize() );
	scrollWindow->GetScrollPixelsPerUnit(&stepx, &stepy);
	scrollWindow->GetViewStart( &startx, &starty );

	wxRect eventSourceWindowRect( 
		scrollWindow->ScreenToClient( eventSourceWindow->GetScreenPosition() ), 
		eventSourceWindow->GetSize() );
	
	Bool shouldScroll = false;

	int diff = 0;

	if ( eventSourceWindowRect.GetTop() < 0 )
	{
		diff = eventSourceWindowRect.GetTop();
		shouldScroll = true;
	}
	else if ( eventSourceWindowRect.GetBottom() > view.y )
	{
		diff = eventSourceWindowRect.GetBottom() - view.y + 1;
		diff += stepy - 1;
		shouldScroll = true;
	}

	if ( shouldScroll == true && stepy > 0 )
	{
		starty = (starty * stepy + diff) / stepy;
		scrollWindow->Scroll( startx, starty );
	}
		
	scrollWindow->Thaw();

	event.Skip();
}

void CEdDialogScrollOnFocusHandler::OnCharPressed( wxKeyEvent& event )
{
	wxWindow* eventSource = static_cast<wxWindow*>( event.GetEventObject() );

	wxFocusEvent focusEvent( wxEVT_SET_FOCUS );
	focusEvent.SetEventObject( eventSource );
	OnSetFocus( focusEvent );

	event.Skip();
}

void CEdDialogScrollOnFocusHandler::OnChildSetFocus( wxChildFocusEvent& event )
{
	wxFocusEvent focusEvent( wxEVT_SET_FOCUS );
	focusEvent.SetEventObject( event.GetEventObject() );
	OnSetFocus( focusEvent );	
}


void CEdDialogArrowTraverseHandler::ConnectTo( wxWindow* window )
{
	window->Connect( wxEVT_KEY_DOWN, wxKeyEventHandler( CEdDialogArrowTraverseHandler::OnCharPressed ), NULL, this );
	window->SetClientData( NULL );
}

void CEdDialogArrowTraverseHandler::OnCharPressed( wxKeyEvent& event )
{
	wxWindow* eventSource = static_cast<wxWindow*>( event.GetEventObject() );

	if ( ShouldIgnoreHandlingEvent( eventSource, event ) == true )
	{
		event.Skip();
		return;
	}

	Bool isNavigateUpAllowed = true;
	Bool isNavigateDownAllowed = true;
	wxTextCtrl* eventSourceTextCtrl = wxDynamicCast( eventSource, wxTextCtrl );
	if ( eventSourceTextCtrl && ( wxIsCtrlDown() == true && wxIsShiftDown() == true ) == false )
	{
		Int32 numberOfTextCtrlLines = eventSourceTextCtrl->GetNumberOfLines();
		if ( eventSourceTextCtrl->IsMultiLine() == true && numberOfTextCtrlLines > 1 )
		{
			long currentTextCtrlPosition = eventSourceTextCtrl->GetInsertionPoint();
			long currentTextCtrlColumn, currentTextCtrlLine;
			eventSourceTextCtrl->PositionToXY( 
				currentTextCtrlPosition, &currentTextCtrlColumn, &currentTextCtrlLine );
			if ( currentTextCtrlLine != 0 )
			{
				isNavigateUpAllowed = false;
			}
			if ( currentTextCtrlLine != numberOfTextCtrlLines - 1 )
			{
				isNavigateDownAllowed = false;
			}
		}
		
	}

	Bool navigationSucceded = false;
	if ( event.GetKeyCode() == WXK_DOWN && isNavigateDownAllowed == true ||
		event.GetKeyCode() == WXK_RETURN && eventSourceTextCtrl && eventSourceTextCtrl->IsMultiLine() == false )
	{
		navigationSucceded = eventSource->Navigate( wxNavigationKeyEvent::IsForward );
	}
	else if ( event.GetKeyCode() == WXK_UP && isNavigateUpAllowed == true )
	{
		navigationSucceded = eventSource->Navigate( wxNavigationKeyEvent::IsBackward );
	}
	
	if ( navigationSucceded == false )
	{
		event.Skip();
	}
}

bool CEdDialogArrowTraverseHandler::ShouldIgnoreHandlingEvent( wxWindow* eventSource, wxKeyEvent &event )
{
	Bool isInSelectionMode = wxIsCtrlDown() == true || wxIsShiftDown() == true;
	Bool isNotTraverseKey = ( event.GetKeyCode() != WXK_DOWN && event.GetKeyCode() != WXK_UP && event.GetKeyCode() != WXK_RETURN );

	CEdTextControl * textControl =  wxDynamicCast( eventSource, CEdTextControl );
	Bool customScenario = textControl && textControl->CustomArrowTraverseRule( event );

	return isNotTraverseKey || isInSelectionMode || customScenario;
}

void CEdDialogCaretOnFocusHandler::ConnectTo( wxWindow* window )
{
	if ( window->IsKindOf( CLASSINFO( wxTextCtrl ) ) == true )
	{
		window->Connect( wxEVT_SET_FOCUS, 
			wxFocusEventHandler( CEdDialogCaretOnFocusHandler::OnFocus ), NULL, this );
		
		wxTextCtrl* textField = wxStaticCast( window, wxTextCtrl );
		if ( textField->IsMultiLine() == true )
		{
			textField->Connect( wxEVT_LEFT_DOWN, 
				wxMouseEventHandler( CEdDialogCaretOnFocusHandler::OnMultilineMouseDown ), NULL, this );
			textField->Connect( wxEVT_LEFT_UP, 
				wxMouseEventHandler( CEdDialogCaretOnFocusHandler::OnMultilineMouseUp ), NULL, this );
		}
	}
}

void CEdDialogCaretOnFocusHandler::OnFocus( wxFocusEvent& event )
{
	wxTextCtrl* eventSource = static_cast<wxTextCtrl*>( event.GetEventObject() );
	
	if ( eventSource->GetForegroundColour() != *wxLIGHT_GREY )
	{
		eventSource->SetInsertionPointEnd();
	}
	
	
	event.Skip();
}

void CEdDialogCaretOnFocusHandler::OnMultilineMouseDown( wxMouseEvent& event )
{
	wxTextCtrl* eventSource = static_cast<wxTextCtrl*>( event.GetEventObject() );
	if ( eventSource != wxWindow::FindFocus() )
	{
		m_manualFocusOnMouseClick = true;
		eventSource->SetFocus();
	}
	else
	{
		event.Skip();
	}
}

void CEdDialogCaretOnFocusHandler::OnMultilineMouseUp( wxMouseEvent& event )
{
	if ( m_manualFocusOnMouseClick == true )
	{
		m_manualFocusOnMouseClick = false;
	}
	else
	{
		event.Skip();
	}
}

void CEdDialogHyperlinkHandler::ConnectTo( wxWindow* window )
{
	if ( window->IsKindOf( CLASSINFO( wxStaticText ) ) == false )
	{
		return;
	}

	window->Connect( wxEVT_LEFT_UP, 
		wxMouseEventHandler( CEdDialogHyperlinkHandler::OnLinkClick ), NULL, this );
	window->Connect( wxEVT_ENTER_WINDOW, 
		wxMouseEventHandler( CEdDialogHyperlinkHandler::OnMouseEnter ), NULL, this );
	window->Connect( wxEVT_LEAVE_WINDOW, 
		wxMouseEventHandler( CEdDialogHyperlinkHandler::OnMouseLeave ), NULL, this );
	window->SetCursor( wxCursor( wxCURSOR_HAND ) );
	window->SetForegroundColour( wxColour( 0, 0, 192 ) );
}

void CEdDialogHyperlinkHandler::OnLinkClick( wxMouseEvent& event )
{
	wxWindow* eventSource = static_cast<wxWindow*>( event.GetEventObject() );
	wxWindow* gotoWindow = static_cast<wxWindow*>( eventSource->GetClientData() );
	if ( gotoWindow != NULL )
	{
		gotoWindow->SetFocus();
	}
}

void CEdDialogHyperlinkHandler::OnMouseEnter( wxMouseEvent& event )
{
	event.Skip();
}

void CEdDialogHyperlinkHandler::OnMouseLeave( wxMouseEvent& event )
{
	event.Skip();
}

void CEdDialogAutoExpandHandler::ConnectTo( wxWindow* window )
{
	if ( window->IsKindOf( CLASSINFO( wxTextCtrl ) ) == false )
	{
		return;
	}

	if( window->IsKindOf( CLASSINFO( CEdTextControl ) ) )
	{
		window->Bind( wxEVT_EDTC_STRING_UPDATED, &CEdDialogAutoExpandHandler::OnTextChanged, this );
	}
	else
	{
		window->Bind( wxEVT_COMMAND_TEXT_UPDATED, &CEdDialogAutoExpandHandler::OnTextChanged, this );
	}
}

void CEdDialogAutoExpandHandler::OnTextChanged( wxCommandEvent& event )
{
	ASSERT( event.GetEventObject()->IsKindOf( CLASSINFO( wxTextCtrl ) ) );

	wxTextCtrl* eventSource = static_cast<wxTextCtrl*>( event.GetEventObject() );
	wxString text = eventSource->GetValue();

	wxSize windowSize = eventSource->GetSize();

	Int32 possibleLinesFromSize = eventSource->GetSize().y / eventSource->GetCharHeight();
	Int32 actualLines = eventSource->GetNumberOfLines();

	Int32 extraLines = 0;

	if ( possibleLinesFromSize - actualLines != extraLines 
		&& ( eventSource == wxWindow::FindFocus() || m_storySceneEditor->IsInitialized() == true ) ) 
	{
		// Verify high actual lines number
		if ( actualLines > possibleLinesFromSize + 0 )
		{
			for ( Int32 i = 0; i < actualLines; ++i )
			{
				if ( eventSource->GetLineLength( i ) <= 1 )
				{
					actualLines = i + 1;
					break;
				}
			}
		}

		m_storySceneEditor->Freeze();

		possibleLinesFromSize = max( possibleLinesFromSize, 1 );
		//Int32 newHeight = ( eventSource->GetSize().y / possibleLinesFromSize ) * ( actualLines + extraLines );
		Int32 newHeight = eventSource->GetCharHeight() * ( actualLines + extraLines );
		newHeight += 0.25f * eventSource->GetCharHeight();
		eventSource->SetMinSize( wxSize( -1, newHeight ) );
		eventSource->SetMaxSize( wxSize( -1, newHeight ) );
		eventSource->SetSize( wxSize( -1, newHeight ) );

		m_storySceneEditor->GetDialogTextPanel()->SetVirtualSize( m_storySceneEditor->GetDialogTextPanel()->GetBestVirtualSize() );
		m_storySceneEditor->Thaw();
	}

	event.Skip();
}

void CEdDialogAutoExpandHandler::OnKeyDown( wxKeyEvent& event )
{
	wxTextCtrl* eventSource = static_cast<wxTextCtrl*>( event.GetEventObject() );

	event.Skip();
}

CEdDialogAutoExpandHandler::CEdDialogAutoExpandHandler( CEdSceneEditorScreenplayPanel* dialogEditor )
	: m_storySceneEditor( dialogEditor )
{
}

CEdDialogManualScriptScrollHandler::CEdDialogManualScriptScrollHandler( wxScrolledWindow* scrollWindow )
	: m_scrollWindow( scrollWindow )
{
}

void CEdDialogManualScriptScrollHandler::ConnectTo( wxWindow* window )
{
	window->Connect( wxEVT_MOUSEWHEEL, 
		wxMouseEventHandler( CEdDialogManualScriptScrollHandler::OnMouseScroll ), NULL, this );
}

void CEdDialogManualScriptScrollHandler::OnMouseScroll( wxMouseEvent& event )
{
	if ( event.GetEventType() == wxEVT_MOUSEWHEEL )
	{
		m_scrollWindow->HandleOnMouseWheel( event );
	}
}

void CEdPseudoButtonHandler::ConnectTo( wxWindow* window )
{
	if ( window->IsKindOf( CLASSINFO( wxStaticText ) ) == false 
		&& window->IsKindOf( CLASSINFO( wxStaticBitmap ) ) == false )
	{
		return;
	}

	window->Connect( wxEVT_LEFT_UP, 
		wxMouseEventHandler( CEdPseudoButtonHandler::OnMouseLeftButtonUp ), NULL, this );
	window->SetCursor( wxCursor( wxCURSOR_HAND ) );
}

void CEdPseudoButtonHandler::OnMouseLeftButtonUp( wxMouseEvent& event )
{
	wxWindow* eventSource = wxStaticCast( event.GetEventObject(), wxWindow );

	wxCommandEvent buttonEvent( wxEVT_COMMAND_BUTTON_CLICKED, eventSource->GetId() );
	buttonEvent.SetEventObject( eventSource );
	eventSource->GetEventHandler()->ProcessEvent( buttonEvent );
}


Bool CEdDialogTranslationHelperHandler::m_areTooltipsEnabled = false;

void CEdDialogTranslationHelperHandler::ConnectTo( wxWindow* window )
{
 	window->Connect( wxEVT_SET_FOCUS, 
 		wxFocusEventHandler( CEdDialogTranslationHelperHandler::OnFocusSet ), NULL, this );
	window->Connect( wxEVT_KILL_FOCUS, 
		wxFocusEventHandler( CEdDialogTranslationHelperHandler::OnFocusLost ), NULL, this );
}

void CEdDialogTranslationHelperHandler::OnFocusSet( wxFocusEvent& event )
{
	wxTextCtrl* eventSource = wxStaticCast( event.GetEventObject(), wxTextCtrl );
	
	String text = eventSource->GetValue().wc_str();
	if ( m_content->IsFallback( ) == true )
	{
		eventSource->ChangeValue( wxEmptyString );
	}
	if ( m_areTooltipsEnabled == true && m_tipWindow == NULL )
	{
		wxPoint position = eventSource->GetScreenPosition();
		position.y -= eventSource->GetSize().y + 5;
		m_tipWindow = new wxDialog( eventSource, wxID_ANY, wxEmptyString, 
			position, eventSource->GetSize(), wxSTAY_ON_TOP|wxBORDER_NONE );
		m_tipWindow->SetSizer( new wxBoxSizer( wxVERTICAL ) );
		m_tooltipField = new wxTextCtrl( m_tipWindow, wxID_ANY, 
			m_content->GetString().AsChar(), wxDefaultPosition, wxDefaultSize, 
			wxTE_MULTILINE|wxTE_READONLY|wxBORDER_RAISED );
		m_tooltipField->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_INFOBK ) );
		m_tooltipField->Enable( false );
		m_tipWindow->GetSizer()->Add( m_tooltipField, 1, wxEXPAND|wxALL );
		m_tipWindow->Layout();
		m_tipWindow->Show();
		eventSource->SetFocus();
	}
	event.Skip();
}

void CEdDialogTranslationHelperHandler::OnFocusLost( wxFocusEvent& event )
{
	wxWindow* focusedWindow = wxWindow::FindFocus();

	if ( event.GetEventObject() != NULL ) // It may always be NULL (according to wx docs)
	{
		// Re-setting the value here seems to break to-clipboard copying. Looks like some problem with the event ordering.
		// The value seems to be kept up-to date anyway, so this call was probably redundant.
// 		wxTextCtrl* eventSource = wxStaticCast( event.GetEventObject(), wxTextCtrl );
// 		eventSource->SetValue( m_content->GetString().AsChar() );
	}

	if ( m_tipWindow != NULL && focusedWindow != m_tipWindow && focusedWindow != m_tooltipField )
	{	
		m_tipWindow->Hide();
		m_tipWindow->Destroy();
		m_tipWindow = NULL;
		m_tooltipField = NULL;
	}
	event.Skip();
}