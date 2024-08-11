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
#else
#	error Undefined Architecture
#endif

#endif // _RED_OS_H_
