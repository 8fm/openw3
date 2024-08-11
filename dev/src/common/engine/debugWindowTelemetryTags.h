/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
#ifndef NO_TELEMETRY

#include "redGuiWindow.h"
#include "renderer.h"

namespace DebugWindows
{
	//! This debug window provide access to set telemetry session tags
	//! List of predefined tags is set in TelemetryServiceConfig.ini->Game->PredefinedTags
	//! as a string separated by spaces
	class CDebugWindoTelemetryTags : public RedGui::CRedGuiWindow
	{
	public:
		CDebugWindoTelemetryTags();
		~CDebugWindoTelemetryTags();

	private:
		void NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime );
		void NotifyCheckBoxValueChanged( RedGui::CRedGuiEventPackage& eventPackage, Bool value );
		void CreateControls();

	private:
		TDynArray<RedGui::CRedGuiCheckBox*> m_chackBoxTags;
	};

}	// namespace DebugWindows

#endif	// NO_TELEMETRY
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
