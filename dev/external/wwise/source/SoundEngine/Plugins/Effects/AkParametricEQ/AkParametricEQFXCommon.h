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


#ifndef _AK_PARAMETRICEQFX_COMMON_H_
#define _AK_PARAMETRICEQFX_COMMON_H_

#include "AkDSPUtils.h"

// Filter module number
enum AkBandNumber
{
	BAND1 = 0,
	BAND2 = 1,
	BAND3 = 2
};

static const AkUInt8 NUMBER_FILTER_MODULES = 3;
static const AkUInt8 NUMBER_COEFFICIENTS_PER_BAND = 5;
static const AkUInt8 NUMBER_FILTER_MEMORY = 4;

static void Process(	AkAudioBuffer * io_pBuffer, 
						AkReal32 * in_pfCoefs, 
						AkReal32 * io_pfMems,
						AkUInt32 in_uNumProcessedChannels )
{
	//START HERE
	const AkReal32 b0 = in_pfCoefs[0];	// b0
	const AkReal32 b1 = in_pfCoefs[1];	// b1
	const AkReal32 b2 = in_pfCoefs[2];	// b2
	const AkReal32 a1 = in_pfCoefs[3];	// a1
	const AkReal32 a2 = in_pfCoefs[4];	// a2

	for ( unsigned int uChan = 0; uChan < in_uNumProcessedChannels; ++uChan )
	{
		AkReal32 * AK_RESTRICT pfBuf = io_pBuffer->GetChannel(uChan);	
		const AkReal32 * AK_RESTRICT pfEnd = pfBuf + io_pBuffer->uValidFrames;
		AkUInt32 uChannelOffset = NUMBER_FILTER_MEMORY*uChan;
		AkReal32 xn1 = io_pfMems[uChannelOffset];	// xn1
		AkReal32 xn2 = io_pfMems[uChannelOffset+1];	// xn2 -> mem will be thrashed so xn2 can be used as tmp register
		AkReal32 yn1 = io_pfMems[uChannelOffset+2];	// yn1
		AkReal32 yn2 = io_pfMems[uChannelOffset+3];	// yn2 -> mem will be thrashed so xn2 can be used as tmp register	
		
		while ( pfBuf < pfEnd )
		{
			// Feedforward part
			const AkReal32 in = *pfBuf;
			AkReal32 out = in * b0;
			xn2 = xn2 * b2;
			out = out + xn2;
			xn2 = xn1;
			xn1 = xn1 * b1;
			out = out + xn1;
			xn1 = in;

			// Feedback part
			yn2 = yn2 * a2;
			out = out + yn2;
			yn2 = yn1;
			yn1 = yn1 * a1;
			out = out + yn1;
			yn1 = out;
			*pfBuf = out;		

			++pfBuf;
		}

		RemoveDenormal( xn1 );
		RemoveDenormal( xn2 );
		RemoveDenormal( yn1 );
		RemoveDenormal( yn2 );

		// save registers to memory
		io_pfMems[uChannelOffset] = xn1;
		io_pfMems[uChannelOffset+1] = xn2;
		io_pfMems[uChannelOffset+2] = yn1;
		io_pfMems[uChannelOffset+3] = yn2;
	}
	//STOP HERE
}

#endif // _AK_PARAMETRICEQFX_COMMON_H_
