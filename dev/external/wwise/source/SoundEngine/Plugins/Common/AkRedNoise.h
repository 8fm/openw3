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

// Red (or brown) noise or 1/f^2 noise:
// Simply using white noise and filtering it with one pole filter that has -6 dB per octave slope -20 dB per decade)

#ifndef _AK_REDNOISE_H_
#define _AK_REDNOISE_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include "AkWhiteNoise.h" // Random number generator is white noise
#include "../Effects/Common/OnePoleFilter.h"

namespace DSP
{
	static const AkReal32 AKREDNOISE_GAIN = 10.f; // 20 dB	
	static const AkReal32 AKREDNOISE_A1COEF = -0.995f;
	static const AkReal32 AKREDNOISE_B0COEF = AKREDNOISE_GAIN * (1.f + AKREDNOISE_A1COEF);

	class  CAkRedNoise : public CAkWhiteNoise
	{
	public:

#ifndef __SPU__
		CAkRedNoise();
#endif
		void GenerateBuffer(	
			AkReal32 * out_pfBuffer, 
			AkUInt32 in_uNumFrames );

	protected:

		OnePoleFilter RedFilter;
	};
} // namespace DSP

#endif // _AK_REDNOISE_H_
