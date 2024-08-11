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

//////////////////////////////////////////////////////////////////////
//
// AkADPCMCodec.cpp
//
// ADPCM decompressor codec (based on IMA ADPCM). 
// See ADPCM document on Sharepoint for details.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include "AkADPCMCodec.h"
#include "AkCommon.h"

// This array is used by NextStepIndex to determine the next step index to use.  
// The step index is an index to the m_asStep[] array, below.
const AkInt16 CAkADPCMCodec::m_asNextStep[16] =
{
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};

// This array contains the array of step sizes used to encode the ADPCM
// samples.  The step index in each ADPCM block is an index to this array.
#define NUMSTEPINDEXVAL 89
#define NUMSTEPINDVALMINUSONE 88
const AkInt16 CAkADPCMCodec::m_asStep[NUMSTEPINDEXVAL] =
{
        7,     8,     9,    10,    11,    12,    13,
       14,    16,    17,    19,    21,    23,    25,
       28,    31,    34,    37,    41,    45,    50,
       55,    60,    66,    73,    80,    88,    97,
      107,   118,   130,   143,   157,   173,   190,
      209,   230,   253,   279,   307,   337,   371,
      408,   449,   494,   544,   598,   658,   724,
      796,   876,   963,  1060,  1166,  1282,  1411,
     1552,  1707,  1878,  2066,  2272,  2499,  2749,
     3024,  3327,  3660,  4026,  4428,  4871,  5358,
     5894,  6484,  7132,  7845,  8630,  9493, 10442,
    11487, 12635, 13899, 15289, 16818, 18500, 20350,
    22385, 24623, 27086, 29794, 32767
};

unsigned short CAkADPCMCodec::CalculateDecodeAlignment( unsigned short nChannels )
{
	return ADPCM_BLOCK_SIZE * nChannels;
}

// Validates an ADPCM format structure.
bool CAkADPCMCodec::IsValidImaAdpcmFormat( WaveFormatEx & wfx )
{
    if(WAVE_FORMAT_ADPCM != wfx.wFormatTag)
    {
        return false;
    }
    if((wfx.nChannels < 1) || (wfx.nChannels > AK_VOICE_MAX_NUM_CHANNELS))
    {
        return false;
    }
    if(ADPCM_BITS_PER_SAMPLE != wfx.wBitsPerSample)
    {
        return false;
    }
    if(CalculateDecodeAlignment(wfx.nChannels) != wfx.nBlockAlign)
    {
        return false;
    }
	return true;
}

// Decode to 16 bit integer PCM
bool CAkADPCMCodec::Decode(	unsigned char * pbSrc,
							unsigned char * pbDst,
							AkUInt32 cBlocks,
							AkUInt32 nBlockAlignment,
							AkUInt32 nChannels )
{
	AKASSERT( !(ADPCM_SAMPLES_PER_BLOCK & 1 ) ); // this routines assume an even number of samples in a block.

	AkInt16 * psDst = reinterpret_cast<AkInt16*>( pbDst );

    // Enter the main loop   
    while(cBlocks--)
    {
		unsigned char *	pbBlock = pbSrc;

		// Block header
		int nPredSample = *(AkInt16 *)pbBlock;
		pbBlock += sizeof(AkInt16);

		int nStepIndex = *(unsigned char *)pbBlock;
		pbBlock += sizeof(AkInt16);

		// Write out first sample
        *psDst = (AkInt16)nPredSample;
		psDst += nChannels;

		AkUInt32 cPairs = (ADPCM_SAMPLES_PER_BLOCK - 1) / 2;

		int nEncSample;
		int	nStepSize;

        // Enter the block loop
		do
        {
            unsigned char bSample = *pbBlock++;

            // Sample 1
            nEncSample = (bSample & 0x0F);
            nStepSize = m_asStep[nStepIndex];
            nPredSample = DecodeSample(nEncSample, nPredSample, nStepSize);
            nStepIndex = NextStepIndex(nEncSample, nStepIndex);
            *psDst = (AkInt16)nPredSample;
			psDst += nChannels;

            // Sample 2
            nEncSample = (bSample >> 4);
            nStepSize = m_asStep[nStepIndex];
            nPredSample = DecodeSample(nEncSample, nPredSample, nStepSize);
            nStepIndex = NextStepIndex(nEncSample, nStepIndex);
            *psDst = (AkInt16)nPredSample;
			psDst += nChannels;
        }
        while(--cPairs);

        unsigned char bSample = *pbBlock++;

        // Sample 1
        nEncSample = (bSample & 0x0F);
        nStepSize = m_asStep[nStepIndex];
        nPredSample = DecodeSample(nEncSample, nPredSample, nStepSize);
        NextStepIndex(nEncSample, nStepIndex);
        *psDst = (AkInt16)nPredSample;
		psDst += nChannels;

        // Skip padding
        pbSrc += nBlockAlignment;
    }

    return true;
}

// Computes the next step index
int CAkADPCMCodec::NextStepIndex(int nEncodedSample, int nStepIndex)
{
	nStepIndex += m_asNextStep[nEncodedSample];
	if( nStepIndex < 0 )
	{
		nStepIndex = 0;
	}
	else if( nStepIndex >= NUMSTEPINDEXVAL )
	{
		nStepIndex = NUMSTEPINDVALMINUSONE;
	}
	return nStepIndex;
}

// Decodes an encoded sample.
int CAkADPCMCodec::DecodeSample( int nEncodedSample,
								 int nPredictedSample,
								 int nStepSize )
{
	// PERFORMANCE NOTE: on modern processors, the multiply method is faster than the branch/shift method.

/*	int iDifference = nStepSize >> 3;
	if(nEncodedSample & 4) 
		iDifference += nStepSize;
	if(nEncodedSample & 2) 
		iDifference += nStepSize >> 1;
	if(nEncodedSample & 1) 
		iDifference += nStepSize >> 2;
*/
	int iDifference = ((nEncodedSample & 7)*2+1)*nStepSize/8;

	if(nEncodedSample & 8)
		iDifference = -iDifference;

    int	iNewSample = nPredictedSample + iDifference;

	// Only clip if we need to
    if((AkInt32)(AkInt16)iNewSample != iNewSample)
    {
        if(iNewSample < -32768)
        {
            iNewSample = -32768;
        }
        else
        {
            iNewSample = 32767;
        }
    }

    return iNewSample;
}

