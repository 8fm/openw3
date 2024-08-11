/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
#ifndef NO_MARKER_SYSTEMS

#include "../../common/engine/reviewSystem.h"
#include "freeCamera.h"
#include "redGuiSeparator.h"
#include "redGuiList.h"
#include "redGuiLabel.h"
#include "redGuiPanel.h"
#include "redGuiButton.h"
#include "redGuiCheckBox.h"
#include "redGuiComboBox.h"
#include "redGuiImage.h"
#include "redGuiTextBox.h"
#include "redGuiProgressBox.h"
#include "redGuiSpin.h"
#include "redGuiModalWindow.h"
#include "redGuiOpenFileDialog.h"
#include "redGuiLoginBox.h"
#include "redGuiManager.h"
#include "redGuiRenderManager.h"
#include "redGuiGraphicContext.h"
#include "debugWindowsManager.h"
#include "debugWindowReviewMarkers.h"
#include "debugWindowReviewMarkersAddMarker.h"
#include "debugWindowReviewMarkersEditMarker.h"
#include "debugWindowReviewMarkersShowMarker.h"
#include "debugWindowReviewMarkersFilterMarker.h"
#include "game.h"
#include "world.h"
#include "selectionManager.h"
#include "bitmapTexture.h"
#include "baseEngine.h"
#include "entity.h"

namespace DebugWindows
{
	CDebugWindowReviewMarkers::CDebugWindowReviewMarkers() 
		: RedGui::CRedGuiWindow( 200, 200, 590, 500 )
		, m_imidiateMode( false )
		, m_isLoggedIn( false )
	{
		SetCaption( TXT("Review markers system") );

		// create all controls contain by the review marker debug window
		CreateControls();
		CreateDependentWindows();

		GRedGui::GetInstance().EventViewportInput.Bind( this, &CDebugWindowReviewMarkers::NotifyEventViewportInput );
		GRedGui::GetInstance().EventTick.Bind( this, &CDebugWindowReviewMarkers::NotifyOnTick );
	}

	CDebugWindowReviewMarkers::~CDebugWindowReviewMarkers()
	{
		GRedGui::GetInstance().EventTick.Unbind( this, &CDebugWindowReviewMarkers::NotifyOnTick );
		GRedGui::GetInstance().EventViewportInput.Unbind( this, &CDebugWindowReviewMarkers::NotifyEventViewportInput );

		UnregisterDependentWindows();
	}

	void CDebugWindowReviewMarkers::CreateControls()
	{
		// create depend windows
		{
			m_waitingBox = new RedGui::CRedGuiProgressBox();
			GRedGui::GetInstance().RegisterWindowInActiveDesktop( m_waitingBox );
		}
		{
			m_loginBox = new RedGui::CRedGuiLoginBox( TXT("ReviewSystem") );
			m_loginBox->SetCaption( TXT("Login window to review flags") );
			m_loginBox->EventLoginClicked.Bind( this, &CDebugWindowReviewMarkers::NotifyOnLoginToReviewSystem );
			m_loginBox->EventCancelClicked.Bind( this, &CDebugWindowReviewMarkers::NotifyOnCancelLoginProcess );
			GRedGui::GetInstance().RegisterWindowInActiveDesktop( m_loginBox );
		}

		// create left panel
		{
			m_leftPanel = new RedGui::CRedGuiPanel(5, 5, 300, 700);
			m_leftPanel->SetBorderVisible(false);
			m_leftPanel->SetBackgroundColor(Color::CLEAR);
			AddChild(m_leftPanel);
			m_leftPanel->SetDock(RedGui::DOCK_Fill );

			CreateLeftPanelControls();
		}
	}

	void CDebugWindowReviewMarkers::CreateLeftPanelControls()
	{
		m_mainControlPanel = new RedGui::CRedGuiPanel(0, 0, m_leftPanel->GetWidth(), 20);
		m_mainControlPanel->SetBorderVisible(false);
		m_mainControlPanel->SetBackgroundColor(Color::CLEAR);
		m_mainControlPanel->SetMargin(Box2(0, 5, 0, 5));
		m_leftPanel->AddChild(m_mainControlPanel);
		m_mainControlPanel->SetDock(RedGui::DOCK_Top);
		{
			m_showFlagButton = new RedGui::CRedGuiButton(0,0,20,20);
			m_showFlagButton->SetBorderVisible(false);
			m_showFlagButton->SetBackgroundColor(Color::CLEAR);
			m_showFlagButton->SetMargin(Box2(3, 0, 3, 0));
			m_showFlagButton->SetDock(RedGui::DOCK_Left);
			m_showFlagButton->SetTabIndex( 0 );
			m_showFlagButton->SetImage( RedGui::Resources::GReviewEyeIcon );
			m_showFlagButton->EventButtonClicked.Bind( this, &CDebugWindowReviewMarkers::NotifyEventButtonClickedShowFlag );
			m_showFlagButton->SetSimpleToolTip(TXT("Show information about flag"));
			m_mainControlPanel->AddChild(m_showFlagButton);

			m_addNewFlagButton = new RedGui::CRedGuiButton(0,0,20,20);
			m_addNewFlagButton->SetBorderVisible(false);
			m_addNewFlagButton->SetBackgroundColor(Color::CLEAR);
			m_addNewFlagButton->SetMargin(Box2(3, 0, 3, 0));
			m_addNewFlagButton->SetDock(RedGui::DOCK_Left);
			m_addNewFlagButton->SetTabIndex( 1 );
			m_addNewFlagButton->SetImage( RedGui::Resources::GReviewAddIcon );
			m_addNewFlagButton->EventButtonClicked.Bind( this, &CDebugWindowReviewMarkers::NotifyEventButtonClickedNewFlag );
			m_addNewFlagButton->SetSimpleToolTip(TXT("Add new flag"));
			m_mainControlPanel->AddChild(m_addNewFlagButton);

			m_editExistingFlagButton = new RedGui::CRedGuiButton(0,0,20,20);
			m_editExistingFlagButton->SetBorderVisible(false);
			m_editExistingFlagButton->SetBackgroundColor(Color::CLEAR);
			m_editExistingFlagButton->SetMargin(Box2(3, 0, 3, 0));
			m_editExistingFlagButton->SetDock(RedGui::DOCK_Left);
			m_editExistingFlagButton->SetTabIndex( 2 );
			m_editExistingFlagButton->SetImage( RedGui::Resources::GReviewPropertiesIcon );
			m_editExistingFlagButton->EventButtonClicked.Bind(this, &CDebugWindowReviewMarkers::NotifyEventButtonClickedEditFlag);
			m_editExistingFlagButton->SetSimpleToolTip(TXT("Edit flag"));
			m_mainControlPanel->AddChild(m_editExistingFlagButton);

			m_autoSyncTime = new RedGui::CRedGuiSpin(0,0, 45, 20);
			m_autoSyncTime->SetBorderVisible(false);
			m_autoSyncTime->SetMargin(Box2(3, 0, 3, 0));
			m_autoSyncTime->SetTabIndex( 3 );
			m_autoSyncTime->SetDock(RedGui::DOCK_Left);
			m_autoSyncTime->SetSimpleToolTip(TXT("Time (in minutes) between refresh"));
			m_autoSyncTime->EventValueChanged.Bind( this, &CDebugWindowReviewMarkers::NotifyEventValueChangedSyncTime );
			m_mainControlPanel->AddChild(m_autoSyncTime);

			m_autoSyncCheckBox = new RedGui::CRedGuiCheckBox(0, 0, 20, 20);
			m_autoSyncCheckBox->SetText(TXT("Auto sync"));
			m_autoSyncCheckBox->SetBorderVisible(false);
			m_autoSyncCheckBox->SetMargin(Box2(3, 0, 3, 0));
			m_autoSyncCheckBox->SetTabIndex( 4 );
			m_autoSyncCheckBox->SetDock(RedGui::DOCK_Left);
			m_autoSyncCheckBox->SetSimpleToolTip(TXT("Enable automatic refresh"));
			m_autoSyncCheckBox->EventCheckedChanged.Bind( this, &CDebugWindowReviewMarkers::NotifyEventCheckedChangedAutoSync );
			m_mainControlPanel->AddChild(m_autoSyncCheckBox);

			m_refreshFlagsButton = new RedGui::CRedGuiButton(0,0,20,20);
			m_refreshFlagsButton->SetBorderVisible(false);
			m_refreshFlagsButton->SetBackgroundColor(Color::CLEAR);
			m_refreshFlagsButton->SetMargin(Box2(3, 0, 3, 0));
			m_refreshFlagsButton->SetTabIndex( 5 );
			m_refreshFlagsButton->SetDock(RedGui::DOCK_Left);
			m_refreshFlagsButton->SetImage( RedGui::Resources::GReviewRefreshIcon );
			m_refreshFlagsButton->SetSimpleToolTip(TXT("Refresh flags"));
			m_refreshFlagsButton->EventButtonClicked.Bind( this, &CDebugWindowReviewMarkers::NotifyEventButtonClickedRefreshFlags );
			m_mainControlPanel->AddChild(m_refreshFlagsButton);

			m_openFiltersButton = new RedGui::CRedGuiButton(0,0,20,20);
			m_openFiltersButton->SetBorderVisible(false);
			m_openFiltersButton->SetBackgroundColor(Color::CLEAR);
			m_openFiltersButton->SetMargin(Box2(3, 0, 3, 0));
			m_openFiltersButton->SetTabIndex( 6 );
			m_openFiltersButton->SetDock(RedGui::DOCK_Left);
			m_openFiltersButton->SetImage( RedGui::Resources::GReviewFiltersIcon );
			m_openFiltersButton->SetSimpleToolTip(TXT("Open filters"));
			m_openFiltersButton->EventButtonClicked.Bind(this, &CDebugWindowReviewMarkers::NotifyEventButtonClickedShowFilter);
			m_mainControlPanel->AddChild(m_openFiltersButton);

			RedGui::CRedGuiButton* resetFiltersButton = new RedGui::CRedGuiButton( 0, 0, 20, 20 );
			resetFiltersButton->SetBorderVisible( false );
			resetFiltersButton->SetBackgroundColor( Color::CLEAR );
			resetFiltersButton->SetMargin( Box2(3, 0, 3, 0) );
			resetFiltersButton->SetTabIndex( 6 );
			resetFiltersButton->SetDock( RedGui::DOCK_Left );
			resetFiltersButton->SetImage( RedGui::Resources::GReviewResetFiltersIcon );
			resetFiltersButton->SetSimpleToolTip( TXT("Reset filters") );
			resetFiltersButton->EventButtonClicked.Bind( this, &CDebugWindowReviewMarkers::NotifyEventButtonClickedResetFilter );
			m_mainControlPanel->AddChild( resetFiltersButton );

			RedGui::CRedGuiButton* showFreeCamInfoButton = new RedGui::CRedGuiButton( 0, 0, 20, 20 );
			showFreeCamInfoButton->SetBorderVisible( false );
			showFreeCamInfoButton->SetBackgroundColor( Color::CLEAR );
			showFreeCamInfoButton->SetMargin( Box2( 0, 0, 10, 0 ) );
			showFreeCamInfoButton->SetTabIndex( 6 );
			showFreeCamInfoButton->SetDock( RedGui::DOCK_Right );
			showFreeCamInfoButton->SetImage( RedGui::Resources::GReviewFreeCam );
			showFreeCamInfoButton->SetSimpleToolTip( TXT("Free cam info") );
			showFreeCamInfoButton->EventButtonClicked.Bind( this, &CDebugWindowReviewMarkers::NotifyEventButtonClickedShowFreeCamInfo );
			m_mainControlPanel->AddChild( showFreeCamInfoButton );
		}

		// search panel
		RedGui::CRedGuiPanel* searchPanel = new RedGui::CRedGuiPanel(0, 0, m_leftPanel->GetWidth(), 20);
		searchPanel->SetBorderVisible( false );
		searchPanel->SetBackgroundColor( Color::CLEAR );
		searchPanel->SetMargin( Box2( 0, 5, 0, 5 ) );
		searchPanel->SetDock( RedGui::DOCK_Top );
		m_leftPanel->AddChild( searchPanel );
		{
			m_searchButton = new RedGui::CRedGuiButton( 0, 0, 60, 30 );
			m_searchButton->SetMargin( Box2( 1, 1, 1, 1) );
			m_searchButton->SetBorderVisible(false);
			m_searchButton->SetText( TXT("Search") );
			m_searchButton->SetDock( RedGui::DOCK_Right );
			m_searchButton->SetTabIndex( 9 );
			m_searchButton->EventButtonClicked.Bind( this, &CDebugWindowReviewMarkers::NotifySearchClicked );
			searchPanel->AddChild( m_searchButton );

 			m_searchCategory = new RedGui::CRedGuiComboBox( 0, 0, 90, 10 );
 			m_searchCategory->SetMargin( Box2( 0, 2, 5, 0 ) );
 			m_searchCategory->SetDock( RedGui::DOCK_Left );
 			m_searchCategory->AddItem( TXT("None") );
 			m_searchCategory->AddItem( TXT("Summary") );
 			m_searchCategory->AddItem( TXT("TTP No.") );
			m_searchCategory->SetTabIndex( 7 );
 			m_searchCategory->SetSelectedIndex( 0 );
 			searchPanel->AddChild( m_searchCategory );

			m_searchArea = new RedGui::CRedGuiTextBox( 0, 0, 100, 20 );
			m_searchArea->SetMargin( Box2( 1, 1, 1, 1) );
			m_searchArea->SetDock( RedGui::DOCK_Fill );
			m_searchArea->SetTabIndex( 8 );
			m_searchArea->EventTextEnter.Bind( this, &CDebugWindowReviewMarkers::NotifySearchTextEnter );
			searchPanel->AddChild( m_searchArea );
		}

		// create list for all review mark flags
		m_reviewFlagList = new RedGui::CRedGuiList( 0, 0, m_leftPanel->GetWidth(), m_leftPanel->GetHeight() );
		m_reviewFlagList->SetDock(RedGui::DOCK_Fill);
		m_reviewFlagList->AppendColumn( TXT("Name"), 200 );
		m_reviewFlagList->AppendColumn( TXT("Priority"), 50, RedGui::SA_Integer );
		m_reviewFlagList->AppendColumn( TXT("State"), 70 );
		m_reviewFlagList->AppendColumn( TXT("Type"), 130 );
		m_reviewFlagList->AppendColumn( TXT("Date"), 130);
		m_reviewFlagList->SetTextAlign( RedGui::IA_MiddleLeft );
		m_reviewFlagList->SetTabIndex( 10 );
		m_reviewFlagList->EventColumnClicked.Bind( this, &CDebugWindowReviewMarkers::NotifyEventColumnClicked );
		m_reviewFlagList->EventDoubleClickItem.Bind( this, &CDebugWindowReviewMarkers::NotifyEventDoubleClickItemFlag );
		m_leftPanel->AddChild(m_reviewFlagList);
	}

	void CDebugWindowReviewMarkers::FillFlagsWindow()
	{
		m_reviewFlagList->RemoveAllItems();

		TDynArray< CReviewFlag* > flags;
		m_attachedMarkerSystem->GetFlags( flags );
		const Uint32 flagCount = flags.Size();
		for( Uint32 i=0; i<flagCount; ++i )
		{
			CReviewFlag* flag = flags[i];
			Uint32 itemIndex = m_reviewFlagList->AddItem( flags[i]->m_summary.AsChar() );

			// fill row
			const Uint32 lastCommentIndex = flags[i]->m_comments.Size() - 1;
			m_reviewFlagList->SetItemText( ToString( flags[i]->m_comments[lastCommentIndex].m_priority ).AsChar(), itemIndex, RMC_Priority );

			TDynArray< String > states;
			m_attachedMarkerSystem->GetStateList( states );
			if( flags[i]->m_comments[lastCommentIndex].m_state-1 < states.Size() )
			{
				m_reviewFlagList->SetItemText( states[flags[i]->m_comments[lastCommentIndex].m_state-1].AsChar() , itemIndex, RMC_State );
			}

			TDynArray< String > types;
			m_attachedMarkerSystem->GetBugTypeList( types );
			if( flags[i]->m_type < types.Size() )
			{
				m_reviewFlagList->SetItemText( types[flags[i]->m_type].AsChar() , itemIndex, RMC_Type );
			}

			tm* localTime = &flags[i]->m_comments[lastCommentIndex].m_creationDate;
			String dateTime = ToString(localTime->tm_year)+TXT("-")+ToString(localTime->tm_mon)+TXT("-")+ToString(localTime->tm_mday)+TXT("_");
			dateTime += ToString(localTime->tm_hour)+TXT(":")+ToString(localTime->tm_min)+TXT(":")+ToString(localTime->tm_sec);
			dateTime.ReplaceAll(TXT("_"), TXT(" "), true);
			m_reviewFlagList->SetItemText( dateTime.AsChar() , itemIndex, RMC_Date );
		}
	}

	void CDebugWindowReviewMarkers::OnWindowClosed( CRedGuiControl* control )
	{
		if( m_imidiateMode == false )
		{
			GEngine->GetMarkerSystems()->TurnOffSystems();
			GGame->Unpause( TXT("Review marker system") );
			GGame->EnableFreeCamera( false );
		}
		else
		{
			m_waitingBox->Hide();
			// unregister listener for messages from review marker tool
			GEngine->GetMarkerSystems()->UnregisterListener( MST_Review, this );
		}
	}

	void CDebugWindowReviewMarkers::OnWindowOpened( CRedGuiControl* control )
	{
		// connect with database
		if( m_attachedMarkerSystem != nullptr )
		{
			if( m_attachedMarkerSystem->IsConnected() == false )
			{
				m_attachedMarkerSystem->Connect();
			}
		}

		if( m_imidiateMode == false )
		{
			ClearControls();

			GGame->Pause( TXT("Review marker system") );
			GGame->EnableFreeCamera( true );
			GEngine->GetMarkerSystems()->TurnOnSystems();
		}
		else
		{
			m_imidiateMode = false;
			GEngine->GetMarkerSystems()->RegisterListener( MST_Review, this );
		}

		SearchFlags();

		m_attachedMarkerSystem->SendRequest( MSRT_SynchronizeData );
		m_waitingBox->Show( TXT("Review marker system - Please wait for the synchronization with the database"), true );
	}

	void CDebugWindowReviewMarkers::NotifyEventDoubleClickItemFlag( RedGui::CRedGuiEventPackage&, Int32 selectedIndex )
	{
		if( selectedIndex > -1 )
		{
			CReviewFlag* selectedFlag = m_attachedMarkerSystem->GetFlag( selectedIndex );
			CWorld* world = GGame->GetActiveWorld();
			if ( world )
			{
				if(selectedFlag->m_flagEntity != nullptr)
				{
					world->GetSelectionManager()->DeselectAll();
					world->GetSelectionManager()->Select( selectedFlag->m_flagEntity.Get() );
				}
			}

			CReviewFlagComment* selectedComment = &selectedFlag->m_comments[selectedFlag->m_comments.Size()-1];
			EulerAngles eulerAngles(selectedComment->m_cameraOrientation.X,selectedComment->m_cameraOrientation.Y,selectedComment->m_cameraOrientation.Z);
			GGame->EnableFreeCamera(true);
			CGameFreeCamera& freeCamera = const_cast<CGameFreeCamera&>(GGame->GetFreeCamera());
			freeCamera.MoveTo(selectedComment->m_cameraPosition, eulerAngles);

			ShowFlagWindow( selectedIndex );
		}
	}

	void CDebugWindowReviewMarkers::NotifyEventColumnClicked( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedIndex )
	{
		static Bool ascendingOrder = true;
		ascendingOrder = !ascendingOrder;

		m_attachedMarkerSystem->SetSortingSettings( static_cast< EReviewSortCategory >( selectedIndex ), ( ascendingOrder == true ) ? MSSO_Ascending : MSSO_Descending );
	}

	void CDebugWindowReviewMarkers::NotifyEventViewportInput( RedGui::CRedGuiEventPackage&, IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
		if( GetVisible() == false && m_imidiateMode == false )
		{
			return;
		}

		if( GRedGui::GetInstance().GetInputManager()->IsControlPressed() == true )
		{
			if( m_freeCameraMode == false )
			{
				m_globalAlphaValue = GRedGui::GetInstance().GetRenderManager()->GetGraphicContext()->GetGlobalAlpha();
				GRedGui::GetInstance().GetRenderManager()->GetGraphicContext()->SetGlobalAlpha( 0 );
				m_freeCameraMode = true;

				GRedGui::GetInstance().GetInputManager()->ChangeMouseMode( MM_ClipAndCapture );
				GRedGui::GetInstance().GetInputManager()->HideCursor();
			}

			CGameFreeCamera& freeCamera = const_cast< CGameFreeCamera& >( GGame->GetFreeCamera() );
			freeCamera.ProcessInput( key, action, data );
		}
		else
		{
			if( m_freeCameraMode == true )
			{
				GRedGui::GetInstance().GetRenderManager()->GetGraphicContext()->SetGlobalAlpha( m_globalAlphaValue );
				m_freeCameraMode = false;

				GRedGui::GetInstance().GetInputManager()->ChangeMouseMode( MM_Normal );
				GRedGui::GetInstance().GetInputManager()->ShowCursor();
			}
		}
	}

	void CDebugWindowReviewMarkers::NotifyEventButtonClickedRefreshFlags( RedGui::CRedGuiEventPackage& eventPackage )
	{
		m_attachedMarkerSystem->SendRequest( MSRT_SynchronizeData );
	}

	void CDebugWindowReviewMarkers::NotifyEventCheckedChangedAutoSync( RedGui::CRedGuiEventPackage&, Bool value )
	{
		m_attachedMarkerSystem->SetAutoSync(value);
	}

	void CDebugWindowReviewMarkers::NotifyEventValueChangedSyncTime( RedGui::CRedGuiEventPackage&, Int32 value )
	{
		m_attachedMarkerSystem->SetSyncTime( value );
	}

	void CDebugWindowReviewMarkers::ProcessMessage( enum EMarkerSystemMessage message, enum EMarkerSystemType systemType, IMarkerSystemInterface* system )
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
					}
				}
				return;
			case MSM_SystemUnregistered:
				{
					if( systemType == MST_Review )
					{
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
	}

	void CDebugWindowReviewMarkers::InternalProcessMessage()
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

		GDebugWin::GetInstance().LockHiding();
		switch( message )
		{
		case MSM_DatabaseConnectionStart:
			{
				m_waitingBox->Show( TXT("Review marker system - Connecting with database"), true );
				this->SetEnabled( false );
			}
			break;
		case MSM_DatabaseConnected:
			{
				FillFlagsWindow();

				m_waitingBox->Hide();
				this->SetEnabled( true );
			}
			break;
		case MSM_DatabaseLostConnection:
			{
				String errorMessage = TXT("Review system lost connection with database. Please restart application.\n");
				if ( m_attachedMarkerSystem )
				{
					errorMessage += m_attachedMarkerSystem->GetDBInitError();
				}
				GRedGui::GetInstance().MessageBox( errorMessage, RedGui::MESSAGEBOX_Error );
				m_waitingBox->Hide();
			}
			break;
		case MSM_SynchronizationStart:
			{
				m_waitingBox->Show( TXT("Review marker system - Synchronization with database"), true );
				this->SetEnabled( false );
			}
			break;
		case MSM_SynchronizationEnd:
			{
				FillFlagsWindow();

				m_waitingBox->Hide();
				this->SetEnabled( true );
			}
			break;
		case MSM_DataAreUpdated:
		case MSM_DataAreSorted:
			{
				this->SetEnabled( false );

				FillFlagsWindow();

				m_waitingBox->Hide();
				this->SetEnabled( true );
			}
			break;
		}
		GDebugWin::GetInstance().UnlockHiding();
	}

	void CDebugWindowReviewMarkers::NotifySearchClicked( RedGui::CRedGuiEventPackage& eventPackage )
	{
		SearchFlags();
	}

	void CDebugWindowReviewMarkers::NotifySearchTextEnter( RedGui::CRedGuiEventPackage& eventPackage )
	{
		SearchFlags();
	}

	void CDebugWindowReviewMarkers::NotifyEventButtonClickedShowFlag( RedGui::CRedGuiEventPackage& eventPackage )
	{
		Int32 selectedFlagIndex = m_reviewFlagList->GetSelection();

		if( selectedFlagIndex != -1 )
		{
			ShowFlagWindow(selectedFlagIndex);

		}
		else
		{
			GRedGui::GetInstance().MessageBox( TXT("At first, you must choose flag from the list") );
		}
	}

	void CDebugWindowReviewMarkers::NotifyEventButtonClickedNewFlag( RedGui::CRedGuiEventPackage& eventPackage )
	{
		if ( GGame->GetActiveWorld() )
		{
			if ( m_addMarkerWindow->SetFlagToShow( m_attachedMarkerSystem, m_attachedMarkerSystem->CreateNewFlag() ) )
			{
				m_imidiateMode = true;
				this->SetVisible( false );
				m_addMarkerWindow->SetParentWindow( this );
				m_addMarkerWindow->SetVisible( true );
			}
			else
			{
				GRedGui::GetInstance().MessageBox( TXT("Couldn't create new flag! Please, try again.") );
			}
		}
	}

	void CDebugWindowReviewMarkers::NotifyEventButtonClickedEditFlag( RedGui::CRedGuiEventPackage& eventPackage )
	{
		Int32 selectedFlagIndex = m_reviewFlagList->GetSelection();

		if( selectedFlagIndex != -1 )
		{
			m_imidiateMode = true;
			this->SetVisible( false );
			m_editMarkerWindow->SetParentWindow( this );
			m_editMarkerWindow->SetFlagToShow( m_attachedMarkerSystem, m_attachedMarkerSystem->GetFlag( selectedFlagIndex ) );
			m_editMarkerWindow->SetVisible( true );
		}
		else
		{
			GRedGui::GetInstance().MessageBox( TXT("At first, you must choose flag from the list") );
		}
	}

	void CDebugWindowReviewMarkers::NotifyEventButtonClickedShowFilter( RedGui::CRedGuiEventPackage& eventPackage )
	{
		m_imidiateMode = true;
		this->SetVisible( false );
		m_filterMarkerWindow->SetParentWindow( this );
		m_filterMarkerWindow->SetMarkerSystem( m_attachedMarkerSystem );
		m_filterMarkerWindow->SetVisible( true );
	}

	void CDebugWindowReviewMarkers::NotifyEventButtonClickedShowFreeCamInfo( RedGui::CRedGuiEventPackage& eventPackage )
	{
		GRedGui::GetInstance().MessageBox( TXT("If you want use free camera, should hold 'Control' key and use WSAD keys and mouse to control the camera"), RedGui::MESSAGEBOX_Info );
	}

	void CDebugWindowReviewMarkers::NotifyEventButtonClickedResetFilter( RedGui::CRedGuiEventPackage& eventPackage )
	{
		TDynArray< Bool > states;
		m_attachedMarkerSystem->GetStateValues ( states );
		const Uint32 stateCount = states.Size();
		for( Uint32 i=0; i<stateCount; ++i )
		{
			states[i] = true;
		}
		TDynArray< Bool > types;
		m_attachedMarkerSystem->GetTypesValues( types );
		const Uint32 typeCount = types.Size();
		for( Uint32 i=0; i<typeCount; ++i )
		{
			types[i] = true;
		}
		m_attachedMarkerSystem->SetFilters( states, types );

		m_searchCategory->SetSelectedIndex( 0 );
		m_searchArea->SetText( TXT("") );
		m_attachedMarkerSystem->SetSearchFilter( ReviewSearchNone, TXT("") );
	}

	void CDebugWindowReviewMarkers::CreateDependentWindows()
	{
		m_addMarkerWindow = new CDebugWindowReviewMarkersAddMarker();
		GRedGui::GetInstance().RegisterWindowInActiveDesktop( m_addMarkerWindow );

		m_showMarkerWindow = new CDebugWindowReviewMarkersShowMarker();
		GRedGui::GetInstance().RegisterWindowInActiveDesktop( m_showMarkerWindow );

		m_editMarkerWindow = new CDebugWindowReviewMarkersEditMarker();
		GRedGui::GetInstance().RegisterWindowInActiveDesktop( m_editMarkerWindow );

		m_filterMarkerWindow = new CDebugWindowReviewMarkersFilterMarker();
		GRedGui::GetInstance().RegisterWindowInActiveDesktop( m_filterMarkerWindow );
	}

	void CDebugWindowReviewMarkers::UnregisterDependentWindows()
	{
		GRedGui::GetInstance().UnregisterWindowFromActiveDesktop( m_addMarkerWindow );
		GRedGui::GetInstance().UnregisterWindowFromActiveDesktop( m_showMarkerWindow );
		GRedGui::GetInstance().UnregisterWindowFromActiveDesktop( m_editMarkerWindow );
		GRedGui::GetInstance().UnregisterWindowFromActiveDesktop( m_filterMarkerWindow );
	}

	void CDebugWindowReviewMarkers::EditFlagDirectly( CReviewFlag* flag )
	{
		m_imidiateMode = true;
		this->SetVisible( false );
		m_editMarkerWindow->SetParentWindow( this );
		m_editMarkerWindow->SetFlagToShow( m_attachedMarkerSystem, flag );
		m_editMarkerWindow->SetVisible( true );
	}

	void CDebugWindowReviewMarkers::ClearControls()
	{
		if( m_addMarkerWindow->GetVisible() == true || m_showMarkerWindow->GetVisible() == true ||
			m_editMarkerWindow->GetVisible() == true || m_filterMarkerWindow->GetVisible() == true )
		{
			SetVisible( false );
		}

		m_autoSyncTime->SetValue( 0, true );
		m_autoSyncCheckBox->SetChecked( false, true );
		m_reviewFlagList->RemoveAllItems();

		m_searchArea->SetText( TXT("") );
		m_searchCategory->SetSelectedIndex( ReviewSearchBySummary );

		m_waitingBox->Hide();

		m_imidiateMode = false;
	}

	void CDebugWindowReviewMarkers::SearchFlags()
	{
		String phrase = m_searchArea->GetText();
		EReviewSearchType searchType = static_cast<EReviewSearchType>( m_searchCategory->GetSelectedIndex() );
		m_attachedMarkerSystem->SetSearchFilter( searchType, phrase );
	}

	void CDebugWindowReviewMarkers::SetVisible( Bool value )
	{
		if( value == true && m_imidiateMode == false && m_isLoggedIn == false)
		{
			// check if a window is opened in the game
			if( GIsGame == false && GIsEditorGame == false )
			{
				GRedGui::GetInstance().MessageBox( TXT("Review Debug Page in editor is inactive. Please, use Review system tool in Tools panel."), TXT("Warning"), RedGui::MESSAGEBOX_Warning );
				SetVisible( false );
				return;
			}

			GEngine->GetMarkerSystems()->RegisterListener( MST_Review, this );
			m_loginBox->SetVisible( true );
			m_isLoggedIn = false;
			return;
		}
		else
		{
			m_waitingBox->Hide();
		}

		CRedGuiWindow::SetVisible( value );
	}

	void CDebugWindowReviewMarkers::NotifyOnLoginToReviewSystem( RedGui::CRedGuiEventPackage& eventPackage )
	{
		const String user = m_loginBox->GetLogin();
		const String password = m_loginBox->GetPassword();

		m_isLoggedIn = m_attachedMarkerSystem->TryToLoadToTTP( user, password );
		if( m_isLoggedIn == true )
		{
			CRedGuiWindow::SetVisible( true );
			GRedGui::GetInstance().MessageBox( TXT("Login success!"), TXT("Review system"), RedGui::MESSAGEBOX_Info );
		}
		else
		{
			GRedGui::GetInstance().MessageBox( TXT("Login failed!"), TXT("Review system"), RedGui::MESSAGEBOX_Error );
			m_loginBox->SetVisible( true );
		}
	}

	void CDebugWindowReviewMarkers::NotifyOnCancelLoginProcess( RedGui::CRedGuiEventPackage& package )
	{
		GEngine->GetMarkerSystems()->UnregisterListener( MST_Review, this );
	}

	void CDebugWindowReviewMarkers::ShowFlagWindow( Int32 selectedFlagIndex )
	{
		m_imidiateMode = true;
		this->SetVisible( false );
		m_showMarkerWindow->SetParentWindow( this );
		m_showMarkerWindow->SetFlagToShow( m_attachedMarkerSystem, m_attachedMarkerSystem->GetFlag( selectedFlagIndex ) );
		m_showMarkerWindow->SetVisible( true );
	}

	void CDebugWindowReviewMarkers::NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime )
	{
		InternalProcessMessage();
	}
}	// namespace DebugWindows

#endif	// NO_MARKER_SYSTEMS
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
