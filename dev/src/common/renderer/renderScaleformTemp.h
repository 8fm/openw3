/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#if defined( RED_PLATFORM_WINPC ) || defined( RED_PLATFORM_DURANGO )
# include "renderScaleformTempD3D11.h"
#elif defined( RED_PLATFORM_ORBIS )
# include "renderScaleformTempOrbis.h"
#else
# error Unsupported platform!
#endif
