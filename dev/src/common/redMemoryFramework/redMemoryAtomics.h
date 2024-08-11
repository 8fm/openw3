/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_ATOMICS_H_INCLUDED
#define _RED_MEMORY_ATOMICS_H_INCLUDED
#pragma once

#include "redMemoryFrameworkPlatform.h"

#ifdef RED_MEMORY_FRAMEWORK_PLATFORM_WINDOWS_API
#include "redMemoryAtomicsWinAPI.inl"
#elif defined( RED_MEMORY_FRAMEWORK_PLATFORM_ORBIS_API )
#include "redMemoryAtomicsOrbis.inl"
#elif defined( RED_MEMORY_FRAMEWORK_PLATFORM_DURANGO_API )
#include "redMemoryAtomicsDurango.inl"
#endif

#endif