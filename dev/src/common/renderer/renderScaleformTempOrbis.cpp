/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

/////////////////////////////////////////////////////////////////////////
#ifdef USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////

#ifdef RED_PLATFORM_ORBIS

// TBD: Included here again because hackishly, we don't include the .h here
#include "renderScaleformIncludes.h"

RED_WARNING_PUSH()
RED_DISABLE_WARNING_CLANG( "-Wc++11-narrowing" )
#include "../../../external/gfx4/Src/Render/PS4/PS4_ShaderDescs.cpp"
#pragma comment( lib, "../../../external/gfx4/Lib/PS4/Release_LCUE/libgfxshaders.a" )
RED_WARNING_POP()

#endif // #ifdef RED_PLATFORM_ORBIS

/////////////////////////////////////////////////////////////////////////
#endif // USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////
