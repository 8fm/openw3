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
 
#ifndef _AK_FLANGERFXINFO_H_
#define _AK_FLANGERFXINFO_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/Plugin/PluginServices/AkFXTailHandler.h>
#include "LFOMultiChannel.h"

struct AkFlangerFXInfo
{  	
	AkFlangerFXParams			Params;					
	AkFlangerFXParams			PrevParams;			
	AkFXTailHandler				FXTailHandler;	
	AkUInt32					uNumProcessedChannels;
	AkUInt32					uSampleRate;
} AK_ALIGN_DMA;

struct FlangerOutputPolicy
{
	AkForceInline void OutputFromLFO(AkReal32 *pfBuf, AkReal32 fLFO, AkReal32 fAmp) { *pfBuf = fLFO; }
};

typedef DSP::MultiChannelLFO<DSP::Bipolar, FlangerOutputPolicy> FlangerLFO;

#endif // _AK_FLANGERFXINFO_H_


