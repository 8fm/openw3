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
#include <AK/SoundEngine/Common/IAkProcessorFeatures.h>

#if (defined(AK_CPU_X86_64) || defined(AK_CPU_X86)) && !defined AK_IOS

namespace AK
{
#if !defined (AK_APPLE)	&& !defined(AK_LINUX)
	struct AkProcessorSupportInfo
	{
		AkUInt32 uSIMDProcessorSupport;
		AkUInt32 uL2CacheSize;
		AkUInt32 uCacheLineSize;

		AkProcessorSupportInfo() 
		{
			uSIMDProcessorSupport = 0;
			uL2CacheSize = 0;
			uCacheLineSize = 0;
		}
	};
#endif
	
	class AkRuntimeEnvironmentMgr : public IAkProcessorFeatures
	{
	public:
		static AkRuntimeEnvironmentMgr * Instance();
		virtual ~AkRuntimeEnvironmentMgr(){};

		virtual bool GetSIMDSupport(AkSIMDProcessorSupport in_eSIMD);
		virtual AkUInt32 GetCacheSize();
		virtual AkUInt32 GetCacheLineSize();

	protected:
		AkRuntimeEnvironmentMgr();	
		
#if !defined (AK_APPLE) && !defined(AK_LINUX)
		AkProcessorSupportInfo ProcessorInfo;	
#endif		
	};
}

#endif // defined(AK_CPU_X86_64) || defined(AK_CPU_X86)
