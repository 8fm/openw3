/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include <wx/htmllbox.h>
#include "../../common/game/communitySystem.h"


class CMaraudersMapCanvas;
class CMaraudersMapItemBase;
class CAIHistoryDebugCanvas;
struct SAIEvent;

///////////////////////////////////////////////////////////////////////////////

class CMaraudersMap : public wxSmartLayoutPanel, public IEdEventListener
{
	DECLARE_EVENT_TABLE()

public:
	CMaraudersMap( wxWindow* parent );
	~CMaraudersMap();

	typedef SAgentStub SAgentStub;

	void SaveOptionsToConfig();
	void LoadOptionsFromConfig();

	void OnShow( wxShowEvent& event );
	void OnExit( wxCommandEvent &event );
	void OnHelp( wxCommandEvent &event );
	void OnFilters( wxCommandEvent &event );
	void OnFiltersType( wxCommandEvent &event );
	void OnWindowShowHide( wxCommandEvent &event );
	void OnPlayerTeleport( wxCommandEvent &event );
	void OnCameraTeleport( wxCommandEvent &event );
	void OnGotoMap( wxCommandEvent &event );
	void OnGotoCamera( wxCommandEvent &event );
	void OnFollowPlayer( wxCommandEvent &event );
	void OnSavePlayerPos( wxCommandEvent &event );
	void OnRestorePlayerPos( wxCommandEvent &event );
	void OnClearPlayerPos( wxCommandEvent &event );

	void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );

	//! Returns currently selected item
	const CMaraudersMapItemBase* GetSelectedItem();

	//! Returns the last logged game start time
	RED_INLINE Float GetGameStartTime() const { return m_gameStartTime; }

	//! Displays AI event description
	void DisplayAIEventDescription( const SAIEvent& event );

protected:
	void DispatchEditorEvent( const CName& name, IEdEventData* data );

private:
	void OnTimer( wxCommandEvent &event );
	void OnClose( wxCloseEvent &event );
	void OnChoiceRefreshRate( wxCommandEvent &event );
	void OnTextCtrlGameTimeChange( wxCommandEvent &event );

	// Side panel
	void UpdateSidePanel();

	// Global info window
	void OnDescriptionWinLinkClickedGlobalInfo( wxHtmlLinkEvent &event );
	void SetGlobalInfoDescription( const wxString &text );
	void FillGlobalInfoWindow();

	// Layers window
	void OnLayersWindowRightMouseClick( wxMouseEvent &event );
	void OnLayersWindowDeselectAll( wxCommandEvent &event );
	void OnLayersWindowSelectAll( wxCommandEvent &event );
	void OnCheckListBoxLayers( wxCommandEvent &event );

	// Description window
	void OnDescriptionWinLinkClicked( wxHtmlLinkEvent &event );
	void SetCurrentDescription( const wxString &text );

	// Find panel
	void OnFindTextInput( wxCommandEvent &event );
	void OnFindTextEnter( wxCommandEvent &event );
	void OnFindItemsSelected( wxCommandEvent &event );
	void OnFindItemsDoubleclicked( wxCommandEvent &event );
	void UpdateFindPanel( Bool forceUpdate = false );
	CMaraudersMapItemBase* GetSelectedItemOnFindListBox();

	void Init();

	// Filters
	void UpdateFiltersMenu();
	void UpdateFiltersTypeMenu();

	// Layers
	Bool IsLayerHidden( const CLayerInfo *layerInfo ) const;

	// Utility methods
	int GetRefreshFactor();
	void SetLookAtInfo( const wxString &text );

	// Goto
	void GotoMapFromComboBox();
	void GotoCameraFromComboBox();
	Bool GetGotoWorldPositionVector( Vector &vec );
	void GotoMap( Vector &gotoVec );
	void GotoCamera( Vector &gotoVec );
	void SavePlayerPos();
	void RestorePlayerPos( const String &savedPlayerPosName );
	void ClearPlayerPos();
	void ClearSavedPlayerPosMenu();

	// Toolbar
	void OnToolBar( wxCommandEvent &event );
	void OnToolbarComboBoxEnter( wxCommandEvent &event );
	void ToolbarSetCurrentGameplayTime();

private:
	Float             m_gameStartTime;

	// GUI elements
	wxPanel          *m_panelMain;
	wxPanel          *m_panelSide;
	wxNotebook       *m_toolsNotebook;
	wxSplitterWindow *m_splitterMain;
	wxMenuBar        *m_menuBar;

	// Description panel
	wxStaticText     *m_descriptionLabel;
	wxHtmlWindow     *m_descriptionWindow; // current selected item description window
	wxSizer          *m_descriptionSizer;

	// Look at panel
	wxStaticText     *m_lookAtLabel;
	wxHtmlWindow     *m_lookAtWindow;
	wxSizer          *m_lookAtSizer;

	// Global info panel
	wxStaticText     *m_globalInfoLabel;
	wxHtmlWindow	 *m_globalInfoWindow;
	wxSizer          *m_globalInfoSizer;
	wxCheckBox		 *m_globalInfoMappinsCheckBox;

	// Layers panel
	wxStaticText     *m_layersLabel;
	wxCheckListBox   *m_layersWindow;
	wxSizer          *m_layersSizer;

	// Find panel
	wxStaticText     *m_findLabel;
	wxCheckBox       *m_findUseTags;
	wxCheckBox       *m_findOnEnterSearch;
	wxTextCtrl       *m_findTextInput;
	wxListBox        *m_findItemsList;
	wxSizer          *m_findSizer;
	wxArrayString     m_findItemsAll;
	wxArrayString     m_findItemsFiltered;
	TDynArray< Uint32 > m_findItemsIndexes;

	// Window menu
	wxMenu *m_windowMenu;
	Int32		m_windowMenuItemGlobalInfoId;
	Int32     m_windowMenuItemLayersId;
	Int32     m_windowMenuItemDescriptionId;
	Int32     m_windowMenuItemFindId;
	Int32		m_windowMenuItemLookAtId;

	// Tools menu
	wxMenu     *m_toolsMenu;
	wxMenu     *m_savedPlayerPosMenu;
	wxChoice   *m_choiceRefreshRate;
	wxComboBox *m_comboBoxToolbar;
	wxTextCtrl *m_textCtrlTime;
	wxTextCtrl *m_textCtrlWorldPosition;

	// Filters type menu
	wxMenu *m_filtersTypeMenu;
	Int32     m_filtersTypeMenuItemProcessing;
	Int32     m_filtersTypeMenuItemVisibility;
	Int32     m_filtersTypeMenuItemSelecting;
	Int32     m_filtersTypeMenuItemFinding;

	wxToolBar  *m_toolBar;
	const Int32   m_idAutoStartTool;
	const Int32   m_idStartTool;
	const Int32   m_idPauseTool;
	const Int32   m_idStopTool;
	const Int32   m_idTimeScaleUpTool;
	const Int32   m_idTimeScaleDownTool;
	const Int32   m_idSplitterTool;
	const Int32   m_idGotoTool;
	Int32         m_lastKnownSashPos;

	// Panels enabled status
	Bool m_isGlobalInfoPanelVisible;
	Bool m_isLayersPanelVisible;
	Bool m_isDescriptionPanelVisible;
	Bool m_isFindPanelVisible;
	Bool m_isLookAtPanelVisible;

	// Filters
	enum EFilters
	{
		FILTER_ENCOUNTER_ITEMS, FILTER_ACTION_POINTS_ITEMS, FILTER_MESH_ITEMS, FILTER_WAYPOINT_ITEMS,
		FILTER_STICKER_ITEMS, FILTER_PLAYER, FILTER_NPCS, FILTER_AGENT_STUBS, FILTER_DENIED_AREAS,
		FILTER_DOOR_ITEMS, FILTER_COMMUNITY_AREAS,
		FILTER_SIZE
	};
	enum EFiltersType
	{
		FILTERS_TYPE_PROCESSING, FILTERS_TYPE_VISIBILITY, FILTERS_TYPE_SELECTING, FILTERS_TYPE_FINDING, FILTERS_TYPE_SIZE
	};
	String GetFriendlyFilterName( EFilters filter );
	String GetFriendlyFilterName( EFilters filter, EFiltersType filterType );
	Bool IsFilterEnabled( EFilters filter, EFiltersType filterType );
	void SwitchFilter( EFilters filter, EFiltersType filterType, Bool enable );
	TSet< EFilters > m_filtersEnabledProcessing;
	TSet< EFilters > m_filtersEnabledVisibility;
	TSet< EFilters > m_filtersEnabledSelecting;
	TSet< EFilters > m_filtersEnabledFinding;
	EFiltersType m_filtersTypeCurrent;

	// General
	CEdTimer*						m_timer;
	CMaraudersMapCanvas*			m_canvas;
	CAIHistoryDebugCanvas*			m_aiDebugCanvas;
	wxHtmlWindow*					m_aiEventDescription;

	// Goto
	THashMap< String, Vector > m_savedPlayerPos;

	// Items
	TDynArray< SAgentStub* >			m_agentsStubs;
	TDynArray< CDeniedAreaComponent* >	m_deniedAreas;
	TDynArray< CCommunityArea* >		m_communityAreas;
	
	TDynArray< CLayerInfo * >			m_layers;
	TDynArray< String >					m_layersHidden; // not included default layers like "hires terrain"

	// Mappins
	TDynArray< Vector >					m_mappinsPositions;
};

///////////////////////////////////////////////////////////////////////////////

