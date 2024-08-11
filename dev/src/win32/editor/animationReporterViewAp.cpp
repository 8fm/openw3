/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "animationReporter.h"

void CEdAnimationReporterWindow::RefreshViewAp()
{
	wxString html;

	html += wxT("<table border=1>");

	wxString s1 = m_apShowAllOwners ? wxT("<a href=\"ShowFirstOwner\">First</a>") : wxT("<a href=\"ShowAllOwners\">All</a>");

	html += wxT("<tr>");
	html += wxString::Format( wxT("<th align=left><i>Num</i></th>") );
	html += wxString::Format( wxT("<th align=center><i>Path</i></th>") );
	html += wxString::Format( wxT("<th align=center><i>Owner(s) </i>%s</th>"), s1.wc_str() );
	html += wxT("</tr>");

	for ( Uint32 i=0; i<m_apNodes.Size(); ++i )
	{
		const APNode& node = m_apNodes[ i ];

		html += wxT("<tr>");

		html += wxString::Format( wxT("<td align=left>%d</td>"), i );
		html += wxString::Format( wxT("<td align=left>%s</td>"), node.m_jobTreePath.AsChar() );
		
		if ( m_apShowAllOwners )
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

	wxHtmlWindow* htmlWindow = XRCCTRL( *this, "viewApHtml", wxHtmlWindow );
	htmlWindow->SetPage( html );
}

void CEdAnimationReporterWindow::OnApViewLinkClicked( wxHtmlLinkEvent& event )
{
	const wxHtmlLinkInfo& linkInfo = event.GetLinkInfo();
	wxString href = linkInfo.GetHref();

	if ( href == wxT("ShowFirstOwner") )
	{
		m_apShowAllOwners = false;
		RefreshViewAp();
	}
	else if ( href == wxT("ShowAllOwners") )
	{
		m_apShowAllOwners = true;
		RefreshViewAp();
	}
}
