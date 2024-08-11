/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "dialogEditor.h"
#include "dialogEditorLocAnalyser.h"
#include "storyScenePreviewPlayer.h"
#include "voice.h"
#include "popupNotification.h"

#include "../../common/game/storySceneEvent.h"
#include "../../common/game/storySceneEventOverridePlacement.h"
#include "../../common/game/storySceneEventScenePropPlacement.h"
#include "../../common/game/storySceneEventWorldPropPlacement.h"
#include "../../common/game/storySceneEventEnhancedCameraBlend.h"

#include "../../common/game/storySceneLine.h"

#include "../../common/core/depot.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/localizationManager.h"
#include "../../common/game/storySceneCutsceneSection.h"
#include "../../common/game/storySceneVideo.h"

#include "../../../external/BasicExcel/BasicExcel.cpp"
#include "../../../external/BasicExcel/ExcelFormat.cpp"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

BEGIN_EVENT_TABLE( CEdSceneLocAnalyserExportDialog, wxDialog )
	EVT_CHECKBOX( XRCID( "DLA_AllLanguages" ), CEdSceneLocAnalyserExportDialog::OnAllLanguagesCheck )
END_EVENT_TABLE()

CEdSceneLocAnalyserExportDialog::CEdSceneLocAnalyserExportDialog( wxWindow* parent )
	: m_warnForAbsDiff( true )
	, m_warnLevel( 1.0f )
	, m_sortType( 0 )
	, m_verbose( false )
	, m_onlyDLGLines( false )
{
	wxXmlResource::Get()->LoadDialog( this, parent, TEXT( "DialogLocAnalyzerExport" ) );
	Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdSceneLocAnalyserExportDialog::OnExport ), NULL, this );
	
	m_DLALanguageCheckList = XRCCTRL( *this, "DLA_languageCheckList", wxCheckListBox );

	TDynArray< String > textLanguages, speechLanguages;
	SLocalizationManager::GetInstance().GetAllAvailableLanguages( textLanguages, speechLanguages );
	const String& langRef = GCommonGame->GetSystem< CStorySceneSystem >()->GetLangRefName();

	// we want the ref language to be first on our list
	m_DLALanguageCheckList->Append( langRef.AsChar() );
	textLanguages.Remove( langRef );
	for ( const String& lang : textLanguages )
	{
		m_DLALanguageCheckList->Append( lang.AsChar() );
	}
	m_DLALanguageCheckList->Fit();
}

void CEdSceneLocAnalyserExportDialog::OnExport( wxCommandEvent& event )
{
	RED_UNUSED( event );

	wxRadioBox* warnForRB = XRCCTRL( *this, "DLA_WarnFor", wxRadioBox );
	if( warnForRB )
	{
		m_warnForAbsDiff = warnForRB->GetSelection() == 0;
	}

	wxTextCtrl* txtCtrl = XRCCTRL( *this, "DLA_WarnLevel", wxTextCtrl );
	if( txtCtrl )
	{
		wxString val = txtCtrl->GetValue();
		double d = 0.0;
		if( val.ToDouble( &d ) )
		{
			m_warnLevel = ( Float )d;
		}
	}

	wxChoice* sortTypeChoice = XRCCTRL( *this, "DLA_SortType", wxChoice );
	if( sortTypeChoice )
	{
		m_sortType = ( Uint32 )sortTypeChoice->GetSelection();
	}

	wxCheckBox* verbose = XRCCTRL( *this, "DLA_Verbose", wxCheckBox );
	if( verbose )
	{
		m_verbose = verbose->IsChecked();
	}

	m_languages.Clear();
	for ( Uint32 i = 0; i < m_DLALanguageCheckList->GetCount(); ++i )
	{
		if ( m_DLALanguageCheckList->IsChecked( i ) )
		{
			m_languages.PushBack( m_DLALanguageCheckList->GetString( i ).c_str().AsWChar() );
		}
	}

	wxCheckBox* onlyDLGLines = XRCCTRL( *this, "DLA_OnlyDialog", wxCheckBox );
	if( onlyDLGLines )
	{
		m_onlyDLGLines = onlyDLGLines->IsChecked();
	}

	EndModal( wxID_OK );
}

void CEdSceneLocAnalyserExportDialog::OnAllLanguagesCheck( wxCommandEvent& event )
{
	for ( Uint32 i = 0; i < m_DLALanguageCheckList->GetCount(); i++ )
	{
		m_DLALanguageCheckList->Check( i, event.IsChecked() );
	}
}



CEdSceneLocAnalyser::LangEntry::LangEntry()
	: m_duration( 0.0f )
	, m_fakeDuration( 0.0f )
	, m_timeDiff( 0.0f )
	, m_percentDiff( 0.0f )
	, m_fakeTimeDiff( 0.0f )
	, m_fakePercentDiff( 0.0f )
{
}


CEdSceneLocAnalyser::SummaryEntry::SummaryEntry()
	: m_maxRealAbsDiff( 0.0f )
	, m_maxRealPercentDiff( 0.0f )
	, m_maxFakeAbsDiff( 0.0f )
	, m_maxFakePercentDiff( 0.0f )
{
}

void CEdSceneLocAnalyser::SummaryEntry::Clear()
{
	m_maxRealAbsDiff = 0.0f;
	m_maxRealPercentDiff = 0.0f;
	m_maxFakeAbsDiff = 0.0f;
	m_maxFakePercentDiff = 0.0f;
}



CEdSceneLocAnalyser::Summary::Summary()
	: m_totalLineWarnings( 0 )
	, m_totalSectionWarnings( 0 )
	, m_sectionsCount( 0 )
{
}

void CEdSceneLocAnalyser::Summary::Clear()
{
	m_totalLineWarnings =  0;
	m_totalSectionWarnings = 0;
	m_sectionsCount = 0;
	m_linesCountInSection.Clear();
	m_langLineTotalWarnings.Clear();
	m_langSectionTotalWarnings.Clear();
	m_langLinesWarningsPerSection.Clear();
	m_sectionsSummary.Clear();
	m_linesSummary.Clear();
}


CEdSceneLocAnalyser::ExportIndex::ExportIndex( const Summary& s )
{
	m_totalLineWarnings			= s.m_totalLineWarnings;
	m_totalSectionWarnings		= s.m_totalSectionWarnings;
	m_sectionsCount				= s.m_sectionsCount;

	m_linesCountInSection = s.m_linesCountInSection;
	m_langLineTotalWarnings.PushBack( s.m_langLineTotalWarnings );
	m_langSectionTotalWarnings.PushBack( s.m_langSectionTotalWarnings );

	m_langLinesWarningsPerSection = s.m_langLinesWarningsPerSection;

	m_linesSummary.m_maxRealAbsDiff		= s.m_linesSummary.m_maxRealAbsDiff;
	m_linesSummary.m_maxRealPercentDiff	= s.m_linesSummary.m_maxRealPercentDiff;
	m_linesSummary.m_maxFakeAbsDiff		= s.m_linesSummary.m_maxFakeAbsDiff;
	m_linesSummary.m_maxFakePercentDiff	= s.m_linesSummary.m_maxFakePercentDiff;

	m_sectionsSummary.m_maxRealAbsDiff		= s.m_sectionsSummary.m_maxRealAbsDiff;
	m_sectionsSummary.m_maxRealPercentDiff	= s.m_sectionsSummary.m_maxRealPercentDiff;
	m_sectionsSummary.m_maxFakeAbsDiff		= s.m_sectionsSummary.m_maxFakeAbsDiff;
	m_sectionsSummary.m_maxFakePercentDiff	= s.m_sectionsSummary.m_maxFakePercentDiff;
}




//////////////////////////////////////////////////////////////////////////

CEdSceneLocAnalyser::CEdSceneLocAnalyser()
	: m_enabled( false )
	, m_window( nullptr )
	, m_referenceLangID( -1 )
	, m_renderOptionShowSections( true )
	, m_warnForAbsDiff( true )
	, m_renderOptionRealTime( true )
	, m_renderOptionWarnLevel( 0.2f )
	, m_sortType( ST_DEFAULT )
	, m_verbose( true )
	, m_showAllLanguages( false )
	, m_onlyDLGLines( false )
{
	FillRootExportGroup( m_rootExportGroup );

	TDynArray< String > speechLanguages;
	SLocalizationManager::GetInstance().GetAllAvailableLanguages( m_langs, speechLanguages );
	m_chosenLangs.PushBack( m_langs );
}

CEdSceneLocAnalyser::~CEdSceneLocAnalyser()
{
}

void CEdSceneLocAnalyser::Enable( Bool flag )
{
	m_enabled = flag;
}

Bool CEdSceneLocAnalyser::IsEnabled() const
{
	return m_enabled;
}

void CEdSceneLocAnalyser::BindToWindow( CEdSceneEditor* ed, wxPanel* window )
{
	SCENE_ASSERT( !m_window );
	m_window = window;

	{
		wxHtmlWindow* html = XRCCTRL( *m_window, "locHtml", wxHtmlWindow );

		if( !html )
		{
			RED_ASSERT( false, TXT( "locHtml not found for window" ) );
			return;
		}

		html->Connect( wxEVT_COMMAND_HTML_LINK_CLICKED, wxHtmlLinkEventHandler( CEdSceneEditor::OnToolLocLinkClicked ), NULL, ed );
		html->Connect( wxEVT_COMMAND_HTML_CELL_HOVER, wxHtmlCellEventHandler( CEdSceneEditor::OnToolLocLinkHover ), NULL, ed );
	}

	{
		wxTextCtrl* txtCtrl = XRCCTRL( *m_window, "DLA_WarnLevel", wxTextCtrl );
		if( !txtCtrl )
		{
			RED_ASSERT( false, TXT( "DLA_WarnLevel control not found for window" ) );
			return;
		}
		
		txtCtrl->SetValidator( wxTextValidator( wxFILTER_NUMERIC ) );
	}

	{
		wxChoice* sortByChoice = XRCCTRL( *m_window, "DLA_SortBy", wxChoice );
		if( sortByChoice )
		{
			for( Uint32 i = 0; i < m_langs.Size(); ++i )
			{
				sortByChoice->Insert( wxString::Format( wxT( "%s" ), m_langs[ i ].AsChar() ), i );
			}
		}
	}

	window->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( CEdSceneEditor::OnLocAnalyzerCommonEvt ), NULL, ed );
	window->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdSceneEditor::OnLocAnalyzerCommonEvt ), NULL, ed );
	window->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CEdSceneEditor::OnLocAnalyzerCommonEvt ), NULL, ed );
	window->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdSceneEditor::OnLocAnalyzerCommonEvt ), NULL, ed );

	RenderHTML();
}

Bool CEdSceneLocAnalyser::IsBindedToWindow() const
{
	return m_window != nullptr;
}

void CEdSceneLocAnalyser::ParseScene( const CStoryScene* scene )
{
	m_lineEntry.Clear();
	m_sectionEntry.Clear();
	m_langsFlag.Clear();
	
	{ // parse sections
		const CStorySceneSection* lastParsed = nullptr;
		for ( Uint32 i = 0; i < scene->GetNumberOfSections(); ++i )
		{
			const CStorySceneSection* section = scene->GetSection( i );
			if( section && ( section != lastParsed ) )
			{
				lastParsed = section;
				ParseSection( section );
			}
		}
	}

	{ // update lang data
		const Uint32 numLangs = m_chosenLangs.Size();
		RED_ASSERT( numLangs > 0, TXT( "No languages specified" ) );

		m_langsFlag.Resize( numLangs );
		for ( Uint32 i = 0; i < numLangs; ++i )
		{
			m_langsFlag[ i ] = HasLocForLang( m_chosenLangs[ i ] );
		}
	}
}

void CEdSceneLocAnalyser::RenderHTML()
{
	SCENE_ASSERT( m_window );

	if ( !m_window )
	{
		return;
	}

	{
		wxRadioBox* showRB = XRCCTRL( *m_window, "DLA_Show", wxRadioBox );
		if( showRB )
		{
			m_renderOptionShowSections = showRB->GetSelection() == 0;
		}

		wxRadioBox* warnForRB = XRCCTRL( *m_window, "DLA_WarnFor", wxRadioBox );
		if( warnForRB )
		{
			m_warnForAbsDiff = warnForRB->GetSelection() == 0;
		}

		wxRadioBox* timeSetupRB = XRCCTRL( *m_window, "DLA_TimeSetup", wxRadioBox );
		if( timeSetupRB )
		{
			m_renderOptionRealTime = timeSetupRB->GetSelection() == 0;
		}

		wxTextCtrl* txtCtrl = XRCCTRL( *m_window, "DLA_WarnLevel", wxTextCtrl );
		if( txtCtrl )
		{
			wxString val = txtCtrl->GetValue();
			double d = 0.0;
			if( val.ToDouble( &d ) )
			{
				m_renderOptionWarnLevel = ( Float )d;
			}
		}

		wxChoice* sortTypeChoice = XRCCTRL( *m_window, "DLA_SortType", wxChoice );
		if( sortTypeChoice )
		{
			Uint32 selection = ( Uint32 )sortTypeChoice->GetSelection();
			m_sortType = ( ESortType )selection;
		}

		wxChoice* sortByChoice = XRCCTRL( *m_window, "DLA_SortBy", wxChoice );
		if( sortByChoice )
		{
			wxString selected = sortByChoice->GetStringSelection();
			m_sortBy = selected;
		}

		wxCheckBox* showAllLangs = XRCCTRL( *m_window, "DLA_AllLanguages", wxCheckBox );
		if( showAllLangs )
		{
			m_showAllLanguages = showAllLangs->IsChecked();
			
			if ( m_showAllLanguages )
			{
				m_chosenLangs.Clear();
				m_chosenLangs.PushBack( m_langs );
			}
			else
			{
				PickDefaultLanguages();
			}
		}
	}

	wxHtmlWindow* html = XRCCTRL( *m_window, "locHtml", wxHtmlWindow );
	SCENE_ASSERT( html );

	wxString code;
	GenerateTableHTML( code );
	html->SetPage( code );
}

void CEdSceneLocAnalyser::PickDefaultLanguages()
{
	m_chosenLangs.Clear();
	m_chosenLangs.Reserve( 7 );

	m_chosenLangs.PushBack( TXT( "PL" ) );
	m_chosenLangs.PushBack( TXT( "EN" ) );
	m_chosenLangs.PushBack( TXT( "DE" ) );
	m_chosenLangs.PushBack( TXT( "FR" ) );
	m_chosenLangs.PushBack( TXT( "JP" ) );
	m_chosenLangs.PushBack( TXT( "RU" ) );
	m_chosenLangs.PushBack( TXT( "BR" ) );
}

Bool CEdSceneLocAnalyser::AlreadyParsedLine( Uint32 stringId ) const
{
	const Uint32 numData = m_lineEntry.Size();
	for ( Uint32 i = 0; i < numData; ++i )
	{
		if ( m_lineEntry[ i ].m_id == stringId )
		{
			return true;
		}
	}

	return false;
}

Bool CEdSceneLocAnalyser::AlreadyParsedSection( const String& sectionName ) const
{
	const Uint32 num = m_sectionEntry.Size();
	for ( Uint32 i = 0; i < num; ++i )
	{
		if ( m_sectionEntry[ i ].m_sectionName == sectionName )
		{
			return true;
		}
	}

	return false;
}


Bool CEdSceneLocAnalyser::HasLocForLang( const String& lang ) const
{
	Uint32 lid = GetLangId( lang );
	const Uint32 numData = m_lineEntry.Size();
	for ( Uint32 i = 0; i < numData; ++i )
	{
		RED_ASSERT( lid < m_lineEntry[ i ].m_langEntry.Size() );
		const LineLangEntry& langEntry = m_lineEntry[ i ].m_langEntry[ lid ];

		if ( langEntry.m_duration > 0.0f )
		{
			return true;
		}
	}

	return false;
}

Uint32 CEdSceneLocAnalyser::GetLangId( const String& lang ) const
{
	SCENE_ASSERT( m_chosenLangs.Size() > 0 );

	for ( Uint32 i = 0; i < m_chosenLangs.Size(); ++i )
	{
		if ( m_chosenLangs[ i ] == lang )
		{
			return i;
		}
	}

	SCENE_ASSERT( 0 );
	return 0;
}

Uint32 CEdSceneLocAnalyser::GetCurrLangId() const
{
	return GetLangId( SLocalizationManager::GetInstance().GetCurrentLocale() );
}

Uint32 CEdSceneLocAnalyser::GetRefLangId() const
{
	if( m_referenceLangID != -1 )
	{
		return ( Uint32 )m_referenceLangID;
	}

	const String& langRef = GCommonGame->GetSystem< CStorySceneSystem >()->GetLangRefName();
	Uint32 tmp = GetLangId( langRef );
	m_referenceLangID = ( Int32 )tmp;
	return tmp;
}

void CEdSceneLocAnalyser::ParseSection( const CStorySceneSection* section )
{
	SCENE_ASSERT( section );

	if( m_onlyDLGLines && !IsDLGSection( section ) )
	{
		return;
	}

	if ( GFeedback && m_verbose )
	{
		GFeedback->BeginTask( TXT( "Localization" ), false );
	}

	Uint32& linesInSection = m_summary.m_linesCountInSection.GetRef( section->GetName() );
	linesInSection = 0;

	const Uint32 numLangs = m_chosenLangs.Size();
	RED_ASSERT( numLangs > 0, TXT( "No languages specified" ) );

	Int32 firstLineIndex = -1;
	Int32 lastLineIndex = -1;

	const TDynArray< CStorySceneElement* >& elements = section->GetElements();
	const Uint32 numElements = elements.Size();
	for ( Uint32 i = 0; i < numElements; ++i )
	{
		if ( GFeedback && m_verbose )
		{
			GFeedback->UpdateTaskProgress( i, numElements );
		}

		const CStorySceneElement* elem = elements[ i ];
		SCENE_ASSERT( elem );

		if ( const CStorySceneLine* line = Cast< const CStorySceneLine >( elem ) )
		{
			const Uint32 lineStringId = line->GetLocalizedContent()->GetIndex();
			const String& lineVO = line->GetVoiceFileNameRef();

			// @todo MS: optimise this, this is O(N2) !!!!!!!!!!!!
			if ( AlreadyParsedLine( lineStringId ) )
			{
				continue;
			}
			linesInSection++;

			if ( GFeedback && m_verbose )
			{
				GFeedback->UpdateTaskInfo( TXT("Line - %s"), lineVO.AsChar() );
			}

			LineEntry lineEntry;
			lineEntry.m_id = lineStringId;
			lineEntry.m_langEntry.Resize( numLangs );

			for ( Uint32 j = 0; j < numLangs; ++j )
			{
				const String& langName = m_chosenLangs[ j ];
				LineLangEntry& langEntry = lineEntry.m_langEntry[ j ];
				const LanguagePack* pack = SLocalizationManager::GetInstance().GetLanguagePackSync( lineStringId, false, langName );

				langEntry.m_lineText		= pack ? pack->GetText()								: TXT( "<None>" );
				langEntry.m_duration		= pack ? pack->GetSpeechBuffer().GetDuration()			: 0.f;
				langEntry.m_fakeDuration	= pack ? line->CalcFakeDuration( langEntry.m_lineText )	: 0.f;
			}

			ParseEntries( lineEntry.m_langEntry, lineEntry.m_maxEntry, m_summary.m_linesSummary );

			if ( firstLineIndex == -1 )
			{
				firstLineIndex = m_lineEntry.SizeInt();
			}

			lineEntry.m_defaultOrder = m_lineEntry.Size();
			m_lineEntry.PushBack( lineEntry );

			lastLineIndex = m_lineEntry.SizeInt();
		}
	}


	if ( firstLineIndex != -1 && lastLineIndex != -1 && !AlreadyParsedSection( section->GetName() ) )
	{
		SectionEntry sectionEntry;
		sectionEntry.m_sectionName = section->GetName();
		sectionEntry.m_langEntry.Resize( numLangs );

		for ( Int32 i = firstLineIndex; i < lastLineIndex; ++i )
		{
			LineEntry& lineEntry = m_lineEntry[ i ];
			lineEntry.m_sectionName = sectionEntry.m_sectionName;

			for ( Uint32 j = 0; j < numLangs; ++j )
			{
				LangEntry& sectionLangEntry = sectionEntry.m_langEntry[ j ];
				LangEntry& lineLangEntry = lineEntry.m_langEntry[ j ];

				sectionLangEntry.m_duration += lineLangEntry.m_duration;
				sectionLangEntry.m_fakeDuration += lineLangEntry.m_fakeDuration;
			}
		}

		ParseEntries( sectionEntry.m_langEntry, sectionEntry.m_maxEntry, m_summary.m_sectionsSummary );
		sectionEntry.m_defaultOrder = m_sectionEntry.Size();
		m_sectionEntry.PushBack( sectionEntry );

		TDynArray< Uint32 >& warnings = m_summary.m_langLinesWarningsPerSection.GetRef( sectionEntry.m_sectionName );
		FillArrayWithZeros( warnings, m_chosenLangs.Size() );
		m_summary.m_sectionsCount++;
	}

	if ( GFeedback && m_verbose )
	{
		GFeedback->EndTask();
	}
}




void CEdSceneLocAnalyser::CollectUsedLangs( TDynArray< String >& langs ) const
{
	const Uint32 num = m_chosenLangs.Size();
	for ( Uint32 i=0; i<num; ++i )
	{
		if ( HasLocForLang( m_chosenLangs[ i ] ) )
		{
			langs.PushBack( m_chosenLangs[ i ] );
		}
	}
}

void CEdSceneLocAnalyser::GenerateTable( BasicExcelWorksheet& worksheet, const CellFormat& warningFormat, const CellFormat& headerFormat )
{
	if ( m_chosenLangs.Empty() )
	{
		worksheet.Cell( 0, 0 )->SetString( "No languages chosen" );
		return;
	}

	if( m_renderOptionShowSections )
	{
		SortSections( m_sectionEntry );
		GenerateTableHeader( worksheet, TXT( "Section" ), headerFormat );
		FillTable< SectionEntry >( worksheet, m_sectionEntry, warningFormat );
	}
	else
	{
		SortSections( m_lineEntry );
		GenerateTableHeader( worksheet, TXT( "Line" ), headerFormat );
		FillTable< LineEntry >( worksheet, m_lineEntry, warningFormat );
	}
}

void CEdSceneLocAnalyser::GenerateTableHTML( wxString& outCode )
{

	outCode += wxT( "<table border=1>" );

	if( m_renderOptionShowSections )
	{
		SortSections( m_sectionEntry );
		GenerateTableHeaderHTML( outCode, TXT( "Section" ) );
		GenerateCommonSLHTML< SectionEntry >( outCode, m_sectionEntry );
	}
	else
	{
		SortSections( m_lineEntry );
		GenerateTableHeaderHTML( outCode, TXT( "Line" ) );
		GenerateCommonSLHTML< LineEntry >( outCode, m_lineEntry );
	}

	outCode += wxT("</table>");
}

String CEdSceneLocAnalyser::GenerateCellColorHTML( const LangEntry& le, Uint32 langID, const String& parentSection, Bool ignoreLangID ) const
{
	Float timeDiff = le.m_timeDiff;
	Float percentDiff = le.m_percentDiff;

	if( !m_renderOptionRealTime )
	{
		timeDiff = le.m_fakeTimeDiff;
		percentDiff = le.m_fakePercentDiff;
	}

	if( !ignoreLangID )
	{
		if ( m_renderOptionRealTime && !m_langsFlag[ langID ] )
		{
			return TXT( " bgcolor=\"#EEEEFF\"" );
		}

		if( langID == GetRefLangId() )
		{
			return TXT( " bgcolor=\"#D3AAD3\"" );
		}
	}

	if( timeDiff <= 0.0f )
	{
		return TXT( " bgcolor=\"#AAFFAA\"" );
	}

	if( ( m_warnForAbsDiff && timeDiff >= m_renderOptionWarnLevel )
		|| ( !m_warnForAbsDiff && percentDiff >= m_renderOptionWarnLevel ) )
	{
		if ( m_renderOptionRealTime && langID >= 0 && langID < m_chosenLangs.Size() )
		{
			if ( m_renderOptionShowSections )
			{
				m_summary.m_totalSectionWarnings++;
				m_summary.m_langSectionTotalWarnings[langID]++;
			}
			else
			{
				m_summary.m_totalLineWarnings++;
				m_summary.m_langLineTotalWarnings[langID]++;
				TDynArray< Uint32 >& warnings = m_summary.m_langLinesWarningsPerSection.GetRef( parentSection );
				warnings[langID]++;
			}
		}
		return TXT( " bgcolor=\"#FFAAAA\"" );
	}

	return TXT( "" );
}

void CEdSceneLocAnalyser::GenerateLangEntryHTML( wxString& outCode, const LangEntry& langEntry ) const
{
	Float duration = langEntry.m_duration;
	Float timeDiff = langEntry.m_timeDiff;
	Float percentDiff = langEntry.m_percentDiff;

	if( !m_renderOptionRealTime )
	{
		duration = langEntry.m_fakeDuration;
		timeDiff = langEntry.m_fakeTimeDiff;
		percentDiff = langEntry.m_fakePercentDiff;
	}

	outCode += wxString::Format( wxT( "<p><b>%1.2f</b></p>" ), duration );

	if( timeDiff >= 0.0f )
	{
		outCode += wxString::Format( wxT( "<p>+%1.2fs</p>" ), timeDiff );
	}
	else
	{
		outCode += wxString::Format( wxT( "<p>%1.2fs</p>" ), timeDiff );
	}

	if( percentDiff >= 0.0f )
	{
		outCode += wxString::Format( wxT( "<p></br>+%1.2f%%</p>" ), percentDiff );
	}
	else
	{
		outCode += wxString::Format( wxT( "<p></br>%1.2f%%</p>" ), percentDiff );
	}
}

void CEdSceneLocAnalyser::GenerateLangSingleEntry( BasicExcelWorksheet& worksheet, Uint32 row, Uint32& column, 
						const LangEntry& langEntry, const Uint32 langId, const String& parentSection, const CellFormat& warningFormat ) const
{
	Bool setWarning = false;
	Float duration = langEntry.m_duration;
	Float timeDiff = langEntry.m_timeDiff;
	Float percentDiff = langEntry.m_percentDiff;

	if ( !m_renderOptionRealTime )
	{
		duration = langEntry.m_fakeDuration;
		timeDiff = langEntry.m_fakeTimeDiff;
		percentDiff = langEntry.m_fakePercentDiff;
	}

	if ( ( m_warnForAbsDiff && timeDiff >= m_renderOptionWarnLevel )
		|| ( !m_warnForAbsDiff && percentDiff >= m_renderOptionWarnLevel ) )
	{
		if ( m_renderOptionRealTime && langId < m_chosenLangs.Size() && langId >= 0 )
		{
			if ( m_renderOptionShowSections )
			{
				m_summary.m_totalSectionWarnings++;
				m_summary.m_langSectionTotalWarnings[langId]++;
			}
			else
			{
				m_summary.m_totalLineWarnings++;
				m_summary.m_langLineTotalWarnings[langId]++;
				TDynArray< Uint32 >& warnings = m_summary.m_langLinesWarningsPerSection.GetRef( parentSection );
				warnings[langId]++;
			}
		}
		worksheet.Cell( row, column )->SetFormat( warningFormat );
		worksheet.Cell( row, column + 1 )->SetFormat( warningFormat );
		worksheet.Cell( row, column + 2 )->SetFormat( warningFormat );
	}

	worksheet.Cell( row, column )->SetDouble( duration );
	if ( langId != GetRefLangId() )
	{
		worksheet.Cell( row, ++column )->SetDouble( timeDiff );
		worksheet.Cell( row, ++column )->SetDouble( percentDiff );
	}
}

void CEdSceneLocAnalyser::GenerateSceneSummaryLine( BasicExcelWorksheet& worksheet, Uint32 row, const CellFormat& warningFormat ) const
{
	Uint32 total = 0, refLangID = GetRefLangId();
	worksheet.Cell( row, 1 )->SetString( "SUMMARY" );

	for ( Uint32 i = 0; i < m_chosenLangs.Size(); ++i )
	{
		if ( i != refLangID )
		{
			if ( m_renderOptionShowSections )
			{
				total += m_summary.m_langSectionTotalWarnings[i];
				UpdateSummaryCell( *( worksheet.Cell( row, i * 3 + 2 ) ), m_summary.m_langSectionTotalWarnings[i], warningFormat );
			}
			else
			{
				total += m_summary.m_langLineTotalWarnings[i];
				UpdateSummaryCell( *( worksheet.Cell( row, i * 3 + 2 ) ), m_summary.m_langLineTotalWarnings[i], warningFormat );
			}
		}
	}

	UpdateSummaryCell( *( worksheet.Cell( row, m_chosenLangs.Size() * 3 + 3  ) ),total, warningFormat );
}

void CEdSceneLocAnalyser::GenerateTableHeaderHTML( wxString& outCode, const String& caption )
{
	outCode += wxString::Format( wxT( "<tr><th width=150 align=left>%s</th>" ), caption.AsChar() );

	for( Uint32 l = 0; l < m_chosenLangs.Size(); ++l )
	{
		GenerateSingleHeaderEntryHTML( outCode, l );
	}

	outCode += wxT( "<th width=80 align=center>MAX</th></tr>" );
}

void CEdSceneLocAnalyser::GenerateTableHeader( BasicExcelWorksheet& worksheet, const String& caption, const CellFormat& headerFormat ) const
{
	UpdateHeaderCell( *( worksheet.Cell( 1, 1 ) ), caption, headerFormat );

	Uint32 column = 1;
	Uint32 refLangId = GetRefLangId();
	String header;

	for( Uint32 l = 0; l < m_chosenLangs.Size(); ++l )
	{
		GenerateSingleHeaderText( header, l );
		UpdateHeaderCell( *( worksheet.Cell( 1, ++column ) ), header + TXT( " duration [s]" ), headerFormat );

		if( l != refLangId )
		{
			UpdateHeaderCell( *( worksheet.Cell( 1, ++column ) ), header + TXT( " time diff [s]" ), headerFormat );
			UpdateHeaderCell( *( worksheet.Cell( 1, ++column ) ), header + TXT( " % diff [%]" ), headerFormat );
		}
	}

	// increment column to seperate summary from the rest of table content
	++column;
	UpdateHeaderCell( *( worksheet.Cell( 1, ++column ) ), TXT( "MAX duration [s]" ), headerFormat );
	UpdateHeaderCell( *( worksheet.Cell( 1, ++column ) ), TXT( "MAX time diff [s]" ), headerFormat );
	UpdateHeaderCell( *( worksheet.Cell( 1, ++column ) ), TXT( "MAX % diff [%]" ), headerFormat );
}

void CEdSceneLocAnalyser::GenerateSingleHeaderEntryHTML( wxString& outCode, Uint32 langID )
{
	if( langID > m_chosenLangs.Size() )
	{
		RED_ASSERT( false, TXT( "Wrong lang ID" ) );
		outCode += wxT( "<th width=70 align=center>ERROR</th>" );
		return;
	}

	outCode += wxT( "<th width=70 align=center" );

	if ( !m_langsFlag[ langID ] )
	{
		outCode += wxT( " bgcolor=\"#FF0000\"" );
	}

	const String& lang = m_chosenLangs[ langID ];

	if( langID == GetRefLangId() )
	{
		outCode += wxString::Format( wxT( ">[%s] REF</th>" ), lang.AsChar() );
	}
	else
	{
		outCode += wxString::Format( wxT( ">[%s]</th>" ), lang.AsChar() );
	}
}

void CEdSceneLocAnalyser::GenerateSingleHeaderText( String& header, Uint32 langID ) const
{
	if( langID > m_chosenLangs.Size() )
	{
		RED_ASSERT( false, TXT( "Wrong lang ID" ) );
		header = TXT( "ERROR" );
		return;
	}

	const String& lang = m_chosenLangs[ langID ];

	if( langID == GetRefLangId() )
	{
		header = String::Printf( TXT( "%ls REF" ), lang.AsChar() );
	}
	else
	{
		header = String::Printf( TXT( "%ls" ), lang.AsChar() );
	}
}

void CEdSceneLocAnalyser::UpdateHeaderCell( BasicExcelCell& cell, const String& headerText, const CellFormat& headerFormat ) const
{
	cell.SetString( UNICODE_TO_ANSI( headerText.AsChar() ) );
	cell.SetFormat( headerFormat );
}

void CEdSceneLocAnalyser::OnCommonEvt( wxCommandEvent& event )
{
	RenderHTML();
}

Int32 CEdSceneLocAnalyser::GetSortByColumn()
{
	if( m_sortBy == TXT( "Max" ) )
	{
		return -1;
	}

	return ( Int32 )GetLangId( m_sortBy );
}

void CEdSceneLocAnalyser::BeginBatchExport()
{
	m_exportIndex.Clear();
}

void CEdSceneLocAnalyser::EndBatchExport()
{
	if ( m_chosenLangs.Size() == 0)
	{
		return;
	}

	{ // export scene summary
		String pathToExport = m_savePath;
		pathToExport.Replace( TXT( ".csv" ), TXT( "" ) );
		pathToExport += TXT( "\\summary.xls" );

		{ // sort output
			struct OrderByWarnings
			{
				RED_INLINE Bool operator()( const ExportIndex& e1, const ExportIndex& e2 )  const { return e1.m_totalLineWarnings > e2.m_totalLineWarnings; }
			};

			Sort( m_exportIndex.Begin(), m_exportIndex.End(), OrderByWarnings() );
		}

		BasicExcel workbook;
		workbook.New( 1 );
		workbook.RenameWorksheet( "Sheet1", "Summary" );
		XLSFormatManager formatManager( workbook );

		CellFormat warningFormat( formatManager );
		warningFormat.set_background( MAKE_COLOR2( EGA_RED, 0 ) );

		ExcelFont boldFont;
		boldFont.set_weight( FW_BOLD );
		CellFormat headerFormat( formatManager );
		headerFormat.set_font( boldFont );

		BasicExcelWorksheet* worksheet = workbook.GetWorksheet( "Summary" );		
		FillSummaryTableHeaders( *worksheet, headerFormat );
		FillSummaryTableContent( *worksheet, warningFormat );	

		if ( !pathToExport.Empty() )
		{
			workbook.SaveAs( UNICODE_TO_ANSI( pathToExport.AsChar() ) );
		}
	}

	wxMessageBox( wxT( "Loc Analyser export finished" ), wxT( "Done" ), wxCENTRE|wxOK );
}

void CEdSceneLocAnalyser::FillSummaryTableHeaders( BasicExcelWorksheet& worksheet, const CellFormat& headerFormat ) const
{
	UpdateHeaderCell( *( worksheet.Cell( 0, 4 ) ), TXT( "CORRUPTED LINES AND SECTIONS COUNT" ), headerFormat );

	Uint32 row = 1, column = 1;
	UpdateHeaderCell( *( worksheet.Cell( row, column ) ), TXT( "SCENE" ), headerFormat );
	UpdateHeaderCell( *( worksheet.Cell( row, ++column ) ), TXT( "lines in file" ), headerFormat );
	UpdateHeaderCell( *( worksheet.Cell( row, ++column ) ), TXT( "sections in file" ), headerFormat );
	UpdateHeaderCell( *( worksheet.Cell( row, ++column ) ), TXT( "TOTAL lines" ), headerFormat );
	UpdateHeaderCell( *( worksheet.Cell( row, ++column ) ), TXT( "TOTAL sections" ), headerFormat );

	Uint32 refLangId = GetRefLangId();
	for ( Uint32 i = 0; i < m_chosenLangs.Size(); ++i )
	{
		if ( i != refLangId )
		{
			String header;
			GenerateSingleHeaderText( header, i );
			UpdateHeaderCell( *( worksheet.Cell( row, ++column ) ), header + TXT( " lines" ), headerFormat );
			UpdateHeaderCell( *( worksheet.Cell( row, ++column ) ), header + TXT( " lines [%]" ), headerFormat );
			UpdateHeaderCell( *( worksheet.Cell( row, ++column ) ), header + TXT( " sections" ), headerFormat );
			UpdateHeaderCell( *( worksheet.Cell( row, ++column ) ), header + TXT( " sections [%]" ), headerFormat );
		}
	}
}

void CEdSceneLocAnalyser::FillArrayWithZeros( TDynArray< Uint32 >& dynArray, const Uint32 size ) const
{
	dynArray.Reserve( size );
	for ( Uint32 i = 0; i < m_chosenLangs.Size(); ++i )
	{
		dynArray.PushBack( 0 );
	}
}

void CEdSceneLocAnalyser::FillSummaryTableContent( BasicExcelWorksheet& worksheet, const CellFormat& warningFormat ) const
{
	Uint32 row = 3;
	FillSummaryTableScenesInfo( worksheet, warningFormat, row );
	FillSummaryTableSectionsInfo( worksheet, warningFormat, row );	
}

void CEdSceneLocAnalyser::FillSummaryTableScenesInfo( BasicExcelWorksheet& worksheet, const CellFormat& warningFormat, Uint32& row ) const
{
	Uint32 totalLineWarnings = 0, totalSectionWarnings = 0, totalLinesCount = 0, totalSectionsCount = 0;
	TDynArray< Uint32 > langLineTotalWarnings, langSectionTotalWarnings;
	FillArrayWithZeros( langLineTotalWarnings, m_chosenLangs.Size() );
	FillArrayWithZeros( langSectionTotalWarnings, m_chosenLangs.Size() );

	// insert summary info for each scene
	Uint32 refLangId = GetRefLangId();
	TDynArray< Uint32 > totalLinesInScene;
	Uint32 column = 0;
	for ( Uint32 i = 0; i < m_exportIndex.Size(); ++i )
	{
		column = 0;
		const ExportIndex& ei = m_exportIndex[ i ];

		TDynArray< Uint32 > linesPerSection;
		ei.m_linesCountInSection.GetValues( linesPerSection );

		totalLinesInScene.PushBack( 0 );
		for ( const Uint32& it : linesPerSection )
		{
			totalLinesInScene.Last() += it;
		}

		worksheet.Cell( ++row, ++column )->SetString( UNICODE_TO_ANSI( ei.m_linkCode.AsChar() ) );
		worksheet.Cell( row, ++column )->SetInteger( totalLinesInScene.Last() );
		worksheet.Cell( row, ++column )->SetInteger( ei.m_sectionsCount );

		UpdateSummaryCell( *( worksheet.Cell( row, ++column ) ), ei.m_totalLineWarnings, warningFormat );
		UpdateSummaryCell( *( worksheet.Cell( row, ++column ) ), ei.m_totalSectionWarnings, warningFormat );

		totalLinesCount += totalLinesInScene.Last();
		totalSectionsCount += ei.m_sectionsCount;
		totalLineWarnings += ei.m_totalLineWarnings;
		totalSectionWarnings += ei.m_totalSectionWarnings;

		for ( Uint32 j = 0; j < m_chosenLangs.Size(); ++j )
		{
			if ( j != refLangId )
			{
				InsertBrokenLines( worksheet, row, ++column, ei.m_langLineTotalWarnings[j], totalLinesInScene.Last(), warningFormat );
				InsertBrokenLines( worksheet, row, ++column, ei.m_langSectionTotalWarnings[j], ei.m_sectionsCount, warningFormat );
				langLineTotalWarnings[j] += ei.m_langLineTotalWarnings[j];
				langSectionTotalWarnings[j] += ei.m_langSectionTotalWarnings[j];
			}
		}
	}

	// insert summary info for the whole report
	column = 0;
	worksheet.Cell( 2, ++column )->SetString( "SUMMARY" );
	worksheet.Cell( 2, ++column )->SetInteger( totalLinesCount );
	worksheet.Cell( 2, ++column )->SetInteger( totalSectionsCount );
	UpdateSummaryCell( *( worksheet.Cell( 2, ++column ) ), totalLineWarnings, warningFormat );
	UpdateSummaryCell( *( worksheet.Cell( 2, ++column ) ), totalSectionWarnings, warningFormat );

	for ( Uint32 j = 0; j < m_chosenLangs.Size(); ++j )
	{
		if ( j != refLangId )
		{
			InsertBrokenLines( worksheet, 2, ++column, langLineTotalWarnings[j], totalLinesCount, warningFormat );
			InsertBrokenLines( worksheet, 2, ++column, langSectionTotalWarnings[j], totalSectionsCount, warningFormat );
		}
	}
}

void CEdSceneLocAnalyser::FillSummaryTableSectionsInfo( BasicExcelWorksheet& worksheet, const CellFormat& warningFormat, Uint32& row ) const
{
	// insert summary info for each section
	++row;
	Uint32 column = 0;
	for ( Uint32 i = 0; i < m_exportIndex.Size(); ++i )
	{
		const ExportIndex& ei = m_exportIndex[ i ];
		TDynArray< Uint32 > linesPerSection;
		ei.m_linesCountInSection.GetValues( linesPerSection );

		Uint32 totalLinesInScene = 0;
		for ( const Uint32& it : linesPerSection )
		{
			totalLinesInScene += it;
		}

		if ( totalLinesInScene == 0 )
		{
			continue;
		}

		worksheet.Cell( ++row, column )->SetString( UNICODE_TO_ANSI( ei.m_linkCode.AsChar() ) );

		Uint32 refLangId = GetRefLangId();
		for ( THashMap< String, TDynArray< Uint32 > >::const_iterator it = ei.m_langLinesWarningsPerSection.Begin(); it != ei.m_langLinesWarningsPerSection.End(); ++it, ++row )
		{
			worksheet.Cell( row, ++column )->SetString( UNICODE_TO_ANSI( it->m_first.AsChar() ) );

			Uint32 lines = 0;
			ei.m_linesCountInSection.Find( it->m_first, lines );
			worksheet.Cell( row, ++column )->SetInteger( lines );

			Uint32 totalLineWarnings = 0;
			column = 6;

			const TDynArray< Uint32 >& warnsCount = it->m_second;
			for ( Uint32 j = 0; j < m_chosenLangs.Size(); ++j )
			{
				if ( j != refLangId )
				{
					InsertBrokenLines( worksheet, row, column, warnsCount[j], lines, warningFormat );
					totalLineWarnings += warnsCount[j];
					column += 3;
				}
			}
			UpdateSummaryCell( *( worksheet.Cell( row, 4 ) ), totalLineWarnings, warningFormat );
			column = 0;
		}
	}
}

void CEdSceneLocAnalyser::UpdateSummaryCell( BasicExcelCell& cell, const Uint32 value, const CellFormat& warningFormat ) const
{
	cell.SetInteger( value );
	if ( value > 5 )
	{
		cell.SetFormat( warningFormat );
	}
}

void CEdSceneLocAnalyser::InsertBrokenLines( BasicExcelWorksheet& worksheet, Uint32 row, Uint32& column, Uint32 brokenLines, Uint32 allLines, const CellFormat& warningFormat ) const
{
	UpdateSummaryCell( *( worksheet.Cell( row, column ) ), brokenLines, warningFormat );
	worksheet.Cell( row, ++column )->SetDouble( allLines != 0 ? 100.0 * ( Float ) brokenLines / ( Float ) allLines : 0 );
}

void CEdSceneLocAnalyser::ExportBatchEntry( const String& entry )
{
	if ( m_chosenLangs.Size() == 0 )
	{
		return;
	}

	CStoryScene* storyScene = LoadResource< CStoryScene >( entry );

	if( storyScene )
	{
		m_summary.Clear();
		FillArrayWithZeros( m_summary.m_langLineTotalWarnings, m_chosenLangs.Size() );
		FillArrayWithZeros( m_summary.m_langSectionTotalWarnings, m_chosenLangs.Size() );

		ParseScene( storyScene );
		if ( m_lineEntry.Empty() )
		{
			return;
		}

		m_renderOptionRealTime = true;
		m_renderOptionShowSections = true;
		BasicExcel workbook;
		workbook.New( 2 );

		XLSFormatManager formatManager( workbook );
		CellFormat warningFormat( formatManager);
		warningFormat.set_background( MAKE_COLOR2( EGA_RED, 0 ) );

		ExcelFont boldFont;
		boldFont.set_weight( FW_BOLD );
		CellFormat headerFormat( formatManager );
		headerFormat.set_font( boldFont );

		workbook.RenameWorksheet( "Sheet1", "RealTimeSections" );
		BasicExcelWorksheet* worksheet = workbook.GetWorksheet( "RealTimeSections" );
		GenerateTable( *worksheet, warningFormat, headerFormat );

		workbook.RenameWorksheet( "Sheet2", "RealTimeLines" );
		worksheet = workbook.GetWorksheet( "RealTimeLines" );
		m_renderOptionShowSections = false;
		GenerateTable( *worksheet, warningFormat, headerFormat );

		String pathToExport = m_savePath;
		String fileName;
		{ // determine export path
			pathToExport.Replace( TXT( ".csv" ), TXT( "" ) );
			TDynArray< String > splEntry = entry.Split( TXT( "\\" ) );
			if( splEntry.Size() == 0 )
			{
				RED_ASSERT( false );
				return;
			}

			fileName = splEntry[ splEntry.Size() - 1 ];
			fileName.Replace( TXT( ".w2scene" ), TXT( ".xls" ) );
			pathToExport += TXT( "\\" );
			pathToExport += fileName;
		}

		if ( !pathToExport.Empty() )
		{
			String tmpFileName = fileName;
			tmpFileName.Replace( TXT( ".xls" ), TXT( "" ) );

			ExportIndex ei( m_summary );
			ei.m_linkCode = tmpFileName;
			m_exportIndex.PushBack( ei );
		}
		workbook.SaveAs( UNICODE_TO_ANSI( pathToExport.AsChar() ) );
	}
}



void CEdSceneLocAnalyser::ShowSetupDialog( wxWindow* parent )
{
	CEdSceneLocAnalyserExportDialog dialog( parent );
	dialog.ShowModal();

	m_renderOptionWarnLevel = dialog.GetWarnLevel();
	m_warnForAbsDiff = dialog.GetWarnForAbsDiff();
	m_sortType = ( ESortType )dialog.GetSortType();
	m_verbose = dialog.GetVerbose();
	m_onlyDLGLines = dialog.GetDLGLinesOnly();
	m_sortBy = TXT( "Max" );
	m_chosenLangs.Clear();
	dialog.GetLanguages( m_chosenLangs );
}

Bool CEdSceneLocAnalyser::IsDLGSection( const CStorySceneSection* section ) const
{
	if( !section )
	{
		return false;
	}

	return ( !section->IsGameplay() && !( section->IsA< CStorySceneCutsceneSection >() || section->IsA< CStorySceneVideoSection >() ) );
}



#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
