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

// Generates uniform probability white noise using linear congruential generator algorithm

#include "AkWhiteNoise.h"
#include <AK/Tools/Common/AkAssert.h>

#ifndef __SPU__
#include "../../AkAudiolib/Common/AkRandom.h"
#endif

#ifdef AK_XBOX360
#include <AK/SoundEngine/Common/AkSimd.h>
//#define USE_PREFETCH // Does make PIX happy but not any faster
#endif

namespace DSP
{
#ifndef __SPU__
	CAkWhiteNoise::CAkWhiteNoise()
	{
		m_uLCGState = (AkUInt32)AKRANDOM::AkRandom();
	}
#endif // __SPU__

	void CAkWhiteNoise::GenerateBuffer(	
		AkReal32 * AK_RESTRICT out_pfBuffer, 
		AkUInt32 in_uNumFrames )
	{
#ifdef AK_XBOX360 
		// Reduce LHS by a factor of 4 doing int to float conversions in VMX
		// Int-To-VMX LHS present (4x less than int-float cast) 
		// Tried unrolling to group LHS with no success
		// Tried using entirely different algorithm for NoiseGen (XorShift) which had no PIX bottlenecks but many more instructions
		// http://www.jstatsoft.org/v08/i14/paper

		AKASSERT( in_uNumFrames % 4 == 0 );	
		const __vector4 vNoiseGain = {AKNOISE_GAIN,AKNOISE_GAIN,AKNOISE_GAIN,AKNOISE_GAIN};
		__vector4 * AK_RESTRICT pvBuf = (__vector4 * AK_RESTRICT) out_pfBuffer;
		const __vector4 * pvEnd = (__vector4 *) ( out_pfBuffer + in_uNumFrames );
		__declspec(align(16)) AkUInt32 uState[4];
		uState[3] = m_uLCGState;
#ifdef USE_PREFETCH
		AKSIMD_PREFETCHMEMORY(0,pvBuf);
#endif
		while ( pvBuf < pvEnd )
		{
#ifdef USE_PREFETCH
			AKSIMD_PREFETCHMEMORY(AKSIMD_ARCHCACHELINESIZE,pvBuf); // Prefetch 1 cache lin ahead
#endif
			// No integer multiply on XBox360
			uState[0] = (uState[3] * AKNOISE_LCG_RANDSCALE) + AKNOISE_LCG_RANDOFFSET;
			uState[1] = (uState[0]* AKNOISE_LCG_RANDSCALE) + AKNOISE_LCG_RANDOFFSET;
			uState[2] = (uState[1] * AKNOISE_LCG_RANDSCALE) + AKNOISE_LCG_RANDOFFSET;
			uState[3] = (uState[2] * AKNOISE_LCG_RANDSCALE) + AKNOISE_LCG_RANDOFFSET;

			// AkInt32 iRandVal = ((AkInt32) m_uLCGState);
			// AkReal32 fRandVal = iRandVal * ONEOVERMAXRANDVAL;
			// *pfBuf++ = fRandVal;

			__vector4 vRand = __lvx( uState, 0); // Grouped LHS		
			__vector4 vNoiseOut = __vcfsx( vRand, 31 );
			vNoiseOut = vNoiseOut * vNoiseGain;
			__stvx(vNoiseOut, pvBuf, 0);
			++pvBuf;
		}
		m_uLCGState = uState[3];
// TODO: Vectorize this for PS3 is a low hanging fruit
#else // XBOX360

		// PC vectorized version of this was slower
		// http://software.intel.com/en-us/articles/fast-random-number-generator-on-the-intel-pentiumr-4-processor/

		// Use scalar LCG algorithm (vectorized version was slower)
		{
			// Straight-up C++ version
			AkReal32 * AK_RESTRICT pfBuf = (AkReal32 * AK_RESTRICT) out_pfBuffer;
			const AkReal32 * pfEnd = (AkReal32 *) ( pfBuf + in_uNumFrames );
			while ( pfBuf < pfEnd )
			{
				*pfBuf++ = GenerateAudioSample();
			}
		}

#endif // XBOX360
	}
} // namespace DSP

