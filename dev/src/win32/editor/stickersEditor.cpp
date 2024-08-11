/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifndef NO_MARKER_SYSTEMS

#include "../../common/engine/stickersSystem.h"
#include "stickersEditor.h"
#include "sceneExplorer.h"
#include "../../common/core/feedback.h"

BEGIN_EVENT_TABLE( CEdStickersPanel, wxPanel )
	EVT_TOOL( XRCID("m_newSticker"), CEdStickersPanel::OnShowAddNewStickerWindow )
	EVT_TOOL( XRCID("m_editSticker"), CEdStickersPanel::OnShowEditStickerWindow )
	EVT_TOOL( XRCID("m_deleteSticker"), CEdStickersPanel::OnDeleteSelectedSticker )
	EVT_TOOL( XRCID("m_filtersForSticker"), CEdStickersPanel::OnShowFilterPanel )
	EVT_TOOL( XRCID("m_refreshSticker"), CEdStickersPanel::OnRefreshStickers )
	END_EVENT_TABLE()

CEdStickersPanel::CEdStickersPanel(wxWindow* parent)
{
	// Load layouts from XRC
	wxXmlResource::Get()->LoadPanel( this, parent, wxT("StickersPanel") );

	// register listener for messages from review marker tool
	GEngine->GetMarkerSystems()->RegisterListener( MST_Sticker, this );

	// Load additional windows
	m_addStickerWindow = wxXmlResource::Get()->LoadDialog( this, "AddNewSticker" );	
	m_showModifyStickerWindow = wxXmlResource::Get()->LoadDialog( this, "ShowSticker" );	

	// main panel
	m_stickersList = XRCCTRL( *this, "m_stickersList", wxListBox );
	m_stickersList->Connect(wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( CEdStickersPanel::OnSingleClickSticker), nullptr, this );
	m_stickersList->Connect(wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( CEdStickersPanel::OnDoubleClickSticker), nullptr, this );
	m_autoSyncTime = XRCCTRL( *this, "m_autoSyncTimeForSticker", wxSpinCtrl );
	m_autoSyncTime->SetValue(m_attachedMarkerSystem->GetSyncTime());
	m_autoSyncTime->Connect(wxEVT_COMMAND_SPINCTRL_UPDATED , wxCommandEventHandler( CEdStickersPanel::OnUpdateAutoSyncTime), nullptr, this );
	m_autoSyncEnable = XRCCTRL( *this, "m_autoSyncEnableForSticker", wxCheckBox );
	m_autoSyncEnable->SetValue(m_attachedMarkerSystem->GetAutoSync());
	m_autoSyncEnable->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED , wxCommandEventHandler( CEdStickersPanel::OnEnableAutoSyncStickers), nullptr, this );
	m_filtersPanel = XRCCTRL( *this, "m_filtersForSticker", wxPanel );
	m_mainPanel = XRCCTRL( *this, "m_mainPanelForStickers", wxPanel );
	m_connectionPanel = XRCCTRL( *this, "m_connectionPanelForStickers", wxPanel );
	m_mainToolbar = XRCCTRL( *this, "m_stickersSystemToolbar", wxToolBar );

	// searching
	m_searchLine = XRCCTRL( *this, "m_stickerSearchLine", wxTextCtrl );
	m_searchLine->Bind( wxEVT_COMMAND_TEXT_ENTER, &CEdStickersPanel::OnSearchEnterClicked, this );
	m_searchButton = XRCCTRL( *this, "m_stickerSearchButton", wxButton );
	m_searchButton->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdStickersPanel::OnSearchButtonClicked, this );	

	// m_filtersPanel
	m_filterShowOnMap = XRCCTRL( *this, "m_filterShowStickerOnMap", wxCheckBox );
	m_filterShowOnMap->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdStickersPanel::OnCheckShowOnMap), nullptr, this );
	m_filterCategoriesList = XRCCTRL( *this, "m_stickerFilterStatesList", wxCheckListBox );
	m_filterCategoriesList->Bind( wxEVT_COMMAND_LISTBOX_SELECTED, &CEdStickersPanel::OnSelectFilter, this );
	m_filterButton = XRCCTRL( *this, "m_stickerFilterButton", wxButton );
	m_filterButton->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdStickersPanel::OnClickFilter), nullptr, this );
	m_filterSelectAll = XRCCTRL( *this, "m_selectAllFilters", wxCheckBox );
	m_filterSelectAll->Bind( wxEVT_COMMAND_CHECKBOX_CLICKED, &CEdStickersPanel::OnSelectAllFilters, this );

	// m_addStickerWindow
	m_newStickerTitle = XRCCTRL( *this, "m_stickerTitle", wxTextCtrl );
	m_newStickerDescription = XRCCTRL( *this, "m_stickerDescription", wxTextCtrl );
	m_newStickerCategory = XRCCTRL( *this, "m_stickerCategory", wxChoice );
	m_newStickerEntity = XRCCTRL( *this, "m_setStickerPosition", wxBitmapButton );
	m_newStickerEntity->Connect(wxEVT_LEFT_DOWN, wxCommandEventHandler( CEdStickersPanel::OnClickSetStickerOnMap), nullptr, this );
	m_newStickerAdd = XRCCTRL( *this, "m_addNewSticker", wxButton );
	m_newStickerAdd->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdStickersPanel::OnClickAddSticker), nullptr, this );
	m_newStickerCancel = XRCCTRL( *this, "m_cancelNewStickerWindow", wxButton );
	m_newStickerCancel->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdStickersPanel::OnClickAddStickerCancel), nullptr, this );

	// m_showModifyStickerWindow
	m_stickerTitle = XRCCTRL( *this, "m_modifyStickerTitle", wxTextCtrl );
	m_stickerDescription = XRCCTRL( *this, "m_modifyStickerDescription", wxTextCtrl );
	m_stickerCategory = XRCCTRL( *this, "m_modifyStickerCategory", wxChoice );
	m_editStickerButton = XRCCTRL( *this, "m_modifySticker", wxButton );
	m_editStickerButton->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdStickersPanel::OnClickModifySticker), nullptr, this );
	m_cancelStickerButton = XRCCTRL( *this, "m_closeStickerWindow", wxButton );
	m_cancelStickerButton->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdStickersPanel::OnClickCloseStickerWindow), nullptr, this );

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

CEdStickersPanel::~CEdStickersPanel()
{
	GEngine->GetMarkerSystems()->UnregisterListener( MST_Sticker, this );

	SEvents::GetInstance().UnregisterListener( CNAME( NodeSelected ), this );
	SEvents::GetInstance().UnregisterListener( CNAME( NodeDeselected ), this );
	SEvents::GetInstance().UnregisterListener( CNAME( EditorTick ), this );
}

void CEdStickersPanel::FillStickersWindow()
{
	m_stickersList->Clear();

	const TDynArray<CSticker*>& stickers = m_attachedMarkerSystem->GetStickers();
	if(stickers.Size() > 0)
	{
		wxArrayString tab;

		for(Uint32 i=0; i<stickers.Size(); ++i)
		{
			tab.Add(stickers[i]->m_title.AsChar());
		}

		if(tab.size() > 0)
		{
			m_stickersList->InsertItems(tab, 0);
		}
	}
}

void CEdStickersPanel::FillFilterPanel()
{
	m_filterShowOnMap->SetValue(m_attachedMarkerSystem->GetShowOnMap());

	m_filterCategoriesList->Clear();
	for(Uint32 i=0; i<m_attachedMarkerSystem->GetStickerCategories().Size(); ++i)
	{
		m_filterCategoriesList->Insert(m_attachedMarkerSystem->GetStickerCategories()[i].AsChar(), 0);
		m_filterCategoriesList->Check(0, true);
	}
	m_filterSelectAll->SetValue( true );
}

void CEdStickersPanel::OnShowAddNewStickerWindow( wxCommandEvent& event )
{
	if(m_addStickerWindow->IsVisible() == false)
	{
		// clear data
		m_showModifyStickerWindow->SetTitle("Show Sticker marker");
		m_newStickerTitle->SetValue("");
		m_newStickerDescription->SetValue("");

		m_newStickerCategory->Clear();
		const TDynArray<String>& category = m_attachedMarkerSystem->GetStickerCategories();
		for(Uint32 i=0; i<category.Size(); ++i)
		{
			m_newStickerCategory->Append(category[i].AsChar());
		}
		m_newStickerCategory->Select(category.Size()-1);

		m_stickersList->Enable(false);
		this->Refresh();

		m_addStickerWindow->CenterOnScreen();
		m_addStickerWindow->Show();
		m_attachedMarkerSystem->CreateNewSticker();
	}
}

void CEdStickersPanel::OnShowEditStickerWindow( wxCommandEvent& event )
{
	wxArrayInt selections;
	m_stickersList->GetSelections(selections);

	if(selections.size() > 0)
	{
		if(m_showModifyStickerWindow->IsVisible() == false)
		{
			// clear data
			m_stickerTitle->SetValue("");
			m_stickerDescription->SetValue("");
			m_stickerCategory->Clear();
			const TDynArray<String>& category = m_attachedMarkerSystem->GetStickerCategories();
			for(Uint32 i=0; i<category.Size(); ++i)
			{
				m_stickerCategory->Append(category[i].AsChar());
			}
			m_editStickerButton->SetLabel("Modify Sticker");
			m_cancelStickerButton->SetLabel("Close");
			m_stickerTitle->Enable(false);
			m_stickerDescription->Enable(false);
			m_stickerCategory->Enable(false);

			m_stickersList->Enable(false);
			this->Refresh();

			m_stickerOpenToEdit = m_attachedMarkerSystem->GetSticker(selections[0]);

			// set information in window
			m_stickerTitle->SetLabel(m_stickerOpenToEdit->m_title.AsChar());
			m_stickerDescription->SetLabel(m_stickerOpenToEdit->m_description.AsChar());
			m_stickerCategory->SetSelection(m_stickerOpenToEdit->m_type-1);

			m_showModifyStickerWindow->CenterOnScreen();
			m_showModifyStickerWindow->Show();
		}
	}
	else
	{
		wxMessageBox("At first, you must choose Sticker from the list", "Information", wxOK | wxICON_INFORMATION);
	}
}

void CEdStickersPanel::OnDeleteSelectedSticker( wxCommandEvent& event )
{
	if(wxMessageBox("Operation is not undoable! Do you want to delete this Sticker?", "Deleting...", wxYES_NO | wxICON_WARNING) == wxYES)
	{
		wxArrayInt selections;
		m_stickersList->GetSelections(selections);

		if(selections.size() > 0)
		{
			CSticker& sticker = *m_attachedMarkerSystem->GetSticker(selections[0]);
			if(m_attachedMarkerSystem->DeleteSticker(sticker) == true)
			{
				wxMessageBox("Sticker was deleted.", "Success", wxOK | wxICON_INFORMATION);
			}
		}
		else
		{
			wxMessageBox("At first, you must choose sticker from the list", "Information", wxOK | wxICON_INFORMATION);
		}
	}
}

void CEdStickersPanel::OnSingleClickSticker( wxCommandEvent& event )
{
	wxArrayInt selections;
	m_stickersList->GetSelections(selections);

	if(selections.size() > 0)
	{
		CSticker* selectedSticker = m_attachedMarkerSystem->GetSticker(selections[0]);
		CWorld* world = GGame->GetActiveWorld();
		if ( world )
		{
			CSelectionManager* selectionManager = world->GetSelectionManager();
			if( selectionManager != nullptr )
			{
				selectionManager->DeselectAll();
				if( selectedSticker->m_stickerEntity != nullptr )
				{
					selectionManager->Select( selectedSticker->m_stickerEntity. Get() );
				}
			}
		}
	}
}

void CEdStickersPanel::OnDoubleClickSticker( wxCommandEvent& event )
{
	wxArrayInt selections;
	m_stickersList->GetSelections(selections);

	if(selections.size() > 0)
	{
		CSticker* selectedSticker = m_attachedMarkerSystem->GetSticker(selections[0]);
		CWorld* world = GGame->GetActiveWorld();
		if ( world )
		{
			CSelectionManager* selectionManager = world->GetSelectionManager();
			if( selectionManager != nullptr )
			{
				selectionManager->DeselectAll();
				if( selectedSticker->m_stickerEntity != nullptr )
				{
					selectionManager->Select( selectedSticker->m_stickerEntity.Get() );
				}
			}
		}

		wxTheFrame->GetWorldEditPanel()->SetCameraPosition(selectedSticker->m_position + Vector(0,0,10));
		EulerAngles eulerAngles(0.0f, 270.0f, -90.0f);
		wxTheFrame->GetWorldEditPanel()->SetCameraRotation(eulerAngles);
	}
}

void CEdStickersPanel::OnUpdateAutoSyncTime( wxCommandEvent& event )
{
	m_attachedMarkerSystem->SetSyncTime(event.GetInt());
}

void CEdStickersPanel::OnEnableAutoSyncStickers( wxCommandEvent& event )
{
	m_attachedMarkerSystem->SetAutoSync(event.IsChecked());
}

void CEdStickersPanel::OnShowFilterPanel( wxCommandEvent& event )
{
	if(m_filtersPanel->IsShown() == true)
	{
		m_filtersPanel->Hide();
		Layout();
	}
	else
	{
		FillFilterPanel();
		m_filtersPanel->Show();
		Layout();
	}
}

void CEdStickersPanel::OnRefreshStickers( wxCommandEvent& event )
{
	m_attachedMarkerSystem->SendRequest( MSRT_SynchronizeData );
}

void CEdStickersPanel::OnCheckShowOnMap( wxCommandEvent& event )
{
	m_attachedMarkerSystem->SetShowOnMap(event.GetInt() != 0);
}

void CEdStickersPanel::OnClickFilter( wxCommandEvent& event )
{
	TDynArray<Bool> categories;
	for(Int32 i=m_filterCategoriesList->GetCount()-1; i>-1; --i)
	{
		categories.PushBack(m_filterCategoriesList->IsChecked(i));
	}
	m_attachedMarkerSystem->SetFilters( categories );
}

void CEdStickersPanel::OnSelectFilter( wxCommandEvent& event )
{
	Bool filterState = ( event.GetInt() == 0 ) ? false : true;
	if( filterState == false )
	{
		m_filterSelectAll->SetValue( false );
	}
}

void CEdStickersPanel::OnSelectAllFilters( wxCommandEvent& event )
{
	Bool selectAll = ( event.GetInt() == 0 ) ? false : true;
	for (Uint32 i=0; i<m_filterCategoriesList->GetCount(); ++i)
	{
		m_filterCategoriesList->Check( i, selectAll );
	}
}

void CEdStickersPanel::OnClickSetStickerOnMap( wxCommandEvent& event )
{
	m_attachedMarkerSystem->WaitForEntity();

	if( m_attachedMarkerSystem->GetNewSticker()->m_stickerEntity == nullptr )
	{
		THandle< CEntityTemplate > entityTemplate = m_attachedMarkerSystem->GetStickerTemplate();
		String drop = TXT("Resources:") + entityTemplate->GetDepotPath();

		wxString wxDrop = drop.AsChar();
		wxTextDataObject myData( wxDrop );

		wxDropSource dragSource(this);
		dragSource.SetData(myData);
		wxDragResult result = dragSource.DoDragDrop( wxDrag_DefaultMove );
	}
	else
	{
		wxMessageBox("Sticker mark is on the map!", "Warning", wxOK | wxICON_WARNING);
	}
}

void CEdStickersPanel::OnClickAddSticker( wxCommandEvent& event )
{
	if(m_newStickerTitle->GetValue().empty() == true)
	{
		wxMessageBox("Sticker must have a title!", "Remember!!!", wxOK | wxICON_INFORMATION);
		return;
	}
	if( m_attachedMarkerSystem->GetNewSticker()->m_stickerEntity == nullptr )
	{
		wxMessageBox("You must set sticker on map!", "Remember!!!", wxOK | wxICON_INFORMATION);
		return;
	}

	CSticker& newSticker = *m_attachedMarkerSystem->GetNewSticker();
	newSticker.m_title = m_newStickerTitle->GetValue();
	newSticker.m_description = m_newStickerDescription->GetValue();
	newSticker.m_type = m_newStickerCategory->GetSelection()+1;

	if ( newSticker.m_stickerEntity != nullptr )
	{
		// Add basic tags
		TagList tags = newSticker.m_stickerEntity.Get()->GetTags();
		tags.AddTag(CNAME( LockedObject ));
		tags.AddTag(CNAME( StickerObject ));
		newSticker.m_stickerEntity.Get()->SetTags( tags );
	}

	if(m_attachedMarkerSystem->AddNewSticker() == true)
	{
		m_stickersList->SetSelection(-1);
		m_stickersList->Enable(true);
		m_addStickerWindow->Hide();
		wxMessageBox("Add sticker succeed", "Information", wxOK | wxICON_INFORMATION);
	}
}

void CEdStickersPanel::OnClickAddStickerCancel( wxCommandEvent& event )
{
	if(wxMessageBox("Do you want close window?", "Closing...", wxYES_NO | wxICON_QUESTION) == wxYES)
	{
		if(m_attachedMarkerSystem->GetNewSticker()->m_stickerEntity != nullptr )
		{
			m_attachedMarkerSystem->GetNewSticker()->m_stickerEntity.Get()->Destroy();
		}
		m_stickersList->SetSelection(-1);
		m_stickersList->Enable(true);
		m_addStickerWindow->Hide();
	}
}

void CEdStickersPanel::OnClickModifySticker( wxCommandEvent& event )
{
	if(m_editStickerButton->GetLabel() == "Modify Sticker")
	{
		m_showModifyStickerWindow->SetTitle("Modify sticker");
		m_editStickerButton->SetLabel("Accept modification");
		m_cancelStickerButton->SetLabel("Cancel");
		m_stickerTitle->Enable();
		m_stickerDescription->Enable();
		m_stickerCategory->Enable();

		// unlock entity (can move it)
		if(m_stickerOpenToEdit->m_stickerEntity != nullptr )
		{
			TagList tags = m_stickerOpenToEdit->m_stickerEntity.Get()->GetTags();
			if( tags.HasTag(CNAME( LockedObject )) == true )
			{
				tags.SubtractTag(CNAME( LockedObject ));
				m_stickerOpenToEdit->m_stickerEntity.Get()->SetTags(tags);
			}
		}
	}
	else if(m_editStickerButton->GetLabel() == "Accept modification")
	{
		m_stickerOpenToEdit->m_title = m_stickerTitle->GetValue();
		m_stickerOpenToEdit->m_description = m_stickerDescription->GetValue();
		m_stickerOpenToEdit->m_type = m_stickerCategory->GetSelection()+1;
		if(m_attachedMarkerSystem->ModifySticker(*m_stickerOpenToEdit) == true)
		{
			// lock entity (can't move it)
			if(m_stickerOpenToEdit->m_stickerEntity != nullptr)
			{
				TagList tags = m_stickerOpenToEdit->m_stickerEntity.Get()->GetTags();
				tags.AddTag(CNAME( LockedObject ));
				m_stickerOpenToEdit->m_stickerEntity.Get()->SetTags(tags);
			}

			m_stickersList->SetSelection(-1);
			m_stickersList->Enable(true);
			m_showModifyStickerWindow->Hide();

			wxMessageBox("Sticker was modified.", "Success", wxOK | wxICON_INFORMATION);
		}
	}
	m_showModifyStickerWindow->Layout();
}

void CEdStickersPanel::OnClickCloseStickerWindow( wxCommandEvent& event )
{
	if(m_cancelStickerButton->GetLabel() == "Close")
	{
		m_showModifyStickerWindow->Hide();
	}
	else if(m_cancelStickerButton->GetLabel() == "Cancel")
	{
		if(wxMessageBox("Do you want close window?", "Closing...", wxYES_NO | wxICON_QUESTION) == wxYES)
		{
			m_showModifyStickerWindow->Hide();
		}
	}

	// lock entity (can't move it)
	if(m_stickerOpenToEdit->m_stickerEntity != nullptr)
	{
		TagList tags = m_stickerOpenToEdit->m_stickerEntity.Get()->GetTags();
		tags.AddTag(CNAME( LockedObject ));
		m_stickerOpenToEdit->m_stickerEntity.Get()->SetTags(tags);
	}

	m_stickersList->SetSelection(-1);
	m_stickersList->Enable(true);
	m_showModifyStickerWindow->Layout();
}

void CEdStickersPanel::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if( name == CNAME( NodeSelected ) )
	{
		CEntity* entity = Cast< CEntity >( GetEventData< CNode* >( data ) );
		if( entity != nullptr )
		{
			if( entity->GetTags().HasTag( CNAME(StickerObject) ) == true )
			{
				if(m_stickerOpenToEdit == nullptr && m_showModifyStickerWindow->IsVisible() == false)
				{
					const TDynArray<CSticker*>& stickers = m_attachedMarkerSystem->GetStickers();
					for(Uint32 i=0; i<stickers.Size(); ++i)
					{
						if( stickers[i]->m_stickerEntity.Get() == entity )
						{
							m_stickersList->Select(i);
							m_stickerOpenToEdit = stickers[i];
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
			if( entity->GetTags().HasTag( CNAME(StickerObject) ) == true )
			{
				if( m_showModifyStickerWindow->IsVisible() == false )
				{
					m_stickersList->DeselectAll();
					m_stickerOpenToEdit = nullptr;
				}
			}
		}
	}
	else if( name == CNAME( EditorTick ) )
	{
		InternalProcessMessage();
	}
}

void CEdStickersPanel::ProcessMessage( enum EMarkerSystemMessage message, enum EMarkerSystemType systemType, IMarkerSystemInterface* system )
{
	switch( message )
	{
	case MSM_SystemRegistered:
		{
			if( systemType == MST_Sticker )
			{
				m_attachedMarkerSystem = static_cast< CStickersSystem* >( system );
				this->Enable();
			}
		}
		return;
	case MSM_SystemUnregistered:
		{
			if( systemType == MST_Sticker )
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

void CEdStickersPanel::InternalProcessMessage()
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
			FillStickersWindow();

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
			FillStickersWindow();
			this->Enable();
			GFeedback->EndTask();
		}
		break;
	case MSM_DataAreUpdated:
	case MSM_DataAreSorted:
		{
			this->Disable();
			FillStickersWindow();
			this->Enable();
		}
		break;
	}
}

void CEdStickersPanel::OnSearchButtonClicked( wxCommandEvent& event )
{
	String phrase = m_searchLine->GetValue();
	m_attachedMarkerSystem->SetSearchFilter( phrase );
}

void CEdStickersPanel::OnSearchEnterClicked( wxCommandEvent& event )
{
	OnSearchButtonClicked( wxCommandEvent() );
}

#endif	// NO_MARKER_SYSTEM
