/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
#ifdef USE_ARRAY_METRICS

#include "redGuiWindow.h"

namespace DebugWindows
{
	class CDebugWindowArrayMemoryMetrics : public RedGui::CRedGuiWindow
	{
	public:
		CDebugWindowArrayMemoryMetrics();
		~CDebugWindowArrayMemoryMetrics();

		void NotifyOnClickedSummaryToLog( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyOnClickedSummaryToFile( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyOnClickedDetailsToLog( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyOnClickedDetailsToFile( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyDumpSummaryFileOK( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyDumpDetailsFileOK( RedGui::CRedGuiEventPackage& eventPackage );

	private:
		void CreateControls();

		RedGui::CRedGuiSaveFileDialog*	m_dumpSummaryDialog;
		RedGui::CRedGuiSaveFileDialog*	m_dumpDetailsDialog;
	};
}

#endif	//NO_RED_GUI
#endif	//NO_DEBUG_WINDOWS
#endif  //USE_ARRAY_METRICS