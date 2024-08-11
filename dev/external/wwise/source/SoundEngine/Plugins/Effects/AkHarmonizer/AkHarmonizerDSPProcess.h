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
 
#ifndef _AK_HARMONIZERDSPPROCESS_H_
#define _AK_HARMONIZERDSPPROCESS_H_

#include <AK/SoundEngine/Common/AkCommonDefs.h>
#include "AkHarmonizerFXInfo.h"

#ifndef __PPU__

void AkHarmonizerDSPProcessVoice( 
	AkReal32 * in_pfCurChan,
	AkHarmonizerFXInfo & io_FXInfo, 
	AkUInt32 in_uChannelIndex,
	AkUInt32 in_uVoiceIndex,
	AkReal32 * in_pfMonoPitchedVoice,
	AkReal32 * in_pfWet,
	AkUInt32 in_uNumFrames,
	bool	 in_bNoMoreData,
	AkReal32 in_fResamplingFactor,
	AkReal32 * in_pfPVTDWindow
#ifdef __SPU__
	, AkUInt8 * in_pScratchMem
	, AkUInt32 in_uDMATag
#endif
	);

void AkHarmonizerDSPProcess(	AkAudioBuffer * io_pBuffer, 
								AkHarmonizerFXInfo & io_FXInfo, 
								AkReal32 * in_pfTempStorage
#ifdef __SPU__
								, AkReal32 * in_pfScratchMem
								, AkUInt32 in_uDMATag
#endif
								);

#endif // #ifndef __PPU__

#endif // _AK_HARMONIZERDSPPROCESS_H_