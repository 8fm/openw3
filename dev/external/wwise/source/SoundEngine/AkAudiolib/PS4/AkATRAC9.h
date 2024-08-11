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

#include "AkFileParserBase.h"

// Wwise/ATRAC9-specific WaveFormatExtensible format (actually
// a mix of things from the ATRAC9 fmt chunk and ATRAC9
// fact chunk)
struct AkATRAC9WaveFormatExtensible
	: public WaveFormatExtensible
{
	AkUInt8 atrac9_configData[4];						// ATRAC9 setting information
	AkUInt32 atrac9_totalSamplesPerChannel;				// Total samples per channel
	AkUInt16 atrac9_delaySamplesInputOverlapEncoder;	// Number of extra samples to decode before getting valid output from the decoder. This number of extra samples is added by the encoder at the beginning of the data. They should be decoded and discarded.
};

#define AK_ATRAC9_OUTPUT_SAMPLE_RATE		48000
#define AK_ATRAC9_FULLPLUGINID				CAkEffectsMgr::GetMergedID( AkPluginTypeCodec, AKCOMPANYID_AUDIOKINETIC, AKCODECID_ATRAC9 )

#define AK_ATRAC9_OUTPUT_BITS_PER_SAMPLE	16
#define AK_ATRAC9_OUTPUT_BYTES_PER_SAMPLE	(AK_ATRAC9_OUTPUT_BITS_PER_SAMPLE/8)

