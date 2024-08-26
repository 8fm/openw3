/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_OS_H_
#define _RED_OS_H_

#include "architecture.h"

#if defined( RED_PLATFORM_WIN32 ) || defined ( RED_PLATFORM_WIN64 )

#	define _HAS_EXCEPTIONS 0
#	define _WIN32_WINNT		_WIN32_WINNT_VISTA
#	define NTDDI_VERSION	NTDDI_VISTASP1

#	define NOATOM
//#	define NOMEMMGR		// Used in a couple of places
//#	define NOMENUS		// Used by output.cpp
//#	define NOMETAFILE	// Used by GDI+
#	define NOOPENFILE
#	define NOSERVICE
#	define NOHELP
//#	define NOTEXTMETRIC // Used by Direct X
#	define NOWH
#	define NOMCX

#	include <SdkDdkVer.h>
#	include <windows.h>
#elif defined( RED_PLATFORM_ORBIS )
#	include <kernel.h>
#	include <scebase.h>
#elif defined( RED_PLATFORM_DURANGO )
#	include <xdk.h>

#	define NOATOM
//#	define NOMEMMGR		// Used in a couple of places
//#	define NOMENUS		// Used by output.cpp
//#	define NOMETAFILE	// Used by GDI+
#	define NOOPENFILE
#	define NOSERVICE
#	define NOHELP
//#	define NOTEXTMETRIC // Used by Direct X
#	define NOWH
#	define NOMCX

#	include <Windows.h>
#elif defined( RED_PLATFORM_LINUX )
#	ifndef _GNU_SOURCE
#		define _GNU_SOURCE
#	endif
#	ifndef _LARGEFILE_SOURCE
#		define _LARGEFILE_SOURCE
#	endif
#	ifndef _LARGEFILE64_SOURCE
#		define _LARGEFILE64_SOURCE
#	endif
#	if defined(_FILE_OFFSET_BITS)
#		if _FILE_OFFSET_BITS != 64
#			error _FILE_OFFSET_BITS pre-defined to unsupported value!
#		endif
#	else
#		define _FILE_OFFSET_BITS 64
#	endif
#	include <fcntl.h>
#	include <unistd.h>
#	include <time.h>
#	include <sys/types.h>
#	include <sys/syscall.h>
#	include <sys/stat.h>
#	include <sys/time.h>
	// where should I actually put these?
#	include <cstdarg>
#	include <utility>
#	include <pthread.h>

#else
#	error Undefined Architecture
#endif

#endif // _RED_OS_H_
