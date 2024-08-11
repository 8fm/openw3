/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
#ifndef NO_MARKER_SYSTEMS

#include "redGuiWindow.h"

class CReviewSystem;
class CReviewFlagComment;
class CReviewFlag;

namespace DebugWindows
{
	class CDebugWindowReviewMarkersFilterMarker : public RedGui::CRedGuiWindow
	{
	public:
		CDebugWindowReviewMarkersFilterMarker();
		~CDebugWindowReviewMarkersFilterMarker();

		void CreateControls();

		void SetParentWindow( RedGui::CRedGuiWindow* parentWindow );
		void SetMarkerSystem( CReviewSystem* reviewSystem );

	private:
		virtual void OnWindowClosed(CRedGuiControl* control);
		virtual void OnWindowOpened(CRedGuiControl* control);

		void NotifyOnUseFiltersClicked( RedGui::CRedGuiEventPackage& package );

		void SetFilters();

		void NotifyOnResetFiltersClicked( RedGui::CRedGuiEventPackage& package );

		void NotifyOnShowMapChanged( RedGui::CRedGuiEventPackage& package, Bool value );
		void NotifyOnDownloadClosedFlagsChanged( RedGui::CRedGuiEventPackage& package, Bool value );

	private:
		RedGui::CRedGuiButton*		m_useFilters;
		RedGui::CRedGuiButton*		m_resetFilters;

		RedGui::CRedGuiCheckBox*	m_showOnMap;
		RedGui::CRedGuiCheckBox*	m_downloadClosedFlag;

		RedGui::CRedGuiList*		m_flagStates;
		RedGui::CRedGuiList*		m_flagTypes;

		RedGui::CRedGuiWindow*		m_parentWindow;
		CReviewSystem*				m_attachedMarkerSystem;
	};

}	// namespace DebugWindows

#endif	// NO_MARKER_SYSTEMS
#endif	// NO_RED_GUI
#endif	// NO_DEBUG_WINDOWS
