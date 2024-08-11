/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "gotoDlg.h"

#include "events/eventGoto.h"

wxIMPLEMENT_CLASS( CSSGotoDialog, wxDialog );

BEGIN_EVENT_TABLE( CSSGotoDialog, wxDialog )
	EVT_BUTTON( wxID_OK, CSSGotoDialog::OnOK )
	EVT_CHAR_HOOK( CSSGotoDialog::OnKeyDown )
	EVT_SHOW( CSSGotoDialog::OnShow )
END_EVENT_TABLE()

CSSGotoDialog::CSSGotoDialog( wxWindow* parent, const wxString& file )
	: m_lblLineNumber( nullptr )
	, m_txtLineNumber( nullptr )
	, m_file( file )
	, m_maxLines( 0 )
{
	// Load layout from XRC
	wxXmlResource::Get()->LoadDialog( this, parent, wxT( "GotoDialog" ) );

	m_lblLineNumber	= XRCCTRL( *this, "lblLineNumber", wxStaticText );
	m_txtLineNumber	= XRCCTRL( *this, "txtLineNumber", wxTextCtrl );
}

CSSGotoDialog::~CSSGotoDialog()
{

}

void CSSGotoDialog::SetMaxLines( int maxLines )
{
	if ( maxLines >= 0 )
	{
		m_maxLines = maxLines;
		wxString msg;
		msg.Printf( wxT( "Line number ( 1, %d )" ), m_maxLines );
		m_lblLineNumber->SetLabel( msg );
	}
}

void CSSGotoDialog::OnShow( wxShowEvent& event )
{
	if( event.IsShown() )
	{
		m_txtLineNumber->SelectAll();
	}

	event.Skip();
}

void CSSGotoDialog::OnKeyDown( wxKeyEvent& event )
{
	if( event.GetKeyCode() == WXK_ESCAPE )
	{
		Close( true );
	}
	else if( event.GetKeyCode() == WXK_RETURN || event.GetKeyCode() == WXK_NUMPAD_ENTER )
	{
		Go();
	}
	else
	{
		event.Skip();
	}
}

void CSSGotoDialog::OnOK( wxCommandEvent& event )
{
	Go();

	// Don't consume the event
	// This means wxWidgets can automatically handle closing and destruction of the window
	event.Skip();
}

void CSSGotoDialog::Go()
{
	// Convert the user input into something useable
	wxString lineNumTxtVal = m_txtLineNumber->GetValue();
	long lineNumLong;
	if ( lineNumTxtVal.ToLong( &lineNumLong ) )
	{
		int destLineNum = static_cast< int >( lineNumLong );

		if( destLineNum > m_maxLines )
		{
			destLineNum = m_maxLines;
		}
		else if( destLineNum < 0 )
		{
			destLineNum = 0;
		}

		CGotoEvent* newEvent = new CGotoEvent( m_file, destLineNum, true );
		QueueEvent( newEvent );
	}
	else
	{
		wxMessageBox( wxT( "Bad line number" ), wxT( "Go To Line" ) );
	}
}
