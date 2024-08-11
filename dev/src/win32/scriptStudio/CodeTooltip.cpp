#include "build.h"

#include "CodeTooltip.h"

#include "Scintilla.h"
#include "styledDocument.h"

IMPLEMENT_CLASS( CSSCodeToolTip, wxPopupWindow );

CSSCodeToolTip::CSSCodeToolTip( wxWindow* parent, const wxString& contents, wxPoint position /*= wxDefaultPosition */ )
:	wxPopupWindow( parent )
{
	wxBoxSizer* windowSizer = new wxBoxSizer( wxVERTICAL );
	SetSizer( windowSizer );

	wxPanel* mainPanel = new wxPanel( this );
	windowSizer->Add( mainPanel, 1, wxEXPAND, 0 );

	wxBoxSizer* panelSizer = new wxBoxSizer( wxVERTICAL );
	mainPanel->SetSizer( panelSizer );

	CSSStyledDocument* ctrl = new CSSStyledDocument( mainPanel );
	panelSizer->Add( ctrl, 1, wxEXPAND, 0 );

	ctrl->SetText( contents );

	int width = 0;
	for( int i = 0; i < ctrl->GetNumberOfLines(); ++i )
	{
		wxString lineText = ctrl->GetLine( i );

		int lineWidth = ctrl->TextWidth( STYLE_DEFAULT, lineText );

		if( lineWidth > width )
		{
			width = lineWidth;
		}
	}

	int height = ctrl->TextHeight( 0 ) * ctrl->GetNumberOfLines();

	wxSize windowSize = GetSize();
	windowSize.SetWidth( width );
	windowSize.SetHeight( height );
	SetSize( windowSize );

	ctrl->SetUseHorizontalScrollBar( false );
	ctrl->SetUseVerticalScrollBar( false );
	ctrl->SetEditable( false );

	// 4 = Number of margins defined by scintilla
	for( int i = 0; i < 4; ++i )
	{
		ctrl->SetMarginWidth( i, 0 );
	}

	ctrl->SetMargins( 0, 0 );

	SetPosition( position );

	// Causes a crash if we don't consume the event
	parent->Bind( wxEVT_MOVING, &CSSCodeToolTip::CloseMe, this );
	parent->Bind( wxEVT_MOTION, &CSSCodeToolTip::CloseMe, this );
	parent->Bind( wxEVT_KILL_FOCUS, &CSSCodeToolTip::CloseMe, this );
	parent->Bind( wxEVT_ENTER_WINDOW, &CSSCodeToolTip::CloseMe, this );
	parent->Bind( wxEVT_LEAVE_WINDOW, &CSSCodeToolTip::CloseMe, this );
	ctrl->Bind( wxEVT_MOTION, &CSSCodeToolTip::CloseMe, this );
	ctrl->Bind( wxEVT_KILL_FOCUS, &CSSCodeToolTip::CloseMe, this );
	parent->Bind( wxEVT_LEFT_DOWN, &CSSCodeToolTip::CloseMe, this );
	parent->Bind( wxEVT_RIGHT_DOWN, &CSSCodeToolTip::CloseMe, this );
	parent->Bind( wxEVT_MOUSEWHEEL, &CSSCodeToolTip::CloseMe, this );

	// These are fine, however
	parent->Bind( wxEVT_CHAR, &CSSCodeToolTip::CloseMeSkipEvent, this );
	 
	 
	Show();
}

CSSCodeToolTip::~CSSCodeToolTip()
{
}

void CSSCodeToolTip::CloseMeSkipEvent( wxEvent& event )
{
	event.Skip();

	CloseMe( event );
}

void CSSCodeToolTip::CloseMe( wxEvent& )
{
	Hide();
	Destroy();
}
