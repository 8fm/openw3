/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "redGuiWindow.h"
#include "../core/engineTime.h"

namespace DebugWindows
{
	class CDebugWindowStreamingInstaller : public RedGui::CRedGuiWindow
	{
	public:
		CDebugWindowStreamingInstaller();
		~CDebugWindowStreamingInstaller();

	private:
		virtual void OnWindowOpened( CRedGuiControl* control ) override;
		virtual void OnWindowClosed( CRedGuiControl* control ) override;

	private:
		void NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime );
		void CreateControls();

	private:
		// gui
// 		RedGui::CRedGuiCheckBox*				m_timelineLegend;
// 		static RedGui::CRedGuiCheckBox*			m_pauseTimeline;
// 		static RedGui::CRedGuiTimelineChart*	m_timelineChart;
// 		RedGui::CRedGuiLabel*					m_pendingTasksLabel;
// 		RedGui::CRedGuiList*					m_taskList;
// 		RedGui::CRedGuiAdvancedSlider*			m_zoomSlider;
	};

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
