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
#include "redGuiListItem.h"
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
#include "debugWindowReviewMarkersFilterMarker.h"
#include "entity.h"
#include "bitmapTexture.h"


namespace DebugWindows
{
	CDebugWindowReviewMarkersFilterMarker::CDebugWindowReviewMarkersFilterMarker() 
		: RedGui::CRedGuiWindow( 200, 200, 350, 360 )
	{
		SetCaption( TXT("Review markers system - Filters markers") );

		// create all controls contain by the review marker debug window
		CreateControls();
	}

	CDebugWindowReviewMarkersFilterMarker::~CDebugWindowReviewMarkersFilterMarker()
	{
		/* intentionally empty */
	}

	void CDebugWindowReviewMarkersFilterMarker::CreateControls()
	{
		// informations
		RedGui::CRedGuiPanel* buttonsPanel = new RedGui::CRedGuiPanel( 0, 0, GetWidth(), 20 );
		buttonsPanel->SetBorderVisible( false );
		buttonsPanel->SetBackgroundColor( Color::CLEAR );
		buttonsPanel->SetMargin( Box2(5, 5, 5, 5) );
		buttonsPanel->SetDock( RedGui::DOCK_Bottom );
		AddChild( buttonsPanel );
		{
			m_resetFilters = new RedGui::CRedGuiButton( 0, 0, 100, 20 );
			m_resetFilters->SetText( TXT("Reset") );
			m_resetFilters->SetDock( RedGui::DOCK_Right );
			m_resetFilters->SetTabIndex( 5 );
			m_resetFilters->EventButtonClicked.Bind( this, &CDebugWindowReviewMarkersFilterMarker::NotifyOnResetFiltersClicked );
			buttonsPanel->AddChild( m_resetFilters );

			m_useFilters = new RedGui::CRedGuiButton( 0, 0, 100, 20 );
			m_useFilters->SetMargin( Box2( 0, 0, 5, 0 ) );
			m_useFilters->SetText( TXT("Apply") );
			m_useFilters->SetDock( RedGui::DOCK_Fill );
			m_useFilters->SetTabIndex( 4 );
			m_useFilters->EventButtonClicked.Bind( this, &CDebugWindowReviewMarkersFilterMarker::NotifyOnUseFiltersClicked );
			buttonsPanel->AddChild( m_useFilters );
		}

		// informations
		RedGui::CRedGuiPanel* infoPanel = new RedGui::CRedGuiPanel( 0, 0, GetWidth(), GetHeight() );
		infoPanel->SetBorderVisible( false );
		infoPanel->SetMargin( Box2(5, 5, 5, 5) );
		infoPanel->SetDock(RedGui::DOCK_Fill);
		AddChild(infoPanel);
		{
			// row for check box options
			RedGui::CRedGuiPanel* optionsPanel = new RedGui::CRedGuiPanel( 0, 0, 100, 20 );
			optionsPanel->SetBackgroundColor( Color::CLEAR );
			optionsPanel->SetBorderVisible( false );
			optionsPanel->SetMargin( Box2( 30, 5, 10, 5 ) );
			optionsPanel->SetDock( RedGui::DOCK_Top );
			infoPanel->AddChild( optionsPanel );
			{
				m_showOnMap = new RedGui::CRedGuiCheckBox( 0, 0, 100, 10 );
				m_showOnMap->SetText( TXT("Show on map") );
				m_showOnMap->SetMargin( Box2( 0, 2, 10, 0 ) );
				m_showOnMap->SetDock( RedGui::DOCK_Left );
				m_showOnMap->SetTabIndex( 0 );
				m_showOnMap->EventCheckedChanged.Bind( this, &CDebugWindowReviewMarkersFilterMarker::NotifyOnShowMapChanged );
				optionsPanel->AddChild( m_showOnMap );

				m_downloadClosedFlag = new RedGui::CRedGuiCheckBox( 0, 0, 100, 10 );
				m_downloadClosedFlag->SetText( TXT("Download closed flag") );
				m_downloadClosedFlag->SetMargin( Box2( 10, 2, 10, 0 ) );
				m_downloadClosedFlag->SetDock( RedGui::DOCK_Left );
				m_downloadClosedFlag->SetTabIndex( 1 );
				m_downloadClosedFlag->EventCheckedChanged.Bind( this, &CDebugWindowReviewMarkersFilterMarker::NotifyOnDownloadClosedFlagsChanged );
				optionsPanel->AddChild( m_downloadClosedFlag );
			}
		}

		// add separator
		RedGui::CRedGuiSeparator* separator = new RedGui::CRedGuiSeparator( 0, 0, GetWidth(), 1 );
		infoPanel->AddChild( separator );
		separator->SetMargin( Box2( 10, 10, 10, 10 ) );
		separator->SetDock( RedGui::DOCK_Top );

		// information bottom panel
		RedGui::CRedGuiPanel* bottomInfoPanel = new RedGui::CRedGuiPanel( 0, 0, GetWidth(), 220 );
		bottomInfoPanel->SetBackgroundColor( Color::CLEAR );
		bottomInfoPanel->SetBorderVisible( false );
		bottomInfoPanel->SetMargin( Box2( 5, 5, 5, 5 ) );
		bottomInfoPanel->SetDock( RedGui::DOCK_Fill );
		infoPanel->AddChild( bottomInfoPanel );
		{
			m_flagStates = new RedGui::CRedGuiList( 0,0, 100, 100 );
			m_flagStates->SetBackgroundColor( Color::CLEAR );
			m_flagStates->SetMargin( Box2( 10, 5, 10, 5 ) );
			m_flagStates->SetDock( RedGui::DOCK_Top );
			m_flagStates->SetSelectionMode( RedGui::SM_Multiple );
			m_flagStates->AppendColumn( TXT("States"), 100 );
			m_flagStates->SetTabIndex( 2 );
			infoPanel->AddChild( m_flagStates );

			m_flagTypes = new RedGui::CRedGuiList( 0, 0, 100, 120 );
			m_flagTypes->SetBackgroundColor( Color::CLEAR );
			m_flagTypes->SetMargin( Box2( 10, 5, 10, 5 ) );
			m_flagTypes->SetDock( RedGui::DOCK_Top );
			m_flagTypes->SetSelectionMode( RedGui::SM_Multiple );
			m_flagTypes->AppendColumn( TXT("Types"), 100 );
			m_flagTypes->SetTabIndex( 3 );
			infoPanel->AddChild( m_flagTypes );
		}
	}

	void CDebugWindowReviewMarkersFilterMarker::NotifyOnUseFiltersClicked( RedGui::CRedGuiEventPackage& package )
	{
		SetFilters();

		this->SetVisible( false );
	}

	void CDebugWindowReviewMarkersFilterMarker::NotifyOnResetFiltersClicked( RedGui::CRedGuiEventPackage& package )
	{
		const Uint32 stateCount = m_flagStates->GetItemCount();
		for( Uint32 i=0; i<stateCount; ++i )
		{
			m_flagStates->GetItem( i )->SetSelected( true );
		}

		const Uint32 typeCount = m_flagTypes->GetItemCount();
		for( Uint32 i=0; i<typeCount; ++i )
		{
			m_flagTypes->GetItem( i )->SetSelected( true );
		}

		SetFilters();
	}

	void CDebugWindowReviewMarkersFilterMarker::NotifyOnShowMapChanged( RedGui::CRedGuiEventPackage& package, Bool value )
	{
		m_attachedMarkerSystem->SetShowOnMap( value );
	}

	void CDebugWindowReviewMarkersFilterMarker::NotifyOnDownloadClosedFlagsChanged( RedGui::CRedGuiEventPackage& package, Bool value )
	{
		m_attachedMarkerSystem->SetDownloadClosedFlags( value );
	}

	void CDebugWindowReviewMarkersFilterMarker::OnWindowClosed( CRedGuiControl* control )
	{
		if( m_parentWindow != nullptr )
		{
			m_parentWindow->SetVisible( true );
			m_parentWindow = nullptr;
		}

		m_attachedMarkerSystem->UnlockUpdate();
	}

	void CDebugWindowReviewMarkersFilterMarker::OnWindowOpened( CRedGuiControl* control )
	{
		m_attachedMarkerSystem->LockUpdate();

		m_showOnMap->SetChecked( m_attachedMarkerSystem->GetShowOnMap() );
		m_downloadClosedFlag->SetChecked( m_attachedMarkerSystem->GetDownloadClosedFlags() );

		TDynArray< String > states;
		TDynArray< Bool > stateValues;
		m_attachedMarkerSystem->GetStateList( states );
		m_attachedMarkerSystem->GetStateValues( stateValues );

		m_flagStates->RemoveAllItems();
		const Uint32 stateCount = states.Size();
		for( Uint32 i=0; i<stateCount; ++i )
		{
			m_flagStates->AddItem( states[i] );
			if( stateValues.Size() > i && stateValues[i] == true )
			{
				m_flagStates->SetSelection( i );
			}
		}

		TDynArray< String > types;
		TDynArray< Bool > typeValues;
		m_attachedMarkerSystem->GetBugTypeList( types );
		m_attachedMarkerSystem->GetTypesValues( typeValues );

		m_flagTypes->RemoveAllItems();
		const Uint32 typeCount = types.Size();
		for( Uint32 i=0; i<typeCount; ++i )
		{
			m_flagTypes->AddItem( types[i] );
			if( typeValues.Size() > i && typeValues[i] == true )
			{
				m_flagTypes->SetSelection( i );
			}
		}
	}

	void CDebugWindowReviewMarkersFilterMarker::SetParentWindow( RedGui::CRedGuiWindow* parentWindow )
	{
		m_parentWindow = parentWindow;
	}

	void CDebugWindowReviewMarkersFilterMarker::SetMarkerSystem( CReviewSystem* reviewSystem )
	{
		m_attachedMarkerSystem = reviewSystem;
	}

	void CDebugWindowReviewMarkersFilterMarker::SetFilters()
	{
		TDynArray<Bool> states;
		const Uint32 stateCount = m_flagStates->GetItemCount();
		for( Uint32 i=0; i<stateCount; ++i )
		{
			states.PushBack( m_flagStates->IsSelected( i ) );
		}

		TDynArray<Bool> types;
		const Uint32 typeCount = m_flagTypes->GetItemCount();
		for( Uint32 i=0; i<typeCount; ++i )
		{
			types.PushBack(m_flagTypes->IsSelected( i ) );
		}

		m_attachedMarkerSystem->SetFilters( states, types );
	}

}	// namespace DebugWindows

#endif	// NO_MARKER_SYSTEMS
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
