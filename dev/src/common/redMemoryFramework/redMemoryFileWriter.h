/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_FILE_WRITER_H
#define _RED_MEMORY_FILE_WRITER_H
#pragma once

////////////////////////////////////////////////////////////////
// Include platform-specific implementation
#ifdef RED_MEMORY_FRAMEWORK_PLATFORM_WINDOWS_API
#include "redMemoryFileWriterWinAPI.h"
#elif defined( RED_MEMORY_FRAMEWORK_PLATFORM_ORBIS_API )
#include "redMemoryFileWriterOrbis.h"
#elif defined( RED_MEMORY_FRAMEWORK_PLATFORM_DURANGO_API )
#include "redMemoryFileWriterDurango.h"
#endif

#endif