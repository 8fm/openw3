/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/

#pragma once

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/AkWwiseSDKVersion.h>

//This version number will appear readable in hexadecimal.
//For example, 2009.2 will be 0x20090200
#define AK_COMM_PROTOCOL_VERSION ((AK_WWISESDK_VERSION_MAJOR / 1000 << 28) | \
	(AK_WWISESDK_VERSION_MAJOR % 1000 / 100 << 24) | \
	(AK_WWISESDK_VERSION_MAJOR % 100 / 10 << 20) | \
	(AK_WWISESDK_VERSION_MAJOR % 10 << 16) | \
	(AK_WWISESDK_VERSION_MINOR << 8))

namespace CommunicationDefines
{
	enum ConsoleType
	{
		ConsoleUnknown,
		ConsoleWindows,
        ConsoleXBox360,
		ConsolePS3,
		ConsoleWii,
		ConsoleMac,
		ConsoleVitaSW,
		ConsoleVitaHW,
		ConsolePS4,
		ConsoleiOS,
		Console3DS,
		ConsoleWiiUSW,
		ConsoleWiiUHW,
		ConsoleAndroid,
		ConsoleXboxOne,
		ConsoleLinux
	};

#if defined( AK_WIN )
	static ConsoleType g_eConsoleType = ConsoleWindows;
#elif defined( AK_XBOX360 )
	static ConsoleType g_eConsoleType = ConsoleXBox360;
#elif defined( AK_PS3 )
	static ConsoleType g_eConsoleType = ConsolePS3;
#elif defined( AK_WII )
	static ConsoleType g_eConsoleType = ConsoleWii;
#elif defined( AK_MAC_OS_X )
	static ConsoleType g_eConsoleType __attribute__ ((__unused__)) = ConsoleMac;
#elif defined( AK_IOS )
	static ConsoleType g_eConsoleType __attribute__ ((__unused__)) = ConsoleiOS;
#elif defined( AK_VITA_HW )
	static ConsoleType g_eConsoleType = ConsoleVitaHW;
#elif defined( AK_VITA )
	static ConsoleType g_eConsoleType = ConsoleVitaSW;
#elif defined( AK_PS4 )
	static ConsoleType g_eConsoleType = ConsolePS4;
#elif defined( AK_3DS )
	static ConsoleType g_eConsoleType __attribute__ ((__unused__)) = Console3DS;
#elif defined( AK_WIIU_SOFTWARE )
	static ConsoleType g_eConsoleType = ConsoleWiiUSW;
#elif defined( AK_WIIU )
	static ConsoleType g_eConsoleType = ConsoleWiiUHW;
#elif defined( AK_ANDROID )
	static ConsoleType g_eConsoleType __attribute__ ((__unused__)) = ConsoleAndroid;
#elif defined( AK_XBOXONE )
	static ConsoleType g_eConsoleType = ConsoleXboxOne;
#elif defined( AK_NACL )
	static ConsoleType g_eConsoleType = ConsoleUnknown;
#elif defined( AK_LINUX )
	static ConsoleType g_eConsoleType __attribute__ ((__unused__)) = ConsoleLinux;
#else
	#error CommunicationDefines.h : Console is undefined
#endif

	inline bool NeedEndianSwap(ConsoleType in_eType)
	{
		return in_eType == ConsoleXBox360
			|| in_eType == ConsolePS3
			|| in_eType == ConsoleWii
			|| in_eType == ConsoleWiiUSW
			|| in_eType == ConsoleWiiUHW;
	}
	
	const unsigned int kProfilingSessionVersion = 1;	//First version

#pragma pack(push, 1)
	struct ProfilingSessionHeader
	{
		AkUInt32 uVersion;
		AkUInt32 uProtocol;
		AkUInt32 uConsoleType;
	};
#pragma pack(pop)
}
