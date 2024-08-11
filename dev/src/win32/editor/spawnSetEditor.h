/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#if 0

#include "spawnSetGrid.h"

class CNPCSpawnSet;
class CEdTagEditor;
class CEdTagViewEditor;
class CEdSpawnSetActionEditor;
class ISpawnSetTimetableAction;

class CEdSpawnSetEditor : public wxSmartLayoutPanel, public IEdEventListener
{
	DECLARE_EVENT_TABLE()

protected:
	CSpawnSetGrid                      *m_grid;
	CNPCSpawnSet                       *m_spawnSet;

	// Editors for custom cells
	CEdTagViewEditor                   *m_tagEditor;
	CEdSpawnSetActionEditor            *m_actionEditor;
	CEdPropertiesBrowserWithStatusbar  *m_properties;

public:
	CEdSpawnSetEditor( wxWindow* parent, CNPCSpawnSet* spawnSet );
	~CEdSpawnSetEditor();

    virtual wxString GetShortTitle();

	// Save / Load options from config file
	void SaveOptionsToConfig();
	void LoadOptionsFromConfig();

protected:
	void DispatchEditorEvent( const CName& name, IEdEventData* data );

	void OnFinderUpdated( wxCommandEvent& event );
	void OnFinderCleared( wxCommandEvent& event );
	void OnSave( wxCommandEvent& event );
	void OnExit( wxCommandEvent& event );
	void OnEditCopy( wxCommandEvent& event );
	void OnEditCut( wxCommandEvent& event );
	void OnEditPaste( wxCommandEvent& event );
	void OnEditDelete( wxCommandEvent& event );
	void OnEditUndo( wxCommandEvent& event );
	void OnEditRedo( wxCommandEvent& event );
	void OnInsertRow( wxCommandEvent& event );
	void OnInsertCommentRow( wxCommandEvent& event );
	void OnRemoveCommentRow( wxCommandEvent& event );
	void OnAppendRow( wxCommandEvent& event );

	void AppendRow();
	void OnRemoveRow( wxCommandEvent& event );
	void OnEditCell( wxCommandEvent& event );
	void OnClearCell( wxCommandEvent& event );
	void OnGotoResource( wxCommandEvent& event );
	void OnCellLeftClick( wxGridEvent &event );
	void OnCellRightClick( wxGridEvent &event );
	void OnLabelRightClick( wxGridEvent &event );

	void ShowSubRowSubmenu( Int32 row, Int32 colToReadOnlyCheck, const String &itemText, wxPoint menuPos = wxDefaultPosition );
	void ShowMainSubmenu( Int32 row, Int32 colToReadOnlyCheck, const String &itemText, wxPoint menuPos = wxDefaultPosition );
	void OnCellLeftDoubleClick( wxGridEvent &event );
	void OnCellSelect( wxGridEvent &event );
	void OnCellChange( wxGridEvent &event );
	void OnCellChange( wxSheetEvent &event );
	void OnRangeSelect( wxGridRangeSelectEvent &event );

	void OnAddSubRow( wxCommandEvent& event );
	void OnRemoveCurrentSubRow( wxCommandEvent& event );
	void OnAddBeforeCurrentSubRow( wxCommandEvent& event );
	void OnRemoveLastSubRow( wxCommandEvent& event );

	void OnTagsCanceled( wxCommandEvent &event );
	void OnTagsSaved( wxCommandEvent &event );
	void OnActionSaved( wxCommandEvent &event );
	void OnActionCanceled( wxCommandEvent &event );

	void OnGridMotion( wxMouseEvent& event );
	void OnGridKeyDown( wxKeyEvent& event );
	void OnGridSize( wxSizeEvent& event );
	void OnGridPanelRightMouseClick( wxMouseEvent& event );

	void OnMouseCaptureLost( wxMouseCaptureLostEvent& WXUNUSED(event) );

	enum COLUMN_NAMES { COL_FIRST = 0, COL_COMMENT = 0,
						COL_ENTRY_TAGS = 0, COL_ENTITY_TEMPLATE = 1, COL_ENTITY_APPEARANCES = 2, COL_ENTITY_WEIGHT = 3, COL_CHAR_TAGS = 4, COL_SPAWN_TIME_ARRAY = 5, COL_SPAWN_QUANTITY_ARRAY = 6, 
						COL_SPAWN_TAG_ARRAY = 7, COL_LAYER_ARRAY = 8, COL_LAYER_WEIGHT_ARRAY = 9, COL_USE_EXISTING = 10,
						COL_LAST = 11, COL_COUNT = 11 /* the number of labels */ };

private:
	void CellChange( const int row, const int col ); // saves data from changed cell (UI -> internal data)

	void LoadDataFromResource(); // copies data from 'm_spawnSet' resource to grid editor 'm_grid'

	void RunCustomEditor( const Int32 row, const Int32 col );
	void ContextMenu( Int32 row, Int32 col, wxPoint menuPos = wxDefaultPosition );

	void AddSubRow( Bool addNewEntryToSST = true ); // based on current cell selected

	// helpers, utility functions
	void AddSubRowEntry( Int32 row, Int32 col, Bool addNewEntryToSST = true ); // used by OnAddSubRow() only
	void RemoveSubrowIfEmpty( Int32 row );
	String TagListArrayToString( const TagList& tagList );
	void CalcGridRowLabelValues();
	void SetMainRows();
	
	Int32 GetSpawnSetRowNum( Int32 gridRowNum );
	Int32 GetSpawnSetSubRowNum( Int32 gridRowNum );
	Int32 GetGridMainRowNum( Int32 gridRowNum );

	void InsertNewRowComment( int gridRowNum );
	Bool IsRowComment( int gridRowNum );
	Bool IsRow( Int32 gridRowNum );
	Bool IsSubRow( Int32 gridRowNum );

	void DisableCell( Int32 row, Int32 col );
	void EnableCell( Int32 row, Int32 col, Bool readOnly = false );
	Bool IsCellEnabled( Int32 row, Int32 col );

	void InsertNewRow( Int32 row );
	void InsertNewSubrow( Int32 row );

	void RefreshGrid();
	void RefreshColumn( Int32 col );

	const COLORREF m_disabledCellColor;
	wxMenu *m_menuCell; // Menu "Cell"
	wxMenu *m_menuRow; // Menu "Row"
	wxPanel *m_guiGridPanel;
	static const Int32 TOOLTIP_DELAY = 1500;
	Bool m_omitCellChange;

	// Methods for custom cells.
	//  Print* methods write apropriate value to the cell grid
	//  Clear* methods reset the cell grid value and associated spawn set data as well
	//  Browse* methods open resource (but not editor!) associated with the cell grid value
	Bool SetCellTemplate( Int32 row, Int32 col );
	Bool ClearCellTemplate( Int32 row, Int32 col );
	Bool BrowseCellTemplate( Int32 row, Int32 col );
	Bool SetCellGameTime( Int32 row, Int32 col, GameTime *gameTime /* out */ );
	void PrintCellGameTime( Int32 row, Int32 col, const GameTime *gameTime );
	void PrintCellBool( Int32 row, Int32 col, Bool value );
	Bool GetCellBool( Int32 row, Int32 col );

	// Refactor: full methods for setting cells
	Bool SetCell( Int32 row, Int32 col, Int32 value );
	Bool SetCell( Int32 row, Int32 col, Float value );
	Bool SetCell( Int32 row, Int32 col, Bool value );
	Bool SetCell( Int32 row, Int32 col, const String &value );
	Bool SetCell( Int32 row, Int32 col, CName value );
	Bool SetCell( Int32 row, Int32 col, const GameTime *value );
	Bool SetCell( Int32 row, Int32 col, const CEntityTemplate *value );
	Bool SetCell( Int32 row, Int32 col, const TDynArray<CName> &value );
	Bool SetCell( Int32 row, Int32 col, const TagList& value );

	// Copy/Paste methods
	void CopyCellsInColumn( int columnNumber, int topRow, int bottomRow );
	void CopyCellsInRow( int rowNumber, int leftColumn, int rightColumn );
	void CopyCells( int topLeftCol, int topLeftRow, int bottomRightCol, int bottomRightRow );

	// Auxiliary methods for Copy/Paste
	Bool IsEnoughSpaceInColumn( int columnNumber, int topRow, int bottomRow );

	// Structure for Copy/Paset
	struct SClipboardDataCol
	{
		SClipboardDataCol() : m_columnNumber( 999 ), m_data( NULL ) {}
		~SClipboardDataCol()
		{
			m_columnNumber = 999;
			if ( m_data )
			{
				delete m_data;
				m_data = NULL;
			}
		}

		UInt m_columnNumber;
		void *m_data;
	};

	struct SClipboardDataRow
	{
		SClipboardDataRow()
		{
			for ( Int32 i = COL_FIRST; i < COL_COUNT; ++i )
			{
				m_validMembers[ i ] = false;
			}
		}
		TagList						m_entryTags;
		Int32							m_charactersCount;
		String						m_template;
		Float						m_templateWeight;
		TagList						m_characterTags;
		Bool						m_useExistingCharacters;
		GameTime					m_timeStart;
		GameTime					m_timeEnd;
		TagList						m_spawnPointsTags;
		TagList						m_despawnPointsTags;
		String						m_layer;
		ISpawnSetTimetableAction*	m_action;
		Float						m_actionWeight;

		Bool						m_validMembers[ COL_COUNT ];
	};

	struct SSpawnSetEntry
	{
		TagList										m_entryTags;
		TagList										m_characterTags;
		Bool										m_useExistingCharacters;
		Int32											m_charactersCount;
		String										m_template;
		Float										m_templateWeight;
		GameTime									m_timeStart;
		GameTime									m_timeEnd;
		TagList										m_spawnPointsTags;
		TagList										m_despawnPointsTags;
		String										m_layer;
		TagList										m_areaTags;
		ISpawnSetTimetableAction					*m_action;
		Float										m_actionWeight;
	};

	struct SClipboardData
	{
		//TDynArray< SSpawnSetEntry > m_data;
		//TDynArray< Bool >  m_validMembers[ COL_COUNT ];

		TDynArray< TDynArray< CVariant > > m_data;
	};

	CName GetCellType( Int32 row, Int32 col );
	void* GetCellRawValue( Int32 row, Int32 col );
	void SetCellRawValue( Int32 row, Int32 col, void *data );

	// Commentary entries
};

#endif