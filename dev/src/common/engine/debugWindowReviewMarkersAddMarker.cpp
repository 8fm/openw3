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
#include "redGuiOpenFileDialog.h"
#include "redGuiManager.h"
#include "debugWindowsManager.h"
#include "debugWindowReviewMarkersAddMarker.h"
#include "viewport.h"
#include "game.h"
#include "world.h"
#include "dynamicLayer.h"
#include "bitmapTexture.h"
#include "entity.h"


namespace DebugWindows
{
	CDebugWindowReviewMarkersAddMarker::CDebugWindowReviewMarkersAddMarker() 
		: RedGui::CRedGuiWindow( 200, 200, 400, 400 )
	{
		SetCaption( TXT("Review markers system - Add new marker") );

		// create all controls contain by the review marker debug window
		CreateControls();
	}

	CDebugWindowReviewMarkersAddMarker::~CDebugWindowReviewMarkersAddMarker()
	{
		/* intentionally empty */
	}

	void CDebugWindowReviewMarkersAddMarker::CreateControls()
	{
		m_addNewFlagButton = new RedGui::CRedGuiButton( 0, 0, 100, 20 );
		m_addNewFlagButton->SetMargin( Box2( 5, 5, 5, 5 ) );
		m_addNewFlagButton->SetText( TXT("Add new flag") );
		m_addNewFlagButton->SetDock( RedGui::DOCK_Bottom );
		m_addNewFlagButton->SetTabIndex( 5 );
		m_addNewFlagButton->EventButtonClicked.Bind( this, &CDebugWindowReviewMarkersAddMarker::NotifyOnAddNewFlagClicked );
		AddChild( m_addNewFlagButton );

		// informations
		RedGui::CRedGuiPanel* infoPanel = new RedGui::CRedGuiPanel( 0, 0, GetWidth(), GetHeight() );
		infoPanel->SetBorderVisible( false );
		infoPanel->SetMargin( Box2(5, 5, 5, 5) );
		infoPanel->SetDock( RedGui::DOCK_Fill );
		AddChild( infoPanel );
		{
			// row for summary
			RedGui::CRedGuiPanel* summaryPanel = new RedGui::CRedGuiPanel( 0, 0, 100, 20 );
			summaryPanel->SetBackgroundColor( Color::CLEAR );
			summaryPanel->SetBorderVisible( false );
			summaryPanel->SetMargin( Box2( 10, 5, 10, 5 ) );
			summaryPanel->SetDock( RedGui::DOCK_Top );
			infoPanel->AddChild( summaryPanel );
			{
				RedGui::CRedGuiLabel* summaryLabel = new RedGui::CRedGuiLabel( 0, 0, 100, 10 );
				summaryLabel->SetText( TXT("Summary: ") );
				summaryLabel->SetMargin( Box2( 0, 2, 10, 0 ) );
				summaryLabel->SetDock( RedGui::DOCK_Left );
				summaryPanel->AddChild( summaryLabel );

				m_summaryText = new RedGui::CRedGuiTextBox( 0, 0, 150, 20 );
				m_summaryText->SetDock( RedGui::DOCK_Fill );
				m_summaryText->SetTabIndex( 0 );
				summaryPanel->AddChild( m_summaryText );
			}

			// row for priority
			RedGui::CRedGuiPanel* priorityPanel = new RedGui::CRedGuiPanel( 0,0, 100, 20 );
			priorityPanel->SetBackgroundColor( Color::CLEAR );
			priorityPanel->SetBorderVisible( false );
			priorityPanel->SetMargin( Box2( 10, 5, 10, 5 ) );
			infoPanel->AddChild( priorityPanel );
			priorityPanel->SetDock( RedGui::DOCK_Top );
			{
				RedGui::CRedGuiLabel* priorityLabel = new RedGui::CRedGuiLabel( 0, 0, 100, 10 );
				priorityLabel->SetText( TXT("Priority: ") );
				priorityLabel->SetMargin( Box2( 0, 2, 10, 0 ) );
				priorityLabel->SetDock( RedGui::DOCK_Left );
				priorityPanel->AddChild( priorityLabel );

				m_flagPriority = new RedGui::CRedGuiComboBox( 0, 0, 150, 20 );
				m_flagPriority->SetDock( RedGui::DOCK_Fill );
				m_flagPriority->SetTabIndex( 1 );
				priorityPanel->AddChild( m_flagPriority );
			}

			// row for type
			RedGui::CRedGuiPanel* typePanel = new RedGui::CRedGuiPanel( 0,0, 100, 20 );
			typePanel->SetBackgroundColor( Color::CLEAR );
			typePanel->SetBorderVisible( false );
			typePanel->SetMargin( Box2( 10, 5, 10, 5 ) );
			typePanel->SetDock( RedGui::DOCK_Top );
			infoPanel->AddChild( typePanel );
			{
				RedGui::CRedGuiLabel* typeLabel = new RedGui::CRedGuiLabel( 0, 0, 100, 10 );
				typeLabel->SetText( TXT("Type: ") );
				typeLabel->SetMargin( Box2( 0, 2, 10, 0 ) );
				typeLabel->SetDock( RedGui::DOCK_Left );
				typePanel->AddChild( typeLabel );

				m_flagType = new RedGui::CRedGuiComboBox( 0, 0, 150, 20 );
				m_flagType->SetDock( RedGui::DOCK_Fill );
				m_flagType->SetTabIndex( 2 );
				typePanel->AddChild( m_flagType );
			}

			// button to add new flag on map
			RedGui::CRedGuiPanel* setOnMapPanel = new RedGui::CRedGuiPanel( 0,0, 100, 20 );
			setOnMapPanel->SetBackgroundColor( Color::CLEAR );
			setOnMapPanel->SetBorderVisible( false );
			setOnMapPanel->SetMargin( Box2( 10, 5, 10, 5 ) );
			setOnMapPanel->SetDock( RedGui::DOCK_Top );
			infoPanel->AddChild( setOnMapPanel );
			{
				RedGui::CRedGuiLabel* setOnMapLabel = new RedGui::CRedGuiLabel( 0, 0, 100, 10 );
				setOnMapLabel->SetText( TXT("Set on map: ") );
				setOnMapLabel->SetMargin( Box2( 0, 2, 10, 0 ) );
				setOnMapLabel->SetDock( RedGui::DOCK_Left );
				setOnMapPanel->AddChild( setOnMapLabel );

				m_setFlagOnMap = new RedGui::CRedGuiButton( 0, 0, 20, 20 );
				m_setFlagOnMap->SetMargin( Box2( 2, 0, 0, 0 ) );
				m_setFlagOnMap->SetDock( RedGui::DOCK_Left );
				m_setFlagOnMap->SetTabIndex( 3 );
				m_setFlagOnMap->SetImage( RedGui::Resources::GReviewMoveIcon );
				m_setFlagOnMap->EventButtonClicked.Bind( this, &CDebugWindowReviewMarkersAddMarker::NotifyOnSetFlagOnMapClicked );
				setOnMapPanel->AddChild( m_setFlagOnMap );
			}

			// row for description label
			RedGui::CRedGuiPanel* descriptionPanel = new RedGui::CRedGuiPanel( 0,0, 100, 30 );
			descriptionPanel->SetBackgroundColor( Color::CLEAR );
			descriptionPanel->SetBorderVisible( false );
			descriptionPanel->SetMargin( Box2( 10, 5, 10, 0 ) );
			descriptionPanel->SetDock( RedGui::DOCK_Top );
			infoPanel->AddChild( descriptionPanel );
			{
				RedGui::CRedGuiLabel* descriptionLabel = new RedGui::CRedGuiLabel( 0, 0, 100, 10 );
				descriptionLabel->SetText( TXT("Description: ") );
				descriptionLabel->SetMargin( Box2( 0, 12, 10, 0 ) );
				descriptionLabel->SetDock( RedGui::DOCK_Left );
				descriptionPanel->AddChild( descriptionLabel );
			}

			// description
			m_descriptionText = new RedGui::CRedGuiTextBox( 0, 0, 100, 100 );
			m_descriptionText->SetMargin( Box2( 10, 0, 10, 5 ) );
			m_descriptionText->SetDock( RedGui::DOCK_Fill );
			m_descriptionText->SetMultiLine( true );
			m_descriptionText->SetTabIndex( 4 );
			infoPanel->AddChild( m_descriptionText );
		}
	}

	void CDebugWindowReviewMarkersAddMarker::SetParentWindow( RedGui::CRedGuiWindow* parentWindow )
	{
		m_parentWindow = parentWindow;
	}

	Bool CDebugWindowReviewMarkersAddMarker::SetFlagToShow( CReviewSystem* reviewSystem, CReviewFlag* flag )
	{
		m_attachedMarkerSystem = reviewSystem;
		m_flag = flag;
		return m_flag != nullptr && m_attachedMarkerSystem != nullptr;
	}

	void CDebugWindowReviewMarkersAddMarker::OnWindowClosed( CRedGuiControl* control )
	{
		// remove temporary flag if it exists
		CReviewFlag* newFlag = m_attachedMarkerSystem->GetNewFlag();
		if( newFlag != nullptr )
		{
			if( newFlag->m_flagEntity != nullptr )
			{
				GGame->GetActiveWorld()->GetDynamicLayer()->RemoveEntity( newFlag->m_flagEntity.Get() );
				newFlag->m_flagEntity = nullptr;
			}
		}

		// show parent window with flags
		if( m_parentWindow != nullptr )
		{
			m_parentWindow->SetVisible( true );
			m_parentWindow = nullptr;
		}

		m_attachedMarkerSystem->UnlockUpdate();
	}

	void CDebugWindowReviewMarkersAddMarker::OnWindowOpened( CRedGuiControl* control )
	{
		m_attachedMarkerSystem->LockUpdate();

		ClearControls();

		m_attachedMarkerSystem->CreateNewFlag();
	}

	void CDebugWindowReviewMarkersAddMarker::NotifyOnSetFlagOnMapClicked( RedGui::CRedGuiEventPackage& package )
	{
		m_setFlagOnMap->SetEnabled( false );
		Minimize();
		m_setFlagMode = false;

		GRedGui::GetInstance().EventViewportClick.Bind( this, &CDebugWindowReviewMarkersAddMarker::NotifyOnChangePosition );
	}

	void CDebugWindowReviewMarkersAddMarker::NotifyOnAddNewFlagClicked( RedGui::CRedGuiEventPackage& package )
	{
		GDebugWin::GetInstance().LockHiding();

		if( m_flag == nullptr )
		{
			GRedGui::GetInstance().MessageBox( TXT( "Flag was not created. Try again." ), TXT( "Failure" ), RedGui::MESSAGEBOX_Error );
			SetVisible( false );
			return;
		}
		if( m_summaryText->GetText().Empty() == true )
		{
			GRedGui::GetInstance().MessageBox( TXT("Flag must have a summary!"), TXT("Remember!!!"), RedGui::MESSAGEBOX_Warning );
			return;
		}
		if( m_descriptionText->GetText().Empty() == true )
		{
			GRedGui::GetInstance().MessageBox( TXT("Flag must have a description!"), TXT("Remember!!!"), RedGui::MESSAGEBOX_Warning );
			return;
		}
		if( m_flag->m_flagEntity == nullptr )
		{
			GRedGui::GetInstance().MessageBox( TXT("You must set flag on map!"), TXT("Remember!!!"), RedGui::MESSAGEBOX_Warning );
			return;
		}

		// create new entry with flag
		CReviewFlagComment newComment;

		Char lpszUsername[255];
		DWORD dUsername = sizeof(lpszUsername);
		newComment.m_author = TXT("Unknown user");
		if( GetUserName(lpszUsername, &dUsername) == (BOOL)true )
		{
			newComment.m_author = lpszUsername;
		}
			

		newComment.m_priority = m_flagPriority->GetSelectedIndex();
		newComment.m_state = 1;

		const EulerAngles& eulerAngles = GGame->GetFreeCamera().GetRotation();
		newComment.m_cameraOrientation = Vector( eulerAngles.Roll, eulerAngles.Pitch, eulerAngles.Yaw );
		newComment.m_cameraPosition = GGame->GetFreeCamera().GetPosition();
		newComment.m_description = m_descriptionText->GetText();

		m_flag->m_linkToVideo = TXT("");	// link to video is deprecated
		m_flag->m_mapName = GGame->GetActiveWorld()->GetDepotPath();
		m_flag->m_summary = m_summaryText->GetText();
		m_flag->m_type = m_flagType->GetSelectedIndex();
		newComment.m_flagPosition = m_flag->m_flagEntity->GetWorldPosition();
		m_flag->m_comments.PushBack( newComment );

		String pathToScreen = TXT("");
		if(m_attachedMarkerSystem->AddNewFlag( pathToScreen ) == true)
		{
			GRedGui::GetInstance().MessageBox(TXT("Add flag succeed"), TXT("Information"), RedGui::MESSAGEBOX_Info);
			SetVisible( false );
		}
		else
		{
			GRedGui::GetInstance().MessageBox(TXT("Flag has not been added"), TXT("Error"), RedGui::MESSAGEBOX_Error);
		}

		GDebugWin::GetInstance().UnlockHiding();
	}

	void CDebugWindowReviewMarkersAddMarker::NotifyOnChangePosition( RedGui::CRedGuiEventPackage& eventPackage, IViewport* view, Int32 button, Bool state, Vector2 mousePosition )
	{
		if ( m_flag )
		{
			if( m_setFlagMode == true )
			{
				GRedGui::GetInstance().EventViewportClick.Unbind( this, &CDebugWindowReviewMarkersAddMarker::NotifyOnChangePosition );

				if( m_flag->m_flagEntity == nullptr )
				{
					CreateNewFlagEntity();
				}

				SetFlagPosition( view, mousePosition );

				Minimize();
				m_setFlagOnMap->SetEnabled( true );
			}

			m_setFlagMode = true;
		}
		else
		{
			GRedGui::GetInstance().MessageBox( TXT("Couldn't change flag's position. Close the AddMarker window and try again.") );
		}
	}

	void CDebugWindowReviewMarkersAddMarker::SetFlagPosition( IViewport* view, const Vector& mouseClickPosition )
	{
		if ( GGame->GetActiveWorld() == nullptr )
		{
			GRedGui::GetInstance().MessageBox( TXT("No active world. Try again after loading game.") );
			return;
		}

		if ( m_flag == nullptr )
		{
			GRedGui::GetInstance().MessageBox( TXT("Couldn't set flag's position. Close AddMarker window and try again.") );
			return;
		}

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

	void CDebugWindowReviewMarkersAddMarker::CreateNewFlagEntity()
	{
		THandle< CEntityTemplate > entityTemplate = m_attachedMarkerSystem->GetFlagTemplate( RFS_Opened );
		if( entityTemplate != nullptr && m_flag != nullptr )
		{
			EntitySpawnInfo einfo;
			einfo.m_spawnPosition = Vector::ZEROS;
			einfo.m_detachTemplate = false;
			einfo.m_template = entityTemplate;

			// remove if exist
			if( m_flag->m_flagEntity.Get() != nullptr )
			{
				GGame->GetActiveWorld()->GetDynamicLayer()->RemoveEntity( m_flag->m_flagEntity.Get() );
				m_flag->m_flagEntity = nullptr;
			}

			// Create flag
			m_flag->m_flagEntity = GGame->GetActiveWorld()->GetDynamicLayer()->CreateEntitySync( einfo );
			if ( m_flag->m_flagEntity.Get() != nullptr )
			{
				// Add basic tags
				TagList tags = m_flag->m_flagEntity->GetTags();
				tags.AddTag( CNAME( LockedObject ) );
				tags.AddTag( CNAME( ReviewFlagObject ) );
				m_flag->m_flagEntity->SetTags( tags );
			}
		}
		else
		{
			GRedGui::GetInstance().MessageBox( entityTemplate == nullptr ? 
				TXT( "Couldn't find flag's entity template. Close AddMarker window and try again." ) : 
				TXT( "Couldn't create flag entity. Close AddMarker window and try again." ) )	;
		}
	}

	void CDebugWindowReviewMarkersAddMarker::ClearControls()
	{
		// clear controls
		m_setFlagOnMap->SetToggleValue( false );
		m_summaryText->SetText( TXT("") );
		m_descriptionText->SetText( TXT("") );

		// set correct values
		TDynArray<String> types;
		m_attachedMarkerSystem->GetBugTypeList( types );
		m_flagType->ClearAllItems();
		for(Uint32 i=0; i<types.Size(); ++i)
		{
			m_flagType->AddItem(  types[i] );
		}
		( types.Size() > 0 ) ? m_flagType->SetSelectedIndex( 0 ) : m_flagType->SetSelectedIndex( -1 );

		TDynArray< String > priorities;
		m_attachedMarkerSystem->GetPriorityList( priorities );
		m_flagPriority->ClearAllItems();
		for(Uint32 i=0; i<priorities.Size(); ++i)
		{
			m_flagPriority->AddItem( priorities[i] );
		}
		( priorities.Size() > 0 ) ? m_flagPriority->SetSelectedIndex( 0 ) : m_flagPriority->SetSelectedIndex( -1 );

		m_setFlagMode = false;
	}

}	// namespace DebugWindows

#endif	// NO_MARKER_SYSTEMS
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
