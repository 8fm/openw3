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

// Control rate random values helper class

#ifndef _AK_RANDOMVALUES_H_
#define _AK_RANDOMVALUES_H_

#include <AK/SoundEngine/Common/AkTypes.h>

#ifndef __SPU__
#include "../../AkAudiolib/Common/AkRandom.h"
#endif

namespace DSP
{
	// LCG
	#define LCG_RANDSCALE (214013)
	#define LCG_RANDOFFSET (2531011)
	static const AkReal32 LCG_ONEOVERMAXRANDVAL	= ( 1.f / 0xFFFFFFFF ); // 2^32
	static const AkUInt32 LCG_MAXRANDVAL = 0xFFFFFFFF;
	static const AkUInt32 LCG_MAXRANDVALOVERTWO = LCG_MAXRANDVAL / 2;
	class  CAkRandomValues
	{
	public:
#ifndef __SPU__
	CAkRandomValues()
	{
		m_uLCGState = (AkUInt32)AKRANDOM::AkRandom();
	}
#endif

	// Fire or not an event based on a probability (0-1)
	bool PositiveEventCheck( AkReal32 in_fProbability )
	{
		AkReal32 fRand = GenerateValue();
		if ( fRand > in_fProbability )
			return false;
		else
			return true;
	}

	// return true or false with the same probability
	bool FairBernoulli( )
	{
		m_uLCGState = (m_uLCGState * LCG_RANDSCALE) + LCG_RANDOFFSET;
		return (m_uLCGState > (LCG_MAXRANDVALOVERTWO) );
	}

	// Generate (uniform PDF) random value in (0-1) range 
	AkReal32 GenerateValue()
	{
		m_uLCGState = (m_uLCGState * LCG_RANDSCALE) + LCG_RANDOFFSET;
		return (AkReal32)m_uLCGState * LCG_ONEOVERMAXRANDVAL;
	}

	// Generate (triangular PDF) random value in (0-1) range
	AkReal32 GenerateTriangularValue()
	{
		AkReal32 fRandVal1 =  GenerateValue( );
		AkReal32 fRandVal2 = GenerateValue( );
		return ((fRandVal1 + fRandVal2) / 2.f);
	}

	// Returns a random float value between in_fMin and in_fMax
	AkReal32 RandomRange( AkReal32 in_fMin, AkReal32 in_fMax )
	{
		AkReal32 fRandVal = GenerateValue( );
		return ( fRandVal * (in_fMax - in_fMin) + in_fMin );
	}

	protected:
		AkUInt32 m_uLCGState;
	};
} // namespace DSP


#endif // _AK_RANDOMVALUES_H_
