/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_DATA_VALIDATION

#include "dataError.h"

RED_DECLARE_NAME( ShowDataErrorReporter );

enum EDataErrorColumn
{
	DEC_UID,
	DEC_AssetName,
	DEC_Priority,
	DEC_LastEditBy,
	DEC_Hit,
	DEC_Description,
	DEC_Path,

	DEC_Count,
};

namespace
{
	typedef TPair< wxGrid*, Uint32 > SingleSearchResult;
	typedef TDynArray< SingleSearchResult > SearchResults;
}

class CEdDataErrorReporterWindow : public wxDialog, public IEdEventListener, public IDataErrorListener
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Editor );

public:
	CEdDataErrorReporterWindow( wxWindow* parent );
	~CEdDataErrorReporterWindow();

	void ShowWindow();
	void RequestClearContent();
	void UpdateContent();

	// implement IDataErrorListener interface
	virtual void OnDataErrorReported( const SDataError& error );
	virtual void StartProcessing();
	virtual void ProcessDataErrors( const TDynArray< SDataError >& errors );
	virtual void StoreNonProcessedErrors( const TDynArray< SDataError >& errors );
	virtual void StopProcessing();

private:
	// Implement interface IEdEventListener
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );

	// process bugs, connect it to category and get info to grid
	wxGrid* ProcessCategory( const String& categoryName );
	void ProcessBugs( const SDataError& dataError, wxGrid* categoryGrid );
	void UpdateSummaryLabel();

	Bool CollectResourcesPaths( TDynArray< String >& resourcesPaths );
	void CheckDataErrorsForSpecifiedResources( const TDynArray< String > resourcesPaths );

	Bool GetLocalPathToSaveFile( String& localDepotPath );
	void SaveExportedErrors( C2dArray* bugsInfo, const String& localDepotPath );

	void FillBugsInfoHeaders( C2dArray* bugsInfo );
	void FillBugsInfo( C2dArray* bugsInfo, const THashMap< String, TDynArray< Uint32 > >& categoriesErrorsUids, Bool includeNonProcessedErrors );
	void InsertNewCategoryToBugsInfo( C2dArray* bugsInfo, const String& categoryName );
	void FillBugsInfoRow( C2dArray* bugsInfo, const SDataError& error );
	void FillBugsInfoRow( C2dArray* bugsInfo, const wxGrid* grid, const Uint32 gridRow );

	void CollectErrorsUidsFromGrid( THashMap< String, TDynArray< Uint32 > >& errorsUids, const String& categoryName, Bool checkPriority );
	void CollectNonProcessedErrorsUids( THashMap< String, TDynArray< Uint32 > >& errorsUids );

	// callback function for wxWidgets controls
	void OnSelectCategoryFilter( wxCommandEvent& event );
	void OnSelectPriorityFilter( wxCommandEvent& event );
	void OnSearchButtonClicked( wxCommandEvent& event );
	void OnSearchEnterClicked( wxCommandEvent& event );
	void OnExportToCSVButtonClicked( wxCommandEvent& event );
	void OnContinuousReportingClicked( wxCommandEvent& event );
	void OnAdditionalReportingButtonClicked( wxCommandEvent& event );
	void OnSelectPreviousResult( wxCommandEvent& event );
	void OnSelectNextResult( wxCommandEvent& event );
	void OnColumnPopupClick( wxCommandEvent &event );
	void OnRowPopupClick( wxCommandEvent &event );
	void OnRefresh( wxCommandEvent& event );
	void OnCheckErrorsButtonClicked( wxCommandEvent& event );
	void OnExportAllButtonClicked( wxCommandEvent& event );
	void OnClearContent( wxCommandEvent& event );
	void OnSelectRowInGrid( wxGridEvent& event );
	void OnSortByColumn( wxGridEvent& event );
	void OnColumnSizeChanged( wxGridSizeEvent& event );
	void OnShowColumnContextMenu( wxGridEvent& event );
	void OnShowRowContextMenu( wxGridEvent& event );

	// helper functions
	void ShowInAssetBrowser();
	void ShowInExplorer();
	void ShowAllResourceInstances();

	void SelectRow( wxGrid* category, Uint32 rowIndex );
	void ClearSearchResult();
	void UpdateSearchLabel();
	void InternalRefresh();
	void FlushAllCoughtIssue();
	void SkipToSelectedRow();
	
	void AddDataErrorToWindow( const SDataError& dataError );
	void StorageDataError( const SDataError& dataError );

	Int32 SearchExistingError( Uint32 uid, wxGrid* grid ) const;

	void RefreshWindow();	

	// wx controls
	wxCheckBox*						m_criticalFilter;			//!<
	wxCheckBox*						m_errorFilter;				//!<
	wxCheckBox*						m_warningFilter;			//!<
	wxCheckBox*						m_bugsFilter;				//!<
	wxCheckBox*						m_continuousReporting;		//!< 
	wxCheckBox*						m_additionalReporting;		//!< 
	wxButton*						m_clearContent;				//!<
	wxButton*						m_refresh;					//!<
	wxButton*						m_export;					//!< 
	wxButton*						m_checkErrors;				//!<
	wxButton*						m_exportAll;				//!<

	wxCheckListBox*					m_categoryFilterList;		//!< 
	wxSplitterWindow*				m_splitterWindow;			//!<
	wxScrolledWindow*				m_bugsPanel;				//!< 

	wxTextCtrl*						m_searchLine;				//!< 
	wxButton*						m_searchButton;				//!< 
	wxButton*						m_leftResult;				//!< 
	wxButton*						m_rightResult;				//!< 
	wxStaticText*					m_searchResultsLabel;		//!< 
	wxStaticText*					m_summaryLabel;

	wxMenu*							m_columnContextMenu;		//!< 
	wxMenu*							m_rowContextMenu;			//!< 

	THashMap< String, wxGrid* >		m_bugsList;					//!< 
	THashMap< String, wxPanel* >	m_categoriesPanels;			//!< 
	TDynArray< SDataError >			m_dataErrorsNotDisplayed;
	THashMap< Uint32, SDataError >	m_dataErrorsNotProcessed;
	Uint32							m_displayedErrors;

	wxGrid*							m_selectedCategory;			//!< 
	Int32							m_selectedRowInCategory;	//!< 

	SearchResults					m_searchingResults;			//!< 
	Int32							m_activeSelectedResult;		//!< 

	Bool							m_notClearBugs;				//!< 
	Bool							m_flushAllToWindow;			//!< 

	Bool							m_priorities[ DES_Count ];	//!< 
	Uint32							m_displayedErrorsLimit;
};

#endif	// NO_DATA_VALIDATION
