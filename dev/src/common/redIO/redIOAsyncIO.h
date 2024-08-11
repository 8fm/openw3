/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "redIOCommon.h"

//#define RED_USE_NATIVE_PROACTOR

#if defined( RED_PLATFORM_ORBIS ) && defined( RED_USE_NATIVE_PROACTOR )
# include "redIOProactorOrbisAPI.h"
#else
# include "redIOProactorGenericAPI.h"
#endif

REDIO_NAMESPACE_BEGIN

#if defined( RED_PLATFORM_ORBIS ) && defined( RED_USE_NATIVE_PROACTOR )
 	typedef OSAPI::CIOProactor CAsyncIO;
#else
	typedef GenericAPI::CIOProactor CAsyncIO;
#endif

extern CAsyncIO GAsyncIO;

//////////////////////////////////////////////////////////////////////////
// For now see redIOProactorGenericAPI.h and redIOCommon.h for the interface.
// Sorry for the inconvenience.
//////////////////////////////////////////////////////////////////////////

REDIO_NAMESPACE_END
