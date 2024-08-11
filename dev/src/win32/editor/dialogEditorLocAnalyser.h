
#pragma once

#include "exporters\storySceneExporter.h"
#include "exporters\localizationExporter.h"



class CEdSceneEditor;
class BasicExcelWorksheet;
class BasicExcelCell;
struct CellFormat;

class CEdSceneLocAnalyserExportDialog : public wxDialog
{
	DECLARE_EVENT_TABLE();

	Bool				m_warnForAbsDiff;
	Float				m_warnLevel;
	Uint32				m_sortType;
	Bool				m_verbose;
	Bool				m_onlyDLGLines;
	TDynArray< String > m_languages;

	wxCheckListBox*		m_DLALanguageCheckList;

public:
	CEdSceneLocAnalyserExportDialog( wxWindow* parent );
	void OnExport( wxCommandEvent& event );
	void OnAllLanguagesCheck( wxCommandEvent& event );

	Bool GetWarnForAbsDiff() const { return m_warnForAbsDiff; }
	Float GetWarnLevel() const { return m_warnLevel; }
	Uint32 GetSortType() const { return m_sortType; }
	Bool GetVerbose() const { return m_verbose; }
	void GetLanguages( TDynArray< String >& languages ) const { languages.PushBack( m_languages ); }
	Bool GetDLGLinesOnly() const { return m_onlyDLGLines; }
};



class CEdSceneLocAnalyser : public CStorySceneExporter, public AbstractLocalizationExporter
{
public:

	enum ESortType
	{
		ST_DEFAULT,
		ST_ABSOLUTE,
		ST_PERCENT
	};

private:
	struct LangEntry
	{
		Float						m_duration;
		Float						m_fakeDuration;
		Float						m_timeDiff;
		Float						m_percentDiff;
		Float						m_fakeTimeDiff;
		Float						m_fakePercentDiff;

		LangEntry();
	};

	struct LineLangEntry : public LangEntry
	{
		String						m_lineText;
	};

	struct LineEntry
	{
		Uint32						m_id;
		Uint32						m_defaultOrder;
		TDynArray< LineLangEntry >	m_langEntry;
		LineLangEntry				m_maxEntry;
		String						m_sectionName;

		const String& GetCaption( Uint32 refID ) const { return m_langEntry[ refID ].m_lineText; }
		LineEntry() : m_defaultOrder( 0 ) {}
	};

	struct SectionEntry
	{
		String						m_sectionName;
		Uint32						m_defaultOrder;
		TDynArray< LangEntry >		m_langEntry;
		LangEntry					m_maxEntry;

		const String& GetCaption( Uint32 refID ) const { RED_UNUSED( refID ); return m_sectionName; }
		SectionEntry() : m_defaultOrder( 0 ) {}
	};

	struct SummaryEntry
	{
		Float						m_maxRealAbsDiff;
		Float						m_maxRealPercentDiff;
		Float						m_maxFakeAbsDiff;
		Float						m_maxFakePercentDiff;

		SummaryEntry();
		void Clear();
	};

	struct Summary
	{
		Uint32										m_totalLineWarnings;
		Uint32										m_totalSectionWarnings;
		THashMap< String, Uint32 >					m_linesCountInSection;
		Uint32										m_sectionsCount;
		TDynArray< Uint32 >							m_langSectionTotalWarnings;
		TDynArray< Uint32 >							m_langLineTotalWarnings;

		THashMap< String, TDynArray< Uint32 > >		m_langLinesWarningsPerSection;

		SummaryEntry								m_sectionsSummary;
		SummaryEntry								m_linesSummary;

		Summary();
		void Clear();
	};

	struct ExportIndex	: public Summary
	{
		String						m_linkCode;

		ExportIndex() {}
		ExportIndex( const Summary& s );
	};

private:
	Bool						m_enabled;
	wxPanel*					m_window;

	TDynArray< LineEntry >		m_lineEntry;
	TDynArray< SectionEntry >	m_sectionEntry;

	TDynArray< String >			m_langs;
	TDynArray< String >			m_chosenLangs;
	TDynArray< Bool >			m_langsFlag;
	mutable Int32				m_referenceLangID;

	Bool						m_renderOptionShowSections;
	Bool						m_warnForAbsDiff;
	Bool						m_renderOptionRealTime;
	Float						m_renderOptionWarnLevel;

	ESortType					m_sortType;
	String						m_sortBy;
	
	mutable Summary				m_summary;
	TDynArray< ExportIndex >	m_exportIndex;

	Bool						m_verbose;

	Bool						m_showAllLanguages;
	Bool						m_onlyDLGLines;

public:
	CEdSceneLocAnalyser();
	~CEdSceneLocAnalyser();

public:
	void Enable( Bool flag );
	Bool IsEnabled() const;

public:
	void BindToWindow( CEdSceneEditor* ed, wxPanel* window );
	Bool IsBindedToWindow() const;

	void ParseScene( const CStoryScene* scene );
	void CollectUsedLangs( TDynArray< String >& langs ) const;

private:
	void ParseSection( const CStorySceneSection* section );

	template< typename T >
	void ParseEntries( TDynArray< T >& langEntry, LangEntry& maxEntry, SummaryEntry& summary );

private:
	void RenderHTML();
	void PickDefaultLanguages();
	
private:
	Uint32 GetRefLangId() const;
	Uint32 GetCurrLangId() const;
	Uint32 GetLangId( const String& lang ) const;
	
	Bool AlreadyParsedLine( Uint32 stringId ) const;
	Bool AlreadyParsedSection( const String& sectionName ) const;
	Bool HasLocForLang( const String& lang ) const;
	
private:
	void GenerateTableHTML( wxString& outCode );
	String GenerateCellColorHTML( const LangEntry& le, Uint32 langID, const String& parentSectionName, Bool ignoreLangID = false ) const;
	void GenerateLangEntryHTML( wxString& outCode, const LangEntry& langEntry ) const;
	void GenerateTableHeaderHTML( wxString& outCode, const String& caption );
	void GenerateSingleHeaderEntryHTML( wxString& outCode, Uint32 langID );

	void GenerateTable( BasicExcelWorksheet& worksheet, const CellFormat& warningFormat, const CellFormat& headerFormat );
	void GenerateLangSingleEntry( BasicExcelWorksheet& worksheet, Uint32 row, Uint32& column, const LangEntry& langEntry, const Uint32 langId, const String& parentSection, const CellFormat& warningFormat ) const;
	void GenerateTableHeader( BasicExcelWorksheet& worksheet, const String& caption, const CellFormat& headerFormat ) const;
	void GenerateSingleHeaderText( String& header, Uint32 langID ) const;
	void UpdateHeaderCell( BasicExcelCell& cell, const String& headerText, const CellFormat& headerFormat ) const;
	void GenerateSceneSummaryLine( BasicExcelWorksheet& worksheet, Uint32 row, const CellFormat& warningFormat ) const;

	void FillSummaryTableHeaders( BasicExcelWorksheet& worksheet, const CellFormat& headerFormat ) const;
	void FillSummaryTableContent( BasicExcelWorksheet& worksheet, const CellFormat& warninigFormat ) const;
	void FillSummaryTableScenesInfo( BasicExcelWorksheet& worksheet, const CellFormat& warninigFormat, Uint32& row ) const;
	void FillSummaryTableSectionsInfo( BasicExcelWorksheet& worksheet, const CellFormat& warninigFormat, Uint32& row ) const;
	void UpdateSummaryCell( BasicExcelCell& cell, const Uint32 value, const CellFormat& warningFormat ) const;
	void InsertBrokenLines( BasicExcelWorksheet& worksheet, Uint32 row, Uint32& column, Uint32 brokenLines, Uint32 allLines, const CellFormat& warning ) const;
	void FillArrayWithZeros( TDynArray< Uint32 >& dynArray, const Uint32 size ) const;

	template< typename T >
	void GenerateCommonSLHTML( wxString& outCode, const TDynArray< T >& entries ) const;

	template< typename T >
	void GenerateCommonSingleEntryHTML( wxString& outCode, const T& langEntry, Uint32 j ) const;

	template< typename T >
	void FillTable( BasicExcelWorksheet& worksheet, const TDynArray< T >& entries, const CellFormat& warningFormat ) const;

public:
	void OnCommonEvt( wxCommandEvent& event );

private:

	Int32 GetSortByColumn();

	template< typename T >
	void SortSections( TDynArray< T >& sortList );

public:
	virtual Bool ExportToDirectory() { return true; }
	virtual void ShowSetupDialog( wxWindow* parent );

protected:
	virtual Bool	CanExportFile( CDiskFile* file ) const { return file != NULL && file->GetFileName().EndsWith( TXT( ".w2scene" ) ); }
	virtual String	GetResourceExtension() const { return TXT( ".w2scene" ); }

public:
	virtual void BeginBatchExport();
	virtual void EndBatchExport();
	virtual void ExportBatchEntry ( const String& entry );

	// @todo MS
	virtual Bool IsBatchEntryValid( const String& entry ) const { return true; }
	// @todo MS
	virtual Bool ValidateBatchEntry( const String& entry ) { return true; }

	virtual void PushBatchEntryError( const String& entry, const String& errorMsg ) {}

private:

	Bool IsDLGSection( const CStorySceneSection* section ) const;
};


template< typename T >
void CEdSceneLocAnalyser::GenerateCommonSLHTML( wxString& outCode, const TDynArray< T >& entries ) const
{
	for( Uint32 i = 0; i < entries.Size(); ++i )
	{
		const T& singleEntry = entries[ i ];
		outCode += wxString::Format( wxT( "<tr><th align=left>%s</th>" ), singleEntry.GetCaption( GetRefLangId() ).AsChar() );

		for( Uint32 j = 0; j < m_chosenLangs.Size(); ++j )
		{
			GenerateCommonSingleEntryHTML( outCode, singleEntry, j );
		}

		{ // max
			outCode += wxString::Format( wxT( "<th%s>" ), GenerateCellColorHTML( singleEntry.m_maxEntry, 0, singleEntry.m_sectionName, true ).AsChar() );	
			GenerateLangEntryHTML( outCode, singleEntry.m_maxEntry );
			outCode += wxT( "</th>" );
		}

		outCode += wxT( "</tr>" );
	}
}

template< typename T >
void CEdSceneLocAnalyser::GenerateCommonSingleEntryHTML( wxString& outCode, const T& singleEntry, Uint32 j ) const
{
	const LangEntry& langEntry = singleEntry.m_langEntry[ j ];
	outCode += wxString::Format( wxT( "<th%s>" ), GenerateCellColorHTML( langEntry, j, singleEntry.m_sectionName ).AsChar() );			
	GenerateLangEntryHTML( outCode, langEntry );
	outCode += wxT( "</th>" );
}


template< typename T >
void CEdSceneLocAnalyser::FillTable( BasicExcelWorksheet& worksheet, const TDynArray< T >& entries, const CellFormat& warningFormat ) const
{
	Uint32 row = 1;
	String currSectionName;
	for( Uint32 i = 0; i < entries.Size(); ++i )
	{
		Uint32 column = 0;

		const T& singleEntry = entries[ i ];
		if ( !m_renderOptionShowSections )
		{
			if ( currSectionName != singleEntry.m_sectionName )
			{
				++row;
				currSectionName = singleEntry.m_sectionName;
			}
			worksheet.Cell( row + 1, column )->SetString( UNICODE_TO_ANSI( currSectionName.AsChar() ) );
		}
		worksheet.Cell( ++row, ++column )->SetString( UNICODE_TO_ANSI( singleEntry.GetCaption( GetRefLangId() ).AsChar() ) );
		++column;

		for( Uint32 l = 0; l < m_chosenLangs.Size(); ++l, ++column )
		{
			GenerateLangSingleEntry( worksheet, row, column, singleEntry.m_langEntry[l], l, singleEntry.m_sectionName, warningFormat );
		}

		// max	
		++column;
		GenerateLangSingleEntry( worksheet, row, column, singleEntry.m_maxEntry, -1, singleEntry.m_sectionName, warningFormat );
	}

	++row;
	if ( m_renderOptionRealTime )
	{
		GenerateSceneSummaryLine( worksheet, ++row, warningFormat );
	}
}

template< typename T >
void CEdSceneLocAnalyser::ParseEntries( TDynArray< T >& langEntry, LangEntry& maxEntry, SummaryEntry& summary )
{
	const Uint32 numLangs = m_chosenLangs.Size();

	LangEntry& refEntry = langEntry[ GetRefLangId() ];

	for ( Uint32 j = 0; j < numLangs; ++j )
	{
		if( j == GetRefLangId() )
		{
			continue;
		}

		LangEntry& entry = langEntry[ j ];

		if( refEntry.m_duration > 0.0f && entry.m_duration > 0.0f )
		{
			entry.m_timeDiff = entry.m_duration - refEntry.m_duration;
			entry.m_percentDiff	= ( entry.m_timeDiff / refEntry.m_duration ) * 100.0f;
		}
		else
		{
			// no ref lang loaded
			entry.m_timeDiff = 0.0f;
			entry.m_percentDiff = 0.0f;
		}
		
		if( refEntry.m_fakeDuration > 0.0f && entry.m_fakeDuration > 0.0f )
		{
			entry.m_fakeTimeDiff = entry.m_fakeDuration - refEntry.m_fakeDuration;
			entry.m_fakePercentDiff = ( entry.m_fakeTimeDiff / refEntry.m_fakeDuration ) * 100.0f;
		}
		else
		{
			// no ref lang fake loaded
			entry.m_fakeTimeDiff = 0.0f;
			entry.m_fakePercentDiff = 0.0f;
		}

		if( entry.m_duration > maxEntry.m_duration )				maxEntry.m_duration = entry.m_duration;
		if( entry.m_fakeDuration > maxEntry.m_fakeDuration )		maxEntry.m_fakeDuration = entry.m_fakeDuration;
		if( entry.m_timeDiff > maxEntry.m_timeDiff )				maxEntry.m_timeDiff = entry.m_timeDiff;
		if( entry.m_percentDiff > maxEntry.m_percentDiff )			maxEntry.m_percentDiff = entry.m_percentDiff;
		if( entry.m_fakeTimeDiff > maxEntry.m_fakeTimeDiff )		maxEntry.m_fakeTimeDiff = entry.m_fakeTimeDiff;
		if( entry.m_fakePercentDiff > maxEntry.m_fakePercentDiff )	maxEntry.m_fakePercentDiff = entry.m_fakePercentDiff;
	}

	if( maxEntry.m_timeDiff > summary.m_maxRealAbsDiff )			summary.m_maxRealAbsDiff = maxEntry.m_timeDiff;
	if( maxEntry.m_percentDiff > summary.m_maxRealPercentDiff )		summary.m_maxRealPercentDiff = maxEntry.m_percentDiff;
	if( maxEntry.m_fakeTimeDiff > summary.m_maxFakeAbsDiff )		summary.m_maxFakeAbsDiff = maxEntry.m_fakeTimeDiff;
	if( maxEntry.m_fakePercentDiff > summary.m_maxFakePercentDiff )	summary.m_maxFakePercentDiff = maxEntry.m_fakePercentDiff;
}

template< typename T >
void CEdSceneLocAnalyser::SortSections( TDynArray< T >& sortList )
{	
	if( m_sortType == ST_PERCENT )
	{
		struct OrderPercent
		{
			Int32	m_sortByColumn;
			Bool	m_fake;

			OrderPercent( Int32 sortByColumn, Bool fake ) : m_sortByColumn ( sortByColumn ), m_fake( fake ) {}

			RED_INLINE Bool operator()( const T& e1, const T& e2 )  const
			{
				if( m_fake )
				{
					if( m_sortByColumn == -1 )
					{
						return e1.m_maxEntry.m_fakePercentDiff > e2.m_maxEntry.m_fakePercentDiff;
					}

					return e1.m_langEntry[ m_sortByColumn ].m_fakePercentDiff > e2.m_langEntry[ m_sortByColumn ].m_fakePercentDiff;
				}
				else
				{
					if( m_sortByColumn == -1 )
					{
						return e1.m_maxEntry.m_percentDiff > e2.m_maxEntry.m_percentDiff;
					}

					return e1.m_langEntry[ m_sortByColumn ].m_percentDiff > e2.m_langEntry[ m_sortByColumn ].m_percentDiff;
				}
			}
		};

		Sort( sortList.Begin(), sortList.End(), OrderPercent( GetSortByColumn(), !m_renderOptionRealTime) );
	}
	else if( m_sortType == ST_ABSOLUTE )
	{
		struct OrderAbsolute
		{
			Int32	m_sortByColumn;
			Bool	m_fake;

			OrderAbsolute( Int32 sortByColumn, Bool fake ) : m_sortByColumn ( sortByColumn ), m_fake( fake ) {}

			RED_INLINE Bool operator()( const T& e1, const T& e2 )  const
			{
				if( m_fake )
				{
					if( m_sortByColumn == -1 )
					{
						return e1.m_maxEntry.m_fakeTimeDiff > e2.m_maxEntry.m_fakeTimeDiff;
					}

					return e1.m_langEntry[ m_sortByColumn ].m_fakeTimeDiff > e2.m_langEntry[ m_sortByColumn ].m_fakeTimeDiff;
				}
				else
				{
					if( m_sortByColumn == -1 )
					{
						return e1.m_maxEntry.m_timeDiff > e2.m_maxEntry.m_timeDiff;
					}

					return e1.m_langEntry[ m_sortByColumn ].m_timeDiff > e2.m_langEntry[ m_sortByColumn ].m_timeDiff;
				}
			}
		};

		Sort( sortList.Begin(), sortList.End(), OrderAbsolute( GetSortByColumn(), !m_renderOptionRealTime) );
	}
	else
	{
		RED_ASSERT( m_sortType == ST_DEFAULT );
		struct OrderDefault
		{
			RED_INLINE Bool operator()( const T& e1, const T& e2 )  const { return e1.m_defaultOrder < e2.m_defaultOrder; }
		};

		Sort( sortList.Begin(), sortList.End(), OrderDefault() );
	}
}
