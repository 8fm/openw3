/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "scaleformIncludes.h"

#include "scaleformConfig.h"

#ifdef USE_SCALEFORM

#if defined ( RED_PLATFORM_WINPC )
# ifdef RED_ARCH_X64
#	define SCALEFORM_ARCH "x64"
# else
#	define SCALEFORM_ARCH "Win32"
# endif
#elif defined ( RED_PLATFORM_DURANGO )
# define SCALEFORM_ARCH "XboxOne/XDK"
#elif defined( RED_PLATFORM_ORBIS )
# define SCALEFORM_ARCH "PS4"
#endif

#if defined ( RED_PLATFORM_WINPC ) || defined( RED_PLATFORM_DURANGO )
# define SCALEFORM_MSVC "MSvc11"
# define SCALEFORM_LIB_SUFFIX ".lib"
# define SCALEFORM_3RDPARTY_ZLIB_NAME "zlib"
# define SCALEFORM_3RDPARTY_EXPAT_NAME "expat"
# define SCALEFORM_3RDPARTY_PCRE_NAME "pcre"
#elif defined ( RED_PLATFORM_ORBIS )
# define SCALEFORM_MSVC "MSvc11"
# define SCALEFORM_LIB_SUFFIX ".a"
# define SCALEFORM_3RDPARTY_ZLIB_NAME "libz"
# define SCALEFORM_3RDPARTY_EXPAT_NAME "libexpat"
# define SCALEFORM_3RDPARTY_PCRE_NAME "libpcre"
#endif

#if defined ( RED_PLATFORM_WINPC )
# define SCALEFORM_3RD_PARTY_MSVC "MSvc11"
#elif defined( RED_PLATFORM_DURANGO )
# define SCALEFORM_3RD_PARTY_MSVC "MSvc11"
#elif defined( RED_PLATFORM_ORBIS )
# define SCALEFORM_3RD_PARTY_MSVC "MSvc11"
#endif

#if defined( _DEBUG )
#	define SCALEFORM_LIB_PATH			"../../../external/gfx4/Lib/" SCALEFORM_ARCH "/" SCALEFORM_MSVC "/Debug/"
#	define SCALEFORM_3RDPARTY_LIB_JPEG	"../../../external/gfx4/3rdParty/jpeg-8d/lib/" SCALEFORM_ARCH "/" SCALEFORM_3RD_PARTY_MSVC "/Debug/libjpeg" SCALEFORM_LIB_SUFFIX
#	define SCALEFORM_3RDPARTY_LIB_PNG	"../../../external/gfx4/3rdParty/libpng-1.5.13/Lib/" SCALEFORM_ARCH "/" SCALEFORM_3RD_PARTY_MSVC "/Debug/libpng" SCALEFORM_LIB_SUFFIX
#	define SCALEFORM_3RDPARTY_LIB_ZLIB	"../../../external/gfx4/3rdParty/zlib-1.2.7/Lib/" SCALEFORM_ARCH "/" SCALEFORM_3RD_PARTY_MSVC "/Debug/" SCALEFORM_3RDPARTY_ZLIB_NAME SCALEFORM_LIB_SUFFIX
#	define SCALEFORM_3RDPARTY_LIB_EXPAT	"../../../external/gfx4/3rdParty/expat-2.1.0/lib/" SCALEFORM_ARCH "/" SCALEFORM_3RD_PARTY_MSVC "/Debug/" SCALEFORM_3RDPARTY_EXPAT_NAME SCALEFORM_LIB_SUFFIX
#	define SCALEFORM_3RDPARTY_LIB_PCRE	"../../../external/gfx4/3rdParty/pcre/Lib/" SCALEFORM_ARCH "/" SCALEFORM_3RD_PARTY_MSVC "/Debug/" SCALEFORM_3RDPARTY_PCRE_NAME SCALEFORM_LIB_SUFFIX
#elif defined( RED_FINAL_BUILD )
#	define SCALEFORM_LIB_PATH			"../../../external/gfx4/Lib/" SCALEFORM_ARCH "/" SCALEFORM_MSVC "/Shipping/"
#	define SCALEFORM_3RDPARTY_LIB_JPEG	"../../../external/gfx4/3rdParty/jpeg-8d/lib/" SCALEFORM_ARCH "/" SCALEFORM_3RD_PARTY_MSVC "/Release/libjpeg" SCALEFORM_LIB_SUFFIX
#	define SCALEFORM_3RDPARTY_LIB_PNG	"../../../external/gfx4/3rdParty/libpng-1.5.13/Lib/" SCALEFORM_ARCH "/" SCALEFORM_3RD_PARTY_MSVC "/Release/libpng" SCALEFORM_LIB_SUFFIX
#	define SCALEFORM_3RDPARTY_LIB_ZLIB	"../../../external/gfx4/3rdParty/zlib-1.2.7/Lib/" SCALEFORM_ARCH "/" SCALEFORM_3RD_PARTY_MSVC "/Release/" SCALEFORM_3RDPARTY_ZLIB_NAME SCALEFORM_LIB_SUFFIX
#	define SCALEFORM_3RDPARTY_LIB_EXPAT	"../../../external/gfx4/3rdParty/expat-2.1.0/lib/" SCALEFORM_ARCH "/" SCALEFORM_3RD_PARTY_MSVC "/Release/" SCALEFORM_3RDPARTY_EXPAT_NAME SCALEFORM_LIB_SUFFIX
#	define SCALEFORM_3RDPARTY_LIB_PCRE	"../../../external/gfx4/3rdParty/pcre/Lib/" SCALEFORM_ARCH "/" SCALEFORM_3RD_PARTY_MSVC "/Release/" SCALEFORM_3RDPARTY_PCRE_NAME SCALEFORM_LIB_SUFFIX
#else
#	ifdef SF_BUILD_DEBUGOPT
#		define SCALEFORM_VER_DIR "/DebugOpt/"
#	else
#		define SCALEFORM_VER_DIR "/Release/"
#	endif
#	define SCALEFORM_LIB_PATH			"../../../external/gfx4/Lib/" SCALEFORM_ARCH "/" SCALEFORM_MSVC SCALEFORM_VER_DIR
#	define SCALEFORM_3RDPARTY_LIB_JPEG	"../../../external/gfx4/3rdParty/jpeg-8d/lib/" SCALEFORM_ARCH "/" SCALEFORM_3RD_PARTY_MSVC "/Release/libjpeg" SCALEFORM_LIB_SUFFIX
#	define SCALEFORM_3RDPARTY_LIB_PNG	"../../../external/gfx4/3rdParty/libpng-1.5.13/Lib/" SCALEFORM_ARCH "/" SCALEFORM_3RD_PARTY_MSVC "/Release/libpng" SCALEFORM_LIB_SUFFIX
#	define SCALEFORM_3RDPARTY_LIB_ZLIB	"../../../external/gfx4/3rdParty/zlib-1.2.7/Lib/" SCALEFORM_ARCH "/" SCALEFORM_3RD_PARTY_MSVC "/Release/" SCALEFORM_3RDPARTY_ZLIB_NAME SCALEFORM_LIB_SUFFIX
#	define SCALEFORM_3RDPARTY_LIB_EXPAT	"../../../external/gfx4/3rdParty/expat-2.1.0/lib/" SCALEFORM_ARCH "/" SCALEFORM_3RD_PARTY_MSVC "/Release/" SCALEFORM_3RDPARTY_EXPAT_NAME SCALEFORM_LIB_SUFFIX
#	define SCALEFORM_3RDPARTY_LIB_PCRE	"../../../external/gfx4/3rdParty/pcre/Lib/" SCALEFORM_ARCH "/" SCALEFORM_3RD_PARTY_MSVC "/Release/" SCALEFORM_3RDPARTY_PCRE_NAME SCALEFORM_LIB_SUFFIX
#endif

#pragma comment( lib, SCALEFORM_LIB_PATH "libgfx" SCALEFORM_LIB_SUFFIX )

#pragma comment( lib, SCALEFORM_3RDPARTY_LIB_ZLIB )
#pragma comment( lib, SCALEFORM_3RDPARTY_LIB_JPEG )
#pragma comment( lib, SCALEFORM_3RDPARTY_LIB_PNG )
#pragma comment( lib, SCALEFORM_3RDPARTY_LIB_EXPAT )
#pragma comment( lib, SCALEFORM_3RDPARTY_LIB_PCRE )

#if WITH_SCALEFORM_AS3
# pragma comment( lib, SCALEFORM_LIB_PATH "libgfx_as3" SCALEFORM_LIB_SUFFIX)
#endif

#if WITH_SCALEFORM_AS2
# pragma comment( lib, SCALEFORM_LIB_PATH "libgfx_as2" SCALEFORM_LIB_SUFFIX )
#endif

#if WITH_SCALEFORM_VIDEO
# ifdef RED_PLATFORM_DURANGO
# pragma comment( lib, "xaudio2.lib" )
# endif
# pragma comment( lib, SCALEFORM_LIB_PATH "libgfxvideo" SCALEFORM_LIB_SUFFIX )
#endif

//#ifndef RED_FINAL_BUILD
#pragma comment( lib, SCALEFORM_LIB_PATH "libgfx_air" SCALEFORM_LIB_SUFFIX )
//#else
//# error Recompile Scaleform to exclude AIR already!
//#endif


#endif // USE_SCALEFORM
