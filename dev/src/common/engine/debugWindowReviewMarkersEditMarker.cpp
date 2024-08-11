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
#include "redGuiCheckBox.h"
#include "redGuiProgressBox.h"
#include "redGuiSpin.h"
#include "redGuiOpenFileDialog.h"
#include "redGuiManager.h"
#include "debugWindowsManager.h"
#include "debugWindowReviewMarkersEditMarker.h"
#include "viewport.h"
#include "game.h"
#include "world.h"
#include "bitmapTexture.h"
#include "entity.h"

namespace DebugWindows
{
	CDebugWindowReviewMarkersEditMarker::CDebugWindowReviewMarkersEditMarker() 
		: RedGui::CRedGuiWindow( 200, 200, 400, 400 )
	{
		SetCaption( TXT("Review markers system - Edit existing marker") );

		// create all controls contain by the review marker debug window
		CreateControls();
	}

	CDebugWindowReviewMarkersEditMarker::~CDebugWindowReviewMarkersEditMarker()
	{
		/* intentionally empty */
	}

	void CDebugWindowReviewMarkersEditMarker::CreateControls()
	{
		m_acceptModification = new RedGui::CRedGuiButton( 0, 0, 100, 20 );
		m_acceptModification->SetMargin( Box2( 5, 5, 5, 5 ) );
		m_acceptModification->SetText( TXT("Accept modification") );
		m_acceptModification->SetDock( RedGui::DOCK_Bottom );
		m_acceptModification->SetTabIndex( 7 );
		m_acceptModification->EventButtonClicked.Bind( this, &CDebugWindowReviewMarkersEditMarker::NotifyOnAcceptModificationClicked );
		AddChild( m_acceptModification );

		// informations
		RedGui::CRedGuiPanel* infoPanel = new RedGui::CRedGuiPanel( 0, 0, GetWidth(), GetHeight() );
		infoPanel->SetBorderVisible( false );
		infoPanel->SetMargin( Box2(5, 5, 5, 5) );
		infoPanel->SetDock(RedGui::DOCK_Fill);
		AddChild(infoPanel);
		{
			// row for state
			RedGui::CRedGuiPanel* statePanel = new RedGui::CRedGuiPanel( 0,0, 100, 20 );
			statePanel->SetBackgroundColor( Color::CLEAR );
			statePanel->SetBorderVisible( false );
			statePanel->SetMargin( Box2( 10, 5, 10, 5 ) );
			infoPanel->AddChild( statePanel );
			statePanel->SetDock( RedGui::DOCK_Top );
			{
				RedGui::CRedGuiLabel* stateLabel = new RedGui::CRedGuiLabel( 0, 0, 100, 10 );
				stateLabel->SetText( TXT("State: ") );
				stateLabel->SetMargin( Box2( 0, 2, 10, 0 ) );
				statePanel->AddChild( stateLabel) ;
				stateLabel->SetDock( RedGui::DOCK_Left );

				m_keepState = new RedGui::CRedGuiCheckBox( 0, 0, 25, 25 );
				m_keepState->SetChecked( true );
				m_keepState->SetMargin( Box2( 5, 0, 0, 0 ) );
				m_keepState->SetText( TXT("Keep state") );
				m_keepState->SetDock( RedGui::DOCK_Right );
				m_keepState->SetTabIndex( 1 );
				m_keepState->EventCheckedChanged.Bind( this, &CDebugWindowReviewMarkersEditMarker::NotifyOnKeepStateChanged );
				statePanel->AddChild( m_keepState );

				m_states = new RedGui::CRedGuiComboBox( 0, 0, 150, 20 );
				m_states->SetDock( RedGui::DOCK_Fill );
				m_states->SetTabIndex( 0 );
				statePanel->AddChild( m_states );
			}

			// row for priority
			RedGui::CRedGuiPanel* priorityPanel = new RedGui::CRedGuiPanel( 0,0, 100, 20 );
			priorityPanel->SetBackgroundColor( Color::CLEAR );
			priorityPanel->SetBorderVisible( false );
			priorityPanel->SetMargin( Box2( 10, 5, 10, 5 ) );
			priorityPanel->SetDock( RedGui::DOCK_Top );
			infoPanel->AddChild( priorityPanel );
			{
				RedGui::CRedGuiLabel* priorityLabel = new RedGui::CRedGuiLabel( 0, 0, 100, 10 );
				priorityLabel->SetText( TXT("Priority: ") );
				priorityLabel->SetMargin( Box2( 0, 2, 20, 0 ) );
				priorityPanel->AddChild( priorityLabel );
				priorityLabel->SetDock( RedGui::DOCK_Left );

				m_keepPriority = new RedGui::CRedGuiCheckBox( 0, 0, 25, 25 );
				m_keepPriority->SetChecked( true );
				m_keepPriority->SetMargin( Box2( 5, 0, 0, 0 ) );
				m_keepPriority->SetText( TXT("Keep priority") );
				m_keepPriority->SetDock( RedGui::DOCK_Right );
				m_keepPriority->SetTabIndex( 3 );
				m_keepPriority->EventCheckedChanged.Bind( this, &CDebugWindowReviewMarkersEditMarker::NotifyOnKeepPriorityChanged );
				priorityPanel->AddChild( m_keepPriority );

				m_priority = new RedGui::CRedGuiComboBox( 0, 0, 150, 20 );
				m_priority->SetDock(RedGui::DOCK_Fill );
				m_priority->SetTabIndex( 2 );
				priorityPanel->AddChild( m_priority );
			}

			// row for screens
			RedGui::CRedGuiPanel* screenPanel = new RedGui::CRedGuiPanel( 0,0, 100, 20 );
			screenPanel->SetBackgroundColor( Color::CLEAR );
			screenPanel->SetBorderVisible( false );
			screenPanel->SetMargin( Box2( 10, 5, 10, 5 ) );
			screenPanel->SetDock( RedGui::DOCK_Top );
			infoPanel->AddChild( screenPanel );
			{
				RedGui::CRedGuiLabel* priorityLabel = new RedGui::CRedGuiLabel( 0, 0, 100, 10 );
				priorityLabel->SetText( TXT("Screen: ") );
				priorityLabel->SetMargin( Box2( 0, 2, 20, 0 ) );
				priorityLabel->SetDock( RedGui::DOCK_Left );
				screenPanel->AddChild( priorityLabel );

				m_makeScreen = new RedGui::CRedGuiCheckBox( 0, 0, 25, 25 );
				m_makeScreen->SetChecked( false );
				m_makeScreen->SetMargin( Box2( 5, 0, 0, 0 ) );
				m_makeScreen->SetText( TXT("Make screen automatically") );
				m_makeScreen->SetTabIndex( 4 );
				m_makeScreen->SetDock( RedGui::DOCK_Left );
				screenPanel->AddChild( m_makeScreen );
			}

			// row for position button
			RedGui::CRedGuiPanel* positionButtonPanel = new RedGui::CRedGuiPanel( 0,0, 100, 20 );
			positionButtonPanel->SetBackgroundColor( Color::CLEAR );
			positionButtonPanel->SetBorderVisible( false );
			positionButtonPanel->SetMargin( Box2( 10, 5, 10, 5 ) );
			positionButtonPanel->SetDock( RedGui::DOCK_Top );
			infoPanel->AddChild( positionButtonPanel );
			{
				RedGui::CRedGuiLabel* positionLabel = new RedGui::CRedGuiLabel( 0, 0, 100, 10 );
				positionLabel->SetText( TXT("Set on map: ") );
				positionLabel->SetMargin( Box2( 0, 2, 10, 0 ) );
				positionLabel->SetDock( RedGui::DOCK_Left );
				positionButtonPanel->AddChild( positionLabel) ;

				m_positionButton = new RedGui::CRedGuiButton( 0, 0, 20, 20 );
				m_positionButton->SetDock( RedGui::DOCK_Left );
				m_positionButton->SetImage( RedGui::Resources::GReviewMoveIcon );
				m_positionButton->SetImageMode( true );
				m_positionButton->SetMargin( Box2( 10, 0, 0, 0 ) );
				m_positionButton->SetTabIndex( 5 );
				positionButtonPanel->AddChild( m_positionButton );
				m_positionButton->EventButtonClicked.Bind( this, &CDebugWindowReviewMarkersEditMarker::NotifyOnPositionButtonClicked );
			}

			// row for comment label
			{
				RedGui::CRedGuiPanel* commentLabelPanel = new RedGui::CRedGuiPanel(0,0, 100, 20);
				commentLabelPanel->SetBackgroundColor(Color::CLEAR);
				commentLabelPanel->SetBorderVisible(false);
				commentLabelPanel->SetMargin(Box2(10, 5, 10, 5));
				infoPanel->AddChild(commentLabelPanel);
				commentLabelPanel->SetDock(RedGui::DOCK_Top);
				{
					RedGui::CRedGuiLabel* commentLabel = new RedGui::CRedGuiLabel(0, 0, 100, 10);
					commentLabel->SetText(TXT("Comment: "));
					commentLabel->SetMargin(Box2(0, 2, 5, 0));
					commentLabelPanel->AddChild(commentLabel);
					commentLabel->SetDock(RedGui::DOCK_Left);
				}

				m_comments = new RedGui::CRedGuiTextBox( 0, 0, 100, 100 );
				m_comments->SetMargin( Box2( 10, 0, 10, 10 ) );
				m_comments->SetDock( RedGui::DOCK_Fill );
				m_comments->SetMultiLine( true );
				m_comments->SetTabIndex( 6 );
				infoPanel->AddChild( m_comments );
			}
		}

		// create waiting window
		m_waitingBox = new RedGui::CRedGuiProgressBox();
		GRedGui::GetInstance().RegisterWindowInActiveDesktop( m_waitingBox );
	}

	void CDebugWindowReviewMarkersEditMarker::NotifyOnAcceptModificationClicked( RedGui::CRedGuiEventPackage& package )
	{
		GDebugWin::GetInstance().LockHiding();

		if( m_comments->GetText().Empty() == true)
		{
			GRedGui::GetInstance().MessageBox(TXT("Comment must have a text!"), RedGui::MESSAGEBOX_Warning );
			return;
		}

		m_waitingBox->Show( TXT("Adding new comment to flag"), true );

		CReviewFlagComment flagComment;
		flagComment.m_flagId = m_flag->m_databaseId;

		Char lpszUsername[255];
		DWORD dUsername = sizeof( lpszUsername );
		flagComment.m_author = TXT("Unknown user");
		if( GetUserName( lpszUsername, &dUsername ) == (BOOL)true )
		{
			flagComment.m_author = lpszUsername;
		}

		flagComment.m_description = m_comments->GetText();

		if( m_keepState->GetChecked() == true )
		{
			flagComment.m_state = m_flag->m_comments[ m_flag->m_comments.Size() - 1 ].m_state;
		}
		else
		{
			flagComment.m_state = m_states->GetSelectedIndex() + 2;	// skip Opened state
		}

		if( m_keepPriority->GetChecked() == true )
		{
			flagComment.m_priority = m_flag->m_comments[ m_flag->m_comments.Size() - 1 ].m_priority;
		}
		else
		{
			flagComment.m_priority = m_priority->GetSelectedIndex();
		}

		const EulerAngles& eulerAngles = GGame->GetFreeCamera().GetRotation();
		flagComment.m_cameraOrientation = Vector( eulerAngles.Roll, eulerAngles.Pitch, eulerAngles.Yaw );
		flagComment.m_cameraPosition = GGame->GetFreeCamera().GetPosition();
		flagComment.m_flagPosition = m_flag->m_flagEntity->GetPosition();

		Bool makeScreen = m_makeScreen->GetChecked();
		Bool result = m_attachedMarkerSystem->ModifyFlag( *m_flag, flagComment, makeScreen );

		m_waitingBox->Hide();
		SetVisible( false );

		if( result == true )
		{
			GRedGui::GetInstance().MessageBox( TXT("Modify flag succeed"), TXT("Information"), RedGui::MESSAGEBOX_Info );
		}
		else
		{
			GRedGui::GetInstance().MessageBox( TXT("Modify flag failed"), TXT("Error"), RedGui::MESSAGEBOX_Error) ;
		}

		GDebugWin::GetInstance().UnlockHiding();
	}

	void CDebugWindowReviewMarkersEditMarker::SetParentWindow( RedGui::CRedGuiWindow* parentWindow )
	{
		m_parentWindow = parentWindow;
	}

	void CDebugWindowReviewMarkersEditMarker::SetFlagToShow( CReviewSystem* reviewSystem, CReviewFlag* flag )
	{
		m_attachedMarkerSystem = reviewSystem;
		m_flag = flag;
	}

	void CDebugWindowReviewMarkersEditMarker::OnWindowClosed( CRedGuiControl* control )
	{
		if( m_parentWindow != nullptr )
		{
			m_parentWindow->SetVisible( true );
			m_parentWindow = nullptr;
		}

		m_waitingBox->Hide();

		m_attachedMarkerSystem->UnlockUpdate();
	}

	void CDebugWindowReviewMarkersEditMarker::OnWindowOpened( CRedGuiControl* control )
	{
		m_attachedMarkerSystem->LockUpdate();

		ClearControls();

		// Set proper values
		Uint32 commentCount = m_flag->m_comments.Size();
		const CReviewFlagComment& lastComment = m_flag->m_comments[commentCount-1];

		// set default values
		m_keepPriority->SetChecked( true );
		m_keepState->SetChecked( true );
		m_makeScreen->SetChecked( false );
	}

	void CDebugWindowReviewMarkersEditMarker::NotifyOnPositionButtonClicked( RedGui::CRedGuiEventPackage& package )
	{
		m_positionButton->SetEnabled( false );
		Minimize();
		m_changePositionMode = false;

		GRedGui::GetInstance().EventViewportClick.Bind( this, &CDebugWindowReviewMarkersEditMarker::NotifyOnChangePosition );
	}

	void CDebugWindowReviewMarkersEditMarker::NotifyOnChangePosition( RedGui::CRedGuiEventPackage& eventPackage, IViewport* view, Int32 button, Bool state, Vector2 mousePosition )
	{
		if( m_changePositionMode == true )
		{
			GRedGui::GetInstance().EventViewportClick.Unbind( this, &CDebugWindowReviewMarkersEditMarker::NotifyOnChangePosition );

			SetFlagPosition( view, mousePosition );

			Minimize();
			m_positionButton->SetEnabled( true );
		}

		m_changePositionMode = true;
	}

	void CDebugWindowReviewMarkersEditMarker::SetFlagPosition( IViewport* view, const Vector& mouseClickPosition )
	{
		Vector clickedWorldPos;
		Vector correctMouseClickPosition = mouseClickPosition;

		correctMouseClickPosition.X = mouseClickPosition.X - view->GetX();
		correctMouseClickPosition.Y = mouseClickPosition.Y - view->GetY();
		if( correctMouseClickPosition.X < 0.0f )
		{
			correctMouseClickPosition.X = 0.0f;
		}
		if( correctMouseClickPosition.Y < 0.0f )
		{
			correctMouseClickPosition.Y = 0.0f;
		}

		GGame->GetActiveWorld()->ConvertScreenToWorldCoordinates( view, (Int32)correctMouseClickPosition.X, (Int32)correctMouseClickPosition.Y, clickedWorldPos );

		m_flag->m_flagEntity->SetPosition( clickedWorldPos );
	}

	void CDebugWindowReviewMarkersEditMarker::NotifyOnKeepStateChanged( RedGui::CRedGuiEventPackage& package, Bool value )
	{
		if( value == true )
		{
			m_states->SetEnabled( false );
			m_states->SetSelectedIndex( -1 );
		}
		else
		{
			m_states->SetEnabled( true );
			// Set proper values
			Uint32 commentCount = m_flag->m_comments.Size();
			const CReviewFlagComment& lastComment = m_flag->m_comments[commentCount-1];

			// set correct value for flag
			Uint32 stateIndex = ( lastComment.m_state == 1 ) ? 0 : lastComment.m_state - 2;	// "-1" is there because user cannot change state to "Open"
			( m_states->GetItemCount() > 0 ) ? m_states->SetSelectedIndex( stateIndex ) : m_states->SetSelectedIndex( -1 );
		}
	}

	void CDebugWindowReviewMarkersEditMarker::NotifyOnKeepPriorityChanged( RedGui::CRedGuiEventPackage& package, Bool value )
	{
		if( value == true )
		{
			m_priority->SetEnabled( false );
			m_priority->SetSelectedIndex( -1 );
		}
		else
		{
			m_priority->SetEnabled( true );
			// Set proper values
			Uint32 commentCount = m_flag->m_comments.Size();
			const CReviewFlagComment& lastComment = m_flag->m_comments[commentCount-1];

			// set correct value for flag
			( m_priority->GetItemCount() > 0 ) ? m_priority->SetSelectedIndex( lastComment.m_priority ) : m_priority->SetSelectedIndex( -1 );
		}
	}

	void CDebugWindowReviewMarkersEditMarker::ClearControls()
	{
		// states
		TDynArray< String > states;
		m_attachedMarkerSystem->GetStateList( states );
		const Uint32 stateCount = states.Size();
		m_states->ClearAllItems();
		for( Uint32 i=0; i<stateCount; ++i )
		{
			if( i == RFS_Opened )
			{
				continue;
			}
			m_states->AddItem( states[i] );
		}

		// priorities
		TDynArray< String > priorities;
		m_attachedMarkerSystem->GetPriorityList( priorities );
		m_priority->ClearAllItems();
		for(Uint32 i=0; i<priorities.Size(); ++i)
		{
			m_priority->AddItem( priorities[i] );
		}

		m_keepState->SetChecked( false );
		m_keepPriority->SetChecked( false );
		m_makeScreen->SetChecked( true );
		m_comments->SetText( TXT("") );
		m_positionButton->SetToggleValue( false, true );
		m_waitingBox->Hide();
		m_changePositionMode = false;
	}

}	// namespace DebugWindows

#endif	// NO_MARKER_SYSTEMS
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
