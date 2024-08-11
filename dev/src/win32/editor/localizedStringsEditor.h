/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

class CLocalizedStringsWithKeys;

class CLocalizedStringsEditor : public wxSmartLayoutPanel, public IEdEventListener
{
	DECLARE_EVENT_TABLE()

public:
	CLocalizedStringsEditor( wxWindow* parent );
	~CLocalizedStringsEditor();

	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );

	// GUI Methods
	void OnToolBar( wxCommandEvent &event );
	void OnButtonMenuClicked( wxCommandEvent &event );
	void OnStringKeyUpdated( wxCommandEvent &event );
	void OnStringKeyLeaveWindow( wxFocusEvent &event );
	void OnStringCategoryComboSelected( wxCommandEvent &event );
	void OnStringCategoryEnterPressed( wxCommandEvent &event );
	void OnStringValueUpdated( wxCommandEvent &event );
	void OnCategoryFilterChanged( wxCommandEvent &event );
	void OnRemoveEntry( wxCommandEvent &event );
	void OnFind( wxCommandEvent &event );
	void OnMenuExit( wxCommandEvent &event );

protected:
	void SaveOptionsToConfig();
	void LoadOptionsFromConfig();

	void InitializeGui();
	void InitializeGUIEntries();	// obsolete method: 'ReinitializeGUIEntries()' should be used instead
	void ReinitializeGUIEntries();
	void UpdateGUICategories();
	void SaveEntriesToDataBase();

	// GUI factory methods
	void CreateNewEntry();
	void CreateHeader();
	void AppendEntry( Int32 i, Int32 dataIndex );
	void ReuseEntry( Int32 guiIndex, Int32 dataIndex );
	Bool SetEntryValue( Int32 x, Int32 y, const String& text );

	// Pages
	Int32 GetMaxPagesNum();
	Int32 GetCurrentPageNum();
	Bool NextPage(); // returns true on success
	Bool PrevPage(); // returns true on success
	Bool SetCurrentPageNum( Int32 currentPageNum ); // returns true on success

	// Find
	Bool FindAndGoto( const String &text, Bool continueFind = false );

private:
	// GUI: toolbar
	wxToolBar*	m_toolBar;
	const Int32	m_idToolAdd;
	const Int32	m_idToolSave;
	const Int32	m_idNextPage;
	const Int32	m_idPrevPage;
	const Int32	m_idRefresh;
	const Int32	m_idFind;
	const Int32	m_idFindNext;

	// GUI: panels
	wxPanel*			m_panelMain;
	wxSizer*			m_sizerMain;
	wxScrolledWindow*	m_scrolledWindowMain;
	wxSizer*			m_sizerScrolledWindowMain;

	// GUI: header
	TDynArray< String >	m_guiColumnsNames;

	// GUI: entries
	TDynArray< wxTextCtrl * >		m_entriesKeys;			// the list of text controls for 'strings keys' column
	TDynArray< wxComboBox * >		m_entriesCategories;	// the list of combo boxes for 'strings categories' column
	TDynArray< wxTextCtrl * >		m_entriesValues;		// the list of text controls for 'strings values' column
	TDynArray< wxBitmapButton * >	m_entriesMainBtns;		// the list of buttons for buttons column
	TDynArray< wxSizer * >			m_entriesSizers;		// the list of entries sizers (one sizer for one entry)
	TDynArray< wxBitmap >			m_guiButtonsImages;		// bitmaps used for buttons in entries

	// Categories
	wxChoice*		m_choiceCategoriesFilter;	// GUI choice for selecting categories filter
	wxArrayString	m_wxCategories;				// the list of available categories - optimization for combo boxes
	String			m_currentCategoryFilter;	// current category filter value
	const String	m_defaultCategory;			// newly created entries with has this category
	
	// Pages
	Int32				m_numLinesPerPage;			// the maximum number of visible entries per page
	Int32				m_curPageNum;				// the current page number [0, m_maxPagesNum)
	Int32				m_maxPagesNum;				// the current value of maximum pages
	wxStaticText*	m_staticTextPagesInfo;		// GUI text showing info about pages

	// Find
	wxTextCtrl*	m_textCtrlFind;					// GUI text control holding text to find
	Uint32		m_lastFindDataLineNum;			// last find position for "find next" option - data
	Int32			m_lastFindGuiLineNum;			// last find position for "find next" option - GUI

	// GUI and DATA association
	THashMap< Int32, Int32 > m_guiIndexToDataIndex;		// connects GUI entries with data entries

	// Localized Strings data manager
	CLocalizedStringsWithKeys *m_locStrings;
};
