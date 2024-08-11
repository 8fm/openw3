/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "../engine/redGuiWindow.h"

namespace DebugWindows
{
	class CDebugWindowGpuResourceUse : public RedGui::CRedGuiWindow
	{

	public:
		CDebugWindowGpuResourceUse();
		~CDebugWindowGpuResourceUse();

	private:
		void AddItem( RedGui::CRedGuiPanel* parentPanel, const String& label, RedGui::CRedGuiProgressBar*& progressBar );

		void NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime );
		void UpdateProgressBar( RedGui::CRedGuiProgressBar* progress, Int32 num, Int32 maxCount );

	private:

		RedGui::CRedGuiProgressBar*		m_usedTexturesProgressBar;
		RedGui::CRedGuiProgressBar*		m_usedBuffersProgressBar;
		RedGui::CRedGuiProgressBar*		m_usedSamplerStatesProgressBar;
		RedGui::CRedGuiProgressBar*		m_usedQueriesProgressBar;
		RedGui::CRedGuiProgressBar*		m_usedShadersProgressBar;
		RedGui::CRedGuiProgressBar*		m_usedVertexLayoutsProgressBar;
		RedGui::CRedGuiProgressBar*		m_usedSwapChainsProgressBar;

		RedGui::CRedGuiLabel*			m_shaderCacheCompressingDecompressingTimeLabel;
	};
}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
