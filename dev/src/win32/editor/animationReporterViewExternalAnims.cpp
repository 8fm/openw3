/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "animationReporter.h"

void CEdAnimationReporterWindow::RefreshExternalAnimPage()
{
	wxString html;

	html += wxT("<table border=1>");

	wxString s0 = m_externalAnimSort == EAVS_Name ?		wxT("Sort") : wxT("<a href=\"SortName\">Sort</a>");
	wxString s1 = m_externalAnimSort == EAVS_Animset ?	wxT("Sort") : wxT("<a href=\"SortAnimset\">Sort</a>");
	wxString s2 = m_externalAnimSort == EAVS_Owner ?	wxT("Sort") : wxT("<a href=\"SortOwner\">Sort</a>");

	html += wxT("<tr>");
	html += wxString::Format( wxT("<th align=center><i>Name </i>%s</th>"), s0.wc_str() );
	html += wxString::Format( wxT("<th align=center><i>Animset </i>%s</th>"), s1.wc_str() );
	html += wxString::Format( wxT("<th align=center><i>Owner </i>%s</th>"), s2.wc_str() );
	html += wxT("<th align=center><i>Desc </i></th>");
	html += wxT("</tr>");

	/*switch ( m_externalAnimSort )
	{
	case EAVS_Name:
		qsort( m_externalAnims.TypedData(), m_externalAnims.Size(), sizeof( EdAnimReportBehavior* ), &EdAnimReportBehavior::CmpFuncByNames );
		break;
	case EAVS_Owner:
		qsort( m_externalAnims.TypedData(), m_externalAnims.Size(), sizeof( EdAnimReportBehavior* ), &EdAnimReportBehavior::CmpFuncBySize );
		break;
	}*/

	for ( Uint32 i=0; i<m_externalAnims.Size(); ++i )
	{
		const SExternalAnim& anim = m_externalAnims[ i ];

		html += wxT("<tr>");
		html += wxString::Format( wxT("<td align=left>%s</td>"), anim.m_animation.AsChar() );

		if ( !anim.m_animset.Empty() )
		{
			html += wxString::Format( wxT("<td align=left>%s</td>"), anim.m_animset.AsChar() );
		}
		else
		{
			html += wxT("<td align=left>Any</td>");
		}

		html += wxString::Format( wxT("<td align=left>%s</td>"), anim.m_owner.AsChar() );
		html += wxString::Format( wxT("<td align=left>%s</td>"), anim.m_desc.AsChar() );
		html += wxT("</tr>");
	}

	html += wxT("</table>");

	wxHtmlWindow* htmlWindow = XRCCTRL( *this, "viewExternalAnimsHtml", wxHtmlWindow );
	htmlWindow->SetPage( html );
}

void CEdAnimationReporterWindow::OnExternalAnimsViewLinkClicked( wxHtmlLinkEvent& event )
{
	const wxHtmlLinkInfo& linkInfo = event.GetLinkInfo();
	wxString href = linkInfo.GetHref();

	if ( href == wxT("SortName") )
	{
		m_externalAnimSort = EAVS_Name;
		RefreshExternalAnimPage();
	}
	if ( href == wxT("SortAnimset") )
	{
		m_externalAnimSort = EAVS_Animset;
		RefreshExternalAnimPage();
	}
	else if ( href == wxT("SortOwner") )
	{
		m_externalAnimSort = EAVS_Owner;
		RefreshExternalAnimPage();
	}
}
