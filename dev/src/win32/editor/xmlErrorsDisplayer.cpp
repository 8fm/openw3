/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/dataError.h"
#include "xmlErrorsDisplayer.h"
#include "errorsListDlg.h"

CEdXMLErrorsDisplayer::CEdXMLErrorsDisplayer( wxWindow* parent )
	: m_parent( parent )
{
	m_warningColor = TXT( "#505050" );
	m_errorColor = TXT( "#B80000" );
}

CEdXMLErrorsDisplayer::~CEdXMLErrorsDisplayer()
{
	if ( m_dlg )
	{
		m_dlg->Close();
		m_dlg->Destroy();
	}
}

void CEdXMLErrorsDisplayer::GenerateWebString()
{
	m_webString = TXT("<!DOCTYPE html><html><head><title></title><style>html,body { font-family: helvetica; font-size: 13px; padding: 0px; margin: 0px; } .entry { border-bottom: 1px solid gray; padding: 5px } .entry:hover { background: #F8F8F8 } </style></head><body><br/>");

	AppendErrors();

	m_webString += TXT("</body></html>");
}

void CEdXMLErrorsDisplayer::Execute()
{
	if ( !m_dlg )
	{
		m_dlg = new CEdErrorsListDlg( m_parent, false );
		m_dlg->SetHeader( TXT( "XML parsing errors occured:" ) );
		m_dlg->ActivateConfigParamCheckBox( TXT("Show on editor start"), TXT("User"), TXT("Editor"), TXT("showXMLErrorsOnStart") );
	}

	GenerateWebString();
	m_dlg->Execute( m_webString );
}

void CEdXMLErrorsDisplayer::AppendErrors()
{
#ifndef NO_DATA_VALIDATION
	if ( GDataError )
	{
		const THashMap< String, TDynArray< String > >& errorsMap = GDataError->GetXMLErrors();

		// general errors
		const TDynArray< String >* errors = errorsMap.FindPtr( TXT( "General" ) );
		if ( errors )
		{
			m_webString += GenerateFileEntry( TXT( "GENERAL" ), *errors );
		}

		TDynArray< String > keys;
		errorsMap.GetKeys( keys );
		Sort( keys.Begin(), keys.End() );
		keys.Remove( TXT( "General" ) );

		for ( const String& mapKey : keys )
		{
			errors = errorsMap.FindPtr( mapKey );
			if ( errors )
			{
				m_webString += GenerateFileEntry( mapKey, *errors );
			}
		}
	}
#endif
}

String CEdXMLErrorsDisplayer::GenerateFileEntry( const String& header, const TDynArray< String >& errors )
{
	if ( errors.Empty() )
	{
		return String::EMPTY;
	}

	String headerString = String::Printf( TXT("<h5>%ls:</h5>"), header.AsChar() );
	String baseStringStart = TXT( "<div class=\"entry\"><left>" );
	String baseStringEnd = TXT( "</left></div><br/><br/>" );
	String errorsString;

	for ( const String& error : errors )
	{
		errorsString += String::Printf( TXT( "<font color='%ls'>%ls</font><br/>" ),
			error.BeginsWith( TXT( "ERROR" ) ) ? m_errorColor.AsChar() : m_warningColor.AsChar(),
			error.AsChar() );
	}

	String returnString =  headerString + baseStringStart + errorsString + baseStringEnd;
	return returnString;
}
