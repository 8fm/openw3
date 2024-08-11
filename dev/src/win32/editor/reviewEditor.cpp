/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifndef NO_MARKER_SYSTEMS

#include <wx/wupdlock.h>
#include "../../common/engine/reviewSystem.h"
#include "reviewEditor.h"
#include "sceneExplorer.h"
#include "../../common/core/depot.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/bitmapTexture.h"


BEGIN_EVENT_TABLE( CEdReviewPanel, wxPanel )
	EVT_TOOL( XRCID("m_showFlag"), CEdReviewPanel::OnShowFlagWindow )
	EVT_TOOL( XRCID("m_newFlag"), CEdReviewPanel::OnShowAddNewFlagWindow )
	EVT_TOOL( XRCID("m_editFlag"), CEdReviewPanel::OnShowModifyFlagWindow )
	EVT_TOOL( XRCID("m_filters"), CEdReviewPanel::OnShowFilterPanel )
	EVT_TOOL( XRCID("m_advancedSettings"), CEdReviewPanel::OnShowSettingsPanel )
	EVT_TOOL( XRCID("m_refreshFlags"), CEdReviewPanel::OnRefreshFlag )
END_EVENT_TABLE()

CEdReviewPanel::CEdReviewPanel( wxWindow* parent ) 
	: m_attachedMarkerSystem( nullptr )
	, m_flagOpenToEdit( nullptr )
{
	// Load layouts from XRC
	wxXmlResource::Get()->LoadPanel( this, parent, wxT("FlagsListPanel") );

	// register listener for messages from review marker tool
	GEngine->GetMarkerSystems()->RegisterListener( MST_Review, this );

	// Load additional windows
	m_addNewFlagWindow = wxXmlResource::Get()->LoadDialog( this, "AddNewFlagWindow" );	
	m_modifyFlagWindow = wxXmlResource::Get()->LoadDialog( this, "ModifyFlagWindow" );
	m_showFlagWindow = wxXmlResource::Get()->LoadDialog( this, "ShowFlagWindow" );
	m_showFullScreenWindow = wxXmlResource::Get()->LoadDialog( this, "ShowFullScreen" );

	// Load controls and set values
	m_autoSyncTime = XRCCTRL( *this, "m_autoSyncTime", wxSpinCtrl );
	m_autoSyncTime->SetValue(m_attachedMarkerSystem->GetSyncTime());
	m_autoSyncTime->Connect(wxEVT_COMMAND_SPINCTRL_UPDATED , wxCommandEventHandler( CEdReviewPanel::OnUpdateAutoSyncTime), nullptr, this );
	m_autoSyncEnable = XRCCTRL( *this, "m_autoSyncEnable", wxCheckBox );
	m_autoSyncEnable->SetValue(m_attachedMarkerSystem->GetAutoSync());
	m_autoSyncEnable->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED , wxCommandEventHandler( CEdReviewPanel::OnEnableAutoSyncFlag), nullptr, this );
	m_filtersPanel = XRCCTRL( *this, "m_filtersPanel", wxPanel );
	m_settingsPanel = XRCCTRL( *this, "m_advancedSettingsPanel", wxPanel );
	m_mainPanel = XRCCTRL( *this, "m_mainPanel", wxPanel );
	m_connectionPanel = XRCCTRL( *this, "m_connectionPanel", wxPanel );
	m_connectionInfo = XRCCTRL( *this, "m_disconnectInformation", wxStaticText );
	m_mainToolbar = XRCCTRL( *this, "m_reviewSystemToolbar", wxToolBar );

	// create list for flags
	m_flagList = XRCCTRL( *this, "m_flagList", wxListCtrl );
	m_flagList->AppendColumn( TXT("Name"), wxLIST_FORMAT_LEFT, 200 );
	m_flagList->AppendColumn( TXT("Priority"), wxLIST_FORMAT_LEFT, 50 );
	m_flagList->AppendColumn( TXT("State"), wxLIST_FORMAT_LEFT, 100 );
	m_flagList->AppendColumn( TXT("Type"), wxLIST_FORMAT_LEFT, 80 );
	m_flagList->AppendColumn( TXT("Date"), wxLIST_FORMAT_LEFT, 120 );
	m_flagList->Bind( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, &CEdReviewPanel::OnSelectRowInGrid, this );
	m_flagList->Bind( wxEVT_COMMAND_LIST_COL_RIGHT_CLICK, &CEdReviewPanel::OnShowColumnContextMenu, this );
	m_flagList->Bind( wxEVT_COMMAND_LIST_COL_CLICK, &CEdReviewPanel::OnSortByColumn, this );
	m_flagList->Bind( wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK, &CEdReviewPanel::OnShowRowContextMenu, this );

	wxImageList* m_pImageList = new wxImageList(16,16);
	m_flagList->SetImageList( m_pImageList, wxIMAGE_LIST_SMALL );

	// settings panel
	m_projectName = XRCCTRL( *this, "m_projectsTTP", wxChoice );
	m_projectName->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdReviewPanel::OnTTPProjectSelecetd ), nullptr, this );
	m_milestoneName = XRCCTRL( *this, "m_milestonesTTP", wxChoice );
	m_milestoneName->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdReviewPanel::OnTTPMilestoneSelected ), nullptr, this );

	// filters panel
	m_filterShowOnMap = XRCCTRL( *this, "m_filterShowOnMap", wxCheckBox );
	m_filterShowOnMap->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdReviewPanel::OnCheckShowOnMap), nullptr, this );
	m_filterDownloadClosedFlags = XRCCTRL( *this, "m_filterDownloadClosedFlags", wxCheckBox );
	m_filterDownloadClosedFlags->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdReviewPanel::OnCheckShowClosedFlag), nullptr, this );
	m_filterStatesList = XRCCTRL( *this, "m_filterStatesList", wxCheckListBox );
	m_filterTypesList = XRCCTRL( *this, "m_filterTypesList", wxCheckListBox );
	m_filterButton = XRCCTRL( *this, "m_filterButton", wxButton );
	m_filterButton->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdReviewPanel::OnClickFilter), nullptr, this );
	m_filterResetButton = XRCCTRL( *this, "m_filterReset", wxButton );
	m_filterResetButton->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdReviewPanel::OnClickClearFilter), nullptr, this );

	// Show flag window
	wxButton* m_showEditFlagWindow = XRCCTRL( *this, "m_editFlag", wxButton );
	m_showEditFlagWindow->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdReviewPanel::OnClickEditFlagInShow ), nullptr, this );
	wxButton* m_closeShowFlagWindow = XRCCTRL( *this, "m_closeShowWindow", wxButton );
	m_closeShowFlagWindow->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdReviewPanel::OnClickCloseShowWindow ), nullptr, this );
	m_showFullScreenButton = XRCCTRL( *this, "m_showScreenButton", wxBitmapButton );
	m_showFullScreenButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdReviewPanel::OnClickShowFullScreen ), nullptr, this );
	m_showSummary = XRCCTRL( *m_showFlagWindow, "m_showSummary", wxStaticText );
	m_showPriority = XRCCTRL( *m_showFlagWindow, "m_showPriority", wxStaticText );
	m_showState = XRCCTRL( *m_showFlagWindow, "m_showState", wxStaticText );
	m_showType = XRCCTRL( *m_showFlagWindow, "m_showType", wxStaticText );
	m_showAuthor = XRCCTRL( *m_showFlagWindow, "m_showAuthor", wxStaticText );
	m_showCreatedDate = XRCCTRL( *m_showFlagWindow, "m_showCreatedDate", wxStaticText );
	m_commentsList = XRCCTRL( *m_showFlagWindow, "m_showCommentsList", wxListBox );
	m_commentsList->Connect(wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( CEdReviewPanel::OnClickCommentInList), nullptr, this );
	m_commentsList->Connect(wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( CEdReviewPanel::OnDoubleClickCommentInList), nullptr, this );
	m_showCommentAuthor = XRCCTRL( *m_showFlagWindow, "m_showCommentAuthor", wxStaticText );
	m_showCommentState = XRCCTRL( *m_showFlagWindow, "m_showCommentState", wxStaticText );
	m_showCommentPriority = XRCCTRL( *m_showFlagWindow, "m_showCommentPriority", wxStaticText );
	m_showCommentDescription = XRCCTRL( *m_showFlagWindow, "m_showCommentDescription", wxTextCtrl );
	m_showTestTrackNumber = XRCCTRL( *m_showFlagWindow, "m_showTestTrackId", wxStaticText );

	// Show full screen window
	m_fullScreenButton = XRCCTRL( *this, "m_fullScreenButton", wxBitmapButton );
	m_fullScreenButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdReviewPanel::OnClickCloseFullScreen ), nullptr, this );

	// New flag window
	wxButton* m_cancelAddWindow = XRCCTRL( *this, "m_cancelButton", wxButton );
	m_cancelAddWindow->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdReviewPanel::OnCancelButtonClick ), nullptr, this );
	wxButton* m_addNewFlagButton = XRCCTRL( *this, "m_addFlagButton", wxButton );
	m_addNewFlagButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdReviewPanel::OnAddNewFlag ), nullptr, this );
	wxButton* m_setFlagPosButton = XRCCTRL( *this, "m_newFlagPosition", wxButton );
	m_setFlagPosButton->Connect( wxEVT_LEFT_DOWN, wxCommandEventHandler( CEdReviewPanel::OnClickSetFlagPos ), nullptr, this );
	m_newFlagSummary = XRCCTRL( *m_addNewFlagWindow, "m_newFlagSummary", wxTextCtrl );
	m_newFlagDescription = XRCCTRL( *m_addNewFlagWindow, "m_newFlagDescription", wxTextCtrl );
	m_newFlagPriority = XRCCTRL( *m_addNewFlagWindow, "m_newFlagPriority", wxChoice );
	m_newFlagType = XRCCTRL( *m_addNewFlagWindow, "m_newFlagType", wxChoice );
	m_editNewScreenAfterAdding = XRCCTRL( *m_addNewFlagWindow, "m_editNewScreenAfterAdding", wxCheckBox );

	// Modify flag window
	wxButton* m_modifyFlagButton = XRCCTRL( *this, "m_modifyFlag", wxButton );
	m_modifyFlagButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdReviewPanel::OnModifyFlag ), nullptr, this );
	wxButton* m_cancelModifyButton = XRCCTRL( *this, "m_cancelModification", wxButton );
	m_cancelModifyButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdReviewPanel::OnModifyCancel), nullptr, this );
	m_modifyFlagComment = XRCCTRL( *m_modifyFlagWindow, "m_modifyComment", wxTextCtrl );
	m_modifyFlagPriority = XRCCTRL( *m_modifyFlagWindow, "m_modifyPriority", wxChoice );
	m_modifyFlagState = XRCCTRL( *m_modifyFlagWindow, "m_modifyState", wxChoice );
	m_editScreenAfterAdding = XRCCTRL( *m_modifyFlagWindow, "m_editScreenAfterAdding", wxCheckBox );
	m_editKeepState = XRCCTRL( *m_modifyFlagWindow, "m_keepState", wxCheckBox );
	m_editKeepState->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdReviewPanel::OnKeepStateChanged ), nullptr, this );
	m_editKeepPriority = XRCCTRL( *m_modifyFlagWindow, "m_keepPriority", wxCheckBox );
	m_editKeepPriority->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdReviewPanel::OnKeepPriorityChanged ), nullptr, this );
	m_makeScreen = XRCCTRL( *m_modifyFlagWindow, "m_makeScreen", wxCheckBox );

	// create context menu for column
	m_columnContextMenu = new wxMenu();
	m_columnContextMenu->Append( RMC_Priority, TXT("Priority"), TXT(""), (wxItemKind)wxITEM_CHECK )->Check( true );
	m_columnContextMenu->Append( RMC_State, TXT("State"), TXT(""), (wxItemKind)wxITEM_CHECK )->Check( true );
	m_columnContextMenu->Append( RMC_Type, TXT("Type"), TXT(""), (wxItemKind)wxITEM_CHECK )->Check( true );
	m_columnContextMenu->Append( RMC_Date, TXT("Date"), TXT(""), (wxItemKind)wxITEM_CHECK )->Check( true );
	m_columnContextMenu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdReviewPanel::OnColumnPopupClick, this );

	// create context menu for one row
	m_rowContextMenu = new wxMenu();
	m_rowContextMenu->Append( 0, TXT("Look at"), TXT("") );
	m_rowContextMenu->AppendSeparator();
	m_rowContextMenu->Append( 1, TXT("Show"), TXT("") );
	m_rowContextMenu->Append( 2, TXT("Edit"), TXT("") );
	m_rowContextMenu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdReviewPanel::OnRowPopupClick, this );

	m_searchLine = XRCCTRL( *this, "m_searchLine", wxTextCtrl );
	m_searchLine->Bind( wxEVT_COMMAND_TEXT_ENTER, &CEdReviewPanel::OnSearchEnterClicked, this );
	m_searchButton = XRCCTRL( *this, "m_searchButton", wxButton );	
	m_searchButton->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdReviewPanel::OnSearchButtonClicked, this );	
	m_searchCategory = XRCCTRL( *this, "m_searchCategory", wxChoice );

	wxBitmapButton* icon = XRCCTRL( *this, "m_openedMarkerIcon", wxBitmapButton );
	m_pImageList->Add( icon->GetBitmap() );
	icon = XRCCTRL( *this, "m_fixedMarkerIcon", wxBitmapButton );
	m_pImageList->Add( icon->GetBitmap() );
	icon = XRCCTRL( *this, "m_closedMarkerIcon", wxBitmapButton );
	m_pImageList->Add( icon->GetBitmap() );
	icon = XRCCTRL( *this, "m_openedMarkerIcon", wxBitmapButton );	// for re-opened state
	m_pImageList->Add( icon->GetBitmap() );

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

CEdReviewPanel::~CEdReviewPanel()
{
	GEngine->GetMarkerSystems()->UnregisterListener( MST_Review, this );

	SEvents::GetInstance().UnregisterListener( CNAME( NodeSelected ), this );
	SEvents::GetInstance().UnregisterListener( CNAME( NodeDeselected ), this );
	SEvents::GetInstance().UnregisterListener( CNAME( EditorTick ), this );
}

void CEdReviewPanel::FillFlagsWindow()
{
	wxWindowUpdateLocker localUpdateLocker( this );

	if( m_flagList->GetItemCount() > 0 )
	{
		m_flagList->DeleteAllItems();
	}

	TDynArray< CReviewFlag* > flags;
	m_attachedMarkerSystem->GetFlags( flags );
	for( Int32 i=flags.Size()-1; i>=0; --i )
	{
		Int32 rowIndex = m_flagList->InsertItem( RMC_Name, flags[i]->m_summary.AsChar() );

		// fill row
		const Uint32 lastCommentIndex = flags[i]->m_comments.Size() - 1;
		m_flagList->SetItem( rowIndex, RMC_Priority, ToString( flags[i]->m_comments[lastCommentIndex].m_priority ).AsChar() );

		TDynArray< String > states;
		m_attachedMarkerSystem->GetStateList( states );
		Int32 stateIndex = flags[i]->m_comments[lastCommentIndex].m_state-1;
		if( stateIndex < 0 || stateIndex > (Int32)( states.Size() - 1 ) )
		{
			m_flagList->SetItem( rowIndex, RMC_State, TXT("Incorrect state, please fix it.") );
		}
		else
		{
			m_flagList->SetItem( rowIndex, RMC_State, states[stateIndex].AsChar() );
		}

		TDynArray< String > types;
		m_attachedMarkerSystem->GetBugTypeList( types );
		Int32 typeIndex = flags[i]->m_type;
		if( typeIndex < 0 || typeIndex > (Int32)( types.Size() - 1 ) )
		{
			m_flagList->SetItem( rowIndex, RMC_Type, TXT("Incorrect type, please fix it.") );
		}
		else
		{
			m_flagList->SetItem( rowIndex, RMC_Type, types[typeIndex].AsChar() );
		}

		tm* localTime = &flags[i]->m_comments[lastCommentIndex].m_creationDate;
		String dateTime = ToString(localTime->tm_year)+TXT("-")+ToString(localTime->tm_mon)+TXT("-")+ToString(localTime->tm_mday)+TXT("_");
		dateTime += ToString(localTime->tm_hour)+TXT(":")+ToString(localTime->tm_min)+TXT(":")+ToString(localTime->tm_sec);
		dateTime.ReplaceAll(TXT("_"), TXT(" "), true);
		m_flagList->SetItem( rowIndex, RMC_Date, dateTime.AsChar() );

		m_flagList->SetItemImage( rowIndex, flags[i]->m_comments[lastCommentIndex].m_state-1 );
	}

	LayoutRecursively( this, false );
}

void CEdReviewPanel::FillFilterPanel()
{
	m_filterShowOnMap->SetValue(m_attachedMarkerSystem->GetShowOnMap());
	m_filterDownloadClosedFlags->SetValue(m_attachedMarkerSystem->GetDownloadClosedFlags());

	TDynArray< String > states;
	TDynArray< Bool > stateValues;
	m_attachedMarkerSystem->GetStateList( states );
	m_attachedMarkerSystem->GetStateValues( stateValues );

	m_filterStatesList->Clear();
	const Uint32 stateCount = states.Size();
	for(Uint32 i=0; i<stateCount; ++i)
	{
		m_filterStatesList->Insert( states[i].AsChar(), 0 );
		if( stateValues[i] == true )
		{
			m_filterStatesList->Check( 0, true );
		}
	}
	
	TDynArray< String > types;
	TDynArray< Bool > typeValues;
	m_attachedMarkerSystem->GetBugTypeList( types );
	m_attachedMarkerSystem->GetTypesValues( typeValues );

	m_filterTypesList->Clear();
	const Uint32 typeCount = types.Size();
	for(Uint32 i=0; i<typeCount; ++i)
	{
		m_filterTypesList->Insert( types[i].AsChar(), 0 );
		if( typeValues[i] == true )
		{
			m_filterTypesList->Check(0, true);
		}
	}
}

void CEdReviewPanel::FillScreenMiniViewer(CReviewFlagComment& comment)
{
	m_showFullScreenButton->Enable( false );

	if(	comment.GetFlagScreen() != nullptr )
	{
		CBitmapTexture* texture = comment.GetFlagScreen();

		wxImage* image = new wxImage(texture->GetSourceData()->GetWidth(), texture->GetSourceData()->GetHeight());
		unsigned char* data = image->GetData();
		int w = image->GetWidth(), h = image->GetHeight();

		unsigned char* pix = (unsigned char*)texture->GetSourceData()->GetBufferAccessPointer();
		int x,y;
		for(y=0; y<h; ++y)
		{
			for(x=0; x<w; ++x)
			{
				// skip alpha channel
				long posWrite = (y * w + x) * 3;
				long posRead = (y * w + x) * 4;
				data[posWrite] = pix[posRead];
				data[posWrite+1] = pix[posRead+1];
				data[posWrite+2] = pix[posRead+2];
			}
		}

		m_screen = new wxBitmap(*image);

		if( m_screen->IsOk() == true )
		{
			wxBitmap* thumbnail = new wxBitmap(*m_screen);
			wxImage tempImage = thumbnail->ConvertToImage();
			thumbnail = new wxBitmap(tempImage.Rescale(124,124));
			m_showFullScreenButton->SetBitmap( *thumbnail );
			m_showFullScreenButton->Enable( true );
		}
	}
}

void CEdReviewPanel::OnShowFlagWindow( wxCommandEvent& event )
{
	Int32 selectedItemIndex = GetSelectedItemIndex();

	if( selectedItemIndex != -1 )
	{
		if(m_showFlagWindow->IsVisible() == false)
		{
			m_attachedMarkerSystem->LockUpdate();

			// clear data
			m_showSummary->SetLabelText("");
			m_showPriority->SetLabelText("");
			m_showState->SetLabelText("");
			m_showType->SetLabelText("");
			m_showAuthor->SetLabelText("");
			m_showCreatedDate->SetLabelText("");
			m_showCommentAuthor->SetLabelText("");
			m_showCommentState->SetLabelText("");
			m_showCommentPriority->SetLabelText("");
			m_showTestTrackNumber->SetLabelText("");
			m_showCommentDescription->SetValue("");

			m_flagList->Enable(false);

			m_flagOpenToEdit = m_attachedMarkerSystem->GetFlag( selectedItemIndex );

			m_showSummary->SetLabelText(m_flagOpenToEdit->m_summary.AsChar());
			Uint32 newestCommentIndex = m_flagOpenToEdit->m_comments.Size()-1;
			m_showPriority->SetLabelText(ToString(m_flagOpenToEdit->m_comments[newestCommentIndex].m_priority).AsChar());
			TDynArray<String> states;
			m_attachedMarkerSystem->GetStateList( states );
			Int32 stateIndex = m_flagOpenToEdit->m_comments[newestCommentIndex].m_state - 1;
			if( stateIndex < 0 || stateIndex > Int32 ( states.Size() - 1 ) )
			{
				m_showState->SetLabelText( TXT("Incorrect state, please fix it.") );
			}
			else
			{
				m_showState->SetLabelText(states[stateIndex].AsChar());
			}
			TDynArray<String> types;
			m_attachedMarkerSystem->GetBugTypeList( types );
			Int32 typeIndex = m_flagOpenToEdit->m_type;
			if( typeIndex < 0 || typeIndex > (Int32)(types.Size() - 1 ) )
			{
				m_showType->SetLabelText( TXT("Incorrect type, please fix it.") );
			}
			else
			{
				m_showType->SetLabelText(types[typeIndex].AsChar());
			}
			m_showAuthor->SetLabelText(m_flagOpenToEdit->m_comments[0].m_author.AsChar());
			m_showTestTrackNumber->SetLabelText(ToString(m_flagOpenToEdit->m_testTrackNumber).AsChar());
			if(m_flagOpenToEdit->m_linkToVideo.Empty() == false)
			{
				m_showFlagWindow->Layout();
			}
			else
			{
				m_showFlagWindow->Layout();
			}
			
			tm* localTime = &m_flagOpenToEdit->m_comments[0].m_creationDate;
			String dateTime = ToString(localTime->tm_year)+TXT("-")+ToString(localTime->tm_mon)+TXT("-")+ToString(localTime->tm_mday)+TXT("_");
			dateTime += ToString(localTime->tm_hour)+TXT(":")+ToString(localTime->tm_min)+TXT(":")+ToString(localTime->tm_sec);
			dateTime.ReplaceAll(TXT("_"), TXT(" "), true);
			m_showCreatedDate->SetLabelText(dateTime.AsChar());
	
			// add comments
			Uint32 commentCount = m_flagOpenToEdit->m_comments.Size();

			if(commentCount > 0)
			{
				wxArrayString tab;
				for (Uint32 i=0; i<commentCount; ++i)
				{
					tm* localTime = &m_flagOpenToEdit->m_comments[i].m_creationDate;
					String dateTime = ToString(localTime->tm_year)+TXT("-")+ToString(localTime->tm_mon)+TXT("-")+ToString(localTime->tm_mday)+TXT("_");
					dateTime += ToString(localTime->tm_hour)+TXT(":")+ToString(localTime->tm_min)+TXT(":")+ToString(localTime->tm_sec);
					dateTime.ReplaceAll(TXT("_"), TXT(" "), true);
					tab.Add(dateTime.AsChar());
				}

				m_commentsList->Clear();
				m_commentsList->InsertItems(tab, 0);
			}

			// show screen
			Uint32 lastComment = m_flagOpenToEdit->m_comments.Size()-1;
			ASSERT(lastComment >= 0);
			FillScreenMiniViewer(m_flagOpenToEdit->m_comments[lastComment]);

			m_showFlagWindow->CenterOnScreen();
			m_showFlagWindow->Show();
		}
	}
	else
	{
		wxMessageBox("At first, you must choose flag from the list", "Information", wxOK | wxICON_INFORMATION);
	}
}

void CEdReviewPanel::OnShowAddNewFlagWindow( wxCommandEvent& event )
{
	if(m_addNewFlagWindow->IsVisible() == false)
	{
		m_attachedMarkerSystem->LockUpdate();

		m_editNewScreenAfterAdding->SetValue( false );

		// clear data
		m_newFlagSummary->SetValue("");
		m_newFlagDescription->SetValue("");
		m_newFlagPriority->Select(-1);
		m_newFlagType->Select(-1);

		// set correct values
		TDynArray<String> types;
		m_attachedMarkerSystem->GetBugTypeList( types );
		m_newFlagType->Clear();
		for(Uint32 i=0; i<types.Size(); ++i)
		{
			m_newFlagType->Append(types[i].AsChar());
		}
		( types.Size() > 0 ) ? m_newFlagType->Select( 0 ) : m_newFlagType->Select( -1 );

		TDynArray< String > priorities;
		m_attachedMarkerSystem->GetPriorityList( priorities );
		m_newFlagPriority->Clear();
		for(Uint32 i=0; i<priorities.Size(); ++i)
		{
			m_newFlagPriority->Append( priorities[i].AsChar() );
		}
		( priorities.Size() > 0 ) ? m_newFlagPriority->Select( 0 ) : m_newFlagPriority->Select( -1 );

		this->Refresh();

		m_addNewFlagWindow->CenterOnScreen();
		m_addNewFlagWindow->Show();
		m_attachedMarkerSystem->CreateNewFlag();
	}
}

void CEdReviewPanel::OnShowModifyFlagWindow( wxCommandEvent& event )
{
	Int32 selectedItemIndex = GetSelectedItemIndex();

	if( selectedItemIndex != -1 )
	{
		if(m_modifyFlagWindow->IsVisible() == false)
		{
			m_attachedMarkerSystem->LockUpdate();

			// clear data 
			m_modifyFlagComment->SetValue("");
			m_editKeepState->SetValue( true );
			m_modifyFlagState->Enable( false );
			m_modifyFlagState->SetSelection( -1 );
			m_editKeepPriority->SetValue( true );
			m_modifyFlagPriority->Enable( false );
			m_modifyFlagPriority->SetSelection( -1 );

			//
			TDynArray<String> states;
			m_attachedMarkerSystem->GetStateList( states );
			m_modifyFlagState->Clear();
			for(Uint32 i=0; i<states.Size(); ++i)
			{
				if( i == RFS_Opened )
				{
					continue;
				}
				m_modifyFlagState->Append(states[i].AsChar());
			}

			TDynArray< String > priorities;
			m_attachedMarkerSystem->GetPriorityList( priorities );
			m_modifyFlagPriority->Clear();
			for(Uint32 i=0; i<priorities.Size(); ++i)
			{
				m_modifyFlagPriority->Append( priorities[i].AsChar() );
			}

			m_flagList->Enable(false);
			this->Refresh();

			m_flagOpenToEdit = m_attachedMarkerSystem->GetFlag( selectedItemIndex );

			// get last comment
			Uint32 commentCount = m_flagOpenToEdit->m_comments.Size();
			const CReviewFlagComment& lastComment = m_flagOpenToEdit->m_comments[commentCount-1];

			// unlock entity (can move it)
			if(m_flagOpenToEdit->m_flagEntity != nullptr)
			{
				TagList tags = m_flagOpenToEdit->m_flagEntity->GetTags();
				if( tags.HasTag(CNAME( LockedObject )) == true )
				{
					tags.SubtractTag(CNAME( LockedObject ));
					m_flagOpenToEdit->m_flagEntity->SetTags(tags);
				}
			}

			m_modifyFlagWindow->CenterOnScreen();
			m_modifyFlagWindow->Show();
		}
	}
	else
	{
		wxMessageBox("At first, you must choose flag from the list", "Information", wxOK | wxICON_INFORMATION);
	}
}

void CEdReviewPanel::OnRefreshFlag( wxCommandEvent& event )
{
	m_attachedMarkerSystem->SendRequest( MSRT_SynchronizeData );
}

void CEdReviewPanel::OnEnableAutoSyncFlag( wxCommandEvent& event )
{
	m_attachedMarkerSystem->SetAutoSync(event.IsChecked());
}

void CEdReviewPanel::OnUpdateAutoSyncTime( wxCommandEvent& event )
{
	m_attachedMarkerSystem->SetSyncTime(event.GetInt());
}

void CEdReviewPanel::OnShowFilterPanel( wxCommandEvent& event )
{
	wxWindowUpdateLocker localUpdateLocker( this );

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

void CEdReviewPanel::OnShowSettingsPanel( wxCommandEvent& event )
{
	wxWindowUpdateLocker localUpdateLocker( this );

	if( m_settingsPanel->IsShown() == true )
	{
		m_settingsPanel->Hide();
	}
	else
	{
		// set projects
		TDynArray< String > ttpProjects;
		m_attachedMarkerSystem->GetProjectList( ttpProjects );
		m_projectName->Clear();
		for( Uint32 i=0; i<ttpProjects.Size(); ++i )
		{
			m_projectName->Append( ttpProjects[i].AsChar() );
		}
		m_projectName->Select( m_projectName->FindString( m_attachedMarkerSystem->GetDefaultProjectName().AsChar() ) );

		TDynArray< String > ttpMilestones;
		m_attachedMarkerSystem->GetMilestoneList( ttpMilestones );
		m_milestoneName->Clear();
		for( Uint32 i=0; i<ttpMilestones.Size(); ++i )
		{
			m_milestoneName->Append( ttpMilestones[i].AsChar() );
		}
		m_milestoneName->Select( m_milestoneName->FindString( m_attachedMarkerSystem->GetDefaultMilestoneName().AsChar() ) );

		m_settingsPanel->Show();
	}

	LayoutRecursively( this, false );
}

void CEdReviewPanel::OnCheckShowOnMap( wxCommandEvent& event )
{
	m_attachedMarkerSystem->SetShowOnMap(event.GetInt() != 0);
}

void CEdReviewPanel::OnCheckShowClosedFlag( wxCommandEvent& event )
{
	m_attachedMarkerSystem->SetDownloadClosedFlags(event.GetInt() != 0);
}

void CEdReviewPanel::OnClickFilter( wxCommandEvent& event )
{
	TDynArray<Bool> states;
	for(Int32 i=m_filterStatesList->GetCount()-1; i>-1; --i)
	{
		states.PushBack(m_filterStatesList->IsChecked(i));
	}
	TDynArray<Bool> types;
	for(Int32 i=m_filterTypesList->GetCount()-1; i>-1; --i)
	{
		types.PushBack(m_filterTypesList->IsChecked(i));
	}

	m_attachedMarkerSystem->SetFilters( states, types );
}

void CEdReviewPanel::OnClickClearFilter( wxCommandEvent& event )
{
	for (Uint32 i=0; i<m_filterStatesList->GetCount(); ++i)
	{
		m_filterStatesList->Check(i, true);
	}

	for (Uint32 i=0; i<m_filterTypesList->GetCount(); ++i)
	{
		m_filterTypesList->Check(i, true);
	}

	OnClickFilter( wxCommandEvent() );
}

void CEdReviewPanel::OnDoubleClickFlagInList( wxCommandEvent& event )
{
	Int32 selectedItemIndex = GetSelectedItemIndex();

	if( selectedItemIndex != -1 )
	{
		CReviewFlag* selectedFlag = m_attachedMarkerSystem->GetFlag( selectedItemIndex );
		CWorld* world = GGame->GetActiveWorld();
		if ( world )
		{
			CSelectionManager* selectionManager = world->GetSelectionManager();
			if( selectionManager != nullptr )
			{
				selectionManager->DeselectAll();
				if( selectedFlag->m_flagEntity != nullptr )
				{
					selectionManager->Select( selectedFlag->m_flagEntity.Get() );
				}
			}
		}

		CReviewFlagComment* selectedComment = &selectedFlag->m_comments[selectedFlag->m_comments.Size()-1];
		wxTheFrame->GetWorldEditPanel()->SetCameraPosition(selectedComment->m_cameraPosition);
		EulerAngles eulerAngles(selectedComment->m_cameraOrientation.X,selectedComment->m_cameraOrientation.Y,selectedComment->m_cameraOrientation.Z);
		wxTheFrame->GetWorldEditPanel()->SetCameraRotation(eulerAngles);
	}
}

void CEdReviewPanel::OnClickEditFlagInShow( wxCommandEvent& event )
{
	// set correct values
	TDynArray< String > states;
	m_attachedMarkerSystem->GetStateList( states );
	m_modifyFlagState->Clear();
	this->Refresh();
	for(Uint32 i=0; i<states.Size(); ++i)
	{
		if( i == RFS_Opened )
		{
			continue;
		}
		m_modifyFlagState->Append(states[i].AsChar());
	}
	m_modifyFlagState->Select( 0 );
	this->Refresh();

	TDynArray< String > priorities;
	m_attachedMarkerSystem->GetPriorityList( priorities );
	m_modifyFlagPriority->Clear();
	for(Uint32 i=0; i<priorities.Size(); ++i)
	{
		m_modifyFlagPriority->Append( priorities[i].AsChar() );
	}
	m_modifyFlagPriority->Select(m_flagOpenToEdit->m_comments[0].m_priority-1);

	// revert last position for flag
	Uint32 lastCommentIndex = m_flagOpenToEdit->m_comments.Size()-1;
	m_flagOpenToEdit->m_flagEntity->SetPosition(m_flagOpenToEdit->m_comments[lastCommentIndex].m_flagPosition);

	// unlock entity (can move it)
	if(m_flagOpenToEdit->m_flagEntity != nullptr)
	{
		TagList tags = m_flagOpenToEdit->m_flagEntity->GetTags();
		if( tags.HasTag(CNAME( LockedObject )) == true )
		{
			tags.SubtractTag(CNAME( LockedObject ));
			m_flagOpenToEdit->m_flagEntity->SetTags(tags);
		}
	}

	m_showFlagWindow->Hide();
	m_modifyFlagWindow->Show();
}

void CEdReviewPanel::OnClickCloseShowWindow( wxCommandEvent& event )
{
	// revert last position for flag
	Uint32 lastCommentIndex = m_flagOpenToEdit->m_comments.Size()-1;
	m_flagOpenToEdit->m_flagEntity->SetPosition(m_flagOpenToEdit->m_comments[lastCommentIndex].m_flagPosition);

	ClearSelection();

	m_flagList->Enable(true);
	m_showFlagWindow->Hide();

	m_attachedMarkerSystem->UnlockUpdate();
}

void CEdReviewPanel::OnClickShowFullScreen( wxCommandEvent& event )
{
	if(m_showFullScreenWindow->IsShown() == false)
	{
		m_showFullScreenWindow->SetPosition(wxPoint(0,0));
		// grab the native monitor resolution
		HDC Monitor      = GetDC(nullptr);
		int nativeWidth  = GetDeviceCaps(Monitor, HORZRES);
		int nativeHeight = GetDeviceCaps(Monitor, VERTRES);

		if( m_screen->IsOk() == true )
		{
			m_fullScreenButton->SetBitmap(*m_screen);
		}

		m_showFullScreenWindow->SetSize(nativeWidth, nativeHeight);
		m_showFullScreenWindow->Show();
	}
}

void CEdReviewPanel::OnClickCommentInList( wxCommandEvent& event )
{
	wxArrayInt selections;
	m_commentsList->GetSelections(selections);

	if(selections.size() > 0)
	{
		if(m_flagOpenToEdit != nullptr)
		{
			Uint32 selectedCommentIndex = (Uint32)selections[0];
			if(selectedCommentIndex < m_flagOpenToEdit->m_comments.Size())
			{
				CReviewFlagComment& selectedComment = m_flagOpenToEdit->m_comments[selections[0]];
				m_showCommentAuthor->SetLabelText(selectedComment.m_author.AsChar());
				m_showCommentPriority->SetLabelText(ToString(selectedComment.m_priority).AsChar());
				TDynArray< String > states;
				m_attachedMarkerSystem->GetStateList( states );
				m_showCommentState->SetLabelText(states[selectedComment.m_state-1].AsChar());
				m_showCommentDescription->SetValue(selectedComment.m_description.AsChar());

				m_flagOpenToEdit->m_flagEntity->SetPosition(selectedComment.m_flagPosition);
				wxTheFrame->GetWorldEditPanel()->SetCameraPosition(selectedComment.m_cameraPosition);
				EulerAngles eulerAngles(selectedComment.m_cameraOrientation.X,selectedComment.m_cameraOrientation.Y,selectedComment.m_cameraOrientation.Z);
				wxTheFrame->GetWorldEditPanel()->SetCameraRotation(eulerAngles);

				FillScreenMiniViewer(selectedComment);
			}
		}
	}
}

void CEdReviewPanel::OnDoubleClickCommentInList( wxCommandEvent& event )
{
	OnClickCommentInList(event);
}

void CEdReviewPanel::OnClickCloseFullScreen( wxCommandEvent& event )
{
	m_showFullScreenWindow->Hide();
}

void CEdReviewPanel::OnAddNewFlag( wxCommandEvent& event )
{
	if(m_newFlagSummary->GetValue().empty() == true)
	{
		wxMessageBox("Flag must have a summary!", "Remember!!!", wxOK | wxICON_INFORMATION);
		return;
	}
	if(m_newFlagDescription->GetValue().empty() == true)
	{
		wxMessageBox("Flag must have a description!", "Remember!!!", wxOK | wxICON_INFORMATION);
		return;
	}
	if(m_attachedMarkerSystem->GetNewFlag()->m_flagEntity == nullptr)
	{
		wxMessageBox("You must set flag on map!", "Remember!!!", wxOK | wxICON_INFORMATION);
		return;
	}

	CReviewFlagComment newComment;

	Char lpszUsername[255];
	DWORD dUsername = sizeof(lpszUsername);
	if(GetUserName(lpszUsername, &dUsername))
		newComment.m_author = lpszUsername;
	else
		newComment.m_author = TXT("Unknown user");

	newComment.m_priority = m_newFlagPriority->GetCurrentSelection();
	newComment.m_state = 1;
	const EulerAngles& eulerAngles = wxTheFrame->GetWorldEditPanel()->GetCameraRotation();
	newComment.m_cameraOrientation = Vector(eulerAngles.Roll, eulerAngles.Pitch, eulerAngles.Yaw);
	newComment.m_cameraPosition = wxTheFrame->GetWorldEditPanel()->GetCameraPosition();
	newComment.m_description = m_newFlagDescription->GetValue();

	CReviewFlag& newFlag = *m_attachedMarkerSystem->GetNewFlag();
	newFlag.m_linkToVideo = TXT("");
	newFlag.m_mapName = GGame->GetActiveWorld()->GetDepotPath();
	newFlag.m_summary = m_newFlagSummary->GetValue();
	newFlag.m_type = m_newFlagType->GetCurrentSelection();
	newComment.m_flagPosition = newFlag.m_flagEntity->GetWorldPosition();
	newFlag.m_comments.PushBack(newComment);

	if ( newFlag.m_flagEntity != nullptr )
	{
		// Add basic tags
		TagList tags = newFlag.m_flagEntity->GetTags();
		tags.AddTag(CNAME( LockedObject ));
		tags.AddTag(CNAME( ReviewFlagObject ));
		newFlag.m_flagEntity->SetTags( tags );
	}

	String screenPath = TXT("");
	if(m_attachedMarkerSystem->AddNewFlag( screenPath ) == true)
	{
		if( m_editNewScreenAfterAdding->IsChecked() == true )
		{
			OpenExternalFile( screenPath );
		}

		m_addNewFlagWindow->Hide();
		wxMessageBox("Add flag succeed", "Information", wxOK | wxICON_INFORMATION);
	}

	ClearSelection();
	m_flagList->Enable(true);

	m_attachedMarkerSystem->UnlockUpdate();
}

void CEdReviewPanel::OnCancelButtonClick( wxCommandEvent& event )
{
	if(wxMessageBox("Do you want close window?", "Closing...", wxYES_NO | wxICON_QUESTION) == wxYES)
	{
		if(m_attachedMarkerSystem->GetNewFlag()->m_flagEntity != nullptr)
		{
			m_attachedMarkerSystem->GetNewFlag()->m_flagEntity->Destroy();
		}
		m_addNewFlagWindow->Hide();
	}

	ClearSelection();
	m_flagList->Enable(true);

	m_attachedMarkerSystem->UnlockUpdate();
}

void CEdReviewPanel::OnClickSetFlagPos( wxCommandEvent& event )
{
	if(m_attachedMarkerSystem->GetNewFlag()->m_flagEntity == nullptr)
	{
		m_attachedMarkerSystem->WaitForEntity();

		THandle< CEntityTemplate > entityTemplate = m_attachedMarkerSystem->GetFlagTemplate( RFS_Opened );
		String drop = TXT("Resources:") + entityTemplate->GetDepotPath();
		wxTextDataObject myData( drop.AsChar() );

		wxDropSource dragSource(this);
		dragSource.SetData(myData);
		wxDragResult result = dragSource.DoDragDrop( wxDrag_DefaultMove );
	}
	else
	{
		wxMessageBox("Flag is on the map!", "Warning", wxOK | wxICON_WARNING);
		
		CWorld* world = GGame->GetActiveWorld();
		if ( world )
		{
			CSelectionManager* selectionManager = world->GetSelectionManager();
			if( selectionManager != nullptr )
			{
				selectionManager->DeselectAll();
				selectionManager->Select( m_attachedMarkerSystem->GetNewFlag()->m_flagEntity.Get() );
			}
		}
		SEvents::GetInstance().DispatchEvent( CNAME( CenterOnSelected ), nullptr );
	}
}

void CEdReviewPanel::OnModifyFlag( wxCommandEvent& event )
{
	if(m_modifyFlagComment->GetValue().empty() == true)
	{
		wxMessageBox("Comment must have a text!", "Remember!!!", wxOK | wxICON_INFORMATION);
		return;
	}

	CReviewFlagComment flagComment;
	flagComment.m_flagId = m_flagOpenToEdit->m_databaseId;

	Char lpszUsername[255];
	DWORD dUsername = sizeof(lpszUsername);
	if(GetUserName(lpszUsername, &dUsername))
		flagComment.m_author = lpszUsername;
	else
		flagComment.m_author = L"Unknown user";

	flagComment.m_description = m_modifyFlagComment->GetValue();

	if( m_editKeepState->GetValue() == true )
	{
		flagComment.m_state = m_flagOpenToEdit->m_comments[ m_flagOpenToEdit->m_comments.Size() - 1 ].m_state;
	}
	else
	{
		flagComment.m_state = m_modifyFlagState->GetCurrentSelection()+2;	// skip Opened state
	}

	if( m_editKeepPriority->GetValue() == true )
	{
		flagComment.m_priority = m_flagOpenToEdit->m_comments[ m_flagOpenToEdit->m_comments.Size() - 1 ].m_priority;
	}
	else
	{
		flagComment.m_priority = m_modifyFlagPriority->GetCurrentSelection();
	}

	const EulerAngles& eulerAngles = wxTheFrame->GetWorldEditPanel()->GetCameraRotation();
	flagComment.m_cameraOrientation = Vector(eulerAngles.Roll, eulerAngles.Pitch, eulerAngles.Yaw);
	flagComment.m_cameraPosition = wxTheFrame->GetWorldEditPanel()->GetCameraPosition();
	flagComment.m_flagPosition = m_flagOpenToEdit->m_flagEntity->GetPosition();

	if(m_flagOpenToEdit->m_flagEntity != nullptr)
	{
		TagList tags = m_flagOpenToEdit->m_flagEntity->GetTags();
		tags.AddTag(CNAME( LockedObject ));
		m_flagOpenToEdit->m_flagEntity->SetTags(tags);
	}

	Bool makeScreen = m_makeScreen->IsChecked();
	if(m_attachedMarkerSystem->ModifyFlag( *m_flagOpenToEdit, flagComment, makeScreen ) == true)
	{
		if( m_editScreenAfterAdding->IsChecked() == true )
		{
			OpenExternalFile( flagComment.m_pathToScreen );
		}

		m_modifyFlagWindow->Hide();
		m_flagOpenToEdit = nullptr;
		wxMessageBox("Modify flag succeed", "Information", wxOK | wxICON_INFORMATION);
	}
	else
	{
		wxMessageBox("Modify flag failed", "Error", wxOK | wxICON_ERROR);
	}

	ClearSelection();

	m_flagList->Enable(true);

	m_attachedMarkerSystem->UnlockUpdate();
}

void CEdReviewPanel::OnModifyCancel( wxCommandEvent& event )
{
	if(wxMessageBox("Do you want close window?", "Closing...", wxYES_NO | wxICON_QUESTION) == wxYES)
	{
		if(m_flagOpenToEdit->m_flagEntity != nullptr)
		{
			// revert last position for flag
			Uint32 lastCommentIndex = m_flagOpenToEdit->m_comments.Size()-1;
			m_flagOpenToEdit->m_flagEntity->SetPosition(m_flagOpenToEdit->m_comments[lastCommentIndex].m_flagPosition);

			TagList tags = m_flagOpenToEdit->m_flagEntity->GetTags();
			tags.AddTag(CNAME( LockedObject ));
			m_flagOpenToEdit->m_flagEntity->SetTags(tags);
		}

		m_modifyFlagWindow->Hide();
		m_flagOpenToEdit = nullptr;
	}

	ClearSelection();

	m_flagList->Enable(true);

	m_attachedMarkerSystem->UnlockUpdate();
}

void CEdReviewPanel::OnKeepStateChanged( wxCommandEvent& event )
{
	Bool value = ( event.GetInt() == 0 ) ? false : true;
	m_modifyFlagState->Enable( !value );
	if( value == true )
	{
		m_modifyFlagState->SetSelection( -1 );
	}
	else
	{
		// Set proper values
		Uint32 commentCount = m_flagOpenToEdit->m_comments.Size();
		const CReviewFlagComment& lastComment = m_flagOpenToEdit->m_comments[commentCount-1];
		Uint32 stateIndex = ( lastComment.m_state == 1 ) ? 0 : lastComment.m_state - 2;	// "-2" is there because user cannot change state to "Open"
		( m_modifyFlagState->GetCount() > 0 ) ? m_modifyFlagState->Select( stateIndex ) : m_modifyFlagState->Select( -1 );
	}
}

void CEdReviewPanel::OnMakeScreenChanged( wxCommandEvent& event )
{
	Bool value = ( event.GetInt() == 0 ) ? false : true;
	m_editScreenAfterAdding->Enable( value );
	m_editScreenAfterAdding->SetValue( false );
}

void CEdReviewPanel::OnKeepPriorityChanged( wxCommandEvent& event )
{
	Bool value = ( event.GetInt() == 0 ) ? false : true;
	m_modifyFlagPriority->Enable( !value );
	if( value == true )
	{
		m_modifyFlagPriority->SetSelection( -1 );
	}
	else
	{
		// Set proper values
		Uint32 commentCount = m_flagOpenToEdit->m_comments.Size();
		const CReviewFlagComment& lastComment = m_flagOpenToEdit->m_comments[commentCount-1];
		( m_modifyFlagPriority->GetCount() > 0 ) ? m_modifyFlagPriority->Select( lastComment.m_priority ) : m_modifyFlagPriority->Select( -1 );
	}
}

void CEdReviewPanel::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if( name == CNAME( NodeSelected ) )
	{
		CEntity* entity = Cast< CEntity >( GetEventData< CNode* >( data ) );
		if( entity != nullptr )
		{
			if( entity->GetTags().HasTag( CNAME(ReviewFlagObject) ) == true )
			{
				if(m_flagOpenToEdit == nullptr && m_modifyFlagWindow->IsVisible() == false)
				{
					TDynArray<CReviewFlag*> flags;
					m_attachedMarkerSystem->GetFlags( flags );
					for(Uint32 i=0; i<flags.Size(); ++i)
					{
						if( flags[i]->m_flagEntity == entity )
						{
							m_flagList->SetItemState( i, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED );
							m_flagOpenToEdit = flags[i];
							m_flagList->EnsureVisible( i );
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
			if( entity->GetTags().HasTag( CNAME(ReviewFlagObject) ) == true )
			{
				if( m_modifyFlagWindow->IsVisible() == false )
				{
					m_flagList->SetItemState( GetSelectedItemIndex(), 0, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED );
					m_flagOpenToEdit = nullptr;
				}
			}
		}
	}
	else if( name == CNAME( EditorTick ) )
	{
		InternalProcessMessage();
	}
}

void CEdReviewPanel::InternalProcessMessage()
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
			GFeedback->BeginTask( TXT("Review marker system - Connecting with database"), false );
			this->Disable();
		}
		break;
	case MSM_DatabaseConnected:
		{
			FillFlagsWindow();

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
			String errorMessage = TXT( "Connection with database has been lost. " );
			if ( m_attachedMarkerSystem )
			{
				errorMessage += m_attachedMarkerSystem->GetDBInitError();
			}
			m_connectionInfo->SetLabel( errorMessage.AsChar() );
			Layout();

			GFeedback->EndTask();
		}
		break;
	case MSM_TestTrackLostConnection:
		{
			this->Disable();
			m_mainPanel->Disable();
			m_mainToolbar->Disable();
			m_connectionPanel->Show();
			m_connectionInfo->SetLabel( TXT("Connection with Test Track has been lost.") );
			Layout();

			GFeedback->EndTask();
		}
		break;
	case MSM_SynchronizationStart:
		{
			GFeedback->BeginTask( TXT("Review marker system - Synchronization with database"), false );
			this->Disable();
		}
		break;
	case MSM_SynchronizationEnd:
		{
			FillFlagsWindow();
			this->Enable();
			GFeedback->EndTask();
		}
		break;
	case MSM_DataAreUpdated:
	case MSM_DataAreSorted:
		{
			this->Disable();
			FillFlagsWindow();
			this->Enable();
		}
		break;
	}
}

void CEdReviewPanel::ProcessMessage( enum EMarkerSystemMessage message, enum EMarkerSystemType systemType, IMarkerSystemInterface* system )
{
	if( systemType == MST_Review )
	{
		switch( message )
		{
		case MSM_SystemRegistered:
			{
				if( systemType == MST_Review )
				{
					m_attachedMarkerSystem = static_cast< CReviewSystem* >( system );
					this->Enable();
				}
			}
			return;
		case MSM_SystemUnregistered:
			{
				if( systemType == MST_Review )
				{
					this->Disable();
					m_attachedMarkerSystem = nullptr;
				}
			}
			return;
		}

		// add messgae to queue
		{
			Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_synchronizationMutex );
			m_messages.Push( message );
		}
	}
}

void CEdReviewPanel::OnSelectRowInGrid( wxListEvent& event )
{
	Int32 selectedItemIndex = GetSelectedItemIndex();

	if( selectedItemIndex != -1 )
	{
		CReviewFlag* selectedFlag = m_attachedMarkerSystem->GetFlag( selectedItemIndex );
		CWorld* world = GGame->GetActiveWorld();
		if ( world != nullptr )
		{
			CSelectionManager* selectionManager = world->GetSelectionManager();
			if( selectionManager != nullptr )
			{
				selectionManager->DeselectAll();
				if( selectedFlag->m_flagEntity != nullptr )
				{
					selectionManager->Select( selectedFlag->m_flagEntity.Get() );
				}
			}
		}

		CReviewFlagComment* selectedComment = &selectedFlag->m_comments[selectedFlag->m_comments.Size()-1];
		wxTheFrame->GetWorldEditPanel()->SetCameraPosition(selectedComment->m_cameraPosition);
		EulerAngles eulerAngles(selectedComment->m_cameraOrientation.X,selectedComment->m_cameraOrientation.Y,selectedComment->m_cameraOrientation.Z);
		wxTheFrame->GetWorldEditPanel()->SetCameraRotation(eulerAngles);
	}
}

void CEdReviewPanel::OnShowColumnContextMenu( wxListEvent& event )
{
	wxWindowBase::PopupMenu( m_columnContextMenu );
}

void CEdReviewPanel::OnSortByColumn( wxListEvent& event )
{
	wxListCtrl* list = wxDynamicCast( event.GetEventObject(), wxListCtrl );
	Uint32 columnIndex = event.GetColumn();
	static Bool ascendingOrder = true;
	ascendingOrder = !ascendingOrder;

	m_attachedMarkerSystem->SetSortingSettings( static_cast< EReviewSortCategory >( columnIndex ), ( ascendingOrder == true ) ? MSSO_Ascending : MSSO_Descending );
}

void CEdReviewPanel::OnColumnPopupClick( wxCommandEvent& event )
{
	wxWindowUpdateLocker localUpdateLocker( this );

	if( m_columnContextMenu->IsChecked( event.GetId() ) == true )
	{
		m_flagList->SetColumnWidth( event.GetId(), 50 );
	}
	else
	{
		m_flagList->SetColumnWidth( event.GetId(), 0 );
	}
}

void CEdReviewPanel::OnRowPopupClick( wxCommandEvent& event )
{
	switch( event.GetId() )
	{
	case 0:		// look at
		OnSelectRowInGrid( wxListEvent() );
		break;
	case 1:		// show
		OnShowFlagWindow( wxCommandEvent() );
		break;
	case 2:		// edit
		OnShowModifyFlagWindow( wxCommandEvent() );
		break;
	}
}

void CEdReviewPanel::ClearSelection()
{
	CWorld* world = GGame->GetActiveWorld();
	if ( world )
	{
		world->GetSelectionManager()->DeselectAll();
	}

	m_flagList->SetItemState( GetSelectedItemIndex(), 0, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED );
}

Int32 CEdReviewPanel::GetSelectedItemIndex()
{
	return m_flagList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
}

void CEdReviewPanel::OnShowRowContextMenu( wxListEvent& event )
{
	wxWindowBase::PopupMenu( m_rowContextMenu );
}

void CEdReviewPanel::OnSearchButtonClicked( wxCommandEvent& event )
{
	String phrase = m_searchLine->GetValue();
	EReviewSearchType searchType = static_cast<EReviewSearchType>( m_searchCategory->GetSelection() );

	m_attachedMarkerSystem->SetSearchFilter( searchType, phrase );
}

void CEdReviewPanel::OnSearchEnterClicked( wxCommandEvent& event )
{
	OnSearchButtonClicked( wxCommandEvent() );
}

void CEdReviewPanel::OnTTPProjectSelecetd( wxCommandEvent& event )
{
	Int32 selection = m_projectName->GetSelection();
	if( selection == -1 )
	{
		return;
	}

	m_attachedMarkerSystem->SetDefaultProjectName( m_projectName->GetString( selection ).wc_str() );

	// update milestones
	TDynArray< String > ttpMilestones;
	m_attachedMarkerSystem->GetMilestoneList( ttpMilestones );
	m_milestoneName->Clear();
	for( Uint32 i=0; i<ttpMilestones.Size(); ++i )
	{
		m_milestoneName->Append( ttpMilestones[i].AsChar() );
	}
	m_milestoneName->Select( m_milestoneName->FindString( m_attachedMarkerSystem->GetDefaultMilestoneName().AsChar() ) );
	m_attachedMarkerSystem->SendRequest( MSRT_LoadData );
}

void CEdReviewPanel::OnTTPMilestoneSelected( wxCommandEvent& event )
{
	Int32 selection = m_milestoneName->GetSelection();
	if( selection == -1 )
	{
		return;
	}

	m_attachedMarkerSystem->SetDefaultMilestoneName( m_milestoneName->GetString( selection ).wc_str() );
}

#endif // NO_MARKER_SYSTEM
