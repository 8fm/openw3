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
 
#ifndef _AK_HARMONIZERFXINFO_H_
#define _AK_HARMONIZERFXINFO_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include "AkHarmonizerFXParams.h"
#include "AkResamplingPhaseVocoder.h"
#include "MultiChannelBiquadFilter.h"
#include "DelayLineLight.h"
#include <AK/Plugin/PluginServices/AkFXTailHandler.h>

struct AkHarmonizerFXInfo
{  	
	AkHarmonizerFXInfo() :uNumProcessedChannels( 0 ), uTotalNumChannels( 0 ), eProcessChannelMask( 0 ) {}

	DSP::BUTTERFLYSET_NAMESPACE::CAkResamplingPhaseVocoder			PhaseVocoder[AKHARMONIZER_NUMVOICES];
	AK::DSP::MultiChannelBiquadFilter<AK_VOICE_MAX_NUM_CHANNELS>	Filter[AKHARMONIZER_NUMVOICES];
	::DSP::CDelayLight												DryDelay[AK_VOICE_MAX_NUM_CHANNELS];
	AkHarmonizerFXParams											Params;
	AkHarmonizerFXParams											PrevParams;
	AkFXTailHandler													FXTailHandler;	
	AkUInt32														uNumProcessedChannels;
	AkChannelMask													eProcessChannelMask;
	AkUInt32														uTotalNumChannels;
	AkUInt32														uSampleRate;
	bool															bWetPathEnabled;
	bool															bSendMode;
} AK_ALIGN_DMA;

#endif // _AK_HARMONIZERFXINFO_H_


