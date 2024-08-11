/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "redGuiWindow.h"
#include "renderer.h"

namespace RedGui
{
	class CRedGuiClipmapVisualizer;
}


namespace DebugWindows
{
	class CDebugWindowTerrain : public RedGui::CRedGuiWindow
	{
	private:
		enum Tab
		{
			TAB_TileLoadStatus,
		};

		RedGui::CRedGuiTab*					m_tabs;
		RedGui::CRedGuiClipmapVisualizer*	m_tileLoadClipmapView;
		RedGui::CRedGuiSpin*				m_tileLoadClipStackLevel;
		RedGui::CRedGuiLabel*				m_tileLoadLoading;

	public:
		CDebugWindowTerrain();
		~CDebugWindowTerrain();

	private:
		void NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime );
		void CreateControls();

		void UpdateTileLoadStatus();
	};

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
