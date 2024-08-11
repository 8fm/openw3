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
 
#ifndef _AK_STEREODELAYDSPPROCESS_H_
#define _AK_STEREODELAYDSPPROCESS_H_

#include <AK/SoundEngine/Common/AkCommonDefs.h>
#include "AkStereoDelayFXInfo.h"

void AkStereoDelayDSPProcess(	AkAudioBuffer * io_pBuffer, 
								AkStereoDelayFXInfo & io_FXInfo, 
								AkReal32 * in_pfStereoBufferStorage
#ifdef __SPU__
								, AkUInt32 in_uDMATag
#endif
								);

#endif // _AK_STEREODELAYDSPPROCESS_H_