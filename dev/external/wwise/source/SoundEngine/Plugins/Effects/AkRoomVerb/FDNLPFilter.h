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

// One pole low pass filter specific to FDN usage. Coefficients are computed so that frequency dependent decay time 
// can be obtained in FDN 
// y[n] = fB0 * x[n] - fA1 * y[n - 1]

#ifndef _AKFDNLPFILTER_H_
#define _AKFDNLPFILTER_H_

#include "OnePoleFilter.h"
#include <AK/SoundEngine/Common/AkTypes.h>
#include <math.h>
#include <AK/Tools/Common/AkAssert.h>

class FDNLPFilter : public DSP::OnePoleFilter
{
public:
	
	void ComputeFDNLPCoefs( AkReal32 in_fDecayTime, AkReal32 in_fHFDecayRatio, AkUInt32 in_uSampleRate, AkUInt32 in_uFDNDelayLength )
	{	
		AkReal64 dblFDNLineGain = pow(10.0,(-3.0*(AkReal64)in_uFDNDelayLength)/((AkReal64)in_fDecayTime*(AkReal64)in_uSampleRate));
		AkReal64 dblBeta = (20.0*log10(dblFDNLineGain)*log(10.0)/80.0*(1.0 - ((AkReal64)in_fHFDecayRatio*in_fHFDecayRatio)));	
		if ( dblBeta > 0.999 )
			dblBeta = 0.999;
		fB0 = (AkReal32)(dblFDNLineGain*(1.0-dblBeta));
		fA1 = (AkReal32)-dblBeta;	
	}

	void ProcessBuffer( AkReal32 * io_pfBuffer, AkUInt32 in_uNumFrames )
	{
		AKASSERT( !"FDNFilter not designed to used block processing. Using ProcessSample instead." );
	}
};

#endif // _AKFDNLPFILTER_H_
