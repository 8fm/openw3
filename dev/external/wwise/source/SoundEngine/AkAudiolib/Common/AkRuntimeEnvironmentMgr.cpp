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

#include "stdafx.h"
#include <AK/AkPlatforms.h>
#if (defined(AK_CPU_X86_64) || defined(AK_CPU_X86)) && !defined AK_APPLE && !defined AK_LINUX

#include "AkRuntimeEnvironmentMgr.h"
#include <AK/SoundEngine/Common/AkTypes.h>
#include "AkRuntimeDetector.h"

namespace AK
{
#define EDX_SSE_bit			0x2000000	// 25 bit
#define EDX_SSE2_bit		0x4000000	// 26 bit
#define ECX_SSE3_bit		0x1			// 0 bit
#define ECX_SSSE3_bit		0x200		// 9 bit

	AkRuntimeEnvironmentMgr * AkRuntimeEnvironmentMgr::Instance()
	{
		static AkRuntimeEnvironmentMgr g_RuntimeEnvMgr;
		return &g_RuntimeEnvMgr;
	}

	AkRuntimeEnvironmentMgr::AkRuntimeEnvironmentMgr()
	{
		int CPUFeatures[4];
		int CPUFeaturesExt[4];
		AkUInt32 uECX = 0;
		AkUInt32 uEDX = 0;

		AK::AkRunTimeDetector::CPUID(0,CPUFeatures);
		if( CPUFeatures[0] >= 1 )
		{
			AK::AkRunTimeDetector::CPUID(1,CPUFeatures);
			uECX = CPUFeatures[2];
			uEDX = CPUFeatures[3];
		}

		AK::AkRunTimeDetector::CPUID(0x80000000,CPUFeatures);
		unsigned int uExtIds = CPUFeatures[0];
		if( uExtIds >= 0x80000006 )
		{
			AK::AkRunTimeDetector::CPUID(0x80000006,CPUFeaturesExt);
			// Cache info
			ProcessorInfo.uCacheLineSize = CPUFeaturesExt[2] & 0xFF;
			ProcessorInfo.uL2CacheSize = (CPUFeaturesExt[2] >> 16) & 0xFFFF;
		}
		else
		{
			ProcessorInfo.uCacheLineSize = 64;
			ProcessorInfo.uL2CacheSize = 1024;
		}

		// SIMD support
		if( EDX_SSE_bit & uEDX )
			ProcessorInfo.uSIMDProcessorSupport |= AK_SIMD_SSE;
		if( EDX_SSE2_bit & uEDX )
			ProcessorInfo.uSIMDProcessorSupport |= AK_SIMD_SSE2;
		if( ECX_SSE3_bit & uECX )
			ProcessorInfo.uSIMDProcessorSupport |= AK_SIMD_SSE3;
		if( ECX_SSSE3_bit & uECX )
			ProcessorInfo.uSIMDProcessorSupport |= AK_SIMD_SSSE3;
	}

	bool AkRuntimeEnvironmentMgr::GetSIMDSupport(AkSIMDProcessorSupport in_eSIMD)
	{
		return (ProcessorInfo.uSIMDProcessorSupport & in_eSIMD) != 0;
	}

	AkUInt32 AkRuntimeEnvironmentMgr::GetCacheSize()
	{
		return ProcessorInfo.uL2CacheSize;
	}

	AkUInt32 AkRuntimeEnvironmentMgr::GetCacheLineSize()
	{
		return ProcessorInfo.uCacheLineSize;
	}
}

#endif
