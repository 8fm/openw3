/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

/////////////////////////////////////////////////////////////////////////
#ifdef USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////

#if defined( RED_PLATFORM_WINPC ) || defined( RED_PLATFORM_DURANGO )

// TBD: Included here again because hackishly, we don't include the .h here
#include "renderScaleformIncludes.h"

#if defined( USE_DX11 )
#	define SF_D3D_VERSION 11
#elif defined ( USE_DX10 )
#	define SF_D3D_VERSION 10
#else
#	error "Scaleform needs either DX10 or 11 at the moment"
#endif

#if defined( RED_PLATFORM_WINPC )
#include "../../../external/gfx4/Src/Render/D3D1x/D3D1x_ShaderDescs.cpp"
#include "../../../external/gfx4/Src/Render/D3D1x/D3D1x_ShaderBinary.cpp"
#elif defined( RED_PLATFORM_DURANGO )
#include "../../../external/gfx4/Src/Render/D3D1x/XboxOne_ShaderDescs.cpp"
#include "../../../external/gfx4/Src/Render/D3D1x/XboxOne_ShaderBinary.cpp"
#else
#error Unsupported platform
#endif

#endif // #if defined( RED_PLATFORM_WINPC ) || defined( RED_PLATFORM_DURANGO )

/////////////////////////////////////////////////////////////////////////
#endif // USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////
