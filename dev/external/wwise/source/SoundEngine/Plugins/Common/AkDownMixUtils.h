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

#ifndef _AKDOWNMIXUTILS_H_
#define _AKDOWNMIXUTILS_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include "SPUInline.h"
class AkAudioBuffer;

namespace DSP
{	
	// Implement constant power mixdown from any channel configuration down to stereo.
	// Currently setup so that output channel configuration is assumed to be stereo unless the input channel configuration
	// is less than stereo in which case it simply copies the result to a mono output buffer
	// Center and LFE channel levels are adjustable but others are taken full scale.
	// All levels are interpolated for RTPC usage.
	// Constant power implies sum_i=1_N(ChanGain^2) = 1
	// E.g. using 5.1 configuration (CG = Constant power channel gain)
	// CG^2 (L) + CG^2 (R) + (CenterLevel*CG)^2 (C) + (LFELevel*CG)^2 (LFE) + CG^2 (LS) + CG^2 (RS)
	SPUStatic void AkDownMix(	AkAudioBuffer * in_pBuffer, AkAudioBuffer * out_pDownMixBuffer, 
					AkReal32 in_fPrevInputCenterLevel, AkReal32 in_fInputCenterLevel, 
					AkReal32 in_fPrevInputLFELevel , AkReal32 in_fInputLFELevel );



} // namespace DSP

#endif // _AKDOWNMIXUTILS_H_
