/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

enum ESortTableByFieldType
{
	STBFT_SortByAnimSetDepotPath = 0,
	STBFT_SortByMostCommonBonesNum,
	STBFT_SortByMostCommonTracksNum,
	STBFT_SortByIsValid,
};

struct SAnimSetValidationInfo
{
	String m_animSetDepotPath;

	Bool m_isValid;
	TDynArray< String > m_errorInfos;
	Uint32 m_mostCommonBonesNum;
	Uint32 m_mostCommonTracksNum;

	static SAnimSetValidationInfo Validate( const CSkeletalAnimationSet* animSet );
};

class CEdAnimSetReportDlg : public wxFrame
{
	DECLARE_EVENT_TABLE();

public:
	CEdAnimSetReportDlg( wxWindow* parent, const TDynArray< CDirectory* >& dirs );
	~CEdAnimSetReportDlg();

protected:
	void OnRefresh( wxCommandEvent& event );
	void OnSaveToFile( wxCommandEvent& event );
	void OnLinkClicked( wxHtmlLinkEvent& event );

	void RefreshReport();
	void RefreshHtmlOnly();
	void AppendHeaderParagraphHtml( wxString& htmlCode );
	void AppendReportTableHtml( wxString& htmlCode );
	void AppendGeneralValidationInfoRowHtml( wxString& htmlCode, const SAnimSetValidationInfo& validationInfo );

	void SortRaportTable( ESortTableByFieldType fieldToSortByFieldType, Bool sortDescending );

protected:
	wxHtmlWindow* m_htmlWindow;
	const TDynArray< CDirectory* > m_dirs;
	TDynArray< SAnimSetValidationInfo > m_validationInfos;
	String m_htmlCode;
};
