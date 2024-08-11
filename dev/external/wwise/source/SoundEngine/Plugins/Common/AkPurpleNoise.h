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

// Purple noise 
// Use white noise and filter it with biquad filter to approximate +3 dB per octave slope (+10 dB per decade)
// 2nd order filter is far from perfect for this task (filter slope go up by 6dB), but for our purposes this is good enough

#ifndef _AK_PURPLENOISE_H_
#define _AK_PURPLENOISE_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include "AkWhiteNoise.h" // Random number generator is white noise
#include "../Effects/Common/BiquadFilter.h"

namespace DSP
{
	static const AkReal32 AKPURPLENOISE_GAIN = 0.0316f;	

	// The coefficients below are obtained by alternating poles and zeros over frequencies
	static const AkReal32 AKPURPLENOISE_A0COEF = 0.04957526213389f;
	static const AkReal32 AKPURPLENOISE_A1COEF = -0.06305581334498f/AKPURPLENOISE_A0COEF;
	static const AkReal32 AKPURPLENOISE_A2COEF = 0.01483220320740f/AKPURPLENOISE_A0COEF;
	static const AkReal32 AKPURPLENOISE_B0COEF = AKPURPLENOISE_GAIN*1.f/AKPURPLENOISE_A0COEF;
	static const AkReal32 AKPURPLENOISE_B1COEF = AKPURPLENOISE_GAIN*-1.80116083982126f/AKPURPLENOISE_A0COEF;
	static const AkReal32 AKPURPLENOISE_B2COEF = AKPURPLENOISE_GAIN*0.80257737639225f/AKPURPLENOISE_A0COEF;

	class  CAkPurpleNoise : public CAkWhiteNoise
	{
	public:

#ifndef __SPU__
		CAkPurpleNoise();
#endif

		void GenerateBuffer(	
			AkReal32 * out_pfBuffer, 
			AkUInt32 in_uNumFrames );

	protected:

		BiquadFilterMono PurpleFilter;
	};
} // namespace DSP

#endif // _AK_PURPLENOISE_H_
