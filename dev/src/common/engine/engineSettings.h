/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _ENGINE_SETTINGS_H_
#define _ENGINE_SETTINGS_H_

#include "../redSystem/os.h"
#include "../redSystem/compilerExtensions.h"

// temporarily moved here to make sure that the engine still compiles
#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )
#	define W2_PLATFORM_WINBOX

#	ifndef W2_PLATFORM_WIN32
#		define W2_PLATFORM_WIN32
#	endif
#endif

//////////////////////////////////////////////////////
// Middleware usage defines

#define USE_PHYSX
#define USE_APEX
#define USE_SPEED_TREE
#define USE_SCALEFORM
#define USE_UMBRA
#define USE_MSSSAO
#define USE_WWISE

#ifdef RED_PLATFORM_WIN64
	#define USE_ANSEL
#endif // RED_PLATFORM_WIN64

#ifndef RED_PLATFORM_CONSOLE
#	define USE_PHYSX_GPU
//#	define USE_BINK_VIDEO
#	define USE_NVIDIA_FUR
#	define USE_NVSSAO
#ifndef NO_EDITOR
#	define USE_SIMPLYGON
#endif
#endif

#ifdef RED_PLATFORM_CONSOLE
	#define TEXTURE_MEMORY_DEFRAG_ENABLED
#endif

/*
FIXME
For Wwise because xinput is linked from Win32 platform project and causes linker errors in so many other projects (wcc, unit tests, etc)

Build errors: 30>AkRumble.lib(AkRumbleControllerXInput.obj) : error LNK2019: unresolved external symbol XInputSetState referenced in function
"public: virtual enum AKRESULT __cdecl AkRumbleControllerXInput::SetRumble(float,float)" (?SetRumble@AkRumbleControllerXInput@@UEAA?AW4AKRESULT@@MM@Z)
*/
#ifdef RED_PLATFORM_WINPC
# ifdef RED_ARCH_X64
#  define INPUT_UT_ARCH "x64"
# else
#  define INPUT_UT_ARCH "x86"
# endif
# define INPUT_UT_DXSDK_LIB_PATH "../../../external/dxsdk(June2010)/lib/" INPUT_UT_ARCH "/"
# pragma comment ( lib, INPUT_UT_DXSDK_LIB_PATH "xinput.lib" )
#endif

#if defined( RED_PLATFORM_WINPC ) && !defined( RED_FINAL_BUILD )
# define USE_UMBRA_COOKING
#endif

//////////////////////////////////////////////////////
// Engine feature usage defines defines

//Temporary warning disables
RED_DISABLE_WARNING_MSC( 4100 )	// 'identifier' : unreferenced formal parameter
RED_DISABLE_WARNING_MSC( 4189 )	// 'identifier' : local variable is initialized but not referenced
RED_DISABLE_WARNING_MSC( 4714 )	// 'function': marked as __forceinline not inlined

	// This can be removed soon!
#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 )// || defined( RED_PLATFORM_DURANGO )
# include "../../win32/platform/os.h"
#endif

#define LOG_ENGINE( format, ... ) RED_LOG( Engine, format, ##__VA_ARGS__ )
#define ERR_ENGINE( format, ... ) RED_LOG_ERROR( Engine, format, ##__VA_ARGS__ )
#define WARN_ENGINE( format, ... ) RED_LOG_WARNING( Engine, format, ##__VA_ARGS__ );

	// World streaming throttle spreads the layer loading over multiple frames
	// Note, this causes problems with quests / scripts that required layers to be loaded in a timely fashion
	//#define USE_WORLD_STREAMING_THROTTLE	// Limits the maximum number of layers to be processed per-frame by the world streaming

#define COMPILE_ASSERT( expression ) RED_STATIC_ASSERT( expression )


#endif
