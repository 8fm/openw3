/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
#ifndef NO_MARKER_SYSTEMS

#include "redGuiWindow.h"
#include "abstractMarkerSystem.h"

class CReviewSystem;
class CReviewFlagComment;
class CReviewFlag;

namespace DebugWindows
{
	class CDebugWindowReviewMarkersAddMarker;
	class CDebugWindowReviewMarkersShowMarker;
	class CDebugWindowReviewMarkersEditMarker;
	class CDebugWindowReviewMarkersFilterMarker;

	enum EReviewMarkerColumn
	{
		RMC_Name,
		RMC_Priority,
		RMC_State,
		RMC_Type,
		RMC_Date,

		RMC_Count
	};

	class CDebugWindowReviewMarkers : public RedGui::CRedGuiWindow, public IMarkerSystemListener
	{
	public:
		CDebugWindowReviewMarkers();
		~CDebugWindowReviewMarkers();

		void CreateControls();
		void CreateLeftPanelControls();

		void EditFlagDirectly( CReviewFlag* flag );

	private:
		virtual void OnWindowClosed(CRedGuiControl* control);
		virtual void OnWindowOpened(CRedGuiControl* control);

		void SetVisible( Bool value ) override;

		virtual void ProcessMessage( enum EMarkerSystemMessage message, enum EMarkerSystemType systemType, IMarkerSystemInterface* system );

		void FillFlagsWindow();
		void ClearControls();

		void CreateDependentWindows();
		void UnregisterDependentWindows();

		void SearchFlags();
		void ShowFlagWindow( Int32 selectedFlagIndex );

		void InternalProcessMessage();

		void NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime );
		void NotifyEventViewportInput( RedGui::CRedGuiEventPackage&, IViewport* view, enum EInputKey key, enum EInputAction action, Float data );

		// main panel functions
		void NotifyEventDoubleClickItemFlag( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedIndex );
		void NotifyEventColumnClicked( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedIndex );
		void NotifyEventButtonClickedRefreshFlags( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyEventCheckedChangedAutoSync( RedGui::CRedGuiEventPackage& eventPackage, Bool value );
		void NotifyEventValueChangedSyncTime( RedGui::CRedGuiEventPackage& eventPackage, Int32 value );

		// search panel functions
		void NotifySearchClicked( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifySearchTextEnter( RedGui::CRedGuiEventPackage& eventPackage );

		// new windows functions
		void NotifyEventButtonClickedShowFlag( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyEventButtonClickedNewFlag( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyEventButtonClickedEditFlag( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyEventButtonClickedShowFilter( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyEventButtonClickedResetFilter( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyEventButtonClickedShowFreeCamInfo( RedGui::CRedGuiEventPackage& eventPackage );
		
		void NotifyOnLoginToReviewSystem( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyOnCancelLoginProcess( RedGui::CRedGuiEventPackage& eventPackage );

	private:
		RedGui::CRedGuiPanel*					m_leftPanel;
		RedGui::CRedGuiPanel*					m_mainControlPanel;
		RedGui::CRedGuiButton*					m_showFlagButton;
		RedGui::CRedGuiButton*					m_addNewFlagButton;
		RedGui::CRedGuiButton*					m_editExistingFlagButton;
		RedGui::CRedGuiSpin*					m_autoSyncTime;
		RedGui::CRedGuiCheckBox*				m_autoSyncCheckBox;
		RedGui::CRedGuiButton*					m_refreshFlagsButton;
		RedGui::CRedGuiButton*					m_openFiltersButton;
		RedGui::CRedGuiList*					m_reviewFlagList;

		RedGui::CRedGuiButton*					m_searchButton;
		RedGui::CRedGuiTextBox*					m_searchArea;
		RedGui::CRedGuiComboBox*				m_searchCategory;

		RedGui::CRedGuiProgressBox*				m_waitingBox;

		// Dependent windows
		CDebugWindowReviewMarkersAddMarker*		m_addMarkerWindow;
		CDebugWindowReviewMarkersShowMarker*	m_showMarkerWindow;
		CDebugWindowReviewMarkersEditMarker*	m_editMarkerWindow;
		CDebugWindowReviewMarkersFilterMarker*	m_filterMarkerWindow;

		RedGui::CRedGuiLoginBox*				m_loginBox;
		Bool									m_isLoggedIn;
		Bool									m_freeCameraMode;
		Uint32									m_globalAlphaValue;

		// logic
		CReviewSystem*							m_attachedMarkerSystem;
		Bool									m_imidiateMode;		// children window is opened when this window is closed

		Red::Threads::CMutex					m_synchronizationMutex;		//!<
		TQueue< EMarkerSystemMessage >			m_messages;
	};

}	// namespace DebugWindows

#endif	// NO_MARKER_SYSTEMS
#endif	// NO_RED_GUI
#endif	// NO_DEBUG_WINDOWS
