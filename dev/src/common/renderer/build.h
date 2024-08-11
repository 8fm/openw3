/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "..\..\common\core\core.h"
#include "..\..\common\engine\engine.h"
#include "..\..\common\matcompiler\matcompiler.h"

#include "rendererNamesRegistry.h"

// GpuAPI utils
#include "../gpuApiUtils/gpuApiUtils.h"

#ifndef RED_PLATFORM_CONSOLE
#define USE_GPUAPI_HACKS
#endif

#ifdef USE_GPUAPI_HACKS
#include "../gpuApiDX10/gpuApiBase.h"
#endif //USE_GPUAPI_HACKS

#ifndef RED_PLATFORM_ORBIS
#	define USE_DX10
#	define USE_DX11
#endif

#define LOG_RENDERER( format, ... ) RED_LOG( Renderer, format, ##__VA_ARGS__ )
#define ERR_RENDERER( format, ... ) RED_LOG_ERROR( Renderer, format, ##__VA_ARGS__ )
#define WARN_RENDERER( format, ... ) RED_LOG_WARNING( Renderer, format, ##__VA_ARGS__ );

// Renderer
#include "renderStateManager.h"
#include "renderDrawer.h"

#include "renderInterface.h"

#include "renderCounter.h"
#include "renderDebugMacros.h"

#include "renderDynamicResource.h"
#include "renderStaticResource.h"