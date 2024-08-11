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

#ifndef _AK_LPF_COMMON_H_
#define _AK_LPF_COMMON_H_

#include "AkInternalLPFState.h"
#include <AK/SoundEngine/Common/AkCommonDefs.h>
#include "AkSettings.h"

AkReal32 EvalLPFCutoff( AkReal32 in_fCurrentLPFPar, bool in_bIsForFeedbackPipeline );
AkForceInline bool EvalLPFBypass( AkReal32 in_fCurrentLPFPar )
{
	return in_fCurrentLPFPar <= BYPASSMAXVAL;
}

AkForceInline void ComputeLPFCoefs(DSP::BiquadFilterMulti & in_rFilter, AkReal32 fCutFreq)
{
	AkReal32 PiFcOSr			= PI * AkMin( 0.5f * 0.9f, fCutFreq / AK_CORE_SAMPLERATE );
	AkReal32 fIntVal			= 1.f/tanf(PiFcOSr);
	AkReal32 fRootTwoxIntVal	= ROOTTWO * fIntVal;
	AkReal32 fSqIntVal			= fIntVal * fIntVal;

	AkReal32 fB0 = 1.0f / ( 1.0f + fRootTwoxIntVal + fSqIntVal);
	in_rFilter.SetCoefs(fB0, fB0+fB0, fB0, 2.0f * ( 1.0f - fSqIntVal) *fB0, ( 1.0f - fRootTwoxIntVal + fSqIntVal) * fB0);
}

#endif // _AK_LPF_COMMON_H_
