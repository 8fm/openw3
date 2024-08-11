/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "animSetReportDlg.h"
#include "../../common/engine/skeleton.h"

SAnimSetValidationInfo SAnimSetValidationInfo::Validate( const CSkeletalAnimationSet* animSet )
{
	SAnimSetValidationInfo validationInfo;
	validationInfo.m_animSetDepotPath = animSet->GetFile() ? animSet->GetFile()->GetDepotPath() : TXT("Unnamed file");
	validationInfo.m_isValid = true;
	validationInfo.m_mostCommonBonesNum = 0;
	validationInfo.m_mostCommonTracksNum = 0;

	const CSkeleton* skeleton = animSet->GetSkeleton();
	const TDynArray< CSkeletalAnimationSetEntry* >& animations = animSet->GetAnimations();

	if ( animations.Empty() )
	{
		validationInfo.m_isValid = false;
		validationInfo.m_errorInfos.PushBack( TXT("Set does not contain any anims.") );
		return validationInfo;
	}

	const Uint32 MAX_BUCKETS_PER_BONES_NUM_SIZE = 500;
	TStaticArray< TDynArray< CSkeletalAnimationSetEntry* >, MAX_BUCKETS_PER_BONES_NUM_SIZE > bucketsPerBonesNum( MAX_BUCKETS_PER_BONES_NUM_SIZE );

	const Uint32 MAX_BUCKETS_PER_TRACKS_NUM_SIZE = 500;
	TStaticArray< TDynArray< CSkeletalAnimationSetEntry* >, MAX_BUCKETS_PER_TRACKS_NUM_SIZE > bucketsPerTracksNum( MAX_BUCKETS_PER_TRACKS_NUM_SIZE );

	ASSERT( !animations.Empty() );
	Int32 mostCommonBonesNum = -1;
	Int32 mostCommonTracksNum = -1;
	for ( Uint32 i = 0; i < animations.Size(); ++i )
	{
		Uint32 bonesNum = animations[ i ]->GetAnimation()->GetBonesNum();
		Uint32 tracksNum = animations[ i ]->GetAnimation()->GetTracksNum();

		bucketsPerBonesNum[ bonesNum ].PushBack( animations[ i ] );
		bucketsPerTracksNum[ tracksNum ].PushBack( animations[ i ] );

		if ( mostCommonBonesNum == -1 || bucketsPerBonesNum[ mostCommonBonesNum ].Size() < bucketsPerBonesNum[ bonesNum ].Size() )
		{
			mostCommonBonesNum = bonesNum;
		}
		if ( mostCommonTracksNum == -1 || bucketsPerTracksNum[ mostCommonTracksNum ].Size() < bucketsPerTracksNum[ tracksNum ].Size() )
		{
			mostCommonTracksNum = tracksNum;
		}
	}
	const CSkeletalAnimationSetEntry* animWithMostCommonBonesNum = bucketsPerBonesNum[ mostCommonBonesNum ][ 0 ];
	const CSkeletalAnimationSetEntry* animWithMostCommonTracksNum = bucketsPerTracksNum[ mostCommonTracksNum ][ 0 ];
	validationInfo.m_mostCommonBonesNum = animWithMostCommonBonesNum->GetAnimation()->GetBonesNum();
	validationInfo.m_mostCommonTracksNum = animWithMostCommonTracksNum->GetAnimation()->GetTracksNum();

	/* Uncomment this 'if' when needed. Currently it generates too much errors.
	if ( !skeleton )
	{
		validationInfo.m_isValid = false;
		validationInfo.m_errorInfos.PushBack( TXT("There is no skeleton in this anim set.") );
	}
	*/

	if ( skeleton && skeleton->GetBonesNum() != animWithMostCommonBonesNum->GetAnimation()->GetBonesNum() )
	{
		validationInfo.m_isValid = false;
		validationInfo.m_errorInfos.PushBack( String::Printf( TXT("Bones count in skeleton (%d) is different from that in \"%s\" anim (%u).")
			, skeleton->GetBonesNum(), animWithMostCommonBonesNum->GetName().AsChar(), animWithMostCommonBonesNum->GetAnimation()->GetBonesNum() ) );
	}

	for ( Uint32 i = 0; i < animations.Size(); ++i )
	{
		const CSkeletalAnimationSetEntry* anim = animations[ i ];
		if ( anim->GetAnimation()->GetBonesNum() != animWithMostCommonBonesNum->GetAnimation()->GetBonesNum() )
		{
			validationInfo.m_isValid = false;
			validationInfo.m_errorInfos.PushBack( String::Printf( TXT("Bones count in \"%s\" anim (%u) is different from that in \"%s\" anim (%u).")
				, anim->GetName().AsChar(), anim->GetAnimation()->GetBonesNum(), animWithMostCommonBonesNum->GetName().AsChar(), animWithMostCommonBonesNum->GetAnimation()->GetBonesNum() ) );
		}
		if ( anim->GetAnimation()->GetTracksNum() != animWithMostCommonTracksNum->GetAnimation()->GetTracksNum() )
		{
			validationInfo.m_isValid = false;
			validationInfo.m_errorInfos.PushBack( String::Printf( TXT("Tracks count in \"%s\" anim (%u) is different from that in \"%s\" anim (%u)."),
				anim->GetName().AsChar(), anim->GetAnimation()->GetTracksNum(), animWithMostCommonTracksNum->GetName().AsChar(), animWithMostCommonTracksNum->GetAnimation()->GetTracksNum() ) );
		}
	}

	return validationInfo;
}

//////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( CEdAnimSetReportDlg, wxFrame )
	EVT_BUTTON( XRCID("RefreshButton"), CEdAnimSetReportDlg::OnRefresh )
	EVT_BUTTON( XRCID("SaveToFileButton"), CEdAnimSetReportDlg::OnSaveToFile )
	EVT_HTML_LINK_CLICKED( XRCID("HtmlWindow"), CEdAnimSetReportDlg::OnLinkClicked )
END_EVENT_TABLE()

CEdAnimSetReportDlg::CEdAnimSetReportDlg( wxWindow* parent, const TDynArray< CDirectory* >& dirs )
	: m_dirs( dirs )
{
	ASSERT( !dirs.Empty() );

	wxXmlResource::Get()->LoadFrame( this, parent, wxT("AnimSetReport") );

	m_htmlWindow = XRCCTRL( *this, "HtmlWindow", wxHtmlWindow );

	RefreshReport();

	// Show the frame
	Show();
	Layout();
}

CEdAnimSetReportDlg::~CEdAnimSetReportDlg()
{
}

void CEdAnimSetReportDlg::OnRefresh( wxCommandEvent& event )
{
	RefreshReport();
}

void CEdAnimSetReportDlg::OnSaveToFile( wxCommandEvent& event )
{
	CEdFileDialog dlg;
	dlg.SetMultiselection( false );
	dlg.SetIniTag( TXT("CEdAnimSetReportDlg_OnSaveToFile") );
	dlg.AddFormat( TXT("html"), TXT( "HTML" ) );

	if ( dlg.DoSave( (HWND)GetHandle() ) )
	{				
		String filePath = dlg.GetFile();
		GFileManager->SaveStringToFile( filePath, m_htmlCode );
	}
}

void CEdAnimSetReportDlg::OnLinkClicked( wxHtmlLinkEvent& event )
{
	const wxHtmlLinkInfo& linkInfo = event.GetLinkInfo();
	wxString href = linkInfo.GetHref();

	if ( href.StartsWith( wxT("selectAsset:") ) ) // href form is 'selectAsset:assetPath'
	{
		wxString assetPath = href.AfterFirst( ':' );
		String depotPath = assetPath.wc_str();
		SEvents::GetInstance().DispatchEvent( CNAME( SelectAsset ), CreateEventData( depotPath ) );
	}
	else if ( href.StartsWith( wxT("sort:") ) ) // href form is 'sort:isDescendingAsUint32:fieldToSortTypeAsUint32'
	{
		wxString sortInfoStr = href.AfterFirst( ':' );

		wxString isSortDescendingStr = sortInfoStr.BeforeFirst( ':' );
		Uint32 isSortDescendingId;
		Bool conversionSucceeded = FromString< Uint32 >( isSortDescendingStr.wc_str(), isSortDescendingId );
		ASSERT( conversionSucceeded );

		wxString fieldToSortStr = sortInfoStr.AfterFirst( ':' );
		Uint32 fieldToSortId;
		conversionSucceeded = FromString< Uint32 >( fieldToSortStr.wc_str(), fieldToSortId );
		ASSERT( conversionSucceeded );

		SortRaportTable( (ESortTableByFieldType)fieldToSortId, isSortDescendingId != 0 );
		RefreshHtmlOnly();
	}
}

void CEdAnimSetReportDlg::SortRaportTable( ESortTableByFieldType fieldToSortByFieldType, Bool sortDescending )
{
	::Sort( m_validationInfos.Begin(), m_validationInfos.End(),
		[ fieldToSortByFieldType, sortDescending ]( const SAnimSetValidationInfo& a, const SAnimSetValidationInfo& b )
		{ 
			if ( fieldToSortByFieldType == STBFT_SortByAnimSetDepotPath )
			{
				if ( sortDescending )
				{
					return a.m_animSetDepotPath < b.m_animSetDepotPath;
				}
				else
				{
					return a.m_animSetDepotPath > b.m_animSetDepotPath;
				}
			}
			else if ( fieldToSortByFieldType == STBFT_SortByMostCommonBonesNum )
			{
				if ( sortDescending )
				{
					return a.m_mostCommonBonesNum < b.m_mostCommonBonesNum;
				}
				else
				{
					return a.m_mostCommonBonesNum > b.m_mostCommonBonesNum;
				}
			}
			else if ( fieldToSortByFieldType == STBFT_SortByMostCommonTracksNum )
			{
				if ( sortDescending )
				{
					return a.m_mostCommonTracksNum < b.m_mostCommonTracksNum;
				}
				else
				{
					return a.m_mostCommonTracksNum > b.m_mostCommonTracksNum;
				}
			}
			else if ( fieldToSortByFieldType == STBFT_SortByIsValid )
			{
				if ( sortDescending )
				{
					return a.m_isValid < b.m_isValid;
				}
				else
				{
					return a.m_isValid > b.m_isValid;
				}
			}

			ASSERT( false );
			return false;
		}
	);
}

void CEdAnimSetReportDlg::RefreshReport()
{
	m_validationInfos.Clear();
	for ( CResourceIterator< CSkeletalAnimationSet > animSet( m_dirs, TXT("Collecting animation files"), RIF_ReadOnly ); animSet; ++animSet )
	{
		if ( !Cast< CCutsceneTemplate >( animSet.Get() ) && !Cast< CStorySceneDialogset >( animSet.Get() ) )
		{
			m_validationInfos.PushBack( SAnimSetValidationInfo::Validate( animSet.Get() ) );
		}
	}

	RefreshHtmlOnly();
}

void CEdAnimSetReportDlg::RefreshHtmlOnly()
{
	wxString htmlCode;

	AppendHeaderParagraphHtml( htmlCode );
	AppendReportTableHtml( htmlCode );

	m_htmlCode = htmlCode;
	m_htmlWindow->SetPage( htmlCode );
}

void CEdAnimSetReportDlg::AppendHeaderParagraphHtml( wxString& htmlCode )
{
	Bool areAllSetsValid = true;
	Uint32 invalidAnimSetsNum = 0;
	for ( Uint32 i = 0; i < m_validationInfos.Size(); ++i )
	{
		if ( !m_validationInfos[ i ].m_isValid )
		{
			areAllSetsValid = false;
			++invalidAnimSetsNum;
		}
	}
	
	wxString headerColorStr = areAllSetsValid ? wxT("#00ff00") : wxT("#ff0000");

	wxString headerFilesCountStr;
	headerFilesCountStr.Printf( wxT("%u anim sets were checked."), m_validationInfos.Size() );

	wxString headerErrorsCountStr;
	headerErrorsCountStr.Printf( wxT("%u anim sets are invalid."), invalidAnimSetsNum );

	wxString headerParagraphHtml;
	headerParagraphHtml.Printf( wxT("<p><font color=%s size=14>%s %s</font></p>"), headerColorStr.wc_str(), headerFilesCountStr.wc_str(), headerErrorsCountStr.wc_str() );
	htmlCode += headerParagraphHtml;
}

void CEdAnimSetReportDlg::AppendReportTableHtml( wxString& htmlCode )
{
	wxString tableHeader;
	tableHeader.Printf(
		wxT("<table><tr bgcolor=#aaaaaa>")
		wxT("<td>Anim set path <a href=\"sort:%u:%u\">&#x2193</a> <a href=\"sort:%u:%u\">&#x2191</a></td>")
		wxT("<td>Bones num <a href=\"sort:%u:%u\">&#x2193</a> <a href=\"sort:%u:%u\">&#x2191</a></td>")
		wxT("<td>Tracks num <a href=\"sort:%u:%u\">&#x2193</a> <a href=\"sort:%u:%u\">&#x2191</a></td>")
		wxT("<td>Errors</td>")
		wxT("<td>Is valid <a href=\"sort:%u:%u\">&#x2193</a> <a href=\"sort:%u:%u\">&#x2191</a></td></tr>"),
		(Uint32)true, (Uint32)STBFT_SortByAnimSetDepotPath, (Uint32)false, (Uint32)STBFT_SortByAnimSetDepotPath,
		(Uint32)true, (Uint32)STBFT_SortByMostCommonBonesNum, (Uint32)false, (Uint32)STBFT_SortByMostCommonBonesNum,
		(Uint32)true, (Uint32)STBFT_SortByMostCommonTracksNum, (Uint32)false, (Uint32)STBFT_SortByMostCommonTracksNum,
		(Uint32)true, (Uint32)STBFT_SortByIsValid, (Uint32)false, (Uint32)STBFT_SortByIsValid
		);
	htmlCode += tableHeader;
	for ( Uint32 i = 0; i < m_validationInfos.Size(); ++i )
	{
		AppendGeneralValidationInfoRowHtml( htmlCode, m_validationInfos[ i ] );
	}
	htmlCode += wxT("</table>");
}

void CEdAnimSetReportDlg::AppendGeneralValidationInfoRowHtml( wxString& htmlCode, const SAnimSetValidationInfo& validationInfo )
{
	wxString colorStr = validationInfo.m_isValid ? wxT("#aaffaa") : wxT("#ffaaaa");
	wxString statusStr = validationInfo.m_isValid ? wxT("YES") : wxT("NO");
	wxString errorsStr = wxT("<ul>");
	for ( Uint32 i = 0; i < validationInfo.m_errorInfos.Size(); ++i )
	{
		errorsStr += wxT("<li>");
		errorsStr += validationInfo.m_errorInfos[ i ].AsChar();
		errorsStr += wxT("</li>");
	}
	errorsStr += wxT("</ul>");

	wxString rowHtml;
	rowHtml.Printf( wxT("<tr bgcolor=%s><td><a href=\"selectAsset:%s\">%s</a></td><td>%u</td><td>%u</td><td>%s</td><td>%s</td></tr>")
		, colorStr.wc_str(), validationInfo.m_animSetDepotPath.AsChar(), validationInfo.m_animSetDepotPath.AsChar()
		, validationInfo.m_mostCommonBonesNum, validationInfo.m_mostCommonTracksNum, errorsStr.wc_str(), statusStr.wc_str() );
	htmlCode += rowHtml;
}