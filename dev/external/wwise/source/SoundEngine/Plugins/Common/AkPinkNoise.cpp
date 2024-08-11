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

// Pink noise or 1/f noise:
// Pink noise is generated using Gardner's algorithm 

#include "AkPinkNoise.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>

namespace DSP
{
#ifndef __SPU__
	CAkPinkNoise::CAkPinkNoise()
		: m_uIndex(0)
		, m_fRunningSum(0.f)
	{
		AKPLATFORM::AkMemSet( &m_RandGenTable, 0, AKPINKNOISE_NUMROWS*sizeof(AkReal32) );
	}
#endif

	//void CAkPinkNoise::GenerateBuffer(	
	//	AkReal32 * AK_RESTRICT out_pfBuffer, // Note: This buffer must be large enough to hold 2 * in_uNumFrames
	//	AkUInt32 in_uNumFrames )
	//{
	//	// Base generator always fires (first half)
	//	// Secondary generator always fires (second half) but goes to different rows each time
	//	// Note: This algorithm uses the fact that half the time row 0 is the one firing
	//	CAkWhiteNoise::GenerateBuffer( out_pfBuffer, 2*in_uNumFrames );
	//	AkReal32 * AK_RESTRICT pfWhiteGen = out_pfBuffer;
	//	AkReal32 * AK_RESTRICT pfRowsGen = out_pfBuffer+in_uNumFrames;
	//	const AkReal32 * pfEnd = pfRowsGen;
	//	// AKASSERT( (in_uNumFrames % 2) == 0 );
	//	
	//	// Local variables
	//	AkUInt32 uIndex = m_uIndex;
	//	AkReal32 fRunningSum = m_fRunningSum;
	//	while ( pfWhiteGen < pfEnd )
	//	{	
	//		// Increment and mask index 
	//		uIndex = (++uIndex) & AKPINKNOISE_INDEXMASK;

	//		if ( uIndex )
	//		{
	//			// Count number of trailing 0 in current index
	//			AkUInt32 uNumZeros = 0;
	//			AkUInt32 uTempVal = uIndex;
	//			while ( ( uTempVal & 1) == 0 )
	//			{
	//				uTempVal = uTempVal >> 1;
	//				++uNumZeros;
	//			}

	//			// Replace old random from running sum using new random value for runnint total instead
	//			fRunningSum -= m_RandGenTable[uNumZeros];
	//			AkReal32 fNewRandVal = *pfRowsGen++;
	//			fRunningSum += fNewRandVal;
	//			m_RandGenTable[uNumZeros] = fNewRandVal;
	//		}

	//		// Combine base generator and appropriate row result
	//		AkReal32 fWhiteVal = *pfWhiteGen;
	//		AkReal32 fPinkOut = AKPINKNOISE_GAIN * ( fRunningSum + fWhiteVal );
	//		fPinkOut = m_DCFilter.ProcessSample( fPinkOut );
	//		*pfWhiteGen++ = fPinkOut;
	//	}

	//	// Save local variables to state
	//	m_uIndex = uIndex;
	//	m_fRunningSum = fRunningSum;
	//}

	// Get rid of while branch mispredict
	void CAkPinkNoise::GenerateBuffer(	
		AkReal32 * out_pfBuffer, // Note: This buffer must be large enough to hold 2 * in_uNumFrames
		AkUInt32 in_uNumFrames )
	{
		// Base generator always fires (first half)
		// Secondary generator always fires (second half) but goes to different rows each time
		CAkWhiteNoise::GenerateBuffer( out_pfBuffer, 2*in_uNumFrames );
		AkReal32 * AK_RESTRICT pfWhiteGen = out_pfBuffer;
		AkReal32 * AK_RESTRICT pfRowsGen = out_pfBuffer+in_uNumFrames;
		const AkReal32 * pfEnd = pfRowsGen;
		// AKASSERT( (in_uNumFrames % 2) == 0 );
		AkReal32				fRandGenTable[AKPINKNOISE_NUMROWS];
		AKPLATFORM::AkMemCpy(fRandGenTable,m_RandGenTable,AKPINKNOISE_NUMROWS*sizeof(AkReal32));
	
		// Local variables
		AkUInt32 uIndex = m_uIndex;
		AkReal32 fRunningSum = m_fRunningSum;
		while ( pfWhiteGen < pfEnd )
		{	
			// Increment and mask index 
			uIndex = (++uIndex) & AKPINKNOISE_INDEXMASK;

			// http://citeseerx.ist.psu.edu/viewdoc/download;jsessionid=09F8D83E20E9DE2F9D776AE841A1A905?doi=10.1.1.37.8562&rep=rep1&type=pdf
			AkInt32 uNumZeros = CountTrailingZerosTable[((AkUInt32)((uIndex & -static_cast<int>(uIndex)) * 0x077CB531U)) >> 27];

			// Replace old random from running sum using new random value for runnint total instead
			fRunningSum -= fRandGenTable[uNumZeros];
			AkReal32 fNewRandVal = *pfRowsGen++;
			fRunningSum += fNewRandVal;
			fRandGenTable[uNumZeros] = fNewRandVal;

			// Combine base generator and appropriate row result
			AkReal32 fWhiteVal = *pfWhiteGen;
			AkReal32 fPinkOut = AKPINKNOISE_GAIN * ( fRunningSum + fWhiteVal );
			*pfWhiteGen++ = fPinkOut;
		}

		m_DCFilter.ProcessBuffer( out_pfBuffer, in_uNumFrames );

		// Save local variables to state
		m_uIndex = uIndex;
		m_fRunningSum = fRunningSum;
		AKPLATFORM::AkMemCpy(m_RandGenTable,fRandGenTable,AKPINKNOISE_NUMROWS*sizeof(AkReal32));
	}
} // namespace DSP

