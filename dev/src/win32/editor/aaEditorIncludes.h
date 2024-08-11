
#pragma once

#include "../../../external/aaLib/dev/include/Packet.h"
#include "../../../external/aaLib/dev/include/ClientServer.h"

#if defined ( _DEBUG )
#if defined( RED_PLATFORM_WIN32 )
#pragma comment(lib, "../../../external/aaLib/dev/lib/Debug/win32/aaLib_net.lib" )
#elif defined( RED_PLATFORM_WIN64 )
#pragma comment(lib, "../../../external/aaLib/dev/lib/Debug/x64/aaLib_net.lib" )
#endif
#else
#if defined( RED_PLATFORM_WIN32 )
#pragma comment(lib, "../../../external/aaLib/dev/lib/Release/win32/aaLib_net.lib" )
#elif defined( RED_PLATFORM_WIN64 )
#pragma comment(lib, "../../../external/aaLib/dev/lib/Release/x64/aaLib_net.lib" )
#endif
#endif
