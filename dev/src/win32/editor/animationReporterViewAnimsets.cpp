/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "animationReporter.h"

void CEdAnimationReporterWindow::RefreshViewAnimsets()
{
	wxString html;

	html += wxT("<table border=1>");

	wxString s0 = m_animsetSort == AVS_Names ?				wxT("Sort") : wxT("<a href=\"SortNames\">Sort</a>");
	wxString s1 = m_animsetSort == AVS_AnimSize ?			wxT("Sort") : wxT("<a href=\"SortAnimSize\">Sort</a>");
	wxString s2 = m_animsetSort == AVS_AnimNum ?			wxT("Sort") : wxT("<a href=\"SortAnimNum\">Sort</a>");
	wxString s3 = m_animsetSort == AVS_AnimUsed ?			wxT("Sort") : wxT("<a href=\"SortAnimUsedNum\">Sort</a>");
	wxString s4 = m_animsetSort == AVS_AnimUnused ?			wxT("Sort") : wxT("<a href=\"SortAnimUnusedNum\">Sort</a>");
	wxString s5 = m_animsetSort == AVS_AnimWithoutPose ?	wxT("Sort") : wxT("<a href=\"SortAnimWithoutPose\">Sort</a>");
	wxString s6 = m_animsetSort == AVS_AnimWithoutBox ?		wxT("Sort") : wxT("<a href=\"SortAnimWithoutBox\">Sort</a>");
	wxString s7 = m_animsetSort == AVS_AnimInvalidPoses ?	wxT("Sort") : wxT("<a href=\"SortAnimInvalidPose\">Sort</a>");
	wxString s8 = m_animsetSort == AVS_AnimInvalid ?		wxT("Sort") : wxT("<a href=\"SortAnimInvalid\">Sort</a>");

	html += wxT("<tr>");
	html += wxString::Format( wxT("<th align=center><i>Name </i>%s</th>"), s0.wc_str() );
	html += wxString::Format( wxT("<th width=100 align=center><i>Anim size </i>%s</th>"), s1.wc_str() );
	html += wxString::Format( wxT("<th width=100 align=center><i>Animations </i>%s</th>"), s2.wc_str() );
	html += wxString::Format( wxT("<th width=100 align=center><i>Used Anims </i>%s</th>"), s3.wc_str() );
	html += wxString::Format( wxT("<th width=100 align=center><i>Unused Anims </i>%s</th>"), s4.wc_str() );
	html += wxString::Format( wxT("<th width=100 align=center><i>Anims without pose </i>%s</th>"), s5.wc_str() );
	html += wxString::Format( wxT("<th width=100 align=center><i>Anims without box </i>%s</th>"), s6.wc_str() );
	html += wxString::Format( wxT("<th width=100 align=center><i>Invalid poses </i>%s</th>"), s7.wc_str() );
	html += wxString::Format( wxT("<th width=100 align=center><i>Invalid anims </i>%s</th>"), s8.wc_str() );
	html += wxString::Format( wxT("<th align=center><i>Path</i></th>") );
	html += wxT("</tr>");

	switch ( m_animsetSort )
	{
	case AVS_Names:
		qsort( m_animsetRecords.TypedData(), m_animsetRecords.Size(), sizeof( EdAnimReportAnimset* ), &EdAnimReportAnimset::CmpFuncByNames );
		break;
	case AVS_AnimNum:
		qsort( m_animsetRecords.TypedData(), m_animsetRecords.Size(), sizeof( EdAnimReportAnimset* ), &EdAnimReportAnimset::CmpFuncByAnimNum );
		break;
	case AVS_AnimSize:
		qsort( m_animsetRecords.TypedData(), m_animsetRecords.Size(), sizeof( EdAnimReportAnimset* ), &EdAnimReportAnimset::CmpFuncByAnimSize );
		break;
	case AVS_AnimUsed:
		qsort( m_animsetRecords.TypedData(), m_animsetRecords.Size(), sizeof( EdAnimReportAnimset* ), &EdAnimReportAnimset::CmpFuncByAnimUsed );
		break;
	case AVS_AnimUnused:
		qsort( m_animsetRecords.TypedData(), m_animsetRecords.Size(), sizeof( EdAnimReportAnimset* ), &EdAnimReportAnimset::CmpFuncByAnimUnused );
		break;
	case AVS_AnimWithoutPose:
		qsort( m_animsetRecords.TypedData(), m_animsetRecords.Size(), sizeof( EdAnimReportAnimset* ), &EdAnimReportAnimset::CmpFuncByAnimWithoutPose );
		break;
	case AVS_AnimWithoutBox:
		qsort( m_animsetRecords.TypedData(), m_animsetRecords.Size(), sizeof( EdAnimReportAnimset* ), &EdAnimReportAnimset::CmpFuncByAnimWithoutBox );
		break;
	case AVS_AnimInvalidPoses:
		qsort( m_animsetRecords.TypedData(), m_animsetRecords.Size(), sizeof( EdAnimReportAnimset* ), &EdAnimReportAnimset::CmpFuncByInvalidPoses );
		break;
	case AVS_AnimInvalid:
		qsort( m_animsetRecords.TypedData(), m_animsetRecords.Size(), sizeof( EdAnimReportAnimset* ), &EdAnimReportAnimset::CmpFuncByInvalidAnims );
		break;
	}

	for ( Uint32 i=0; i<m_animsetRecords.Size(); ++i )
	{
		const EdAnimReportAnimset* record = m_animsetRecords[ i ];

		html += wxT("<tr>");
		html += wxString::Format( wxT("<td align=left><a href=\"%d.AnimsetName\">%s</a></td>"), i, record->m_name.AsChar() );
		html += wxString::Format( wxT("<td align=center>%s</td>"), MemSizeToText( record->m_animSize ).wc_str() );
		html += wxString::Format( wxT("<td align=center>%d</td>"), record->m_animNum );
		html += wxString::Format( wxT("<td align=center>%d</td>"), record->m_animUsedNum );
		html += wxString::Format( wxT("<td align=center>%d</td>"), record->m_animUnusedNum );
		html += wxString::Format( wxT("<td align=center>%d</td>"), record->m_animWithoutPoseNum );
		html += wxString::Format( wxT("<td align=center>%d</td>"), record->m_animWithoutBoxNum );
		html += wxString::Format( wxT("<td align=center>%d</td>"), record->m_animInvalidPoses );
		html += wxString::Format( wxT("<td align=center>%d</td>"), record->m_animInvalid );
		html += wxString::Format( wxT("<td align=left><a href=\"%s\">%s</a></td>"), record->m_path.AsChar(), record->m_path.AsChar() );
		html += wxT("</tr>");
	}

	html += wxT("<br><br>");
	html += wxT("<i><a href=\"DumpToFile\">Dump list to file</a></i>");
	html += wxT("<br>");

	html += wxT("</table>");

	wxHtmlWindow* htmlWindow = XRCCTRL( *this, "viewAnimsetHtml", wxHtmlWindow );
	htmlWindow->SetPage( html );
}

void CEdAnimationReporterWindow::OnAnimsetViewLinkClicked( wxHtmlLinkEvent& event )
{
	const wxHtmlLinkInfo& linkInfo = event.GetLinkInfo();
	wxString href = linkInfo.GetHref();

	if ( href == wxT("DumpToFile") )
	{
		DumpAnimsetListToFile();
	}
	else if ( href.EndsWith( wxT("AnimsetName") ) )
	{
		String temp = href.wc_str();
		String numStr = temp.StringBefore( TXT("."), false );

		Int32 num = 0;
		FromString( numStr, num );

		SelectAnimset( num );

		ShowAndRefreshView( RV_AnimsetData );
	}
	else if ( href == wxT("SortNames") )
	{
		m_animsetSort = AVS_Names;
		RefreshViewAnimsets();
	}
	else if ( href == wxT("SortAnimNum") )
	{
		m_animsetSort = AVS_AnimNum;
		RefreshViewAnimsets();
	}
	else if ( href == wxT("SortAnimSize") )
	{
		m_animsetSort = AVS_AnimSize;
		RefreshViewAnimsets();
	}
	else if ( href == wxT("SortAnimUsedNum") )
	{
		m_animsetSort = AVS_AnimUsed;
		RefreshViewAnimsets();
	}
	else if ( href == wxT("SortAnimUnusedNum") )
	{
		m_animsetSort = AVS_AnimUnused;
		RefreshViewAnimsets();
	}
	else if ( href == wxT("SortAnimWithoutPose") )
	{
		m_animsetSort = AVS_AnimWithoutPose;
		RefreshViewAnimsets();
	}
	else if ( href == wxT("SortAnimWithoutBox") )
	{
		m_animsetSort = AVS_AnimWithoutBox;
		RefreshViewAnimsets();
	}
	else if ( href == wxT("SortAnimInvalidPoses") )
	{
		m_animsetSort = AVS_AnimInvalidPoses;
		RefreshViewAnimsets();
	}
	else if ( href == wxT("SortAnimInvalid") )
	{
		m_animsetSort = AVS_AnimInvalid;
		RefreshViewAnimsets();
	}
	else
	{
		String depotPath = href.wc_str();
		SEvents::GetInstance().DispatchEvent( CNAME( SelectAsset ), CreateEventData( depotPath ) );
	}
}

void CEdAnimationReporterWindow::DumpAnimsetListToFile()
{
	CEdFileDialog dlg;
	dlg.SetMultiselection( false );
	dlg.SetIniTag( TXT("CEdAnimationReporterWindow_DumpAnimset") );
	dlg.AddFormat( TXT("csv"), TXT( "CSV" ) );

	if ( dlg.DoSave( (HWND)GetHandle() ) )
	{				
		String filePath = dlg.GetFile();

		String data;

		for ( Uint32 i=0; i<m_animsetRecords.Size(); ++i )
		{
			const EdAnimReportAnimset* record = m_animsetRecords[ i ];
			data += String::Printf( TXT("%s;\n"), record->m_path.AsChar() );
		}

		GFileManager->SaveStringToFile( filePath, data );
	}
}
