/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "animationReporter.h"

void CEdAnimationReporterWindow::RefreshViewEntities()
{
	wxString html;

	html += wxT("<table border=1>");

	wxString s4 = m_acShowAllOwners ? wxT("<a href=\"ShowFirstOwner\">First</a>") : wxT("<a href=\"ShowAllOwners\">All</a>");

	html += wxT("<tr>");
	html += wxString::Format( wxT("<th align=left><i>Num</i></th>") );
	html += wxString::Format( wxT("<th width=100 align=center><i>Animsets</i></th>") );
	html += wxString::Format( wxT("<th width=100 align=center><i>Behaviors</i></th>") );
	html += wxString::Format( wxT("<th align=center><i>Owner(s) </i>%s</th>"), s4.wc_str() );
	html += wxT("</tr>");

	/*switch ( m_entitiesSort )
	{
	case EVS_Name:
		qsort( m_entityTemplateRecords.TypedData(), m_entityTemplateRecords.Size(), sizeof( EdEntityTemplateRecord* ), &EdEntityTemplateRecord::CmpFuncByNames );
		break;
	case EVS_Animset:
		qsort( m_entityTemplateRecords.TypedData(), m_entityTemplateRecords.Size(), sizeof( EdEntityTemplateRecord* ), &EdEntityTemplateRecord::CmpFuncByAnimsets );
		break;
	case EVS_Behavior:
		qsort( m_entityTemplateRecords.TypedData(), m_entityTemplateRecords.Size(), sizeof( EdEntityTemplateRecord* ), &EdEntityTemplateRecord::CmpFuncByBehaviors );
		break;
	}*/

	for ( Uint32 i=0; i<m_acNodes.Size(); ++i )
	{
		const ACNode& node = m_acNodes[ i ];

		html += wxT("<tr>");

		html += wxString::Format( wxT("<td align=left>%d</td>"), i );
		html += wxString::Format( wxT("<td align=center>%d</td>"), node.m_animsets.Size() );
		html += wxString::Format( wxT("<td align=center>%d</td>"), node.m_behaviors.Size() );
		
		if ( m_acShowAllOwners )
		{
			html += wxT("<td align=left>");
			html += wxT("<table>");

			for ( Uint32 j=0; j<node.m_owners.Size(); ++j )
			{
				html += wxT("<tr>");
				html += wxString::Format( wxT("<th>%d.</th>"), j );
				html += wxString::Format( wxT("<th align=left>%s</th>"), node.m_owners[ j ].AsChar() );
				html += wxT("</tr>");
			}

			html += wxT("</table>");
			html += wxT("</td>");
		}
		else
		{
			if ( node.m_owners.Size() > 0 )
			{
				html += wxString::Format( wxT("<td align=left><a href=\"%s\">%s</a></td>"), node.m_owners[0].AsChar(), node.m_owners[0].AsChar() );
			}
			else
			{
				html += wxT("<td align=left>Invalid</td>");
			}
		}

		html += wxT("</tr>");
	}

	html += wxT("</table>");

	wxHtmlWindow* htmlWindow = XRCCTRL( *this, "viewEntitiesHtml", wxHtmlWindow );
	htmlWindow->SetPage( html );
}

void CEdAnimationReporterWindow::OnEntityViewLinkClicked( wxHtmlLinkEvent& event )
{
	const wxHtmlLinkInfo& linkInfo = event.GetLinkInfo();
	wxString href = linkInfo.GetHref();

	if ( href == wxT("ShowFirstOwner") )
	{
		m_acShowAllOwners = false;
		RefreshViewEntities();
	}
	else if ( href == wxT("ShowAllOwners") )
	{
		m_acShowAllOwners = true;
		RefreshViewEntities();
	}
	else if ( href == wxT("SortName") )
	{
		m_entitiesSort = EVS_Name;
		RefreshViewEntities();
	}
	else if ( href == wxT("SortAnimset") )
	{
		m_entitiesSort = EVS_Animset;
		RefreshViewEntities();
	}
	else if ( href == wxT("SortBehavior") )
	{
		m_entitiesSort = EVS_Behavior;
		RefreshViewEntities();
	}
	else if ( href.EndsWith( TXT("Name") ) )
	{
		String temp = href.wc_str();
		String numStr = temp.StringBefore( TXT("."), false );

		Int32 num = 0;
		FromString( numStr, num );

		SelectEntity( num );

		ShowAndRefreshView( RV_EntityData );
	}
	else
	{
		String depotPath = href.wc_str();
		SEvents::GetInstance().DispatchEvent( CNAME( SelectAsset ), CreateEventData( depotPath ) );
	}
}
