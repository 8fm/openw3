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

#ifndef _INTERNALLPFSTATE_H_
#define _INTERNALLPFSTATE_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include "BiquadFilter.h"

#define AKLPFNUMMEMORIES			(4)
#define BYPASSMAXVAL 0.1f			// Filter will be bypassed for values lower than this

// Number of interpolated value between current and target 
static const AkUInt32 NUMBLOCKTOREACHTARGET = 8;

struct AkInternalLPFState
{
	DSP::BiquadFilterMulti m_LPF;
	AkReal32    fCurrentLPFPar;			
	AkReal32	fTargetLPFPar;			
	AkUInt16	uNumInterBlocks;
	AkUInt16	uChannelMask;
#ifdef AK_PS3	
	AkUInt32	uValidFrames;					
	AkUInt32	uMaxFrames;					
#endif // PS3
	bool		bComputeCoefsForFeedback;
	bool		bBypassFilter;
	bool        bTargetDirty; // fTargetLPFPar has changed since last execution
	bool		bFirstSetLPF; // Flags first set pitch received, not needed at execute time

	bool IsInterpolating()
	{
		return (uNumInterBlocks < NUMBLOCKTOREACHTARGET);
	}
} AK_ALIGN_DMA;

#endif // _INTERNALLPFSTATE_H_
