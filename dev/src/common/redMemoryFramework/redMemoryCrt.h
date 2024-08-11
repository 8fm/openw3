/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_FRAMEWORK_CRT_H
#define _RED_MEMORY_FRAMEWORK_CRT_H
#pragma once

#include "redMemoryFrameworkPlatform.h"

/////////////////////////////////////////////////////////////////
// This should just define implementations for:
// AlignedMalloc, AlignedFree, AlignedRealloc

#ifdef RED_MEMORY_FRAMEWORK_PLATFORM_WINDOWS_API
	#include "redMemoryCrtWinAPI.inl"
#elif defined( RED_MEMORY_FRAMEWORK_PLATFORM_ORBIS_API )
	#include "redMemoryCrtOrbis.inl"
#elif defined( RED_MEMORY_FRAMEWORK_PLATFORM_DURANGO_API )
	#include "redMemoryCrtDurango.inl"
#endif

#endif // RED_MEMORY_FRAMEWORK_CRT_H