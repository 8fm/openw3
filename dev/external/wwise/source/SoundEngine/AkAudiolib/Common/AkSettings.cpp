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

#include "stdafx.h"
#include "AkSettings.h"

#include "PlatformAudiolibDefs.h"

#ifdef AK_LOW_LATENCY
	#define PC_WAIT_TIME_DIVISOR (4.0f)
#else
	#define PC_WAIT_TIME_DIVISOR (2.0f)
#endif

namespace AkAudioLibSettings
{
	AkUInt32 g_pipelineCoreFrequency = DEFAULT_NATIVE_FREQUENCY ;
	AkTimeMs g_msPerBufferTick = (AkUInt32)( LE_MAX_FRAMES_PER_BUFFER / ( DEFAULT_NATIVE_FREQUENCY / 1000.0f ) );
	AkUInt32 g_pcWaitTime = ((AkUInt32)( 1000.0f * AK_NUM_VOICE_REFILL_FRAMES / DEFAULT_NATIVE_FREQUENCY / PC_WAIT_TIME_DIVISOR ));
	AkUInt32 g_uLpfUpdatePeriod = DEFAULT_LPF_UPDATE_PERIOD;

	void SetSampleFrequency( AkUInt32 in_uSampleFrequency )
	{
		g_pipelineCoreFrequency = in_uSampleFrequency;
		g_msPerBufferTick = (AkUInt32)( LE_MAX_FRAMES_PER_BUFFER / ( in_uSampleFrequency / 1000.0f ) );
		g_pcWaitTime = ((AkUInt32)( 1000.0f * AK_NUM_VOICE_REFILL_FRAMES / in_uSampleFrequency / PC_WAIT_TIME_DIVISOR ));
		g_uLpfUpdatePeriod = ( in_uSampleFrequency*DEFAULT_LPF_UPDATE_PERIOD) / DEFAULT_NATIVE_FREQUENCY;
	}
}
