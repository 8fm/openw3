/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "errorList.h"
#include "frame.h"
#include "documentView.h"

#include "solution/slnContainer.h"
#include "solution/file.h"

wxIMPLEMENT_CLASS( CSSErrorList, wxTextCtrl );

BEGIN_EVENT_TABLE( CSSErrorList, wxTextCtrl )
	EVT_LEFT_DCLICK( CSSErrorList::OnLeftDClick )
END_EVENT_TABLE()

CSSErrorList::CSSErrorList( wxWindow* parent, Solution* solution )
	: wxTextCtrl( parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP )
	, m_solution( solution )
{
	SetFont( wxFont( 8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Courier New") ) );
}

void CSSErrorList::Clear()
{
	// Clear internal data
	m_errors.Clear();
	m_warnings.Clear();

	// Clear text
	wxTextCtrl::SetValue( wxEmptyString );
}

void CSSErrorList::AddLine( const wxString& message )
{
	AppendText( message );
}

void CSSErrorList::AddLine( EType type, const wxString& file, int line, const wxString& message )
{
	const wxChar* errorType = nullptr;

	if( type == Error )
	{
		errorType = wxT( "error" );
	}
	else if( type == Warning )
	{
		errorType = wxT( "warning" );
	}

	wxString buffer;
	buffer.Printf( wxT( "%s(%i): %s: %s\n" ), file.wx_str(), line, errorType, message.wx_str() );

	AppendText( buffer );

	if( type == Error )
	{
		m_errors.Add( buffer );
	}
	else if( type == Warning )
	{
		m_warnings.Add( buffer );
	}
}

void CSSErrorList::List( EType type )
{
	wxArrayString* strings = nullptr;
	wxChar* title = nullptr;

	if( type == Error )
	{
		strings = &m_errors;
		title = wxT( "Errors:\n" );
	}
	else if( type == Warning )
	{
		strings = &m_warnings;
		title = wxT( "Warnings:\n" );
	}

	if( strings && strings->GetCount() > 0 )
	{
		AppendText( title );

		for( size_t i = 0; i < strings->GetCount(); ++i )
		{
			AppendText( ( *strings )[ i ] );
		}
	}
}

#define REGEX_SCRIPT_FILEPATH				wxT( "^(.+\\.ws)" )
#define REGEX_SCRIPT_LINENUMBER_SUFFIX		wxT( "\\(([0-9]+)\\)" )
#define REGEX_SCRIPT_FILEPATH_LINENUMBER	REGEX_SCRIPT_FILEPATH REGEX_SCRIPT_LINENUMBER_SUFFIX

void CSSErrorList::OnLeftDClick( wxMouseEvent& )
{
	// Get clicked line
	long selectionStart=0, selectionEnd=0;
	GetSelection( &selectionStart, &selectionEnd );

	// Convert selection to line index
	long positionX = 0, positionY = 0;
	if ( PositionToXY( selectionEnd, &positionX, &positionY ) )
	{
		wxString lineText = GetLineText( positionY );

		wxRegEx filepathOnly( REGEX_SCRIPT_FILEPATH, wxRE_ADVANCED );

		if( filepathOnly.Matches( lineText ) )
		{
			wxString filename = filepathOnly.GetMatch( lineText, 1 );
			long lineNumber = 1;

			wxRegEx filepathAndLinenumber( REGEX_SCRIPT_FILEPATH_LINENUMBER, wxRE_ADVANCED );

			if( filepathAndLinenumber.Matches( lineText ) )
			{
				wxString line = filepathAndLinenumber.GetMatch( lineText, 2 );
				line.ToLong( &lineNumber );
			}

			SolutionFilePtr file = m_solution->FindFile( filename.wc_str() );
			if ( file )
			{
				wxTheFrame->OpenFileAndGotoLine( file, true, lineNumber );
				if ( file->m_document )
				{
					file->m_document->SetFocus();
				}
			}
		}
	}
}
