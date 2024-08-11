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

//////////////////////////////////////////////////////////////////////
//
// PlatformAudiolibDefs.h
//
// AkAudioLib Internal defines
//
//////////////////////////////////////////////////////////////////////
#ifndef _PLATFORM_AUDIOLIB_DEFS_H_
#define _PLATFORM_AUDIOLIB_DEFS_H_

#include <AK/SoundEngine/Common/AkSoundEngine.h>

//----------------------------------------------------------------------------------------------------
// Audio file format
//----------------------------------------------------------------------------------------------------
// Little-Endian platforms RIFF ID
#define AkPlatformRiffChunkId RIFFChunkId

//----------------------------------------------------------------------------------------------------
// Voice manager
//----------------------------------------------------------------------------------------------------
#ifdef AK_LOW_LATENCY
#define AK_NUM_VOICE_REFILL_FRAMES				(256)
#else
#define AK_NUM_VOICE_REFILL_FRAMES				(1024)
#endif

#define AK_MIN_NUM_REFILLS_IN_VOICE_BUFFER		(2)
#define AK_DEFAULT_NUM_REFILLS_IN_VOICE_BUFFER	(4)

#define LE_MAX_FRAMES_PER_BUFFER				(AK_NUM_VOICE_REFILL_FRAMES)

//----------------------------------------------------------------------------------------------------
// Bank manager platform-specific
//----------------------------------------------------------------------------------------------------
#define AK_BANK_MGR_THREAD_STACK_BYTES			(AK_DEFAULT_STACK_SIZE)
#define AK_THREAD_BANK_MANAGER_PRIORITY			(AK_THREAD_PRIORITY_NORMAL)

#endif //_PLATFORM_AUDIOLIB_DEFS_H_
