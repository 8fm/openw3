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

#ifndef _AK_WHITENOISE_H_
#define _AK_WHITENOISE_H_

#include <AK/SoundEngine/Common/AkTypes.h>

namespace DSP
{
	static const AkReal32 AKNOISE_GAIN = 0.5f;	// 6 dB headroom
	
	// LCG algorithm
	#define AKNOISE_LCG_RANDSCALE (214013)
	#define AKNOISE_LCG_RANDOFFSET (2531011)
	static const AkReal32 AKNOISE_LCG_ONEOVERMAXRANDVAL	= ( AKNOISE_GAIN / 0x80000000 ); // 2^31

	class  CAkWhiteNoise
	{
	public:
#ifndef __SPU__
		CAkWhiteNoise();
#endif

		void GenerateBuffer(	
			AkReal32 * AK_RESTRICT out_pfBuffer, 
			AkUInt32 in_uNumFrames );

		// Audio sample (with some headroom) using either LCG or XorShift algorithm (platform specific)
		AkReal32 GenerateAudioSample()
		{
			return AKNOISE_LCG_ONEOVERMAXRANDVAL * GenerateLCG();
		}
	
	protected:

		AkReal32 GenerateLCG()
		{
			m_uLCGState = (m_uLCGState * AKNOISE_LCG_RANDSCALE) + AKNOISE_LCG_RANDOFFSET;
			AkInt32 iRandVal = (AkInt32) m_uLCGState;
			return (AkReal32)iRandVal;
		}

		AkUInt32 m_uLCGState;
	};
} // namespace DSP


#endif // _AK_WHITENOISE_H_
