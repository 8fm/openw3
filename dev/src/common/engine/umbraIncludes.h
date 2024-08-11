#pragma once

#ifdef USE_UMBRA

	#define UMBRA_DIR "..\\..\\..\\external\\umbra3.3.13-change-26-2826-2\\"
	#include "..\\..\\..\\..\\external\\umbra3.3.13-change-26-2826-2\\interface\\optimizer\\umbraScene.hpp"
	#include "..\\..\\..\\..\\external\\umbra3.3.13-change-26-2826-2\\interface\\optimizer\\umbraTask.hpp"
	#include "..\\..\\..\\..\\external\\umbra3.3.13-change-26-2826-2\\interface\\optimizer\\\umbraBuilder.hpp"
	#include "..\\..\\..\\..\\external\\umbra3.3.13-change-26-2826-2\\interface\\runtime\\umbraTome.hpp"
	#include "..\\..\\..\\..\\external\\umbra3.3.13-change-26-2826-2\\interface\\runtime\\umbraQuery.hpp"

#if defined( RED_PLATFORM_WIN64 )
# define UMBRA_LIB_PATH			UMBRA_DIR "\\lib\\win64\\"
# define UMBRA_LIB_ARCH			"64"
# define UMBRA_LIB_EXTENSION	".lib"
# elif defined( RED_PLATFORM_WIN32 )
# define UMBRA_LIB_PATH			UMBRA_DIR "\\lib\\win32\\"
# define UMBRA_LIB_ARCH			"32"
# define UMBRA_LIB_EXTENSION	".lib"
# elif defined( RED_PLATFORM_DURANGO )
# define UMBRA_LIB_PATH			UMBRA_DIR "\\lib\\durango\\"
# define UMBRA_LIB_ARCH			""
# define UMBRA_LIB_EXTENSION	".lib"
#elif defined( RED_PLATFORM_ORBIS )
# define UMBRA_LIB_PATH			UMBRA_DIR "\\lib\\orbis\\"
# define UMBRA_LIB_ARCH			""
# define UMBRA_LIB_EXTENSION	".a"
#endif

#ifdef _DEBUG
# define UMBRA_LIB_SUFFIX "_d"
#else
# define UMBRA_LIB_SUFFIX ""
#endif

#if defined( RED_PLATFORM_WINPC ) || defined( RED_PLATFORM_DURANGO )
# pragma comment( lib, UMBRA_LIB_PATH "umbracommon" UMBRA_LIB_ARCH UMBRA_LIB_SUFFIX UMBRA_LIB_EXTENSION )
# pragma comment( lib, UMBRA_LIB_PATH "umbraruntime" UMBRA_LIB_ARCH UMBRA_LIB_SUFFIX UMBRA_LIB_EXTENSION )
#endif

#if defined( RED_PLATFORM_ORBIS )
# pragma comment( lib, UMBRA_LIB_PATH "libumbracommon" UMBRA_LIB_ARCH UMBRA_LIB_SUFFIX UMBRA_LIB_EXTENSION )
# pragma comment( lib, UMBRA_LIB_PATH "libumbraruntime" UMBRA_LIB_ARCH UMBRA_LIB_SUFFIX UMBRA_LIB_EXTENSION )
#endif

#if defined( RED_PLATFORM_WINPC ) && defined( USE_UMBRA_COOKING )
# pragma comment( lib, UMBRA_LIB_PATH "umbraoptimizer" UMBRA_LIB_ARCH UMBRA_LIB_SUFFIX UMBRA_LIB_EXTENSION )
#endif

#endif // USE_UMBRA
