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
 
#ifndef _AK_PITCHSHIFTERFXINFO_H_
#define _AK_PITCHSHIFTERFXINFO_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include "AkPitchShifterFXParams.h"
#include "MultiChannelBiquadFilter.h"
#include "DelayLineLight.h"
#include <AK/Plugin/PluginServices/AkFXTailHandler.h>
#include "AkDelayPitchShift.h"

struct AkPitchShifterFXInfo
{ 
	AK::DSP::AkDelayPitchShift			PitchShifter;
	AK::DSP::MultiChannelBiquadFilter<AK_VOICE_MAX_NUM_CHANNELS>	Filter;
	::DSP::CDelayLight					DryDelay[AK_VOICE_MAX_NUM_CHANNELS];
	AkFXTailHandler						FXTailHandler;	
	AkPitchShifterFXParams				Params;
	AkPitchShifterFXParams				PrevParams;
	AkUInt32							uNumProcessedChannels;
	AkChannelMask						eProcessChannelMask;
	AkUInt32							uTotalNumChannels;
	AkUInt32							uSampleRate;
	AkUInt32							uTailLength;
#ifdef AK_PS3
	AkUInt32							uSizeDelayMem;
#endif
	bool								bSendMode;

} AK_ALIGN_DMA;

#endif // _AK_PITCHSHIFTERFXINFO_H_


