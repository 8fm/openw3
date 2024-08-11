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
	class CDebugWindowReviewMarkersAddMarker : public RedGui::CRedGuiWindow
	{
	public:
		CDebugWindowReviewMarkersAddMarker();
		~CDebugWindowReviewMarkersAddMarker();

		void CreateControls();

		void SetParentWindow( RedGui::CRedGuiWindow* parentWindow );
		Bool SetFlagToShow( CReviewSystem* reviewSystem, CReviewFlag* flag );

	private:
		virtual void OnWindowClosed(CRedGuiControl* control);
		virtual void OnWindowOpened(CRedGuiControl* control);

		void NotifyOnAddNewFlagClicked( RedGui::CRedGuiEventPackage& package );
		void NotifyOnSetFlagOnMapClicked( RedGui::CRedGuiEventPackage& package );

		void NotifyOnChangePosition( RedGui::CRedGuiEventPackage& eventPackage, IViewport* view, Int32 button, Bool state, Vector2 mousePosition );

		void SetFlagPosition( IViewport* view, const Vector& mouseClickPosition );
		void CreateNewFlagEntity();
		void ClearControls();

	private:
		RedGui::CRedGuiButton*		m_addNewFlagButton;
		RedGui::CRedGuiButton*		m_setFlagOnMap;

		RedGui::CRedGuiTextBox*		m_summaryText;
		RedGui::CRedGuiTextBox*		m_descriptionText;

		RedGui::CRedGuiComboBox*	m_flagPriority;
		RedGui::CRedGuiComboBox*	m_flagType;

		RedGui::CRedGuiWindow*		m_parentWindow;
		CReviewSystem*				m_attachedMarkerSystem;
		CReviewFlag*				m_flag;

		Bool						m_setFlagMode;
	};

}	// namespace DebugWindows

#endif	// NO_MARKER_SYSTEMS
#endif	// NO_RED_GUI
#endif	// NO_DEBUG_WINDOWS
