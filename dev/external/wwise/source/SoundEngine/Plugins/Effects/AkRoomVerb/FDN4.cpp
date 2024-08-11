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

// 4x4 feedback delay network. 
// Use some temporary buffer to process 1 delay line at a time and localize memory access patterns
// Each delay line applies a normalized gain one pole low pass filter y[n] = fB0 * x[n] - fA1 * y[n-1]
// The feedback matrix used is HouseHolder

// Note: Feedback matrix used (implicit) is HouseHolder matrix that maximizes echo density 
// (no zero entries). This matrix recursion can be computed in 2N operation using matrix properties. 
// 4 x 4 using following values -2/N, 1-2/N
// -.5,  .5, -.5, -.5
// -.5, -.5,  .5, -.5
// -.5, -.5, -.5,  .5
//  .5, -.5, -.5, -.5
// Algorithm: 
// 1) Take the sum of all delay outputs d1 feedback = (d1 + d2 + d3 + d4)
// 2) Multiply by -2/N -> d1 feedback = -0.5(d1 + d2 + d3 + d4)
// 3) Add the full output of one delay line further to effectively change the coefficient 
// that was wrong in the previous computation
// i.e. -> d1 feedback = -0.5(d1 + d2 + d3 + d4) + d2 == -.5d1 + .5d2 -.5d3 -.5d4

#include "FDN4.h"
#include "AkDSPUtils.h"
#include <math.h>
#include <AK/SoundEngine/Common/IAkPlugin.h>

namespace DSP
{
	AkReal32 FDN4::ComputeMaxStableHFRatio(	
		AkUInt32 in_uDelayLineLength, 
		AkReal32 in_fDecayTime, 
		AkReal32 in_fHFDecayRatio, 
		AkUInt32 in_uSampleRate )
	{
		// Note: For larger delay length values with small reverb time and high HFRatio, the system may become unstable.
		// Fix: Look for those potential instabilities and update fReverbTimeRatioScale (dependent on HFRatio) untill it leads
		// to a stable system. The effective HFRatio may not be exactly the same as the user selected. It needs to be the same
		// for all filter computations.
		// Compute A1 coefficient for maximum delay length (worst case)
		AkReal64 fReverbTimeRatioScale = 1.0 - ((AkReal64)in_fHFDecayRatio*in_fHFDecayRatio);
		AkReal64 dblFDNLineGain = pow(10.0,(-3.0*(AkReal64)in_uDelayLineLength)/((AkReal64)in_fDecayTime*(AkReal64)in_uSampleRate));
		AkReal64 fScaleFactorWC = 20.0*log10(dblFDNLineGain)*log(10.0)/80.0;
		AkReal64 fA1WC = (fScaleFactorWC*fReverbTimeRatioScale);
		if ( fA1WC > 1.0 )
		{
			// Compute new fReverbTimeRatioScale for worst case scenario to avoid unstability
			fReverbTimeRatioScale = 1.0 / fScaleFactorWC;
		}
		AkReal32 fHFRPrime = (AkReal32)sqrt(1.0-fReverbTimeRatioScale);
		return fHFRPrime;
	}

	void FDN4::ProcessFeedbackMatrix( AkReal32 in_fDelayOut[4], AkReal32 out_fFeedback[4] )
	{
		// TODO: Consider using dot product instead on platform where it is available
		AkReal32 fScaledDelaySum = -0.5f * (in_fDelayOut[0] + in_fDelayOut[1] + in_fDelayOut[2] + in_fDelayOut[3]);
		out_fFeedback[0] = fScaledDelaySum + in_fDelayOut[1];
		out_fFeedback[1] = fScaledDelaySum + in_fDelayOut[2];
		out_fFeedback[2] = fScaledDelaySum + in_fDelayOut[3];
		out_fFeedback[3] = fScaledDelaySum + in_fDelayOut[0];
	}

	void FDN4::ChangeDecay( AkReal32 in_fDecayTime, AkReal32 in_fHFDecayRatio, AkUInt32 in_uSampleRate )
	{
		AkReal32 fMaxHFRatio = ComputeMaxStableHFRatio( FDNDelayLine[3].GetDelayLength(), in_fDecayTime, in_fHFDecayRatio, in_uSampleRate ); 

		// This should not get called before Init() is called
		delayLowPassFilter[0].ComputeFDNLPCoefs( in_fDecayTime, fMaxHFRatio, in_uSampleRate, FDNDelayLine[0].GetDelayLength() );
		delayLowPassFilter[1].ComputeFDNLPCoefs( in_fDecayTime, fMaxHFRatio, in_uSampleRate, FDNDelayLine[1].GetDelayLength() );
		delayLowPassFilter[2].ComputeFDNLPCoefs( in_fDecayTime, fMaxHFRatio, in_uSampleRate, FDNDelayLine[2].GetDelayLength() );
		delayLowPassFilter[3].ComputeFDNLPCoefs( in_fDecayTime, fMaxHFRatio, in_uSampleRate, FDNDelayLine[3].GetDelayLength() );
	}

	AKRESULT FDN4::Init(	
		AK::IAkPluginMemAlloc * in_pAllocator, 
		AkUInt32 in_uDelayLineLength[4], 
		AkReal32 in_fDecayTime, 
		AkReal32 in_fHFDecayRatio, 
		AkUInt32 in_uSampleRate )
	{
#define RETURNIFNOTSUCCESS( __FONCTIONEVAL__ )	\
{												\
	AKRESULT __result__ = (__FONCTIONEVAL__);	\
	if ( __result__ != AK_Success )				\
		return __result__;						\
}												\

		AkReal32 fMaxHFRatio = ComputeMaxStableHFRatio( in_uDelayLineLength[3], in_fDecayTime, in_fHFDecayRatio, in_uSampleRate ); 

		// Delay lines
		RETURNIFNOTSUCCESS( FDNDelayLine[0].Init( in_pAllocator, in_uDelayLineLength[0] ) );	
		RETURNIFNOTSUCCESS( FDNDelayLine[1].Init( in_pAllocator, in_uDelayLineLength[1] ) );
		RETURNIFNOTSUCCESS( FDNDelayLine[2].Init( in_pAllocator, in_uDelayLineLength[2] ) );
		RETURNIFNOTSUCCESS( FDNDelayLine[3].Init( in_pAllocator, in_uDelayLineLength[3] ) );
		// Delay lines LPF	
		delayLowPassFilter[0].ComputeFDNLPCoefs( in_fDecayTime, fMaxHFRatio, in_uSampleRate, in_uDelayLineLength[0] );
		delayLowPassFilter[1].ComputeFDNLPCoefs( in_fDecayTime, fMaxHFRatio, in_uSampleRate, in_uDelayLineLength[1] );
		delayLowPassFilter[2].ComputeFDNLPCoefs( in_fDecayTime, fMaxHFRatio, in_uSampleRate, in_uDelayLineLength[2] );
		delayLowPassFilter[3].ComputeFDNLPCoefs( in_fDecayTime, fMaxHFRatio, in_uSampleRate, in_uDelayLineLength[3] );

		return AK_Success;
	}

	void FDN4::Term( AK::IAkPluginMemAlloc * in_pAllocator )
	{
		FDNDelayLine[0].Term( in_pAllocator );
		FDNDelayLine[1].Term( in_pAllocator );
		FDNDelayLine[2].Term( in_pAllocator );
		FDNDelayLine[3].Term( in_pAllocator );
	}

	void FDN4::Reset( )
	{
		FDNDelayLine[0].Reset();
		FDNDelayLine[1].Reset();
		FDNDelayLine[2].Reset();
		FDNDelayLine[3].Reset();
		delayLowPassFilter[0].Reset();
		delayLowPassFilter[1].Reset();
		delayLowPassFilter[2].Reset();
		delayLowPassFilter[3].Reset();
	}


#include "FDN4Dsp.inl"

}