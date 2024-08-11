#include "build.h"

#include "textControl.h"

#include "wx/caret.h"

wxDEFINE_EVENT( wxEVT_EDTC_STRING_UPDATED, wxCommandEvent );

wxIMPLEMENT_CLASS( CEdTextControl, wxTextCtrlEx );

CEdTextControl::CEdTextControl()
:	m_hintDisplayed( false ),
	m_hintSet( false ),
	m_hintColour( 128, 128, 128 ),
	m_enterKeyWorkaroundEnabled( false )
{

}

CEdTextControl::CEdTextControl
(
	wxWindow* parent,
	wxWindowID id,
	const wxString& value,
	const wxPoint& pos,
	const wxSize& size,
	long style,
	const wxValidator& validator,
	const wxString& name
)
:	wxTextCtrlEx
	(
		parent,
		id,
		value,
		pos,
		size,
		style,
		validator,
		name
	),
	m_hintDisplayed( false ),
	m_hintSet( false ),
	m_hintColour( 128, 128, 128 ),
	m_enterKeyWorkaroundEnabled( false )
{
	Bind( wxEVT_CHAR, &CEdTextControl::OnChar, this );
	Bind( wxEVT_COMMAND_TEXT_COPY, &CEdTextControl::OnClipboard, this );
	Bind( wxEVT_COMMAND_TEXT_CUT, &CEdTextControl::OnClipboard, this );
	Bind( wxEVT_COMMAND_TEXT_PASTE, &CEdTextControl::OnClipboard, this );
}

CEdTextControl::~CEdTextControl()
{

}

void CEdTextControl::OnChar( wxKeyEvent& keyEvent )
{
	SendStringUpdatedEvent();

	keyEvent.Skip();
}

bool CEdTextControl::SetHint( const wxString& hint )
{
	HideHint();

	m_hintText = hint;

	if( m_hintSet )
	{
		if( hint.IsEmpty() )
		{
			Unbind( wxEVT_SET_FOCUS, &CEdTextControl::OnHintFocus, this );
			Unbind( wxEVT_KILL_FOCUS, &CEdTextControl::OnHintFocus, this );

			m_hintSet = false;
		}
	}
	else
	{
		if( !hint.IsEmpty() )
		{
			Bind( wxEVT_SET_FOCUS, &CEdTextControl::OnHintFocus, this );
			Bind( wxEVT_KILL_FOCUS, &CEdTextControl::OnHintFocus, this );

			m_hintSet = true;
		}
	}

	DisplayHint();

	return true;
}

void CEdTextControl::SetValue( const wxString& value )
{
	HideHint();

	wxTextCtrl::SetValue( value );

	SendStringUpdatedEvent();

	DisplayHint();
}

void CEdTextControl::ChangeValue( const wxString& value )
{
	HideHint();

	wxTextCtrl::SetValue( value );

	DisplayHint();
}

void CEdTextControl::OnHintFocus( wxFocusEvent& event )
{
	if( event.GetEventType() == wxEVT_SET_FOCUS )
	{
		HideHint();
	}
	else if( event.GetEventType() == wxEVT_KILL_FOCUS )
	{
		DisplayHint();
	}

	event.Skip();
}

void CEdTextControl::DisplayHint()
{
	if( m_hintSet )
	{
		if( IsEmpty() )
		{
			if( !m_hintDisplayed )
			{
				m_otherColour = wxTextCtrl::GetForegroundColour();
				wxTextCtrl::SetForegroundColour( m_hintColour );
			}

			wxTextCtrl::ChangeValue( m_hintText );
			
			m_hintDisplayed = true;
		}
	}
}

void CEdTextControl::HideHint()
{
	if( m_hintDisplayed )
	{
		ASSERT( m_hintSet, TXT( "How is a hint displayed if it was never set?" ) );
		ASSERT( !wxTextCtrl::IsEmpty(), TXT( "If a hint is displayed then the control shouldn't be empty" ) );
		ASSERT( wxTextCtrl::GetValue() == m_hintText, TXT( "Value of the text control does not match the hint, which is currently displayed" ) );

		wxTextCtrl::SetForegroundColour( m_otherColour );

		wxTextCtrl::ChangeValue( wxEmptyString );

		m_hintDisplayed = false;
	}
}

wxString CEdTextControl::GetValue() const
{
	if( m_hintDisplayed )
	{
		return wxEmptyString;
	}

	return wxTextCtrl::GetValue();
}

bool CEdTextControl::IsEmpty() const
{
	if( !wxTextCtrl::IsEmpty() )
	{
		if( wxTextCtrl::GetValue() == m_hintText )
		{
			ASSERT( m_hintDisplayed );
			return true;
		}

		return false;
	}

	return true;
}

bool CEdTextControl::MSWShouldPreProcessMessage( WXMSG* msg )
{
	if( m_enterKeyWorkaroundEnabled )
	{
		return wxTextCtrlEx::MSWShouldPreProcessMessage( msg );
	}

	return wxTextCtrl::MSWShouldPreProcessMessage( msg );
}

void CEdTextControl::OnClipboard( wxClipboardTextEvent& event )
{
	SendStringUpdatedEvent();

	event.Skip();
}

void CEdTextControl::SendStringUpdatedEvent()
{
	wxCommandEvent* event = new wxCommandEvent( wxEVT_EDTC_STRING_UPDATED, GetId() );
	event->SetEventObject( this );
	GetEventHandler()->QueueEvent( event );
}


