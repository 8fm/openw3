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

// Generic one pole filter. Coefficients are set explicitely by client.
// y[n] = fB0 * x[n] - fA1 * y[n - 1]
// To be used on mono signals, create as many instances as there are channels if need be
// Also provides processing per sample

#include "OnePoleFilter.h"
#include "AkDSPUtils.h"
#include <math.h>

namespace DSP
{
	void OnePoleFilter::ProcessBuffer( AkReal32 * io_pfBuffer, AkUInt32 in_uNumFrames )
	{
		AkReal32 * AK_RESTRICT pfBuf = (AkReal32 * AK_RESTRICT) io_pfBuffer;
		const AkReal32 * const fEnd = pfBuf + in_uNumFrames;
		while ( pfBuf < fEnd )
		{
			AkReal32 fOut = *pfBuf * fB0 - fA1 * fFFbk1;
			fFFbk1 = fOut;
			*pfBuf++ = fOut;		
		}

		RemoveDenormal( fFFbk1 );
	}

	void OnePoleFilter::SetCoefs( FilterType eType, AkReal32 in_fFc, AkUInt32 in_uSr )
	{
		ComputeCoefs( eType, in_fFc, in_uSr, fB0, fA1 );
	}

	void OnePoleFilter::ComputeCoefs( FilterType eType, AkReal32 in_fFc, AkUInt32 in_uSr, AkReal32 & out_fB0, AkReal32 & out_fA1 )
	{
		switch ( eType )
		{
			case FILTERCURVETYPE_NONE:
				out_fA1 = 0.f;
				out_fB0 = 1.f;
				break;
			case FILTERCURVETYPE_LOWPASS:
				{
					static const AkReal32 fPI = 3.1415926535f;
					AkReal32 fNormalizedFc = in_fFc / (AkReal32)in_uSr;
					AkReal32 fTemp = 2.f - cos(2.f*fPI*fNormalizedFc);
					out_fA1 = sqrt(fTemp * fTemp - 1.f) - fTemp;
					out_fB0 = 1.f + out_fA1;
					break;
				}
			case FILTERCURVETYPE_HIGHPASS:
				{
					static const AkReal32 fPI = 3.1415926535f;
					AkReal32 fNormalizedFc = in_fFc / (AkReal32)in_uSr;
					AkReal32 fTemp = 2.f + cos(2.f*fPI*fNormalizedFc);
					out_fA1 = fTemp - sqrt(fTemp * fTemp - 1.f);
					out_fB0 = 1.f - out_fA1;
				}
		}
	}

} // namespace DSP

