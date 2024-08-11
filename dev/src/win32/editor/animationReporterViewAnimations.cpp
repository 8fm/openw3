/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "animationReporter.h"

void CEdAnimationReporterWindow::RefreshViewAnimations()
{
	wxString html;

	html += wxT("<table border=1>");

	wxString s1 = m_animationSort == ANVS_Name ?			wxT("Sort") : wxT("<a href=\"SortName\">Sort</a>");
	wxString s2 = m_animationSort == ANVS_Used ?			wxT("Sort") : wxT("<a href=\"SortUsed\">Sort</a>");
	wxString s3 = m_animationSort == ANVS_AnimSize ?		wxT("Sort") : wxT("<a href=\"SortAnimSize\">Sort</a>");
	wxString s4 = m_animationSort == ANVS_MotionExSize ?	wxT("Sort") : wxT("<a href=\"SortMotionExSize\">Sort</a>");
	wxString s5 = m_animationSort == ANVS_MotionExNum ?		wxT("Sort") : wxT("<a href=\"SortMotionExNum\">Sort</a>");
	wxString s6 = m_animationSort == ANVS_Duration ?		wxT("Sort") : wxT("<a href=\"SortDuration\">Sort</a>");

	html += wxT("<tr>");
	html += wxString::Format( wxT("<th width=100 align=left><i>Name </i>%s</th>"), s1.wc_str() );
	html += wxString::Format( wxT("<th width=100 align=left><i>Used </i>%s</th>"), s2.wc_str() );
	html += wxString::Format( wxT("<th width=100 align=center><i>Anim size </i>%s</th>"), s3.wc_str() );
	html += wxString::Format( wxT("<th width=100 align=center><i>MotionEx size </i>%s</th>"), s4.wc_str() );
	html += wxString::Format( wxT("<th width=100 align=center><i>MotionEx num </i>%s</th>"), s5.wc_str() );
	html += wxString::Format( wxT("<th width=100 align=center><i>Duration </i>%s</th>"), s6.wc_str() );
	html += wxT("<th width=100 align=center><i>Status </i></th>");
	html += wxT("</tr>");

	if ( m_selectedAnimset )
	{
		TDynArray< EdAnimReportAnimation* >& animations = m_selectedAnimset->m_animations;

		switch ( m_animationSort )
		{
		case ANVS_Name:
			qsort( animations.TypedData(), animations.Size(), sizeof( EdAnimReportAnimation* ), &EdAnimReportAnimation::CmpFuncByNames );
			break;
		case ANVS_Used:
			qsort( animations.TypedData(), animations.Size(), sizeof( EdAnimReportAnimation* ), &EdAnimReportAnimation::CmpFuncByUsedNum );
			break;
		case ANVS_AnimSize:
			qsort( animations.TypedData(), animations.Size(), sizeof( EdAnimReportAnimation* ), &EdAnimReportAnimation::CmpFuncByAnimSize );
			break;
		case ANVS_MotionExSize:
			qsort( animations.TypedData(), animations.Size(), sizeof( EdAnimReportAnimation* ), &EdAnimReportAnimation::CmpFuncByMotionExSize );
			break;
		case ANVS_MotionExNum:
			qsort( animations.TypedData(), animations.Size(), sizeof( EdAnimReportAnimation* ), &EdAnimReportAnimation::CmpFuncByMotionExNum );
			break;
		case ANVS_Duration:
			qsort( animations.TypedData(), animations.Size(), sizeof( EdAnimReportAnimation* ), &EdAnimReportAnimation::CmpFuncByDuration );
			break;
		}

		for ( Uint32 i=0; i<animations.Size(); ++i )
		{
			const EdAnimReportAnimation* record = animations[ i ];

			html += wxT("<tr>");

			if ( record->m_used == 0 )
			{
				html += wxT("<font color=\"#FF0000\">");
			}

			html += wxString::Format( wxT("<td align=left>%s</td>"), record->m_name.AsString().AsChar() );
			html += wxString::Format( wxT("<td align=center>%d</td>"), record->m_used );
			html += wxString::Format( wxT("<td align=center>%s</td>"), MemSizeToText( record->m_animSize ).wc_str() );
			html += wxString::Format( wxT("<td align=center>%s</td>"), MemSizeToText( record->m_motionExSize ).wc_str() );
			html += wxString::Format( wxT("<td align=center>%d</td>"), record->m_motionExNum );
			html += wxString::Format( wxT("<td align=center>%.1f</td>"), record->m_duration );

			if ( record->m_isValid )
			{
				html += wxT("<td align=center>Valid</td>");
			}
			else
			{
				html += wxT("<td align=center>Invalid</td>");
			}

			if ( record->m_used == 0 )
			{
				html += wxT("</font>");
			}

			html += wxT("</tr>");
		}
	}

	html += wxT("</table>");

	wxHtmlWindow* htmlWindow = XRCCTRL( *this, "viewAnimationHtml", wxHtmlWindow );
	htmlWindow->SetPage( html );
}

void CEdAnimationReporterWindow::OnAnimationViewLinkClicked( wxHtmlLinkEvent& event )
{
	const wxHtmlLinkInfo& linkInfo = event.GetLinkInfo();
	wxString href = linkInfo.GetHref();

	if ( href == wxT("SortName") )
	{
		m_animationSort = ANVS_Name;
		RefreshViewAnimations();
	}
	else if ( href == wxT("SortUsed") )
	{
		m_animationSort = ANVS_Used;
		RefreshViewAnimations();
	}
	else if ( href == wxT("SortAnimSize") )
	{
		m_animationSort = ANVS_AnimSize;
		RefreshViewAnimations();
	}
	else if ( href == wxT("SortMotionExSize") )
	{
		m_animationSort = ANVS_MotionExSize;
		RefreshViewAnimations();
	}
	else if ( href == wxT("SortMotionExNum") )
	{
		m_animationSort = ANVS_MotionExNum;
		RefreshViewAnimations();
	}
	else if ( href == wxT("SortDuration") )
	{
		m_animationSort = ANVS_Duration;
		RefreshViewAnimations();
	}
}
