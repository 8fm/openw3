/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "../engine/debugWindowsManager.h"
#include "debugWindowNPCViewer.h"
#include "debugWindowVehicleViewer.h"
#include "debugWindowGameWorld.h"
#include "debugWindowBoatSettings.h"

void RegisterGameDebugWindows()
{
	DebugWindows::CDebugWindowsManager & debugManager = GDebugWin::GetInstance();
	debugManager.RegisterWindow( new DebugWindows::CDebugWindowNPCViewer(), DebugWindows::DW_NPCViewer );
	debugManager.RegisterWindow( new DebugWindows::CDebugWindowVehicleViewer(), DebugWindows::DW_VehicleViewer );
#ifndef FINAL
    debugManager.RegisterWindow( new DebugWindows::CDebugWindowBoatSettings(), DebugWindows::DW_BoatSettings );
#endif
	debugManager.RegisterWindow( new DebugWindows::CDebugWindowGameWorld(), DebugWindows::DW_GameWorld );
}

#endif
#endif
