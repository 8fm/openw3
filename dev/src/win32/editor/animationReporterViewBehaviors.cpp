/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "animationReporter.h"

void CEdAnimationReporterWindow::RefreshViewBehaviors()
{
	wxString html;

	html += wxT("<table border=1>");

	wxString s0 = m_behaviorSort == BVS_Names ?		wxT("Sort") : wxT("<a href=\"SortNames\">Sort</a>");
	wxString s1 = m_behaviorSort == BVS_Size ?		wxT("Sort") : wxT("<a href=\"SortSize\">Sort</a>");
	wxString s2 = m_behaviorSort == BVS_BlockNums ?	wxT("Sort") : wxT("<a href=\"SortNodes\">Sort</a>");
	wxString s3 = m_behaviorSort == BVS_AnimNums ?	wxT("Sort") : wxT("<a href=\"SortAnims\">Sort</a>");

	html += wxT("<tr>");
	html += wxString::Format( wxT("<th align=center><i>Name</i>%s</th>"), s0.wc_str() );
	html += wxString::Format( wxT("<th width=100 align=center><i>Size </i>%s</th>"), s1.wc_str() );
	html += wxString::Format( wxT("<th width=100 align=center><i>Nodes </i>%s</th>"), s2.wc_str() );
	html += wxString::Format( wxT("<th width=100 align=center><i>Animations </i>%s</th>"), s3.wc_str() );
	html += wxString::Format( wxT("<th align=center><i>Path</i></th>") );
	html += wxT("</tr>");

	switch ( m_behaviorSort )
	{
	case BVS_Names:
		qsort( m_behaviorRecords.TypedData(), m_behaviorRecords.Size(), sizeof( EdAnimReportBehavior* ), &EdAnimReportBehavior::CmpFuncByNames );
		break;
	case BVS_Size:
		qsort( m_behaviorRecords.TypedData(), m_behaviorRecords.Size(), sizeof( EdAnimReportBehavior* ), &EdAnimReportBehavior::CmpFuncBySize );
		break;
	case BVS_BlockNums:
		qsort( m_behaviorRecords.TypedData(), m_behaviorRecords.Size(), sizeof( EdAnimReportBehavior* ), &EdAnimReportBehavior::CmpFuncByBlockNums );
		break;
	case BVS_AnimNums:
		qsort( m_behaviorRecords.TypedData(), m_behaviorRecords.Size(), sizeof( EdAnimReportBehavior* ), &EdAnimReportBehavior::CmpFuncByAnimsNums );
		break;
	}

	for ( Uint32 i=0; i<m_behaviorRecords.Size(); ++i )
	{
		const EdAnimReportBehavior* record = m_behaviorRecords[ i ];

		html += wxT("<tr>");
		html += wxString::Format( wxT("<td align=left>%s</td>"), record->m_name.AsChar() );
		html += wxString::Format( wxT("<td align=center>%s</td>"), MemSizeToText( record->m_size ).wc_str() );
		html += wxString::Format( wxT("<td align=center>%d</td>"), record->m_blockNums );
		html += wxString::Format( wxT("<td align=center>%d</td>"), record->m_usedAnimations.Size() );
		html += wxString::Format( wxT("<td align=left><a href=\"%s\">%s</a></td>"), record->m_path.AsChar(), record->m_path.AsChar() );
		html += wxT("</tr>");
	}

	html += wxT("</table>");

	wxHtmlWindow* htmlWindow = XRCCTRL( *this, "viewBehaviorHtml", wxHtmlWindow );
	htmlWindow->SetPage( html );
}

void CEdAnimationReporterWindow::OnBehaviorViewLinkClicked( wxHtmlLinkEvent& event )
{
	const wxHtmlLinkInfo& linkInfo = event.GetLinkInfo();
	wxString href = linkInfo.GetHref();

	if ( href == wxT("SortSize") )
	{
		m_behaviorSort = BVS_Size;
		RefreshViewBehaviors();
	}
	else if ( href == wxT("SortNames") )
	{
		m_behaviorSort = BVS_Names;
		RefreshViewBehaviors();
	}
	else if ( href == wxT("SortNodes") )
	{
		m_behaviorSort = BVS_BlockNums;
		RefreshViewBehaviors();
	}
	else if ( href == wxT("SortAnims") )
	{
		m_behaviorSort = BVS_AnimNums;
		RefreshViewBehaviors();
	}
	else
	{
		String depotPath = href.wc_str();
		SEvents::GetInstance().DispatchEvent( CNAME( SelectAsset ), CreateEventData( depotPath ) );
	}
}
