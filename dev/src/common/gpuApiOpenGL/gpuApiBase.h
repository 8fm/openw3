/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../redSystem/architecture.h"

#if defined( RED_PLATFORM_WINPC )
#pragma comment (lib,  "opengl32.lib")

#include "../../../external/glew/include/GL/glew.h"
#include "../../../external/glew/include/GL/wglew.h"
#pragma comment (lib,  "../../../external/glew/lib/Release/x64/glew32.lib")
#endif