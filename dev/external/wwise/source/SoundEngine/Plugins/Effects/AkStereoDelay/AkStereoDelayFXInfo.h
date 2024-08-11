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
 
#ifndef _AK_STEREODELAYFXINFO_H_
#define _AK_STEREODELAYFXINFO_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/Plugin/PluginServices/AkFXTailHandler.h>
#include "AkStereoDelayFXParams.h"
#include "AkStereoDelayLine.h"

#ifdef AK_71AUDIO
#define AK_NUM_STEREO_DELAY_LINES	(3)
#elif defined( AK_REARCHANNELS )
#define AK_NUM_STEREO_DELAY_LINES	(2)
#else
#define AK_NUM_STEREO_DELAY_LINES	(1)
#endif

struct AkStereoDelayFXInfo
{  	
	AK::DSP::CStereoDelayLine	StereoDelay[AK_NUM_STEREO_DELAY_LINES]; // Front (+ rear) (+sides)
	AkStereoDelayFXParams		Params;
	AkStereoDelayFXParams		PrevParams;
	AkFXTailHandler				FXTailHandler;	
	AkUInt32					uTailLength;
	AkUInt32					uSampleRate;
	AkUInt32					uMaxBufferSize;
	bool						bRecomputeFilterCoefs;
	bool						bSendMode;	
} AK_ALIGN_DMA;

#endif // _AK_STEREODELAYFXINFO_H_


