/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "errorsListDlg.h"
#include "missingTemplatesErrorsDisplayer.h"
#include "..\versionControl\versionControlP4.h"

CEdMissingTemplatesErrorsDisplayer::CEdMissingTemplatesErrorsDisplayer( wxWindow* parent )
	: m_parent( parent )
{
}

void CEdMissingTemplatesErrorsDisplayer::GenerateWebString( const TSortedMap< String, TDynArray< String > >& layersMissingTemplates )
{
	m_webString = TXT("<!DOCTYPE html><html><head><title></title><style>html,body { font-family: helvetica; font-size: 13px; padding: 0px; margin: 0px; } .entry { border-bottom: 1px solid gray; padding: 5px } .entry:hover { background: #F8F8F8 } </style></head><body><br/>");

	AppendEntries( layersMissingTemplates );

	m_webString += TXT("</body></html>");
}

Bool CEdMissingTemplatesErrorsDisplayer::Execute( const TSortedMap< String, TDynArray< String > >& layersMissingTemplates )
{
	GenerateWebString( layersMissingTemplates );

	CEdErrorsListDlg dlg( m_parent, true, true );
	dlg.SetHeader( TXT( "Oops, someone used templated objects that no longer exist!" ) );
	dlg.SetFooter( TXT( "WARNING! If you save the layer you will lose all the entities without templates. Do you want to continue loading?" ) );
	return dlg.Execute( m_webString );
}

void CEdMissingTemplatesErrorsDisplayer::AppendEntries( const TSortedMap< String, TDynArray< String > >& layersMissingTemplates )
{
	for ( const auto& layersTemplates : layersMissingTemplates )
	{
		AppendLayerEntry( layersTemplates.m_first, layersTemplates.m_second );
	}
}

void CEdMissingTemplatesErrorsDisplayer::AppendLayerEntry( const String& layer, const TDynArray< String >& missingTemplates )
{
	if ( missingTemplates.Empty() )
	{
		return;
	}

	String layerLastEditBy;
	GVersionControl->FileLastEditedBy( GFileManager->GetDataDirectory() + layer, layerLastEditBy );
	String headerString = String::Printf( TXT("<h5>On layer %ls</br>"), layer.AsChar() );
	if ( !layerLastEditBy.Empty() ) 
	{
		headerString += String::Printf( TXT("(last edited by <b>%ls</b>)"), layerLastEditBy.AsChar() );
	}
	headerString += TXT( "</h5>" );
	m_webString += headerString;

	// display templates
	String baseStringStart = TXT( "<div class=\"entry\"><ul>" );
	String baseStringEnd = TXT( "</ul></div><br/><br/>" );
	String templatesString;
	Uint32 unknownFilesCount = 0;
	for ( const String& missing : missingTemplates )
	{
		if ( !missing.Empty() )
		{
			templatesString += String::Printf( TXT( "<li><font color='#505050'>%ls " ), missing.AsChar() );

			String lastEditedBy;
			GVersionControl->FileLastEditedBy( GFileManager->GetDataDirectory() + missing, lastEditedBy );
			if ( !lastEditedBy.Empty() )
			{
				templatesString += String::Printf( TXT("<small style='display: block; text-align: left; margin-top: 2px'>(last edited by <b>%ls</b>)</small>"), lastEditedBy.AsChar() );
			}
			templatesString += TXT( "</font></li>" );
		}
		else
		{
			++unknownFilesCount;
		}
	}

	if ( unknownFilesCount > 0 )
	{
		String unknownTemplatesString = TXT( "<font color='#505050'>" );
		if ( !templatesString.Empty() )
		{
			unknownTemplatesString += TXT( "...and " );
		}
		unknownTemplatesString += String::Printf( TXT( "%d other unknown templates</font><br/>" ), unknownFilesCount );
		templatesString += unknownTemplatesString;
	}

	m_webString += baseStringStart + templatesString + baseStringEnd;
}
