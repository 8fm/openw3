/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_SYSTEM_CRASH_REPORTER_H_
#define _RED_SYSTEM_CRASH_REPORTER_H_
#pragma once

#if defined( RED_PLATFORM_DURANGO )
	#include "crashReportDataBufferDurango.h"
#elif defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 )
	#include "crashReportDataBufferWindows.h"
#elif defined( RED_PLATFORM_ORBIS )
	#include "crashReportDataBufferOrbis.h"
#endif

#endif