/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "redGuiWindow.h"

namespace DebugWindows
{
	class CDebugWindowProfilerSettings : public RedGui::CRedGuiWindow
	{
	public:
		CDebugWindowProfilerSettings();
		~CDebugWindowProfilerSettings();

	private:
		void CreateControls();
		void NotifyApplyClicked( RedGui::CRedGuiEventPackage& eventPackage );
		Uint32 GetActiveChannelsFromCheckBoxes();
		void NotifyChannelAllCheckChanged( RedGui::CRedGuiEventPackage& eventPackage, Bool value );
		void NotifyWindowOpened( RedGui::CRedGuiEventPackage& eventPackage );

	private:
		RedGui::CRedGuiSpin*					m_levelSpinBox;
		TDynArray< RedGui::CRedGuiCheckBox* >	m_profilersCheckBoxes;
		TDynArray< RedGui::CRedGuiCheckBox* >	m_channelsCheckBoxes;
	};

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
