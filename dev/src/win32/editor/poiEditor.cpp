/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifndef NO_MARKER_SYSTEMS

#include <wx/wupdlock.h>
#include "../../common/engine/poiSystem.h"
#include "poiEditor.h"
#include "../../common/core/feedback.h"

BEGIN_EVENT_TABLE( CEdPOIPanel, wxPanel )
	EVT_TOOL( XRCID("m_newPOI"), CEdPOIPanel::OnShowAddNewPOIWindow )
	EVT_TOOL( XRCID("m_editPOI"), CEdPOIPanel::OnShowEditPOIWindow )
	EVT_TOOL( XRCID("m_deletePOI"), CEdPOIPanel::OnDeleteSelectedPOI )
	EVT_TOOL( XRCID("m_filtersForPOI"), CEdPOIPanel::OnShowFilterPanel )
	EVT_TOOL( XRCID("m_refreshPOI"), CEdPOIPanel::OnRefreshPOI )
	EVT_TOOL( XRCID("m_blackBox"), CEdPOIPanel::OnOpenBlackBox )
END_EVENT_TABLE()

CEdPOIPanel::CEdPOIPanel(wxWindow* parent)
{
	// Load layouts from XRC
	wxXmlResource::Get()->LoadPanel( this, parent, wxT("POIPanel") );

	// register listener for messages from review marker tool
	GEngine->GetMarkerSystems()->RegisterListener( MST_POI, this );

	// Load additional windows
	m_addPOIWindow = wxXmlResource::Get()->LoadDialog( this, "AddNewPOIWindow" );	
	m_showModifyPOIWindow = wxXmlResource::Get()->LoadDialog( this, "ShowPOIMarker" );	

	// create list for flags
	m_pointsList = XRCCTRL( *this, "m_poiList", wxListCtrl );
	m_pointsList->AppendColumn( TXT("Name"), wxLIST_FORMAT_LEFT, 200 );
	m_pointsList->AppendColumn( TXT("Type"), wxLIST_FORMAT_LEFT, 100 );
	m_pointsList->Bind( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, &CEdPOIPanel::OnSelectRowInGrid, this );
	m_pointsList->Bind( wxEVT_COMMAND_LIST_COL_RIGHT_CLICK, &CEdPOIPanel::OnShowColumnContextMenu, this );
	m_pointsList->Bind( wxEVT_COMMAND_LIST_COL_CLICK, &CEdPOIPanel::OnSortByColumn, this );
	m_pointsList->Bind( wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK, &CEdPOIPanel::OnShowRowContextMenu, this );
	
	//
	wxImageList* m_pImageList = new wxImageList(16,16);
	m_pointsList->SetImageList( m_pImageList, wxIMAGE_LIST_SMALL );

	// load rest of main panel
	m_autoSyncTime = XRCCTRL( *this, "m_autoSyncTimeForPOI", wxSpinCtrl );
	m_autoSyncTime->SetValue(m_attachedMarkerSystem->GetSyncTime());
	m_autoSyncTime->Connect(wxEVT_COMMAND_SPINCTRL_UPDATED , wxCommandEventHandler( CEdPOIPanel::OnUpdateAutoSyncTime), nullptr, this );
	m_autoSyncEnable = XRCCTRL( *this, "m_autoSyncEnableForPOI", wxCheckBox );
	m_autoSyncEnable->SetValue(m_attachedMarkerSystem->GetAutoSync());
	m_autoSyncEnable->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED , wxCommandEventHandler( CEdPOIPanel::OnEnableAutoSyncFlag), nullptr, this );
	m_filtersPanel = XRCCTRL( *this, "m_filtersPanelForPOI", wxPanel );
	m_mainPanel = XRCCTRL( *this, "m_mainPanelForPOI", wxPanel );
	m_connectionPanel = XRCCTRL( *this, "m_connectionPanelForPOI", wxPanel );
	m_mainToolbar = XRCCTRL( *this, "m_poiSystemToolbar", wxToolBar );

	// m_filtersPanel
	m_filterShowOnMap = XRCCTRL( *this, "m_filterShowPoiOnMap", wxCheckBox );
	m_filterShowOnMap->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdPOIPanel::OnCheckShowOnMap), nullptr, this );
	m_filterSelectAll = XRCCTRL( *this, "m_selectAllFilters", wxCheckBox );
	m_filterSelectAll->Bind( wxEVT_COMMAND_CHECKBOX_CLICKED, &CEdPOIPanel::OnSelectAllFilters, this );
	m_filterCategoriesList = XRCCTRL( *this, "m_poiFilterStatesList", wxCheckListBox );
	m_filterButton = XRCCTRL( *this, "m_poiFilterButton", wxButton );
	m_filterButton->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdPOIPanel::OnClickFilter), nullptr, this );

	// m_addPOIWindow
	m_newPOITitle = XRCCTRL( *this, "m_newPOITitle", wxTextCtrl );
	m_newPOIDescription = XRCCTRL( *this, "m_newPOIDescription", wxTextCtrl );
	m_newPOISnapToTerrain = XRCCTRL( *this, "m_snapToTerrain", wxCheckBox );
	// marks
	wxButton* mark = XRCCTRL( *this, "m_questMarker", wxButton );
	mark->SetLabel("1");	// mark->SetLabel("Quest");	- database id
	mark->Connect(wxEVT_LEFT_DOWN, wxCommandEventHandler( CEdPOIPanel::OnClickSetPOIOnMap), nullptr, this );
	m_pImageList->Add( mark->GetBitmap() );
	mark = nullptr;
	mark = XRCCTRL( *this, "m_sideQuestMark", wxButton );
	mark->SetLabel("2");	// mark->SetLabel("SideQuest");	- database id
	mark->Connect(wxEVT_LEFT_DOWN, wxCommandEventHandler( CEdPOIPanel::OnClickSetPOIOnMap), nullptr, this );
	m_pImageList->Add( mark->GetBitmap() );
	mark = nullptr;
	mark = XRCCTRL( *this, "m_landmarkMarker", wxBitmapButton );
	mark->SetLabel("3");	// mark->SetLabel("Landmark");	- database id
	mark->Connect(wxEVT_LEFT_DOWN, wxCommandEventHandler( CEdPOIPanel::OnClickSetPOIOnMap), nullptr, this );
	m_pImageList->Add( mark->GetBitmap() );
	mark = nullptr;
	mark = XRCCTRL( *this, "m_interiorMarker", wxBitmapButton );
	mark->SetLabel("4");	// mark->SetLabel("Interior");	- database id
	mark->Connect(wxEVT_LEFT_DOWN, wxCommandEventHandler( CEdPOIPanel::OnClickSetPOIOnMap), nullptr, this );
	m_pImageList->Add( mark->GetBitmap() );
	mark = nullptr;
	mark = XRCCTRL( *this, "m_dungeonMarker", wxBitmapButton );
	mark->SetLabel("5");	// mark->SetLabel("Dungeon");	- database id
	mark->Connect(wxEVT_LEFT_DOWN, wxCommandEventHandler( CEdPOIPanel::OnClickSetPOIOnMap), nullptr, this );
	m_pImageList->Add( mark->GetBitmap() );
	mark = nullptr;
	mark = XRCCTRL( *this, "m_gameplayMarker", wxBitmapButton );
	mark->SetLabel("6");	// mark->SetLabel("Gameplay");	- database id
	mark->Connect(wxEVT_LEFT_DOWN, wxCommandEventHandler( CEdPOIPanel::OnClickSetPOIOnMap), nullptr, this );
	m_pImageList->Add( mark->GetBitmap() );
	mark = nullptr;
	mark = XRCCTRL( *this, "m_moodMarker", wxBitmapButton );
	mark->SetLabel("7");	// mark->SetLabel("Mood");	- database id
	mark->Connect(wxEVT_LEFT_DOWN, wxCommandEventHandler( CEdPOIPanel::OnClickSetPOIOnMap), nullptr, this );
	m_pImageList->Add( mark->GetBitmap() );
	mark = nullptr;
	mark = XRCCTRL( *this, "m_communityMarker", wxBitmapButton );
	mark->SetLabel("8");	// mark->SetLabel("Community");	- database id
	mark->Connect(wxEVT_LEFT_DOWN, wxCommandEventHandler( CEdPOIPanel::OnClickSetPOIOnMap), nullptr, this );
	m_pImageList->Add( mark->GetBitmap() );
	mark = nullptr;
	mark = XRCCTRL( *this, "m_roadSignMarker", wxBitmapButton );
	mark->SetLabel("9");	// mark->SetLabel("RoadSign");	- database id
	mark->Connect(wxEVT_LEFT_DOWN, wxCommandEventHandler( CEdPOIPanel::OnClickSetPOIOnMap), nullptr, this );
	m_pImageList->Add( mark->GetBitmap() );
	mark = nullptr;
	mark = XRCCTRL( *this, "m_harborMarker", wxBitmapButton );
	mark->SetLabel("10");	// mark->SetLabel("Harbor");	- database id
	mark->Connect(wxEVT_LEFT_DOWN, wxCommandEventHandler( CEdPOIPanel::OnClickSetPOIOnMap), nullptr, this );
	m_pImageList->Add( mark->GetBitmap() );
	mark = nullptr;
	mark = XRCCTRL( *this, "m_settlementMarker", wxBitmapButton );
	mark->SetLabel("11");	// mark->SetLabel("Settlement");	- database id
	mark->Connect(wxEVT_LEFT_DOWN, wxCommandEventHandler( CEdPOIPanel::OnClickSetPOIOnMap), nullptr, this );
	m_pImageList->Add( mark->GetBitmap() );
	mark = nullptr;
	mark = XRCCTRL( *this, "m_cutsceneMarker", wxBitmapButton );
	mark->SetLabel("12");	// mark->SetLabel("Cutscene");	- database id
	mark->Connect(wxEVT_LEFT_DOWN, wxCommandEventHandler( CEdPOIPanel::OnClickSetPOIOnMap), nullptr, this );
	m_pImageList->Add( mark->GetBitmap() );
	mark = nullptr;
	wxButton* addPOIButton = XRCCTRL( *this, "m_addPOIButton", wxButton );
	addPOIButton->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdPOIPanel::OnClickAddPOI), nullptr, this );
	wxButton* cancelPOIButton = XRCCTRL( *this, "m_cancelAddPOIButton", wxButton );
	cancelPOIButton->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdPOIPanel::OnClickAddPOICancel), nullptr, this );

	// m_showModifyPOIWindow
	m_POITitle = XRCCTRL( *this, "m_modifyPOITitle", wxTextCtrl );
	m_POIDescription = XRCCTRL( *this, "m_modifyPOIDescription", wxTextCtrl );
	m_snapToTerrain = XRCCTRL( *this, "m_modifySnapToTerrain", wxCheckBox );
	m_POICategory = XRCCTRL( *this, "m_modifyPOICategory", wxChoice );
	m_POICategory->Connect(wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdPOIPanel::OnClickSetPOICategory), nullptr, this );
	m_POICategoryIcon = XRCCTRL( *this, "m_modifyPOICategoryIcon", wxStaticBitmap );
	m_editPOIButton = XRCCTRL( *this, "m_modifyPOI", wxButton );
	m_editPOIButton->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdPOIPanel::OnClickModifyPOI), nullptr, this );
	m_cancelPOIButton = XRCCTRL( *this, "m_closePOI", wxButton );
	m_cancelPOIButton->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdPOIPanel::OnClickClosePOIWindow), nullptr, this );

	// create context menu for column
	m_columnContextMenu = new wxMenu();
	m_columnContextMenu->Append( PC_Type, TXT("Type"), TXT(""), (wxItemKind)wxITEM_CHECK )->Check( true );
	m_columnContextMenu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdPOIPanel::OnColumnPopupClick, this );

	// create context menu for one row
	m_rowContextMenu = new wxMenu();
	m_rowContextMenu->Append( 0, TXT("Look at"), TXT("") );
	m_rowContextMenu->AppendSeparator();
	m_rowContextMenu->Append( 1, TXT("Show"), TXT("") );
	m_rowContextMenu->Append( 2, TXT("Edit"), TXT("") );
	m_rowContextMenu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdPOIPanel::OnRowPopupClick, this );

	m_searchLine = XRCCTRL( *this, "m_poiSearchLine", wxTextCtrl );
	m_searchLine->Bind( wxEVT_COMMAND_TEXT_ENTER, &CEdPOIPanel::OnSearchEnterClicked, this );
	m_searchButton = XRCCTRL( *this, "m_poiSearchButton", wxButton );	
	m_searchButton->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdPOIPanel::OnSearchButtonClicked, this );

	SEvents::GetInstance().RegisterListener( CNAME( NodeSelected ), this );
	SEvents::GetInstance().RegisterListener( CNAME( NodeDeselected ), this );
	SEvents::GetInstance().RegisterListener( CNAME( EditorTick ), this );

	// connect with database
	if( m_attachedMarkerSystem != nullptr )
	{
		if( m_attachedMarkerSystem->IsConnected() == false )
		{
			m_attachedMarkerSystem->Connect();
		}
	}
}

CEdPOIPanel::~CEdPOIPanel()
{
	GEngine->GetMarkerSystems()->UnregisterListener( MST_POI, this );

	SEvents::GetInstance().UnregisterListener( CNAME( NodeSelected ), this );
	SEvents::GetInstance().UnregisterListener( CNAME( NodeDeselected ), this );
	SEvents::GetInstance().UnregisterListener( CNAME( EditorTick ), this );
}

void CEdPOIPanel::FillPointsWindow()
{
	wxWindowUpdateLocker localUpdateLocker( this );

	if( m_pointsList->GetItemCount() > 0 )
	{
		m_pointsList->DeleteAllItems();
	}

	const TDynArray<CPointofInterest*>& points = m_attachedMarkerSystem->GetPoints();
	for( Int32 i=points.Size()-1; i>=0; --i )
	{
		Int32 rowIndex = m_pointsList->InsertItem( PC_Name, points[i]->m_name.AsChar() );

		const TDynArray< String >& categories = m_attachedMarkerSystem->GetPointCategories();
		const Uint32 categoryIndex = points[i]->m_category-1;
		if( categoryIndex < categories.Size() )
		{
			m_pointsList->SetItem( rowIndex, PC_Type, categories[categoryIndex].AsChar() );
			m_pointsList->SetItemImage( rowIndex, categoryIndex );
		}
		else
		{
			m_pointsList->SetItem( rowIndex, PC_Type, TXT("Unsupported category") );
			m_pointsList->SetItemImage( rowIndex, -1 );
		}

	}

	LayoutRecursively( this, false );
}

void CEdPOIPanel::FillFilterPanel()
{
	m_filterShowOnMap->SetValue(m_attachedMarkerSystem->GetShowOnMap());

	m_filterCategoriesList->Clear();
	for(Uint32 i=0; i<m_attachedMarkerSystem->GetPointCategories().Size(); ++i)
	{
		m_filterCategoriesList->Insert(m_attachedMarkerSystem->GetPointCategories()[i].AsChar(), 0);
		m_filterCategoriesList->Check(0, true);
	}
	m_filterSelectAll->SetValue( true );
}

void CEdPOIPanel::SelectPOI( const String& name  )
{
	FillPointsWindow();
	const TDynArray<CPointofInterest*>& points = m_attachedMarkerSystem->GetPoints();
	for(Uint32 i=0; i<points.Size(); ++i)
	{
		if( points[i]->m_name == name )
		{
			m_pointsList->SetItemState( i, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED );
			m_pointOpenToEdit = points[i];
			m_pointsList->EnsureVisible( i );
			wxCommandEvent tempE;
			OnShowEditPOIWindow(tempE);
			return;
		}
	}
}

void CEdPOIPanel::OnShowAddNewPOIWindow( wxCommandEvent& event )
{
	if(m_addPOIWindow->IsVisible() == false)
	{
		// clear data
		m_showModifyPOIWindow->SetTitle("Show POI marker");
		m_newPOITitle->SetValue("");
		m_newPOIDescription->SetValue("");
		m_newPOISnapToTerrain->SetValue( false );

		m_pointsList->Enable(false);
		this->Refresh();

		m_addPOIWindow->CenterOnScreen();
		m_addPOIWindow->Show();
		m_attachedMarkerSystem->CreateNewPoint();
	}
}

void CEdPOIPanel::OnShowEditPOIWindow( wxCommandEvent& event )
{
	Int32 selectedItemIndex = GetSelectedItemIndex();

	if( selectedItemIndex != -1 )
	{
		if(m_showModifyPOIWindow->IsVisible() == false)
		{
			// clear data
			m_POITitle->SetValue("");
			m_POIDescription->SetValue("");
			m_POICategory->Clear();
			const TDynArray<String>& category = m_attachedMarkerSystem->GetPointCategories();
			for(Uint32 i=0; i<category.Size(); ++i)
			{
				m_POICategory->Append(category[i].AsChar());
			}
			m_editPOIButton->SetLabel("Modify POI");
			m_cancelPOIButton->SetLabel("Close");
			m_POITitle->Enable(false);
			m_POIDescription->Enable(false);
			m_snapToTerrain->Enable(false);
			m_POICategory->Enable(false);

			m_pointsList->Enable(false);
			this->Refresh();

			m_pointOpenToEdit = m_attachedMarkerSystem->GetPoint( selectedItemIndex );

			// set information in window
			m_POITitle->SetLabel(m_pointOpenToEdit->m_name.AsChar());
			m_POIDescription->SetLabel(m_pointOpenToEdit->m_description.AsChar());
			m_snapToTerrain->SetValue(m_pointOpenToEdit->m_snappedToTerrain);
			m_POICategory->SetSelection(m_pointOpenToEdit->m_category-1);

			SetIconForCategory(m_pointOpenToEdit->m_category);

			m_showModifyPOIWindow->CenterOnScreen();
			m_showModifyPOIWindow->Show();
		}
	}
	else
	{
		wxMessageBox("At first, you must choose point from the list", "Information", wxOK | wxICON_INFORMATION);
	}
}

void CEdPOIPanel::OnDeleteSelectedPOI( wxCommandEvent& event )
{
	if(wxMessageBox("Operation is not undoable! Do you want to delete this point?", "Deleting...", wxYES_NO | wxICON_WARNING) == wxYES)
	{
		Int32 selectdeItemIndex = GetSelectedItemIndex();
		if( selectdeItemIndex != -1 )
		{
			CPointofInterest& point = *m_attachedMarkerSystem->GetPoint( selectdeItemIndex );
			if(m_attachedMarkerSystem->DeletePoint(point) == true)
			{
				wxMessageBox("Point was deleted.", "Success", wxOK | wxICON_INFORMATION);
			}
			else
			{
				wxMessageBox("The point was not deleted.", "Failed", wxOK | wxICON_ERROR);
			}
		}
		else
		{
			wxMessageBox("At first, you must choose point from the list", "Information", wxOK | wxICON_INFORMATION);
		}
	}
}

void CEdPOIPanel::OnUpdateAutoSyncTime( wxCommandEvent& event )
{
	m_attachedMarkerSystem->SetSyncTime(event.GetInt());
}

void CEdPOIPanel::OnEnableAutoSyncFlag( wxCommandEvent& event )
{
	m_attachedMarkerSystem->SetAutoSync(event.IsChecked());
}

void CEdPOIPanel::OnShowFilterPanel( wxCommandEvent& event )
{
	if(m_filtersPanel->IsShown() == true)
	{
		m_filtersPanel->Hide();
	}
	else
	{
		FillFilterPanel();
		m_filtersPanel->Show();
	}
	LayoutRecursively( this, false );
}

void CEdPOIPanel::OnRefreshPOI( wxCommandEvent& event )
{
	m_attachedMarkerSystem->SendRequest( MSRT_SynchronizeData );
}

void CEdPOIPanel::OnOpenBlackBox( wxCommandEvent& event )
{
	if(m_attachedMarkerSystem->GetBlackBoxPath().Empty() == true)
	{
		// Ask for world file
		CEdFileDialog fileDialog;
		fileDialog.SetIniTag( TXT("BlackBox") );
		fileDialog.AddFormat( TXT("exe"), TXT( "Executable file" ) );

		if ( fileDialog.DoOpen( (HWND)GetHandle(), true ) )
		{
			m_attachedMarkerSystem->SetBlackBoxPath(fileDialog.GetFile());
		}
	}

	if(m_attachedMarkerSystem->GetBlackBoxPath().Empty() == false)
	{
		OpenExternalFile( m_attachedMarkerSystem->GetBlackBoxPath().AsChar() );
	}
}

void CEdPOIPanel::OnCheckShowOnMap( wxCommandEvent& event )
{
	m_attachedMarkerSystem->SetShowOnMap(event.GetInt() != 0);
}

void CEdPOIPanel::OnClickFilter( wxCommandEvent& event )
{
	TDynArray<Bool> categories;
	for(Int32 i=m_filterCategoriesList->GetCount()-1; i>-1; --i)
	{
		categories.PushBack( m_filterCategoriesList->IsChecked( i ) );
	}	
	m_attachedMarkerSystem->SetFilters( categories );
}

void CEdPOIPanel::OnSelectAllFilters( wxCommandEvent& event )
{
	Bool selectAll = ( event.GetInt() == 0 ) ? false : true;
	const Uint32 categoryCount = m_filterCategoriesList->GetCount();
	for( Uint32 i=0; i<categoryCount; ++i )
	{
		m_filterCategoriesList->Check( i, selectAll );
	}
}

void CEdPOIPanel::OnClickSetPOIOnMap( wxCommandEvent& event )
{
	wxButton* parent = wxDynamicCast(event.GetEventObject(), wxButton);
	ASSERT(parent != nullptr);
	String label = parent->GetLabel();
	FromString(label, m_newPointCategoryId);

	m_attachedMarkerSystem->WaitForEntity();

	// TODO - add to select different entity for different points
	if(m_attachedMarkerSystem->GetNewPoint()->m_poiEntity == nullptr)
	{
		THandle< CEntityTemplate > entityTemplate = m_attachedMarkerSystem->GetPointTemplate( static_cast< EPOICategory >( m_newPointCategoryId - 1 ) );
		String drop = TXT("Resources:") + entityTemplate->GetDepotPath();
		wxTextDataObject myData( drop.AsChar() );

		wxDropSource dragSource(this);
		dragSource.SetData(myData);
		wxDragResult result = dragSource.DoDragDrop( wxDrag_DefaultMove );
	}
	else
	{
		wxMessageBox("POI mark is on the map!", "Warning", wxOK | wxICON_WARNING);
	}
}

void CEdPOIPanel::OnClickAddPOI( wxCommandEvent& event )
{
	if(m_newPOITitle->GetValue().empty() == true)
	{
		wxMessageBox("Point must have a title!", "Remember!!!", wxOK | wxICON_INFORMATION);
		return;
	}
	if(m_attachedMarkerSystem->GetActiveLevelId() == -1)
	{
		wxMessageBox("This map does not support POI!", "Error!!!", wxOK | wxICON_ERROR);
		return;
	}
	if(m_attachedMarkerSystem->GetNewPoint()->m_poiEntity == nullptr)
	{
		wxMessageBox("You must set point on map!", "Remember!!!", wxOK | wxICON_INFORMATION);
		return;
	}

	CPointofInterest& newPoint = *m_attachedMarkerSystem->GetNewPoint();
	newPoint.m_name = m_newPOITitle->GetValue();
	newPoint.m_description = m_newPOIDescription->GetValue();
	newPoint.m_snappedToTerrain = m_newPOISnapToTerrain->GetValue();
	newPoint.m_levelId = m_attachedMarkerSystem->GetActiveLevelId();
	newPoint.m_category = m_newPointCategoryId;
	
	if ( newPoint.m_poiEntity != nullptr )
	{
		// Add basic tags
		TagList tags = newPoint.m_poiEntity->GetTags();
		tags.AddTag(CNAME( LockedObject ));
		tags.AddTag(CNAME( PointObject ));
		newPoint.m_poiEntity->SetTags( tags );
	}

	if(m_attachedMarkerSystem->AddNewPoint() == true)
	{
		m_addPOIWindow->Hide();
		wxMessageBox("Add flag succeed", "Information", wxOK | wxICON_INFORMATION);
	}

	ClearSelection();
	m_pointsList->Enable(true);
	m_addPOIWindow->Hide();
}

void CEdPOIPanel::OnClickAddPOICancel( wxCommandEvent& event )
{
	if(wxMessageBox("Do you want close window?", "Closing...", wxYES_NO | wxICON_QUESTION) == wxYES)
	{
		if(m_attachedMarkerSystem->GetNewPoint()->m_poiEntity != nullptr)
		{
			m_attachedMarkerSystem->GetNewPoint()->m_poiEntity->Destroy();
		}

		ClearSelection();
		m_pointsList->Enable(true);
		m_addPOIWindow->Hide();
	}
}

void CEdPOIPanel::OnClickSetPOICategory( wxCommandEvent& event )
{
	SetIconForCategory(event.GetInt()+1);
}

void CEdPOIPanel::OnClickModifyPOI( wxCommandEvent& event )
{
	if(m_editPOIButton->GetLabel() == "Modify POI")
	{
		m_showModifyPOIWindow->SetTitle("Modify POI marker");
		m_editPOIButton->SetLabel("Accept modification");
		m_cancelPOIButton->SetLabel("Cancel");
		m_POITitle->Enable();
		m_POIDescription->Enable();
		m_snapToTerrain->Enable();
		m_POICategory->Enable();

		// unlock entity (can move it)
		if(m_pointOpenToEdit->m_poiEntity != nullptr)
		{
			TagList tags = m_pointOpenToEdit->m_poiEntity->GetTags();
			if( tags.HasTag(CNAME( LockedObject )) == true )
			{
				tags.SubtractTag(CNAME( LockedObject ));
				m_pointOpenToEdit->m_poiEntity->SetTags(tags);
			}
		}
	}
	else if(m_editPOIButton->GetLabel() == "Accept modification")
	{
		m_pointOpenToEdit->m_name = m_POITitle->GetValue();
		m_pointOpenToEdit->m_description = m_POIDescription->GetValue();
		m_pointOpenToEdit->m_snappedToTerrain = m_snapToTerrain->GetValue();
		m_pointOpenToEdit->m_category = m_POICategory->GetSelection()+1;
		if(m_attachedMarkerSystem->ModifyPoint(*m_pointOpenToEdit) == true)
		{
			// lock entity (can't move it)
			if(m_pointOpenToEdit->m_poiEntity != nullptr)
			{
				TagList tags = m_pointOpenToEdit->m_poiEntity->GetTags();
				tags.AddTag(CNAME( LockedObject ));
				m_pointOpenToEdit->m_poiEntity->SetTags(tags);
			}

			ClearSelection();
			m_pointsList->Enable(true);

			wxMessageBox("Point was modified.", "Success", wxOK | wxICON_INFORMATION);

			m_showModifyPOIWindow->Hide();
		}
	}
}

void CEdPOIPanel::OnClickClosePOIWindow( wxCommandEvent& event )
{
	if(m_cancelPOIButton->GetLabel() == "Close")
	{
		m_showModifyPOIWindow->Hide();
	}
	else if(m_cancelPOIButton->GetLabel() == "Cancel")
	{
		if(wxMessageBox("Do you want close window?", "Closing...", wxYES_NO | wxICON_QUESTION) == wxYES)
		{
			m_showModifyPOIWindow->Hide();
		}
	}

	// lock entity (can't move it)
	if(m_pointOpenToEdit->m_poiEntity != nullptr)
	{
		TagList tags = m_pointOpenToEdit->m_poiEntity->GetTags();
		tags.AddTag(CNAME( LockedObject ));
		m_pointOpenToEdit->m_poiEntity->SetTags(tags);
	}

	ClearSelection();
	m_pointsList->Enable(true);
}

void CEdPOIPanel::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if( name == CNAME( NodeSelected ) )
	{
		CEntity* entity = Cast< CEntity >( GetEventData< CNode* >( data ) );
		if( entity != nullptr )
		{
			if( entity->GetTags().HasTag( CNAME(PointObject) ) == true )
			{
				if(m_pointOpenToEdit == nullptr && m_showModifyPOIWindow->IsVisible() == false)
				{
					const TDynArray<CPointofInterest*>& points = m_attachedMarkerSystem->GetPoints();
					for(Uint32 i=0; i<points.Size(); ++i)
					{
						if( points[i]->m_poiEntity == entity )
						{
							m_pointsList->SetItemState( i, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED );
							m_pointOpenToEdit = points[i];
							m_pointsList->EnsureVisible( i );
						}
					}
				}
			}
		}
	}
	else if( name == CNAME( NodeDeselected ) )
	{
		CEntity* entity = Cast< CEntity >( GetEventData< CNode* >( data ) );
		if( entity != nullptr )
		{
			if( entity->GetTags().HasTag( CNAME(PointObject) ) == true )
			{
				if( m_showModifyPOIWindow->IsVisible() == false )
				{
					m_pointsList->SetItemState( GetSelectedItemIndex(), 0, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED );
					m_pointOpenToEdit = nullptr;
				}
			}
		}
	}
	else if( name == CNAME( EditorTick ) )
	{
		InternalProcessMessage();
	}
}

void CEdPOIPanel::SetIconForCategory(Uint32 categoryId) 
{
	// set correct icon for point
	wxBitmap* icon = nullptr;
	wxBitmapButton* mark = nullptr;
	switch(categoryId-1)
	{
	case POIC_Quest:
		mark = XRCCTRL( *this, "m_questMarker", wxBitmapButton );
		icon = new wxBitmap(mark->GetBitmap());
		break;
	case POIC_SideQuest:
		mark = XRCCTRL( *this, "m_sideQuestMark", wxBitmapButton );
		icon = new wxBitmap(mark->GetBitmap());
		break;
	case POIC_Landmark:
		mark = XRCCTRL( *this, "m_landmarkMarker", wxBitmapButton );
		icon = new wxBitmap(mark->GetBitmap());
		break;
	case POIC_Mood:
		mark = XRCCTRL( *this, "m_moodMarker", wxBitmapButton );
		icon = new wxBitmap(mark->GetBitmap());
		break;
	case POIC_Interrior:
		mark = XRCCTRL( *this, "m_interiorMarker", wxBitmapButton );
		icon = new wxBitmap(mark->GetBitmap());
		break;
	case POIC_Dungeon:
		mark = XRCCTRL( *this, "m_dungeonMarker", wxBitmapButton );
		icon = new wxBitmap(mark->GetBitmap());
		break;
	case POIC_Gameplay:
		mark = XRCCTRL( *this, "m_gameplayMarker", wxBitmapButton );
		icon = new wxBitmap(mark->GetBitmap());
		break;
	case POIC_Community:
		mark = XRCCTRL( *this, "m_communityMarker", wxBitmapButton );
		icon = new wxBitmap(mark->GetBitmap());
		break;
	case POIC_RoadSign:
		mark = XRCCTRL( *this, "m_roadSignMarker", wxBitmapButton );
		icon = new wxBitmap(mark->GetBitmap());
		break;
	case POIC_Harbor:
		mark = XRCCTRL( *this, "m_harborMarker", wxBitmapButton );
		icon = new wxBitmap(mark->GetBitmap());
		break;
	case POIC_Settlement:
		mark = XRCCTRL( *this, "m_settlementMarker", wxBitmapButton );
		icon = new wxBitmap(mark->GetBitmap());
		break;
	case POIC_Cutscene:
		mark = XRCCTRL( *this, "m_cutsceneMarker", wxBitmapButton );
		icon = new wxBitmap(mark->GetBitmap());
		break;
	}
	if(icon != nullptr && icon->IsOk() == true)
	{
		m_POICategoryIcon->SetBitmap(*icon);
		m_POICategoryIcon->Show();
	}
}

void CEdPOIPanel::ProcessMessage( enum EMarkerSystemMessage message, enum EMarkerSystemType systemType, IMarkerSystemInterface* system )
{
	switch( message )
	{
	case MSM_SystemRegistered:
		{
			if( systemType == MST_POI )
			{
				m_attachedMarkerSystem = static_cast< CPoiSystem* >( system );
				this->Enable();
			}
		}
		return;
	case MSM_SystemUnregistered:
		{
			if( systemType == MST_POI )
			{
				this->Disable();
				m_attachedMarkerSystem = nullptr;
			}
		}
		return;
	}

	// add message to queue
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_synchronizationMutex );
		m_messages.Push( message );
	}
}

void CEdPOIPanel::InternalProcessMessage()
{
	EMarkerSystemMessage message = MSM_Count;
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_synchronizationMutex );
		if( m_messages.Empty() == true )
		{
			return;
		}
		message = m_messages.Front();
		m_messages.Pop();
	}

	switch( message )
	{
	case MSM_DatabaseConnectionStart:
		{
			GFeedback->BeginTask( TXT("Connecting with database"), false );
			this->Disable();
		}
		break;
	case MSM_DatabaseConnected:
		{
			FillPointsWindow();

			this->Enable();
			m_mainPanel->Enable();
			m_mainToolbar->Enable();
			m_connectionPanel->Hide();
			Layout();

			GFeedback->EndTask();
		}
		break;
	case MSM_DatabaseLostConnection:
		{
			this->Enable();
			m_mainPanel->Disable();
			m_mainToolbar->Disable();
			m_connectionPanel->Show();
			Layout();

			GFeedback->EndTask();
		}
		break;
	case MSM_SynchronizationStart:
		{
			GFeedback->BeginTask( TXT("Synchronization with database"), false );
			this->Disable();
		}
		break;
	case MSM_SynchronizationEnd:
		{
			FillPointsWindow();
			this->Enable();
			GFeedback->EndTask();
		}
		break;
	case MSM_DataAreUpdated:
	case MSM_DataAreSorted:
		{
			this->Disable();
			FillPointsWindow();
			this->Enable();
		}
		break;
	}
}

void CEdPOIPanel::OnShowColumnContextMenu( wxListEvent& event )
{
	wxWindowBase::PopupMenu( m_columnContextMenu );
}

void CEdPOIPanel::OnSortByColumn( wxListEvent& event )
{
	wxListCtrl* list = wxDynamicCast( event.GetEventObject(), wxListCtrl );
	Uint32 columnIndex = event.GetColumn();
	static Bool ascendingOrder = true;
	ascendingOrder = !ascendingOrder;

	m_attachedMarkerSystem->SetSortingSettings( static_cast< EPOISortCategory >( columnIndex ), ( ascendingOrder == true ) ? MSSO_Ascending : MSSO_Descending );
}

void CEdPOIPanel::OnColumnPopupClick( wxCommandEvent& event )
{
	wxWindowUpdateLocker localUpdateLocker( this );

	if( m_columnContextMenu->IsChecked( event.GetId() ) == true )
	{
		m_pointsList->SetColumnWidth( event.GetId(), 50 );
	}
	else
	{
		m_pointsList->SetColumnWidth( event.GetId(), 0 );
	}
}

void CEdPOIPanel::OnRowPopupClick( wxCommandEvent& event )
{
	switch( event.GetId() )
	{
	case 0:		// look at
		OnSelectRowInGrid( wxListEvent() );
		break;
	case 1:		// show
		OnShowEditPOIWindow( wxCommandEvent() );
		break;
	case 2:		// edit
		OnShowEditPOIWindow( wxCommandEvent() );
		OnClickModifyPOI( wxCommandEvent() );
		break;
	}
}

void CEdPOIPanel::ClearSelection()
{
	CWorld* world = GGame->GetActiveWorld();
	if ( world )
	{
		world->GetSelectionManager()->DeselectAll();
	}

	m_pointsList->SetItemState( GetSelectedItemIndex(), 0, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED );
}

Int32 CEdPOIPanel::GetSelectedItemIndex()
{
	return m_pointsList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
}

void CEdPOIPanel::OnShowRowContextMenu( wxListEvent& event )
{
	wxWindowBase::PopupMenu( m_rowContextMenu );
}

void CEdPOIPanel::OnSearchButtonClicked( wxCommandEvent& event )
{
	String phrase = m_searchLine->GetValue();
	m_attachedMarkerSystem->SetSearchFilter( phrase );
}

void CEdPOIPanel::OnSearchEnterClicked( wxCommandEvent& event )
{
	OnSearchButtonClicked( wxCommandEvent() );
}

void CEdPOIPanel::OnSelectRowInGrid( wxListEvent& event )
{
	Int32 selectedItemIndex = GetSelectedItemIndex();
	if( selectedItemIndex != -1 )
	{
		CPointofInterest* selectedPoint = m_attachedMarkerSystem->GetPoint( selectedItemIndex );
		CWorld* world = GGame->GetActiveWorld();
		if ( world )
		{
			CSelectionManager* selectionManager = world->GetSelectionManager();
			if( selectionManager != nullptr )
			{
				selectionManager->DeselectAll();
				if( selectedPoint->m_poiEntity != nullptr )
				{
					selectionManager->Select( selectedPoint->m_poiEntity.Get() );
				}
			}
		}

		wxTheFrame->GetWorldEditPanel()->SetCameraPosition(selectedPoint->m_worldPosition + Vector(0,0,10));
		EulerAngles eulerAngles(0.0f, 270.0f, -90.0f);
		wxTheFrame->GetWorldEditPanel()->SetCameraRotation(eulerAngles);
	}
}

#endif	// NO_MARKER_SYSTEM
