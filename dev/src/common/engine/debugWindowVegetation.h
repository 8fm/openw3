/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "redGuiWindow.h"
#include "renderer.h"

namespace DebugWindows
{
	class CDebugWindowVegetation : public RedGui::CRedGuiWindow
	{
	public:
		CDebugWindowVegetation();
		~CDebugWindowVegetation();

	private:
		void NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime );
		void CreateControls();

		void UpdateHeapStatLabel( Uint32 index, const String& heapName, const SSpeedTreeResourceMetrics::HeapStats& stats );
		void UpdateVegetationInfo( Uint32 index, const String& text );

	private:
		RedGui::CRedGuiTab*		m_lods;
		RedGui::CRedGuiList*	m_lodList[3];
		RedGui::CRedGuiLabel*	m_memoryHeapMetrics[8];
		static const Uint32		sc_numVegetationInfos = 15;
		RedGui::CRedGuiLabel*	m_vegetationInfos[sc_numVegetationInfos];
	};

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
