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

#pragma once

#include <ajm.h>

const AkUInt16 AKAT9_RINGBUFFER_SAMPLES = 6*1024; // 4 times 1024 samples, will be allocated * numChannels

#define SamplesToOutputBytes( _a ) _a * uNumChannels * sizeof(AkUInt16)
#define SampleToBufferIndex( _a ) _a * uNumChannels

struct sAkDecodeResult
{		
	SceAjmSidebandResult	sResult;
	SceAjmSidebandStream	sStream;
	SceAjmSidebandFormat	sFormat;
	SceAjmSidebandMFrame	sMFrame;
};

class CAkSkipDecodedFrames
{
public:
	CAkSkipDecodedFrames();
	
	AkUInt32 			SetSkipFrames(AkUInt32 in_uSkipStartIndex, AkUInt32 in_uSkipEndIndex, bool in_bReadBeforeSkip);
	void				Reset();

	AkUInt32			uSkipStartBufferSamples;
	AkUInt32			uSkipSamplesLength;
	AkUInt8				bReadBeforeSkip:1;
};

class CAkDecodingRingbuffer
{
public:
	CAkDecodingRingbuffer();
	
	bool				Create(AkUInt32 in_size);
	void				Destroy();
	void				Reset();

	AkUInt32			SkipFramesIfNeeded(const AkUInt32 in_uReadLength);

	AkUInt16*			m_pData;
	AkUInt32			m_WriteSampleIndex;
	AkUInt32			m_ReadSampleIndex;
	AkUInt32			m_SamplesBeingConsumed;
	AkUInt32			m_RingbufferUsedSample;

	CAkSkipDecodedFrames m_SkipFramesStart;
	CAkSkipDecodedFrames m_SkipFrames;
};