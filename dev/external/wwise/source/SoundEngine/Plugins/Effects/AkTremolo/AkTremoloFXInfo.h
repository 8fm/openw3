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
 
#ifndef _AK_TREMOLOFXINFO_H_
#define _AK_TREMOLOFXINFO_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include "LFOMultiChannel.h"

struct AkTremoloFXInfo
{  	
	AkTremoloFXParams			Params;					
	AkTremoloFXParams			PrevParams;
	AkUInt32					uSampleRate;
} AK_ALIGN_DMA;

struct TremoloOutputPolicy
{
	AkForceInline void OutputFromLFO(AkReal32 *pfBuf, AkReal32 fLFO, AkReal32 fAmp) 
	{ 
		fGain += fGainInc;
		*pfBuf *= fGain * ( (1-fAmp) + fLFO ); 
	}

	AkReal32 fGain;
	AkReal32 fGainInc;
};

typedef DSP::MultiChannelLFO<DSP::Unipolar, TremoloOutputPolicy> TremoloLFO;

#endif // _AK_TREMOLOFXINFO_H_


