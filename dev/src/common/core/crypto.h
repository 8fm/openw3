/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../redSystem/architecture.h"

#ifdef RED_PLATFORM_WINPC
# define RED_USE_CRYPTO
#endif

#ifdef RED_USE_CRYPTO

#ifdef RED_PLATFORM_WINPC
# if _MSC_VER == 1700
#  define LIB_SUBFOLDER ""
# else
#  define LIB_SUBFOLDER "vs2022/"
# endif

# ifdef _DEBUG
#  pragma comment( lib, "external/cryptopp562/" LIB_SUBFOLDER "x64/Output/Debug/cryptlib.lib")
# else
#   pragma comment( lib, "external/cryptopp562/" LIB_SUBFOLDER "x64/Output/Release/cryptlib.lib")
# endif
#endif // RED_PLATFORM_WINPC

#endif // RED_USE_CRYPTO
