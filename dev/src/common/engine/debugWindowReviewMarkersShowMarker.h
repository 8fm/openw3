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
	class CDebugWindowReviewMarkersShowMarker : public RedGui::CRedGuiWindow
	{
	public:
		CDebugWindowReviewMarkersShowMarker();
		~CDebugWindowReviewMarkersShowMarker();

		void CreateControls();

		void SetParentWindow( RedGui::CRedGuiWindow* parentWindow );
		void SetFlagToShow( CReviewSystem* reviewSystem, CReviewFlag* flag );
		
	private:
		void FillInformation();
		void ClearControls();

		void OnWindowOpened( CRedGuiControl* control );
		void OnWindowClosed( CRedGuiControl* control );

		void NotifyOnEditButtonClicked( RedGui::CRedGuiEventPackage& package );
		void NotifyCopyScreenPathClicked( RedGui::CRedGuiEventPackage& package );
		void NotifyOnCommentSelected( RedGui::CRedGuiEventPackage& package, Int32 selectedItemIndex );

		void UpdateSelectedCommentInfo( Int32 selectedItemIndex );

	private:
		RedGui::CRedGuiButton*			m_closeWindow;
		RedGui::CRedGuiButton*			m_editMarker;

		RedGui::CRedGuiList*			m_comments;

		RedGui::CRedGuiWindow*			m_parentWindow;

		RedGui::CRedGuiLabel*			m_showFlagSummary;
		RedGui::CRedGuiLabel*			m_showFlagAuthor;
		RedGui::CRedGuiLabel*			m_showFlagPriority;
		RedGui::CRedGuiLabel*			m_showFlagState;
		RedGui::CRedGuiLabel*			m_showFlagType;
		RedGui::CRedGuiLabel*			m_showFlagCreated;
		RedGui::CRedGuiLabel*			m_showFlagTTPNumber;
		RedGui::CRedGuiLabel*			m_commentAuthor;
		RedGui::CRedGuiLabel*			m_commentState;
		RedGui::CRedGuiLabel*			m_commentPriority;
		RedGui::CRedGuiTextBox*			m_commentDescription;

		CReviewFlag*					m_flag;

		CReviewSystem*					m_attachedMarkerSystem;
	};

}	// namespace DebugWindows

#endif	// NO_MARKER_SYSTEMS
#endif	// NO_RED_GUI
#endif	// NO_DEBUG_WINDOWS
