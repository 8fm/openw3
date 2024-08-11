/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_NETWORK_H_
#define _RED_NETWORK_H_

#include "../redSystem/log.h"

#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )
#	include "platform/platformWindows.h"
#elif defined( RED_PLATFORM_ORBIS )
#	include "platform/platformOrbis.h"
#else
#	error No red network implementation for current platform
#endif


#endif //_RED_NETWORK_H_
