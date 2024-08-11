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

#if defined( AK_PS4 ) 
#include <cpuid.h>
#else
#include <intrin.h>
#endif

namespace AK
{
	namespace AkRunTimeDetector
	{
		static void CPUID(AkUInt32 in_uOpcode, int out_uCPUFeatures[4])
		{
#if defined( AK_PS4 ) 
			unsigned int eax = out_uCPUFeatures[0], 
				ebx = out_uCPUFeatures[1], 
				ecx = out_uCPUFeatures[2], 
				edx = out_uCPUFeatures[3];

			__get_cpuid( in_uOpcode, &eax, &ebx, &ecx, &edx );
			out_uCPUFeatures[0] = eax;
			out_uCPUFeatures[1] = ebx;
			out_uCPUFeatures[2] = ecx;
			out_uCPUFeatures[3] = edx;
#else
			__cpuid( out_uCPUFeatures, in_uOpcode );
#endif

			
		}
	}
}
