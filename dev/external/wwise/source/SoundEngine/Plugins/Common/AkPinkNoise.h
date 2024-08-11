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
// Pink noise is generated using Gardner's algorithm (see "White and Brown Music, Fractal Curves and
// 1/f fluctuations."). A short overview of the algorithm is provided here.
// The pink noise is obtained by summing the output of N uniform random number generators 
// in a particular way. Each random generator triggers a new value when its bit changes while
// counting the output value index in binary (N-bit number). Each bit in the number is associated
// with one of the random numbers. Whenever the the bit changes from 0 to 1 or 1 to 0, the associated 
// random generator computes a new value and the sum is recomputed. Since not all random generators
// fire a new value at each new samples this constraints the possible values the new output can jump
// to in a similar fashion than a lowpass filter would constrain them. Controlling the firing order
// this way result in a -3dB/octave slope (more accurately -10dB per decade) that is characteristic 
// to pink noise. Below is a 4-bit simplification of random generator firing as a function of the 
// sample index.
// This algorithm by its nature generates DC offset so we use a one pole HP filter to remove it
/*
-------------------------------------------
Bit					Generator
3	2	1	0		1	2	3	4
-------------------------------------------
1	1	1	1		(init time)	
0	0	0	0		X	X	X	X
0	0	0	1					X
0	0	1	0				X	X	
0	0	1	1					X
0	1	0	0			X	X	X
0	1	0	1					X
0	1	1	0				X	X
ETC.

A revised implementation by McCartney generates higher quality pink noise more efficiently by reordering the
firing of the random number generator as showed in the figure below.

xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx 
x x x x x x x x x x x x x x x x 
 x   x   x   x   x   x   x   x 
   x       x       x       x 
       x               x 
               x 

This ensures that only 2 generators are firing for every sample. This is accomplished by counting the
number of trailing zeroes in an increasing index.
*/


#ifndef _AK_PINKNOISE_H_
#define _AK_PINKNOISE_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include "AkWhiteNoise.h" // Random number generator is white noise
#include "../Effects/Common/OnePoleZeroHPFilter.h"

namespace DSP
{
	// 128 bytes alignment to ensure single cache line on 360
#ifdef AK_XBOX360
	__declspec(align(128)) static const AkInt32 CountTrailingZerosTable[32] = 
#else
	static const AkInt32 CountTrailingZerosTable[32] = 
#endif
	{
		0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8, 
		31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
	};

	// Pink noise algorithm definitions
	static const AkUInt32 AKPINKNOISE_NUMROWS = 16;		// Number of random generators used in Gardner's algorithm
	static const AkUInt32 AKPINKNOISE_NUMRANDOMBITS = 24;
	static const AkUInt32 AKPINKNOISE_RANDOMSHIFT = ((sizeof(AkUInt32)*8)-AKPINKNOISE_NUMRANDOMBITS);
	static const AkReal32 AKPINKNOISE_DCCUTFREQ = 40.f;
	static const AkUInt32 AKPINKNOISE_INDEXMASK  = (1<<AKPINKNOISE_NUMROWS) - 1;
	static const AkReal32 AKPINKNOISE_GAIN = 2.5f / (AKPINKNOISE_NUMROWS + 1);
	class  CAkPinkNoise : public CAkWhiteNoise
	{
	public:

#ifndef __SPU__
		CAkPinkNoise();
#endif

		void Init( AkUInt32 in_uSampleRate )
		{
			m_DCFilter.ComputeCoefs( AKPINKNOISE_DCCUTFREQ, in_uSampleRate );
		}

		void GenerateBuffer(	
			AkReal32 * out_pfBuffer, // Note: This buffer must be large enough to hold 2 * in_uNumFrames
			AkUInt32 in_uNumFrames );

	protected:

		OnePoleZeroHPFilter		m_DCFilter;		// Remove DC offset
		AkUInt32				m_uIndex;		// Current generator index	
		AkReal32				m_fRunningSum;	// Current running sum
		AkReal32				m_RandGenTable[AKPINKNOISE_NUMROWS];	// Table of random generator output (rows)
	};
} // namespace DSP

#endif // _AK_PINKNOISE_H_
