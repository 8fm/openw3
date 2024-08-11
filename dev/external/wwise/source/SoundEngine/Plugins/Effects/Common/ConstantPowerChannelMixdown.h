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

// Implement constant power mixdown of a number of input channel.
// Center and LFE channel are adjustable but others are taken full scale.
// Center and LFE channel gains are not changing at runtime and thus do not require interpolation.
// Constant power implies sum_i=1_N(ChanGain^2) = 1
// E.g. using 5.1 configuration (CG = Constant power channel gain)
// CG^2 (L) + CG^2 (R) + (CenterLevel*CG)^2 (C) + (LFELevel*CG)^2 (LFE) + CG^2 (LS) + CG^2 (RS)

#ifndef _AKCONSTANTPOWERCHANNELMIXDOWN_H_
#define _AKCONSTANTPOWERCHANNELMIXDOWN_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include "SPUInline.h"
class AkAudioBuffer;

namespace DSP
{		
	SPUStatic void ConstantPowerChannelMixdown(	
		AkAudioBuffer * in_pAudioBufferIn,
		AkUInt32 in_uNumFrames,
		AkUInt32 in_uFrameOffset,
		AkReal32 * out_pfBufferOut, 
		AkChannelMask in_uChannelMask, 
		AkReal32 in_fInCenterGain,
		AkReal32 in_fInLFEGain );

} // namespace DSP

#endif // _AKCONSTANTPOWERCHANNELMIXDOWN_H_