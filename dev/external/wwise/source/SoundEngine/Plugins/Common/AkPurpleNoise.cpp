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

#include "AkPurpleNoise.h"

namespace DSP
{
#ifndef __SPU__
	CAkPurpleNoise::CAkPurpleNoise()
	{
		PurpleFilter.SetCoefs( 
			AKPURPLENOISE_B0COEF,
			AKPURPLENOISE_B1COEF,
			AKPURPLENOISE_B2COEF,
			AKPURPLENOISE_A1COEF,
			AKPURPLENOISE_A2COEF );
	}
#endif

	void CAkPurpleNoise::GenerateBuffer(	
		AkReal32 * out_pfBuffer, 
		AkUInt32 in_uNumFrames )
	{
		CAkWhiteNoise::GenerateBuffer( out_pfBuffer, in_uNumFrames );
		PurpleFilter.ProcessBuffer( out_pfBuffer, in_uNumFrames );
	}

} // namespace DSP

