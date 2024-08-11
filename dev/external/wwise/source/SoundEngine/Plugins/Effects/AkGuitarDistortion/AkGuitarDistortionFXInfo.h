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
 
#ifndef _AK_GUITARDISTORTIONFXINFO_H_
#define _AK_GUITARDISTORTIONFXINFO_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include "AkMultiBandEQ.h"
#include "AkDistortion.h"
#include "AkRectifier.h"
#include "AkGuitarDistortionFXParams.h"

struct AkGuitarDistotionFXInfo
{  	
	DSP::CAkMultiBandEQ			PreEQ;
	DSP::CAkMultiBandEQ			PostEQ;
	DSP::CAkDistortion			Distortion;
	DSP::CAkRectifier			Rectifier;
	AkGuitarDistortionFXParams	Params;
	AkUInt32					uNumChannels;
	AkUInt32					uSampleRate;
	AkReal32					fCurrentGain;	
	AkReal32					fCurrentWetDryMix;
} AK_ALIGN_DMA;

#endif // _AK_GUITARDISTORTIONFXINFO_H_


