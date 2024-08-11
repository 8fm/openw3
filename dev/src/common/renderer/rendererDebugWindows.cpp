/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "../engine/debugWindowsManager.h"
#include "debugWindowDynamicTextures.h"
#include "debugWindowRenderResources.h"
#include "debugWindowGpuResourceUse.h"
#include "debugWindowTextureStreaming.h"

void RegisterRendererDebugWindows()
{
	DebugWindows::CDebugWindowsManager& debugManager = GDebugWin::GetInstance();
	debugManager.RegisterWindow( new DebugWindows::CDebugWindowDynamicTextures()	, DebugWindows::DW_DynamicTextures );
	debugManager.RegisterWindow( new DebugWindows::CDebugWindowdRenderResources()	, DebugWindows::DW_RenderResources );
	debugManager.RegisterWindow( new DebugWindows::CDebugWindowGpuResourceUse()		, DebugWindows::DW_GpuResourceUse );
	debugManager.RegisterWindow( new DebugWindows::CDebugWindowTextureStreaming()	, DebugWindows::DW_TextureStreaming );
}

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
