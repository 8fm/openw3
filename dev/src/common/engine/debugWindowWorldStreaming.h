/*
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "redGuiWindow.h"

namespace RedGui
{
	class CRedGuiComboBox;
	class CRedGuiCheckBox;
}

namespace DebugWindows
{
	class CDebugWindowWorldStreaming : public RedGui::CRedGuiWindow
	{
	public:
		CDebugWindowWorldStreaming();
		~CDebugWindowWorldStreaming();

	private:
		void OnWindowOpened( CRedGuiControl* control );
		void NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime );
		void NotifyEventGridLevelSelected( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedIndex );
		void NotifyEventOptionChanged( RedGui::CRedGuiEventPackage& eventPackage, Bool option );

		class CDebugWindowWSGrid*	m_custom;
		RedGui::CRedGuiComboBox*	m_gridLevelBox;
		RedGui::CRedGuiCheckBox*	m_gridShowElements;
		RedGui::CRedGuiCheckBox*	m_gridShowHistogram;
		RedGui::CRedGuiCheckBox*	m_gridShowCounts;
	};
}

#endif
#endif
