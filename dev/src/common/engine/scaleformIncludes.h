/**
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */
#pragma once

//////////////////////////////////////////////////////////////////////////
#ifdef USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////

// In release or no-opts, use DebugOpt vs just Release
// #if !defined( _DEBUG ) && !defined( RED_FINAL_BUILD)
// # ifndef _DEBUGOPT
// #  define _DEBUGOPT
// # endif
// #endif


#ifdef RED_COMPILER_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-register"
#endif

// TBD: SF_BUILD_DEBUGOPT
#ifdef RED_FINAL_BUILD
# define SF_BUILD_SHIPPING
#endif

#ifdef RED_PLATFORM_DURANGO
# define SF_OS_WINMETRO
#endif

#include <GFxConfig.h>

#include "scaleformHacks.h"
#include "scaleformTypes.h"

#ifdef RED_PLATFORM_DURANGO
# include <eh.h> // SF_Threads.h includes <ppl.h>, which uses __uncaught_exception in concrt.h
#endif

#include <GFx_Kernel.h>
#include <GFx.h>
#undef snprintf // hkString::sprintf otherwise broken by GFx.h -> AS2/AS2_Action.h: #define snprintf _snprintf

namespace SF = Scaleform;
namespace GFx = SF::GFx;

#include "scaleformUtils.inl"

#ifdef RED_COMPILER_CLANG
#pragma clang diagnostic pop
#endif

//////////////////////////////////////////////////////////////////////////
#endif // USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////

