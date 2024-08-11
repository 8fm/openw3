/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "redGuiWindow.h"
#include "showFlags.h"

namespace DebugWindows
{
	class CDebugWindowRenderingFlags : public RedGui::CRedGuiWindow
	{
	public:
		CDebugWindowRenderingFlags();
		~CDebugWindowRenderingFlags();

	private:
		void NotifySelectedListItem( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedOption);
		void NotifyCheckBoxValueChanged( RedGui::CRedGuiEventPackage& eventPackage, Bool value );

		void HideAllPanels();

		// Rendering
		void CreateRenderingFlagsWindow();
		void ShowRenderingFlagsPanel();

		// Post process
		void CreatePostprocessFlagsWindow();
		void ShowPostprocessFlagsPanel();

		// Debug
		void CreateDebugFlagsWindow();
		void ShowDebugFlagsPanel();

		// Umbra
		void CreateUmbraFlagsWindow();
		void ShowUmbraFlagsPanel();

		void CreatePhysicsFlagsWindow();
		void ShowPhysicsFlagsPanel();

	private:
		void SortAndFillMenu( RedGui::CRedGuiScrollPanel* menuPanel, TDynArray< RedGui::CRedGuiCheckBox* >& flags );

	private:
		TDynArray< EShowFlags >					m_showFlags;
		RedGui::CRedGuiPanel*					m_rightPanel;

		RedGui::CRedGuiScrollPanel*				m_postprocessFlagsPanel;
		RedGui::CRedGuiScrollPanel*				m_renderingFlagsPanel;
		RedGui::CRedGuiScrollPanel*				m_debugFlagsPanel;
		RedGui::CRedGuiScrollPanel*				m_umbraFlagsPanel;
		RedGui::CRedGuiScrollPanel*				m_physicsFlagsPanel;
		TDynArray< RedGui::CRedGuiCheckBox* >	m_renderingFlags;
		TDynArray< RedGui::CRedGuiCheckBox* >	m_postprocessFlags;
		TDynArray< RedGui::CRedGuiCheckBox* >	m_debugFlags;
		TDynArray< RedGui::CRedGuiCheckBox* >	m_umbraFlags;
		TDynArray< RedGui::CRedGuiCheckBox* >	m_physicsFlags;
	};

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
