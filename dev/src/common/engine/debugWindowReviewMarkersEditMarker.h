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
	class CDebugWindowReviewMarkersEditMarker : public RedGui::CRedGuiWindow
	{
	public:
		CDebugWindowReviewMarkersEditMarker();
		~CDebugWindowReviewMarkersEditMarker();

		void CreateControls();

		void SetParentWindow( RedGui::CRedGuiWindow* parentWindow );
		void SetFlagToShow( CReviewSystem* reviewSystem, CReviewFlag* flag );

	private:
		virtual void OnWindowClosed(CRedGuiControl* control);
		virtual void OnWindowOpened(CRedGuiControl* control);

		void NotifyOnAcceptModificationClicked( RedGui::CRedGuiEventPackage& package );
		void NotifyOnPositionButtonClicked( RedGui::CRedGuiEventPackage& package );
		void NotifyOnKeepStateChanged( RedGui::CRedGuiEventPackage& package, Bool value );
		void NotifyOnKeepPriorityChanged( RedGui::CRedGuiEventPackage& package, Bool value );

		void NotifyOnChangePosition( RedGui::CRedGuiEventPackage& eventPackage, IViewport* view, Int32 button, Bool state, Vector2 mousePosition );

		void SetFlagPosition( IViewport* view, const Vector& mouseClickPosition );
		void ClearControls();

	private:
		RedGui::CRedGuiButton*		m_acceptModification;
		RedGui::CRedGuiComboBox*	m_priority;
		RedGui::CRedGuiComboBox*	m_states;
		RedGui::CRedGuiCheckBox*	m_keepState;
		RedGui::CRedGuiCheckBox*	m_keepPriority;
		RedGui::CRedGuiCheckBox*	m_makeScreen;
		RedGui::CRedGuiTextBox*		m_comments;

		RedGui::CRedGuiButton*		m_positionButton;
		
		RedGui::CRedGuiWindow*		m_parentWindow;

		CReviewSystem*				m_attachedMarkerSystem;
		CReviewFlag*				m_flag;

		RedGui::CRedGuiProgressBox*	m_waitingBox;

		Bool						m_changePositionMode;
	};

}	// namespace DebugWindows

#endif	// NO_MARKER_SYSTEMS
#endif	// NO_RED_GUI
#endif	// NO_DEBUG_WINDOWS
