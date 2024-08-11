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

// Helper routines related to memory allocation based on parameters
// Kept apart because Wwise plug-ins needs to be able to estimate memory usage

#ifndef _AKDELAYLENGTHS_H_
#define _AKDELAYLENGTHS_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include <stdlib.h>
#include "AlgoTunings.h"
#include "RandNormalizedFloat.h"

namespace DelayLengths
{
#ifndef AK_OPTIMIZED
	static AkUInt32 ComputeMemoryStats( 
		AkReal32 in_fDensity, 
		AkReal32 in_fRoomShape, 
		AkUInt32 in_uNumReverbUnits, 
		AkReal32 in_fERLongestTapTime,
		AkReal32 in_fReverbDelay,
		AkReal32 in_fERFrontbackDelay,
		AkUInt32 in_uSampleRate,
		AlgorithmTunings & in_Tunings,
		AkReal32 * out_pModalDensity = NULL,
		AkReal32 * out_pEchoDensity = NULL );
#endif

	static int AkDelayQSortCompareFloat(const void * a, const void * b)
	{
		float fDiff = ( *(float*)a - *(float*)b );
		if ( fDiff < 0.f )
			return -1;
		else if ( fDiff > 0.f )
			return 1;
		else
			return 0;
	}

	// Compute FDN delay times (in ms) based on user parameters and algorithm tunings
	static void ComputeFDNDelayTimes(	
		AkReal32 in_fDensity, 
		AkReal32 in_fRoomShape, 
		AkUInt32 in_uNumDelayLines, 
		AlgorithmTunings & in_Tunings,
		AkReal32 * out_pfFDNDelayTimes )
	{
		AkReal32 fTmean = (in_Tunings.fDensityDelayMax-in_Tunings.fDensityDelayMin)/100.f*in_fDensity + in_Tunings.fDensityDelayMin;
		AkReal32 fRoomShapeScale = (in_Tunings.fRoomShapeMax-in_Tunings.fRoomShapeMin)/100.f*in_fRoomShape + in_Tunings.fRoomShapeMin; 
		AkReal32 fTmin = fTmean*fRoomShapeScale;
		AkReal32 fTmax = fTmean+(fTmean-fTmin); 

		// Random things should always behave the same (it does for each instances)
		DSP::RandNormFloat RandGen;
		RandGen.SetSeed(7032007);

		// Linearly increasing values from Tmin to Tmax with some randomization relative to current value to avoid ringing
		for ( AkUInt32 i = 0; i < in_uNumDelayLines; i++ )
		{
			AkReal32 fDiff = fTmax - fTmin;
			AkReal32 fDelayTime = (i*fDiff)/(in_uNumDelayLines-1) + fTmin;
			AkReal32 fRandom = RandGen.Tick();
			fRandom *= fDelayTime * (in_Tunings.fDensityDelayRdmPerc/100.f);
			out_pfFDNDelayTimes[i] = fDelayTime + fRandom;
		}
		qsort(out_pfFDNDelayTimes, in_uNumDelayLines, sizeof(AkReal32), AkDelayQSortCompareFloat);
	}

	// Compute FDN delay times (in ms) based on user parameters and algorithm tunings
	static void ComputeFDNInputDelayTimes(	
		AkUInt32 in_uNumDelayLines, 
		AlgorithmTunings & in_Tunings,
		AkReal32 * out_pfFDNInputDelayTimes )
	{
		// Random things should always behave the same (it does for each instances)
		DSP::RandNormFloat RandGen;
		RandGen.SetSeed(29101977);

		for ( AkUInt32 i = 0; i < in_uNumDelayLines; i++ )
		{
			AkReal32 fDelayTime = in_Tunings.fReverbUnitInputDelay/in_uNumDelayLines;
			AkReal32 fRandom = RandGen.Tick();
			fRandom *= fDelayTime * (in_Tunings.fReverbUnitInputDelayRmdPerc/100.f);
			out_pfFDNInputDelayTimes[i] = fDelayTime + fRandom;  
		}
	}

	// Compute Allpass filter delay times (in ms) based on user parameters and algorithm tunings
	static void ComputeDiffusionFiltersDelayTimes(	
		AkUInt32 in_uNumReverbUnits, 
		AlgorithmTunings & in_Tunings,
		AkReal32 * out_pfAPFDelayTimes )
	{
		// Each delay is a certain percentage of the previous one
		AkReal32 fPrevLength = in_Tunings.fDiffusionDelayMax;

		// Random things should always behave the same (it does for each instances)
		DSP::RandNormFloat RandGen;
		RandGen.SetSeed(3072007);

		// Linearly increasing values from Tmin to Tmax with some randomization relative to Tmean to avoid ringing
		for ( AkUInt32 i = 0; i < in_uNumReverbUnits; i++ )
		{
			AkReal32 fDelayTime = fPrevLength;	
			fPrevLength *= in_Tunings.fDiffusionDelayScalePerc/100.f;
			AkReal32 fRandom = RandGen.Tick();
			fRandom *= fDelayTime * (in_Tunings.fDiffusionDelayRdmPerc/100.f);
			out_pfAPFDelayTimes[i] = fDelayTime + fRandom;
		}
		qsort(out_pfAPFDelayTimes, in_uNumReverbUnits, sizeof(AkReal32), AkDelayQSortCompareFloat);
	}

	// Change an input integer value into its next prime value
	// Assumes that in_uPrevVal is part of a sorted array
	// Will not provide unique prime number if more than 2 numbers map to the same prime in that array
	static void MakePrimeNumber( AkUInt32 & in_uIn, AkUInt32 in_uPrevVal )
	{
		// First ensure its odd
		if ( (in_uIn & 1) == 0) 
			in_uIn++;	

		// Only need to compute up to square root (math theorem)
		AkInt32 iStop = (AkInt32) sqrt((AkReal64) in_uIn) + 1; 
		while ( true ) 
		{
			bool bFoundDivisor = false;
			for (AkInt32 i = 3; i < iStop; i+=2 )
			{
				if ( (in_uIn % i) == 0) 
				{
					// Can be divided by some number so not prime
					bFoundDivisor = true;
					break;
				}
			}

			if (!bFoundDivisor)
			{
				// Could not find dividors so its a prime number
				if ( in_uIn != in_uPrevVal )
					break;	// Unique done.
				// otherwise continue to next prime
			}
			in_uIn += 2;	// Otherwise try the next odd number
		}
	}

	// Take delay times and convert them to prime delay lengths
	static void ComputePrimeDelayLengths(	
		AkReal32 * in_pfDelayTimes, 
		AkUInt32 in_uSampleRate, 
		AkUInt32 in_uNumDelays, 
		AkUInt32 * out_puDelayLengths )
	{
		// Assume incoming delay times are sorted
		// Ensure the values are prime numbers and not equal to other delays
		for ( AkUInt32 i = 0; i < in_uNumDelays; ++i )
		{
			out_puDelayLengths[i] = (AkUInt32)((in_pfDelayTimes[i]/1000.f)*in_uSampleRate);
			if ( i == 0 )
				MakePrimeNumber( out_puDelayLengths[i], 0 ); 
			else
				MakePrimeNumber( out_puDelayLengths[i], out_puDelayLengths[i-1] );
		}
	}

#ifndef AK_OPTIMIZED	
	static AkUInt32 ComputeMemoryStats( 
		AkReal32 in_fDensity, 
		AkReal32 in_fRoomShape, 
		AkUInt32 in_uNumReverbUnits, 
		AkReal32 in_fERLongestTapTime,
		AkReal32 in_fReverbDelay,
		AkReal32 in_fERFrontbackDelay,
		AkUInt32 in_uSampleRate,
		AlgorithmTunings & in_Tunings,
		AkReal32 * out_pModalDensity, /* = NULL */
		AkReal32 * out_pEchoDensity ) /* = NULL */
	{
		AkUInt32 uERDelayLength = (AkUInt32) (in_fERLongestTapTime/1000.f * in_uSampleRate);
		AkUInt32 uReverbDelayLength = (AkUInt32) (in_fReverbDelay/1000.f * in_uSampleRate);
		AkUInt32 uERFrontBackDelay = (AkUInt32) (in_fERFrontbackDelay/1000.f * in_uSampleRate) * 2; 

		////////////////////// Compute delay times based on parameters and tunings ////////////////////

		AkReal32 fFDNDelayTimes[MAXNUMREVERBUNITS*4];
		ComputeFDNDelayTimes(	in_fDensity, 
			in_fRoomShape, 
			in_uNumReverbUnits*4, 
			in_Tunings,
			fFDNDelayTimes );

		AkReal32 fAPFDelayTimes[NUMDIFFUSIONALLPASSFILTERS];
		ComputeDiffusionFiltersDelayTimes( NUMDIFFUSIONALLPASSFILTERS, in_Tunings, fAPFDelayTimes );

		AkReal32 fFDNInputDelayTimes[MAXNUMREVERBUNITS];
		ComputeFDNInputDelayTimes( in_uNumReverbUnits, in_Tunings, fFDNInputDelayTimes );

		////////////////////// Convert to prime delay line lengths ////////////////////

		AkUInt32 uFDNDelayLengths[MAXNUMREVERBUNITS*4];
		ComputePrimeDelayLengths(	fFDNDelayTimes, 
			in_uSampleRate,
			in_uNumReverbUnits*4, 
			uFDNDelayLengths );

		AkUInt32 uSumFDNDelays = 0;
		AkReal32 fModalDensity = 0.f;
		AkReal32 fEchoDensity = 0.f;
		for ( AkUInt32 i = 0; i < in_uNumReverbUnits*4; i++ )
		{
			uSumFDNDelays += uFDNDelayLengths[i];
			AkReal32 fDelayTime = (AkReal32)uFDNDelayLengths[i] / in_uSampleRate;
			fModalDensity += fDelayTime;
			fEchoDensity += 1.f/fDelayTime;
		}

		AkUInt32 uAPFDelayLengths[NUMDIFFUSIONALLPASSFILTERS];
		DelayLengths::ComputePrimeDelayLengths( fAPFDelayTimes, 
			in_uSampleRate, 
			NUMDIFFUSIONALLPASSFILTERS, 
			uAPFDelayLengths );

		AkUInt32 uSumAllpassDelays = 0;
		for ( AkUInt32 i = 0; i < NUMDIFFUSIONALLPASSFILTERS; i++ )
		{
			uSumAllpassDelays += uAPFDelayLengths[i];
		}

		AkUInt32 uFDNInputDelayLengths[MAXNUMREVERBUNITS];
		AkUInt32 uSumFDNInputDelays = 0;
		for ( AkUInt32 i = 0; i < in_uNumReverbUnits; i++ )
		{
			uFDNInputDelayLengths[i] = (AkUInt32)(fFDNInputDelayTimes[i]/1000.f*in_uSampleRate);
			uSumFDNInputDelays += uFDNInputDelayLengths[i];
		}

		AkUInt32 uTotalMemFrames = uSumAllpassDelays + uSumFDNDelays + uSumFDNInputDelays + uERDelayLength + uReverbDelayLength + uERFrontBackDelay;

		if ( out_pModalDensity != NULL )
			*out_pModalDensity = fModalDensity;
		if ( out_pEchoDensity != NULL )
			*out_pEchoDensity = fEchoDensity;
		return uTotalMemFrames * sizeof(AkReal32);
	}
#endif

} // namespace DelayLengths

#endif // _AKDELAYLENGTHS_H_
