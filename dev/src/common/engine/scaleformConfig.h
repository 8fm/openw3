/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#ifdef USE_SCALEFORM

// WWise audio support
# if 0//defined(USE_WWISE) && ( defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO ) || defined( RED_PLATFORM_ORBIS ) )
#  define WITH_SCALEFORM_WWISE 1
# else
#  define WITH_SCALEFORM_WWISE 0
# endif

// XAudio2 audio support
# if defined( RED_PLATFORM_WINPC ) || defined( RED_PLATFORM_DURANGO )
#  define WITH_SCALEFORM_XA2 1
# else
#  define WITH_SCALEFORM_XA2 0
# endif

# if defined( RED_PLATFORM_ORBIS )
# define WITH_SCALEFORM_AUDIOOUT 1
# else
# define WITH_SCALEFORM_AUDIOOUT 0
# endif

// ActionScript support
# define WITH_SCALEFORM_AS2 0
# define WITH_SCALEFORM_AS3 1

// Scaleform Video support
# define WITH_SCALEFORM_VIDEO 1

#else

# define WITH_SCALEFORM_WWISE 0
# define WITH_SCALEFORM_XA2 0
# define WITH_SCALEFORM_AUDIOOUT 0
# define WITH_SCALEFORM_AS2 0
# define WITH_SCALEFORM_AS3 0
# define WITH_SCALEFORM_VIDEO 0

#endif // !USE_SCALEFORM