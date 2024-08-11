/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/////////////////////////////////////////////////////////////////////////
#ifdef USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////

#if defined( USE_DX11 )
#	define SF_D3D_VERSION 11
#elif defined ( USE_DX10 )
#	define SF_D3D_VERSION 10
#else
#	error "Scaleform needs either DX10 or 11 at the moment"
#endif

#include "../../../external/gfx4/Src/Render/D3D1x/D3D1x_Config.h"
#include "../../../external/gfx4/Src/Render/D3D1x/D3D1x_Sync.h"

#ifdef RED_PLATFORM_DURANGO
#include "../../../external/gfx4/Src/Render/D3D1x/XboxOne_ShaderDescs.h"
#else
#include "../../../external/gfx4/Src/Render/D3D1x/D3D1x_ShaderDescs.h"
#endif

/////////////////////////////////////////////////////////////////////////
#endif // USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////