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
// AkADPCMCodec.h
//
// ADPCM decompressor codec (based on IMA ADPCM). 
// See ADPCM document on Sharepoint for details.
//
//////////////////////////////////////////////////////////////////////

#ifndef __AK_ADPCM_CODEC_H__
#define __AK_ADPCM_CODEC_H__

#include "AkFileParserBase.h" // for WaveFormatEx

#define ADPCM_SAMPLES_PER_BLOCK		(64)
#define ADPCM_BLOCK_SIZE			(36) // Per channel
#define ADPCM_WAVE_FORMAT			(0x0069)                                        
#define ADPCM_BITS_PER_SAMPLE       (4)

class CAkADPCMCodec
{
public:

	// Format descriptions verification
	static bool IsValidImaAdpcmFormat( WaveFormatEx & wfxFormat );

	// Data conversion functions
	static bool Decode(	unsigned char * pbSrc, 
							unsigned char * pbDst, 
							AkUInt32 cBlocks, 
							AkUInt32 nBlockAlignment,
							AkUInt32 nChannels );

private:
	CAkADPCMCodec(); // class should not be instantiated

	// Decoded data alignment
	static inline unsigned short CalculateDecodeAlignment( unsigned short nChannels );

	// Computes the next step index
	static AkForceInline int NextStepIndex( int nEncodedSample, int nStepIndex );
	// Decodes a single sample
	static AkForceInline int DecodeSample(	int nInputSample, 
										int nPredictedSample, 
										int nStepSize );

private:
	static const AkInt16	m_asNextStep[16];           // Step increment array
	static const AkInt16	m_asStep[89];               // Step value array
};

#endif // __AK_ADPCM_CODEC_H__

