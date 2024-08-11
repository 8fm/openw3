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
 
#ifndef _AK_TIMESTRETCHFXINFO_H_
#define _AK_TIMESTRETCHFXINFO_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include "AkTimeStretchFXParams.h"

//#define USE_SIMPLE_RESAMPLER 

#ifdef USE_SIMPLE_RESAMPLER
#include "LinearResampler.h"
#else
#include "AkPhaseVocoder.h"
#endif

struct AkTimeStretchFXInfo
{  	
	AkTimeStretchFXParams		Params;
	AkTimeStretchFXParams		PrevParams;
	AkUInt32					uNumChannels;
	AkUInt32					uSampleRate;
	AkReal32					fTSRandomOffset;

#ifdef USE_SIMPLE_RESAMPLER
	DSP::CAkLinearResampler		LinearResampler;
#else
	DSP::BUTTERFLYSET_NAMESPACE::CAkPhaseVocoder PhaseVocoder;
	AkUInt8						uNoTSModeCount;
	bool						bCanEnterNoTSMode;		
#endif
} AK_ALIGN_DMA;

#endif // _AK_TIMESTRETCHFXINFO_H_


