/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifndef _SECURE_SCL
#define _SECURE_SCL 0
#endif

#define _SCL_SECURE_NO_WARNINGS

#ifndef _HAS_EXCEPTIONS 
#define _HAS_EXCEPTIONS 0
#endif // !_HAS_EXCEPTIONS 

#include "utility.h"
#include "../../common/redSystem/crt.h"
#include "../../common/redSystem/error.h"
#include "../../common/redMath/redmathbase.h"
#include "../../common/redMath/numericalutils.h"
#include "../../common/redMemoryFramework/redMemoryFramework.h"
#include "../../common/redIO/redIO.h"
#include "../../common/gpuApiUtils/gpuApiInterface.h"
#include "../../common/gpuApiUtils/gpuApiMemory.h"

#define DEVICE_WIDTH 1280
#define DEVICE_HEIGHT 720

#if defined(RED_PLATFORM_WINPC)
	#define SHADERS_PATH TXT("../../dev/src/tests/unitTestsGpuApi")
#elif defined(RED_PLATFORM_ORBIS)
	#define SHADERS_PATH TXT("/app0")
#elif defined(RED_PLATFORM_DURANGO)
#define SHADERS_PATH TXT("")
#endif

#define LOG_GAT( format, ... ) RED_LOG( GAT, format, ##__VA_ARGS__ )
#define ERR_GAT( format, ... ) RED_LOG_ERROR( GAT, format, ##__VA_ARGS__ )
#define WARN_GAT( format, ... ) RED_LOG_WARNING( GAT, format, ##__VA_ARGS__ );

// Implement those for each render api
void SInitializePlatform( const CommandLineArguments & args );
int Run();
void SShutdownPlatform();